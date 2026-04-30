cmake_minimum_required(VERSION 3.30)

project(ultraterminal LANGUAGES C CXX RC)

include(CTest)
find_package(Git QUIET)

add_custom_target(ultraterminal_unicode_purge_check ALL
    COMMAND "${CMAKE_COMMAND}"
        -DULTRATERMINAL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_unicode_purge.cmake"
    VERBATIM
)

add_test(NAME unicode_token_purge
    COMMAND "${CMAKE_COMMAND}"
        -DULTRATERMINAL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_unicode_purge.cmake"
)

add_test(NAME rexx_export_surface
    COMMAND "${CMAKE_COMMAND}"
        -DULTRATERMINAL_SOURCE_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}
        -DULTRATERMINAL_BIN_DIR:PATH=${CMAKE_BINARY_DIR}/bin
        -DULTRATERMINAL_OBJDUMP:FILEPATH=${CMAKE_OBJDUMP}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_rexx_exports.cmake"
)

option(ULTRATERMINAL_ENABLE_HTML_HELP "Enable HTML Help integration." ON)
option(ULTRATERMINAL_STRICT_FEATURES "Keep legacy feature set enabled during modernization." ON)
option(ULTRATERMINAL_BUILD_LEGACY_CNCTSTD "Build the legacy cnctstd driver DLL." OFF)
option(ULTRATERMINAL_ENABLE_IBM3270 "Enable native IBM 3270 host support: tn3270 over Telnet and ssh3270 over SSH tunnel." ON)
option(ULTRATERMINAL_ENABLE_IBM3270_TLS "Enable IBM 3270 direct-transport TLS configuration and OpenSSL build integration." ON)
option(ULTRATERMINAL_BUILD_IBM3270_OPENSSL "Add an ExternalProject helper for cross-building OpenSSL for the Windows target." ON)
option(ULTRATERMINAL_ENABLE_IBM5250 "Enable native IBM 5250 host support: tn5250 over Telnet and ssh5250 over SSH tunnel." ON)
option(ULTRATERMINAL_ENABLE_IBM5250_TLS "Enable IBM 5250 direct-transport TLS settings and OpenSSL build integration." ON)
option(ULTRATERMINAL_BUILD_IBM5250_OPENSSL "Use the shared ExternalProject helper for IBM 5250 OpenSSL support." ON)
option(ULTRATERMINAL_ENABLE_VIDEOTEX "Enable modern Videotex/Viewdata-over-IP terminal support." ON)
option(ULTRATERMINAL_ENABLE_SSH "Enable SSH transport settings and COM transport boundary." ON)
option(ULTRATERMINAL_BUILD_OPENSSH "Prepare the in-process OpenSSH build integration." ON)
option(ULTRATERMINAL_ENABLE_SSH_AGENT "Enable SSH agent settings for the OpenSSH transport." ON)
option(ULTRATERMINAL_ENABLE_SSH_FORWARDING "Enable SSH forwarding settings for tunnel transports." ON)
option(ULTRATERMINAL_SYNC_OPENSSH_SOURCE "Synchronize the OpenSSH portable source tree during configure." ON)
option(ULTRATERMINAL_SYNC_REGINA_SOURCE "Synchronize the Regina source tree during configure." ON)
option(ULTRATERMINAL_USE_REGINA_STUB "Build the minimal Regina compatibility stub instead of the real Regina interpreter." OFF)

set(ULTRATERMINAL_OPENSSH_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh-portable"
    CACHE PATH "Local OpenSSH portable source directory used for the in-process SSH build.")
set(ULTRATERMINAL_OPENSSH_REPOSITORY
    "https://github.com/openssh/openssh-portable.git"
    CACHE STRING "Git repository used to synchronize the OpenSSH portable source tree.")
set(ULTRATERMINAL_REGINA_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/regina"
    CACHE PATH "Local Regina Rexx source directory used for the regina.dll build.")
set(ULTRATERMINAL_REGINA_REPOSITORY
    "https://github.com/0branch/regina.git"
    CACHE STRING "Git repository used to synchronize the Regina Rexx source tree.")

set(ULTRATERMINAL_IBM3270_OPENSSL_VERSION "3.5.0" CACHE STRING "OpenSSL version used by the IBM 3270/OpenSSH OpenSSL helper.")
set(ULTRATERMINAL_IBM3270_OPENSSL_URL
    "https://www.openssl.org/source/openssl-${ULTRATERMINAL_IBM3270_OPENSSL_VERSION}.tar.gz"
    CACHE STRING "OpenSSL source URL used by the IBM host/OpenSSH OpenSSL helper.")

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

set(ULTRATERMINAL_COMMON_DEFINES
    WIN32
    _WIN32
    _WINDOWS
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    NT
    _NTSDK
    SDK_CODE
    NO_SMARTHEAP
    NT_EDITION
    ISOLATION_AWARE_ENABLED
)

set(ULTRATERMINAL_UNICODE_DEFINES
    UNICODE
    _UNICODE
    ULTRATERMINAL_UNICODE_ONLY
)

