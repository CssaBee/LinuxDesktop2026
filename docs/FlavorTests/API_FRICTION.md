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
- Root construction style is now a documented choice instead of an unresolved
  inconsistency: use `linuxdesktop::root::request_builder` for ordinary app
  topology, portable policy, overrides, named roots, and component roots; use
  raw `linuxdesktop::root::options` when product-specific root policy would be
  obscured by a fluent chain; use `linuxdesktop::paths::resolver_options` when
  the seam only needs path families or resource locations.

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
- `linuxdesktop::desktop::desktop_bundle` fits Notepad++ desktop registration
  when the product adapter owns the public "register Notepad++ with the
  desktop" vocabulary and translates it into launcher metadata, icon staging,
  optional autostart, text-file association/default intent, activation
  follow-up, and cleanup reporting.
- `ld_core` provides product-diagnostic translation helpers so adapters can map
  shared severity, codes, messages, related paths, and diagnostic handling flags
  into product-owned diagnostics without hand-copying each report shape. The
  Notepad++ FlavorTest now returns `startup_diagnostic` rather than
  `linuxdesktop::diagnostic`, while preserving the prompt/log handling bits the
  product would need.

Friction:

- Local config needs both requested and active state. The API exposes that, but
  product code must keep the two flags straight or it will blur "marker exists"
  with "portable mode accepted."
- The in-tree FlavorTest uses `request_builder`, which is the recommended style
  for Notepad++'s startup topology because app identity, install resources,
  portable marker policy, command-line/cloud overrides, and plugin/log roots
  read as one root request. Existing cross-port code that still builds raw
  `linuxdesktop::root::options` is valid during the maintained proof, but new
  examples should prefer the builder unless the surrounding product model is
  already clearer as an options object.
- Static named roots can now be declared once with a `named_root_handle`, passed
  to `request_builder`, and used for lookup without repeating the string key.
  Dynamic roots still use the original string lookup surface.
- Desktop registration bundle construction is verbose for a product that thinks
  in one preference or installer action. The verbosity did not require a new
  helper from this single proof because it keeps staged artifacts, activation,
  and cleanup explicit.

Leakage:

- `notepadpp_settings_backend.hpp` exposes Notepad++ result structs, but their
  fields still mirror LinuxDesktop2026 decisions closely: copied defaults,
  validated write backup, and dry-run import actions. That is honest evidence,
  but still asks the product adapter to translate library mechanics into
  application behavior names.
- The in-tree FlavorTest startup state now translates diagnostics into
  Notepad++-owned `startup_diagnostic` values. The remaining result fields that
  mirror backup paths, copied defaults, dry-run imports, registration statuses,
  and activation follow-up are acceptable evidence because those are the
  product behaviors an installer, settings dialog, or startup warning would
  present.
- `notepadpp_desktop_registration.hpp` keeps LinuxDesktop2026 headers out of the
  product-facing surface, but its result still mirrors registration statuses and
  activation follow-up because those are the behaviors a Notepad++ installer or
  first-run setup would have to present.

Boundary notes:

- The CMake dependency list in the cross-port is now `ld_desktop`, `ld_root`,
  `ld_settings`, and `ld_migration`. The cross-port does not link `ld_paths`
  directly because `ld_root` carries that dependency.
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
- `linuxdesktop::desktop::desktop_bundle` fits qBittorrent's launcher,
  optional autostart, `.torrent` MIME declaration, torrent default-app intent,
  `magnet:` handler, icon install, activation follow-up, and uninstall cleanup
  reporting through one product adapter call.

Friction:

- Log placement uses a named root, but the static request/lookup pair no longer
  repeats the string key when declared through `named_root_handle`.
- Desktop registration still requires product translation for what the user
  sees as one "integrate qBittorrent with the desktop" setting. The adapter
  maps staged artifacts, unsupported Windows-shaped mappings, activation
  follow-up, and cleanup statuses into qBittorrent-owned result fields.
- Managed policy is useful validation pressure for desktop registration, but
  the product should not promise active policy state from staged dconf defaults
  and lock files. The adapter keeps the dconf activation diagnostic explicit.

Boundary notes:

- qBittorrent owns the policy branch where command-line profile roots win,
  otherwise an executable-adjacent `profile` directory activates portable mode.
  LinuxDesktop2026 should not hide that precedence unless another product
  repeats the same shape.
- `SpecialFolder` stays product-shaped and does not expose LinuxDesktop2026
  root names.
- Desktop Flavor validation now runs qBittorrent registration across
  GNOME-like, KDE-like, Xfce-like, bare window-manager, and Windows-shaped
  scenarios. The assertions stay on staged artifacts, capability limits,
  activation plans, cleanup reports, and diagnostics rather than live shell,
  file-manager, or single-instance behavior.

## KeePassXC

Fit:

- `linuxdesktop::root::options` fits the roaming/local split and a
  machine-local `local-settings` named root.
- `write_common_config()` fits settings export.
- `plan_rename_file()` fits old cache config migration when translated into
  `LocalConfigMigration`.

