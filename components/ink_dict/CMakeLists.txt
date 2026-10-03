idf_component_register(SRCS "ink_dict.c" "ink_dict_gzip.c" INCLUDE_DIRS "." REQUIRES ink_math)
target_include_directories(${COMPONENT_LIB} SYSTEM PRIVATE "${PROJECT_DIR}/prototypes/math/.deps/freetype/include")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
