/**
 * @file h264_encoder.c
 * @brief H.264 하드웨어 인코더 래퍼 모듈 구현
 */

#include "h264_encoder.h"
#include "esp_h264_enc_single_hw.h"
#include "esp_h264_alloc.h"  // esp_h264_aligned_calloc을 위한 헤더
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "h264_enc";

struct h264_encoder_t {
    esp_h264_enc_handle_t hw_encoder;
    h264_encoder_config_t config;

    // 출력 버퍼
    uint8_t *out_buffer;
    uint32_t out_buffer_size;

    // 포맷 변환 버퍼 (I420 → O_UYY_E_VYY)
    uint8_t *convert_buffer;
    uint32_t convert_buffer_size;

    // SPS/PPS 저장
    uint8_t *sps_data;
    uint32_t sps_size;
    uint8_t *pps_data;
    uint32_t pps_size;
    bool headers_ready;

    // 프레임 카운터
    uint32_t frame_count;
};

/**
 * @brief I420 (YUV420 planar) 포맷을 O_UYY_E_VYY (packed) 포맷으로 변환
 *
 * I420 포맷:
 *   Y 평면: width x height
 *   U 평면: (width/2) x (height/2)
 *   V 평면: (width/2) x (height/2)
 *
 * O_UYY_E_VYY 포맷:
 *   라인 1 (홀수): u y y u y y u y y...
 *   라인 2 (짝수): v y y v y y v y y...
 *   라인 3 (홀수): u y y u y y u y y...
 *   ...
 *
 * @param width 프레임 너비
 * @param height 프레임 높이
 * @param i420_data I420 입력 데이터
 * @param out_data O_UYY_E_VYY 출력 데이터
 */
static void convert_i420_to_o_uyy_e_vyy(uint32_t width, uint32_t height,
                                         const uint8_t *i420_data,
                                         uint8_t *out_data)
{
    const uint8_t *y_plane = i420_data;
    const uint8_t *u_plane = y_plane + (width * height);
    const uint8_t *v_plane = u_plane + (width * height / 4);

    uint8_t *out = out_data;

    // Process 2 lines at a time (U line, then V line)
    // Pattern follows ESP official test code (h264_io.c:read_enc_cb_420)
    // 16-pixel blocks with specific U/Y ordering for hardware MacroBlock processing
    for (uint32_t row = 0; row < height; row += 2) {
        // First line (U line) - process in 16-pixel blocks
        const uint8_t *y_row1 = y_plane + (row * width);
        const uint8_t *u_row_base = u_plane + ((row / 2) * (width / 2));

        for (uint32_t col = 0; col < width; col += 16) {
            // 16 pixels = 8 U values + 16 Y values = 24 bytes
            // Pattern from ESP test: u y y u y y u y / y u y y u y y u / y y u y y u y / y u
            // Important: U/V are subsampled 2:1 horizontally, so col/2 gives U/V column index
            const uint8_t *u_row = u_row_base + (col / 2);

            *out++ = u_row[0]; *out++ = y_row1[col + 0]; *out++ = y_row1[col + 1];   // u y y
            *out++ = u_row[1]; *out++ = y_row1[col + 2]; *out++ = y_row1[col + 3];   // u y y
            *out++ = u_row[2]; *out++ = y_row1[col + 4]; *out++ = y_row1[col + 5];   // u y y

            *out++ = y_row1[col + 6]; *out++ = u_row[3]; *out++ = y_row1[col + 7];   // y u y
            *out++ = y_row1[col + 8]; *out++ = u_row[4]; *out++ = y_row1[col + 9];   // y u y
            *out++ = u_row[5]; *out++ = y_row1[col + 10]; *out++ = y_row1[col + 11]; // u y y

            *out++ = y_row1[col + 12]; *out++ = y_row1[col + 13]; *out++ = u_row[6]; // y y u
            *out++ = y_row1[col + 14]; *out++ = y_row1[col + 15]; *out++ = u_row[7]; // y y u
        }

        // Second line (V line) - same pattern with V
        const uint8_t *y_row2 = y_plane + ((row + 1) * width);
        const uint8_t *v_row_base = v_plane + ((row / 2) * (width / 2));

        for (uint32_t col = 0; col < width; col += 16) {
            const uint8_t *v_row = v_row_base + (col / 2);

            *out++ = v_row[0]; *out++ = y_row2[col + 0]; *out++ = y_row2[col + 1];   // v y y
            *out++ = v_row[1]; *out++ = y_row2[col + 2]; *out++ = y_row2[col + 3];   // v y y
            *out++ = v_row[2]; *out++ = y_row2[col + 4]; *out++ = y_row2[col + 5];   // v y y

            *out++ = y_row2[col + 6]; *out++ = v_row[3]; *out++ = y_row2[col + 7];   // y v y
            *out++ = y_row2[col + 8]; *out++ = v_row[4]; *out++ = y_row2[col + 9];   // y v y
            *out++ = v_row[5]; *out++ = y_row2[col + 10]; *out++ = y_row2[col + 11]; // v y y

            *out++ = y_row2[col + 12]; *out++ = y_row2[col + 13]; *out++ = v_row[6]; // y y v
            *out++ = y_row2[col + 14]; *out++ = y_row2[col + 15]; *out++ = v_row[7]; // y y v
        }
    }
}

