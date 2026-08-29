# Flavor Tests

FlavorTests is a small harness for proving that LinuxDesktop2026 APIs can
replace real upstream platform code without growing into a bulky adapter layer.

The goal is not to invent new sample applications. The goal is to take a few
production-shaped methods from upstream projects, keep their application
responsibilities intact, and refactor only the platform-root, file-hydration,
backup-write, and desktop-effect portions through `ld_*` APIs.

Selected upstream subprojects:

- `notepadpp/`: refactors several `NppParameters` seams from `Parameters.cpp`.
  It preserves Notepad++ policy around `doLocalConf.xml`, `-settingsDir`, cloud
  settings, session location, plugin config roots, shortcuts XML, find-history
  XML, and session backup recovery while replacing Win32 path discovery,
  directory setup, model-file hydration, and validated backup writes with
  `ld_settings` calls, including `ld_settings::write_common_config` for the
  common save path.
  - See the survey notes in `docs/examples/migration-examples.md` and
    `docs/survey/notepad-settings-config-audit.md`.
- `prusaslicer/`: refactors production-style config hydration and snapshot
  persistence from PrusaSlicer's config/snapshot area, plus the Linux old
  datadir migration check and recent-project config persistence. It keeps
  PrusaSlicer in charge of profile names, XML/INI content, prompt policy, and
  validation while replacing repeated copy/migration/backup/temp-write
  mechanics with `ld_settings::hydrate_config_bundle`,
  `ld_migration::plan_migration`, `ld_settings::write_with_backup`, and
  `ld_settings::write_common_config` for the repeated save path.
  - Source anchors: `src/slic3r/Config/Snapshot.cpp`,
    `src/slic3r/Config/Snapshot.hpp`, and config/resource initialization around
    `src/slic3r/GUI/GUI_App.cpp`.
  - See `docs/survey/extended-watchlist-fit-audit.md`.
- `openrgb/`: refactors the config/profile root selection from
  `ResourceManager.cpp`, profile/configuration saves from `ProfileManager.cpp`,
  and the Linux desktop autostart write path from `AutoStart/AutoStart-Linux.cpp`.
  It keeps OpenRGB's JSON/profile naming, config-directory switching, profile
  reload policy, and startup policy while replacing platform path selection,
  validated JSON writes, and desktop-file side effects with
  `ld_paths::resolve_app_paths`, `ld_settings::write_with_backup`, and
  `ld_desktop::apply_autostart`.
  - See `docs/examples/migration-examples.md` and `docs/survey/extended-watchlist-fit-audit.md`.
- `keepassxc/`: refactors KeePassXC-style config file selection from
  `Config.cpp`, including portable config, roaming vs local settings,
  import/export behavior, and old local-config migration from cache to state.
  It is meant to stress whether LinuxDesktop2026 root and persistence vocabulary
  remains understandable when an application already has local/roaming policy.
- `qbittorrent/`: refactors qBittorrent-style startup profile selection from
  `Application::Application()` and profile location behavior. It keeps
  `--profile`, named configurations, executable-adjacent `profile/` portable
  mode, relative fastresume behavior, and file logger settings as app policy.
  This is intentionally one of the larger slices because qBittorrent's profile
  model is a useful pressure test for surrounding-code fit.
- `obs/`: refactors OBS-style C path helpers and config save behavior from
  `libobs` utility APIs. It keeps buffer-return and integer status conventions
  at the product boundary so the sample can expose whether C++ report and option
  types leak too far into public C-shaped code.
- `kicad/`: refactors KiCad-style `SETTINGS_MANAGER` path decisions for user,
  project, color, toolbar, and project-backup settings. It is intentionally one
  of the larger slices because project-scoped backup roots and per-project
  disambiguation are likely to reveal integration problems that simpler config
  roots miss.
- `audacity/`: refactors Audacity-style file-config probing and save/backup
  behavior. It is focused on the repeated safe-write call shape and now routes
  the common validated backup write through `ld_settings::write_common_config`.
