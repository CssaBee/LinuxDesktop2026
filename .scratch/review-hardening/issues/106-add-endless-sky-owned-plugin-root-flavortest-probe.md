# 106 - Add Endless Sky Owned Plugin Root FlavorTest Probe

**What to build:** Add endless-sky/endless-sky as a review-hardening FlavorTest
probe under `docs/FlavorTests/endless_sky/`, focused on installation-owned
resource plugins, user-owned config/data plugins, and plugin diagnostics.

**Blocked by:** 105 - Add Amiberry Root Topology FlavorTest Probe.

**Status:** implemented

- [x] Add `docs/FlavorTests/endless_sky/src/endless_sky_flavor.hpp` and
  `docs/FlavorTests/endless_sky/src/endless_sky_flavor.cpp`.
- [x] Add `docs/FlavorTests/endless_sky/test/endless_sky_flavor_tests.cpp`
  and wire it through `docs/FlavorTests/CMakeLists.txt` with
  `add_flavor_product(endless_sky)`.
- [x] Update `docs/FlavorTests/README.md` with the covered Endless Sky slice.
- [x] Update `docs/FlavorTests/SOURCES.md` with source anchors and wiki
  evidence.
- [x] Update `docs/FlavorTests/API_FRICTION.md` with any typed named plugin
  path-set or product-diagnostic friction.

## Implementation Notes

The probe now covers separate bundled and user-owned plugin roots, download
target selection, save/preference placement, and the API friction around
component-root scoping. The resource plugin root uses an `ld_root` named
resource root; the user plugin root uses a named data root so the product
keeps plugin scan and diagnostics policy.

## Source Anchors

Use endless-sky/endless-sky as the source anchor. Record exact source snapshots
in `SOURCES.md` before implementation.

- Plugin documentation:
  `https://github.com/endless-sky/endless-sky/wiki/CreatingPlugins`
- Resource plugin roots: Linux `/usr/share/games/endless-sky/plugins/`,
  Windows executable-adjacent `plugins\`, and macOS app-bundle resources.
- User plugin roots: Linux `~/.local/share/endless-sky/plugins/`, Windows
  `%APPDATA%\endless-sky\plugins\`, and macOS Application Support.
- Plugin diagnostics: `errors.txt` beside the user plugin root, including the
  documented two-instance contention behavior.
- Plugin payload conventions: product-owned metadata, data, images, sounds,
  shaders, zip support, and dependency/conflict interpretation.

## Implementation Shape

Create a small Endless-Sky-facing adapter that models plugin-root planning and
diagnostic placement without loading game content:

- `RuntimeEnvironment`: platform defaults, executable directory, resource root,
  user data root, and process environment.
- `PluginRootPolicy`: product rules for resource-owned versus user-owned
  plugins, zip/folder eligibility, and plugin-root precedence.
- `PluginDiscoveryPlan`: Endless Sky vocabulary for resource plugin root, user
  plugin root, errors file, selected candidates, disabled candidates, and
  diagnostics.
- `PluginDiagnosticsWriter`: models first-error truncation and existing writer
  contention as product diagnostics while using LinuxDesktop2026 only for safe
  path placement if needed.

LinuxDesktop2026 should provide root and named plugin-path candidate reporting.
Endless Sky should continue to own plugin metadata syntax, dependency/conflict
rules, content loading, zip handling, and gameplay diagnostics.

## Implemented Tests

- Linux resolves separate installation-owned and user-owned plugin roots with
  different writability labels.
- Downloadable plugins target the user-owned plugin root rather than bundled
  resources.
- User plugin diagnostics place `errors.txt` beside the user-owned plugin root.
- Saves and preferences stay in Endless Sky data vocabulary.
- Product plugin metadata, dependencies, conflicts, zip layout, and content
  folder rules stay outside LinuxDesktop2026.
- API friction notes judge whether typed named plugin path sets are adequate or
  whether product-owned plugin roots need a narrower helper.

## Future Expansion

- Add Windows executable-adjacent and macOS app-bundle resource plugin probes
  against explicit platform defaults.
- Model duplicate or contended diagnostics writers as Endless-Sky-shaped
  warnings without implying `ld_settings` solves multi-process whole-file
  coordination.

## Out Of Scope

- Building or porting Endless Sky.
- Implementing plugin parsing, zip loading, sprite/image/sound validation, or
  gameplay diagnostics.
- Adding Endless Sky plugins as a first-class `ld_paths` plugin kind.
- Treating multi-instance `errors.txt` behavior as a generic settings commit
  contract.
- Treating this as maintained consumer proof.

## Evidence Fit

FlavorTests and source review should catch this: the risk is treating "plugin
directory" as a single path family when real products split install-owned
resources from user-owned plugin data and diagnostics.

## Release Gate

Does not block `0.2.0`.
