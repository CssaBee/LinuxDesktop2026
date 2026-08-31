# 47 - Add Public Root Topology Surface

**What to build:** Applications with repeated root-topology needs should be able
to resolve user-owned and app-owned named roots without going through the
settings module, while simple apps can keep using direct path resolution. The
surface should be public, minimal, and independent enough that users do not
bring in settings just to resolve application-owned roots.

**Blocked by:** 46 - Separate Path Families From Location Roles.

**Status:** implemented

- [x] The public root topology API uses user-owned and app-owned root vocabulary
  rather than settings/config-only names.
- [x] `ld_root` is packaged/exported as its own public target and depends only
  on the lower-level modules it actually uses.
- [x] Named roots, component roots, app-local roots, install-adjacent roots, root
  source reporting, and root creation diagnostics are covered by public tests.
- [x] Notepad++, qBittorrent, and KiCad FlavorTests migrate their shared root
  setup through the new surface.
- [x] Walnut remains a direct path-resolution reference case, with no root
  topology dependency added.
- [x] OpenIPC Dashboard service-profile policy remains product-owned unless a
  second maintained consumer repeats the same service-root topology.
- [x] CMake install/export and consumer examples prove that a root-topology user
  links `ld_root` without also linking `ld_settings`.
- [x] The public API does not expose settings lifecycle concepts such as config
  layers, hydration, storage backends, default seeding, or safe writes.

## Breaking Change

Move generic named-root and component-root vocabulary out of the settings-owned
public surface for new callers. Any types promoted from the experimental
settings builder should be renamed or reshaped to root-domain language now,
before they become a broader compatibility promise.

## Dependency Boundary

`ld_root` may depend on `ld_paths` and `ld_core`, but not on `ld_settings`.
Users choose:

- `ld_paths` for platform families, locations, path lists, plugin search roots,
  and directory helpers.
- `ld_root` for user-owned/app-owned root topology.
- `ld_settings` for config layers, hydration, settings writes, backups, and
  settings-specific overlays.

## Implementation Notes

Implemented as public `linuxdesktop::root` with C++ `options`, `report`,
`request_builder`, `purpose_kind`, `ownership_kind`, named roots, component
roots, app-local roots, and install-adjacent resource support. Added
`ld_root_c.h` for C callers while preserving C-style `ld_root_*` prefixes.

`ld_root` is exported as `LinuxDesktop2026::ld_root`, depends on `ld_core` and
`ld_paths`, and has dedicated unit tests plus an install-tree consumer that
links root topology without `ld_settings`.
