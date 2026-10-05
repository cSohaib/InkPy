cmake_minimum_required(VERSION 3.16)
project(ink_reader_preview LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
set(DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../math/.deps" CACHE PATH "Pinned sources")
include(../../cmake/math-sources.cmake)
add_library(math-vendor STATIC ${MATH_VENDOR_SOURCES})
target_include_directories(math-vendor SYSTEM PUBLIC "${MT}" "${FT}/include" "${XML}")
target_include_directories(math-vendor BEFORE PRIVATE "${OVERLAY}" "${INK_MATH_ROOT}")
target_compile_definitions(math-vendor PRIVATE FT2_BUILD_LIBRARY FT_CONFIG_MODULES_H="ink_ft_modules.h" FT_CONFIG_OPTIONS_H="ink_ft_options.h")
set(CORE "${CMAKE_CURRENT_SOURCE_DIR}/../../components/ink_layout")
add_library(md4c STATIC "${DEPS}/md4c/src/md4c.c")
include(../../cmake/bidi-sources.cmake)
target_sources(md4c PRIVATE ${INK_BIDI_SOURCES})
target_include_directories(md4c PRIVATE "${INK_BIDI_VENDOR}")
target_compile_definitions(md4c PRIVATE ${INK_BIDI_DEFINITIONS})
target_include_directories(md4c PUBLIC "${DEPS}/md4c/src")
set_source_files_properties("${DEPS}/md4c/src/md4c.c" PROPERTIES
    COMPILE_OPTIONS "-include;${CORE}/md_budget.h"
    COMPILE_DEFINITIONS "malloc=ink_md_malloc;realloc=ink_md_realloc;free=ink_md_free")
add_executable(reader-math probe.c "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-math PRIVATE "${CORE}" "${INK_MATH_ROOT}")
target_compile_definitions(reader-math PRIVATE INK_READER_MATH _FILE_OFFSET_BITS=64)
target_compile_options(reader-math PRIVATE -Wall -Wextra -Werror)
target_link_libraries(reader-math PRIVATE md4c math-vendor)
add_executable(reader-device device.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-device PRIVATE "${CORE}" "${INK_MATH_ROOT}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(reader-device SYSTEM PRIVATE "${FT}/include")
target_compile_options(reader-device PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-device PRIVATE md4c math-vendor)
add_executable(dict-test "../../components/ink_dict/test.c" "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c")
target_include_directories(dict-test PRIVATE "../../components/ink_dict")
target_include_directories(dict-test SYSTEM PRIVATE "${FT}/include")
target_compile_options(dict-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(dict-test PRIVATE math-vendor)
add_executable(dict-ui dictionary-ui.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "${CORE}/ink_layout.c" "${CORE}/md_budget.c")
target_include_directories(dict-ui PRIVATE "${CORE}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(dict-ui SYSTEM PRIVATE "${FT}/include")
target_compile_options(dict-ui PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(dict-ui PRIVATE md4c math-vendor)
add_executable(font-ui "../browser/font_probe.c" "../../components/ink_math/ink_font.c"
    "../../components/ink_browser/browser_draw.c" "../../components/ink_browser/ink_power.c"
    "../../components/ink_browser/ink_keyboard.c" "../../components/ink_python/ink_console.c")
target_include_directories(font-ui PRIVATE "../../components/ink_math" "../../components/ink_browser" "../../components/ink_python")
target_include_directories(font-ui SYSTEM PRIVATE "${FT}/include")
target_compile_definitions(font-ui PRIVATE INK_USE_FONTS=1)
target_compile_options(font-ui PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(font-ui PRIVATE math-vendor)
add_executable(reader-font device.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "../../components/ink_math/ink_font.c" "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-font PRIVATE "${CORE}" "${INK_MATH_ROOT}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(reader-font SYSTEM PRIVATE "${FT}/include")
target_compile_definitions(reader-font PRIVATE INK_USE_FONTS=1)
target_compile_options(reader-font PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-font PRIVATE md4c math-vendor)
add_executable(reader-tables tables.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "../../components/ink_math/ink_font.c" "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-tables PRIVATE "${CORE}" "${INK_MATH_ROOT}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(reader-tables SYSTEM PRIVATE "${FT}/include")
target_compile_definitions(reader-tables PRIVATE INK_USE_FONTS=1)
target_compile_options(reader-tables PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-tables PRIVATE md4c math-vendor)

add_library(book-input STATIC ../../components/ink_epub/ink_zip.c ../../components/ink_epub/ink_xml.c ../../components/ink_epub/ink_epub.c ../../components/ink_image/ink_image.c ../../components/ink_image/vendor/tjpgd.c)
target_include_directories(book-input PUBLIC ../../components/ink_epub ../../components/ink_image ../../components/ink_math)
target_compile_options(book-input PRIVATE -Wall -Wextra -Werror)
target_link_libraries(book-input PUBLIC math-vendor)
foreach(target reader-device reader-font reader-tables dict-ui)
    target_link_libraries(${target} PRIVATE book-input)
endforeach()
add_executable(reader-epub epub.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "../../components/ink_math/ink_font.c" "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-epub PRIVATE "${CORE}" "${INK_MATH_ROOT}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(reader-epub SYSTEM PRIVATE "${FT}/include")
target_compile_definitions(reader-epub PRIVATE INK_USE_FONTS=1)
target_compile_options(reader-epub PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-epub PRIVATE md4c book-input)

add_executable(reader-arabic arabic.c "../../components/ink_reader/ink_reader.c"
    "../../components/ink_dict/ink_dict.c" "../../components/ink_dict/ink_dict_gzip.c"
    "../../components/ink_math/ink_font.c" "${CORE}/ink_layout.c" "${CORE}/md_budget.c" "${INK_MATH_ROOT}/ink_math.cpp")
target_include_directories(reader-arabic PRIVATE "${CORE}" "${INK_MATH_ROOT}" "../../components/ink_reader" "../../components/ink_dict")
target_include_directories(reader-arabic SYSTEM PRIVATE "${FT}/include")
target_compile_definitions(reader-arabic PRIVATE INK_USE_FONTS=1)
target_compile_options(reader-arabic PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-arabic PRIVATE md4c book-input)

add_executable(reader-incremental incremental.c "${CORE}/ink_layout.c" "${CORE}/md_budget.c")
target_include_directories(reader-incremental PRIVATE "${CORE}")
target_compile_options(reader-incremental PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(reader-incremental PRIVATE md4c)
