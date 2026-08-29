# 24 — Add Settings Root Construction Helpers

**What to build:** Add C++17-friendly helpers or builders for common
`ld_settings` root and named-root requests so call sites do not rely on dense
aggregate literals, unlabeled booleans, or repeated enum combinations.

**Blocked by:** 22 — Translate Library Reports At Product Boundaries.

**Status:** ready-for-agent

- [ ] Common config, state, cache, session, log, profile-data, plugin-config,
  and component-root requests can be expressed through named helpers or a small
  builder API.
- [ ] Helpers target call-site clarity only; they must not create a second
  application-profile ontology above `root_purpose`, `persistence_class`, and
  existing root requests.
- [ ] Existing low-level `root_options`, `named_root_request`, and
  `component_root_request` remain available for unusual cases.
- [ ] KeePassXC, qBittorrent, KiCad, FreeCAD, and Notepad++ FlavorTests prefer
  the helper layer where it reduces aggregate literals and unlabeled booleans
  without moving application policy into the library.
- [ ] Tests cover helper output equivalence with the existing explicit request
  shapes.
