idf_build_get_property(INKPY_MATH_DIAGNOSTIC INKPY_MATH_DIAGNOSTIC)
idf_build_get_property(INKPY_BROWSER INKPY_BROWSER)
set(app_source "main.c")
set(extra_sources "")
set(extra_requires "")
if(INKPY_MATH_DIAGNOSTIC)
    list(APPEND extra_sources "math_diagnostic.c")
    list(APPEND extra_requires ink_math)
endif()
if(INKPY_BROWSER)
    set(app_source "browser_app.c")
    list(APPEND extra_sources "../components/ink_browser/ink_browser.c" "../components/ink_browser/browser_draw.c")
endif()
idf_component_register(SRCS ${app_source} "board.c" "display.c" "input.c" "sleep.c" ${extra_sources}
    INCLUDE_DIRS "." "../components/ink_browser"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_driver_ledc
             esp_driver_sdmmc fatfs sdmmc esp_timer esp_hw_support esp_system
             esp_psram app_update ${extra_requires})
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)


if(INKPY_MATH_DIAGNOSTIC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE INKPY_MATH_DIAGNOSTIC=1)
endif()

