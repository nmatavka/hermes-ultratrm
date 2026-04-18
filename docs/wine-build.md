# Building UltraTerminal for Wine on macOS

## Dependencies

Install the cross-toolchain and generator:

```sh
/opt/homebrew/bin/brew install llvm-mingw ninja
```

If `llvm-mingw` is installed somewhere else, export `LLVM_MINGW_ROOT` before configuring.

## Configure

```sh
cmake --preset macos-wine-x64-debug
cmake --preset macos-wine-x64-release
```

## Build

```sh
cmake --build --preset macos-wine-x64-release
```

The produced Windows binaries land in `build/macos-wine-x64-release/bin/`.
With TN3270 enabled, the output directory also includes `UltraTerminal3287.exe`,
the sibling 3287 printer helper used by TN3270 associated printer sessions.

## Smoke Test

Run the compiled app under Wine from the output directory so sibling DLLs are resolved next to the EXE:

```sh
bash scripts/run-wine.sh
```

Run the TN3270 artifact smoke test:

```sh
bash scripts/smoke-tn3270.sh
```

Equivalent manual launch:

```sh
cd build/macos-wine-x64-release/bin
wine ./UltraTerminal.exe
```

## What to Verify

- `UltraTerminal.exe` starts under Wine.
- `UltraTerminal.dll`, `hticons.dll`, and `htrn_jis.dll` are present beside the EXE.
- the main window opens
- Help, New Connection, and Emulator Settings dialogs open
- translation DLL discovery still finds `htrn_jis.dll`
- Winsock/Telnet settings are reachable
- TN3270 appears in the Terminal Emulation list
- `UltraTerminal3287.exe` is present beside the main EXE and DLLs
- TAPI remains enabled and degrades gracefully if Wine cannot service the telephony APIs
