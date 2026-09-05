# FlavorTest API Friction Notes

These notes describe the current integration feel of the FlavorTests and the
Notepad++ cross-port. They are not a changelog. A passing FlavorTest means the
behavior can be represented through LinuxDesktop2026; it does not mean the API
is painless enough for upstream adoption.

Read each section as a boundary check:

- Fit: LinuxDesktop2026 vocabulary that belongs at the product adapter edge.
- Friction: places where the caller still has to know too much, translate too
  much, duplicate request/lookup names, or work around a missing or awkward
  LinuxDesktop2026 API.
- Leakage: LinuxDesktop2026 concepts that escape into product-facing types,
  CMake linkage, long-lived state, or tests in a way real users would feel and
  that should be removed or contained.
- Boundary notes: acceptable product/toolkit ownership that should stay outside
  LinuxDesktop2026 unless repeated evidence justifies a new helper.

## Cross-Cutting State

- `ld_paths` is the lightest usable entry point. It handles ordinary
  config/data/state/cache/runtime roots, executable/install/resource locations,
  and plugin path sets without requiring settings, root topology, migration, or
  desktop effects.
- `ld_root` is the shared topology layer for app/user-owned roots. It depends on
  `ld_paths`, but callers that only need topology do not need to include
  `ld_settings`.
- `ld_settings` is settings-lifecycle specific: default hydration, common config
  writes, and settings-layer reporting. Generic named roots belong in `ld_root`,
  not here. The installed package enforces that boundary: a settings-only
  consumer links only `LinuxDesktop2026::ld_settings`, and the install-tree
  fixture fails configure if `ld_settings` exposes `ld_desktop` through its
  CMake interface.
- `ld_migration` is intentionally separate. Flavor adapters should keep raw
  migration plans private and return product-shaped migration decisions.
- The public CMake path-default generator is necessary integration surface, not
  test scaffolding. FlavorTests and cross-port code can get deterministic XDG or
  Windows defaults without carrying private `platform_paths.hpp` helpers.

Current global pain:

- `docs/FlavorTests/CMakeLists.txt` links each flavor support library only to
  the modules its source uses. That keeps dependency evidence honest, but it
  also makes accidental public-header coupling visible when a flavor pulls in a
  broader module than its product shape really needs.
- Diagnostics are generic across modules. That is useful at the adapter edge,
  and `ld_core` carries severity plus library-owned handling flags for logging,
  status display, and user prompts.
- Cross-module workflows expose module ownership directly in user code. Calls
  such as `linuxdesktop::root::request_builder` followed by
  `linuxdesktop::settings::write_with_backup` make the dependency story
  explicit, but users still have to know which module owns each operation.
- Transitive desktop linkage is not an acceptable shortcut for cross-module
  ergonomics. Callers that need desktop effects should link `ld_desktop`
  directly, even when settings, paths, migration, and desktop work happen in
  the same product adapter.

## Notepad++

Fit:

- The cross-port consumes an installed LinuxDesktop2026 package, runs
  `linuxdesktop2026_generate_path_defaults()`, and passes generated platform
  defaults into `linuxdesktop::root::options`.
- `linuxdesktop::root` fits the main layout decision: install resources,
  command-line settings directory, cloud settings directory, portable marker,
  privileged install policy, session root, and plugin config root.
- `linuxdesktop::settings` fits default XML hydration and common validated
  config writes.
- `linuxdesktop::migration` fits the legacy config import mechanic when the raw
  plan stays behind the Notepad++ adapter.
- `ld_core` provides product-diagnostic translation helpers so adapters can map
  shared severity, codes, messages, related paths, and diagnostic handling flags
  into product-owned diagnostics without hand-copying each report shape.
  `"portable-denied-privileged-install"` becomes a Notepad++ diagnostic with
  `handling.prompt_user = true`, so the adapter does not need a parallel
  disposition vocabulary or diagnostic-code table.

Friction:

- Local config needs both requested and active state. The API exposes that, but
  product code must keep the two flags straight or it will blur "marker exists"
  with "portable mode accepted."