set(ULTRATERMINAL_INTERNAL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set(ULTRATERMINAL_ARCHIVE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")

function(ultraterminal_collect_nt_sources out_var dir_name)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/sources" _source_lines)

    set(_sources)
    set(_in_block FALSE)

    foreach(_line IN LISTS _source_lines)
        string(STRIP "${_line}" _line)

        if(_line STREQUAL "" OR _line MATCHES "^#")
            continue()
        endif()

        if(NOT _in_block)
            if(_line MATCHES "^SOURCES=")
                set(_in_block TRUE)
                string(REGEX REPLACE "^SOURCES=" "" _line "${_line}")
            else()
                continue()
            endif()
        endif()

        set(_continued FALSE)
        if(_line MATCHES "\\\\$")
            set(_continued TRUE)
            string(REGEX REPLACE "\\\\$" "" _line "${_line}")
            string(STRIP "${_line}" _line)
        endif()

        if(NOT _line STREQUAL "")
            string(STRIP "${_line}" _line)
            separate_arguments(_tokens UNIX_COMMAND "${_line}")
            foreach(_token IN LISTS _tokens)
                string(REPLACE "\\" "/" _token "${_token}")
                cmake_path(NORMAL_PATH _token OUTPUT_VARIABLE _normalized_token)
                list(APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/${_normalized_token}")
            endforeach()
        endif()

        if(NOT _continued)
            set(_in_block FALSE)
        endif()
    endforeach()

    set(${out_var} "${_sources}" PARENT_SCOPE)
endfunction()

function(ultraterminal_filter_sources out_var)
    set(options DROP_RC)
    cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

    set(_filtered)
    foreach(_src IN LISTS ARG_UNPARSED_ARGUMENTS)
        if(ARG_DROP_RC AND _src MATCHES "\\.rc$")
            continue()
        endif()
        list(APPEND _filtered "${_src}")
    endforeach()

    set(${out_var} "${_filtered}" PARENT_SCOPE)
endfunction()

function(ultraterminal_apply_common_settings target_name)
    target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${target_name} PRIVATE
        ${ULTRATERMINAL_COMMON_DEFINES}
        ULTRATERMINAL_STRICT_FEATURES=$<IF:$<BOOL:${ULTRATERMINAL_STRICT_FEATURES}>,1,0>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_HTML_HELP}>:ULTRATERMINAL_ENABLE_HTML_HELP=1>
        $<$<NOT:$<BOOL:${ULTRATERMINAL_ENABLE_HTML_HELP}>>:ULTRATERMINAL_DISABLE_HTML_HELP>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_IBM3270}>:INCL_IBM3270;ULTRATERMINAL_ENABLE_IBM3270=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_IBM3270_TLS}>:ULTRATERMINAL_ENABLE_IBM3270_TLS=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_IBM5250}>:INCL_IBM5250;ULTRATERMINAL_ENABLE_IBM5250=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_IBM5250_TLS}>:ULTRATERMINAL_ENABLE_IBM5250_TLS=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_VIDEOTEX}>:INCL_VIDEOTEX;ULTRATERMINAL_ENABLE_VIDEOTEX=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_SSH}>:INCL_SSH;ULTRATERMINAL_ENABLE_SSH=1>
        $<$<BOOL:${ULTRATERMINAL_BUILD_OPENSSH}>:ULTRATERMINAL_BUILD_OPENSSH=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_SSH_AGENT}>:ULTRATERMINAL_ENABLE_SSH_AGENT=1>
        $<$<BOOL:${ULTRATERMINAL_ENABLE_SSH_FORWARDING}>:ULTRATERMINAL_ENABLE_SSH_FORWARDING=1>
        $<$<CONFIG:Debug>:WIN_DEBUG;VERSION=WIN_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:WIN_RELEASE;VERSION=WIN_RELEASE>
    )
    target_compile_options(${target_name} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-std=gnu89>
        $<$<COMPILE_LANGUAGE:C>:-fcommon>
        $<$<COMPILE_LANGUAGE:C>:-fms-extensions>
        $<$<COMPILE_LANGUAGE:C>:-Wno-unknown-pragmas>
        $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
        $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>
        $<$<COMPILE_LANGUAGE:C>:-Wno-format>
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++98>
        $<$<COMPILE_LANGUAGE:CXX>:-fms-extensions>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unknown-pragmas>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
    )
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ULTRATERMINAL_INTERNAL_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${ULTRATERMINAL_INTERNAL_OUTPUT_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${ULTRATERMINAL_ARCHIVE_OUTPUT_DIR}"
    )
endfunction()

function(ultraterminal_apply_unicode_settings target_name)
    target_compile_definitions(${target_name} PRIVATE
        ${ULTRATERMINAL_UNICODE_DEFINES}
    )
endfunction()

function(ultraterminal_embed_manifest out_var target_name manifest_path resource_id)
    file(TO_CMAKE_PATH "${manifest_path}" _manifest_path)
    set(_manifest_rc "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_manifest.rc")
    file(GENERATE OUTPUT "${_manifest_rc}" CONTENT
        "${resource_id} 24 \"${_manifest_path}\"\n")
    set_source_files_properties("${_manifest_rc}" PROPERTIES
        OBJECT_DEPENDS "${manifest_path}")
    set(${out_var} "${_manifest_rc}" PARENT_SCOPE)
endfunction()

function(ultraterminal_sync_git_source sync_label repository source_dir required_file)
    if(ULTRATERMINAL_STRICT_FEATURES)
        set(_strict_sync ON)
    else()
        set(_strict_sync OFF)
    endif()

    if(NOT GIT_FOUND)
        if(ULTRATERMINAL_STRICT_FEATURES)
            message(FATAL_ERROR "Git is required to synchronize ${sync_label}, but no git executable was found.")
        else()
            message(WARNING "Skipping ${sync_label} synchronization because no git executable was found.")
            return()
        endif()
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DSOURCE_DIR=${source_dir}
            -DREPOSITORY=${repository}
            -DSYNC_LABEL=${sync_label}
            -DREQUIRED_FILE=${required_file}
            -DSTRICT_SYNC=${_strict_sync}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sync_git_source.cmake"
        RESULT_VARIABLE _sync_rc
        OUTPUT_VARIABLE _sync_out
        ERROR_VARIABLE _sync_err
    )

    if(NOT _sync_rc EQUAL 0)
        if(ULTRATERMINAL_STRICT_FEATURES)
            message(FATAL_ERROR "Failed to synchronize ${sync_label}: ${_sync_out}${_sync_err}")
        else()
            message(WARNING "Failed to synchronize ${sync_label}: ${_sync_out}${_sync_err}")
        endif()
    endif()
