idf_build_get_property(INKPY_MATH_DIAGNOSTIC INKPY_MATH_DIAGNOSTIC)
idf_build_get_property(INKPY_BROWSER INKPY_BROWSER)
idf_build_get_property(INKPY_PYTHON_DIAGNOSTIC INKPY_PYTHON_DIAGNOSTIC)
set(app_source "main.c")
set(extra_sources "")
set(extra_requires "")
if(INKPY_PYTHON_DIAGNOSTIC)
    set(app_source "python_diagnostic.c")
    set(embed "${CMAKE_CURRENT_LIST_DIR}/../generated-python/embed")
    if(NOT EXISTS "${embed}/genhdr/qstrdefs.generated.h")
        message(FATAL_ERROR "Run scripts/prepare-python-device.sh first")
    endif()
    file(GLOB python_vendor "${embed}/py/*.c" "${embed}/port/*.c")
    list(REMOVE_ITEM python_vendor "${embed}/port/mphalport.c")
    list(APPEND extra_sources "python_worker.c" "python_gc.c"
        "../components/ink_python/ink_python.c" "../components/ink_python/ink_console.c"
        "../components/ink_browser/ink_keyboard.c" ${python_vendor})
endif()
if(INKPY_MATH_DIAGNOSTIC)
    list(APPEND extra_sources "math_diagnostic.c")
    list(APPEND extra_requires ink_math)
endif()
if(INKPY_BROWSER)
    set(app_source "browser_app.c")
    list(APPEND extra_sources "../components/ink_browser/ink_browser.c" "../components/ink_browser/browser_draw.c" "../components/ink_browser/ink_text.c" "../components/ink_browser/ink_keyboard.c")
    list(APPEND extra_sources "../components/ink_browser/ink_editor.c" "../components/ink_browser/ink_power.c" "boot.c")
endif()
idf_component_register(SRCS ${app_source} "board.c" "display.c" "input.c" "sleep.c" ${extra_sources}
    INCLUDE_DIRS "." "../components/ink_browser"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_driver_ledc
             esp_driver_sdmmc fatfs sdmmc esp_timer esp_hw_support esp_system
             esp_psram app_update ${extra_requires})
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
if(INKPY_PYTHON_DIAGNOSTIC)
    target_include_directories(${COMPONENT_LIB} PRIVATE "python_port" "${embed}"
        "${embed}/port" "../components/ink_python")
    # Generated upstream sources retain warnings without turning them into errors.
    set_source_files_properties(${python_vendor} PROPERTIES COMPILE_OPTIONS "-Wno-error;-Wno-unused-parameter")
    # ESP-IDF owns __assert_func; omit embed's fallback implementation only.
    set_source_files_properties("${embed}/port/embed_util.c" PROPERTIES COMPILE_DEFINITIONS NDEBUG)
    set_source_files_properties(python_gc.c PROPERTIES COMPILE_OPTIONS "-fno-optimize-sibling-calls")
endif()


if(INKPY_MATH_DIAGNOSTIC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE INKPY_MATH_DIAGNOSTIC=1)
endif()
