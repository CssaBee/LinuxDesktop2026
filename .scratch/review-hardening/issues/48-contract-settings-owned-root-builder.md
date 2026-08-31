# 48 - Contract Settings-Owned Root Builder

**What to build:** Once public root topology exists, settings should stop being
the public home for generic named/component root construction and should keep
only settings lifecycle behavior. By the end of this ticket, no generic
root-topology compatibility debt should remain in `ld_settings`.

**Blocked by:** 47 - Add Public Root Topology Surface.

**Status:** implemented

- [x] Settings root resolution consumes the public root topology result where
  that reduces duplication without moving hydration or writes out of settings.
- [x] Settings-specific overlays, config layers, default hydration, safe writes,
  and backup behavior remain discoverable from the settings module.
- [x] Public settings APIs that duplicate generic root topology are removed or
  reshaped to settings-only vocabulary under the documented pre-1.0 break.
- [x] FlavorTests and the maintained Notepad++ proof branch show that product
  adapters no longer need settings vocabulary just to resolve app-owned roots.
- [x] API friction notes are updated to show which root seams moved and which
  stayed product-owned.
- [x] `ld_settings` does not require callers to link or include `ld_root` unless
  they use settings behavior that actually needs root topology.
- [x] No deprecated generic root-builder aliases, duplicate public structs, or
  temporary adapter layers remain after the migration.
- [x] Build/install/package tests prove the minimal dependency choices: paths
  only, root without settings, and settings without unrelated plugin/path-set
  APIs.

## Breaking Change

Remove the settings-owned generic root builder as a public API once the
equivalent root-domain surface exists. Keep only settings-specific root inputs
that are genuinely part of config lifecycle: settings overrides, sync-config
overrides, portable/local activation, managed/enforced layers, hydration, safe
writes, and diagnostics translation.

## Final Boundary

By the end of task 48, the dependency graph should be boring and explicit:

- `ld_paths` depends on `ld_core`.
- `ld_root` depends on `ld_paths` and `ld_core`.
- `ld_settings` depends on `ld_root` only for root topology it uses, and on
  lower-level modules needed for settings lifecycle.

No user should need to bring in `ld_settings` for path discovery or app-root
topology, and no user should need to bring in `ld_root` for simple path-family,
resource-location, path-list, or plugin-search-root work.

## Implementation Notes

Implemented as a pre-1.0 breaking contraction. C++ settings no longer exposes
generic named/component root topology; that vocabulary moved to
`linuxdesktop::root`. Settings keeps `root_options`, `root_report`,
`root_builder`, config layers, portable/settings overlays, hydration, writes,
and diagnostic translation.

The C settings ABI no longer exposes named/component root request or report
structs. C callers use `ld_root_c.h` for topology and `ld_settings_c.h` for
settings lifecycle behavior. Notepad++, qBittorrent, KiCad, KeePassXC, and the
maintained Notepad++ proof now use `ld_root` for topology where needed.