esp_err_t h264_encoder_init(const h264_encoder_config_t *config, h264_encoder_handle_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->width == 0 || config->height == 0 || config->fps == 0) {
        ESP_LOGE(TAG, "Invalid encoder config: width=%u, height=%u, fps=%u",
                 (unsigned)config->width, (unsigned)config->height, (unsigned)config->fps);
        return ESP_ERR_INVALID_ARG;
    }

    // 인코더 구조체 할당
    h264_encoder_handle_t encoder = calloc(1, sizeof(struct h264_encoder_t));
    if (!encoder) {
        ESP_LOGE(TAG, "Failed to allocate encoder structure");
        return ESP_ERR_NO_MEM;
    }

    memcpy(&encoder->config, config, sizeof(h264_encoder_config_t));
    encoder->headers_ready = false;
    encoder->frame_count = 0;

    // 출력 버퍼 할당
    // H.264 하드웨어 인코더는 16바이트 정렬된 버퍼를 요구함
    // esp_h264_aligned_calloc을 사용하여 정렬된 메모리 할당
    //
    // 주의: H.264 인코더는 최악의 경우 압축이 거의 안 될 수 있음
    // 실제 카메라 데이터는 첫 I-frame에서 YUV 크기와 비슷하거나 클 수 있음
    // YUV420 크기의 1.5배로 할당 (여유 확보)
    uint32_t yuv_size = config->width * config->height * 3 / 2;  // YUV420 전체 크기
    uint32_t requested_size = yuv_size + (yuv_size / 2);  // YUV420 크기의 1.5배 (안전 마진)
    uint32_t actual_size = 0;

    ESP_LOGI(TAG, "[DEBUG] Allocating H.264 output buffer:");
    ESP_LOGI(TAG, "[DEBUG]   Resolution: %ux%u", (unsigned)config->width, (unsigned)config->height);
    ESP_LOGI(TAG, "[DEBUG]   YUV420 size: %u bytes (%.2f MB)",
             (unsigned)yuv_size, yuv_size / (1024.0f * 1024.0f));
    ESP_LOGI(TAG, "[DEBUG]   Requested size: %u bytes (%.2f MB) [1.5x safety margin]",
             (unsigned)requested_size,
             requested_size / (1024.0f * 1024.0f));
    ESP_LOGI(TAG, "[DEBUG]   Allocating from: SPIRAM with 16-byte alignment");

    encoder->out_buffer = esp_h264_aligned_calloc(16, 1, requested_size, &actual_size, ESP_H264_MEM_SPIRAM);
    if (!encoder->out_buffer) {
        ESP_LOGE(TAG, "Failed to allocate aligned output buffer (requested: %.2f MB)",
                 requested_size / (1024.0f * 1024.0f));
        ESP_LOGE(TAG, "[DEBUG] Available SPIRAM: %u bytes (%.2f MB)",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024.0f * 1024.0f));
        free(encoder);
        return ESP_ERR_NO_MEM;
    }

    encoder->out_buffer_size = actual_size;  // 정렬 후 실제 할당된 크기 사용
    ESP_LOGI(TAG, "[DEBUG] Output buffer allocated successfully:");
    ESP_LOGI(TAG, "[DEBUG]   Address: %p", encoder->out_buffer);
    ESP_LOGI(TAG, "[DEBUG]   Actual size: %u bytes (%.2f MB)",
             (unsigned)actual_size,
             actual_size / (1024.0f * 1024.0f));

    // 포맷 변환 버퍼 할당 (I420 → O_UYY_E_VYY)
    // 입력과 동일한 크기 필요
    encoder->convert_buffer_size = yuv_size;
    encoder->convert_buffer = esp_h264_aligned_calloc(16, 1, encoder->convert_buffer_size,
                                                       &actual_size, ESP_H264_MEM_SPIRAM);
    if (!encoder->convert_buffer) {
        ESP_LOGE(TAG, "Failed to allocate format conversion buffer (%.2f MB)",
                 encoder->convert_buffer_size / (1024.0f * 1024.0f));
        esp_h264_free(encoder->out_buffer);
        free(encoder);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "[DEBUG] Format conversion buffer allocated: %u bytes (%.2f MB)",
             (unsigned)encoder->convert_buffer_size,
             encoder->convert_buffer_size / (1024.0f * 1024.0f));

    // H.264 하드웨어 인코더 설정
    esp_h264_enc_cfg_hw_t hw_cfg = {
        .pic_type = ESP_H264_RAW_FMT_O_UYY_E_VYY,  // 하드웨어 인코더 지원 포맷
        .gop = (config->gop_size > 0) ? config->gop_size : config->fps,  // 기본값: 1초당 1 I-frame
        .fps = config->fps,
        .res = {
            .width = config->width,
            .height = config->height,
        },
        .rc = {
            .bitrate = (config->bitrate > 0) ? config->bitrate : (config->width * config->height * 3 / 10),  // 기본 비트레이트
            .qp_min = 35,  // 최소 품질 파라미터 (10→35: 더 높은 압축률, 2Mbps 타겟)
            .qp_max = 51,  // 최대 품질 파라미터
        },
    };

    esp_h264_err_t ret = esp_h264_enc_hw_new(&hw_cfg, &encoder->hw_encoder);
    if (ret != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "Failed to create H.264 hardware encoder: %d", ret);
        heap_caps_free(encoder->out_buffer);
        free(encoder);
        return ESP_FAIL;
    }

    // 인코더 열기
    ret = esp_h264_enc_open(encoder->hw_encoder);
    if (ret != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "Failed to open H.264 encoder: %d", ret);
        esp_h264_enc_del(encoder->hw_encoder);
        heap_caps_free(encoder->out_buffer);
        free(encoder);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "H.264 encoder initialized: %ux%u @ %ufps, GOP=%u, bitrate=%u",
             (unsigned)config->width, (unsigned)config->height,
             (unsigned)config->fps, (unsigned)hw_cfg.gop,
             (unsigned)hw_cfg.rc.bitrate);

    *out_handle = encoder;
    return ESP_OK;
}

