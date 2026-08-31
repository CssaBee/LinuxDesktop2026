# FlavorTest API Friction Notes

These notes record what still feels unnatural at each product seam after the
first ergonomics pass. A passing FlavorTest means the selected behavior can be
refactored through LinuxDesktop2026 today; it does not mean the API is ready for
real upstream adoption.

Use these notes before treating a FlavorTest as integration-readiness evidence:

- Acceptable platform-mechanism vocabulary is vocabulary a product would
  reasonably tolerate at an adapter boundary: root discovery, named roots,
  config-default seeding, backup writes, durable writes, migration planning, and
  desktop effects.
- Product-boundary leakage is vocabulary that escapes into a product-shaped
  public method, result type, stored state, or caller obligation where the
  application would normally speak in its own terms.
- Convenience helpers are only successful when they improve local reasoning.
  Shorter call sites that still require readers to reconstruct LinuxDesktop2026
  policy are evidence for a future API change.

## Notepad++

Current proof-branch state:

- `LinuxDesktop2026-crossport-notepadpp` now consumes an installed
  LinuxDesktop2026 package and calls
  `linuxdesktop2026_generate_path_defaults()` from CMake. The generated header
  selects the OS-specific `platform_path_defaults` factory at configure time.
- `NotepadPlusPlusSettingsBackend::resolve()` passes those generated defaults
  through `linuxdesktop::root::options::platform_defaults`, while Notepad++
  still owns install-root, home/runtime inputs, command-line settings, cloud
  choice, local-config marker, and privileged-install policy.
- The in-tree FlavorTest remains useful for behavior coverage, but the
  cross-port proof branch is the stronger current consumer signal.

Acceptable mechanism vocabulary:

- `linuxdesktop::root::resolve_app_roots()`, generated platform defaults, named roots,
  `ensure_config_defaults()`, `write_common_config()`, and `ldm::plan_copy()`
  are adapter-level platform mechanics.
- The builder/factory split now hides OS selection without hiding Notepad++
  decisions: CMake chooses XDG vs. Windows defaults, and the product supplies
  the roots it has accepted.

Product-boundary leakage:

- The proof layout still carries raw `linuxdesktop::diagnostic` values. That is
  acceptable for the proof harness, but a real Notepad++ patch should translate
  them to startup warnings or log messages before storing long-lived product
  state.
- Portable/local config needs both "requested" and "active" state so a rejected
  `doLocalConf.xml` under a protected install can be reported. The current API
  already exposes this, but product code still has to preserve the distinction.

Helper assessment:

- The former path-default gap is closed for the maintained Notepad++ proof:
  consumers no longer need a private `platform_paths.hpp`-style helper or host
  environment injection to get deterministic platform roots.
- Remaining friction is translation work, not missing mechanism: diagnostics
  and layout structs should become Notepad++ vocabulary before any upstreamable
  patch, while the LinuxDesktop2026 calls can stay at the adapter boundary.

## Audacity

Remaining visible concepts:

- `FileConfig::Flush()` only needs common validated backup write behavior.
- `FileConfig::Init()` still owns the probing and warning loop; this is product
  behavior, not a LinuxDesktop2026 responsibility.

Acceptable mechanism vocabulary:

- `write_common_config()` is acceptable inside `Flush()` because the method is
  already a persistence boundary.

Product-boundary leakage:

- None in the public seam after report translation. `FlushResult` is
  Audacity-shaped and only reports the outcome and backup file.

Helper assessment:

- The write facade improved both length and reasoning here. This is the clearest
  positive example for the current helper design.

## qBittorrent

Remaining visible concepts:

- `Profile::init()` now uses `linuxdesktop::root::request_builder` for app identity,
  executable resource root, injected environment, portable marker, and log root
  placement.
- The profile-dir override branch remains outside the builder chain because it
  is qBittorrent policy: command-line profile roots win, otherwise an
  executable-adjacent `profile` directory activates portable mode.
