/**
 * @file h264_multi_encoder.c
 * @brief 병렬 H.264 인코더 매니저 구현
 */

#include "h264_multi_encoder.h"
#include "h264_encoder.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "h264_multi";

/**
 * @brief 인코딩 작업 요청
 */
typedef struct {
    uint8_t *yuv420_data;    ///< YUV420 데이터 (동적 할당)
    uint32_t yuv420_size;    ///< 데이터 크기
    uint64_t pts;            ///< Presentation timestamp
    uint32_t frame_index;    ///< 프레임 인덱스 (순서 보장용)
} encode_job_t;

/**
 * @brief 인코더 워커 컨텍스트
 */
typedef struct {
    h264_encoder_handle_t encoder;     ///< H.264 인코더 인스턴스
    TaskHandle_t task;                 ///< 워커 태스크
    QueueHandle_t job_queue;           ///< 작업 큐
    uint32_t encoder_id;               ///< 인코더 ID (0, 1, 2)
    volatile uint32_t processed_frames; ///< 처리된 프레임 수
    volatile bool stop;                 ///< 종료 플래그
    struct h264_multi_encoder_t *parent; ///< 부모 멀티 인코더
} encoder_worker_t;

/**
 * @brief 출력 프레임 (순서 보장용)
 */
typedef struct {
    h264_encoded_frame_t frame;  ///< 인코딩된 프레임
    uint8_t *data_copy;          ///< 데이터 복사본 (동적 할당)
    uint32_t frame_index;        ///< 프레임 인덱스
    bool ready;                  ///< 준비 완료 플래그
} output_frame_t;

/**
 * @brief 멀티 인코더 구조체
 */
struct h264_multi_encoder_t {
    h264_multi_encoder_config_t config;

    // 워커 인코더들
    encoder_worker_t *workers;
    uint32_t num_workers;

    // 출력 버퍼 (순서 보장)
    output_frame_t *output_buffer;
    uint32_t output_buffer_size;   ///< 출력 버퍼 크기 (프레임 수)
    uint32_t next_output_index;    ///< 다음 출력 프레임 인덱스

    // 콜백
    h264_multi_encoder_cb_t callback;
    void *user_data;

    // 동기화
    SemaphoreHandle_t mutex;

    // 통계
    volatile uint32_t total_frames;
    volatile uint32_t completed_frames;
    volatile uint32_t dropped_frames;
};

/**
 * @brief 인코더 워커 태스크
 */
