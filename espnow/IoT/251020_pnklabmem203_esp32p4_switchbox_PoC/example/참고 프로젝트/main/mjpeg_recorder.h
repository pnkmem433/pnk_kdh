#ifndef MJPEG_RECORDER_H
#define MJPEG_RECORDER_H

#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mjpeg_recorder mjpeg_recorder_t;

esp_err_t mjpeg_recorder_start(const char *path, uint32_t width, uint32_t height, uint32_t fps, mjpeg_recorder_t **out);
esp_err_t mjpeg_recorder_write_frame(mjpeg_recorder_t *rec, const uint8_t *jpeg, uint32_t size);
esp_err_t mjpeg_recorder_stop(mjpeg_recorder_t *rec);

#ifdef __cplusplus
}
#endif

#endif // MJPEG_RECORDER_H

