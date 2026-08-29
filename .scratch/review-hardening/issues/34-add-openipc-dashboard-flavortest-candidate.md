# 34 - Add OpenIPC Dashboard FlavorTest

**What to build:** Add OpenIPC/dashboard as a real FlavorTest product under
`docs/FlavorTests/openipc_dashboard/`, using source-anchored Dashboard-shaped
classes and tests instead of leaving it as a candidate note.

**Blocked by:** None.

**Status:** proposed

- [ ] `docs/FlavorTests/openipc_dashboard/src/openipc_dashboard_flavor.hpp`
  and `docs/FlavorTests/openipc_dashboard/src/openipc_dashboard_flavor.cpp`
  exist.
- [ ] `docs/FlavorTests/openipc_dashboard/test/openipc_dashboard_flavor_tests.cpp`
  exists and is wired through `docs/FlavorTests/CMakeLists.txt` with
  `add_flavor_product(openipc_dashboard)`.
- [ ] `docs/FlavorTests/README.md` lists OpenIPC Dashboard as a covered
  FlavorTest.
- [ ] `docs/FlavorTests/SOURCES.md` records the OpenIPC Dashboard upstream
  source anchors and the packaging/smoke/non-typical usage anchors considered
  during selection.
- [ ] `docs/FlavorTests/API_FRICTION.md` records where Qt already solves the
  seam and where LinuxDesktop2026 still adds value through neutral root,
  service-profile, and diagnostics vocabulary.
- [ ] The slice tests multiple hardware-free production-shaped seams. It may
  add more than one Dashboard-facing class/function where that keeps the adapter
  readable.

## Decision

Do not evaluate OpenIPC Dashboard as a candidate. Add it to FlavorTests, like
FreeCAD, qBittorrent, and Walnut, but keep the first pass hardware-free and
focused on desktop versus server-only profile isolation, path/root placement,
deployment-policy diagnostics, and readiness/lifecycle reporting.

OpenIPC Dashboard is useful because it is a maintained cross-platform Qt/QML
application that already has Linux and Windows packaging, an embedded web
companion, autonomous `--server-only` mode, and an explicit service data-root
override. Qt handles a lot of platform placement through `QStandardPaths` and
`QSettings`, so this FlavorTest should prove whether LinuxDesktop2026 can add
portable service-profile and diagnostic vocabulary without fighting Qt or
owning Dashboard's camera, web, or security policy.

## Source Anchors

Use OpenIPC/dashboard `main` at commit `d402f0ee3f83` as the primary source
snapshot. Source links should be recorded in `SOURCES.md`; do not paste
upstream code into FlavorTests.

- `src/main.cpp`
  - argument scan for `--server-only`, `--initialize-admin`, and `--data-root`
  - `OPENIPC_DATA_ROOT` and `QT_QPA_PLATFORM=offscreen` environment setup
  - `QGuiApplication` organization/application naming
  - `QSettings::setDefaultFormat` and `QSettings::setPath`
  - custom log file setup under `AppPaths::dataDirectory()`
  - TLS self-test and QML smoke branches
  - server-only branch calling `webServer()->setServerOnlyMode(true)` and
    `webServer()->start()`
  - guarded initial administrator bootstrap from
    `OPENIPC_INITIAL_ADMIN_PASSWORD_FILE`
- `src/backend/AppPaths.h`
  - `AppPaths::runtimeRoot`
  - `AppPaths::dataDirectory`
  - `AppPaths::configDirectory`
  - `AppPaths::evidenceDirectory`
- `src/backend/SystemController.cpp`
  - constructor-owned backend graph that wires user, state, analytics, logs,
    archive, incident, fleet, and web server services together
  - app-setting import normalization for local path settings
- `src/backend/SystemControllerSettings.cpp`
  - `SystemController::saveAppSettings`
  - `SystemController::openFolder`
  - `SystemController::normalizeLocalPath`
  - deployment profile normalization when saving settings
- `src/backend/SystemControllerState.cpp`
  - `stateDatabasePath`
  - state load/save through `StateStore`
  - recording/screenshot default roots under `AppPaths::dataDirectory()`
- `src/backend/UserManager.cpp`
  - `users.json` lookup under config/data roots
  - QSettings-backed user/config behavior
