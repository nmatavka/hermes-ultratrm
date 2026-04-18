set(_files
    "${PROJECT_SOURCE_DIR}/comssh/comssh.c"
    "${PROJECT_SOURCE_DIR}/third_party/openssh_ultra/openssh_ultra.c"
    "${PROJECT_SOURCE_DIR}/third_party/openssh_ultra/openssh_ultra.h")

set(_forbidden
    "ssh.exe"
    "ULTRATERMINAL_SSH_EXE"
    "CreateProcess"
    "SshStartProcess"
    "SshBuildCommand"
    "SshPipeReaderThread")

foreach(_file IN LISTS _files)
    file(READ "${_file}" _text)
    foreach(_pattern IN LISTS _forbidden)
        if(_text MATCHES "${_pattern}")
            message(FATAL_ERROR
                "External SSH process bridge pattern '${_pattern}' found in ${_file}")
        endif()
    endforeach()
endforeach()
