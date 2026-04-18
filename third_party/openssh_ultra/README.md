OpenSSH UltraTerminal integration
=================================

This subtree is the UltraTerminal SSH glue layer. The checked-in
`openssh-10.3p1` source tree is the selected upstream OpenSSH portable source;
UltraTerminal must not depend on an external `ssh` executable for its SSH
transport.

The public boundary is `utssh`:

* `SSH Interactive Shell` is represented as an in-process OpenSSH session
  channel with PTY semantics.
* `SSH Tunnel` is represented as an in-process OpenSSH `direct-tcpip` channel
  carrying the existing emulator byte stream.
* The `comssh` driver owns one `UTSSH_SESSION` and routes decrypted channel
  bytes through the same UltraTerminal receive pipeline used by Direct TCP.

The old process bridge, executable discovery, and command-line construction
helpers have been removed. The current implementation is fail-closed until the
OpenSSH packet, authentication, host-key, and channel state machines are fully
linked behind this API.
