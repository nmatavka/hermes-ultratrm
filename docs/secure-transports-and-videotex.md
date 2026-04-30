# UltraTerminal Secure Transport and Videotex Smoke Notes

This build keeps legacy `Viewdata` and `Minitel` modes unchanged and adds a
separate `Videotex` emulator for modern Videotex/Viewdata-over-IP services.

## Build

```sh
cmake --preset macos-wine-x64-debug
cmake --preset macos-wine-x64-release
cmake --build --preset macos-wine-x64-release
```

Expected PE outputs are in `build/macos-wine-x64-release/bin`.

## Videotex

1. Start UltraTerminal under Wine or Windows 11.
2. Open Terminal Emulation and select `Videotex`.
3. Connect with the existing TCP/IP path to a Videotex service endpoint.
4. Confirm Mode 7 frames render at 24x40.
5. Use `#` or Enter for the Viewdata `_` send key.
6. Use `Ctrl+R` or `F5` to toggle reveal.
7. Use `Ctrl+B` or `F6` to toggle flashing cells.
8. Use `F7` to display the current built-in profile note.

The built-in profile data mirrors the copied `vidtexrc`: NXTel, TeeFax,
Telstar fast/slow, CCL4, and Night Owl. Current UI plumbing exposes the mode
itself; profile selection is represented in saved settings and runtime code.

## SSH

The SSH COM driver is active and owns an in-process `UTSSH_SESSION`. It no
longer discovers or launches an external OpenSSH client. The old pipe bridge
has been removed so future SSH work happens behind the embedded OpenSSH
runtime boundary in `UltraTerminal.dll`.

Transport checks:

1. `SSH Interactive Shell` opens an in-process session channel and sends shell
   output through the normal terminal receive path.
2. `SSH Tunnel` opens an in-process `direct-tcpip` channel and sends decrypted
   tunnel bytes into the same emulator dispatch used by direct TCP.
3. IBM 3270 over SSH is labeled `ssh3270`; it can still carry host-required
   TN3270E negotiation and EOR framing before records reach the IBM 3270 runtime.
4. IBM 5250 over SSH is labeled `ssh5250`; it and Videotex receive raw tunnel
   bytes, matching their direct TCP integration.
5. Existing saved Direct TCP/Telnet sessions keep working.
6. Passwords and private-key passphrases are never saved by UltraTerminal.
7. The embedded runtime is currently fail-closed until OpenSSH packet,
   authentication, host-key, and channel handling are completed behind `utssh`.

## Regression Checks

Confirm these still work after a release build:

* Direct TCP/Telnet VT/ANSI sessions.
* Legacy Viewdata and Minitel modes.
* IBM 3270 and IBM 5250 direct connection paths (`tn3270`/`tn5250`).
* TAPI paths on Windows, with graceful Wine TAPI stubs.
* HTML Help and JIS translation DLL loading.
