# Shared host/ESP32 sources. Fetches are explicit in scripts/fetch-math.sh.
set(INK_MATH_ROOT "${CMAKE_CURRENT_LIST_DIR}/../components/ink_math")
set(MT "${DEPS}/MicroTeX/src")
set(FT "${DEPS}/freetype")
set(XML "${DEPS}/tinyxml2")
if(NOT EXISTS "${MT}/core/macro_impl.h")
    message(FATAL_ERROR "Missing math sources. Run bash scripts/fetch-math.sh")
endif()
# Pinned upstream mathop incorrectly defaults to noLimits. Generate an overlay;
# do not mutate cached upstream. Explicit nolimits still overrides this default.
file(READ "${MT}/core/macro_impl.h" macro_header)
set(old "inline macro(mathop) {\n  auto a = _math_type(tp, args, AtomType::bigOperator);\n  a->_limitsType = LimitsType::noLimits;")
string(FIND "${macro_header}" "${old}" found)
if(found EQUAL -1)
    message(FATAL_ERROR "MicroTeX mathop patch context changed")
endif()
string(REPLACE "${old}" "inline macro(mathop) {\n  auto a = _math_type(tp, args, AtomType::bigOperator);\n  a->_limitsType = LimitsType::normal;" macro_header "${macro_header}")
set(OVERLAY "${CMAKE_CURRENT_BINARY_DIR}/microtex-overlay")
file(MAKE_DIRECTORY "${OVERLAY}/core")
file(WRITE "${OVERLAY}/core/macro_impl.h" "${macro_header}")
file(GLOB MT_SOURCES CONFIGURE_DEPENDS "${MT}/atom/*.cpp" "${MT}/box/*.cpp"
  "${MT}/core/*.cpp" "${MT}/fonts/*.cpp" "${MT}/utils/*.cpp"
  "${MT}/res/builtin/*.cpp" "${MT}/res/font/*.cpp" "${MT}/res/parser/*.cpp"
  "${MT}/res/reg/*.cpp" "${MT}/res/sym/*.cpp")
# macro_def.cpp includes its sibling header by basename; compile its generated
# copy beside the patched header to avoid picking the original first.
configure_file("${MT}/core/macro_def.cpp" "${OVERLAY}/core/macro_def.cpp" COPYONLY)
list(REMOVE_ITEM MT_SOURCES "${MT}/core/macro_def.cpp")
list(APPEND MT_SOURCES "${OVERLAY}/core/macro_def.cpp" "${MT}/render.cpp")
# GCC on Xtensa exposes an uninitialized accumulator for malformed UTF-8.
# Initialize it; this legacy helper is still not a validating reader decoder.
file(READ "${MT}/utils/utf.cpp" utf_source)
string(FIND "${utf_source}" "unsigned int codepoint;" utf_found)
if(utf_found EQUAL -1)
    message(FATAL_ERROR "MicroTeX UTF accumulator patch context changed")
endif()
string(REPLACE "unsigned int codepoint;" "unsigned int codepoint = 0;" utf_source "${utf_source}")
string(REPLACE "#include \"utf.h\"" "#include \"utils/utf.h\"" utf_source "${utf_source}")
file(MAKE_DIRECTORY "${OVERLAY}/utils")
file(WRITE "${OVERLAY}/utils/utf.cpp" "${utf_source}")
list(REMOVE_ITEM MT_SOURCES "${MT}/utils/utf.cpp")
list(APPEND MT_SOURCES "${OVERLAY}/utils/utf.cpp")
# Only required FreeType objects, not the full upstream archive. gzip supports
# the SFNT loader's link requirements; no external zlib library is linked.
set(FT_SOURCES)
foreach(src base/ftbase base/ftinit base/ftsystem base/ftdebug base/ftglyph
            base/ftbitmap base/ftbbox base/ftmm sfnt/sfnt truetype/truetype raster/raster
            psnames/psnames gzip/ftgzip)
    list(APPEND FT_SOURCES "${FT}/src/${src}.c")
endforeach()
set(MATH_VENDOR_SOURCES ${MT_SOURCES} ${FT_SOURCES} "${XML}/tinyxml2.cpp")
