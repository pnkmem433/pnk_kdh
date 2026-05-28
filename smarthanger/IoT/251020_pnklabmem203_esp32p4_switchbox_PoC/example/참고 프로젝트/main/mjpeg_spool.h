#ifndef MJPEG_SPOOL_H
#define MJPEG_SPOOL_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mjpeg_spool mjpeg_spool_t;

// Callback for progress reporting during flush: (current_frame, total_frames, user_data)
typedef void (*mjpeg_spool_progress_cb_t)(uint32_t current, uint32_t total, void *user_data);

esp_err_t mjpeg_spool_start(uint32_t width, uint32_t height, uint32_t fps, mjpeg_spool_t **out);
esp_err_t mjpeg_spool_add_raw_frame(mjpeg_spool_t *s, const uint8_t *yuv420, uint32_t size);
void mjpeg_spool_set_fps(mjpeg_spool_t *s, uint32_t fps);
esp_err_t mjpeg_spool_flush_to_file(mjpeg_spool_t *s, const char *path, mjpeg_spool_progress_cb_t progress_cb, void *user_data);
void mjpeg_spool_stop(mjpeg_spool_t *s);

#ifdef __cplusplus
}
#endif

#endif // MJPEG_SPOOL_H

