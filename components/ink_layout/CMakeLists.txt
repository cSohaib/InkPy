if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register()
    return()
endif()
set(md4c "${PROJECT_DIR}/prototypes/math/.deps/md4c/src/md4c.c")
idf_component_register(SRCS "ink_layout.c" "md_budget.c" "${md4c}"
    INCLUDE_DIRS "." "${PROJECT_DIR}/prototypes/math/.deps/md4c/src")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
set_source_files_properties("${md4c}" PROPERTIES COMPILE_OPTIONS
    "-include;${CMAKE_CURRENT_LIST_DIR}/md_budget.h;-Dmalloc=ink_md_malloc;-Drealloc=ink_md_realloc;-Dfree=ink_md_free;-Wno-unused-parameter")
