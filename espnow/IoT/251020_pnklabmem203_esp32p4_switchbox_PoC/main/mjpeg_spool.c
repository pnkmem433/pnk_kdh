#include "mjpeg_spool.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "mjpeg_recorder.h"
#include "driver/jpeg_encode.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

typedef struct {
    uint8_t *data;      // YUV420 raw data
    uint32_t size;      // Frame size
} raw_frame_t;

struct mjpeg_spool {
    uint32_t width, height, fps;
    uint32_t count;
    uint32_t cap;
    uint32_t frame_size;  // Size of each YUV420 frame

    // Circular buffer in SPIRAM
    uint8_t *ring_buffer;
    size_t ring_buffer_size;
    uint32_t write_idx;
    uint32_t read_idx;

    // SD card streaming
    FILE *raw_fp;
    char raw_path[64];

    // Multi-threading
    TaskHandle_t writer_task;
    QueueHandle_t frame_queue;
    SemaphoreHandle_t mutex;
    volatile bool stop_writer;
    volatile uint32_t frames_written;
    // Notifies flusher/stopper that writer finished and file is closed
    TaskHandle_t flush_waiter;
};

static const char *TAG = "mjpeg_spool";

// Background writer task - writes frames from ring buffer to SD card
static void sd_writer_task(void *arg)
{
    mjpeg_spool_t *s = (mjpeg_spool_t *)arg;

    // Write directly from ring buffer (no batch buffer needed)
    ESP_LOGI(TAG, "SD writer task started (direct write from ring buffer)");
    ESP_LOGI(TAG, "  Ring buffer ptr: %p, size: %zu bytes, capacity: %"PRIu32" frames",
             s->ring_buffer, s->ring_buffer_size, s->cap);

    uint32_t write_iterations = 0;
    uint64_t last_log = esp_timer_get_time();
    uint64_t total_bytes_written = 0;
    uint32_t log_interval_frames = 50;

    while (!s->stop_writer || s->read_idx != s->write_idx) {
        write_iterations++;

        // Check if there's data to write
        if (s->read_idx == s->write_idx) {
            vTaskDelay(pdMS_TO_TICKS(5));  // Reduced from 10ms to 5ms for faster response
            continue;
        }

        // Write multiple frames in batch if available (up to 3 frames per iteration)
        // This reduces mutex overhead and improves SD write efficiency
        int batch_count = 0;
        const int MAX_BATCH = 3;
        uint64_t batch_start_us = esp_timer_get_time();

        while (batch_count < MAX_BATCH && s->read_idx != s->write_idx) {
            xSemaphoreTake(s->mutex, portMAX_DELAY);

            // Check again inside mutex
            if (s->read_idx == s->write_idx) {
                xSemaphoreGive(s->mutex);
                break;
            }

            // Get pointer to frame in ring buffer
            uint8_t *frame_ptr = s->ring_buffer + (s->read_idx * s->frame_size);
            uint32_t current_read_idx = s->read_idx;
            s->read_idx = (s->read_idx + 1) % s->cap;

            xSemaphoreGive(s->mutex);  // Release mutex BEFORE SD write

            // Verify pointer is within ring buffer bounds
            if (frame_ptr < s->ring_buffer || frame_ptr >= (s->ring_buffer + s->ring_buffer_size)) {
                ESP_LOGE(TAG, "CRITICAL: frame_ptr out of bounds! ptr=%p, ring_buffer=%p, size=%zu, read_idx=%"PRIu32,
                         frame_ptr, s->ring_buffer, s->ring_buffer_size, current_read_idx);
                break;
            }

            // Write frame directly to SD card
            uint64_t write_start_us = esp_timer_get_time();
            size_t written = fwrite(frame_ptr, 1, s->frame_size, s->raw_fp);
            uint64_t write_duration_us = esp_timer_get_time() - write_start_us;

            if (written == s->frame_size) {
                s->frames_written++;
                batch_count++;
                total_bytes_written += written;

                // Log write performance every N frames
                if (s->frames_written % log_interval_frames == 0) {
                    uint64_t now = esp_timer_get_time();
                    float elapsed_sec = (now - last_log) / 1000000.0f;
                    float write_speed_mbps = (total_bytes_written / elapsed_sec) / (1024.0f * 1024.0f);
                    ESP_LOGI(TAG, "SD write: %"PRIu32" frames, %.2f MB/s (last write: %.2f ms)",
                             s->frames_written, write_speed_mbps, write_duration_us / 1000.0f);
                    last_log = now;
                    total_bytes_written = 0;
                }
            } else {
                ESP_LOGW(TAG, "Incomplete write: %zu/%"PRIu32" bytes (%.2f ms)",
                         written, s->frame_size, write_duration_us / 1000.0f);
                break;  // Stop batch on error
            }
        }

        uint64_t batch_duration_us = esp_timer_get_time() - batch_start_us;
        if (batch_count > 0 && batch_duration_us > 100000) {  // >100ms is concerning
            ESP_LOGW(TAG, "Slow batch write: %d frames in %.2f ms", batch_count, batch_duration_us / 1000.0f);
        }

        // Reduce flush frequency to improve performance (flush every 100 frames instead of 50)
        if (s->stop_writer || (s->frames_written % 100 == 0)) {
            fflush(s->raw_fp);
        }

        // No delay - write as fast as possible
    }

    ESP_LOGI(TAG, "Writer task finishing: wrote %"PRIu32" frames total", s->frames_written);

    // Final flush and close - IMPORTANT: Close file BEFORE task exits
    fflush(s->raw_fp);
    fclose(s->raw_fp);
    s->raw_fp = NULL;

    ESP_LOGI(TAG, "SD file closed, writer task exiting");

    // Notify waiter (if any) that writer is done and file is closed
    if (s->flush_waiter) {
        xTaskNotifyGive(s->flush_waiter);
        s->flush_waiter = NULL;
    }

    vTaskDelete(NULL);
}

