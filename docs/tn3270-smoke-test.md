# UltraTerminal TN3270 Smoke Test

This checklist covers the repeatable TN3270/TN3270E build and launch path for
Wine on macOS and Windows 11.

## Build

```sh
cmake --preset macos-wine-x64-debug
cmake --preset macos-wine-x64-release
cmake --build --preset macos-wine-x64-release
```

The release output should contain:

- `UltraTerminal.exe`
- `UltraTerminal.dll`
- `UltraTerminal3287.exe`
- `hticons.dll`
- `htrn_jis.dll`

## Launch

Run from the output directory so the sibling DLLs and helper EXE resolve:

```sh
bash scripts/run-wine.sh
```

For a non-interactive artifact check, run:

```sh
bash scripts/smoke-tn3270.sh
```

This verifies the expected PE32+ x86-64 outputs, checks that the release
OpenSSL cross-build produced `libssl.a` and `libcrypto.a`, inspects the main
DLL imports for Help/TAPI/Winsock, and starts the 3287 helper under Wine with
its quiet flag.

## Manual Checks

- Open Terminal Emulation settings and confirm `TN3270` is listed.
- Create a TCP/IP connection to a TN3270E host or test server.
- Confirm Telnet negotiation reaches BINARY, EOR, TERMINAL-TYPE, and TN3270E
  device-type/function subnegotiation.
- Confirm a formatted 3270 screen paints into the terminal window, including
  protected fields, intensity, reverse video, underline, blanking, and cursor
  placement.
- Confirm Enter, Clear, PF1-PF24, PA1, SysReq, Tab, BackTab, arrows,
  Backspace, Delete, and protected-field-aware character entry behave as
  expected.
- Confirm normal VT/Telnet connections still receive and send character data.
- Confirm `UltraTerminal3287.exe` starts manually from the output directory;
  use this before wiring an associated-printer host test.
- If TLS is enabled in settings, confirm the selected OpenSSL cross-build target
  has produced Windows `libssl` and `libcrypto` before running TLS host tests.

## Notes

The native adapter is in-process and uses Telnet EOR framing. TN3270 records are
routed to the emulator directly; normal Telnet sessions continue through the
existing character stream.

Current parity gaps are intentionally visible instead of hidden: TLS settings and
OpenSSL artifacts are present, but live socket wrapping still needs host-level
TLS testing; IND$FILE menu commands reach the TN3270 runtime hook, but full DFT
send/receive is not complete; the 3287 helper is launchable, but full host-driven
SCS printer protocol still needs the real printer session loop.
