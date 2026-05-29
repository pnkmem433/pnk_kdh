set(srcs
    "main.c"
    "app_video.c"
    # "app_lcd.c"  # Removed - not used, saves flash space
    "../sdmmc/sd_card.c"
    "mjpeg_recorder.c"
    "mjpeg_spool.c"
)

idf_component_register(
    SRCS "${srcs}"
    PRIV_INCLUDE_DIRS . ../sdmmc
    REQUIRES esp_driver_sdmmc esp_driver_ppa esp_driver_jpeg fatfs vfs esp_timer esp_hw_support
)