- `src/backend/analytics/AnalyticsEngine.cpp`
  - modules, evidence snapshots/clips, and analytics event store under the
    selected app data root
- `src/backend/web/DashboardWebDeploymentPolicy.h/.cpp`
  - `DashboardWebDeploymentPolicy::fromSettings`
  - `originAllowed`
  - `localHttpUrl`, `publicHttpUrl`, and `publicWebSocketUrl`
  - `publicStatus`
- `src/backend/web/DashboardWebServer.h/.cpp`
  - `DashboardWebServer::applySettings`
  - `start`, `stop`, `restart`
  - `status`
  - `healthStatus`
  - `setServerOnlyMode`
  - bind-conflict and readiness diagnostics
- `src/backend/web/DashboardWebApi.cpp`
  - `/api/v1/health/live`
  - `/api/v1/health/ready`
  - `/api/v1/server`
  - `logsData`, `diagnosticsData`, and sensitive-data scrubbing
- `src/backend/PathUtils.cpp`
  - `PathUtils::localPathFromUserInput`
  - file URL, tilde, Windows-drive, and Linux absolute-without-slash handling

## Non-Typical Usage Search Notes

Also record these as supporting anchors:

- `tests/server_only_smoke.py` starts the packaged executable with
  `--server-only`, isolated `OPENIPC_DATA_ROOT`, fixed web ports, XDG roots,
  and offscreen Qt, then checks readiness, port conflict rejection, shutdown,
  recovery on the same profile, and one-shot administrator bootstrap.
- `packaging/systemd/openipc-dashboard.service` runs the AppImage-style
  `AppRun --server-only` with `QT_QPA_PLATFORM=offscreen`,
  `OPENIPC_DATA_ROOT=/var/lib/openipc-dashboard`, hardened service settings,
  and an explicit writable data path.
- `packaging/windows/install-headless-task.ps1` models the non-service Windows
  headless case with a scheduled task, `--server-only`, and `--data-root`.
- `docs/WEB_SERVER.md` and `docs/WEB_DEPLOYMENT.md` document the product
  contract: desktop and autonomous web modes share backend state, while a
  dedicated service data root keeps users, state, settings, analytics data,
  modules, and logs isolated from the interactive desktop profile.
- `tests/DashboardWebDeploymentPolicyTests.cpp`, `tests/PathUtilsTests.cpp`,
  `tests/StateStoreTests.cpp`, and `tests/UserManagerTests.cpp` show existing
  unit coverage that the FlavorTest should paraphrase into LinuxDesktop2026
  source anchors rather than copying Qt test code.
- Browser imports, archive downloads, logs, diagnostics, and web configuration
  are non-camera paths worth considering because they exercise canonical roots,
  redaction, and "do not expose local filesystem paths" policy.

## Implementation Shape

Create a small Dashboard-facing adapter that models source control flow without
pulling in Qt, QML, GStreamer, SQL, web sockets, or camera hardware:

- `CommandLineOptions`
  - fields: `server_only`, `initialize_admin_username`, `data_root_override`,
    `smoke_qml`, `self_test_tls`, and original argument list if needed.
- `RuntimeEnvironment`
  - fields: home directory, process environment map, executable directory,
    platform name, TLS available, web sockets available, WebRTC available,
    existing users, and simulated occupied ports.
- `ApplicationProfile`
  - method: `resolve(const CommandLineOptions&, const RuntimeEnvironment&)`.
  - returns Dashboard vocabulary for runtime root, config root, data root,
    log file, users file, state database, modules root, analytics event store,
    evidence roots, QSettings root, and whether Qt offscreen mode is required.
- `ServerModeBootstrap`
  - method: `prepare(const ApplicationProfile&, const CommandLineOptions&, const RuntimeEnvironment&)`.
  - returns whether desktop QML or server-only mode should start, plus
    Dashboard-shaped startup diagnostics.
- `DeploymentPolicy`
  - method: `fromSettings(const DashboardSettings&)`.
  - preserves Dashboard's `localhost`, `lan`, `vpn`, and `reverse_proxy`
    concepts while letting tests verify bind-address, secure-cookie, proxy,
    public URL, and validation behavior.
- `ReadinessStatus`
  - fields: `ready`, `running`, version, profile, startup time, TLS/WebRTC/Web
    socket availability, bootstrap-required flag, and last error.
