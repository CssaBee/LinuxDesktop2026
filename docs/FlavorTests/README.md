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
  `ld_settings` calls.
  - See the survey notes in `docs/examples/migration-examples.md` and
    `docs/survey/notepad-settings-config-audit.md`.
- `prusaslicer/`: refactors production-style config hydration and snapshot
  persistence from PrusaSlicer's config/snapshot area, plus the Linux old
  datadir migration check and recent-project config persistence. It keeps
  PrusaSlicer in charge of profile names, XML/INI content, prompt policy, and
  validation while replacing repeated copy/migration/backup/temp-write
  mechanics with `ld_settings::hydrate_config_bundle`,
  `ld_settings::plan_migration`, and `ld_settings::write_with_backup`.
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
  `ld_settings::effects::apply_autostart`.
  - See `docs/examples/migration-examples.md` and `docs/survey/extended-watchlist-fit-audit.md`.

Each subproject contains its own `src/` and `test/` directories. The tests are
intentionally written against the refactored production-shaped classes and
methods rather than against generic helper functions:

- `notepadpp_flavor_tests`
- `prusaslicer_flavor_tests`
- `openrgb_flavor_tests`

See `SOURCES.md` for the upstream method/class anchors used by each slice.

Seam rule:

- The adapter code should stay smaller than the original upstream control flow
  it replaces.
- The app still owns file formats, UI, and policy.
- LinuxDesktop2026 owns root discovery, candidate reporting, hydration, backup
  writes, and desktop-effect plumbing.

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
