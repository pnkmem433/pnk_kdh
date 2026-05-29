/*
 * ESP32-P4 10초 동영상 녹화 프로젝트
 * MIPI_CSI 카메라 → MJPEG 인코딩 → SD카드 저장
 */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_video_init.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_cache_private.h"
#include "esp_timer.h"
#include "app_video.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include "sd_card.h"
#include "h264_encoder.h"
#include "h264_multi_encoder.h"
#include "h264_spool.h"

#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))

#define GPIO_OUTPUT_IO_20    20
#define GPIO_OUTPUT_IO_26    26
#define GPIO_OUTPUT_IO_27    27
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_20) | (1ULL<<GPIO_OUTPUT_IO_26)| (1ULL<<GPIO_OUTPUT_IO_27))

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len, void *user_data);

static const char *TAG = "app_main";

static size_t data_cache_line_size = 0;
static h264_multi_encoder_handle_t s_h264_multi_encoder = NULL;  // 멀티 인코더로 변경
static h264_spool_handle_t s_h264_spool = NULL;
static uint32_t s_recorded_frames = 0;
static uint64_t s_record_start_us = 0;
static uint32_t s_actual_fps = 20;  // 1280x960 @ 20fps (OV5647 네이티브 지원)
static bool s_recording_started = false;
static bool s_video_saved = false;

#ifndef MJPEG_QUALITY
#define MJPEG_QUALITY 30
#endif

#if CONFIG_EXAMPLE_ENABLE_PRINT_FPS_RATE_VALUE
static int fps_count;
static int64_t start_time;
#endif

void gpio_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_20, 1);
    gpio_set_level(GPIO_OUTPUT_IO_26, 0);
    gpio_set_level(GPIO_OUTPUT_IO_27, 0);
}

void app_main(void)
{
    gpio_init();

    ESP_ERROR_CHECK(esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size));

    // 1. SD카드 초기화
    ESP_LOGI(TAG, "=== SD카드 초기화 ===");
    esp_err_t sd_ret = sd_card_init();
    if (sd_ret != ESP_OK) {
        ESP_LOGE(TAG, "SD카드 초기화 실패: %s", esp_err_to_name(sd_ret));
        ESP_LOGE(TAG, "SD카드를 확인하고 다시 시도하세요.");
        return;
    } else {
        ESP_LOGI(TAG, "SD카드 초기화 완료");
    }

    // 2. 카메라 초기화
    ESP_LOGI(TAG, "=== 카메라 초기화 ===");
    ESP_LOGI(TAG, "I2C 핀 설정: SCL=GPIO%d, SDA=GPIO%d",
             CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN,
             CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN);

    esp_err_t ret = app_video_main(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "카메라 초기화 실패: 0x%x (%s)", ret, esp_err_to_name(ret));
        ESP_LOGE(TAG, "카메라 연결 및 전원을 확인하세요");
        return;
    }
    ESP_LOGI(TAG, "카메라 드라이버 초기화 완료");

    // Open the video device
    ESP_LOGI(TAG, "카메라 장치 열기: %s", EXAMPLE_CAM_DEV_PATH);
    int video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT_YUV420);
    if (video_cam_fd0 < 0) {
        ESP_LOGE(TAG, "카메라 장치 열기 실패");
        ESP_LOGE(TAG, "카메라가 올바르게 연결되어 있는지 확인하세요");
        return;
    }

    ESP_LOGI(TAG, "카메라 초기화 완료 (1280x960 @ 20fps, YUV420)");

	gpio_set_level(GPIO_OUTPUT_IO_20, 1);

    // 카메라 버퍼 할당
    #define CAM_BUF_COUNT 2
    void *camera_buf[CAM_BUF_COUNT];
    for (int i = 0; i < CAM_BUF_COUNT; i++) {
        camera_buf[i] = heap_caps_aligned_calloc(data_cache_line_size, 1, app_video_get_buf_size(), MALLOC_CAP_SPIRAM);
        if (!camera_buf[i]) {
            ESP_LOGE(TAG, "카메라 버퍼 %d 할당 실패", i);
            for (int j = 0; j < i; j++) {
                heap_caps_free(camera_buf[j]);
            }
            return;
        }
    }
    ESP_ERROR_CHECK(app_video_set_bufs(video_cam_fd0, CAM_BUF_COUNT, (void *)camera_buf));

    // Register the video frame operation callback
    ESP_ERROR_CHECK(app_video_register_frame_operation_cb(camera_video_frame_operation));

    // Start the camera stream task
    ESP_ERROR_CHECK(app_video_stream_task_start(video_cam_fd0, 0, NULL));

    ESP_LOGI(TAG, "카메라 스트리밍 시작");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== 잠시 후 동영상 녹화를 시작합니다 (1280x960 @ 20fps, 5초) ===");

