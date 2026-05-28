#include "sd_card.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "driver/jpeg_encode.h"

// Include LDO power control if supported
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

static const char *TAG = "SD_CARD";

static sdmmc_card_t *card = NULL;
static bool is_mounted = false;

#if SOC_SDMMC_IO_POWER_EXTERNAL
static sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
#endif

#define MOUNT_POINT "/sdcard"

// ESP32-P4 SDMMC Slot 1 GPIO pins (from working example)
#define SDMMC_CLK_GPIO  43
#define SDMMC_CMD_GPIO  44
#define SDMMC_D0_GPIO   39
// Provided by user for 4-bit bus
#define SDMMC_D1_GPIO   40
#define SDMMC_D2_GPIO   41
#define SDMMC_D3_GPIO   42

// BMP file header structures
typedef struct __attribute__((packed)) {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} bmp_file_header_t;

typedef struct __attribute__((packed)) {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} bmp_info_header_t;

esp_err_t sd_card_init(void)
{
    if (is_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");

    esp_err_t ret;

    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 10,  // Increased from 5 to allow more concurrent files
        .allocation_unit_size = 128 * 1024
    };

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // Target high-speed (40MHz) if card supports

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Set bus width: prefer 4-bit if D1/D2/D3 GPIOs are available
    slot_config.width = 1;

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    // ESP32-P4 SDMMC Slot 1 GPIO pins (verified working configuration)
    slot_config.clk = SDMMC_CLK_GPIO;
    slot_config.cmd = SDMMC_CMD_GPIO;
    slot_config.d0 = SDMMC_D0_GPIO;
#if defined(SDMMC_D1_GPIO) && defined(SDMMC_D2_GPIO) && defined(SDMMC_D3_GPIO)
    slot_config.d1 = SDMMC_D1_GPIO;
    slot_config.d2 = SDMMC_D2_GPIO;
    slot_config.d3 = SDMMC_D3_GPIO;
    slot_config.width = 4;
#endif
#endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

#if SOC_SDMMC_IO_POWER_EXTERNAL
    // For SoCs where the SD power can be supplied both via an internal or external power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to
    // the SD card and the internal LDO power supply, we need to initialize the power supply first.
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4,  // Default LDO channel for SDMMC
    };

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LDO power control driver: %s", esp_err_to_name(ret));
        return ret;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
    ESP_LOGI(TAG, "LDO power control initialized");
#endif

    // Log the pins used
#if defined(SDMMC_D1_GPIO) && defined(SDMMC_D2_GPIO) && defined(SDMMC_D3_GPIO)
    ESP_LOGI(TAG, "Using SD card pins: CLK=%d, CMD=%d, D0=%d, D1=%d, D2=%d, D3=%d",
             slot_config.clk, slot_config.cmd, slot_config.d0,
             slot_config.d1, slot_config.d2, slot_config.d3);
#else
    ESP_LOGI(TAG, "Using SD card pins: CLK=%d, CMD=%d, D0=%d",
             slot_config.clk, slot_config.cmd, slot_config.d0);
#endif

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
#if SOC_SDMMC_IO_POWER_EXTERNAL
        // Clean up LDO if initialization failed
        sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
        pwr_ctrl_handle = NULL;
#endif
        return ret;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    is_mounted = true;
    ESP_LOGI(TAG, "SD card initialized successfully");

    return ESP_OK;
}

esp_err_t sd_card_deinit(void)
{
    if (!is_mounted) {
        return ESP_OK;
    }

    // All done, unmount partition and disable SDMMC peripheral
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Card unmounted");

    // Deinitialize the power control driver if it was used
#if SOC_SDMMC_IO_POWER_EXTERNAL
    if (pwr_ctrl_handle != NULL) {
        ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete LDO power control driver: %s", esp_err_to_name(ret));
            return ret;
        }
        pwr_ctrl_handle = NULL;
    }
#endif

    is_mounted = false;
    card = NULL;

    return ESP_OK;
}

esp_err_t sd_card_save_image(const char *filename, const uint8_t *data, size_t size)
{
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == NULL || data == NULL || size == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Saving image to %s (%d bytes)", filepath, size);

    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno=%d)", strerror(errno), errno);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);

    if (written != size) {
        ESP_LOGE(TAG, "Failed to write all data (wrote %d of %d bytes)", written, size);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Image saved successfully");
    return ESP_OK;
}