// Helper to convert YUV420 to RGB565
static inline uint8_t clip8(int v) { return (v < 0) ? 0 : (v > 255 ? 255 : v); }
static void yuv420_to_rgb565(const uint8_t *yuv, int w, int h, uint8_t *rgb565)
{
    const uint8_t *Y = yuv;
    const uint8_t *U = Y + w * h;
    const uint8_t *V = U + (w/2) * (h/2);
    uint16_t *dst = (uint16_t *)rgb565;
    for (int y = 0; y < h; y++) {
        const uint8_t *yrow = Y + y * w;
        const uint8_t *urow = U + (y/2) * (w/2);
        const uint8_t *vrow = V + (y/2) * (w/2);
        for (int x = 0; x < w; x++) {
            int Yv = yrow[x], Uv = urow[x/2], Vv = vrow[x/2];
            int C = Yv - 16; if (C < 0) C = 0;
            int D = Uv - 128, E = Vv - 128;
            int R = (298*C + 409*E + 128) >> 8;
            int G = (298*C - 100*D - 208*E + 128) >> 8;
            int B = (298*C + 516*D + 128) >> 8;
            dst[y*w + x] = (uint16_t)(((clip8(R) & 0xF8) << 8) | ((clip8(G) & 0xFC) << 3) | (clip8(B) >> 3));
        }
    }
}

