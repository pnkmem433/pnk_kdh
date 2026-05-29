/**
 * @file mp4_muxer.c
 * @brief 간단한 MP4 muxer 구현
 *
 * 최소 기능 MP4 컨테이너 생성 (H.264 비디오 트랙만 지원)
 * ISO/IEC 14496-12 기반 간소화 구현
 */

#include "mp4_muxer.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "mp4_mux";

/**
 * @brief MP4 muxer 구조체
 */
struct mp4_muxer_t {
    FILE *fp;
    mp4_muxer_config_t config;

    // SPS/PPS 저장 (첫 NAL에서 추출)
    uint8_t *sps;
    uint32_t sps_size;
    uint8_t *pps;
    uint32_t pps_size;

    // NAL unit 샘플 정보
    uint32_t *sample_sizes;    ///< 각 샘플(NAL) 크기
    uint32_t sample_count;     ///< 총 샘플 수
    uint32_t sample_capacity;  ///< 샘플 배열 용량

    // mdat 시작 위치
    uint64_t mdat_pos;
};

/**
 * @brief MP4 box 헤더 쓰기
 */
static void write_box_header(FILE *fp, uint32_t size, const char *type)
{
    fwrite(&size, 4, 1, fp);
    fwrite(type, 1, 4, fp);
}

/**
 * @brief uint32 빅엔디안 쓰기
 */
static inline void write_be32(FILE *fp, uint32_t v)
{
    uint8_t b[4] = {
        (v >> 24) & 0xFF,
        (v >> 16) & 0xFF,
        (v >> 8) & 0xFF,
        v & 0xFF
    };
    fwrite(b, 1, 4, fp);
}

/**
 * @brief uint16 빅엔디안 쓰기
 */
static inline void write_be16(FILE *fp, uint16_t v)
{
    uint8_t b[2] = { (v >> 8) & 0xFF, v & 0xFF };
    fwrite(b, 1, 2, fp);
}