- `saveFileLoggerSettings()` now uses `write_common_config()` for the ordinary
  durable settings save.

Acceptable mechanism vocabulary:

- `linuxdesktop::root::resolve_app_roots()` and the log named-root request are adapter-level
  platform mechanics.
- Keeping qBittorrent's `SpecialFolder` enum at the product boundary is good;
  callers do not need to learn LinuxDesktop2026 root terminology.

Product-boundary leakage:

- None in the public result types. The LinuxDesktop2026 concepts remain inside
  the adapter implementation.

Helper assessment:

- The root-request builder removes request boilerplate without swallowing the
  portable profile decision. That split is the desired shape: mechanics in the
  helper, product branching in the adapter.
- The write facade is a good fit for file logger settings because qBittorrent
  only contributes the product path, INI content, and validation callback.

## KeePassXC

Remaining visible concepts:

- `Config::open()` still combines portable config, roaming/local split, XDG
  overrides, and a machine-local named root.
- `exportSettings()` now uses `write_common_config()` for a common durable
  validated export.
- `migrateOldLocalConfig()` still combines old cache detection with migration
  planning, but it now returns a KeePassXC-shaped result.

Acceptable mechanism vocabulary:

- Root discovery and the local-settings named root are acceptable adapter
  mechanics because KeePassXC has explicit roaming/local policy.
- Using `plan_move_file()` internally is acceptable because the old cache config
  migration is a platform file move.

Product-boundary leakage:

- Fixed in the follow-up pass. `migrateOldLocalConfig()` now returns
  `LocalConfigMigration`, carrying availability, blocked state, prompt intent,
  dry-run state, source/target paths, and a KeePassXC action name. The raw
  `migration_plan` stays inside the adapter implementation.

Helper assessment:

- `plan_move_file()` removes mechanical action construction, but it only
  improves the product seam after the adapter translates the raw plan.
- `write_common_config()` removes low-level backup/replace setup from settings
  export without hiding KeePassXC's roaming/local filtering.
- Root helpers help the local-settings declaration; they do not resolve the
  deeper roaming/local naming mismatch between KeePassXC and LinuxDesktop2026.

## KiCad

Remaining visible concepts:

- `SETTINGS_MANAGER` uses `linuxdesktop::root::request_builder` for injected environment setup
  and named roots for colors, toolbars, and project-backups.
- `Save()` now uses `write_common_config()` for common durable JSON settings
  writes.

Acceptable mechanism vocabulary:

- Named roots are acceptable inside the constructor because KiCad has real
  user, project, color, toolbar, and backup placement distinctions.
- `GetPathForSettingsFile()` and `GetBackupRootForProject()` stay product-shaped
  and preserve KiCad's project policy.

Product-boundary leakage:

- None in public result types. LinuxDesktop2026 reports are translated before
  leaving the adapter implementation.

Helper assessment:

- The root-request builder makes KiCad's ordinary environment and named-root
  setup easier to scan, but it does not solve the product-shaped project backup
  path. That keyed-by-project fallback should remain KiCad code unless more
  flavors repeat it.
- The write facade is a good fit for `Save()` because KiCad still owns the
  settings target and JSON validation choice.

## FreeCAD

Remaining visible concepts:

- `ApplicationConfig::build()` still calls `ld_paths::resolve_app_paths()` only
  to validate or exercise the path layer after FreeCAD-specific environment
  variables have already selected paths.
- `ConfigurationSet` stores a FreeCAD-shaped deprecated-path migration decision
  instead of a raw LinuxDesktop2026 plan.
- `saveUserParameter()` now uses `write_common_config()` for a common durable
  XML-shaped user-parameter save.

Acceptable mechanism vocabulary:

- Consulting the path resolver is acceptable inside the adapter, but FreeCAD
  must continue to own `FREECAD_USER_HOME`, `FREECAD_USER_DATA`,
  `FREECAD_USER_TEMP`, and command-line override precedence.
