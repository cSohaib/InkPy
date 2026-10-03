idf_component_register(SRCS "ink_reader.c" INCLUDE_DIRS "." REQUIRES ink_layout ink_dict ink_math)
target_compile_definitions(${COMPONENT_LIB} PRIVATE INK_USE_FONTS=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
