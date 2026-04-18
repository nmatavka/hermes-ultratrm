#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="${1:-${root_dir}/build/macos-wine-x64-release}"
bin_dir="${build_dir}/bin"
toolchain_root="${LLVM_MINGW_ROOT:-${root_dir}/../.local/toolchains/llvm-mingw-20260324-ucrt-macos-universal}"
objdump_bin="${toolchain_root}/bin/x86_64-w64-mingw32-objdump"

export MVK_CONFIG_LOG_LEVEL="${MVK_CONFIG_LOG_LEVEL:-0}"
export WINEDEBUG="${WINEDEBUG:--all}"

required_bins=(
    UltraTerminal.exe
    UltraTerminal.dll
    UltraTerminal3287.exe
    hticons.dll
    htrn_jis.dll
)

if [[ ! -d "${bin_dir}" ]]; then
    echo "Build output not found: ${bin_dir}" >&2
    echo "Run: cmake --preset macos-wine-x64-release && cmake --build --preset macos-wine-x64-release" >&2
    exit 1
fi

for name in "${required_bins[@]}"; do
    if [[ ! -f "${bin_dir}/${name}" ]]; then
        echo "Missing expected output: ${bin_dir}/${name}" >&2
        exit 1
    fi
done

file "${bin_dir}/UltraTerminal.exe" \
     "${bin_dir}/UltraTerminal.dll" \
     "${bin_dir}/UltraTerminal3287.exe" \
     "${bin_dir}/hticons.dll" \
     "${bin_dir}/htrn_jis.dll"

openssl_lib_dir="${build_dir}/tn3270-openssl/install/lib64"
if [[ -f "${openssl_lib_dir}/libssl.a" && -f "${openssl_lib_dir}/libcrypto.a" ]]; then
    ls -l "${openssl_lib_dir}/libssl.a" "${openssl_lib_dir}/libcrypto.a"
else
    echo "OpenSSL static libraries not found in ${openssl_lib_dir}" >&2
    exit 1
fi

if [[ -x "${objdump_bin}" ]]; then
    "${objdump_bin}" -p "${bin_dir}/UltraTerminal.dll" | rg "DLL Name|hhctrl|TAPI|WSOCK"
else
    echo "Skipping import-table check; objdump not found at ${objdump_bin}" >&2
fi

if command -v "${WINE:-wine}" >/dev/null 2>&1; then
    "${WINE:-wine}" "${bin_dir}/UltraTerminal3287.exe" --quiet
else
    echo "Skipping Wine helper launch; wine is not on PATH" >&2
fi