#if CONFIG_EXAMPLE_ENABLE_PRINT_FPS_RATE_VALUE
    start_time = esp_timer_get_time();
#endif
}

// Progress callback for video save
static void video_save_progress_cb(uint32_t current, uint32_t total, void *user_data)
{
    static uint32_t last_percent = 0;
    uint32_t percent = (current * 100) / total;

    // 1% 단위로 로그 출력
    if (percent > last_percent) {
        ESP_LOGI(TAG, "동영상 녹화 후 저장중 %lu%%", (unsigned long)percent);
        last_percent = percent;
    }
}

// 멀티 인코더 완료 콜백 (인코딩된 프레임을 스풀에 추가)
static void multi_encoder_callback(const h264_encoded_frame_t *frame, void *user_data)
{
    if (!frame || !s_h264_spool) {
        return;
    }

    // 스풀에 추가
    if (h264_spool_add_frame(s_h264_spool, frame) == ESP_OK) {
        s_recorded_frames++;

        // 1% 단위로 진행률 출력
        static uint32_t last_progress = 0;
        uint32_t progress = (s_recorded_frames * 100) / 100; // 100 프레임 기준 (5초 @ 20fps)
        if (progress > last_progress && progress <= 100) {
            ESP_LOGI(TAG, "동영상 녹화중 %lu%% (%lu/100 프레임)", (unsigned long)progress, (unsigned long)s_recorded_frames);
            last_progress = progress;
        }
    }
}

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len, void *user_data)
{
#if CONFIG_EXAMPLE_ENABLE_PRINT_FPS_RATE_VALUE
    fps_count++;
    if (fps_count == 50) {
        int64_t end_time = esp_timer_get_time();
        ESP_LOGI(TAG, "fps: %f", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;
    }
#endif

    static uint32_t s_frame_counter = 0;
    s_frame_counter++;

    // 녹화 시작 (프레임 10 이후, 1회만)
    if (!s_recording_started && !s_video_saved && s_frame_counter >= 10 && sd_card_is_mounted()) {
        // H.264 멀티 인코더 초기화
        // ESP32-P4 H.264 하드웨어 제약: 인터럽트 1개만 사용 가능 → 인코더 1개만 가능
        // 해결: 인코더 1개 사용, 성능 개선은 다른 방법 활용 (해상도 감소, GOP 증가 등)
        h264_multi_encoder_config_t enc_config = {
            .width = camera_buf_hes,
            .height = camera_buf_ves,
            .fps = 20,  // 1280x960 @ 20fps (OV5647 네이티브 지원)
            .gop_size = 20,  // 1초당 1 I-frame
            .bitrate = 800000,  // 800 Kbps (1280x960 최적화)
            .num_encoders = 1,  // ESP32-P4 제약: 하드웨어 인코더 1개만 사용 가능
        };

        if (h264_multi_encoder_init(&enc_config, multi_encoder_callback, NULL, &s_h264_multi_encoder) != ESP_OK) {
            ESP_LOGW(TAG, "H.264 멀티 인코더 초기화 실패");
            return;
        }

        // H.264 스풀 시작
        if (h264_spool_start(camera_buf_hes, camera_buf_ves, 20, &s_h264_spool) != ESP_OK) {
            ESP_LOGW(TAG, "H.264 스풀 시작 실패");
            h264_multi_encoder_deinit(s_h264_multi_encoder);
            s_h264_multi_encoder = NULL;
            return;
        }

        s_recorded_frames = 0;
        s_record_start_us = esp_timer_get_time();
        s_recording_started = true;

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== 동영상 녹화시작 ===");
        ESP_LOGI(TAG, "해상도: %ux%u @ 20fps, 포맷: H.264 (하드웨어 인코더 1개)", (unsigned)camera_buf_hes, (unsigned)camera_buf_ves);
        ESP_LOGI(TAG, "[DEBUG] Camera buffer: camera_buf_len=%u bytes (%.2f MB)",
                 (unsigned)camera_buf_len, camera_buf_len / (1024.0f * 1024.0f));
        ESP_LOGI(TAG, "[DEBUG] YUV420 size: %u bytes (%.2f MB)",
                 (unsigned)(camera_buf_hes * camera_buf_ves * 3 / 2),
                 (camera_buf_hes * camera_buf_ves * 3 / 2) / (1024.0f * 1024.0f));
    }

    // 녹화 중 - YUV420 → H.264 비동기 인코딩 (멀티 인코더)
    if (s_recording_started && s_h264_multi_encoder && s_h264_spool && !s_video_saved) {
        // Cache sync
        uintptr_t buf_addr2 = (uintptr_t)camera_buf;
        uintptr_t aligned_start2 = buf_addr2 & ~((uintptr_t)data_cache_line_size - 1);
        size_t align_head2 = buf_addr2 - aligned_start2;
        size_t msync_len2 = camera_buf_len + align_head2;
        msync_len2 = (msync_len2 + data_cache_line_size - 1) & ~((size_t)data_cache_line_size - 1);
        (void)esp_cache_msync((void *)aligned_start2, msync_len2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

        uint64_t pts = esp_timer_get_time();

        // H.264 비동기 인코딩 (멀티 인코더에 작업 제출)
        // 주의: camera_buf_len은 캐시 정렬된 크기 (4147200 bytes)
        // 실제 YUV420 크기는 width * height * 3 / 2 (3110400 bytes for 1920x1080)
        uint32_t yuv420_size = camera_buf_hes * camera_buf_ves * 3 / 2;

        // 프레임 인덱스를 기준으로 적절한 인코더에 분배
        static uint32_t encode_frame_index = 0;
        h264_multi_encoder_encode_async(s_h264_multi_encoder, camera_buf, yuv420_size,
                                        pts, encode_frame_index);
        encode_frame_index++;

        uint64_t now = esp_timer_get_time();
        uint64_t recording_duration = now - s_record_start_us;

        // 100 프레임 캡처 완료 (카메라 캡처 기준, 5초 @ 20fps)
        // 멀티 인코더는 비동기로 계속 처리 중
        if (encode_frame_index >= 100) {
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "=== 100 프레임 캡처 완료, 인코딩 완료 대기 중... ===");

            // 멀티 인코더 종료 및 모든 작업 완료 대기
            ESP_LOGI(TAG, "모든 인코딩 작업 완료 대기 (타임아웃: 60초)...");
            h264_multi_encoder_wait_all(s_h264_multi_encoder, 60000);

            // 통계 확인
            uint32_t total, completed, dropped;
            h264_multi_encoder_get_stats(s_h264_multi_encoder, &total, &completed, &dropped);

            // 실제 FPS 계산
            float actual_fps_float = (s_recorded_frames * 1000000.0f) / recording_duration;
            s_actual_fps = (uint32_t)(actual_fps_float + 0.5f);

            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "=== 녹화 완료 ===");
            ESP_LOGI(TAG, "캡처 프레임: 100개 (5초 @ 20fps, 720p)");
            ESP_LOGI(TAG, "인코딩 요청: %u개, 완료: %u개, 드롭: %u개",
                     (unsigned)total, (unsigned)completed, (unsigned)dropped);
            ESP_LOGI(TAG, "스풀 저장: %" PRIu32 "개 프레임", s_recorded_frames);
            ESP_LOGI(TAG, "실제 FPS: %.2f", actual_fps_float);

            // FPS 업데이트
            h264_spool_set_fps(s_h264_spool, s_actual_fps);

            // H.264 파일 저장
            // 파일명: .h264 확장자는 FAT32에서 errno 22 발생
            // 해결: .mp4 확장자 사용 (H.264 raw stream은 .mp4 컨테이너에도 사용 가능)
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "=== 동영상 파일 저장 중 ===");
            ESP_LOGI(TAG, "파일명: /sdcard/video001.mp4");
            ESP_LOGI(TAG, "동영상 녹화 후 저장중 0%%");

            (void)h264_spool_flush_to_file(s_h264_spool, "/sdcard/video001.mp4", video_save_progress_cb, NULL);

            h264_spool_stop(s_h264_spool);
            s_h264_spool = NULL;

            h264_multi_encoder_deinit(s_h264_multi_encoder);
            s_h264_multi_encoder = NULL;

            s_video_saved = true;

            ESP_LOGI(TAG, "동영상 녹화 후 저장중 100%%");
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "=== 저장완료 ===");
            ESP_LOGI(TAG, "파일: /sdcard/video001.mp4");
            ESP_LOGI(TAG, "");
        }
    }
}