esp_err_t mjpeg_spool_start(uint32_t width, uint32_t height, uint32_t fps, mjpeg_spool_t **out)
{
    if (!width || !height || !fps || !out) return ESP_ERR_INVALID_ARG;

    mjpeg_spool_t *s = calloc(1, sizeof(*s));
    if (!s) return ESP_ERR_NO_MEM;

    s->width = width; s->height = height; s->fps = fps;
    s->count = 0;
    s->write_idx = 0;
    s->read_idx = 0;
    s->stop_writer = false;
    s->frames_written = 0;
    s->flush_waiter = NULL;

    // Calculate YUV420 frame size: width * height * 1.5
    s->frame_size = (width * height * 3) / 2;

    // Get available SPIRAM
    size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Available SPIRAM: %.2f MB", free_spiram / (1024.0f * 1024.0f));

    // Use 100% of SPIRAM (no batch reserve due to extreme memory limitation)
    size_t usable_spiram = free_spiram;  // Use all available SPIRAM
    s->cap = usable_spiram / s->frame_size;

    // Minimum 5 frames for ring buffer (~0.17 sec @ 30fps, absolute minimum for 1080p)
    if (s->cap < 5) {
        ESP_LOGE(TAG, "Insufficient SPIRAM for ring buffer (need %.2f MB, have %.2f MB)",
                 (5 * s->frame_size) / (1024.0f * 1024.0f), free_spiram / (1024.0f * 1024.0f));
        free(s);
        return ESP_ERR_NO_MEM;
    }
    // No upper limit - use all available SPIRAM for maximum buffer
    // More buffer = better tolerance for SD write latency spikes

    ESP_LOGI(TAG, "Ring buffer: %"PRIu32" frames (%.2f sec @ 30fps, %.2f MB)",
             s->cap, s->cap / 30.0f, (s->cap * s->frame_size) / (1024.0f * 1024.0f));

    // Allocate ring buffer
    s->ring_buffer_size = s->frame_size * s->cap;
    s->ring_buffer = (uint8_t *)heap_caps_malloc(s->ring_buffer_size, MALLOC_CAP_SPIRAM);
    if (!s->ring_buffer) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer (%.2f MB)", s->ring_buffer_size / (1024.0f * 1024.0f));
        free(s);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Ring buffer allocated: ptr=%p, size=%zu bytes (%.2f MB), range: %p - %p",
             s->ring_buffer, s->ring_buffer_size, s->ring_buffer_size / (1024.0f * 1024.0f),
             s->ring_buffer, s->ring_buffer + s->ring_buffer_size);

    // Create mutex
    s->mutex = xSemaphoreCreateMutex();
    if (!s->mutex) {
        heap_caps_free(s->ring_buffer);
        free(s);
        return ESP_ERR_NO_MEM;
    }

    // Open SD card file
    snprintf(s->raw_path, sizeof(s->raw_path), "/sdcard/TEMP_RAW.yuv");
    s->raw_fp = fopen(s->raw_path, "wb");
    if (!s->raw_fp) {
        ESP_LOGE(TAG, "Failed to open raw file: %s", s->raw_path);
        vSemaphoreDelete(s->mutex);
        heap_caps_free(s->ring_buffer);
        free(s);
        return ESP_FAIL;
    }

    // Set large SD write buffer (512KB for maximum throughput)
    // Larger buffer reduces the number of physical SD writes
    size_t buf_size = 512 * 1024;
    uint8_t *sd_write_buf = heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL);
    if (!sd_write_buf) {
        // Fallback to 256KB if 512KB fails
        buf_size = 256 * 1024;
        sd_write_buf = heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL);
    }
    if (sd_write_buf) {
        setvbuf(s->raw_fp, (char *)sd_write_buf, _IOFBF, buf_size);
        ESP_LOGI(TAG, "SD write buffer: %zu KB", buf_size / 1024);
    } else {
        ESP_LOGW(TAG, "Failed to allocate SD buffer, using default");
    }

    // Create SD writer task with HIGH priority
    // Stack size increased to 16KB to prevent stack overflow
    BaseType_t ret = xTaskCreatePinnedToCore(
        sd_writer_task,
        "sd_writer",
        16384,  // 16KB stack (increased from 8KB to prevent overflow)
        s,
        20,  // High priority (higher than camera task)
        &s->writer_task,
        1   // Core 1
    );

    ESP_LOGI(TAG, "Writer task created: stack=16KB, priority=20, core=1");

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create writer task");
        fclose(s->raw_fp);
        vSemaphoreDelete(s->mutex);
        heap_caps_free(s->ring_buffer);
        free(s);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Hybrid spool initialized: ring buffer %"PRIu32" frames (%.2f sec), SD streaming to %s",
             s->cap, s->cap / 30.0f, s->raw_path);

    *out = s;
    return ESP_OK;
}