- Planning a deprecated-path copy is acceptable platform mechanics.

Product-boundary leakage:

- Fixed in the follow-up pass. `ConfigurationSet` now carries
  `DeprecatedPathMigration`, with availability, blocked state, prompt intent,
  dry-run status, source/target paths, and a FreeCAD action name. The raw
  `migration_plan` stays inside the adapter implementation.

Helper assessment:

- `plan_copy_directory()` removes action setup; the public seam is now clearer
  because callers see a FreeCAD migration decision instead of a
  LinuxDesktop2026 migration plan.
- The path resolver call currently feels like harness coverage more than a
  natural FreeCAD refactor. A future helper should either earn its place in
  FreeCAD's environment map flow or this slice should document why it is only
  compatibility evidence.
- `write_common_config()` is a good fit for user-parameter saves because FreeCAD
  still controls the XML path and validation while the platform helper owns the
  backup/replace mechanics.

## PrusaSlicer

Remaining visible concepts:

- `load_config_bundle()` still constructs `hydrate_options` because the app
  owns model roots, target roots, and vendor-profile metadata.
- `OldDatadirCheck` returns a PrusaSlicer-shaped old-datadir migration
  decision.
- Save methods use `write_common_config()` and keep PrusaSlicer-owned
  validation callbacks.

Acceptable mechanism vocabulary:

- `ensure_config_defaults()` is a good name for shipped profile seeding as long
  as PrusaSlicer keeps parsing and merge policy.
- `plan_copy_directory()` is acceptable inside the old datadir check because the
  platform action is a dry-run directory copy.

Product-boundary leakage:

- Fixed in the follow-up pass. `OldDatadirCheck` now keeps the prompt decision
  and returns `OldDatadirMigration`, carrying availability, blocked state,
  dry-run status, source/target paths, and a PrusaSlicer action name. The raw
  `migration_plan` stays inside the adapter implementation.

Helper assessment:

- `write_common_config()` made snapshot/app-config/recent-project saves clearer.
- `ensure_config_defaults()` improved naming, but the hydration option object is
  still not especially product-shaped. That may be acceptable because vendor
  profile metadata is unusually explicit.
- The migration helper shortened setup; the product boundary improved once the
  adapter translated the raw plan into `OldDatadirMigration`.

## OpenRGB

Remaining visible concepts:

- `ResourceManager::SetupConfigurationDirectory()` uses the path resolver
  directly for resources/config/profile roots.
- JSON save helpers use a local `write_json_file()` wrapper over
  `write_common_config()`.
- Autostart methods translate `ld_desktop::effect_report` into
  `AutostartUpdate`.

Acceptable mechanism vocabulary:

- Path resolution and desktop autostart application are adapter-level platform
  mechanics.
- `AutostartUpdate` keeps product-shaped public return values and avoids
  leaking `effect_report`.

Product-boundary leakage:

- None in the public result types after report translation.

Helper assessment:

- The local `write_json_file()` wrapper is useful product glue: it names
  OpenRGB's JSON-object expectation, keeps rejected JSON from replacing the
  target, and still delegates common backup/replace mechanics to
  `write_common_config()`. If more JSON saves repeat this pattern, add a
  JSON-oriented convenience helper instead of returning to low-level options.
- Autostart remains fairly verbose; that verbosity is acceptable because it
  keeps executable, arguments, working directory, enabled state, dry-run, and
  write permission explicit.

## OBS

Remaining visible concepts:

- The public seam intentionally keeps OBS-style C conventions:
  `os_get_config_path()` writes to caller buffers and returns integer status,
  while `config_save_safe()` returns `0`/`-1`.
- `resolve_config_root()` and `config_save_safe()` use LinuxDesktop2026 only
  inside private implementation.