endfunction()

if(ULTRATERMINAL_ENABLE_SSH AND ULTRATERMINAL_BUILD_OPENSSH AND ULTRATERMINAL_SYNC_OPENSSH_SOURCE)
    ultraterminal_sync_git_source(
        "OpenSSH portable source"
        "${ULTRATERMINAL_OPENSSH_REPOSITORY}"
        "${ULTRATERMINAL_OPENSSH_SOURCE_DIR}"
        "${ULTRATERMINAL_OPENSSH_SOURCE_DIR}/readconf.c"
    )
endif()

if(NOT ULTRATERMINAL_USE_REGINA_STUB AND ULTRATERMINAL_SYNC_REGINA_SOURCE)
    ultraterminal_sync_git_source(
        "Regina Rexx source"
        "${ULTRATERMINAL_REGINA_REPOSITORY}"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.c"
    )
endif()

if(NOT ULTRATERMINAL_USE_REGINA_STUB AND NOT EXISTS "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.c")
    if(ULTRATERMINAL_STRICT_FEATURES)
        message(FATAL_ERROR
            "The real Regina source tree is required at ${ULTRATERMINAL_REGINA_SOURCE_DIR} "
            "unless ULTRATERMINAL_USE_REGINA_STUB is enabled.")
    else()
        message(WARNING
            "The real Regina source tree is unavailable at ${ULTRATERMINAL_REGINA_SOURCE_DIR}; "
            "falling back to the Regina stub.")
        set(ULTRATERMINAL_USE_REGINA_STUB ON)
    endif()
endif()