static void encoder_worker_task(void *arg)
{
    encoder_worker_t *worker = (encoder_worker_t *)arg;
    struct h264_multi_encoder_t *multi = worker->parent;

    ESP_LOGI(TAG, "Worker %u started on core %d",
             (unsigned)worker->encoder_id, xPortGetCoreID());

    while (!worker->stop) {
        encode_job_t job;

        // 작업 대기 (100ms 타임아웃)
        if (xQueueReceive(worker->job_queue, &job, pdMS_TO_TICKS(100)) != pdTRUE) {
            continue;
        }

        // 인코딩 수행
        h264_encoded_frame_t encoded_frame;
        esp_err_t ret = h264_encoder_encode(worker->encoder,
                                              job.yuv420_data,
                                              job.yuv420_size,
                                              job.pts,
                                              &encoded_frame);

        if (ret == ESP_OK) {
            // 출력 버퍼에 저장 (순서 보장)
            xSemaphoreTake(multi->mutex, portMAX_DELAY);

            uint32_t buf_idx = job.frame_index % multi->output_buffer_size;
            output_frame_t *out = &multi->output_buffer[buf_idx];

            // 기존 데이터 해제
            if (out->data_copy) {
                heap_caps_free(out->data_copy);
            }

            // 데이터 복사 (인코더 버퍼는 다음 인코딩 시 덮어써질 수 있음)
            // 큰 프레임을 위해 SPIRAM에서 할당
            out->data_copy = heap_caps_malloc(encoded_frame.size, MALLOC_CAP_SPIRAM);
            if (out->data_copy) {
                memcpy(out->data_copy, encoded_frame.data, encoded_frame.size);

                out->frame = encoded_frame;
                out->frame.data = out->data_copy;  // 복사본 포인터로 교체
                out->frame_index = job.frame_index;
                out->ready = true;

                multi->completed_frames++;

                // 순서대로 출력 (콜백 호출)
                while (multi->output_buffer[multi->next_output_index % multi->output_buffer_size].ready) {
                    uint32_t idx = multi->next_output_index % multi->output_buffer_size;
                    output_frame_t *ready_frame = &multi->output_buffer[idx];

                    if (multi->callback) {
                        multi->callback(&ready_frame->frame, multi->user_data);
                    }

                    // 콜백 완료 후 즉시 메모리 해제 (메모리 누수 방지)
                    if (ready_frame->data_copy) {
                        heap_caps_free(ready_frame->data_copy);
                        ready_frame->data_copy = NULL;
                    }

                    ready_frame->ready = false;
                    multi->next_output_index++;
                }
            } else {
                ESP_LOGE(TAG, "Worker %u: Failed to allocate output buffer (%u bytes)",
                         (unsigned)worker->encoder_id, (unsigned)encoded_frame.size);
                multi->dropped_frames++;
            }

            xSemaphoreGive(multi->mutex);

            worker->processed_frames++;
        } else {
            ESP_LOGW(TAG, "Worker %u: Encoding failed for frame %u",
                     (unsigned)worker->encoder_id, (unsigned)job.frame_index);
            multi->dropped_frames++;
        }

        // 입력 버퍼 해제
        free(job.yuv420_data);
    }

    ESP_LOGI(TAG, "Worker %u stopped (processed: %u frames)",
             (unsigned)worker->encoder_id, (unsigned)worker->processed_frames);

    vTaskDelete(NULL);
}

