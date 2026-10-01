if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register(REQUIRES newlib)
    return()
endif()
set(DEPS "${PROJECT_DIR}/prototypes/math/.deps")
include("${PROJECT_DIR}/cmake/math-sources.cmake")
idf_component_register(SRCS "ink_math.cpp" ${MATH_VENDOR_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES newlib)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(${COMPONENT_LIB} SYSTEM PRIVATE
    "${OVERLAY}" "${MT}" "${FT}/include" "${XML}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    FT2_BUILD_LIBRARY FT_CONFIG_MODULES_H="ink_ft_modules.h" FT_CONFIG_OPTIONS_H="ink_ft_options.h")
# Keep upstream warnings distinct from InkPy's own warnings-as-errors.
set_source_files_properties(${MATH_VENDOR_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-error=reorder;-Wno-error=switch")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
set_source_files_properties(ink_math.cpp PROPERTIES COMPILE_OPTIONS "-Werror;-std=gnu++17")
set_property(SOURCE ${MT_SOURCES} "${XML}/tinyxml2.cpp" APPEND PROPERTY COMPILE_OPTIONS -std=gnu++17)
