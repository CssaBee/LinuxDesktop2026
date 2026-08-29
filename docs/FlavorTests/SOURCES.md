# Flavor Test Source Anchors

These tests are source-anchored refactors, not clean-room sample apps. The code
under each product directory keeps the surrounding class and method shape close
enough that a reviewer can judge whether LinuxDesktop2026 calls would blend into
that product.

The files here intentionally avoid vendoring large upstream files. Each slice is
a small, buildable extraction of the surrounding production responsibilities:
application policy and file formats stay in the product code; repeated platform
path, directory, hydration, backup-write, and desktop-effect mechanics move to
`ld_*`.

## Notepad++

- Upstream file: `PowerEditor/src/Parameters.cpp`
- Anchors: `NppParameters::load()`, `getSettingsFolder()`, XML config-family
  hydration, shortcuts XML loading/saving, find-history serialization, and
  session backup behavior.
- Refactored files: `notepadpp/src/notepadpp_flavor.*`
- Tests: `notepadpp/test/notepadpp_flavor_tests.cpp`

The extracted slice keeps `NppParameters` member assignment, the
`doLocalConf.xml` portable marker, `-settingsDir` priority, cloud-config
priority, per-user XML files, plugin config roots, shortcut HMAC bookkeeping
inputs, find-history XML ownership, session save validation, and session backup
recovery. The LinuxDesktop2026 seams are intentionally small: resolve the app
roots, hydrate missing model XML, and perform durable validated writes.

## PrusaSlicer

- Upstream files: `src/slic3r/Config/Snapshot.cpp`,
  `src/slic3r/Config/Snapshot.hpp`, and config initialization near
  `src/slic3r/GUI/GUI_App.cpp`.
- Anchors: preset bundle/vendor config hydration, snapshot persistence,
  `check_old_linux_datadir()` migration prompting, app config persistence, and
  recent-project config serialization.
- Refactored files: `prusaslicer/src/prusaslicer_flavor.*`
- Tests: `prusaslicer/test/prusaslicer_flavor_tests.cpp`

The extracted slice keeps PrusaSlicer's app-owned preset bundle loading and
snapshot XML validation, legacy-datadir prompt policy, recent-project data
shape, and config section validation, while using LinuxDesktop2026 for
first-run model hydration, dry-run migration planning, and durable temp-write
plus backup behavior.

## OpenRGB

- Upstream files: `ResourceManager.cpp` and `AutoStart/AutoStart-Linux.cpp`
- Anchors: `ResourceManager::ResourceManager()`,
  `SetupConfigurationDirectory()`, `GetConfigurationDirectory()`, settings load,
  `SetConfigurationDirectory()`, settings save, profile-manager setup,
  controller configuration save, and Linux autostart.
- Refactored files: `openrgb/src/openrgb_flavor.*`
- Tests: `openrgb/test/openrgb_flavor_tests.cpp`

The extracted slice keeps the ResourceManager startup flow and downstream
consumers of the config directory, OpenRGB's `OpenRGB.json`,
`Configuration.json`, and `profiles/` conventions, while replacing environment
probing, validated JSON writes, and XDG autostart file planning with
`ld_paths`, `ld_settings::write_with_backup`, and `ld_settings::effects`.
