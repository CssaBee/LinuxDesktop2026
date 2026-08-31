# FlavorTest API Friction Notes

These notes describe the current integration feel of the FlavorTests and the
Notepad++ cross-port. They are not a changelog. A passing FlavorTest means the
behavior can be represented through LinuxDesktop2026; it does not mean the API
is painless enough for upstream adoption.

Read each section as a boundary check:

- Fit: LinuxDesktop2026 vocabulary that belongs at the product adapter edge.
- Friction: places where the caller still has to know too much, translate too
  much, or pull in more shape than the product naturally has.
- Leakage: LinuxDesktop2026 concepts that escape into product-facing types,
  CMake linkage, long-lived state, or tests in a way real users would feel.

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
  not here.
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

## Notepad++

Fit:

- The cross-port consumes an installed LinuxDesktop2026 package, runs
  `linuxdesktop2026_generate_path_defaults()`, and passes generated platform
  defaults into `linuxdesktop::root::options`.
- `linuxdesktop::root` fits the main layout decision: install resources,
  command-line settings directory, cloud settings directory, app-local marker,
  privileged install policy, session root, and plugin config root.
- `linuxdesktop::settings` fits default XML hydration and common validated
  config writes.
- `linuxdesktop::migration` fits the legacy config import mechanic when the raw
  plan stays behind the Notepad++ adapter.
- `ld_core` provides product-diagnostic translation helpers so adapters can map
  shared severity, codes, messages, related paths, and diagnostic handling flags
  into product-owned diagnostics without hand-copying each report shape.
  `"app_local-denied-privileged-install"` becomes a Notepad++ diagnostic with
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
- The CMake dependency list in the cross-port is honest enough:
  `ld_root`, `ld_settings`, and `ld_migration`. It does not need `ld_paths`
  directly because `ld_root` carries that dependency.

## Audacity

Fit:

- `write_common_config()` fits `FileConfig::Flush()`: Audacity owns the target
  file and validation, LinuxDesktop2026 owns backup and atomic replace
  mechanics.

Friction:

- The helper is good for writes only. Audacity's probing and warning loop stays
  product code, so there is no broader settings-root proof here.

Leakage:

- The public product seam exposes Audacity-shaped write results. The
  LinuxDesktop2026 write report stays inside the adapter.

## qBittorrent

Fit:

- `linuxdesktop::root::request_builder` fits `Profile::init()` for app identity,
  executable resource root, controlled test environment, portable marker policy,
  and a machine-local log root.
- `write_common_config()` fits the ordinary `qBittorrent.ini` save path.

Friction:

- The product policy branch remains outside the builder: command-line profile
  roots win, otherwise an executable-adjacent `profile` directory activates
  portable mode. That is the right ownership, but it means the adapter still has
  meaningful product branching around the LinuxDesktop2026 call.
- Log placement is a named root request and lookup pair. The lookup is still
  string keyed.

Leakage:

- No product-facing qBittorrent result type needs LinuxDesktop2026 root names.
  `SpecialFolder` stays product-shaped.

## KeePassXC

Fit:

- `linuxdesktop::root::options` fits the roaming/local split and a
  machine-local `local-settings` named root.
- `write_common_config()` fits settings export.
- `plan_move_file()` fits old cache config migration when translated into
  `LocalConfigMigration`.

Friction:

- KeePassXC has enough XDG and roaming/local vocabulary that the raw options
  object is clearer than the fluent builder. That is a sign the builder should
  stay optional, not become the blessed path for all root consumers.
- The product still needs to translate generic app-local/root diagnostics into
  KeePassXC prompts or warnings.
- The local-settings root uses LinuxDesktop2026 purpose and ownership terms in
  the adapter. That is acceptable, but it does not map one-to-one to
  KeePassXC's own naming.

Leakage:

- The public migration result is product-shaped. Raw `migration_plan` does not
  cross the KeePassXC seam.

## KiCad

Fit:

- `linuxdesktop::root::request_builder` fits ordinary config topology plus
  named roots for colors, toolbars, and project backups.
- `write_common_config()` fits JSON settings saves.

Friction:

- Project backup placement remains KiCad policy. A generic backup named root can
  provide a base, but keyed-by-project fallback logic should stay in KiCad.
- The named-root request/lookup pattern is readable for three roots, but it
  would get noisy for a larger KiCad component map.

Leakage:

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

Leakage:

- Public FreeCAD migration state is product-shaped. The raw copy-directory plan
  stays private.

## PrusaSlicer

Fit:

- `ensure_config_defaults()` fits shipped vendor profile seeding.
- `write_common_config()` fits snapshot, app config, and recent-project saves.
- `plan_copy_directory()` fits old datadir migration once translated into
  `OldDatadirMigration`.

Friction:

- `hydrate_options` is still LinuxDesktop2026-shaped at a point where the
  product thinks in vendor bundles, model roots, target roots, and merge
  metadata. It is acceptable, but it is not especially graceful.
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

- The JSON wrapper is product glue, not generic settings topology. If more
  slices repeat it, a JSON-oriented write helper may be justified.
- Autostart remains verbose because executable, arguments, working directory,
  enabled state, dry-run mode, and write permission are all explicit.

Leakage:

- Public OpenRGB result types are product-shaped. Desktop effect reports and
  path diagnostics stay inside the adapter.

## OBS

Fit:

- `linuxdesktop::paths` fits private config-root resolution.
- `write_with_backup()` fits `config_save_safe()` because OBS intentionally
  keeps C-shaped buffers and integer status conventions.

Friction:

- This slice deliberately uses the lower-level write API. The convenience write
  facade would be less representative of OBS's actual C boundary.
- OBS is good evidence that LinuxDesktop2026 can stay private, but it does not
  prove the C ABI is broad enough for general adoption.

Leakage:

- No product-facing OBS boundary exposes LinuxDesktop2026 types.

## Walnut

Fit:

- `linuxdesktop::paths` is enough for executable-adjacent resources and a
  normal config root.
- Walnut keeps renderer startup, GPU selection, distribution-mode entry point,
  headless/test launch, and image lookup in product vocabulary.

Friction:

- Walnut is negative evidence for forcing `linuxdesktop::root` into simple
  graphics bootstrap. Direct path resolver options are easier to read here.
- Diagnostics still need product translation into `StartupDiagnostic`, but the
  translation is straightforward.

Leakage:

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
- Dashboard's Qt lifecycle, QSettings mechanics, QML startup, redaction policy,
  and event-loop ownership remain outside the LinuxDesktop2026 abstraction.

Leakage:

- Public Dashboard result types use Dashboard vocabulary. LinuxDesktop2026 path
  diagnostics stay inside the adapter.
- The FlavorTest harness links Dashboard to `ld_paths` only, matching the
  source dependency.

## Dependency Pain

Current desired dependency shape:

- Link `LinuxDesktop2026::ld_paths` alone for plain platform paths,
  executable/install/resource locations, path lists, plugin path sets, and
  lightweight bootstrap adapters.
- Link `LinuxDesktop2026::ld_root` when the caller needs user/app-owned root
  topology, app-local or portable policy, overrides, named roots, or component
  roots.
- Link `LinuxDesktop2026::ld_settings` when the caller needs settings
  hydration, settings layers, or validated settings writes.
- Link `LinuxDesktop2026::ld_migration` when the caller needs dry-run
  copy/move/import planning.
- Link `LinuxDesktop2026::ld_desktop` for desktop effects such as autostart
  integration.

Evidence harness guardrails:

- Future FlavorTests should continue using generated public path defaults. No
  private path-default helpers should be added to make tests easier than real
  installed users' code.