set(ULTRATERMINAL_REGINA_VERSION_DATE "25 Jun 2022")
set(ULTRATERMINAL_REGINA_VERSION_MAJOR "3")
set(ULTRATERMINAL_REGINA_VERSION_MINOR "9")
set(ULTRATERMINAL_REGINA_VERSION_RELEASE "5")
set(ULTRATERMINAL_REGINA_VERSION_SUPP "")
if(NOT ULTRATERMINAL_USE_REGINA_STUB AND EXISTS "${ULTRATERMINAL_REGINA_SOURCE_DIR}/regina.ver")
    file(STRINGS "${ULTRATERMINAL_REGINA_SOURCE_DIR}/regina.ver" _regina_ver_lines)
    foreach(_regina_line IN LISTS _regina_ver_lines)
        if(_regina_line MATCHES "^VER_DATE=\"(.*)\"$")
            set(ULTRATERMINAL_REGINA_VERSION_DATE "${CMAKE_MATCH_1}")
        elseif(_regina_line MATCHES "^VER_MAJOR=([0-9]+)$")
            set(ULTRATERMINAL_REGINA_VERSION_MAJOR "${CMAKE_MATCH_1}")
        elseif(_regina_line MATCHES "^VER_MINOR=([0-9]+)$")
            set(ULTRATERMINAL_REGINA_VERSION_MINOR "${CMAKE_MATCH_1}")
        elseif(_regina_line MATCHES "^VER_RELEASE=([0-9]+)$")
            set(ULTRATERMINAL_REGINA_VERSION_RELEASE "${CMAKE_MATCH_1}")
        elseif(_regina_line MATCHES "^VER_SUPP=\"(.*)\"$")
            set(ULTRATERMINAL_REGINA_VERSION_SUPP "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()

if(NOT ULTRATERMINAL_USE_REGINA_STUB)
    if(EXISTS "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.h")
        file(READ "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.h" _regina_rexxsaa)
        string(REPLACE "#  define EXTNAME(n) __declspec(dllexport)"
                       "#  define EXTNAME(n)"
                       _regina_rexxsaa
                       "${_regina_rexxsaa}")
        string(REPLACE "#  define EXTNAME(n) __declspec(dllimport)"
                       "#  define EXTNAME(n)"
                       _regina_rexxsaa
                       "${_regina_rexxsaa}")
        file(WRITE "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.h" "${_regina_rexxsaa}")
    endif()

    if(EXISTS "${ULTRATERMINAL_REGINA_SOURCE_DIR}/extern.h")
        file(READ "${ULTRATERMINAL_REGINA_SOURCE_DIR}/extern.h" _regina_extern)
        string(REPLACE " __declspec(dllexport);" ";"
                       _regina_extern
                       "${_regina_extern}")
        string(REPLACE " __declspec(dllimport);" ";"
                       _regina_extern
                       "${_regina_extern}")
        file(WRITE "${ULTRATERMINAL_REGINA_SOURCE_DIR}/extern.h" "${_regina_extern}")
    endif()
endif()

set(ULTRATERMINAL_HTMLHELP_IMPORT_LIB "")
if(ULTRATERMINAL_ENABLE_HTML_HELP)
    find_library(ULTRATERMINAL_HTMLHELP_IMPORT_LIB NAMES htmlhelp)

    if(NOT ULTRATERMINAL_HTMLHELP_IMPORT_LIB)
        if(NOT CMAKE_DLLTOOL)
            message(FATAL_ERROR "HTML Help is enabled but no htmlhelp import library was found and CMAKE_DLLTOOL is unavailable.")
        endif()

        set(ULTRATERMINAL_HTMLHELP_IMPORT_LIB "${CMAKE_CURRENT_BINARY_DIR}/libhtmlhelp.a")
        add_custom_command(
            OUTPUT "${ULTRATERMINAL_HTMLHELP_IMPORT_LIB}"
            COMMAND "${CMAKE_DLLTOOL}" -d "${CMAKE_CURRENT_SOURCE_DIR}/cmake/htmlhelp.def" -D hhctrl.ocx -l "${ULTRATERMINAL_HTMLHELP_IMPORT_LIB}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/htmlhelp.def"
            VERBATIM
        )
        add_custom_target(ultraterminal_htmlhelp_import DEPENDS "${ULTRATERMINAL_HTMLHELP_IMPORT_LIB}")
    endif()
endif()

if(((ULTRATERMINAL_ENABLE_IBM3270 AND ULTRATERMINAL_ENABLE_IBM3270_TLS AND ULTRATERMINAL_BUILD_IBM3270_OPENSSL) OR
        (ULTRATERMINAL_ENABLE_IBM5250 AND ULTRATERMINAL_ENABLE_IBM5250_TLS AND ULTRATERMINAL_BUILD_IBM5250_OPENSSL) OR
        (ULTRATERMINAL_ENABLE_SSH AND ULTRATERMINAL_BUILD_OPENSSH)))
    include(ExternalProject)
    find_program(ULTRATERMINAL_MAKE_PROGRAM NAMES gmake make)

    if(ULTRATERMINAL_MAKE_PROGRAM)
        set(ULTRATERMINAL_OPENSSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/openssl/install")
        set(ULTRATERMINAL_OPENSSL_SSL_LIB "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/lib64/libssl.a")
        set(ULTRATERMINAL_OPENSSL_CRYPTO_LIB "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/lib64/libcrypto.a")
        set(ULTRATERMINAL_IBM3270_OPENSSL_SSL_LIB "${ULTRATERMINAL_OPENSSL_SSL_LIB}")
        set(ULTRATERMINAL_IBM3270_OPENSSL_CRYPTO_LIB "${ULTRATERMINAL_OPENSSL_CRYPTO_LIB}")
        ExternalProject_Add(ultraterminal_openssl
            PREFIX "${CMAKE_BINARY_DIR}/openssl"
            INSTALL_DIR "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}"
            URL "${ULTRATERMINAL_IBM3270_OPENSSL_URL}"
            CONFIGURE_COMMAND
                "${CMAKE_COMMAND}" -E env
                    "CC=${CMAKE_C_COMPILER}"
                    "AR=${CMAKE_AR}"
                    "RANLIB=${CMAKE_RANLIB}"
                    "WINDRES=${CMAKE_RC_COMPILER}"
                    <SOURCE_DIR>/Configure mingw64 no-shared no-tests no-asm
                    "--prefix=<INSTALL_DIR>"
            BUILD_COMMAND "${ULTRATERMINAL_MAKE_PROGRAM}"
            INSTALL_COMMAND "${ULTRATERMINAL_MAKE_PROGRAM}" install_sw
            BUILD_BYPRODUCTS
                "${ULTRATERMINAL_OPENSSL_SSL_LIB}"
                "${ULTRATERMINAL_OPENSSL_CRYPTO_LIB}"
        )
    else()
        if(ULTRATERMINAL_STRICT_FEATURES)
            message(FATAL_ERROR "Terminal TLS is enabled but no make/gmake program was found for the OpenSSL cross-build.")
        else()
            message(WARNING "OpenSSL helper requested, but no make/gmake program was found.")
        endif()
    endif()
endif()

ultraterminal_collect_nt_sources(CNCTTAPI_ALL cncttapi)
ultraterminal_collect_nt_sources(COMSTD_ALL comstd)
ultraterminal_collect_nt_sources(COMWSOCK_ALL comwsock)
ultraterminal_collect_nt_sources(EMU_ALL emu)
ultraterminal_collect_nt_sources(TDLL_ALL tdll)
ultraterminal_collect_nt_sources(TERM_ALL term)
ultraterminal_collect_nt_sources(EXT_ALL ext)
ultraterminal_collect_nt_sources(HTRN_JIS_ALL htrn_jis)
ultraterminal_collect_nt_sources(XFER_ALL xfer)

ultraterminal_filter_sources(CNCTTAPI_SOURCES DROP_RC ${CNCTTAPI_ALL})
ultraterminal_filter_sources(COMSTD_SOURCES DROP_RC ${COMSTD_ALL})
ultraterminal_filter_sources(COMWSOCK_SOURCES DROP_RC ${COMWSOCK_ALL})
ultraterminal_filter_sources(EMU_SOURCES DROP_RC ${EMU_ALL})
ultraterminal_filter_sources(TDLL_SOURCES DROP_RC ${TDLL_ALL})
ultraterminal_filter_sources(TERM_SOURCES DROP_RC ${TERM_ALL})
ultraterminal_filter_sources(EXT_SOURCES DROP_RC ${EXT_ALL})
ultraterminal_filter_sources(HTRN_JIS_SOURCES DROP_RC ${HTRN_JIS_ALL})
ultraterminal_filter_sources(XFER_SOURCES DROP_RC ${XFER_ALL})

add_library(cncttapi STATIC ${CNCTTAPI_SOURCES})
add_library(comstd STATIC ${COMSTD_SOURCES})
add_library(comwsock STATIC ${COMWSOCK_SOURCES})
target_compile_definitions(comwsock PRIVATE MULTITHREAD)
if(TARGET ultraterminal_openssl)
    add_dependencies(comwsock ultraterminal_openssl)
    target_include_directories(comwsock PRIVATE
        "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/include"
    )
endif()
add_library(comssh STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/comssh/comssh.c"
)
set(OPENSSH_PORTABLE_DIR "${ULTRATERMINAL_OPENSSH_SOURCE_DIR}")
if(ULTRATERMINAL_ENABLE_SSH AND ULTRATERMINAL_BUILD_OPENSSH AND
        NOT EXISTS "${OPENSSH_PORTABLE_DIR}/readconf.c")
    message(FATAL_ERROR
        "ULTRATERMINAL_BUILD_OPENSSH is ON, but the OpenSSH portable "
        "source tree is missing at ${OPENSSH_PORTABLE_DIR}. Enable "
        "ULTRATERMINAL_SYNC_OPENSSH_SOURCE or populate the configured "
        "source directory to continue the in-process SSH build.")
endif()
set(OPENSSH_LIBSSH_SOURCES
    "${OPENSSH_PORTABLE_DIR}/ssh_api.c"
    "${OPENSSH_PORTABLE_DIR}/ssherr.c"
    "${OPENSSH_PORTABLE_DIR}/ssherr-libcrypto.c"
    "${OPENSSH_PORTABLE_DIR}/sshbuf.c"
    "${OPENSSH_PORTABLE_DIR}/sshkey.c"
    "${OPENSSH_PORTABLE_DIR}/sshbuf-getput-basic.c"
    "${OPENSSH_PORTABLE_DIR}/sshbuf-misc.c"
    "${OPENSSH_PORTABLE_DIR}/sshbuf-getput-crypto.c"
    "${OPENSSH_PORTABLE_DIR}/krl.c"
    "${OPENSSH_PORTABLE_DIR}/bitmap.c"
    "${OPENSSH_PORTABLE_DIR}/authfd.c"
    "${OPENSSH_PORTABLE_DIR}/authfile.c"
    "${OPENSSH_PORTABLE_DIR}/canohost.c"
    "${OPENSSH_PORTABLE_DIR}/channels.c"
    "${OPENSSH_PORTABLE_DIR}/cipher.c"
    "${OPENSSH_PORTABLE_DIR}/cipher-aes.c"
    "${OPENSSH_PORTABLE_DIR}/cipher-aesctr.c"
    "${OPENSSH_PORTABLE_DIR}/cleanup.c"
    "${OPENSSH_PORTABLE_DIR}/compat.c"
    "${OPENSSH_PORTABLE_DIR}/fatal.c"
    "${OPENSSH_PORTABLE_DIR}/hostfile.c"
    "${OPENSSH_PORTABLE_DIR}/log.c"
    "${OPENSSH_PORTABLE_DIR}/match.c"
    "${OPENSSH_PORTABLE_DIR}/moduli.c"
    "${OPENSSH_PORTABLE_DIR}/nchan.c"
    "${OPENSSH_PORTABLE_DIR}/packet.c"
    "${OPENSSH_PORTABLE_DIR}/ssh-pkcs11.c"
    "${OPENSSH_PORTABLE_DIR}/progressmeter.c"
    "${OPENSSH_PORTABLE_DIR}/readpass.c"
    "${OPENSSH_PORTABLE_DIR}/xmalloc.c"
    "${OPENSSH_PORTABLE_DIR}/addr.c"
    "${OPENSSH_PORTABLE_DIR}/addrmatch.c"
    "${OPENSSH_PORTABLE_DIR}/atomicio.c"
    "${OPENSSH_PORTABLE_DIR}/dispatch.c"
    "${OPENSSH_PORTABLE_DIR}/mac.c"
    "${OPENSSH_PORTABLE_DIR}/misc.c"
    "${OPENSSH_PORTABLE_DIR}/utf8.c"
    "${OPENSSH_PORTABLE_DIR}/rijndael.c"
    "${OPENSSH_PORTABLE_DIR}/ssh-ecdsa.c"
    "${OPENSSH_PORTABLE_DIR}/ssh-rsa.c"
    "${OPENSSH_PORTABLE_DIR}/dh.c"
    "${OPENSSH_PORTABLE_DIR}/msg.c"
    "${OPENSSH_PORTABLE_DIR}/entropy.c"
    "${OPENSSH_PORTABLE_DIR}/umac.c"
    "${OPENSSH_PORTABLE_DIR}/umac128.c"
    "${OPENSSH_PORTABLE_DIR}/smult_curve25519_ref.c"
    "${OPENSSH_PORTABLE_DIR}/poly1305.c"
    "${OPENSSH_PORTABLE_DIR}/chacha.c"
    "${OPENSSH_PORTABLE_DIR}/cipher-chachapoly.c"
    "${OPENSSH_PORTABLE_DIR}/cipher-chachapoly-libcrypto.c"
    "${OPENSSH_PORTABLE_DIR}/ssh-ed25519.c"
    "${OPENSSH_PORTABLE_DIR}/digest-openssl.c"
    "${OPENSSH_PORTABLE_DIR}/digest-libc.c"
    "${OPENSSH_PORTABLE_DIR}/hmac.c"
    "${OPENSSH_PORTABLE_DIR}/ed25519.c"
    "${OPENSSH_PORTABLE_DIR}/kex.c"
    "${OPENSSH_PORTABLE_DIR}/kex-names.c"
    "${OPENSSH_PORTABLE_DIR}/kexdh.c"
    "${OPENSSH_PORTABLE_DIR}/kexgex.c"
    "${OPENSSH_PORTABLE_DIR}/kexecdh.c"
    "${OPENSSH_PORTABLE_DIR}/kexc25519.c"
    "${OPENSSH_PORTABLE_DIR}/kexgexc.c"
    "${OPENSSH_PORTABLE_DIR}/kexgexs.c"
    "${OPENSSH_PORTABLE_DIR}/kexsntrup761x25519.c"
    "${OPENSSH_PORTABLE_DIR}/kexmlkem768x25519.c"
    "${OPENSSH_PORTABLE_DIR}/sntrup761.c"
    "${OPENSSH_PORTABLE_DIR}/kexgen.c"
    "${OPENSSH_PORTABLE_DIR}/platform-pledge.c"
    "${OPENSSH_PORTABLE_DIR}/platform-tracing.c"
    "${OPENSSH_PORTABLE_DIR}/platform.c"
    "${OPENSSH_PORTABLE_DIR}/platform-misc.c"
    "${OPENSSH_PORTABLE_DIR}/sshbuf-io.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/arc4random.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/arc4random_uniform.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/base64.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/basename.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bcrypt_pbkdf.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/blowfish.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bsd-asprintf.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bsd-getline.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bsd-getentropy.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bsd-misc.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/bsd-snprintf.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/dirname.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/explicit_bzero.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/fake-rfc2553.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/fmt_scaled.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/freezero.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/getopt_long.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/memmem.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/mktemp.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/openssl-compat.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/port-prngd.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/readpassphrase.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/reallocarray.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/recallocarray.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/setenv.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strcasestr.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strlcat.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strlcpy.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strmode.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strndup.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strnlen.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strsep.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/strtonum.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/timingsafe_bcmp.c"
    "${OPENSSH_PORTABLE_DIR}/openbsd-compat/vis.c"
)
set(OPENSSH_CLIENT_SOURCES
    "${OPENSSH_PORTABLE_DIR}/readconf.c"
)
add_library(openssh_libssh STATIC ${OPENSSH_LIBSSH_SOURCES})
add_library(openssh_client STATIC ${OPENSSH_CLIENT_SOURCES})
add_library(openssh_compat STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh_ultra/openssh_ultra.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh_ultra/ut_win32_compat.c"
)
add_library(openssh_ultra_glue INTERFACE)
add_library(ibm3270_32xx STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/suite3270/tn3270_protocol.c"
)
add_library(ibm3270_core STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/suite3270/ultra_runtime.c"
)
add_library(ibm3270_ultra_glue STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/suite3270/ultra_glue.c"
)
add_library(ibm3270_pr3287 STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/suite3270/ultra_pr3287.c"
)
set(IBM5250_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/conf.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/dbuffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/debug.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/display.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/field.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/macro.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/menu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/printsession.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/record.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/scrollbar.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/scs.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/session.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/sslstream.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/stream.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/telnetstr.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/terminal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/utility.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/version.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/window.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250/wtd.c"
)
add_library(ibm5250_core STATIC ${IBM5250_CORE_SOURCES})
add_library(ibm5250_ultra_glue STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/ultra_stream.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/ultra_runtime.c"
)
add_library(ibm5250_print STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/ultra_printer.c"
)
add_library(vidtex_core STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vidtex/ultra_vidtex.c"
)
add_library(vidtex_ultra_glue STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vidtex/ultra_glue.c"
)
add_library(emu STATIC ${EMU_SOURCES})
add_library(ext2 STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/defclsf.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/pageext.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/fspage.c"
)
add_library(xfer STATIC ${XFER_SOURCES})

foreach(_target IN ITEMS cncttapi comstd comwsock comssh openssh_libssh openssh_client openssh_compat ibm3270_32xx ibm3270_core ibm3270_ultra_glue ibm3270_pr3287 ibm5250_core ibm5250_ultra_glue ibm5250_print vidtex_core vidtex_ultra_glue emu ext2 xfer)
    ultraterminal_apply_common_settings(${_target})
endforeach()

foreach(_target IN ITEMS cncttapi comstd comwsock comssh openssh_compat emu ext2 xfer)
    ultraterminal_apply_unicode_settings(${_target})
endforeach()

target_include_directories(comssh PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh_ultra"
)
target_link_libraries(comssh PRIVATE openssh_compat)

foreach(_target IN ITEMS openssh_libssh openssh_client openssh_compat)
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh_ultra/posix"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssh_ultra"
        "${OPENSSH_PORTABLE_DIR}"
        "${OPENSSH_PORTABLE_DIR}/openbsd-compat"
        "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/include"
    )
    target_compile_definitions(${_target} PRIVATE
        HAVE_CONFIG_H
        WIN32_LEAN_AND_MEAN
    )
    # Microsoft OpenSSH carries compatibility declarations that intentionally
    # shadow MinGW CRT/Winsock headers and OpenSSL 3 deprecations.
    target_compile_options(${_target} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wno-inconsistent-dllimport>
        $<$<COMPILE_LANGUAGE:C>:-Wno-macro-redefined>
        $<$<COMPILE_LANGUAGE:C>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANGUAGE:C>:-Wno-include-next-absolute-path>
        $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
        $<$<COMPILE_LANGUAGE:C>:-Wno-compare-distinct-pointer-types>
    )
endforeach()

target_link_libraries(openssh_client PRIVATE openssh_libssh)
target_link_libraries(openssh_compat PRIVATE openssh_client openssh_libssh)
if(TARGET ultraterminal_openssl)
    add_dependencies(openssh_libssh ultraterminal_openssl)
    target_link_libraries(openssh_libssh PRIVATE
        "${ULTRATERMINAL_OPENSSL_CRYPTO_LIB}"
        crypt32
        ws2_32
    )
endif()

foreach(_target IN ITEMS vidtex_core vidtex_ultra_glue)
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vidtex"
    )
