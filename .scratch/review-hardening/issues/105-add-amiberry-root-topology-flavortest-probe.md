# 105 - Add Amiberry Root Topology FlavorTest Probe

**What to build:** Add BlitterStudio/Amiberry as a review-hardening FlavorTest
probe under `docs/FlavorTests/amiberry/`, focused on rich root topology,
portable fallbacks, environment overrides, and product-owned plugin roots.

**Blocked by:** 104 - Make Filesystem Resolution Non-Mutating By Default.

**Status:** implemented

- [x] Add `docs/FlavorTests/amiberry/src/amiberry_flavor.hpp` and
  `docs/FlavorTests/amiberry/src/amiberry_flavor.cpp`.
- [x] Add `docs/FlavorTests/amiberry/test/amiberry_flavor_tests.cpp` and wire
  it through `docs/FlavorTests/CMakeLists.txt` with
  `add_flavor_product(amiberry)`.
- [x] Update `docs/FlavorTests/README.md` with the covered Amiberry slice.
- [x] Update `docs/FlavorTests/SOURCES.md` with source anchors and wiki
  evidence.
- [x] Update `docs/FlavorTests/API_FRICTION.md` with any `ld_root` or
  `ld_paths` topology friction found by the slice.

## Implementation Notes

The probe now covers Linux split root topology, `base_content_path` fan-out,
existing-only `AMIBERRY_DATA_DIR` selection,
`AMIBERRY_HOME_DIR`/`AMIBERRY_CONFIG_DIR`/`AMIBERRY_PLUGINS_DIR` overrides, and
executable-adjacent portable mode through product-shaped `PathManager` code.
Plugin loading, emulator content scanning, and platform-specific binary suffix
selection remain Amiberry-owned.

## Source Anchors

Use BlitterStudio/Amiberry as the source anchor. Record exact source snapshots
in `SOURCES.md` before implementation.

- Amiberry directory documentation:
  `https://github.com/BlitterStudio/amiberry/wiki/Amiberry-directories`
- Environment overrides: `AMIBERRY_DATA_DIR`, `AMIBERRY_HOME_DIR`,
  `AMIBERRY_CONFIG_DIR`, and `AMIBERRY_PLUGINS_DIR`.
- Config/home/data layout: platform defaults, executable-relative fallbacks,
  Linux split layout, and `base_content_path` behavior.
- Plugin roots: install libdir, user plugin root, executable-relative plugin
  root, and platform-specific shared-library suffixes for product-owned
  optional plugins.

## Implementation Shape

Create a small Amiberry-facing adapter that models startup directory resolution
without building SDL, emulator, filesystem scanning, or dynamic loading:

- `RuntimeEnvironment`: executable directory, home directory, platform defaults,
  process environment, install data/lib roots, and portable-root request.
- `DirectoryPolicy`: parsed product policy for portable mode, config-file
  `base_content_path`, and platform-specific derived folder names.
- `DirectoryPlan`: Amiberry vocabulary for home, config, data, split Linux data
  roots, derived content folders, plugin roots, selected candidates, and
  diagnostics.
- `PluginLocator`: returns product-owned candidate paths for capsimage,
  FloppyBridge, and QEMU-UAE style shared-library plugins without asking
  LinuxDesktop2026 to own their ABI or loader semantics.

LinuxDesktop2026 should own root discovery, candidate reporting, explicit
directory creation, and portable-root mechanics. Amiberry should continue to own
portable-mode selection, `base_content_path`, emulator content categories,
shared-library plugin names, and loading behavior.

## Implemented Tests

- Linux defaults split derived folders between XDG data and the configured
  Amiberry home where the source documentation requires it.
- `AMIBERRY_HOME_DIR`, `AMIBERRY_CONFIG_DIR`, and `AMIBERRY_PLUGINS_DIR` win as
  explicit product targets.
- `base_content_path` moves managed content roots without moving bootstrap log
  policy.
- Portable mode maps config and plugin roots to executable-adjacent fallbacks
  without rewriting the bootstrap location of `amiberry.conf` or
  `amiberry.ini`.

## Future Expansion

- Model `AMIBERRY_DATA_DIR` selection only when it points to an existing bundled
  data directory, with missing data roots diagnosed without silent creation.
- Add Windows and macOS home override probes for their narrower documented
  environment support.
- Expand plugin candidate reporting for install-owned, user-owned, and
  executable-relative roots while keeping capsimage, FloppyBridge, QEMU-UAE, and
  suffix selection product-owned.
- Add explicit creation reporting for Amiberry folders if a future consumer
  needs write-side root creation evidence.

## Out Of Scope

- Building or porting Amiberry.
- Implementing emulator content scanning, UAE config parsing, SDL behavior,
  dynamic-library loading, or plugin ABI compatibility.
- Adding Amiberry plugin formats as first-class `ld_paths` enum kinds.
- Treating this as maintained consumer proof.

## Evidence Fit

FlavorTests plus adversarial path/root tests should catch this: the risk is
`ld_root` looking ergonomic for simple config roots while failing a real product
with multiple related app-owned and user-owned roots, environment overrides,
portable fallback, and plugin-adjacent directories.

## Release Gate

Does not block `0.2.0`.
