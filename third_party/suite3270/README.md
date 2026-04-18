# suite3270-Derived TN3270 Components

UltraTerminal uses a focused in-process subset of suite3270 4.5 protocol
definitions and address helpers for TN3270/TN3270E support.

The original suite3270 source is kept in the repository at `suite3270-4.5`.
This subtree intentionally copies only the protocol constants and small helper
logic needed by the UltraTerminal adapter, with the BSD notice preserved in
`LICENSE.md`. User-visible branding is UltraTerminal; protocol names such as
TN3270, TN3270E, 3278, 3287, and IND$FILE remain unchanged.