esp_err_t mjpeg_spool_add_raw_frame(mjpeg_spool_t *s, const uint8_t *yuv420, uint32_t size)
{
    if (!s || !yuv420 || !size) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s->mutex, portMAX_DELAY);

    // Check if ring buffer is full
    uint32_t next_write = (s->write_idx + 1) % s->cap;
    if (next_write == s->read_idx) {
        uint32_t buffer_occupancy = (s->write_idx >= s->read_idx) ?
            (s->write_idx - s->read_idx) : (s->cap - s->read_idx + s->write_idx);

        xSemaphoreGive(s->mutex);
        // Ring buffer full - drop frame (or could block here)
        static uint32_t drop_count = 0;
        static uint64_t last_drop_log = 0;
        uint64_t now = esp_timer_get_time();

        if (++drop_count % 10 == 0) {
            float elapsed_sec = (now - last_drop_log) / 1000000.0f;
            float drop_rate = (elapsed_sec > 0) ? (10.0f / elapsed_sec) : 0.0f;
            ESP_LOGW(TAG, "Ring buffer FULL (%"PRIu32"/%"PRIu32" frames), dropped %"PRIu32" frames total (%.1f fps drop rate, write_lag: %"PRIu32" frames)",
                     buffer_occupancy, s->cap, drop_count, drop_rate, s->count - s->frames_written);
            last_drop_log = now;
        }
        return ESP_ERR_NO_MEM;
    }

    // Fast memcpy to ring buffer
    uint8_t *dest = s->ring_buffer + (s->write_idx * s->frame_size);
    memcpy(dest, yuv420, size);

    s->write_idx = next_write;
    s->count++;

    xSemaphoreGive(s->mutex);

    return ESP_OK;
}

void mjpeg_spool_set_fps(mjpeg_spool_t *s, uint32_t fps)
{
    if (s) {
        s->fps = fps;
    }
}

