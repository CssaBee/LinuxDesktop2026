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

- `NppParameters::load()` still assembles a full `ld_settings::root_options`
  request because Notepad++ combines install-local config, command-line
  settings, cloud settings, privileged-install denial, session roots, and plugin
  config roots.
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
- Root helpers helped the plugin root, but the main root request remains dense.
  That density reflects real Notepad++ policy rather than just missing syntax.

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

- `Profile::init()` still builds a root request for profile overrides,
  executable-adjacent portable mode, home/environment injection, and log root
  placement.
- `saveFileLoggerSettings()` still uses `write_with_backup()` directly even
  though it follows the common durable config-write pattern.

Acceptable mechanism vocabulary:

- `resolve_app_roots()` and the log named-root request are adapter-level
  platform mechanics.
- Keeping qBittorrent's `SpecialFolder` enum at the product boundary is good;
  callers do not need to learn LinuxDesktop2026 root terminology.

Product-boundary leakage:

- None in the public result types. The LinuxDesktop2026 concepts remain inside
  the adapter implementation.

Helper assessment:

- Root helpers reduce the named-root boilerplate but do not make the portable
  profile decision easier to reason about; qBittorrent's profile model is still
  inherently application-specific.
- The write facade should be adopted for file logger settings if the desired
  durable behavior is the same as other config writes.

## KeePassXC

Remaining visible concepts:

- `Config::open()` still combines portable config, roaming/local split, XDG
  overrides, and a machine-local named root.
- `exportSettings()` still uses `write_with_backup()` directly for a common
  durable validated export.
- `migrateOldLocalConfig()` returns `linuxdesktop::migration::migration_plan`
  from a product method.

Acceptable mechanism vocabulary:

- Root discovery and the local-settings named root are acceptable adapter
  mechanics because KeePassXC has explicit roaming/local policy.
- Using `plan_move_file()` internally is acceptable because the old cache config
  migration is a platform file move.

Product-boundary leakage:

- Returning `migration_plan` is leakage. A product-shaped result should say
  whether a local-config migration is available, whether it is dry-run only, and
  what source/target the UI or caller needs.

Helper assessment:

- `plan_move_file()` removes mechanical action construction, but it does not
  improve the public seam while the method still returns `migration_plan`.
- Root helpers help the local-settings declaration; they do not resolve the
  deeper roaming/local naming mismatch between KeePassXC and LinuxDesktop2026.

## KiCad

Remaining visible concepts:

- `SETTINGS_MANAGER` still builds named roots for colors, toolbars, and
  project-backups.
- `Save()` still uses `write_with_backup()` directly for a common durable JSON
  settings write.

Acceptable mechanism vocabulary:

- Named roots are acceptable inside the constructor because KiCad has real
  user, project, color, toolbar, and backup placement distinctions.
- `GetPathForSettingsFile()` and `GetBackupRootForProject()` stay product-shaped
  and preserve KiCad's project policy.

Product-boundary leakage:

- None in public result types. LinuxDesktop2026 reports are translated before
  leaving the adapter implementation.

Helper assessment:

- Root helpers made the named roots easier to scan, but KiCad still needs a
  more fluent way to express "component config below this app config root" and
  "backup root keyed by project" without manual fallback paths.
- The write facade should be considered for `Save()` if it can keep the durable
  JSON validation choice obvious.

## FreeCAD

Remaining visible concepts:

- `ApplicationConfig::build()` still calls `ld_paths::resolve_app_paths()` only
  to validate or exercise the path layer after FreeCAD-specific environment
  variables have already selected paths.
- `ConfigurationSet` stores
  `linuxdesktop::migration::migration_plan deprecated_path_migration`.
- `saveUserParameter()` still uses `write_with_backup()` directly for a common
  durable XML-shaped user-parameter save.

Acceptable mechanism vocabulary:

- Consulting the path resolver is acceptable inside the adapter, but FreeCAD
  must continue to own `FREECAD_USER_HOME`, `FREECAD_USER_DATA`,
  `FREECAD_USER_TEMP`, and command-line override precedence.
- Planning a deprecated-path copy is acceptable platform mechanics.

Product-boundary leakage:

- Storing `migration_plan` in `ConfigurationSet` is leakage. The product seam
  should carry a FreeCAD-shaped deprecated-path migration decision with source,
  target, dry-run status, and prompt/action intent.

Helper assessment:

- `plan_copy_directory()` removed action setup but did not fix local reasoning
  because callers still see a LinuxDesktop2026 migration plan.
- The path resolver call currently feels like harness coverage more than a
  natural FreeCAD refactor. A future helper should either earn its place in
  FreeCAD's environment map flow or this slice should document why it is only
  compatibility evidence.

## PrusaSlicer

Remaining visible concepts:

- `load_config_bundle()` still constructs `hydrate_options` because the app
  owns model roots, target roots, and vendor-profile metadata.
- `OldDatadirCheck` returns
  `linuxdesktop::migration::migration_plan migration`.
- Save methods use `write_common_config()` and keep PrusaSlicer-owned
  validation callbacks.

Acceptable mechanism vocabulary:

- `ensure_config_defaults()` is a good name for shipped profile seeding as long
  as PrusaSlicer keeps parsing and merge policy.
- `plan_copy_directory()` is acceptable inside the old datadir check because the
  platform action is a dry-run directory copy.

Product-boundary leakage:

- Returning `migration_plan` from `OldDatadirCheck` is leakage. The caller needs
  a prompt decision and source/target paths, not the full LinuxDesktop2026 plan
  object.

Helper assessment:

- `write_common_config()` made snapshot/app-config/recent-project saves clearer.
- `ensure_config_defaults()` improved naming, but the hydration option object is
  still not especially product-shaped. That may be acceptable because vendor
  profile metadata is unusually explicit.
- The migration helper shortened setup but did not improve the product boundary
  while `migration_plan` is still visible.

## OpenRGB

Remaining visible concepts:

- `ResourceManager::SetupConfigurationDirectory()` uses the path resolver
  directly for resources/config/profile roots.
- JSON save helpers still use lower-level `write_with_backup()` behind a local
  `write_json_file()` wrapper.
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

- The local `write_json_file()` wrapper is clear, but it is also evidence that
  `write_common_config()` may not be sufficiently discoverable or expressive for
  JSON config saves outside the first three migrated flavors.
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

- The common write facade is not used here. That is acceptable for now because
  OBS is deliberately testing whether low-level C-style behavior stays possible,
  but it should be revisited if the helper claims to cover all common safe
  config saves.

## Cross-Flavor Follow-Up

- Translate remaining public `migration_plan` exposure in KeePassXC, FreeCAD,
  and PrusaSlicer before treating migration helpers as adoption-ready.
- Revisit durable config writes in qBittorrent, KeePassXC, KiCad, FreeCAD,
  OpenRGB, and OBS. Either adopt `write_common_config()` where it improves local
  reasoning or document why the lower-level `write_with_backup()` call is the
  clearer product fit.
- Consider a root-request builder only if it makes Notepad++, qBittorrent,
  KeePassXC, KiCad, and FreeCAD easier to read without inventing a second
  application-profile model.
- Keep diagnostics internal by default. If a product needs startup or migration
  diagnostics, translate them into product logging, warning, or prompt data
  before storing them in product-facing state.
