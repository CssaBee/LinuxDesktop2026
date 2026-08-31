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
`linuxdesktop::paths`, `linuxdesktop::settings::write_with_backup`, and
`linuxdesktop::desktop::apply_autostart`.

## KeePassXC

- Upstream file: `src/core/Config.cpp`
- Upstream tests: `tests/TestConfig.cpp`
- Anchors: `Config::defaultConfigFiles()`, portable config handling, roaming vs
  local settings, settings import/export, and Linux migration from old cache
  local settings to state-local settings.
- Refactored files: `keepassxc/src/keepassxc_flavor.*`
- Tests: `keepassxc/test/keepassxc_flavor_tests.cpp`

The extracted slice keeps KeePassXC's split between roaming and local settings,
portable-file naming, import filtering, and app-owned key/value policy while
using LinuxDesktop2026 for root selection, old-file migration planning, and
validated backup writes.

## qBittorrent

- Upstream file: `src/app/application.cpp`
- Upstream support area: `src/base/profile_p.cpp`
- Anchors: `Application::Application()` profile initialization, `--profile`,
  executable-adjacent `profile/` portable mode, configuration-name suffixes,
  relative fastresume behavior, and file logger settings persistence.
- Refactored files: `qbittorrent/src/qbittorrent_flavor.*`
- Tests: `qbittorrent/test/qbittorrent_flavor_tests.cpp`

The extracted slice keeps qBittorrent's profile and fastresume rules visible in
product code while using LinuxDesktop2026 for root selection, named log roots,
and durable settings-file updates.

## OBS Studio

- Upstream files: `libobs/util/platform-windows.c`, `libobs/util/platform-nix.c`,
  and `libobs/util/config-file.c`
- Docs anchor: `docs/sphinx/reference-libobs-util-config-file.rst`
- Cross-port role: first FlavorTest review pilot comparing Windows and
  Unix-like OBS utility concepts through source anchors and paraphrased notes,
  without copying upstream snippets into this repository.
- Anchors: `os_get_config_path()`, module config paths, and safe config-file
  save behavior.
- Refactored files: `obs/src/obs_flavor.*`
- Tests: `obs/test/obs_flavor_tests.cpp`

The extracted slice keeps OBS-style C APIs, caller-owned buffers, owned string
paths, and integer return values at the product boundary while using
LinuxDesktop2026 behind those boundaries for path resolution and backup writes.
The cross-port lesson is that LinuxDesktop2026 should match OBS's shared product
concepts, such as config path lookup and safe config persistence, while keeping
platform-specific path construction and backup mechanics private.

## KiCad

- Upstream files: `common/settings/settings_manager.cpp` and
  `include/settings/settings_manager.h`
- Docs anchor: KiCad Doxygen for `SETTINGS_MANAGER`
- Anchors: `GetPathForSettingsFile()`, `GetToolbarSettingsPath()`,
  `GetBackupRootForProject()`, project-key disambiguation, and settings save.
- Refactored files: `kicad/src/kicad_flavor.*`
- Tests: `kicad/test/kicad_flavor_tests.cpp`

The extracted slice keeps KiCad's distinction between user, project, color, and
toolbar settings plus project-vs-user backup policy. LinuxDesktop2026 supplies
named roots and durable writes without owning the project backup decision.

## Audacity

- Upstream files: `AudacityFileConfig.cpp` and `FileConfig.cpp`
- Anchors: local config readability/writability probing, backup file behavior,
  save/restore loop, and dirty-state clearing after a successful flush.
- Refactored files: `audacity/src/audacity_flavor.*`
- Tests: `audacity/test/audacity_flavor_tests.cpp`

The extracted slice keeps Audacity's local file-config object and dirty-state
ownership while using LinuxDesktop2026 for the atomic backup write. It exists
mainly to expose whether repeated write-option setup becomes too visible.

## FreeCAD

- Upstream docs: FreeCAD startup and configuration documentation
- Anchors: startup configuration set construction, `FREECAD_USER_HOME`,
  `FREECAD_USER_DATA`, `FREECAD_USER_TEMP`, `--user-cfg`, `--system-cfg`,
  module path handling, deprecated path behavior, and user parameter saves.
- Refactored files: `freecad/src/freecad_flavor.*`
- Tests: `freecad/test/freecad_flavor_tests.cpp`

The extracted slice keeps FreeCAD's command-line and environment precedence in
application code while using LinuxDesktop2026 for path candidate reporting,
deprecated-path migration planning, and durable parameter-file writes.

## Walnut

