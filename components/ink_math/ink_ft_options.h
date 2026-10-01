#pragma once
#include <freetype/config/ftoption.h>
/* Fonts are complete TTF files on SD. No callback-supplied incremental glyphs. */
#undef FT_CONFIG_OPTION_INCREMENTAL
