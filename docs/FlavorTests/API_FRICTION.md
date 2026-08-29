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

Remaining visible concepts:

- `NppParameters::load()` now uses `ld_settings::root_request_builder` for the
  common app identity, root, override, portable-marker, privileged-install, and
  named-root mechanics.
- The product state keeps copied `linuxdesktop::diagnostic` values because
  Notepad++-style startup diagnostics need to survive after root/default
  resolution.
- Session backup recovery still uses the lower-level `write_with_backup()`
  because it deliberately disables backup creation while restoring from an
  existing `.bak` file.

Acceptable mechanism vocabulary:

- `resolve_app_roots()`, `ensure_config_defaults()`, and the plugin-config named
  root are adapter-level platform mechanics.
- `write_common_config()` is a good fit for normal session, shortcut, and
  find-history saves because validation remains Notepad++-owned.

Product-boundary leakage:

- `loaded_parameters::diagnostics` exposes `linuxdesktop::diagnostic` directly.
  That is useful for the harness, but a real Notepad++ seam would probably
  translate it to startup warnings or log messages before storing it in the
  parameter object.

Helper assessment:

- The config-write facade shortened the repeated save paths and made intent
  clearer.
- The root-request builder is worth keeping as an experimental C++ helper here.
  It makes repeated request mechanics easier to scan while the product policy
  remains visible in the call chain: command-line settings, cloud settings,
  portable marker, privileged-install denial, and plugin config root.

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

- `Profile::init()` now uses `root_request_builder` for app identity,
  executable resource root, injected environment, portable marker, and log root
  placement.
- The profile-dir override branch remains outside the builder chain because it
  is qBittorrent policy: command-line profile roots win, otherwise an
  executable-adjacent `profile` directory activates portable mode.
- `saveFileLoggerSettings()` now uses `write_common_config()` for the ordinary
  durable settings save.

Acceptable mechanism vocabulary:

- `resolve_app_roots()` and the log named-root request are adapter-level
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

- `SETTINGS_MANAGER` uses `root_request_builder` for injected environment setup
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

## Cross-Flavor Follow-Up

- Keep migration plans internal in FlavorTests. KeePassXC, FreeCAD, and
  PrusaSlicer now translate raw migration plans into product-shaped migration
  results before callers see them.
- Keep ordinary durable config saves on `write_common_config()` in qBittorrent,
  KeePassXC, KiCad, FreeCAD, OpenRGB, Audacity, Notepad++, and PrusaSlicer.
  Preserve direct `write_with_backup()` only where the product seam is testing a
  lower-level behavior, such as Notepad++ backup restore or OBS C-style saves.
- Keep `root_request_builder` experimental for now. It earned promotion
  consideration in Notepad++, qBittorrent, and KiCad by removing mechanical
  setup without hiding product policy. Do not force KeePassXC or FreeCAD through
  it until their product-owned XDG and environment precedence rules can be
  expressed more clearly than the direct request objects.
- Keep diagnostics internal by default. If a product needs startup or migration
  diagnostics, translate them into product logging, warning, or prompt data
  before storing them in product-facing state.