- Upstream repository: `StudioCherno/Walnut`
- Source snapshot: `master` at commit `3b8e414fdecf`
- Upstream files: `Walnut/src/Walnut/Application.h`,
  `Walnut/src/Walnut/Application.cpp`, `Walnut/src/Walnut/EntryPoint.h`,
  `Walnut/src/Walnut/Image.cpp`, `WalnutExternal.lua`,
  `Walnut/premake5.lua`, and `WalnutApp/premake5.lua`
- Anchors: `Walnut::ApplicationSpecification`, `Walnut::Application`,
  `Walnut::Application::Run`, `Walnut::Application::Close`,
  `Walnut::CreateApplication(int argc, char** argv)`, `Application::Init`,
  `Application::Shutdown`, `check_vk_result`, `glfw_error_callback`,
  `SetupVulkan`, `SetupVulkanWindow`, command-buffer helpers,
  `SubmitResourceFree`, Windows-only `WL_PLATFORM_WINDOWS` entry-point guard,
  `Walnut::Main`, `g_ApplicationRunning`, `main` versus `WinMain` behavior
  under `WL_DIST`, `Walnut::Image::Image(std::string_view path)`,
  `Image::SetData`, `Image::Resize`, `Image::Release`, `VULKAN_SDK`
  environment use, Windows-only `WL_PLATFORM_WINDOWS`, and `ConsoleApp` versus
  `WindowedApp` distribution mode.
- Downstream/supporting anchors: `StudioCherno/WalnutAppTemplate`,
  `TheCherno/RayTracing`, `TheCherno/Walnut-Chat`, and existing
  CMake/Linux-experiment forks as review context only.
- Refactored files: `walnut/src/walnut_flavor.*`
- Tests: `walnut/test/walnut_flavor_tests.cpp`

The extracted slice does not build Walnut, Vulkan, GLFW, Dear ImGui, or Premake
projects. It keeps the surrounding Walnut control-flow shape visible:
application specification, executable-adjacent resources, renderer capability
checks, GPU selection policy, image path lookup, and entry-point lifecycle state
remain Walnut responsibilities. LinuxDesktop2026 is used only to resolve
platform paths, and those diagnostics are translated before leaving the
Walnut-facing bootstrap plan.

## OpenIPC Dashboard

- Upstream repository: `OpenIPC/dashboard`
- Source snapshot: `main` at commit `d402f0ee3f83`
- Reference role: Qt-native cross-platform application used to decide when
  LinuxDesktop2026 should document, recommend, or adapt to toolkit-owned seams
  instead of replacing them.
- Upstream files: `src/main.cpp`, `src/backend/AppPaths.h`,
  `src/backend/SystemController.cpp`,
  `src/backend/SystemControllerSettings.cpp`,
  `src/backend/SystemControllerState.cpp`, `src/backend/UserManager.cpp`,
  `src/backend/analytics/AnalyticsEngine.cpp`,
  `src/backend/web/DashboardWebDeploymentPolicy.h`,
  `src/backend/web/DashboardWebDeploymentPolicy.cpp`,
  `src/backend/web/DashboardWebServer.h`,
  `src/backend/web/DashboardWebServer.cpp`,
  `src/backend/web/DashboardWebApi.cpp`, and `src/backend/PathUtils.cpp`
- Anchors: command-line handling for `--server-only`, `--initialize-admin`,
  and `--data-root`; `OPENIPC_DATA_ROOT`; `QT_QPA_PLATFORM=offscreen`;
  QGuiApplication naming and QSettings root placement; log setup under
  `AppPaths::dataDirectory()`; TLS self-test and QML smoke branches;
  server-only web-server startup; guarded administrator bootstrap through
  `OPENIPC_INITIAL_ADMIN_PASSWORD_FILE`; runtime/config/data/evidence roots;
  user/state/analytics/module paths; deployment-profile normalization; web
  policy URLs and readiness status; health/log/diagnostic browser output; and
  local path normalization for file URLs, tilde input, Windows drives, and
  Linux absolute-path recovery.
- Supporting anchors: `tests/server_only_smoke.py`,
  `packaging/systemd/openipc-dashboard.service`,
  `packaging/windows/install-headless-task.ps1`, `docs/WEB_SERVER.md`,
  `docs/WEB_DEPLOYMENT.md`,
  `tests/DashboardWebDeploymentPolicyTests.cpp`, `tests/PathUtilsTests.cpp`,
  `tests/StateStoreTests.cpp`, and `tests/UserManagerTests.cpp`
- Refactored files: `openipc_dashboard/src/openipc_dashboard_flavor.*`
- Tests: `openipc_dashboard/test/openipc_dashboard_flavor_tests.cpp`

