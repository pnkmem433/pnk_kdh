/**
 * @file h264_spool.c
 * @brief H.264 스풀링 모듈 구현
 */

#include "h264_spool.h"
#include "mp4_muxer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

static const char *TAG = "h264_spool";

/**
 * @brief 링버퍼 내 프레임 항목
 */
typedef struct {
    uint8_t *data;           ///< NAL unit 데이터 (동적 할당)
    uint32_t size;           ///< NAL unit 크기
    h264_nal_type_t type;    ///< NAL unit 타입
    uint64_t pts;            ///< Presentation timestamp
} h264_frame_entry_t;

/**
 * @brief H.264 스풀 구조체
 */
struct h264_spool_t {
    uint32_t width, height, fps;

    // 링버퍼 (고정 크기 배열)
    h264_frame_entry_t *ring_buffer;
    uint32_t ring_capacity;   ///< 링버퍼 용량 (프레임 수)
    uint32_t write_idx;       ///< 쓰기 인덱스
    uint32_t read_idx;        ///< 읽기 인덱스
    volatile uint32_t count;  ///< 현재 저장된 프레임 수

    // SD 카드 임시 파일
    FILE *temp_fp;
    char temp_path[64];

    // 통계
    volatile uint32_t total_frames;
    volatile uint32_t dropped_frames;
    volatile uint32_t written_frames;

    // 멀티스레딩
    SemaphoreHandle_t mutex;
    TaskHandle_t writer_task;
    volatile bool stop_writer;
    TaskHandle_t flush_waiter;
};

/**
 * @brief SD writer 백그라운드 태스크 (비활성화됨)
 *
 * 이전 전략: 메모리에 프레임 축적 → 10 프레임마다 배치 쓰기
 * 현재 전략: 완전 메모리 버퍼링 (SD 실시간 쓰기 병목 제거)
 * 이유: SD 실시간 쓰기로 인한 FPS 저하 (20fps → 2fps) 해결
 *
 * 이 함수는 더 이상 사용되지 않음 (h264_spool_start에서 비활성화)
 */