esp_err_t mjpeg_spool_flush_to_file(mjpeg_spool_t *s, const char *path, mjpeg_spool_progress_cb_t progress_cb, void *user_data)
{
    if (!s || !path) return ESP_ERR_INVALID_ARG;

    // Stop writer task and wait for completion (writer will close the file)
    s->flush_waiter = xTaskGetCurrentTaskHandle();
    s->stop_writer = true;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "SD writer finished and file closed");

    uint64_t start_time = esp_timer_get_time();

    // Reopen temp file for reading
    FILE *raw_read_fp = fopen(s->raw_path, "rb");
    if (!raw_read_fp) {
        ESP_LOGE(TAG, "Failed to reopen temp file for reading");
        return ESP_FAIL;
    }

    // Determine total frames by file size to avoid partial reads
    if (fseek(raw_read_fp, 0, SEEK_END) != 0) {
        fclose(raw_read_fp);
        return ESP_FAIL;
    }
    long file_bytes = ftell(raw_read_fp);
    if (file_bytes < 0) {
        fclose(raw_read_fp);
        return ESP_FAIL;
    }
    rewind(raw_read_fp);
    uint32_t total_frames = (uint32_t)(file_bytes / (long)s->frame_size);
    ESP_LOGI(TAG, "Starting video save: %s (%"PRIu32" frames @ %"PRIu32"fps, %.2f MB raw YUV420)",
             path, total_frames, s->fps, (total_frames * s->frame_size) / (1024.0f * 1024.0f));

    // Allocate YUV frame buffer for reading
    uint8_t *yuv_buf = heap_caps_malloc(s->frame_size, MALLOC_CAP_SPIRAM);
    if (!yuv_buf) {
        fclose(raw_read_fp);
        return ESP_ERR_NO_MEM;
    }

    // Initialize JPEG encoder for encoding during save
    jpeg_encoder_handle_t jpeg_encoder;
    jpeg_encode_engine_cfg_t eng_cfg = { .intr_priority = 0, .timeout_ms = -1 };
    esp_err_t ret = jpeg_new_encoder_engine(&eng_cfg, &jpeg_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create JPEG encoder");
        return ret;
    }

    // Allocate temporary buffers for encoding
    uint8_t *rgb565_buf = (uint8_t *)heap_caps_malloc(s->width * s->height * 2, MALLOC_CAP_SPIRAM);
    if (!rgb565_buf) {
        jpeg_del_encoder_engine(jpeg_encoder);
        return ESP_ERR_NO_MEM;
    }

    jpeg_encode_memory_alloc_cfg_t mem_cfg = { .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER };
    uint32_t req = s->width * s->height * 2;
    size_t jpeg_temp_buf_cap;
    uint8_t *jpeg_temp_buf = (uint8_t *)jpeg_alloc_encoder_mem(req, &mem_cfg, &jpeg_temp_buf_cap);
    if (!jpeg_temp_buf) {
        heap_caps_free(rgb565_buf);
        jpeg_del_encoder_engine(jpeg_encoder);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "JPEG encoder initialized, starting encode+save...");

    // Open AVI file for writing
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "open %s failed: %s (errno=%d)", path, strerror(errno), errno);
        free(jpeg_temp_buf);
        heap_caps_free(rgb565_buf);
        jpeg_del_encoder_engine(jpeg_encoder);
        return ESP_FAIL;
    }
    uint8_t *io_buf = (uint8_t *)malloc(512 * 1024);
    if (io_buf) setvbuf(fp, (char *)io_buf, _IOFBF, 512 * 1024);

    // RIFF
    fwrite("RIFF", 1, 4, fp);
    uint32_t riffSizePos = (uint32_t)ftell(fp); fwrite("\0\0\0\0", 1, 4, fp);
    fwrite("AVI ", 1, 4, fp);

    // LIST hdrl
    fwrite("LIST", 1, 4, fp);
    uint32_t hdrlSizePos = (uint32_t)ftell(fp); fwrite("\0\0\0\0", 1, 4, fp);
    fwrite("hdrl", 1, 4, fp);

    // avih
    fwrite("avih", 1, 4, fp);
    uint32_t avihSize = 56; fwrite(&avihSize, 4, 1, fp);
    uint32_t usecPerFrame = 1000000U / s->fps; fwrite(&usecPerFrame, 4, 1, fp);
    uint32_t zero32 = 0; fwrite(&zero32, 4, 1, fp); // maxBytesPerSec
    fwrite(&zero32, 4, 1, fp); // pad gran
    uint32_t flags = 0x10; fwrite(&flags, 4, 1, fp); // has index
    uint32_t totalFramesPos = (uint32_t)ftell(fp); fwrite(&zero32, 4, 1, fp); // total frames
    fwrite(&zero32, 4, 1, fp); // init frames
    uint32_t streams = 1; fwrite(&streams, 4, 1, fp);
    uint32_t suggBuf = s->width * s->height * 2; fwrite(&suggBuf, 4, 1, fp);
    fwrite(&s->width, 4, 1, fp); fwrite(&s->height, 4, 1, fp);
    uint32_t reserved[4] = {0}; fwrite(reserved, 4, 4, fp);

    // LIST strl
    fwrite("LIST", 1, 4, fp);
    uint32_t strlSizePos = (uint32_t)ftell(fp); fwrite("\0\0\0\0", 1, 4, fp);
    fwrite("strl", 1, 4, fp);

    // strh
    fwrite("strh", 1, 4, fp);
    uint32_t strhSize = 56; fwrite(&strhSize, 4, 1, fp);
    fwrite("vids", 1, 4, fp); fwrite("MJPG", 1, 4, fp);
    fwrite(&zero32, 4, 1, fp); uint16_t s16=0; fwrite(&s16,2,1,fp); fwrite(&s16,2,1,fp);
    fwrite(&zero32, 4, 1, fp);
    uint32_t scale=1; fwrite(&scale,4,1,fp); uint32_t rate=s->fps; fwrite(&rate,4,1,fp);
    fwrite(&zero32,4,1,fp);
    uint32_t lenPos = (uint32_t)ftell(fp); fwrite(&zero32,4,1,fp);
    uint32_t sugg = s->width * s->height * 2; fwrite(&sugg,4,1,fp);
    uint32_t qual = 0xFFFFFFFF; fwrite(&qual,4,1,fp);
    uint32_t sampleSize=0; fwrite(&sampleSize,4,1,fp);
    uint16_t rc[4] = {0,0,(uint16_t)s->width,(uint16_t)s->height}; fwrite(rc,2,4,fp);

    // strf
    fwrite("strf",1,4,fp); uint32_t strfSize=40; fwrite(&strfSize,4,1,fp);
    uint32_t biSize=40; fwrite(&biSize,4,1,fp);
    int32_t biW=(int32_t)s->width, biH=(int32_t)s->height; fwrite(&biW,4,1,fp); fwrite(&biH,4,1,fp);
    uint16_t planes=1, bitCount=24; fwrite(&planes,2,1,fp); fwrite(&bitCount,2,1,fp);
    fwrite("MJPG",1,4,fp); uint32_t biSizeImage=0; fwrite(&biSizeImage,4,1,fp);
    fwrite(&zero32,4,1,fp); fwrite(&zero32,4,1,fp);
    fwrite(&zero32,4,1,fp); fwrite(&zero32,4,1,fp);

    // patch strl size
    uint32_t strlEnd = (uint32_t)ftell(fp);
    uint32_t strlSize = strlEnd - (strlSizePos + 4);
    fseek(fp, strlSizePos, SEEK_SET); fwrite(&strlSize, 4, 1, fp); fseek(fp, strlEnd, SEEK_SET);

    // patch hdrl size
    uint32_t hdrlEnd = (uint32_t)ftell(fp);
    uint32_t hdrlSize = hdrlEnd - (hdrlSizePos + 4);
    fseek(fp, hdrlSizePos, SEEK_SET); fwrite(&hdrlSize, 4, 1, fp); fseek(fp, hdrlEnd, SEEK_SET);

    // LIST movi
    fwrite("LIST",1,4,fp); uint32_t moviSizePos=(uint32_t)ftell(fp); fwrite("\0\0\0\0",1,4,fp); fwrite("movi",1,4,fp);
    uint32_t moviDataStart=(uint32_t)ftell(fp);

    // Allocate offset array and JPEG sizes array
    uint32_t *offsets=(uint32_t*)malloc(sizeof(uint32_t)*total_frames);
    uint32_t *jpeg_sizes=(uint32_t*)malloc(sizeof(uint32_t)*total_frames);
    if (!offsets || !jpeg_sizes) {
        if (offsets) free(offsets);
        if (jpeg_sizes) free(jpeg_sizes);
        fclose(fp);
        if (io_buf) free(io_buf);
        heap_caps_free(yuv_buf);
        fclose(raw_read_fp);
        free(jpeg_temp_buf);
        heap_caps_free(rgb565_buf);
        jpeg_del_encoder_engine(jpeg_encoder);
        return ESP_ERR_NO_MEM;
    }

    // Encode and write JPEG frames
    jpeg_encode_cfg_t enc_cfg = {
        .height = s->height,
        .width = s->width,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV420,
        .image_quality = 30,
    };

    for (uint32_t i=0;i<total_frames;i++) {
        // Read raw YUV420 frame from temp file
        size_t read_bytes = fread(yuv_buf, 1, s->frame_size, raw_read_fp);
        if (read_bytes != s->frame_size) {
            ESP_LOGW(TAG, "Frame %"PRIu32" read failed", i);
            jpeg_sizes[i] = 0;
            offsets[i] = 0;
            continue;
        }

        // Convert YUV420 → RGB565
        yuv420_to_rgb565(yuv_buf, s->width, s->height, rgb565_buf);

        // Encode to JPEG
        uint32_t out_len = 0;
        esp_err_t encode_ret = jpeg_encoder_process(jpeg_encoder, &enc_cfg, rgb565_buf,
                                         s->width * s->height * 2, jpeg_temp_buf,
                                         jpeg_temp_buf_cap, &out_len);

        if (encode_ret != ESP_OK || out_len == 0) {
            ESP_LOGW(TAG, "Frame %"PRIu32" encoding failed, skipping", i);
            jpeg_sizes[i] = 0;
            offsets[i] = 0;
            continue;
        }

        // Write JPEG frame to AVI
        uint32_t chunkPos=(uint32_t)ftell(fp);
        offsets[i]=chunkPos+8 - moviDataStart;
        jpeg_sizes[i]=out_len;
        fwrite("00dc",1,4,fp);
        fwrite(&out_len,4,1,fp);
        fwrite(jpeg_temp_buf,1,out_len,fp);
        if (out_len & 1) { uint8_t pad=0; fwrite(&pad,1,1,fp); }

        // Yield to prevent watchdog timeout and allow other tasks to run
        vTaskDelay(pdMS_TO_TICKS(1));

        // Report progress every 10 frames or at the end
        if (progress_cb && (i % 10 == 0 || i == total_frames - 1)) {
            progress_cb(i + 1, total_frames, user_data);
        }
    }

    // Close files
    fclose(raw_read_fp);
    heap_caps_free(yuv_buf);

    uint32_t idxStart=(uint32_t)ftell(fp);

    // idx1
    fwrite("idx1",1,4,fp); uint32_t idxSize=total_frames*16; fwrite(&idxSize,4,1,fp);
    for (uint32_t i=0;i<total_frames;i++) {
        if (jpeg_sizes[i] == 0) continue;  // Skip failed frames
        fwrite("00dc",1,4,fp);
        uint32_t f = 0x10; // keyframe
        fwrite(&f,4,1,fp);
        fwrite(&offsets[i],4,1,fp);
        fwrite(&jpeg_sizes[i],4,1,fp);
    }

    // patch movi size
    uint32_t moviSize = idxStart - (moviSizePos + 4);
    fseek(fp, moviSizePos, SEEK_SET); fwrite(&moviSize,4,1,fp); fseek(fp, idxStart + 8 + idxSize, SEEK_SET);

    // patch counts
    fseek(fp, totalFramesPos, SEEK_SET); fwrite(&total_frames,4,1,fp);
    fseek(fp, lenPos, SEEK_SET); fwrite(&total_frames,4,1,fp);

    // patch RIFF size
    uint32_t fileEnd=(uint32_t)ftell(fp);
    uint32_t riffSize=fileEnd - (riffSizePos);
    fseek(fp, riffSizePos, SEEK_SET); fwrite(&riffSize,4,1,fp);

    fclose(fp);
    if (io_buf) free(io_buf);
    free(offsets);
    free(jpeg_sizes);
    free(jpeg_temp_buf);
    heap_caps_free(rgb565_buf);
    jpeg_del_encoder_engine(jpeg_encoder);

    // Delete temp file
    unlink(s->raw_path);

    uint64_t end_time = esp_timer_get_time();
    float elapsed_sec = (end_time - start_time) / 1000000.0f;
    ESP_LOGI(TAG, "Video save complete: %s (%"PRIu32" frames, %.2f seconds)", path, total_frames, elapsed_sec);

    return ESP_OK;
}

