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
file(READ "${MT}/core/macro_def.cpp" macro_source)
string(REPLACE "{ L##code, m(argc, name) }" "result.emplace(L##code, m(argc, name))" macro_source "${macro_source}")
string(REPLACE "{ L##code, m(argc, posOpts, name) }" "result.emplace(L##code, m(argc, posOpts, name))" macro_source "${macro_source}")
string(REPLACE "map<wstring, MacroInfo*> MacroInfo::_commands{"
    "map<wstring, MacroInfo*> MacroInfo::_commands = [] {\n  map<wstring, MacroInfo*> result;" macro_source "${macro_source}")
string(REGEX REPLACE "(mac\\([^\n]+\\))," "\\1;" macro_source "${macro_source}")
string(REPLACE "\n};" "\n  return result;\n}();" macro_source "${macro_source}")
file(WRITE "${OVERLAY}/core/macro_def.cpp" "${macro_source}")
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
# The upstream symbol initializer_list creates ~40 KiB of temporary
# string/shared_ptr pairs on ESP-IDF's pre-scheduler startup stack. Use a
# constant descriptor table and one insertion at a time instead. Keep every
# symbol, delimiter flag, and map's first-key-wins behavior.
file(READ "${MT}/res/builtin/tex_symbols.res.cpp" symbols_source)
set(symbols_begin "map<string, sptr<SymbolAtom>> SymbolAtom::_symbols = {")
string(FIND "${symbols_source}" "${symbols_begin}" symbols_found)
if(symbols_found EQUAL -1)
    message(FATAL_ERROR "MicroTeX symbol initializer patch context changed")
endif()
string(REPLACE "{ #name, sptr < SymbolAtom>(new SymbolAtom(#name, type, false)) }"
    "{ #name, type, false }" symbols_source "${symbols_source}")
string(REPLACE "{ #name, sptr < SymbolAtom>(new SymbolAtom(#name, type, true)) }"
    "{ #name, type, true }" symbols_source "${symbols_source}")
string(REPLACE "${symbols_begin}"
    "namespace {\nstruct SymbolSpec { const char *name; AtomType type; bool delimiter; };\nconstexpr SymbolSpec symbol_specs[] = {"
    symbols_source "${symbols_source}")
string(APPEND symbols_source "\n}\nmap<string, sptr<SymbolAtom>> SymbolAtom::_symbols = [] {\n  map<string, sptr<SymbolAtom>> result;\n  for (const auto &s : symbol_specs)\n    result.emplace(s.name, sptr<SymbolAtom>(new SymbolAtom(s.name, s.type, s.delimiter)));\n  return result;\n}();\n")
file(MAKE_DIRECTORY "${OVERLAY}/res/builtin")
file(WRITE "${OVERLAY}/res/builtin/tex_symbols.res.cpp" "${symbols_source}")
list(REMOVE_ITEM MT_SOURCES "${MT}/res/builtin/tex_symbols.res.cpp")
list(APPEND MT_SOURCES "${OVERLAY}/res/builtin/tex_symbols.res.cpp")
# Two other string maps also create large constructor-stack arrays. Their
# descriptor arrays contain only constant pointers/integers in flash.
foreach(table formula_mappings formula_def)
    if(table STREQUAL "formula_mappings")
        set(table_path "res/builtin/formula_mappings.res.cpp")
        set(table_begin "map<int, string> tex::Formula::_symbolFormulaMappings = {")
        set(table_type "map<int, string>")
        set(table_name "tex::Formula::_symbolFormulaMappings")
        set(key_type "int")
        set(value_type "const char *")
    else()
        set(table_path "core/formula_def.cpp")
        set(table_begin "map<wstring, wstring> Formula::_predefinedTeXFormulasAsString{")
        set(table_type "map<wstring, wstring>")
        set(table_name "Formula::_predefinedTeXFormulasAsString")
        set(key_type "const wchar_t *")
        set(value_type "const wchar_t *")
    endif()
    file(READ "${MT}/${table_path}" table_source)
    string(FIND "${table_source}" "${table_begin}" table_found)
    if(table_found EQUAL -1)
        message(FATAL_ERROR "MicroTeX ${table} patch context changed")
    endif()
    string(REPLACE "${table_begin}"
        "namespace {\nstruct Entry { ${key_type} key; ${value_type} value; };\nconstexpr Entry entries[] = {"
        table_source "${table_source}")
    string(APPEND table_source "\n}\n${table_type} ${table_name} = [] {\n  ${table_type} result;\n  for (const auto &e : entries) result.emplace(e.key, e.value);\n  return result;\n}();\n")
    file(WRITE "${OVERLAY}/${table_path}" "${table_source}")
    list(REMOVE_ITEM MT_SOURCES "${MT}/${table_path}")
    list(APPEND MT_SOURCES "${OVERLAY}/${table_path}")
endforeach()
# Only required FreeType objects, not the full upstream archive. gzip supports
# the SFNT loader's link requirements; no external zlib library is linked.
set(FT_SOURCES)
foreach(src base/ftbase base/ftinit base/ftsystem base/ftdebug base/ftglyph base/ftsynth
            base/ftbitmap base/ftbbox base/ftmm sfnt/sfnt truetype/truetype raster/raster
            psnames/psnames gzip/ftgzip)
    list(APPEND FT_SOURCES "${FT}/src/${src}.c")
endforeach()
set(MATH_VENDOR_SOURCES ${MT_SOURCES} ${FT_SOURCES} "${XML}/tinyxml2.cpp")
