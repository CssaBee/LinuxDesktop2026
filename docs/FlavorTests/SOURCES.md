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
`ld_paths`, `ld_settings::write_with_backup`, and `ld_desktop::apply_autostart`.

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
- Anchors: `os_get_config_path()`, module config paths, and safe config-file
  save behavior.
- Refactored files: `obs/src/obs_flavor.*`
- Tests: `obs/test/obs_flavor_tests.cpp`

The extracted slice keeps OBS-style C APIs, caller-owned buffers, owned string
paths, and integer return values at the product boundary while using
LinuxDesktop2026 behind those boundaries for path resolution and backup writes.

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
