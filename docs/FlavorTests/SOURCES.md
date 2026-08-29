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
  hydration, and session backup behavior.
- Refactored files: `notepadpp/src/notepadpp_flavor.*`
- Tests: `notepadpp/test/notepadpp_flavor_tests.cpp`

The extracted slice keeps `NppParameters` member assignment, the
`doLocalConf.xml` portable marker, `-settingsDir` priority, cloud-config
priority, per-user XML files, plugin config roots, and session save validation.

## PrusaSlicer

- Upstream files: `src/slic3r/Config/Snapshot.cpp`,
  `src/slic3r/Config/Snapshot.hpp`, and config initialization near
  `src/slic3r/GUI/GUI_App.cpp`.
- Refactored files: `prusaslicer/src/prusaslicer_flavor.*`
- Tests: `prusaslicer/test/prusaslicer_flavor_tests.cpp`

The extracted slice keeps PrusaSlicer's app-owned preset bundle loading and
snapshot XML validation, while using LinuxDesktop2026 for first-run model
hydration and durable temp-write plus backup behavior.

## OpenRGB

- Upstream files: `ResourceManager.cpp` and `AutoStart/AutoStart-Linux.cpp`
- Anchors: `ResourceManager::ResourceManager()`,
  `SetupConfigurationDirectory()`, `GetConfigurationDirectory()`, settings load,
  log-manager setup, profile-manager setup, and Linux autostart.
- Refactored files: `openrgb/src/openrgb_flavor.*`
- Tests: `openrgb/test/openrgb_flavor_tests.cpp`

The extracted slice keeps the ResourceManager startup flow and downstream
consumers of the config directory, while replacing environment probing and XDG
autostart file planning with `ld_paths` and `ld_settings::effects`.
