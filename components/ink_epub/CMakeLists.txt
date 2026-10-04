idf_component_register(SRCS "ink_zip.c" "ink_xml.c" "ink_epub.c" INCLUDE_DIRS "." REQUIRES ink_math)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
