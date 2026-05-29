#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SD card
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_init(void);

/**
 * @brief Deinitialize SD card
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief Save image data to SD card
 *
 * @param filename Filename to save (e.g., "image001.jpg")
 * @param data Image data buffer
 * @param size Size of data in bytes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_save_image(const char *filename, const uint8_t *data, size_t size);

/**
 * @brief Save RGB565 frame as BMP file
 *
 * @param filename Filename to save (e.g., "image001.bmp")
 * @param rgb565_data RGB565 image data
 * @param width Image width
 * @param height Image height
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_save_bmp(const char *filename, const uint8_t *rgb565_data, int width, int height);

/**
 * @brief Save image as JPEG to SD card
 *
 * @param filename 8.3 filename (e.g., "FRAME001.JPG")
 * @param data Pointer to pixel data (RGB565/RGB888/GRAY)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param bpp Bytes per pixel of input data (1=GRAY, 2=RGB565, 3=RGB888)
 * @param quality JPEG quality (1-100, higher is better)
 * @return ESP_OK on success
 */
esp_err_t sd_card_save_jpeg(const char *filename, const uint8_t *data, int width, int height, int bpp, int quality);

/**
 * @brief Check if SD card is mounted
 *
 * @return true if mounted, false otherwise
 */
bool sd_card_is_mounted(void);

// JPEG in-memory encoder helpers (persistent encoder)
esp_err_t jpeg_mem_encoder_init(int width, int height, int bpp, int quality);
esp_err_t jpeg_mem_encode(const uint8_t *data, uint8_t **out_buf, uint32_t *out_size);
void jpeg_mem_encoder_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_H */