- The cross-port review compares the shared OBS concept across Windows and
  Unix-like platform helpers instead of treating the Linux backend as the whole
  product model.

Acceptable mechanism vocabulary:

- The private path resolver and backup-write calls are acceptable because they
  do not cross the OBS-shaped boundary.

Product-boundary leakage:

- None. This slice is the strongest evidence that C-shaped call conventions can
  remain intact while the implementation delegates platform mechanics.

Helper assessment:

- The lower-level write call remains intentional here. OBS is deliberately
  testing whether C-shaped callers can keep pointer/buffer and integer return
  conventions while delegating platform mechanics privately; switching this
  slice to the convenience facade would weaken that coverage.
- Keep: preserve OBS-style C boundaries and private LinuxDesktop2026 mechanics.
- Change: every future cross-port FlavorTest should state whether the helper
  matches a product concept shared by more than one backend.
- Defer: do not broaden the C ABI from this evidence before release-candidate
  status.

## Walnut

Remaining visible concepts:

- `ApplicationBootstrap::prepare()` keeps Walnut's `ApplicationSpecification`,
  renderer startup capability checks, GPU selection, `VULKAN_SDK` observation,
  distribution-mode entry-point choice, and headless/test launch behavior in
  Walnut vocabulary.
- `ResourceLocator::resolveImagePath()` preserves
  `Walnut::Image::Image(std::string_view path)`-style caller input while using
  the selected resource root for relative assets.
- `ApplicationLifecycle` models `Close`, `Shutdown`, and the entry-point loop
  state without owning the renderer or frame loop.

Acceptable mechanism vocabulary:

- `ld_paths::resolve_app_paths()` is acceptable inside the bootstrap adapter
  because Walnut needs executable-adjacent resources and an ordinary config root,
  but does not need settings, migration, desktop effects, or watch vocabulary.
- Current friction is lower after the path API cleanup: Walnut now reads
  executable/resource values through `selected_locations` and config through
  ordinary `selected` path families, so the adapter no longer depends on
  resource locations masquerading as user roots.

Product-boundary leakage:

- None in the public seam. Path diagnostics are translated into
  `StartupDiagnostic` values before returning from `prepare()`, and image lookup
  reports missing assets in Walnut terms.

Helper assessment:

- The existing `ld_paths` API is enough for the first Walnut slice. A narrower
  diagnostics helper is not justified yet because Walnut's value is precisely
  that it does not need the heavier settings-root model.
- The former private FlavorTest `platform_paths.hpp` gap is closed for Walnut:
  deterministic user roots now flow through the generated public path-default
  helper and normal resolver options instead of test-only XDG/AppData
  environment scaffolding.
- This slice is useful negative evidence for `linuxdesktop::root::request_builder`: graphics
  app bootstrap with executable-adjacent assets reads more naturally with direct
  path resolver options than with the settings-oriented root helper.

## ld_paths Surface

Current friction:

- The deliberate pre-1.0 break is complete for task 45 and task 46:
  path-list candidates, plugin path-set candidates, resolver path candidates,
  and executable/install/resource location candidates are separate public
  vocabularies in both C++ and C.
- `path_family` is now ordinary user/platform roots only. Plugin search roots
  are path sets, and executable/install/resource values are `location_role`
  entries available from `ld_paths` alone.
- This keeps Walnut/OpenRGB direct-path adapters lightweight while giving future
  `ld_root` work clean install/resource inputs instead of inheriting
  compatibility-shaped path-family names.

## OpenIPC Dashboard

Remaining visible concepts:

- `ApplicationProfile::resolve()` keeps Dashboard's desktop versus service
  profile split, `OPENIPC_DATA_ROOT`, `--data-root`, QSettings placement,
  log/state/modules/analytics/evidence paths, and server-only offscreen intent
  in Dashboard vocabulary.
