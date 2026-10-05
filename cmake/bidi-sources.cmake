set(INK_BIDI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../components/ink_layout")
set(INK_BIDI_VENDOR "${INK_BIDI_ROOT}/vendor/fribidi")
set(INK_BIDI_DEFINITIONS DONT_HAVE_FRIBIDI_CONFIG_H=1 HAVE_STDLIB_H=1
    HAVE_STRING_H=1 HAVE_STRINGS_H=1 HAVE_STRINGIZE=1 STDC_HEADERS=1)
set(INK_BIDI_SOURCES "${INK_BIDI_ROOT}/ink_bidi.c")
foreach(name fribidi-arabic fribidi-bidi fribidi-bidi-types fribidi-brackets
    fribidi-joining fribidi-joining-types fribidi-mirroring fribidi-run fribidi-shape)
    list(APPEND INK_BIDI_SOURCES "${INK_BIDI_VENDOR}/${name}.c")
endforeach()