- The cross-port builds `linuxdesktop::root::options` manually while the in-tree
  FlavorTest uses `request_builder`. Both are valid, but the split makes it
  harder to tell which style should be recommended for real consumers.
- Named roots such as `"xml-config"`, `"session"`, and `"plugin-config"` are
  stringly typed. Product adapters must look them up by the same names they
  requested.

Leakage:

- `notepadpp_settings_backend.hpp` exposes Notepad++ result structs, but their
  fields still mirror LinuxDesktop2026 decisions closely: copied defaults,
  validated write backup, and dry-run import actions. That is honest evidence,
  but still asks the product adapter to translate library mechanics into
  application behavior names.

Boundary notes:

- The CMake dependency list in the cross-port is `ld_root`, `ld_settings`, and
  `ld_migration`. The cross-port does not link `ld_paths` directly because
  `ld_root` carries that dependency.
- Current maintained-proof metrics are recorded in
  `docs/consumer-branches/notepadpp-settings-proof.md`. The live snapshot is
  252 lines of backend implementation, 109 lines of product-shaped header, five
  LinuxDesktop2026 concept families, and zero platform preprocessor branches in
  the proof adapter.

## Audacity

Fit:

- `write_common_config()` fits `FileConfig::Flush()`: Audacity owns the target
  file and validation, LinuxDesktop2026 owns backup and atomic replace
  mechanics.

Boundary notes:

- Audacity's probing and warning loop stays product code. This slice proves
  common write mechanics, not broader settings-root adoption.

## qBittorrent

Fit:

- `linuxdesktop::root::request_builder` fits `Profile::init()` for app identity,
  executable resource root, controlled test environment, portable marker policy,
  and a machine-local log root.
- `write_common_config()` fits the ordinary `qBittorrent.ini` save path.

Friction:

- Log placement is a named root request and lookup pair. The lookup is still
  string keyed.

Boundary notes:

- qBittorrent owns the policy branch where command-line profile roots win,
  otherwise an executable-adjacent `profile` directory activates portable mode.
  LinuxDesktop2026 should not hide that precedence unless another product
  repeats the same shape.
- `SpecialFolder` stays product-shaped and does not expose LinuxDesktop2026
  root names.

## KeePassXC

Fit:

- `linuxdesktop::root::options` fits the roaming/local split and a
  machine-local `local-settings` named root.
- `write_common_config()` fits settings export.
- `plan_rename_file()` fits old cache config migration when translated into
  `LocalConfigMigration`.

Friction:

- KeePassXC has enough XDG and roaming/local vocabulary that the raw options
  object is clearer than the fluent builder. The API does not clearly signal
  when callers should prefer raw options over `request_builder`.
- The product still needs to translate generic portable/root diagnostics into
  KeePassXC prompts or warnings.
- The local-settings root uses LinuxDesktop2026 purpose and ownership terms in
  the adapter, so KeePassXC still has to map those terms back to its own
  naming.

Boundary notes:

- The public migration result is product-shaped. Raw `migration_plan` does not
  cross the KeePassXC seam.

## KiCad

Fit:

- `linuxdesktop::root::request_builder` fits ordinary config topology plus
  named roots for colors, toolbars, and project backups.
- `write_common_config()` fits JSON settings saves.

Friction:

- LinuxDesktop2026 can provide a generic backup named root for KiCad, but there
  is no helper for keyed-by-project fallback paths. The adapter still owns that
  lookup logic.
- The named-root request/lookup pattern is readable for three roots, but it
  would get noisy for a larger KiCad component map.

Boundary notes:

- No public KiCad result type currently exposes LinuxDesktop2026 reports.

## FreeCAD

Fit:

- `linuxdesktop::paths::resolve_app_paths()` is useful as a
  validation/exercise point for FreeCAD-specific environment variables that
  select user paths.
- `plan_copy_directory()` fits deprecated path migration when translated into
  `DeprecatedPathMigration`.
- `write_common_config()` fits XML user-parameter saves.

Friction:

- FreeCAD owns `FREECAD_USER_HOME`, `FREECAD_USER_DATA`,
  `FREECAD_USER_TEMP`, and command-line override precedence. LinuxDesktop2026
  currently sits beside that environment map rather than simplifying it.