esp_err_t h264_encoder_encode(h264_encoder_handle_t handle,
                               const uint8_t *yuv420_data,
                               uint32_t yuv420_size,
                               uint64_t pts,
                               h264_encoded_frame_t *out_frame)
{
    if (!handle || !yuv420_data || !out_frame) {
        return ESP_ERR_INVALID_ARG;
    }

    // YUV420 (I420) → O_UYY_E_VYY 변환
    // ESP32-P4 H.264 인코더가 요구하는 포맷으로 변환
    convert_i420_to_o_uyy_e_vyy(handle->config.width, handle->config.height,
                                 yuv420_data, handle->convert_buffer);

    // 변환된 데이터로 입력 프레임 설정
    esp_h264_enc_in_frame_t in_frame = {
        .raw_data = {
            .buffer = handle->convert_buffer,
            .len = handle->convert_buffer_size,
        },
        .pts = (uint32_t)pts,  // uint64_t → uint32_t 변환
    };

    // 출력 프레임 설정
    esp_h264_enc_out_frame_t enc_frame = {
        .raw_data = {
            .buffer = handle->out_buffer,
            .len = handle->out_buffer_size,
        },
    };

    // 첫 프레임에 대한 상세 디버깅
    if (handle->frame_count == 0) {
        ESP_LOGI(TAG, "[DEBUG] First frame encoding:");
        ESP_LOGI(TAG, "[DEBUG]   Original I420 buffer: %p, size: %u bytes", yuv420_data, (unsigned)yuv420_size);
        ESP_LOGI(TAG, "[DEBUG]   Original I420 sample (first 32 bytes):");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, yuv420_data, 32, ESP_LOG_INFO);

        ESP_LOGI(TAG, "[DEBUG]   Converted O_UYY_E_VYY buffer: %p, size: %u bytes",
                 in_frame.raw_data.buffer, (unsigned)in_frame.raw_data.len);
        ESP_LOGI(TAG, "[DEBUG]   Converted data sample (first 32 bytes):");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, in_frame.raw_data.buffer, 32, ESP_LOG_INFO);

        ESP_LOGI(TAG, "[DEBUG]   Input buffer alignment: %p mod 16 = %d",
                 in_frame.raw_data.buffer, ((uintptr_t)in_frame.raw_data.buffer) % 16);
        ESP_LOGI(TAG, "[DEBUG]   Output buffer: %p, size: %u bytes (%.2f MB)",
                 enc_frame.raw_data.buffer, (unsigned)enc_frame.raw_data.len,
                 enc_frame.raw_data.len / (1024.0f * 1024.0f));
        ESP_LOGI(TAG, "[DEBUG]   Output buffer alignment: %p mod 16 = %d",
                 enc_frame.raw_data.buffer, ((uintptr_t)enc_frame.raw_data.buffer) % 16);
        ESP_LOGI(TAG, "[DEBUG]   PTS: %llu", (unsigned long long)pts);
        ESP_LOGI(TAG, "[DEBUG]   Encoder config: %ux%u, GOP=%u, FPS=%u",
                 (unsigned)handle->config.width, (unsigned)handle->config.height,
                 (unsigned)handle->config.gop_size, (unsigned)handle->config.fps);
    }

    // 인코딩 수행
    ESP_LOGI(TAG, "[DEBUG] Calling esp_h264_enc_process...");
    esp_h264_err_t ret = esp_h264_enc_process(handle->hw_encoder, &in_frame, &enc_frame);
    ESP_LOGI(TAG, "[DEBUG] esp_h264_enc_process returned: %d", ret);

    if (ret != ESP_H264_ERR_OK) {
        ESP_LOGW(TAG, "Frame %u encoding failed: %d",
                 (unsigned)handle->frame_count, ret);

        // 실패 시 상세 정보 출력
        ESP_LOGE(TAG, "[DEBUG] Encoding failed!");
        ESP_LOGE(TAG, "[DEBUG]   Error code: %d (%s)", ret,
                 ret == -1 ? "FAIL" :
                 ret == -2 ? "ARG" :
                 ret == -3 ? "MEM" :
                 ret == -5 ? "UNSUPPORTED" :
                 ret == -6 ? "TIMEOUT" :
                 ret == -7 ? "OVERFLOW" : "UNKNOWN");
        ESP_LOGE(TAG, "[DEBUG]   Frame count: %u", (unsigned)handle->frame_count);
        ESP_LOGE(TAG, "[DEBUG]   Input buffer: %p, size: %u bytes (%.2f MB)",
                 in_frame.raw_data.buffer, (unsigned)in_frame.raw_data.len,
                 in_frame.raw_data.len / (1024.0f * 1024.0f));
        ESP_LOGE(TAG, "[DEBUG]   Output buffer: %p, size: %u bytes (%.2f MB)",
                 enc_frame.raw_data.buffer, (unsigned)enc_frame.raw_data.len,
                 enc_frame.raw_data.len / (1024.0f * 1024.0f));
        ESP_LOGE(TAG, "[DEBUG]   Output buffer before: len=%u, after: len=%u, actual length=%u",
                 (unsigned)handle->out_buffer_size, (unsigned)enc_frame.raw_data.len,
                 (unsigned)enc_frame.length);
        ESP_LOGE(TAG, "[DEBUG]   Expected I-frame size: ~300KB-600KB");

        // 메모리 상태 확인
        ESP_LOGE(TAG, "[DEBUG] Memory status:");
        ESP_LOGE(TAG, "[DEBUG]   Free heap (internal): %u bytes (%.2f KB)",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024.0f);
        ESP_LOGE(TAG, "[DEBUG]   Free heap (SPIRAM): %u bytes (%.2f MB)",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024.0f * 1024.0f));

        return ESP_FAIL;
    }

    // 결과 설정
    out_frame->data = enc_frame.raw_data.buffer;
    out_frame->size = enc_frame.length;  // 실제 인코딩된 데이터 길이
    out_frame->is_keyframe = (enc_frame.frame_type == ESP_H264_FRAME_TYPE_IDR ||
                               enc_frame.frame_type == ESP_H264_FRAME_TYPE_I);
    out_frame->pts = pts;

    // NAL 타입 파싱 (간단히 키프레임 여부로 설정)
    out_frame->type = out_frame->is_keyframe ? H264_NAL_IDR : H264_NAL_SLICE;

    // 첫 3개 프레임의 출력 데이터 분석 (NAL 구조 확인)
    if (handle->frame_count < 3) {
        float compression_ratio = (float)in_frame.raw_data.len / (float)enc_frame.length;
        ESP_LOGI(TAG, "═══ Frame #%u Encoded ═══", (unsigned)handle->frame_count);
        ESP_LOGI(TAG, "  Size: %u bytes (%.2f KB)", (unsigned)enc_frame.length, enc_frame.length / 1024.0f);
        ESP_LOGI(TAG, "  Compression: %.2fx", compression_ratio);
        ESP_LOGI(TAG, "  Type: %s", out_frame->is_keyframe ? "I-frame (IDR)" : "P-frame");

        // 첫 32 바이트 덤프 (NAL 구조 확인)
        ESP_LOGI(TAG, "  First 32 bytes of encoded data:");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, enc_frame.raw_data.buffer,
                                  enc_frame.length < 32 ? enc_frame.length : 32,
                                  ESP_LOG_INFO);

        // Start code 확인
        const uint8_t *data = enc_frame.raw_data.buffer;
        if (enc_frame.length >= 4) {
            if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
                ESP_LOGI(TAG, "  ✓ Contains 4-byte start code (0x00000001)");
            } else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
                ESP_LOGI(TAG, "  ✓ Contains 3-byte start code (0x000001)");
            } else {
                ESP_LOGI(TAG, "  ✗ NO start code detected (raw NAL?)");
                ESP_LOGI(TAG, "  First byte: 0x%02X (NAL type: %u)", data[0], data[0] & 0x1F);
            }
        }
    }

    handle->frame_count++;

    // 주기적으로 인코딩 통계 로그
    if (handle->frame_count % 100 == 0) {
        float compression_ratio = (float)yuv420_size / (float)enc_frame.length;
        ESP_LOGI(TAG, "Encoded %u frames (last: %u bytes, %.1fx compression, type=%s)",
                 (unsigned)handle->frame_count, (unsigned)enc_frame.length,
                 compression_ratio, out_frame->is_keyframe ? "KEY" : "P");
    }

    return ESP_OK;
}