esp_err_t mp4_muxer_create(const char *output_path,
                            const mp4_muxer_config_t *config,
                            mp4_muxer_handle_t *out_handle)
{
    if (!output_path || !config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    mp4_muxer_handle_t muxer = calloc(1, sizeof(struct mp4_muxer_t));
    if (!muxer) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(&muxer->config, config, sizeof(mp4_muxer_config_t));

    muxer->fp = fopen(output_path, "wb");
    if (!muxer->fp) {
        ESP_LOGE(TAG, "Failed to open MP4 file: %s", output_path);
        free(muxer);
        return ESP_FAIL;
    }

    // 샘플 배열 초기화
    muxer->sample_capacity = 1024;
    muxer->sample_sizes = malloc(sizeof(uint32_t) * muxer->sample_capacity);
    if (!muxer->sample_sizes) {
        fclose(muxer->fp);
        free(muxer);
        return ESP_ERR_NO_MEM;
    }

    muxer->sample_count = 0;
    muxer->sps = NULL;
    muxer->pps = NULL;

    // ftyp box 쓰기 (파일 타입)
    write_box_header(muxer->fp, 28, "ftyp");
    fwrite("isom", 1, 4, muxer->fp);  // major brand
    write_be32(muxer->fp, 512);      // minor version
    fwrite("isomiso2avc1mp41", 1, 16, muxer->fp);  // compatible brands

    // mdat box 시작 (나중에 크기 패치)
    muxer->mdat_pos = ftell(muxer->fp);
    write_box_header(muxer->fp, 0, "mdat");  // 크기 0 (나중에 패치)

    ESP_LOGI(TAG, "MP4 muxer created: %ux%u @ %ufps",
             (unsigned)config->width, (unsigned)config->height, (unsigned)config->fps);

    *out_handle = muxer;
    return ESP_OK;
}

/**
 * @brief 단일 NAL unit 처리 (내부 헬퍼)
 */
static esp_err_t process_nal_unit(mp4_muxer_handle_t handle,
                                    const uint8_t *nal_data,
                                    uint32_t nal_size)
{
    if (nal_size == 0) {
        return ESP_OK;
    }

    uint8_t nal_type = nal_data[0] & 0x1F;

    // SPS 저장
    if (nal_type == 7) {
        if (handle->sps) free(handle->sps);
        handle->sps_size = nal_size;
        handle->sps = malloc(handle->sps_size);
        if (handle->sps) {
            memcpy(handle->sps, nal_data, handle->sps_size);
            ESP_LOGI(TAG, "✓ SPS captured (%u bytes)", (unsigned)handle->sps_size);
        }
        return ESP_OK;
    }

    // PPS 저장
    if (nal_type == 8) {
        if (handle->pps) free(handle->pps);
        handle->pps_size = nal_size;
        handle->pps = malloc(handle->pps_size);
        if (handle->pps) {
            memcpy(handle->pps, nal_data, handle->pps_size);
            ESP_LOGI(TAG, "✓ PPS captured (%u bytes)", (unsigned)handle->pps_size);
        }
        return ESP_OK;
    }

    // 비디오 프레임 → mdat에 쓰기 (AVCC format: size + data)
    write_be32(handle->fp, nal_size);
    fwrite(nal_data, 1, nal_size, handle->fp);

    // 샘플 크기 기록
    if (handle->sample_count >= handle->sample_capacity) {
        handle->sample_capacity *= 2;
        handle->sample_sizes = realloc(handle->sample_sizes,
                                        sizeof(uint32_t) * handle->sample_capacity);
    }

    handle->sample_sizes[handle->sample_count] = nal_size + 4;
    handle->sample_count++;

    return ESP_OK;
}

esp_err_t mp4_muxer_add_nal(mp4_muxer_handle_t handle,
                             const uint8_t *nal_data,
                             uint32_t nal_size)
{
    if (!handle || !nal_data || !nal_size) {
        return ESP_ERR_INVALID_ARG;
    }

    // ESP32-P4 H.264 인코더는 IDR 프레임에 SPS+PPS+IDR을 함께 출력
    // 여러 NAL unit을 start code로 구분하여 파싱
    const uint8_t *ptr = nal_data;
    uint32_t remaining = nal_size;
    uint32_t nal_count = 0;

    while (remaining > 0) {
        // Start code 찾기 (0x00 00 00 01 또는 0x00 00 01)
        uint32_t sc_size = 0;
        if (remaining >= 4 && ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 1) {
            sc_size = 4;
        } else if (remaining >= 3 && ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1) {
            sc_size = 3;
        }

        ptr += sc_size;
        remaining -= sc_size;

        if (remaining == 0) break;

        // 다음 start code 찾기
        uint32_t nal_len = 0;
        for (uint32_t i = 0; i + 2 < remaining; i++) {
            if (ptr[i] == 0 && ptr[i+1] == 0) {
                if ((i + 2 < remaining && ptr[i+2] == 1) ||
                    (i + 3 < remaining && ptr[i+2] == 0 && ptr[i+3] == 1)) {
                    nal_len = i;
                    break;
                }
            }
        }

        // 마지막 NAL unit
        if (nal_len == 0) {
            nal_len = remaining;
        }

        // NAL unit 처리
        process_nal_unit(handle, ptr, nal_len);
        nal_count++;

        ptr += nal_len;
        remaining -= nal_len;
    }

    return ESP_OK;
}

/**
 * @brief moov box 생성 (메타데이터)
 */
static void write_moov_box(FILE *fp, mp4_muxer_handle_t handle)
{
    uint64_t moov_start = ftell(fp);

    // moov box 헤더 (크기는 나중에 패치)
    write_be32(fp, 0);  // placeholder
    fwrite("moov", 1, 4, fp);

    // mvhd (movie header)
    write_be32(fp, 108);  // box size
    fwrite("mvhd", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 0);  // creation_time
    write_be32(fp, 0);  // modification_time
    write_be32(fp, handle->config.fps);  // timescale
    write_be32(fp, handle->sample_count);  // duration
    write_be32(fp, 0x00010000);  // rate (1.0)
    write_be16(fp, 0x0100);  // volume (1.0)
    write_be16(fp, 0);  // reserved
    write_be32(fp, 0);  // reserved
    write_be32(fp, 0);  // reserved

    // matrix (identity)
    write_be32(fp, 0x00010000); write_be32(fp, 0); write_be32(fp, 0);
    write_be32(fp, 0); write_be32(fp, 0x00010000); write_be32(fp, 0);
    write_be32(fp, 0); write_be32(fp, 0); write_be32(fp, 0x40000000);

    // pre-defined
    for (int i = 0; i < 6; i++) write_be32(fp, 0);

    write_be32(fp, 2);  // next_track_ID

    // trak box (video track)
    uint64_t trak_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("trak", 1, 4, fp);

    // tkhd (track header)
    write_be32(fp, 92);  // box size
    fwrite("tkhd", 1, 4, fp);
    write_be32(fp, 0x00000007);  // version + flags (track enabled, in movie, in preview)
    write_be32(fp, 0);  // creation_time
    write_be32(fp, 0);  // modification_time
    write_be32(fp, 1);  // track_ID
    write_be32(fp, 0);  // reserved
    write_be32(fp, handle->sample_count);  // duration
    write_be32(fp, 0);  // reserved
    write_be32(fp, 0);  // reserved
    write_be16(fp, 0);  // layer
    write_be16(fp, 0);  // alternate_group
    write_be16(fp, 0);  // volume
    write_be16(fp, 0);  // reserved

    // matrix
    write_be32(fp, 0x00010000); write_be32(fp, 0); write_be32(fp, 0);
    write_be32(fp, 0); write_be32(fp, 0x00010000); write_be32(fp, 0);
    write_be32(fp, 0); write_be32(fp, 0); write_be32(fp, 0x40000000);

    write_be32(fp, handle->config.width << 16);  // width
    write_be32(fp, handle->config.height << 16);  // height

    // mdia box (media)
    uint64_t mdia_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("mdia", 1, 4, fp);

    // mdhd (media header)
    write_be32(fp, 32);  // box size
    fwrite("mdhd", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 0);  // creation_time
    write_be32(fp, 0);  // modification_time
    write_be32(fp, handle->config.fps);  // timescale
    write_be32(fp, handle->sample_count);  // duration
    write_be16(fp, 0x55c4);  // language (und)
    write_be16(fp, 0);  // pre-defined

    // hdlr (handler)
    write_be32(fp, 45);  // box size
    fwrite("hdlr", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 0);  // pre-defined
    fwrite("vide", 1, 4, fp);  // handler_type
    write_be32(fp, 0);  // reserved
    write_be32(fp, 0);  // reserved
    write_be32(fp, 0);  // reserved
    fwrite("VideoHandler\0", 1, 13, fp);  // name

    // minf box (media information)
    uint64_t minf_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("minf", 1, 4, fp);

    // vmhd (video media header)
    write_be32(fp, 20);  // box size
    fwrite("vmhd", 1, 4, fp);
    write_be32(fp, 1);  // version + flags
    write_be16(fp, 0);  // graphicsmode
    write_be16(fp, 0);  // opcolor[0]
    write_be16(fp, 0);  // opcolor[1]
    write_be16(fp, 0);  // opcolor[2]

    // dinf box (data information)
    write_be32(fp, 36);  // box size
    fwrite("dinf", 1, 4, fp);
    write_be32(fp, 28);  // dref box size
    fwrite("dref", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 1);  // entry_count
    write_be32(fp, 12);  // url box size
    fwrite("url ", 1, 4, fp);
    write_be32(fp, 1);  // version + flags (self-contained)

    // stbl box (sample table)
    uint64_t stbl_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("stbl", 1, 4, fp);

    // stsd (sample description)
    uint64_t stsd_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("stsd", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 1);  // entry_count

    // avc1 sample entry
    uint64_t avc1_start = ftell(fp);
    write_be32(fp, 0);  // placeholder
    fwrite("avc1", 1, 4, fp);
    write_be32(fp, 0);  // reserved
    write_be16(fp, 0);  // reserved
    write_be16(fp, 1);  // data_reference_index
    write_be16(fp, 0);  // pre-defined
    write_be16(fp, 0);  // reserved
    for (int i = 0; i < 3; i++) write_be32(fp, 0);  // pre-defined
    write_be16(fp, handle->config.width);  // width
    write_be16(fp, handle->config.height);  // height
    write_be32(fp, 0x00480000);  // horizresolution (72 dpi)
    write_be32(fp, 0x00480000);  // vertresolution (72 dpi)
    write_be32(fp, 0);  // reserved
    write_be16(fp, 1);  // frame_count

    // compressorname (32 bytes)
    for (int i = 0; i < 32; i++) fwrite("\0", 1, 1, fp);

    write_be16(fp, 0x0018);  // depth
    write_be16(fp, 0xFFFF);  // pre-defined

    // avcC box (AVC decoder configuration)
    uint32_t avcc_size = 8 + 5 + 3 + handle->sps_size + 3 + handle->pps_size;
    write_be32(fp, avcc_size);
    fwrite("avcC", 1, 4, fp);
    fwrite("\x01", 1, 1, fp);  // configurationVersion

    // AVCProfileIndication, profile_compatibility, AVCLevelIndication
    uint8_t zero_byte = 0x00;
    if (handle->sps && handle->sps_size >= 4) {
        fwrite(&handle->sps[1], 1, 1, fp);  // AVCProfileIndication
        fwrite(&handle->sps[2], 1, 1, fp);  // profile_compatibility
        fwrite(&handle->sps[3], 1, 1, fp);  // AVCLevelIndication
    } else {
        fwrite(&zero_byte, 1, 1, fp);  // AVCProfileIndication
        fwrite(&zero_byte, 1, 1, fp);  // profile_compatibility
        fwrite(&zero_byte, 1, 1, fp);  // AVCLevelIndication
    }

    fwrite("\xFF", 1, 1, fp);  // lengthSizeMinusOne (4 bytes)
    fwrite("\xE1", 1, 1, fp);  // numOfSequenceParameterSets (1)
    write_be16(fp, handle->sps_size);
    if (handle->sps) fwrite(handle->sps, 1, handle->sps_size, fp);
    fwrite("\x01", 1, 1, fp);  // numOfPictureParameterSets (1)
    write_be16(fp, handle->pps_size);
    if (handle->pps) fwrite(handle->pps, 1, handle->pps_size, fp);

    // Patch avc1 size
    uint64_t avc1_end = ftell(fp);
    fseek(fp, avc1_start, SEEK_SET);
    write_be32(fp, avc1_end - avc1_start);
    fseek(fp, avc1_end, SEEK_SET);

    // Patch stsd size
    uint64_t stsd_end = ftell(fp);
    fseek(fp, stsd_start, SEEK_SET);
    write_be32(fp, stsd_end - stsd_start);
    fseek(fp, stsd_end, SEEK_SET);

    // stts (time-to-sample)
    write_be32(fp, 24);  // box size
    fwrite("stts", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 1);  // entry_count
    write_be32(fp, handle->sample_count);  // sample_count
    write_be32(fp, 1);  // sample_delta

    // stsc (sample-to-chunk)
    write_be32(fp, 28);  // box size
    fwrite("stsc", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 1);  // entry_count
    write_be32(fp, 1);  // first_chunk
    write_be32(fp, handle->sample_count);  // samples_per_chunk
    write_be32(fp, 1);  // sample_description_index

    // stsz (sample sizes)
    write_be32(fp, 20 + handle->sample_count * 4);  // box size
    fwrite("stsz", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 0);  // sample_size (0 = variable)
    write_be32(fp, handle->sample_count);  // sample_count
    for (uint32_t i = 0; i < handle->sample_count; i++) {
        write_be32(fp, handle->sample_sizes[i]);
    }

    // stco (chunk offsets)
    write_be32(fp, 20);  // box size
    fwrite("stco", 1, 4, fp);
    write_be32(fp, 0);  // version + flags
    write_be32(fp, 1);  // entry_count
    write_be32(fp, (uint32_t)(handle->mdat_pos + 8));  // chunk_offset

    // Patch stbl size
    uint64_t stbl_end = ftell(fp);
    fseek(fp, stbl_start, SEEK_SET);
    write_be32(fp, stbl_end - stbl_start);
    fseek(fp, stbl_end, SEEK_SET);

    // Patch minf size
    uint64_t minf_end = ftell(fp);
    fseek(fp, minf_start, SEEK_SET);
    write_be32(fp, minf_end - minf_start);
    fseek(fp, minf_end, SEEK_SET);

    // Patch mdia size
    uint64_t mdia_end = ftell(fp);
    fseek(fp, mdia_start, SEEK_SET);
    write_be32(fp, mdia_end - mdia_start);
    fseek(fp, mdia_end, SEEK_SET);

    // Patch trak size
    uint64_t trak_end = ftell(fp);
    fseek(fp, trak_start, SEEK_SET);
    write_be32(fp, trak_end - trak_start);
    fseek(fp, trak_end, SEEK_SET);

    // Patch moov size
    uint64_t moov_end = ftell(fp);
    fseek(fp, moov_start, SEEK_SET);
    write_be32(fp, moov_end - moov_start);
    fseek(fp, moov_end, SEEK_SET);
}

esp_err_t mp4_muxer_finalize(mp4_muxer_handle_t handle)
{
    if (!handle || !handle->fp) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Finalizing MP4: %u samples", (unsigned)handle->sample_count);

    if (!handle->sps || !handle->pps) {
        ESP_LOGE(TAG, "Missing SPS/PPS - cannot create valid MP4");
        ESP_LOGE(TAG, "  SPS: %s (%u bytes)", handle->sps ? "present" : "MISSING", (unsigned)handle->sps_size);
        ESP_LOGE(TAG, "  PPS: %s (%u bytes)", handle->pps ? "present" : "MISSING", (unsigned)handle->pps_size);
        ESP_LOGE(TAG, "Hint: ESP32-P4 H.264 encoder outputs SPS+PPS+IDR in first frame");
        ESP_LOGE(TAG, "Check if encoder is outputting Annex B format (start codes)");
        return ESP_FAIL;
    }

    // mdat 크기 패치
    uint64_t mdat_end = ftell(handle->fp);
    uint64_t mdat_size = mdat_end - handle->mdat_pos;

    fseek(handle->fp, handle->mdat_pos, SEEK_SET);
    write_be32(handle->fp, (uint32_t)mdat_size);
    fseek(handle->fp, mdat_end, SEEK_SET);

    // moov box 생성
    write_moov_box(handle->fp, handle);

    // 파일 닫기
    fclose(handle->fp);
    handle->fp = NULL;

    ESP_LOGI(TAG, "MP4 file finalized successfully");
    return ESP_OK;
}

void mp4_muxer_destroy(mp4_muxer_handle_t handle)
{
    if (!handle) {
        return;
    }

    if (handle->fp) {
        fclose(handle->fp);
    }

    if (handle->sps) {
        free(handle->sps);
    }

    if (handle->pps) {
        free(handle->pps);
    }

    if (handle->sample_sizes) {
        free(handle->sample_sizes);
    }

    free(handle);
}