- The path resolver call feels like compatibility coverage more than a natural
  FreeCAD refactor. A future helper would need to model product environment
  precedence directly to earn its place.

Boundary notes:

- Public FreeCAD migration state is product-shaped. The raw copy-directory plan
  stays private.

## PrusaSlicer

Fit:

- `ensure_config_defaults()` fits shipped vendor profile seeding.
- `write_common_config()` fits snapshot, app config, and recent-project saves.
- `plan_copy_directory()` fits old datadir migration once translated into
  `OldDatadirMigration`.

Friction:

- `config_defaults_options` is still LinuxDesktop2026-shaped at a point where the
  product thinks in vendor bundles, model roots, target roots, and merge
  metadata.
- Vendor profile metadata is product-specific enough that a generic helper
  should not try to hide parsing or merge policy.

Leakage:

- `prusaslicer_flavor.hpp` still exposes `linuxdesktop::settings::config_file`
  and `validation_callback` in product-facing FlavorTest types. That keeps the
  slice shorter, but a real adapter should translate those to PrusaSlicer-owned
  types.

## OpenRGB

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits resource/config/profile root
  discovery.
- `write_common_config()` fits JSON settings saves through a small local
  `write_json_file()` adapter.
- `linuxdesktop::desktop` fits autostart application as long as
  `AutostartUpdate` remains the public result.

Friction:

- LinuxDesktop2026 has no JSON-oriented write helper. OpenRGB wraps
  `write_common_config()` locally to get validated JSON saves.
- Autostart remains verbose because executable, arguments, working directory,
  enabled state, dry-run mode, and write permission are all explicit.

Boundary notes:

- Public OpenRGB result types are product-shaped. Desktop effect reports and
  path diagnostics stay inside the adapter.

## OBS

Fit:

- `linuxdesktop::paths` fits private config-root resolution.
- `write_with_backup()` fits `config_save_safe()` because OBS intentionally
  keeps C-shaped buffers and integer status conventions.

Boundary notes:

- OBS deliberately uses the lower-level write API because the convenience write
  facade would be less representative of OBS's actual C boundary.
- This slice is evidence that LinuxDesktop2026 can stay private, but it does
  not prove the C ABI is broad enough for general adoption.
- No product-facing OBS boundary exposes LinuxDesktop2026 types.

## Walnut

Fit:

- `linuxdesktop::paths` is enough for executable-adjacent resources and a
  normal config root.
- Walnut keeps renderer startup, GPU selection, distribution-mode entry point,
  headless/test launch, and image lookup in product vocabulary.

Boundary notes:

- Walnut is negative evidence for forcing `linuxdesktop::root` into simple
  graphics bootstrap. Direct path resolver options are easier to read here.
- Diagnostics translate directly into product-owned `StartupDiagnostic`.
- No product-facing Walnut seam exposes LinuxDesktop2026 paths reports.
- The FlavorTest harness links Walnut to `ld_paths` only, matching the source
  dependency.

## OpenIPC Dashboard

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits the desktop default profile.
- The service data-root branch correctly remains product-owned: it selects a
  whole isolated service profile with config, data, evidence, QSettings, users,
  state, modules, analytics, logs, and browser-facing security constraints.

Friction:

- LinuxDesktop2026 does not currently model "one absolute root selects an
  app-owned service profile with named child layout." That may be a future
  helper if another product repeats the shape.

Boundary notes:

- Dashboard's Qt lifecycle, QSettings mechanics, QML startup, redaction policy,
  and event-loop ownership remain outside the LinuxDesktop2026 abstraction.
- Public Dashboard result types use Dashboard vocabulary. LinuxDesktop2026 path
  diagnostics stay inside the adapter.
- The FlavorTest harness links Dashboard to `ld_paths` only, matching the
  source dependency.

## Gearcoleco

Fit:

- `linuxdesktop::settings::root_builder` fits Gearcoleco's installed versus
  portable startup decision, including the command-line `--portable` case and
  executable-adjacent `portable.ini` marker.
- `ensure_config_defaults()` fits first-run `gearcoleco.ini` seeding from the
  executable resource root.