__attribute__((unused)) static void sd_writer_task(void *arg)
{
    h264_spool_handle_t spool = (h264_spool_handle_t)arg;
    ESP_LOGI(TAG, "SD writer task started (chunked batch write mode)");

    uint64_t last_log_us = esp_timer_get_time();
    uint32_t last_written = 0;
    uint32_t chunk_size = 10;  // 10 프레임 단위 배치 쓰기 (메모리 압박 완화)

    while (!spool->stop_writer || spool->read_idx != spool->write_idx) {
        // 사용 가능한 프레임 계산
        uint32_t available_frames = 0;

        xSemaphoreTake(spool->mutex, portMAX_DELAY);
        if (spool->write_idx >= spool->read_idx) {
            available_frames = spool->write_idx - spool->read_idx;
        } else {
            available_frames = spool->ring_capacity - spool->read_idx + spool->write_idx;
        }
        xSemaphoreGive(spool->mutex);

        // 프레임이 없으면 대기
        if (available_frames == 0) {
            if (spool->stop_writer) {
                break;  // 녹화 종료 및 버퍼 비었으면 종료
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // 배치 쓰기 시작 (청크 크기 또는 사용 가능한 모든 프레임)
        uint32_t frames_to_write = (available_frames > chunk_size) ? chunk_size : available_frames;
        ESP_LOGI(TAG, "Writing chunk: %" PRIu32 " frames (buffer: %" PRIu32 "/%" PRIu32 ")",
                 frames_to_write, available_frames, spool->ring_capacity);

        for (uint32_t i = 0; i < frames_to_write; i++) {
            xSemaphoreTake(spool->mutex, portMAX_DELAY);

            // 프레임 읽기
            h264_frame_entry_t *entry = &spool->ring_buffer[spool->read_idx];
            uint8_t *data = entry->data;
            uint32_t size = entry->size;

            spool->read_idx = (spool->read_idx + 1) % spool->ring_capacity;
            spool->count--;

            xSemaphoreGive(spool->mutex);

            // SD에 쓰기
            if (data && size > 0) {
                // NAL 크기 기록 (4 바이트)
                fwrite(&size, sizeof(uint32_t), 1, spool->temp_fp);
                // NAL 데이터
                fwrite(data, 1, size, spool->temp_fp);

                spool->written_frames++;

                // 프레임 데이터 해제 (배치 쓰기 직후)
                heap_caps_free(data);
                entry->data = NULL;
            }
        }

        // 청크 쓰기 완료 후 플러시
        fflush(spool->temp_fp);

        // 쓰기 속도 로깅 (10초마다)
        uint64_t now_us = esp_timer_get_time();
        if (now_us - last_log_us >= 10000000ULL) {
            float fps = (spool->written_frames - last_written) / 10.0f;
            ESP_LOGI(TAG, "SD write: %u frames (%.1f fps, buffer: %u/%u)",
                     (unsigned)spool->written_frames, fps,
                     (unsigned)spool->count, (unsigned)spool->ring_capacity);
            last_log_us = now_us;
            last_written = spool->written_frames;
        }
    }

    ESP_LOGI(TAG, "Writer task finishing: wrote %u frames", (unsigned)spool->written_frames);

    // 파일 닫기
    fflush(spool->temp_fp);
    fclose(spool->temp_fp);
    spool->temp_fp = NULL;

    // 대기자 통지
    if (spool->flush_waiter) {
        xTaskNotifyGive(spool->flush_waiter);
        spool->flush_waiter = NULL;
    }

    vTaskDelete(NULL);
}

esp_err_t h264_spool_start(uint32_t width, uint32_t height, uint32_t fps,
                            h264_spool_handle_t *out_handle)
{
    if (!width || !height || !fps || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    h264_spool_handle_t spool = calloc(1, sizeof(struct h264_spool_t));
    if (!spool) {
        return ESP_ERR_NO_MEM;
    }

    spool->width = width;
    spool->height = height;
    spool->fps = fps;
    spool->write_idx = 0;
    spool->read_idx = 0;
    spool->count = 0;
    spool->total_frames = 0;
    spool->dropped_frames = 0;
    spool->written_frames = 0;
    spool->stop_writer = false;
    spool->flush_waiter = NULL;

    // 링버퍼 용량 계산 (더블 버퍼링 최적화)
    // 전략: 100 프레임 목표, 10 프레임 단위로 배치 쓰기
    // 평균 프레임 크기: 720p @ 20fps (메모리 최적화)
    // 32 MB SPIRAM으로 메모리 압박 최소화
    size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Available SPIRAM: %.2f MB", free_spiram / (1024.0f * 1024.0f));

    // 링버퍼 슬롯: 100 프레임 (5초 @ 20fps, 720p)
    // 실제 메모리는 동적 할당되므로 슬롯 수는 작게 유지
    spool->ring_capacity = 100;  // 100 프레임 슬롯 (5초 @ 20fps)

    // 링버퍼 할당 (프레임 엔트리 배열)
    spool->ring_buffer = heap_caps_calloc(spool->ring_capacity,
                                          sizeof(h264_frame_entry_t),
                                          MALLOC_CAP_SPIRAM);
    if (!spool->ring_buffer) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer (%u frames)",
                 (unsigned)spool->ring_capacity);
        free(spool);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Ring buffer: %u frame slots (%.2f sec @ %ufps) - Chunked batch write mode",
             (unsigned)spool->ring_capacity,
             spool->ring_capacity / (float)fps,
             (unsigned)fps);

    // 뮤텍스 생성
    spool->mutex = xSemaphoreCreateMutex();
    if (!spool->mutex) {
        heap_caps_free(spool->ring_buffer);
        free(spool);
        return ESP_ERR_NO_MEM;
    }

    // SD 임시 파일 열기
    // 파일명 변경: TEMP_H264.dat → h264temp.dat
    // 이유: FAT32에서 TEMP_H264.dat 파일명만 errno 22 발생 (다른 파일명은 모두 성공)
    snprintf(spool->temp_path, sizeof(spool->temp_path), "/sdcard/h264temp.dat");
    ESP_LOGI(TAG, "[DEBUG] Temp file path: %s (length: %d)", spool->temp_path, strlen(spool->temp_path));

    // 디렉토리 존재 확인
    struct stat st;
    ESP_LOGI(TAG, "[DEBUG] Checking /sdcard directory...");
    if (stat("/sdcard", &st) != 0) {
        ESP_LOGE(TAG, "SD card mount point '/sdcard' not found (errno: %d, %s)", errno, strerror(errno));
        vSemaphoreDelete(spool->mutex);
        heap_caps_free(spool->ring_buffer);
        free(spool);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[DEBUG] /sdcard stat success - mode: 0x%lx, size: %ld", (unsigned long)st.st_mode, st.st_size);

    if (!S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "'/sdcard' is not a directory (mode: 0x%lx)", (unsigned long)st.st_mode);
        vSemaphoreDelete(spool->mutex);
        heap_caps_free(spool->ring_buffer);
        free(spool);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[DEBUG] /sdcard is a valid directory");

    // 기존 임시 파일 확인 및 삭제
    ESP_LOGI(TAG, "[DEBUG] Checking if temp file exists...");
    if (stat(spool->temp_path, &st) == 0) {
        ESP_LOGI(TAG, "[DEBUG] Temp file exists (size: %ld bytes, mode: 0x%lx), deleting...", st.st_size, (unsigned long)st.st_mode);
        if (unlink(spool->temp_path) == 0) {
            ESP_LOGI(TAG, "[DEBUG] Temp file deleted successfully");
        } else {
            ESP_LOGW(TAG, "[DEBUG] Failed to delete temp file (errno: %d, %s)", errno, strerror(errno));
        }
    } else {
        ESP_LOGI(TAG, "[DEBUG] Temp file does not exist (errno: %d, %s)", errno, strerror(errno));
    }

    // 파일 열기 전 현재 열린 파일 수 확인
    ESP_LOGI(TAG, "[DEBUG] Attempting to open temp file with mode 'wb'...");
    ESP_LOGI(TAG, "[DEBUG] Available heap - Internal: %lu bytes, SPIRAM: %lu bytes",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    ESP_LOGI(TAG, "Opening temp file: %s", spool->temp_path);

    // ESP-IDF FAT32 VFS: 바이너리 모드 필수
    // 주의: fopen 직후 반드시 setvbuf 호출 필요 (일부 ESP-IDF 버전에서 필수)
    errno = 0;  // errno 초기화
    spool->temp_fp = fopen(spool->temp_path, "wb");
    int fopen_errno = errno;  // fopen 직후 errno 저장

    if (!spool->temp_fp) {
        ESP_LOGE(TAG, "Failed to open temp file: %s", spool->temp_path);
        ESP_LOGE(TAG, "[DEBUG] fopen returned NULL");
        ESP_LOGE(TAG, "[DEBUG] errno: %d (%s)", fopen_errno, strerror(fopen_errno));
        ESP_LOGE(TAG, "[DEBUG] File path length: %d", strlen(spool->temp_path));

        // 추가 디버깅: 다양한 모드로 테스트 파일 생성 시도
        ESP_LOGI(TAG, "[DEBUG] Testing file creation with different modes...");

        // 테스트 1: "w" 모드
        errno = 0;
        FILE *test_fp1 = fopen("/sdcard/test1.tmp", "w");
        if (test_fp1) {
            ESP_LOGI(TAG, "[DEBUG] Test 1 OK: fopen with 'w' mode succeeded");
            fclose(test_fp1);
            unlink("/sdcard/test1.tmp");
        } else {
            ESP_LOGE(TAG, "[DEBUG] Test 1 FAIL: fopen with 'w' mode failed (errno: %d, %s)", errno, strerror(errno));
        }

        // 테스트 2: "wb" 모드
        errno = 0;
        FILE *test_fp2 = fopen("/sdcard/test2.tmp", "wb");
        if (test_fp2) {
            ESP_LOGI(TAG, "[DEBUG] Test 2 OK: fopen with 'wb' mode succeeded");
            fclose(test_fp2);
            unlink("/sdcard/test2.tmp");
        } else {
            ESP_LOGE(TAG, "[DEBUG] Test 2 FAIL: fopen with 'wb' mode failed (errno: %d, %s)", errno, strerror(errno));
        }

        // 테스트 3: 짧은 파일명
        errno = 0;
        FILE *test_fp3 = fopen("/sdcard/test.dat", "wb");
        if (test_fp3) {
            ESP_LOGI(TAG, "[DEBUG] Test 3 OK: Short filename succeeded");
            fclose(test_fp3);
            unlink("/sdcard/test.dat");
        } else {
            ESP_LOGE(TAG, "[DEBUG] Test 3 FAIL: Short filename failed (errno: %d, %s)", errno, strerror(errno));
        }

        // 테스트 4: 실제 h264temp.dat 파일명 재시도
        errno = 0;
        FILE *test_fp4 = fopen("/sdcard/h264temp.dat", "wb");
        if (test_fp4) {
            ESP_LOGI(TAG, "[DEBUG] Test 4 OK: h264temp.dat retry succeeded!");
            fclose(test_fp4);
            unlink("/sdcard/h264temp.dat");
        } else {
            ESP_LOGE(TAG, "[DEBUG] Test 4 FAIL: h264temp.dat retry failed (errno: %d, %s)", errno, strerror(errno));
        }

        vSemaphoreDelete(spool->mutex);
        heap_caps_free(spool->ring_buffer);
        free(spool);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[DEBUG] fopen succeeded! FILE pointer: %p", spool->temp_fp);

    // 쓰기 버퍼 설정 (256KB로 증가 - 배치 쓰기 최적화)
    // 청크 쓰기 시 큰 버퍼가 SD 카드 성능 향상에 도움
    if (setvbuf(spool->temp_fp, NULL, _IOFBF, 256 * 1024) != 0) {
        ESP_LOGW(TAG, "setvbuf 256KB failed, trying 128KB...");
        if (setvbuf(spool->temp_fp, NULL, _IOFBF, 128 * 1024) != 0) {
            ESP_LOGW(TAG, "setvbuf 128KB failed, using default buffering");
        } else {
            ESP_LOGI(TAG, "SD write buffer: 128 KB");
        }
    } else {
        ESP_LOGI(TAG, "SD write buffer: 256 KB (optimized for chunked writes)");
    }

    ESP_LOGI(TAG, "Temp file opened successfully");

    // SD writer 태스크 생성 비활성화 (메모리 버퍼링 전략)
    // 전략: 모든 프레임을 메모리에 저장 → 녹화 완료 후 일괄 SD 저장
    // 이유: SD 실시간 쓰기 병목 제거 (2fps → 20fps 달성)
    spool->writer_task = NULL;  // 백그라운드 쓰기 비활성화

    // SD writer 태스크 비활성화로 인해 아래 코드는 실행되지 않음
    #if 0
    BaseType_t ret = xTaskCreatePinnedToCore(
        sd_writer_task,
        "h264_writer",
        8192,       // 8KB 스택
        spool,
        20,         // 높은 우선순위
        &spool->writer_task,
        1           // Core 1
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create writer task");
        fclose(spool->temp_fp);
        vSemaphoreDelete(spool->mutex);
        heap_caps_free(spool->ring_buffer);
        free(spool);
        return ESP_FAIL;
    }
    #endif  // SD writer 태스크 비활성화 종료

    ESP_LOGI(TAG, "H.264 spool started: ring=%u frames (memory buffering), temp=%s",
             (unsigned)spool->ring_capacity, spool->temp_path);
    ESP_LOGI(TAG, "SD writer task: DISABLED (all frames buffered in memory)");

    *out_handle = spool;
    return ESP_OK;
}

esp_err_t h264_spool_add_frame(h264_spool_handle_t handle,
                                const h264_encoded_frame_t *encoded_frame)
{
    if (!handle || !encoded_frame || !encoded_frame->data || !encoded_frame->size) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    // 링버퍼 가득 찬 경우
    if (handle->count >= handle->ring_capacity) {
        handle->dropped_frames++;

        // 10 프레임마다 경고
        if (handle->dropped_frames % 10 == 0) {
            ESP_LOGW(TAG, "Ring buffer FULL, dropped %u frames (capacity: %u)",
                     (unsigned)handle->dropped_frames,
                     (unsigned)handle->ring_capacity);
        }

        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NO_MEM;
    }

    // 프레임 복사
    h264_frame_entry_t *entry = &handle->ring_buffer[handle->write_idx];

    // 기존 데이터 해제 (재사용 시)
    if (entry->data) {
        heap_caps_free(entry->data);
    }

    // 새 데이터 할당 (SPIRAM 사용 - 큰 프레임 지원)
    entry->data = heap_caps_malloc(encoded_frame->size, MALLOC_CAP_SPIRAM);
    if (!entry->data) {
        ESP_LOGE(TAG, "Failed to allocate frame data (%u bytes)", (unsigned)encoded_frame->size);
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NO_MEM;
    }

    memcpy(entry->data, encoded_frame->data, encoded_frame->size);
    entry->size = encoded_frame->size;
    entry->type = encoded_frame->type;
    entry->pts = encoded_frame->pts;

    handle->write_idx = (handle->write_idx + 1) % handle->ring_capacity;
    handle->count++;
    handle->total_frames++;

    xSemaphoreGive(handle->mutex);

    return ESP_OK;
}

void h264_spool_set_fps(h264_spool_handle_t handle, uint32_t fps)
{
    if (handle) {
        handle->fps = fps;
    }
}

esp_err_t h264_spool_flush_to_file(h264_spool_handle_t handle,
                                    const char *output_path,
                                    h264_spool_progress_cb_t progress_cb,
                                    void *user_data)
{
    if (!handle || !output_path) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Flushing H.264 stream to MP4 file: %s", output_path);

    // 메모리 버퍼링 전략: 모든 프레임을 메모리에서 임시 파일로 쓰기
    ESP_LOGI(TAG, "Writing %" PRIu32 " buffered frames from memory to temp file...", handle->count);

    uint32_t frames_written = 0;
    while (handle->read_idx != handle->write_idx) {
        h264_frame_entry_t *entry = &handle->ring_buffer[handle->read_idx];

        if (entry->data && entry->size > 0) {
            // 프레임 크기 쓰기 (4 bytes)
            if (fwrite(&entry->size, 1, 4, handle->temp_fp) != 4) {
                ESP_LOGE(TAG, "Failed to write frame size");
                break;
            }

            // 프레임 데이터 쓰기
            if (fwrite(entry->data, 1, entry->size, handle->temp_fp) != entry->size) {
                ESP_LOGE(TAG, "Failed to write frame data");
                break;
            }

            frames_written++;
        }

        handle->read_idx = (handle->read_idx + 1) % handle->ring_capacity;
    }

    fflush(handle->temp_fp);
    fclose(handle->temp_fp);
    handle->temp_fp = NULL;

    // 통계 업데이트
    handle->written_frames = frames_written;

    ESP_LOGI(TAG, "Memory flush complete: %" PRIu32 " frames written to temp file", frames_written);

    // 임시 파일 다시 열기 (읽기 모드)
    FILE *temp_fp = fopen(handle->temp_path, "rb");
    if (!temp_fp) {
        ESP_LOGE(TAG, "Failed to reopen temp file for reading");
        return ESP_FAIL;
    }

    // MP4 muxer 생성
    mp4_muxer_config_t mp4_config = {
        .width = handle->width,
        .height = handle->height,
        .fps = handle->fps,
        .duration_sec = (float)handle->written_frames / handle->fps
    };

    mp4_muxer_handle_t mp4_muxer = NULL;
    esp_err_t ret = mp4_muxer_create(output_path, &mp4_config, &mp4_muxer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create MP4 muxer");
        fclose(temp_fp);
        return ret;
    }

    ESP_LOGI(TAG, "Processing %u H.264 NAL units into MP4 container...", (unsigned)handle->written_frames);

    // NAL unit을 읽어서 MP4 muxer에 추가
    ret = ESP_OK;

    for (uint32_t i = 0; i < handle->written_frames; i++) {
        uint32_t nal_size = 0;
        size_t read_count = fread(&nal_size, sizeof(uint32_t), 1, temp_fp);

        if (read_count != 1) {
            ESP_LOGW(TAG, "Failed to read NAL size at frame %u", (unsigned)i);
            ret = ESP_FAIL;
            break;
        }

        if (nal_size == 0 || nal_size > 10 * 1024 * 1024) {
            ESP_LOGE(TAG, "Invalid NAL size %u bytes at frame %u", (unsigned)nal_size, (unsigned)i);
            ret = ESP_FAIL;
            break;
        }

        uint8_t *nal_data = malloc(nal_size);
        if (!nal_data) {
            ESP_LOGE(TAG, "Failed to allocate NAL buffer (%u bytes)", (unsigned)nal_size);
            ret = ESP_ERR_NO_MEM;
            break;
        }

        size_t read_size = fread(nal_data, 1, nal_size, temp_fp);
        if (read_size != nal_size) {
            ESP_LOGW(TAG, "Failed to read NAL data at frame %u", (unsigned)i);
            free(nal_data);
            ret = ESP_FAIL;
            break;
        }

        // MP4 muxer에 NAL unit 추가
        esp_err_t add_ret = mp4_muxer_add_nal(mp4_muxer, nal_data, nal_size);
        free(nal_data);

        if (add_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add NAL to MP4 at frame %u", (unsigned)i);
            ret = ESP_FAIL;
            break;
        }

        // 진행률 콜백 (1% 단위)
        if (progress_cb && (i % (handle->written_frames / 100 + 1) == 0 || i == handle->written_frames - 1)) {
            progress_cb(i + 1, handle->written_frames, user_data);
        }

        // Watchdog 방지
        if (i % 10 == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    // 임시 파일 닫기
    fclose(temp_fp);

    // MP4 finalize (moov box 생성)
    if (ret == ESP_OK) {
        ret = mp4_muxer_finalize(mp4_muxer);
    }

    // MP4 muxer 정리
    mp4_muxer_destroy(mp4_muxer);

    if (ret == ESP_OK) {
        // 파일 크기 확인
        struct stat st;
        if (stat(output_path, &st) == 0) {
            ESP_LOGI(TAG, "MP4 file created successfully!");
            ESP_LOGI(TAG, "  Path: %s", output_path);
            ESP_LOGI(TAG, "  Size: %.2f MB (%ld bytes)", st.st_size / (1024.0f * 1024.0f), st.st_size);
            ESP_LOGI(TAG, "  Frames: %u", (unsigned)handle->written_frames);
            ESP_LOGI(TAG, "  Duration: %.2f sec", (float)handle->written_frames / handle->fps);
        } else {
            ESP_LOGW(TAG, "MP4 file created but stat failed (errno: %d, %s)", errno, strerror(errno));
        }
    } else {
        ESP_LOGE(TAG, "Failed to create MP4 file (error code: %d)", ret);
    }

    // 임시 파일 삭제
    if (unlink(handle->temp_path) == 0) {
        ESP_LOGI(TAG, "Temp file deleted successfully");
    } else {
        ESP_LOGW(TAG, "Failed to delete temp file (errno: %d, %s)", errno, strerror(errno));
    }

    return ret;
}

void h264_spool_stop(h264_spool_handle_t handle)
{
    if (!handle) {
        return;
    }

    ESP_LOGI(TAG, "Stopping H.264 spool (total: %u, dropped: %u, written: %u)",
             (unsigned)handle->total_frames,
             (unsigned)handle->dropped_frames,
             (unsigned)handle->written_frames);

    // Writer 태스크 중지
    if (handle->writer_task) {
        handle->flush_waiter = xTaskGetCurrentTaskHandle();
        handle->stop_writer = true;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // 링버퍼 해제
    if (handle->ring_buffer) {
        for (uint32_t i = 0; i < handle->ring_capacity; i++) {
            if (handle->ring_buffer[i].data) {
                heap_caps_free(handle->ring_buffer[i].data);
            }
        }
        heap_caps_free(handle->ring_buffer);
    }

    // 뮤텍스 해제
    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }

    // 임시 파일 삭제
    unlink(handle->temp_path);

    free(handle);
    ESP_LOGI(TAG, "H.264 spool stopped");
}

esp_err_t h264_spool_get_stats(h264_spool_handle_t handle,
                                uint32_t *out_buffered_frames,
                                uint32_t *out_dropped_frames,
                                uint32_t *out_written_frames)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (out_buffered_frames) {
        *out_buffered_frames = handle->count;
    }
    if (out_dropped_frames) {
        *out_dropped_frames = handle->dropped_frames;
    }
    if (out_written_frames) {
        *out_written_frames = handle->written_frames;
    }

    return ESP_OK;
}
