# 108 - Reduce Named-Root String Lookup Friction

**What to build:** Decide and, if accepted for `0.2.0`, add a C++ convenience
path that reduces duplicate string-key request and lookup for named roots
without moving product-owned naming into LinuxDesktop2026.

**Blocked by:** 107 - Add Minifox Portable Launcher FlavorTest Probe.

**Status:** implemented

- [x] Inventory every FlavorTest and maintained proof that requests a named root
  and later looks it up by repeating the same string key.
- [x] Decide whether the fix should be typed request handles, a request-bound
  lookup wrapper, generated lookup accessors, or documentation-only guidance.
- [x] Keep product vocabulary outside the public LinuxDesktop2026 API: names
  such as `xml-config`, `session`, `plugin-config`, `local-settings`, `colors`,
  `toolbars`, and `project-backups` remain caller-owned labels.
- [x] If an API helper is added, prove it with Notepad++, qBittorrent, KiCad,
  and at least one larger component-map-shaped FlavorTest.
- [x] Preserve the existing string-key surface for callers that construct named
  roots dynamically.
- [x] Update API friction notes, root examples, and the `0.2.0` support matrix
  with the decision.

## Evidence Anchor

`docs/FlavorTests/API_FRICTION.md` records repeated named-root friction:

- Notepad++ requests `xml-config`, `session`, and `plugin-config`, then must
  look those roots up by the same strings.
- qBittorrent log placement is still a named-root request and lookup pair.
- KiCad's three-root map is readable, but the same pattern would get noisy for
  a larger component map.

## Boundary

This ticket is not permission to add product-specific root kinds. The helper
must reduce accidental string duplication while keeping LinuxDesktop2026 at the
topology boundary. Project-specific fallback rules, plugin metadata, media
libraries, service profiles, and merge policies remain product-owned.

## Implementation

Added `linuxdesktop::root::named_root_handle`, created with
`make_named_root_handle(named_root_request)`. Static callers can pass the handle
to `request_builder::named_root()` and later call
`find_named_root(report, handle)`. The handle owns the original request and
label; no product labels become LinuxDesktop2026 enums or first-class root
kinds. The original `find_named_root(report, std::string)` surface remains for
dynamic root maps.

The proof updates Notepad++, qBittorrent, KiCad, and Amiberry. Amiberry acts as
the larger static map because it declares four product roots while keeping
content fan-out and plugin fallback order outside LinuxDesktop2026.

## Release Gate

Blocks `0.2.0`; implemented.
