set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_ultraterminal_tool_prefix "x86_64-w64-mingw32")
set(ULTRATERMINAL_LLVM_MINGW_ROOT "$ENV{LLVM_MINGW_ROOT}" CACHE PATH "Path to the llvm-mingw installation root.")
get_filename_component(_ultraterminal_repo_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

set(_ultraterminal_search_roots
    "${ULTRATERMINAL_LLVM_MINGW_ROOT}"
    "${_ultraterminal_repo_root}/.tools/llvm-mingw"
    "/opt/homebrew/opt/llvm-mingw"
    "/usr/local/opt/llvm-mingw"
    "/opt/homebrew"
    "/usr/local"
)

foreach(_root IN LISTS _ultraterminal_search_roots)
    if(NOT _root)
        continue()
    endif()

    if(EXISTS "${_root}/bin/${_ultraterminal_tool_prefix}-clang")
        set(ULTRATERMINAL_LLVM_MINGW_BIN "${_root}/bin")
        break()
    endif()
endforeach()

if(NOT ULTRATERMINAL_LLVM_MINGW_BIN)
    message(FATAL_ERROR "Unable to locate llvm-mingw. Set LLVM_MINGW_ROOT or install llvm-mingw in Homebrew.")
endif()

set(CMAKE_C_COMPILER "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-clang")
set(CMAKE_CXX_COMPILER "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-clang++")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_AR "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-ar")
set(CMAKE_RANLIB "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-ranlib")
set(CMAKE_DLLTOOL "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-dlltool")

if(EXISTS "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-windres")
    set(CMAKE_RC_COMPILER "${ULTRATERMINAL_LLVM_MINGW_BIN}/${_ultraterminal_tool_prefix}-windres")
elseif(EXISTS "${ULTRATERMINAL_LLVM_MINGW_BIN}/llvm-rc")
    set(CMAKE_RC_COMPILER "${ULTRATERMINAL_LLVM_MINGW_BIN}/llvm-rc")
else()
    message(FATAL_ERROR "Unable to locate a resource compiler in ${ULTRATERMINAL_LLVM_MINGW_BIN}.")
endif()

set(CMAKE_FIND_ROOT_PATH "${ULTRATERMINAL_LLVM_MINGW_BIN}/..")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
