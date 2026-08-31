# 43 - Extract Settings Root Internals

**What to build:** Split `ld_settings` root implementation internals along the
documented ownership boundary while preserving the existing public
`ld_settings` API.

**Blocked by:** 42 - Design Root Module Boundary.

**Status:** ready-for-agent

- [ ] Root resolution code is separated from hydration and write mechanics.
- [ ] Settings-specific overlays remain in `ld_settings`: portable marker,
  settings override, sync-config override, config layers, named settings roots,
  component roots, and settings diagnostics.
- [ ] Generic path selection continues to delegate to `ld_paths`.
- [ ] Public `ld_settings` headers remain source-compatible unless the boundary
  design explicitly justifies a pre-1.0 break.
- [ ] Characterization tests cover the moved public behavior before or during
  extraction.
- [ ] FlavorTests and the Notepad++ proof branch behavior remain unchanged.

## Problem Statement

The current `src/settings.cpp` implementation mixes root resolution,
config-layer construction, hydration, and common writes in one file. That makes
future ownership mistakes easy and hides where a possible `ld_root` extraction
would begin.

## Result

Pending.