esp_err_t h264_encoder_get_headers(h264_encoder_handle_t handle,
                                    const uint8_t **out_sps,
                                    uint32_t *out_sps_size,
                                    const uint8_t **out_pps,
                                    uint32_t *out_pps_size)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // ESP H.264 하드웨어 인코더는 IDR 프레임에 SPS/PPS를 자동으로 포함
    // 별도로 추출할 필요 없음
    ESP_LOGW(TAG, "SPS/PPS extraction not implemented (auto-included in IDR frames)");
    return ESP_ERR_NOT_SUPPORTED;
}

void h264_encoder_deinit(h264_encoder_handle_t handle)
{
    if (!handle) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing H.264 encoder (total frames: %u)",
             (unsigned)handle->frame_count);

    if (handle->hw_encoder) {
        esp_h264_enc_close(handle->hw_encoder);
        esp_h264_enc_del(handle->hw_encoder);
    }

    if (handle->out_buffer) {
        esp_h264_free(handle->out_buffer);  // esp_h264_aligned_calloc으로 할당했으므로 esp_h264_free 사용
    }

    if (handle->convert_buffer) {
        esp_h264_free(handle->convert_buffer);  // 변환 버퍼도 esp_h264_aligned_calloc으로 할당
    }

    if (handle->sps_data) {
        free(handle->sps_data);
    }

    if (handle->pps_data) {
        free(handle->pps_data);
    }

    free(handle);
}