esp_err_t h264_multi_encoder_init(const h264_multi_encoder_config_t *config,
                                   h264_multi_encoder_cb_t callback,
                                   void *user_data,
                                   h264_multi_encoder_handle_t *out_handle)
{
    if (!config || !out_handle || config->num_encoders == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing multi-encoder: %u encoders for %ufps",
             (unsigned)config->num_encoders, (unsigned)config->fps);

    struct h264_multi_encoder_t *multi = calloc(1, sizeof(struct h264_multi_encoder_t));
    if (!multi) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(&multi->config, config, sizeof(h264_multi_encoder_config_t));
    multi->callback = callback;
    multi->user_data = user_data;
    multi->num_workers = config->num_encoders;
    multi->total_frames = 0;
    multi->completed_frames = 0;
    multi->dropped_frames = 0;
    multi->next_output_index = 0;

    // 출력 버퍼 할당 (순서 보장용, 512 프레임)
    multi->output_buffer_size = 512;
    multi->output_buffer = calloc(multi->output_buffer_size, sizeof(output_frame_t));
    if (!multi->output_buffer) {
        ESP_LOGE(TAG, "Failed to allocate output buffer");
        free(multi);
        return ESP_ERR_NO_MEM;
    }

    // 뮤텍스 생성
    multi->mutex = xSemaphoreCreateMutex();
    if (!multi->mutex) {
        free(multi->output_buffer);
        free(multi);
        return ESP_ERR_NO_MEM;
    }

    // 워커 생성
    multi->workers = calloc(multi->num_workers, sizeof(encoder_worker_t));
    if (!multi->workers) {
        vSemaphoreDelete(multi->mutex);
        free(multi->output_buffer);
        free(multi);
        return ESP_ERR_NO_MEM;
    }

    for (uint32_t i = 0; i < multi->num_workers; i++) {
        encoder_worker_t *worker = &multi->workers[i];
        worker->encoder_id = i;
        worker->processed_frames = 0;
        worker->stop = false;
        worker->parent = multi;

        // 인코더 초기화
        h264_encoder_config_t enc_config = {
            .width = config->width,
            .height = config->height,
            .fps = config->fps,
            .gop_size = config->gop_size,
            .bitrate = config->bitrate,
        };

        esp_err_t ret = h264_encoder_init(&enc_config, &worker->encoder);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize encoder %u", (unsigned)i);
            // 이미 생성된 워커들 정리
            for (uint32_t j = 0; j < i; j++) {
                h264_encoder_deinit(multi->workers[j].encoder);
                if (multi->workers[j].task) {
                    multi->workers[j].stop = true;
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
                if (multi->workers[j].job_queue) {
                    vQueueDelete(multi->workers[j].job_queue);
                }
            }
            free(multi->workers);
            vSemaphoreDelete(multi->mutex);
            free(multi->output_buffer);
            free(multi);
            return ret;
        }

        // 작업 큐 생성 (큐 깊이: 10)
        worker->job_queue = xQueueCreate(10, sizeof(encode_job_t));
        if (!worker->job_queue) {
            ESP_LOGE(TAG, "Failed to create job queue for encoder %u", (unsigned)i);
            h264_encoder_deinit(worker->encoder);
            // 정리 로직 (위와 동일)
            for (uint32_t j = 0; j < i; j++) {
                h264_encoder_deinit(multi->workers[j].encoder);
                if (multi->workers[j].task) {
                    multi->workers[j].stop = true;
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
                if (multi->workers[j].job_queue) {
                    vQueueDelete(multi->workers[j].job_queue);
                }
            }
            free(multi->workers);
            vSemaphoreDelete(multi->mutex);
            free(multi->output_buffer);
            free(multi);
            return ESP_ERR_NO_MEM;
        }

        // 워커 태스크 생성
        // Core 분배: 0, 1, 0, 1, 0, 1...
        BaseType_t core_id = (i % 2);
        char task_name[24];  // 버퍼 크기 증가: 16 → 24 (충분한 여유)
        snprintf(task_name, sizeof(task_name), "h264_enc_%u", (unsigned)i);

        BaseType_t task_ret = xTaskCreatePinnedToCore(
            encoder_worker_task,
            task_name,
            8192,  // 8KB 스택
            worker,
            20,    // 높은 우선순위
            &worker->task,
            core_id
        );

        if (task_ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create worker task %u", (unsigned)i);
            h264_encoder_deinit(worker->encoder);
            vQueueDelete(worker->job_queue);
            // 정리 로직 (위와 동일)
            for (uint32_t j = 0; j < i; j++) {
                h264_encoder_deinit(multi->workers[j].encoder);
                if (multi->workers[j].task) {
                    multi->workers[j].stop = true;
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
                if (multi->workers[j].job_queue) {
                    vQueueDelete(multi->workers[j].job_queue);
                }
            }
            free(multi->workers);
            vSemaphoreDelete(multi->mutex);
            free(multi->output_buffer);
            free(multi);
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Worker %u created on core %d", (unsigned)i, (int)core_id);
    }

    ESP_LOGI(TAG, "Multi-encoder initialized successfully (%u workers)", (unsigned)multi->num_workers);

    *out_handle = multi;
    return ESP_OK;
}

esp_err_t h264_multi_encoder_encode_async(h264_multi_encoder_handle_t handle,
                                           const uint8_t *yuv420_data,
                                           uint32_t yuv420_size,
                                           uint64_t pts,
                                           uint32_t frame_index)
{
    if (!handle || !yuv420_data || yuv420_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // 가장 여유로운 워커 선택 (라운드 로빈)
    uint32_t worker_idx = frame_index % handle->num_workers;
    encoder_worker_t *worker = &handle->workers[worker_idx];

    // 작업 생성
    encode_job_t job;
    job.yuv420_data = malloc(yuv420_size);
    if (!job.yuv420_data) {
        ESP_LOGE(TAG, "Failed to allocate job buffer (%u bytes)", (unsigned)yuv420_size);
        handle->dropped_frames++;
        return ESP_ERR_NO_MEM;
    }

    memcpy(job.yuv420_data, yuv420_data, yuv420_size);
    job.yuv420_size = yuv420_size;
    job.pts = pts;
    job.frame_index = frame_index;

    // 큐에 추가 (타임아웃: 100ms)
    if (xQueueSend(worker->job_queue, &job, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Worker %u queue full, dropping frame %u",
                 (unsigned)worker_idx, (unsigned)frame_index);
        free(job.yuv420_data);
        handle->dropped_frames++;
        return ESP_ERR_TIMEOUT;
    }

    handle->total_frames++;

    return ESP_OK;
}

esp_err_t h264_multi_encoder_wait_all(h264_multi_encoder_handle_t handle, uint32_t timeout_ms)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    uint64_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while (1) {
        xSemaphoreTake(handle->mutex, portMAX_DELAY);
        bool all_done = (handle->completed_frames + handle->dropped_frames >= handle->total_frames);
        xSemaphoreGive(handle->mutex);

        if (all_done) {
            ESP_LOGI(TAG, "All encoding jobs completed (%u total, %u completed, %u dropped)",
                     (unsigned)handle->total_frames,
                     (unsigned)handle->completed_frames,
                     (unsigned)handle->dropped_frames);
            return ESP_OK;
        }

        // 타임아웃 체크
        if (timeout_ms > 0) {
            uint64_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now_ms - start_ms >= timeout_ms) {
                ESP_LOGW(TAG, "Wait timeout (%u ms)", (unsigned)timeout_ms);
                return ESP_ERR_TIMEOUT;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t h264_multi_encoder_get_stats(h264_multi_encoder_handle_t handle,
                                        uint32_t *out_total_frames,
                                        uint32_t *out_completed_frames,
                                        uint32_t *out_dropped_frames)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    if (out_total_frames) {
        *out_total_frames = handle->total_frames;
    }
    if (out_completed_frames) {
        *out_completed_frames = handle->completed_frames;
    }
    if (out_dropped_frames) {
        *out_dropped_frames = handle->dropped_frames;
    }

    xSemaphoreGive(handle->mutex);

    return ESP_OK;
}

void h264_multi_encoder_deinit(h264_multi_encoder_handle_t handle)
{
    if (!handle) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing multi-encoder...");

    // 워커 중지
    if (handle->workers) {
        for (uint32_t i = 0; i < handle->num_workers; i++) {
            encoder_worker_t *worker = &handle->workers[i];
            worker->stop = true;
        }

        // 워커 종료 대기
        vTaskDelay(pdMS_TO_TICKS(300));

        // 정리
        for (uint32_t i = 0; i < handle->num_workers; i++) {
            encoder_worker_t *worker = &handle->workers[i];

            if (worker->encoder) {
                h264_encoder_deinit(worker->encoder);
            }

            if (worker->job_queue) {
                // 남은 작업들 정리
                encode_job_t job;
                while (xQueueReceive(worker->job_queue, &job, 0) == pdTRUE) {
                    free(job.yuv420_data);
                }
                vQueueDelete(worker->job_queue);
            }
        }

        free(handle->workers);
    }

    // 출력 버퍼 정리
    if (handle->output_buffer) {
        for (uint32_t i = 0; i < handle->output_buffer_size; i++) {
            if (handle->output_buffer[i].data_copy) {
                heap_caps_free(handle->output_buffer[i].data_copy);
            }
        }
        free(handle->output_buffer);
    }

    // 뮤텍스 해제
    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }

    free(handle);

    ESP_LOGI(TAG, "Multi-encoder deinitialized");
}