- `ServerModeBootstrap`, `DeploymentPolicy`, readiness reporting,
  `PathNormalizer`, and browser diagnostics stay Dashboard-shaped because Qt,
  web routing, administrator bootstrap, import semantics, and redaction policy
  are product responsibilities.

Acceptable mechanism vocabulary:

- `ld_paths::resolve_app_paths()` is acceptable for the desktop default profile
  because Dashboard can consume normal XDG/AppData roots without surrendering
  its service-profile contract.
- The service data-root branch deliberately remains product-owned. Its
  `config/`, `data/`, `evidence/`, QSettings, users, state, modules, analytics,
  and log layout is part of Dashboard's documented autonomous server contract,
  not generic LinuxDesktop2026 placement policy.

Product-boundary leakage:

- None in the public seam. LinuxDesktop2026 path diagnostics are translated into
  `DashboardDiagnostic` values, and the other return types use Dashboard terms:
  deployment profile, readiness, administrator bootstrap, path normalization,
  and browser diagnostics.

Helper assessment:

- This slice is useful negative evidence for over-generalizing service roots.
  A helper that only says "data override" would be too weak for Dashboard
  because the override selects a whole isolated service profile with multiple
  subordinate roots and browser-facing security constraints.
- The former private FlavorTest `platform_paths.hpp` gap is closed for the
  desktop profile: generated public platform defaults now provide deterministic
  config/data/state/runtime roots without exposing raw LinuxDesktop2026 reports
  at Dashboard boundaries.
- A future profile helper may be worth considering only if more FlavorTests need
  to express "one absolute root selects a named service profile with app-owned
  child layout" without leaking LinuxDesktop2026 reports.
- As a reference case, Dashboard says LinuxDesktop2026 should adapt around Qt
  application lifecycle, QSettings mechanics, QML startup, and event-loop
  ownership instead of competing with them.

## Cross-Flavor Follow-Up

- Treat root topology as a solved cross-flavor boundary for the current batch.
  Notepad++, qBittorrent, and KiCad now use `linuxdesktop::root` for shared
  root setup mechanics. That does not justify moving the same behavior into
  `ld_paths` or keeping a duplicate in `ld_settings`.
- The root-boundary design now classifies Notepad++, qBittorrent, and KiCad as
  positive evidence for shared root topology, Walnut as negative evidence for
  overusing root builders in simple graphics bootstrap, and OpenIPC Dashboard
  as negative evidence for flattening service-profile policy into a generic
  helper. See `.scratch/review-hardening/specs/root-module-boundary.md`.
- Keep migration plans internal in FlavorTests. KeePassXC, FreeCAD, and
  PrusaSlicer now translate raw migration plans into product-shaped migration
  results before callers see them.
- Keep ordinary durable config saves on `write_common_config()` in qBittorrent,
  KeePassXC, KiCad, FreeCAD, OpenRGB, Audacity, Notepad++, and PrusaSlicer.
  Preserve direct `write_with_backup()` only where the product seam is testing a
  lower-level behavior, such as Notepad++ backup restore or OBS C-style saves.
- Keep `linuxdesktop::root::request_builder` focused on root topology. It
  earned promotion in Notepad++, qBittorrent, and KiCad by removing mechanical
  setup without hiding product policy. Do not force KeePassXC or FreeCAD
  through it until their product-owned XDG and environment precedence rules can
  be expressed more clearly than direct request objects.
- Keep diagnostics internal by default. If a product needs startup or migration
  diagnostics, translate them into product logging, warning, or prompt data
  before storing them in product-facing state.
- Treat Walnut as a lightweight graphics-app reference slice for path and
  lifecycle seams, not as evidence that LinuxDesktop2026 has solved Vulkan,
  GLFW, or renderer ownership.
- Keep FlavorTests honest about consumer ergonomics. Walnut and OpenIPC
  Dashboard now use public runtime/CMake path defaults, so future FlavorTests
  should not add private path-default policy helpers to make product adapters
  look easier than installed users would experience.