void mjpeg_spool_stop(mjpeg_spool_t *s)
{
    if (!s) return;

    ESP_LOGI(TAG, "Stopping spool (frames_written: %"PRIu32", write_idx: %"PRIu32", read_idx: %"PRIu32")",
             s->frames_written, s->write_idx, s->read_idx);
    ESP_LOGI(TAG, "Ring buffer before stop: ptr=%p, size=%zu", s->ring_buffer, s->ring_buffer_size);

    // Stop writer task and wait for it to close the file (if still running)
    if (s->raw_fp != NULL && s->writer_task && eTaskGetState(s->writer_task) != eDeleted) {
        ESP_LOGI(TAG, "Waiting for writer task to finish...");
        s->flush_waiter = xTaskGetCurrentTaskHandle();
        s->stop_writer = true;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "Writer task notified completion");

        // Additional safety: Wait for task to fully exit before freeing resources
        // This prevents race condition where task might still access ring_buffer
        vTaskDelay(pdMS_TO_TICKS(100));  // Give task time to delete itself

        // Verify task is deleted
        int retry = 0;
        while (s->writer_task && eTaskGetState(s->writer_task) != eDeleted && retry < 10) {
            ESP_LOGI(TAG, "Waiting for task deletion (retry %d/10)...", retry + 1);
            vTaskDelay(pdMS_TO_TICKS(50));
            retry++;
        }

        if (retry >= 10) {
            ESP_LOGW(TAG, "Writer task did not exit cleanly after %d retries, forcing cleanup", retry);
        } else if (retry > 0) {
            ESP_LOGI(TAG, "Writer task deleted after %d retries", retry);
        } else {
            ESP_LOGI(TAG, "Writer task deleted immediately");
        }
    } else {
        ESP_LOGI(TAG, "Writer task already stopped or not running");
        s->stop_writer = true;
    }

    // Temp file was closed by writer; remove it if present
    ESP_LOGI(TAG, "Removing temporary file: %s", s->raw_path);
    unlink(s->raw_path);

    // Free resources - use mutex to ensure no concurrent access to ring_buffer
    ESP_LOGI(TAG, "Freeing ring buffer resources...");
    if (s->mutex) {
        xSemaphoreTake(s->mutex, portMAX_DELAY);
    }

    if (s->ring_buffer) {
        // Verify pointer is valid before freeing
        uint8_t *ptr = s->ring_buffer;
        ESP_LOGI(TAG, "Ring buffer ptr before free: %p (expected range: 0x28000000-0x30000000)", ptr);
        s->ring_buffer = NULL;  // Nullify first to prevent double-free

        // Check if pointer looks valid (not obviously corrupted)
        if ((uintptr_t)ptr >= 0x28000000 && (uintptr_t)ptr < 0x30000000) {
            // SPIRAM range for ESP32-P4
            ESP_LOGI(TAG, "Freeing ring buffer at %p (size: %zu bytes)", ptr, s->ring_buffer_size);
            heap_caps_free(ptr);
            ESP_LOGI(TAG, "Ring buffer freed successfully");
        } else {
            ESP_LOGE(TAG, "Ring buffer pointer corrupted: %p (outside SPIRAM range), skipping free", ptr);
            ESP_LOGE(TAG, "  Expected: 0x28000000 <= ptr < 0x30000000");
            ESP_LOGE(TAG, "  This indicates memory corruption or struct damage");
        }
    } else {
        ESP_LOGW(TAG, "Ring buffer already NULL, nothing to free");
    }

    if (s->mutex) {
        xSemaphoreGive(s->mutex);
        vSemaphoreDelete(s->mutex);
        ESP_LOGI(TAG, "Mutex deleted");
    }

    ESP_LOGI(TAG, "Freeing spool structure");
    free(s);
    ESP_LOGI(TAG, "mjpeg_spool_stop completed");
}
