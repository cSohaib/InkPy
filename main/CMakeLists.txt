idf_component_register(SRCS "main.c" "board.c" "display.c" "input.c" "sleep.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_driver_ledc
             esp_driver_sdmmc fatfs sdmmc esp_timer esp_hw_support esp_system
             esp_psram app_update)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
