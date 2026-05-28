/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
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
#include "mjpeg_recorder.h"
#include "mjpeg_spool.h"

#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))

#define GPIO_OUTPUT_IO_20    20
#define GPIO_OUTPUT_IO_26    26
#define GPIO_OUTPUT_IO_27    27
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_20) | (1ULL<<GPIO_OUTPUT_IO_26)| (1ULL<<GPIO_OUTPUT_IO_27))

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len, void *user_data);

static const char *TAG = "app_main";

static size_t data_cache_line_size = 0;
static mjpeg_spool_t *s_spool = NULL;
static uint32_t s_mjpeg_frames = 0;
static uint64_t s_mjpeg_start_us = 0;
static uint32_t s_actual_fps = 30;  // Will be calculated dynamically
static uint8_t *s_jpeg_conv_buf = NULL; // RGB565 staging when input is YUV420
static size_t s_jpeg_conv_size = 0;
static uint64_t s_app_start_time = 0;  // App start time for delayed recording
static bool s_recording_started = false;  // Track if recording has been initiated
static bool s_video_saved = false;  // Track if video has been saved (only save once)

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
            int Yv = yrow[x];
            int Uv = urow[x/2];
            int Vv = vrow[x/2];
            int C = Yv - 16; if (C < 0) C = 0;
            int D = Uv - 128;
            int E = Vv - 128;
            int R = (298*C + 409*E + 128) >> 8;
            int G = (298*C - 100*D - 208*E + 128) >> 8;
            int B = (298*C + 516*D + 128) >> 8;
            uint8_t r = clip8(R);
            uint8_t g = clip8(G);
            uint8_t b = clip8(B);
            dst[y*w + x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        }
    }
}