esp_err_t sd_card_save_bmp(const char *filename, const uint8_t *rgb565_data, int width, int height)
{
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == NULL || rgb565_data == NULL || width <= 0 || height <= 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Saving BMP to %s (%dx%d)", filepath, width, height);
    ESP_LOGI(TAG, "First few pixels (RGB565): %04x %04x %04x %04x",
             ((uint16_t *)rgb565_data)[0], ((uint16_t *)rgb565_data)[1],
             ((uint16_t *)rgb565_data)[2], ((uint16_t *)rgb565_data)[3]);

    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno=%d)", strerror(errno), errno);
        return ESP_FAIL;
    }

    // Calculate padding (BMP rows must be aligned to 4 bytes)
    int row_size = width * 2;  // 2 bytes per pixel (RGB565)
    int padding = (4 - (row_size % 4)) % 4;
    int padded_row_size = row_size + padding;
    int image_size = padded_row_size * height;

    // BMP file header
    bmp_file_header_t file_header = {
        .bfType = 0x4D42,  // 'BM'
        .bfSize = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + 3 * sizeof(uint32_t) + image_size,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + 3 * sizeof(uint32_t)
    };

    // BMP info header
    bmp_info_header_t info_header = {
        .biSize = sizeof(bmp_info_header_t),
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 16,  // 16-bit
        .biCompression = 3,  // BI_BITFIELDS
        .biSizeImage = image_size,
        .biXPelsPerMeter = 2835,  // 72 DPI
        .biYPelsPerMeter = 2835,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    // Write headers
    fwrite(&file_header, sizeof(bmp_file_header_t), 1, f);
    fwrite(&info_header, sizeof(bmp_info_header_t), 1, f);
    // Write RGB565 bit masks (R5 G6 B5)
    const uint32_t red_mask = 0xF800;
    const uint32_t green_mask = 0x07E0;
    const uint32_t blue_mask = 0x001F;
    fwrite(&red_mask, sizeof(uint32_t), 1, f);
    fwrite(&green_mask, sizeof(uint32_t), 1, f);
    fwrite(&blue_mask, sizeof(uint32_t), 1, f);
    uint8_t padding_bytes[3] = {0, 0, 0};

    // Write from bottom to top (BMP format)
    for (int y = height - 1; y >= 0; y--) {
        const uint8_t *row_ptr = (const uint8_t *)&((const uint16_t *)rgb565_data)[y * width];
        fwrite(row_ptr, 1, row_size, f);
        if (padding > 0) {
            fwrite(padding_bytes, 1, padding, f);
        }
    }
    fclose(f);

    ESP_LOGI(TAG, "BMP saved successfully");
    return ESP_OK;
}

bool sd_card_is_mounted(void)
{
    return is_mounted;
}

// Persistent JPEG encoder state
static jpeg_encoder_handle_t s_jpeg_enc = NULL;
static jpeg_encode_cfg_t s_jpeg_cfg;
static uint8_t *s_jpeg_out = NULL;
static size_t s_jpeg_out_cap = 0;

esp_err_t jpeg_mem_encoder_init(int width, int height, int bpp, int quality)
{
    if (s_jpeg_enc) return ESP_OK;
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;

    jpeg_encode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = -1,
    };
    esp_err_t ret = jpeg_new_encoder_engine(&eng_cfg, &s_jpeg_enc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "jpeg_new_encoder_engine failed: %s", esp_err_to_name(ret));
        return ret;
    }

    jpeg_enc_input_format_t src_type = JPEG_ENCODE_IN_FORMAT_GRAY;
    jpeg_down_sampling_type_t sub_sample = JPEG_DOWN_SAMPLING_GRAY;
    if (bpp == 2) {
        src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
        sub_sample = JPEG_DOWN_SAMPLING_YUV422;
    } else if (bpp == 3) {
        src_type = JPEG_ENCODE_IN_FORMAT_RGB888;
        sub_sample = JPEG_DOWN_SAMPLING_YUV444;
    }
    s_jpeg_cfg = (jpeg_encode_cfg_t) {
        .height = (uint32_t)height,
        .width = (uint32_t)width,
        .src_type = src_type,
        .sub_sample = sub_sample,
        .image_quality = (uint32_t)quality,
    };

    // Allocate output buffer roughly width*height*bpp/2
    jpeg_encode_memory_alloc_cfg_t mem_cfg = { .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER };
    size_t alloc_size = 0;
    uint32_t req = (uint32_t)width * (uint32_t)height * (uint32_t)(bpp > 0 ? bpp : 1);
    s_jpeg_out = (uint8_t *)jpeg_alloc_encoder_mem(req, &mem_cfg, &alloc_size);
    if (!s_jpeg_out || alloc_size == 0) {
        ESP_LOGE(TAG, "jpeg alloc failed");
        jpeg_del_encoder_engine(s_jpeg_enc); s_jpeg_enc = NULL;
        return ESP_ERR_NO_MEM;
    }
    s_jpeg_out_cap = alloc_size;
    ESP_LOGI(TAG, "JPEG encoder ready: %dx%d bpp=%d Q=%d out_cap=%u", width, height, bpp, quality, (unsigned)s_jpeg_out_cap);
    return ESP_OK;
}