- Executable-relative `gamecontrollerdb.txt` and ROM-relative `.sym`/`.noi`
  lookup stay in Gearcoleco code.

Boundary notes:

- Gearcoleco owns the product-facing phrase for portable mode: store
  configuration, state, cache, and related user files beside the emulator
  binary. LinuxDesktop2026 supplies one `portable_root_request` for that policy
  without taking over product wording.
- The root builder uses LinuxDesktop2026 portable-root vocabulary. Gearcoleco's
  adapter maps that vocabulary to emulator startup semantics at the product
  boundary.
- Product-facing startup results expose Gearcoleco concepts. LinuxDesktop2026
  root and hydration reports stay inside the adapter.

## CtrlrX

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits ordinary CtrlrX resource,
  config, data, and cache roots without competing with JUCE.
- `write_common_config()` fits standalone preference saves to `Ctrlr.settings`.
- `resolve_plugin_path_sets()` fits exported plugin destinations for VST3,
  Audio Unit, and AAX while CtrlrX keeps format and panel-ID policy.

Boundary notes:

- The standalone-versus-plugin guard is product policy and wraps the settings
  write. LinuxDesktop2026 should not decide whether a plugin instance mutates
  global application preferences.
- CtrlrX chooses when a plugin export is allowed and which target format the
  user selected. LinuxDesktop2026 resolves search-root sets; JUCE and CtrlrX
  own export semantics and host compatibility rules.
- Public CtrlrX results do not expose LinuxDesktop2026 reports. Plugin path
  kind lookup stays inside the adapter.

## SmartServoFramework

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits SmartServoGui config, data,
  state, device profile, and log roots.
- `write_common_config()` fits persistent device settings once the GUI has
  chosen the device-specific target file.

Friction:

- LinuxDesktop2026 provides no hardware-companion startup helper for serial
  access, driver installation, or OS permission checks. The hardware-facing
  startup pain is only adjacent to the library.
- LinuxDesktop2026 has no reusable device-profile filename helper. The adapter
  still has to sanitize device names at the settings-write boundary.

Boundary notes:

- Public SmartServoGui diagnostics are product-owned. LinuxDesktop2026 only
  contributes translated path and write diagnostics behind the GUI seam.

## KickCAT

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits optional KickUI and EEPROM
  editor config/cache/runtime roots.
- `write_common_config()` fits GUI settings writes for desktop tooling.
- Runtime roots fit simulator socket placement without pulling the EtherCAT core
  into LinuxDesktop2026.

Friction:

- ESI XML lookup is partly resource-root shaped and partly domain-shaped. A
  generic path helper can provide search roots, but product code still owns
  validation, device matching, and launch consequences.

Boundary notes:

- KickCAT is a boundary challenge more than an adoption slice. Network
  interface selection, real-time mode, embedded targets, and bus launch policy
  are not LinuxDesktop2026 responsibilities.
- Public KickCAT tool results expose tool and master-launch vocabulary.
  LinuxDesktop2026 reports stay inside the optional tooling adapter.

## Dependency Pain

Current desired dependency shape:

- Link `LinuxDesktop2026::ld_paths` alone for plain platform paths,
  executable/install/resource locations, path lists, plugin path sets, and
  lightweight bootstrap adapters.
- Link `LinuxDesktop2026::ld_root` when the caller needs user/app-owned root
  topology, portable policy, overrides, named roots, or component
  roots.
- Link `LinuxDesktop2026::ld_settings` when the caller needs settings
  hydration, settings layers, or validated settings writes.
- Link `LinuxDesktop2026::ld_migration` when the caller needs dry-run
  application-settings copy/move/import planning for regular files,
  directories, or app-settings Registry snapshots.
- Link `LinuxDesktop2026::ld_desktop` for desktop effects such as autostart
  integration.

Evidence harness guardrails:

- Future FlavorTests should continue using generated public path defaults. No
  private path-default helpers should be added to make tests easier than real
  installed users' code.
- Install-tree consumers should stay module-specific enough to catch accidental
  public-header or CMake interface coupling. In particular, the settings
  consumer should not include `desktop`, `paths`, or `watch` headers just to
  prove package consumption.
