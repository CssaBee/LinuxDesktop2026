# 109 - Document Root Builder And Raw Options Selection Rule

**What to build:** Make the recommended `ld_root` construction style obvious to
real consumers by documenting when to use `request_builder` and when raw
`root::options` is the clearer integration point.

**Blocked by:** 108 - Reduce Named-Root String Lookup Friction.

**Status:** pending

- [ ] Compare current FlavorTest usage of `linuxdesktop::root::request_builder`
  and manual `linuxdesktop::root::options`.
- [ ] Write the selection rule into the root examples and roadmap: use
  `request_builder` for ordinary app topology, portable policy, overrides, and
  named roots; use raw options when the product already has a dense
  platform/root model that would be obscured by fluent construction.
- [ ] Update Notepad++ and KeePassXC friction notes with the final guidance.
- [ ] Check install-tree examples so downstream users see the same recommended
  style as the in-tree FlavorTests.
- [ ] Avoid introducing a new API unless the documentation pass proves a real
  contradiction in the current surface.

## Evidence Anchor

Notepad++ currently shows a split between manual `root::options` in the
maintained cross-port and `request_builder` in the in-tree FlavorTest. KeePassXC
is the opposite pressure: its XDG and roaming/local vocabulary is explicit
enough that raw options are clearer than a fluent builder.

## Release Gate

Blocks `0.2.0`. This is the small roadmap-clarity ticket that keeps the root
module from looking arbitrary after the FlavorTest round.
