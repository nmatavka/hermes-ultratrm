if(NOT DEFINED ULTRATERMINAL_SOURCE_DIR)
    message(FATAL_ERROR "ULTRATERMINAL_SOURCE_DIR is required")
endif()

set(_root "${ULTRATERMINAL_SOURCE_DIR}")

set(_text_exts
    ".c" ".cc" ".cpp" ".cxx"
    ".h" ".hh" ".hpp" ".hxx"
    ".rc" ".rc2" ".def" ".manifest"
    ".cmake" ".txt" ".in" ".mki" ".mak"
)

file(GLOB_RECURSE _files LIST_DIRECTORIES FALSE "${_root}/*")

string(CONCAT _tchar "T" "CHAR")
string(CONCAT _utchar "_T" "CHAR")
string(CONCAT _lptstr "LP" "TSTR")
string(CONCAT _lpctstr "LPC" "TSTR")
string(CONCAT _ptstr "P" "TSTR")
string(CONCAT _pctstr "PC" "TSTR")
string(CONCAT _text "TE" "XT")
string(CONCAT _text_mixed "Te" "xt")
string(CONCAT _underscore_t "_T")
string(CONCAT _char_mixed "CHAR_" "MIXED")
string(CONCAT _char_narrow "CHAR_" "NARROW")
string(CONCAT _far_east "FAR_" "EAST")
string(CONCAT _dbcs "DB" "CS")
string(CONCAT _mbcs "MB" "CS")
string(CONCAT _char_next_ex_a "CharNextEx" "A")
string(CONCAT _char_prev "Char" "Prev")
string(CONCAT _is_dbcs "Is" "D" "B" "CS" "LeadByte")
string(CONCAT _legacy_htchar "ht" "char" "\\.h")
string(CONCAT _legacy_tchar_header "t" "char" "\\.h")
string(CONCAT _legacy_tchar_source "t" "char" "\\.c")
string(CONCAT _legacy_copy_tchar "copy_" "t" "char")
string(CONCAT _legacy_tchar_to "t" "char" "_to")
string(CONCAT _legacy_to_tchar "_to_" "t" "char")
string(CONCAT _legacy_tchars "T" "CHARS")
string(CONCAT _legacy_etext "E" "TEXT")

set(_word_pattern
    "(^|[^A-Za-z0-9_])(${_tchar}|${_utchar}|${_lptstr}|${_lpctstr}|${_ptstr}|${_pctstr}|${_char_mixed}|${_char_narrow}|${_far_east}|${_dbcs}|${_mbcs}|${_char_next_ex_a}|${_char_prev}|${_is_dbcs})([^A-Za-z0-9_]|$)")
set(_macro_pattern
    "(^|[^A-Za-z0-9_])(${_text}|${_text_mixed}|${_underscore_t})[ \t]*\\(")
set(_legacy_name_pattern
    "(^|[^A-Za-z0-9_])(${_legacy_htchar}|${_legacy_tchar_header}|${_legacy_tchar_source}|${_legacy_copy_tchar}|${_legacy_tchar_to}|${_legacy_to_tchar}|${_legacy_tchars}|${_legacy_etext})([^A-Za-z0-9_]|$)")

set(_failures)

foreach(_file IN LISTS _files)
    file(RELATIVE_PATH _rel "${_root}" "${_file}")

    if(_rel MATCHES "^build/")
        continue()
    endif()

    get_filename_component(_ext "${_file}" EXT)
    list(FIND _text_exts "${_ext}" _ext_index)
    if(_ext_index LESS 0)
        get_filename_component(_name "${_file}" NAME)
        if(NOT _name MATCHES "^(CMakeLists\\.txt|sources|makefile|Makefile|common\\.mki|common\\.mak)$")
            continue()
        endif()
    endif()

    file(READ "${_file}" _content)
    if(_content MATCHES "${_word_pattern}" OR
            _content MATCHES "${_macro_pattern}" OR
            _content MATCHES "${_legacy_name_pattern}")
        list(APPEND _failures "${_rel}")
    endif()
endforeach()

if(_failures)
    list(REMOVE_DUPLICATES _failures)
    string(REPLACE ";" "\n  " _failure_text "${_failures}")
    message(FATAL_ERROR "Unicode-only purge check failed in:\n  ${_failure_text}")
endif()
