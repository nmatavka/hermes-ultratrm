#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
default_build_dir="${script_dir}/../build/macos-wine-x64-release/bin"
binary_dir="${1:-${default_build_dir}}"

if [[ ! -d "${binary_dir}" ]]; then
    echo "Binary directory not found: ${binary_dir}" >&2
    exit 1
fi

export MVK_CONFIG_LOG_LEVEL="${MVK_CONFIG_LOG_LEVEL:-0}"
export WINEDEBUG="${WINEDEBUG:--all}"

cd "${binary_dir}"
exec "${WINE:-wine}" ./UltraTerminal.exe "${@:2}"