#ifndef MJPEG_QUALITY
#define MJPEG_QUALITY 30  // Reduced from 40 for smaller file size
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

    // Initialize SD card (log warning on failure but continue)
    esp_err_t sd_ret = sd_card_init();
    if (sd_ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card init failed: %s", esp_err_to_name(sd_ret));
    }

    // Initialize the video camera
    esp_err_t ret = app_video_main(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "video main init failed with error 0x%x", ret);
        return;
    }

    // Open the video device
    int video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT_YUV420);
    if (video_cam_fd0 < 0) {
        ESP_LOGE(TAG, "video cam open failed");
        return;
    }
	
	gpio_set_level(GPIO_OUTPUT_IO_20, 1);

    // Set the video buffer - allocate our own buffers
    ESP_LOGI(TAG, "Allocating camera buffers");
    #define CAM_BUF_COUNT 2  // Minimum 2 buffers required by driver
    void *camera_buf[CAM_BUF_COUNT];
    for (int i = 0; i < CAM_BUF_COUNT; i++) {
        camera_buf[i] = heap_caps_aligned_calloc(data_cache_line_size, 1, app_video_get_buf_size(), MALLOC_CAP_SPIRAM);
        if (!camera_buf[i]) {
            ESP_LOGE(TAG, "Failed to allocate camera buffer %d", i);
            // Free previously allocated buffers
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

    // Record app start time for delayed recording
    s_app_start_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Camera started. Recording will begin in 10 seconds...");

#if CONFIG_EXAMPLE_ENABLE_PRINT_FPS_RATE_VALUE
    start_time = esp_timer_get_time();  // Get the initial time for frame rate statistics
#endif
}

// Progress callback for video save
static void video_save_progress_cb(uint32_t current, uint32_t total, void *user_data)
{
    int percent = (current * 100) / total;
    ESP_LOGI(TAG, "Saving video: %d%% (%"PRIu32"/%"PRIu32" frames)", percent, current, total);
}

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len, void *user_data)
{
    // No frame skipping - record all frames from camera (camera is already configured for 30fps)
    bool should_record_frame = (s_spool != NULL);

#if CONFIG_EXAMPLE_ENABLE_PRINT_FPS_RATE_VALUE
    fps_count++;
    if (fps_count == 50) {
        int64_t end_time = esp_timer_get_time();
        ESP_LOGI(TAG, "fps: %f", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        ESP_LOGI(TAG, "camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
    }
#endif

    // Check if 10 seconds have passed since app start to begin recording
    static bool s_first_frame_saved = false;
    static uint32_t s_frame_counter = 0;
    s_frame_counter++;

    // Wait 10 seconds after app start before starting recording
    uint64_t current_time = esp_timer_get_time();
    if (!s_recording_started && (current_time - s_app_start_time >= 10ULL * 1000000ULL)) {
        s_recording_started = true;
        ESP_LOGI(TAG, "10 seconds elapsed. Ready to start recording...");
    }

    // Start recording only once
    if (s_recording_started && !s_video_saved && s_frame_counter >= 10 && sd_card_is_mounted() && s_spool == NULL) {
        // Start MJPEG spool for raw YUV420 frames (no encoding during capture)
        // Use s_actual_fps which will be calculated at the end
        if (mjpeg_spool_start(camera_buf_hes, camera_buf_ves, s_actual_fps, &s_spool) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to start MJPEG spooler");
        } else {
            s_mjpeg_frames = 0;
            s_mjpeg_start_us = esp_timer_get_time();
            ESP_LOGI(TAG, "MJPEG recording started: %ux%u (raw YUV420) -> will save to /sdcard/VIDEO001.AVI",
                     (unsigned)camera_buf_hes, (unsigned)camera_buf_ves);
        }
        // Allocate RGB565 conversion buffer for still frame JPEG only
        size_t expected_px0 = (size_t)camera_buf_hes * (size_t)camera_buf_ves;
        size_t bpp0 = expected_px0 ? (camera_buf_len / expected_px0) : 0;
        if ((bpp0 != 2 && bpp0 != 3)) {
            s_jpeg_conv_size = camera_buf_hes * camera_buf_ves * 2;
            s_jpeg_conv_buf = (uint8_t *)heap_caps_malloc(s_jpeg_conv_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        }
    }
    // DISABLED: Don't save JPEG during video recording to avoid SD card lock conflicts
    // JPEG frame saving will happen AFTER video recording completes
    if (false && !s_first_frame_saved && s_recording_started && s_frame_counter >= 10 && sd_card_is_mounted()) {
        // Ensure CPU sees latest DMA-written camera buffer
        uintptr_t buf_addr = (uintptr_t)camera_buf;
        uintptr_t aligned_start = buf_addr & ~((uintptr_t)data_cache_line_size - 1);
        size_t align_head = buf_addr - aligned_start;
        size_t msync_len = camera_buf_len + align_head;
        msync_len = (msync_len + data_cache_line_size - 1) & ~((size_t)data_cache_line_size - 1);
        (void)esp_cache_msync((void *)aligned_start, msync_len, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        esp_err_t save_ret = ESP_FAIL;
        size_t expected_px = (size_t)camera_buf_hes * (size_t)camera_buf_ves;
        size_t bpp = expected_px ? (camera_buf_len / expected_px) : 0;
        // Save one still shot as color JPEG; convert to RGB565 if needed
        if ((bpp != 2 && bpp != 3) && s_jpeg_conv_buf) {
            yuv420_to_rgb565(camera_buf, (int)camera_buf_hes, (int)camera_buf_ves, s_jpeg_conv_buf);
            save_ret = sd_card_save_jpeg("FRAME001.JPG", s_jpeg_conv_buf, (int)camera_buf_hes, (int)camera_buf_ves, 2, MJPEG_QUALITY);
        } else {
            save_ret = sd_card_save_jpeg("FRAME001.JPG", camera_buf, (int)camera_buf_hes, (int)camera_buf_ves, (int)bpp, MJPEG_QUALITY);
        }
        if (save_ret == ESP_OK) {
            s_first_frame_saved = true;
            ESP_LOGI(TAG, "Saved frame #%" PRIu32 " to SD card", s_frame_counter);
        } else {
            ESP_LOGW(TAG, "Failed to save first frame: %s", esp_err_to_name(save_ret));
        }
    }
    // Continuous MJPEG AVI recording (10 seconds) - store raw YUV420
    if (s_spool && sd_card_is_mounted() && should_record_frame) {
        // Ensure cache sync before storing
        uintptr_t buf_addr2 = (uintptr_t)camera_buf;
        uintptr_t aligned_start2 = buf_addr2 & ~((uintptr_t)data_cache_line_size - 1);
        size_t align_head2 = buf_addr2 - aligned_start2;
        size_t msync_len2 = camera_buf_len + align_head2;
        msync_len2 = (msync_len2 + data_cache_line_size - 1) & ~((size_t)data_cache_line_size - 1);
        (void)esp_cache_msync((void *)aligned_start2, msync_len2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

        // Store raw YUV420 frame (no JPEG encoding during capture)
        if (mjpeg_spool_add_raw_frame(s_spool, camera_buf, camera_buf_len) == ESP_OK) {
            s_mjpeg_frames++;
        }

        uint64_t now = esp_timer_get_time();
        uint64_t recording_duration = now - s_mjpeg_start_us;
        if (recording_duration >= 10ULL * 1000000ULL) {
            // Calculate actual FPS achieved
            float actual_fps_float = (s_mjpeg_frames * 1000000.0f) / recording_duration;
            s_actual_fps = (uint32_t)(actual_fps_float + 0.5f);  // Round to nearest integer
            ESP_LOGI(TAG, "Recording complete. Captured %" PRIu32 " frames in %.2f seconds (%.2f fps)",
                     s_mjpeg_frames, recording_duration / 1000000.0f, actual_fps_float);

            // Update spool FPS before flushing
            mjpeg_spool_set_fps(s_spool, s_actual_fps);
            ESP_LOGI(TAG, "Video FPS set to %" PRIu32 " fps", s_actual_fps);

            // Flush to file with progress callback (JPEG encoding happens here)
            ESP_LOGI(TAG, "Saving video file...");
            (void)mjpeg_spool_flush_to_file(s_spool, "/sdcard/VIDEO001.AVI", video_save_progress_cb, NULL);
            mjpeg_spool_stop(s_spool);
            s_spool = NULL;
            s_video_saved = true;  // Mark video as saved - no more recordings

            ESP_LOGI(TAG, "Video recording complete. No further recordings will be made.");
        }
    }
}
