idf_component_register(SRCS "ink_reader.c" INCLUDE_DIRS "." REQUIRES ink_layout)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