endforeach()

if(NOT ULTRATERMINAL_USE_REGINA_STUB)
    set(ULTRATERMINAL_REGINA_CORE_SOURCES
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/funcs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/builtin.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/error.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/variable.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/interprt.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/debug.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/dbgfuncs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/memory.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/parsing.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/files.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/misc.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/unxfuncs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/arxfuncs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/cmsfuncs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/os2funcs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/shell.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxext.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/stack.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/extstack.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/tracing.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/interp.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/cmath.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/convert.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/strengs.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/library.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/strmath.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/signals.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/macros.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/envir.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/expr.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/instore.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxbif.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/yaccsrc.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/lexsrc.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/options.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/os_win.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/wrappers.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/os_other.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/mt_win32.c"
    )
    set(ULTRATERMINAL_REGINA_SUPPORT_SOURCES
        ${ULTRATERMINAL_REGINA_CORE_SOURCES}
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/client.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/mygetopt.c"
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/rexxsaa.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/regina_host/regina_compat.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/regina_host/rexx_rlib.c"
    )

    set_source_files_properties(
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/yaccsrc.c"
        PROPERTIES COMPILE_DEFINITIONS "YYMAXDEPTH=10000"
    )

    add_library(regina_support_static STATIC ${ULTRATERMINAL_REGINA_SUPPORT_SOURCES})
    add_library(regina_threadsafe_objs OBJECT ${ULTRATERMINAL_REGINA_SUPPORT_SOURCES})

    foreach(_target IN ITEMS regina_support_static regina_threadsafe_objs)
        target_include_directories(${_target} PRIVATE
            "${ULTRATERMINAL_REGINA_SOURCE_DIR}"
            "${ULTRATERMINAL_REGINA_SOURCE_DIR}/common"
        )
        target_compile_definitions(${_target} PRIVATE
            WIN32
            REGINA_BITS=64
            REGINA_VERSION_DATE="${ULTRATERMINAL_REGINA_VERSION_DATE}"
            REGINA_VERSION_MAJOR="${ULTRATERMINAL_REGINA_VERSION_MAJOR}"
            REGINA_VERSION_MINOR="${ULTRATERMINAL_REGINA_VERSION_MINOR}"
            REGINA_VERSION_RELEASE="${ULTRATERMINAL_REGINA_VERSION_RELEASE}"
            REGINA_VERSION_SUPP="${ULTRATERMINAL_REGINA_VERSION_SUPP}"
        )
        target_compile_options(${_target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-std=gnu89>
            $<$<COMPILE_LANGUAGE:C>:-fcommon>
            $<$<COMPILE_LANGUAGE:C>:-fms-extensions>
            $<$<COMPILE_LANGUAGE:C>:-funsigned-char>
            $<$<COMPILE_LANGUAGE:C>:-Wno-unknown-pragmas>
            $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
            $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>
            $<$<COMPILE_LANGUAGE:C>:-Wno-format>
            $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-to-int-cast>
            $<$<COMPILE_LANGUAGE:C>:-Wno-int-to-pointer-cast>
            $<$<COMPILE_LANGUAGE:C>:-Wno-void-pointer-to-int-cast>
            $<$<COMPILE_LANGUAGE:C>:-Wno-int-to-void-pointer-cast>
            $<$<COMPILE_LANGUAGE:C>:-Wno-null-dereference>
        )
        set_target_properties(${_target} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${ULTRATERMINAL_ARCHIVE_OUTPUT_DIR}"
        )
    endforeach()

    target_compile_definitions(regina_threadsafe_objs PRIVATE
        DYNAMIC
        __REGINA_DLL
    )

    add_library(regina SHARED
        $<TARGET_OBJECTS:regina_threadsafe_objs>
        "${ULTRATERMINAL_REGINA_SOURCE_DIR}/regina_w32_mingw_dll.def"
    )
    ultraterminal_apply_common_settings(regina)
    set_target_properties(regina PROPERTIES
        OUTPUT_NAME "regina"
        PREFIX ""
    )
    target_link_libraries(regina PRIVATE
        regina_support_static
        kernel32
        user32
        advapi32
        wsock32
    )
else()
    add_library(regina SHARED
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/regina_stub/regina_stub.c"
    )
    ultraterminal_apply_common_settings(regina)
    set_target_properties(regina PROPERTIES
        OUTPUT_NAME "regina"
        PREFIX ""
    )
    target_link_libraries(regina PRIVATE
        kernel32
        user32
    )
endif()

foreach(_target IN ITEMS ibm5250_core ibm5250_ultra_glue ibm5250_print)
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250"
    )
    target_compile_options(${_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=gnu99>)
endforeach()

if(TARGET ultraterminal_openssl AND ULTRATERMINAL_ENABLE_IBM5250_TLS)
    add_dependencies(ibm5250_core ultraterminal_openssl)
    add_dependencies(ibm5250_ultra_glue ultraterminal_openssl)
    target_compile_definitions(ibm5250_core PRIVATE ULTRATERMINAL_IBM5250_HAVE_OPENSSL=1)
    target_compile_definitions(ibm5250_ultra_glue PRIVATE ULTRATERMINAL_IBM5250_HAVE_OPENSSL=1)
    target_include_directories(ibm5250_core PRIVATE
        "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/include"
    )
    target_include_directories(ibm5250_ultra_glue PRIVATE
        "${ULTRATERMINAL_OPENSSL_INSTALL_DIR}/include"
    )
endif()

ultraterminal_embed_manifest(ULTRATERMINAL_DLL_MANIFEST_RC ultraterminal "${CMAKE_CURRENT_SOURCE_DIR}/tdll/UltraTerminal.manifest" 2)
ultraterminal_embed_manifest(HTRN_JIS_MANIFEST_RC htrn_jis "${CMAKE_CURRENT_SOURCE_DIR}/htrn_jis/htrn_jis.manifest" 2)

add_library(ultraterminal SHARED
    ${TDLL_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/tdll/rexxhapi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tdll/rexxhost.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tdll/rexxapis.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/comstd/comstd.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/term/UltraTerminal_resources.rc"
    "${ULTRATERMINAL_DLL_MANIFEST_RC}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tdll/UltraTerminal.def"
)
ultraterminal_apply_common_settings(ultraterminal)
ultraterminal_apply_unicode_settings(ultraterminal)
set_target_properties(ultraterminal PROPERTIES
    OUTPUT_NAME "UltraTerminal"
    PREFIX ""
)
if(TARGET ultraterminal_htmlhelp_import)
    add_dependencies(ultraterminal ultraterminal_htmlhelp_import)
endif()
if(TARGET ultraterminal_openssl)
    add_dependencies(ultraterminal ultraterminal_openssl)
endif()
target_link_libraries(ultraterminal PRIVATE
    "$<LINK_GROUP:RESCAN,ext2,emu,xfer,cncttapi,comstd,comwsock,comssh,openssh_compat,openssh_client,openssh_libssh,ibm3270_ultra_glue,ibm3270_core,ibm3270_32xx,ibm5250_ultra_glue,ibm5250_core,vidtex_ultra_glue,vidtex_core>"
    regina
    kernel32
    user32
    uuid
    advapi32
    winspool
    tapi32
    gdi32
    wsock32
    ws2_32
    comdlg32
    comctl32
    shell32
    ole32
    setupapi
    winmm
    "${ULTRATERMINAL_HTMLHELP_IMPORT_LIB}"
    ${ULTRATERMINAL_IBM3270_OPENSSL_SSL_LIB}
    ${ULTRATERMINAL_IBM3270_OPENSSL_CRYPTO_LIB}
    crypt32
)

add_library(hticons SHARED
    ${EXT_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/hticons.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/term/ver_ico.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/hticons.def"
)
ultraterminal_apply_common_settings(hticons)
ultraterminal_apply_unicode_settings(hticons)
set_target_properties(hticons PROPERTIES
    OUTPUT_NAME "hticons"
    PREFIX ""
)
target_link_libraries(hticons PRIVATE
    kernel32
    user32
    uuid
    advapi32
    comctl32
    shell32
)

add_library(htrn_jis SHARED
    ${HTRN_JIS_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/htrn_jis/htrn_jis.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/term/ver_jis.rc"
    "${HTRN_JIS_MANIFEST_RC}"
    "${CMAKE_CURRENT_SOURCE_DIR}/htrn_jis/htrn_jis.def"
)
ultraterminal_apply_common_settings(htrn_jis)
ultraterminal_apply_unicode_settings(htrn_jis)
set_target_properties(htrn_jis PROPERTIES
    OUTPUT_NAME "htrn_jis"
    PREFIX ""
)
target_link_libraries(htrn_jis PRIVATE
    ultraterminal
    kernel32
    user32
)

add_executable(ultraterminal_exe WIN32
    ${TERM_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/term/online.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/term/ver_exe.rc"
)
set_target_properties(ultraterminal_exe PROPERTIES OUTPUT_NAME "UltraTerminal")
ultraterminal_apply_common_settings(ultraterminal_exe)
ultraterminal_apply_unicode_settings(ultraterminal_exe)
target_link_options(ultraterminal_exe PRIVATE -municode)
target_link_libraries(ultraterminal_exe PRIVATE ultraterminal htrn_jis hticons)

if(ULTRATERMINAL_ENABLE_IBM3270)
    add_executable(ultraterminal_3287_printer WIN32
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/suite3270/ut3287.c"
    )
    set_target_properties(ultraterminal_3287_printer PROPERTIES
        OUTPUT_NAME "UltraTerminal3287"
    )
    ultraterminal_apply_common_settings(ultraterminal_3287_printer)
    target_link_libraries(ultraterminal_3287_printer PRIVATE
        ibm3270_pr3287
        kernel32
        user32
    )
endif()

if(ULTRATERMINAL_ENABLE_IBM5250)
    add_executable(ultraterminal_5250_printer WIN32
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/ut5250_printer.c"
    )
    set_target_properties(ultraterminal_5250_printer PROPERTIES
        OUTPUT_NAME "UltraTerminal5250Printer"
    )
    ultraterminal_apply_common_settings(ultraterminal_5250_printer)
    target_include_directories(ultraterminal_5250_printer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tn5250/lib5250"
    )
    target_link_libraries(ultraterminal_5250_printer PRIVATE
        "$<LINK_GROUP:RESCAN,ibm5250_print,ibm5250_ultra_glue,ibm5250_core>"
        ${ULTRATERMINAL_OPENSSL_SSL_LIB}
        ${ULTRATERMINAL_OPENSSL_CRYPTO_LIB}
        crypt32
        kernel32
        user32
        wsock32
        ws2_32
    )
endif()

if(ULTRATERMINAL_BUILD_LEGACY_CNCTSTD)
    add_library(cnctstd SHARED
        "${CMAKE_CURRENT_SOURCE_DIR}/cnctstd/cnctdrvs.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/cnctstd/cncts.c"
    )
    ultraterminal_apply_common_settings(cnctstd)
    target_link_libraries(cnctstd PRIVATE kernel32 user32 comstd ultraterminal)
endif()

if(BUILD_TESTING)
    add_test(NAME ssh_no_external_client
        COMMAND "${CMAKE_COMMAND}"
            "-DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_no_external_ssh.cmake")
endif()
