if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register()
    return()
endif()
set(md4c "${PROJECT_DIR}/prototypes/math/.deps/md4c/src/md4c.c")
include("${PROJECT_DIR}/cmake/bidi-sources.cmake")
idf_component_register(SRCS "ink_layout.c" "md_budget.c" "${md4c}" ${INK_BIDI_SOURCES}
    INCLUDE_DIRS "." "${PROJECT_DIR}/prototypes/math/.deps/md4c/src")
target_include_directories(${COMPONENT_LIB} PRIVATE "${INK_BIDI_VENDOR}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${INK_BIDI_DEFINITIONS})
set_source_files_properties(${INK_BIDI_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-error")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
set_source_files_properties("${md4c}" PROPERTIES COMPILE_OPTIONS
    "-include;${CMAKE_CURRENT_LIST_DIR}/md_budget.h;-Dmalloc=ink_md_malloc;-Drealloc=ink_md_realloc;-Dfree=ink_md_free;-Wno-unused-parameter")