- `freecad/`: refactors FreeCAD-style startup configuration set construction,
  including `FREECAD_USER_HOME`, `FREECAD_USER_DATA`, `FREECAD_USER_TEMP`,
  `--user-cfg`, `--system-cfg`, module paths, deprecated-path migration, and
  user parameter saves. It stresses command-line/environment precedence without
  letting LinuxDesktop2026 own FreeCAD's application policy.

Each subproject contains its own `src/` and `test/` directories. The tests are
intentionally written against the refactored production-shaped classes and
methods rather than against generic helper functions:

- `notepadpp_flavor_tests`
- `prusaslicer_flavor_tests`
- `openrgb_flavor_tests`
- `keepassxc_flavor_tests`
- `qbittorrent_flavor_tests`
- `obs_flavor_tests`
- `kicad_flavor_tests`
- `audacity_flavor_tests`
- `freecad_flavor_tests`

See `SOURCES.md` for the upstream method/class anchors used by each slice.

Seam rule:

- The adapter code should stay smaller than the original upstream control flow
  it replaces.
- The app still owns file formats, UI, and policy.
- LinuxDesktop2026 owns root discovery, candidate reporting, hydration, backup
  writes, and desktop-effect plumbing.
- A passing flavor test is not a verdict that the refactor is seamless. These
  samples exist to feed later critique and defense review rounds against the
  original code shape.

API exposure budget:

- LinuxDesktop2026 vocabulary is acceptable at mechanism seams: calls that
  directly ask a platform library to resolve paths, seed default config files,
  write validated payloads, plan migration actions, or apply desktop effects.
- Product-shaped seams should expose application vocabulary unless the product
  deliberately adopts LinuxDesktop2026 as part of its public platform layer.
  Returning or storing `ld_*` reports, option objects, dense enum combinations,
  or LinuxDesktop2026-specific names from product methods needs an explicit
  justification in the flavor note or a narrower helper API.
- Repeated setup of option objects, unlabeled boolean choices, copied enum
  combinations, direct propagation of rich reports, and adapter code that is
  harder to read than the upstream control flow are warning signs that the
  public API is charging too much framework tax.
- The budget is not a line-count rule. A verbose call site can be acceptable
  when it describes uncommon platform behavior; a short call site can still fail
  if it moves file formats, prompt policy, profile names, or UI decisions out of
  the application.
- Future helper APIs should reduce common product-facing friction without
  hiding mechanism distinctions such as backup versus atomic replacement,
  dry-run versus execution, durable writes, dangerous migration actions, or
  capability diagnostics.

Report translation rule:

- Rich LinuxDesktop2026 reports are appropriate inside adapter code,
  diagnostic consoles, tests that exercise mechanism details, and applications
  that intentionally expose LinuxDesktop2026 as their platform layer.
- Product-shaped methods should translate those reports before returning. A
  Notepad++-style save method should return a Notepad++-shaped save result, an
  OBS-style C seam should keep integer and caller-owned-buffer conventions, and
  migration checks should return product decisions such as "prompt the user" plus
  the source and target the product needs to display.
- A new write, root, or migration convenience API is not complete merely because
  unit tests pass. It must also let the relevant FlavorTest call sites avoid
  unnecessary report propagation across product boundaries.

Layout:

- `<product>/src/` contains the refactored production-shaped code.
- `<product>/test/` contains the focused tests for that product slice.
- `CMakeLists.txt` defines a standalone build that consumes the main
  LinuxDesktop2026 targets.

Build:

```bash
cmake -S docs/FlavorTests -B build-flavor-tests -G Ninja
cmake --build build-flavor-tests
ctest --test-dir build-flavor-tests --output-on-failure
```

CI:

- The workflow in `.github/workflows/flavor-tests.yml` builds only this
  harness and runs its tests.