Friction:

- KeePassXC has enough XDG and roaming/local vocabulary that the raw options
  object is clearer than the fluent builder. This is the intended exception to
  the builder recommendation: keep dense product root policy explicit when a
  chain would make the adapter harder to audit.
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
- Static named-root handles keep the three-root request/lookup pattern readable
  and leave larger dynamic component maps on the string-key API.

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

- `config_defaults_options` is still LinuxDesktop2026-shaped inside the adapter,
  but `prusaslicer_flavor.hpp` now exposes PrusaSlicer-owned vendor profile
  descriptors and snapshot validation callbacks.
- Vendor profile metadata is product-specific enough that a generic helper
  should not try to hide parsing or merge policy.

Leakage:

- No product-facing PrusaSlicer FlavorTest type currently exposes a
  LinuxDesktop2026 type. The implementation file translates
  `VendorProfileFile` to `settings::config_file` at the adapter boundary.

## OpenRGB

Fit:

- `linuxdesktop::paths::resolve_app_paths()` fits resource/config/profile root
  discovery.
- `write_common_config()` fits JSON settings saves through a small local
  `write_json_file()` adapter.
- `linuxdesktop::desktop` fits autostart application as long as
  `AutostartUpdate` remains the public result.
- OpenRGB remains the small desktop-effect counterexample: it needs only
  autostart, so individual `ld_desktop` calls are cheaper than constructing a
  full bundle.

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

## Amiberry

Fit:

- `linuxdesktop::root::request_builder` fits Amiberry's ordinary bootstrap
  config root and named data/state roots while leaving emulator media policy in
  the adapter.
- Portable root handling fits the executable-adjacent `amiberry.portable`
  marker and keeps the distinction between requested and active portable mode.

Friction:

- Amiberry's `base_content_path` is a bulk product root that intentionally
  overrides many derived leaves. LinuxDesktop2026 can express the surrounding
  default topology, but the base-content fan-out remains product code.
- Plugin lookup has install-library, user-home, and executable fallbacks that do
  not fit a single named root cleanly without hiding product order.

Implementation note:

- Amiberry's larger static named-root map now uses `named_root_handle` for
  request and lookup so the adapter names each LinuxDesktop2026 root once while
  keeping `base_content_path` fan-out and plugin fallback order in product code.

Boundary notes:

- Bootstrap files such as `amiberry.conf` remain platform-settings files, while
  `.uae` configuration files and emulator content are product-managed roots.
- Public Amiberry results expose emulator path names. LinuxDesktop2026 root
  reports stay inside the adapter.

## Endless Sky

Fit:

- `linuxdesktop::root` named resource/data roots fit the paired plugin search
  roots: one bundled resource root and one user-owned plugin root for
  downloaded content.
- Named data roots fit saves and preferences without moving game-specific file
  names into LinuxDesktop2026.

Friction:

- Component roots currently scope relative paths under
  `components/<component>/...`, which is mechanically clear but not the natural
  Endless Sky path shape of `plugins/` directly under the user data root.

Boundary notes:

- Endless Sky owns plugin metadata, validation, load order, saves, and game data
  formats. LinuxDesktop2026 only resolves the platform roots that those product
  paths hang from.

## Minifox ComfyUI Launcher

Fit:

- `linuxdesktop::root::request_builder` fits Minifox's portable package model:
  the executable directory is the package root, with named app-local data,
  cache, and runtime roots for `.minifox/` and `.cache/`.
- Named cache/data roots keep profile settings and ZLUDA/Triton/TorchInductor
  cache placement explicit without introducing AI-tool-specific library
  vocabulary.

Friction:

- LinuxDesktop2026 has no external-tool abstraction for Python/ComfyUI
  candidate selection. The FlavorTest currently keeps those candidates in
  Minifox vocabulary and uses LinuxDesktop2026 only to resolve the roots those
  candidates hang from.
- Tool candidate source reporting is now Minifox-owned
  `ToolCandidateSource`, not `ld_paths::candidate_source`; path-source details
  stay private to root/path resolution diagnostics.
- Process launch, stop, monitor, and live console capture are a visible future
  pressure point, but this probe is not enough evidence to add `ld_process`.

Boundary notes:

- CUDA, ROCm, HIP SDK, and ZLUDA eligibility are product/runtime diagnostics,
  not LinuxDesktop2026 capability claims.
- ZLUDA file replacement/restoration remains product-owned reversible runtime
  work. LinuxDesktop2026 should not become a GPU runtime patcher.

## Dependency Pain

Current desired dependency shape:

- Link `LinuxDesktop2026::ld_paths` alone for plain platform paths,
  executable/install/resource locations, path lists, plugin path sets, and
  lightweight bootstrap adapters.
- Link `LinuxDesktop2026::ld_root` when the caller needs user/app-owned root
  topology, portable policy, overrides, named roots, or component
  roots. Prefer `request_builder` for ordinary readable topology, and prefer
  raw `root::options` when a product already has a stronger root-policy object.
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
