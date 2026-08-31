# 43 - Extract Settings Root Internals

**What to build:** Split `ld_settings` root implementation internals along the
documented ownership boundary while preserving the existing public
`ld_settings` API.

**Blocked by:** 42 - Design Root Module Boundary.

**Status:** done

- [x] Root resolution code is separated from hydration and write mechanics.
- [x] Settings-specific overlays remain in `ld_settings`: portable marker,
  settings override, sync-config override, config layers, named settings roots,
  component roots, and settings diagnostics.
- [x] Generic path selection continues to delegate to `ld_paths`.
- [x] Public `ld_settings` headers remain source-compatible unless the boundary
  design explicitly justifies a pre-1.0 break.
- [x] Characterization tests cover the moved public behavior before or during
  extraction.
- [x] FlavorTests and the Notepad++ proof branch behavior remain unchanged.

## Problem Statement

The current `src/settings.cpp` implementation mixes root resolution,
config-layer construction, hydration, and common writes in one file. That makes
future ownership mistakes easy and hides where a possible `ld_root` extraction
would begin.

## Result

Extracted root resolution and root topology internals into the private
`src/settings_roots.cpp` translation unit, with shared private helpers declared
in `src/settings_internal.hpp`. `src/settings.cpp` now keeps the public
stringification surface plus hydration and write mechanics, while
`resolve_app_roots()` continues to own settings-specific overlays and delegates
generic path-family selection to `ld_paths`. No public `ld_settings` headers or
C ABI types changed.

Verification:

- `cmake --build LinuxDesktop2026/build`
- `ctest --test-dir LinuxDesktop2026/build --output-on-failure`
- `cmake --build LinuxDesktop2026/docs/FlavorTests/build`
- `ctest --test-dir LinuxDesktop2026/docs/FlavorTests/build --output-on-failure`
- `cmake --install LinuxDesktop2026/build --prefix /tmp/linuxdesktop2026-crossport-prefix`
- `cmake --build LinuxDesktop2026-crossport-notepadpp/build`
- `ctest --test-dir LinuxDesktop2026-crossport-notepadpp/build --output-on-failure`