esp_err_t jpeg_mem_encode(const uint8_t *data, uint8_t **out_buf, uint32_t *out_size)
{
    if (!s_jpeg_enc || !data || !out_buf || !out_size) return ESP_ERR_INVALID_STATE;
    uint32_t in_size = s_jpeg_cfg.width * s_jpeg_cfg.height;
    if (s_jpeg_cfg.src_type == JPEG_ENCODE_IN_FORMAT_RGB565) in_size *= 2;
    else if (s_jpeg_cfg.src_type == JPEG_ENCODE_IN_FORMAT_RGB888) in_size *= 3;
    uint32_t out_len = 0;
    esp_err_t ret = jpeg_encoder_process(s_jpeg_enc, &s_jpeg_cfg, data, in_size, s_jpeg_out, (uint32_t)s_jpeg_out_cap, &out_len);
    if (ret != ESP_OK) return ret;
    *out_buf = s_jpeg_out;
    *out_size = out_len;
    return ESP_OK;
}

void jpeg_mem_encoder_deinit(void)
{
    if (s_jpeg_enc) {
        jpeg_del_encoder_engine(s_jpeg_enc);
        s_jpeg_enc = NULL;
    }
    // s_jpeg_out is managed by driver; we don't free explicitly here
    s_jpeg_out = NULL;
    s_jpeg_out_cap = 0;
}

esp_err_t sd_card_save_jpeg(const char *filename, const uint8_t *data, int width, int height, int bpp, int quality)
{
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == NULL || data == NULL || width <= 0 || height <= 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Saving JPEG to %s (%dx%d, bpp=%d, Q=%d)", filepath, width, height, bpp, quality);

    // Configure JPEG encoder
    jpeg_encode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = -1,
    };
    jpeg_encoder_handle_t enc = NULL;
    esp_err_t ret = jpeg_new_encoder_engine(&eng_cfg, &enc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "jpeg_new_encoder_engine failed: %s", esp_err_to_name(ret));
        return ret;
    }

    jpeg_enc_input_format_t src_type = JPEG_ENCODE_IN_FORMAT_GRAY;
    jpeg_down_sampling_type_t sub_sample = JPEG_DOWN_SAMPLING_GRAY;
    uint32_t in_size = (uint32_t)width * (uint32_t)height;
    if (bpp == 2) {
        src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
        sub_sample = JPEG_DOWN_SAMPLING_YUV422;
        in_size = (uint32_t)width * (uint32_t)height * 2U;
    } else if (bpp == 3) {
        src_type = JPEG_ENCODE_IN_FORMAT_RGB888;
        sub_sample = JPEG_DOWN_SAMPLING_YUV444;
        in_size = (uint32_t)width * (uint32_t)height * 3U;
    }

    jpeg_encode_cfg_t enc_cfg = {
        .height = (uint32_t)height,
        .width = (uint32_t)width,
        .src_type = src_type,
        .sub_sample = sub_sample,
        .image_quality = (uint32_t)quality,
    };

    // Allocate output buffer (use helper to ensure alignment)
    size_t out_alloc_size = 0;
    // Allocate roughly input size; JPEG should be smaller, but this is safe
    uint32_t out_req_size = in_size;
    jpeg_encode_memory_alloc_cfg_t mem_cfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    uint8_t *out_buf = (uint8_t *)jpeg_alloc_encoder_mem(out_req_size, &mem_cfg, &out_alloc_size);
    if (!out_buf || out_alloc_size == 0) {
        ESP_LOGE(TAG, "Failed to allocate JPEG output buffer (req=%u)", (unsigned)out_req_size);
        jpeg_del_encoder_engine(enc);
        return ESP_ERR_NO_MEM;
    }

    uint32_t out_size = 0;
    ret = jpeg_encoder_process(enc, &enc_cfg, data, in_size, out_buf, (uint32_t)out_alloc_size, &out_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "jpeg_encoder_process failed: %s", esp_err_to_name(ret));
        jpeg_del_encoder_engine(enc);
        return ret;
    }

    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno=%d)", strerror(errno), errno);
        jpeg_del_encoder_engine(enc);
        return ESP_FAIL;
    }

    size_t written = fwrite(out_buf, 1, out_size, f);
    fclose(f);
    jpeg_del_encoder_engine(enc);

    if (written != out_size) {
        ESP_LOGE(TAG, "Failed to write all JPEG data (wrote %u of %u)", (unsigned)written, (unsigned)out_size);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "JPEG saved successfully (%u bytes)", (unsigned)out_size);
    return ESP_OK;
}