- `PathNormalizer`
  - method: `localPathFromUserInput(std::string_view)`.
  - models file URL, native separator, tilde, Linux `/mnt` recovery, and
    Windows drive behavior without depending on Qt.
- `DiagnosticBundle`
  - method or function that translates internal status, log count, storage
    roots, and health into browser-safe Dashboard output while preserving the
    "no arbitrary local filesystem paths or secrets" policy.

The adapter should call LinuxDesktop2026 only at platform seams:

- use `ld_paths::resolve_app_paths` or the current root resolver for desktop
  defaults, explicit `OPENIPC_DATA_ROOT`, `--data-root`, XDG/AppData roots,
  config/data/state/cache/log candidates, and service-profile reporting;
- keep Dashboard-owned policy in Dashboard terms: deployment profile names,
  admin bootstrap rules, HTTP/WebSocket ports, QSettings intent, browser import
  restrictions, and redaction rules;
- translate any `ld_*` report into Dashboard-shaped startup/readiness/root
  diagnostics before returning from product-shaped classes;
- avoid using LinuxDesktop2026 to own web routing, RBAC, session security,
  camera discovery, GStreamer, firmware operations, or QML presentation.

## Required Tests

Add focused tests that prove the flavor pressures the intended APIs:

- desktop default profile resolves config/data/log/state/evidence roots through
  normal user locations and does not require `QT_QPA_PLATFORM=offscreen`;
- `OPENIPC_DATA_ROOT` resolves an isolated service profile with `config/`,
  `data/`, `data/app.log`, `data/state.sqlite3`, `data/modules`,
  `data/analytics_events.sqlite`, and `evidence/snapshots`/`evidence/clips`;
- `--data-root <absolute-path>` wins over the environment form and records the
  override source for service launchers;
- relative or empty data-root overrides are rejected or diagnosed without
  falling silently into the desktop profile;
- `--server-only` sets offscreen mode when no platform override exists and
  starts the web server even when desktop auto-start is disabled;
- `--initialize-admin` is accepted only with `--server-only`, requires a
  password-file path, rejects existing users, and does not model password values
  in command-line diagnostics;
- deployment policy preserves `localhost`, migrates legacy allow-remote settings
  to `lan`, validates `reverse_proxy`, forces secure cookies for a valid reverse
  proxy, and exposes trusted-proxy count without exposing peer addresses;
- readiness status reports `ready=false` with a product error for invalid bind
  address, invalid deployment profile, or occupied port, and reports
  `ready=true` with bounded version/profile/TLS/WebRTC/WebSocket/startup fields
  when startup succeeds;
- `PathNormalizer` preserves relative paths, decodes local file URLs, expands
  `~`, preserves Windows drive paths, and recovers likely Linux absolute paths
  such as `mnt/video`;
- diagnostics and browser-facing status do not expose camera passwords,
  credential-bearing URLs, user secrets, or arbitrary local filesystem paths.

Optional tests, if the adapter remains compact:

- a QML smoke option resolves the smoke harness branch without starting the full
  desktop path;
- TLS self-test returns the Dashboard-shaped exit/status result without
  constructing the web server;
- log rotation policy records max size and file count as Dashboard policy while
  leaving actual file rotation mechanics out of the FlavorTest.

## Out Of Scope

- Building OpenIPC Dashboard, Qt, QML, GStreamer, WebRTC, SQL drivers, AppImage,
  Windows installer, or browser smoke tests.
- Talking to camera hardware, ONVIF, RTSP, Majestic, OpenIPC firmware endpoints,
  Camex, SSH, or analytics models.
- Replacing Qt's `QStandardPaths`, `QSettings`, event loop, QML engine, or
  `QDesktopServices` in the upstream application.
- Implementing the web server, HTTP parser, RBAC/session store, CSRF/origin
  checks, WebSocket, archive streaming, or diagnostic bundle download.
- Treating systemd, Windows Scheduled Task, or AppImage packaging as the API.
  They are supporting evidence for service-profile behavior, not the main seam.

## Acceptance

`ctest --test-dir build-flavor-tests --output-on-failure` includes
`openipc_dashboard_flavor_tests`, and the new OpenIPC Dashboard files show that
LinuxDesktop2026 can support a Qt application with separate desktop and
server-only profiles without leaking raw `ld_*` vocabulary across Dashboard's
product boundaries.
