OpenSSH UltraTerminal integration
=================================

This subtree is the UltraTerminal SSH glue layer. The checked-in
`openssh-portable-latestw_all` source tree is the primary OpenSSH portable
source for the Windows PE build; UltraTerminal must not depend on an external
`ssh` executable for its SSH transport.

The public boundary is `utssh`:

* `SSH Interactive Shell` is represented as an in-process OpenSSH session
  channel with PTY semantics.
* `SSH Tunnel` is represented as an in-process OpenSSH `direct-tcpip` channel
  carrying the existing emulator byte stream.
* The `comssh` driver owns one `UTSSH_SESSION` and routes decrypted channel
  bytes through the same UltraTerminal receive pipeline used by Direct TCP.

The old process bridge, executable discovery, and command-line construction
helpers have been removed. The current implementation links OpenSSH packet,
authentication, host-key, and channel handling in-process behind this API.
