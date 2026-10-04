idf_component_register(SRCS "ink_image.c" "vendor/tjpgd.c" INCLUDE_DIRS "." REQUIRES ink_math)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