The extracted slice does not build Qt, QML, GStreamer, SQL, web sockets, camera
hardware, or OpenIPC's security model. It keeps Dashboard's profile names,
service data-root contract, administrator bootstrap rules, deployment policy,
readiness vocabulary, local import path semantics, and browser redaction policy
in product-shaped code. LinuxDesktop2026 is used only for ordinary desktop
root discovery; service-profile layout and web policy remain Dashboard-owned.
As a reference case, Dashboard is also negative evidence against a generic
toolkit replacement layer: Qt owns application lifecycle, QSettings mechanics,
QML startup, and event-loop behavior well enough that LinuxDesktop2026 should
stay at the narrower desktop-root, diagnostics, packaging, and migration seams.

## Gearcoleco

- Upstream repository: `drhelius/Gearcoleco`
- Source snapshot: `main`, reviewed from the repository README and source tree
  on 2026-08-31.
- Upstream areas: desktop frontend startup under `platforms/`, shared emulator
  settings/resource handling under `src/`, and README-documented portable mode.
- Anchors: `--portable`, executable-adjacent `portable.ini`,
  `gamecontrollerdb.txt` beside the application binary, ROM command-line
  arguments, optional symbol-file argument, and automatic `.sym`/`.noi` symbol
  lookup beside the ROM.
- Refactored files: `gearcoleco/src/gearcoleco_flavor.*`
- Tests: `gearcoleco/test/gearcoleco_flavor_tests.cpp`

The extracted slice does not build SDL, OpenGL, emulation, debugger, ROM
loading, or MCP server code. It keeps Gearcoleco's portable-mode trigger,
controller database name, ROM argument ownership, and symbol lookup order in
product-shaped code. LinuxDesktop2026 supplies deterministic installed versus
portable settings roots and first-run `gearcoleco.ini` hydration.

## CtrlrX

- Upstream repository: `damiensellier/CtrlrX`
- Source snapshot: `main`, reviewed from the repository README/changelog and
  source tree on 2026-08-31.
- Upstream areas: `CtrlrSettings.cpp`, `CtrlrManager.cpp`,
  `CtrlrManagerInstance.cpp`, Projucer project files, exported plugin instance
  handling, preferences/autosave changes, and resource reload behavior.
- Anchors: standalone-only autosave preference updates, `Ctrlr.settings`,
  default look-and-feel preference storage, resource reload roots, VST3/AU/AAX
  export destinations, intermediate `.jucer` project use, and panel identifier
  replacement during plugin export.
- Refactored files: `ctrlrx/src/ctrlrx_flavor.*`
- Tests: `ctrlrx/test/ctrlrx_flavor_tests.cpp`

The extracted slice does not build JUCE, Lua, MIDI, audio plugins, or exported
plugin binaries. It keeps standalone-versus-plugin decisions, preference names,
resource reload order, plugin format naming, and panel identifier policy in
CtrlrX-shaped code. LinuxDesktop2026 supplies root resolution, common config
writes, and plugin path-set discovery behind those product decisions.

## SmartServoFramework

- Upstream repository: `emericg/SmartServoFramework`
- Source snapshot: `master`, reviewed from the repository README and
  documentation layout on 2026-08-31.
- Upstream areas: `SmartServoGui/`, framework examples, and serial
  communication documentation.
- Anchors: Qt control GUI profile roots, persistent device settings, last
  session state, serial port scanning, Linux group/permission notes, macOS and
  Windows USB driver notes, and device-specific profile files.
- Refactored files: `smartservo/src/smartservo_flavor.*`
- Tests: `smartservo/test/smartservo_flavor_tests.cpp`

The extracted slice does not build Qt, the C++ actuator framework, or serial
backends. It keeps link availability, baud rate, actuator scan policy, and safe
device-profile naming in SmartServoGui-shaped code. LinuxDesktop2026 supplies
ordinary desktop roots and validated writes for GUI/device settings only.

## KickCAT

- Upstream repository: `leducp/KickCAT`
- Source snapshot: `master`, reviewed from the repository README, `docs/`, and
  `tools/` layout on 2026-08-31.
- Upstream areas: `tools/kickui/`, `tools/eeprom_editor/`, CLI tools,
  simulator tooling, and documentation for build/runtime options.
- Anchors: KickUI GUI settings, EEPROM editor workspace, ESI XML lookup, network
  simulator runtime socket, master launch interface selection, real-time launch
  flag, and diagnostics for missing interface or ESI files.
- Refactored files: `kickcat/src/kickcat_flavor.*`
- Tests: `kickcat/test/kickcat_flavor_tests.cpp`

The extracted slice does not build the EtherCAT master/slave stack, embedded
targets, socket backends, Python tooling, or ImGui/GLFW frontends. It keeps
network interface selection, real-time behavior, bus launch rules, and ESI
requirements in KickCAT-shaped code. LinuxDesktop2026 is limited to optional
desktop tooling paths and GUI config writes, making this a boundary challenge
for avoiding accidental coupling to real-time or embedded modules.
