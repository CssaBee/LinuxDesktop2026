# FlavorTest API Friction

This document names the API problems visible in the current FlavorTests and
Notepad++ proof work. It is not a changelog. Read it as a map of the
remaining rough edges: what users will feel today, what is acceptable for
`0.2.0`, and what should become future API work only if more integrations repeat
the same pain.

## 0.2.0 Position

`0.2.0` is solid enough to publish as a public prototype because the module
boundaries are now understandable:

- use `ld_paths` for plain path, resource, install, executable, path-list, and
  plugin-root discovery;
- use `ld_root` for app/user root topology, portable roots, named roots,
  component roots, and root overrides;
- use `ld_settings` for shipped default hydration, validated config writes,
  backups, settings layers, and versioned whole-file commits;
- use `ld_desktop` for current autostart and policy effects, while broader
  desktop bundles remain experimental;
- use `ld_migration` for dry-run-first settings migration with explicit regular
  file and directory limits.

The biggest `0.2.0` rule is product-boundary containment. Product-facing headers
should expose product-owned types. LinuxDesktop2026 reports, options, enums, and
rich diagnostics belong inside adapter implementation files unless the product
deliberately presents LinuxDesktop2026 as its public platform layer.

## Current Problems

### Product Result Translation Is Still Work

The library returns rich reports because tests, diagnostics, and migration
previews need detail. Real product seams usually want smaller results: startup
warnings, save status, migration prompts, installer registration status, or
device-profile messages.

Affected flavors:

- Notepad++ still has result fields that closely mirror copied defaults,
  backup paths, dry-run imports, registration statuses, and activation
  follow-up.
- qBittorrent and desktop-registration tests still need product adapters to
  translate staged artifacts, unsupported effects, activation steps, and cleanup
  outcomes.
- KeePassXC, FreeCAD, PrusaSlicer, OpenRGB, OBS, Walnut, OpenIPC Dashboard,
  CtrlrX, SmartServoFramework, KickCAT, Amiberry, Endless Sky, and Minifox now
  keep LinuxDesktop2026 reports private, but each still pays translation cost at
  the adapter edge.

`0.2.0` decision: accept this. Translation is the price of not leaking framework
types into products.

Future ticket trigger: add narrower report-to-product helper APIs only when two
or more maintained integrations repeat the same translation code shape.

### Module Choice Is Visible At Cross-Module Seams

Callers still have to know which module owns each operation. A normal startup
adapter can legitimately touch `ld_root`, `ld_settings`, `ld_migration`, and
`ld_desktop` in one flow. That is honest, but it is not invisible.

Affected flavors:

- Notepad++ is the clearest example: root resolution, default hydration,
  validated saves, migration preview, and desktop registration are separate
  module calls.
- qBittorrent combines root topology, settings writes, and desktop effects.
- OpenRGB uses `ld_paths`, `ld_settings`, and a narrow `ld_desktop` autostart
  call rather than a single "app setup" facade.

`0.2.0` decision: accept this. Transitive linkage or a broad convenience facade
would blur ownership too early.

Future ticket trigger: consider an orchestration helper only after maintained
consumer code shows repeated boilerplate that does not hide mutation,
capability, or ownership distinctions.

### Root Construction Has Two Good Shapes

There is no single best root API. `request_builder` is readable for ordinary
startup topology, but raw `root::options` is better when the product already has
a dense root-policy object.

Affected flavors:

- Good `request_builder` fits: Notepad++, qBittorrent, KiCad, Amiberry, Endless
  Sky, and Minifox.
- Good raw-options fits: KeePassXC, where roaming/local policy is already a
  dense product model.
- Good direct `ld_paths` fits: FreeCAD, Walnut, OpenRGB, OpenIPC Dashboard,
  CtrlrX, SmartServoFramework, and KickCAT when they only need path families or
  resource locations.

`0.2.0` decision: document the selection rule and keep all three entry points.

Future ticket trigger: only add another builder/helper if a repeated product
shape cannot be expressed cleanly by these choices.

### Named Roots Are Better But Not Invisible

`named_root_handle` removes repeated string keys for static roots, but adapters
still need LinuxDesktop2026 purpose and ownership vocabulary when declaring
custom roots.

Affected flavors:

- qBittorrent log roots, KiCad color/toolbar/project-backup roots, Amiberry
  content roots, Endless Sky plugin roots, and Minifox portable package roots
  benefit from handles.
- KeePassXC still has to map the local-settings root back to product wording.
- Dynamic maps still use string lookup because compile-time handles do not fit
  product-generated root lists.

`0.2.0` decision: accept this. The handle fixes accidental duplication without
inventing product-specific root kinds.

Future ticket trigger: add typed helper constructors only for repeated generic
root roles, not for one product's vocabulary.

### Desktop Registration Is Verbose

Desktop effects expose artifact staging, capability limits, activation steps,
and cleanup reporting. That detail is useful for tests and installers, but it
is verbose for a product preference that users experience as one checkbox or
setup action.

Affected flavors:

- Notepad++ and qBittorrent both need product adapters around launcher metadata,
  icons, MIME/default-app intent, URL protocol handlers, activation follow-up,
  and cleanup.
- OpenRGB is the counterexample: a direct autostart call is lighter than a full
  desktop bundle.

`0.2.0` decision: accept the verbosity and keep broad desktop bundles
experimental. The narrow C ABI remains autostart and policy only.

Future ticket trigger: after more desktop-registration integrations, consider
one higher-level C++ builder that reduces boilerplate without hiding staged
versus activated state.

### Migration Must Stay Preview-Shaped

Migration reports are intentionally explicit about dry-run state, dangerous
actions, unsupported metadata semantics, and execution results. That makes
product prompts straightforward but keeps raw migration details too rich for
most public product seams.

Affected flavors:

- Notepad++ and KeePassXC import/move decisions should return product prompts,
  not raw migration plans.
- FreeCAD and PrusaSlicer directory migrations keep raw plans inside adapters.
- Directory moves are limited to verified regular-file/subdirectory trees.
  Symlinks, special files, ownership, permissions, timestamps, xattrs, ACLs,
  sparse extents, and hard-link topology are not preserved as metadata.

`0.2.0` decision: accept the explicit plan/report model and exclude semantic
cross-device file moves. `rename_file` remains atomic-only.

Future ticket trigger: add product-shaped migration convenience helpers only
after repeated maintained consumers need the same prompt/execute/result flow.

### JSON Saves Are Still Product Adapters

LinuxDesktop2026 can validate and write config files, but it does not provide a
JSON-oriented settings save facade.

Affected flavors:

- OpenRGB wraps `write_common_config()` to validate JSON.
- KiCad writes JSON settings but still owns schema, merge, and validation.
- Registry snapshot JSON parsing is library-owned only for the narrow
  `linuxdesktop.settings.registry.snapshot.v1` migration schema.

`0.2.0` decision: accept this. The project should not expose a general JSON
settings API merely because it uses `nlohmann/json` internally for migration
snapshots.

Future ticket trigger: consider format-specific helpers only when products
repeat the same save contract beyond "validate a file after writing it."

### Product Environment Policy Remains Product-Owned

Some applications have environment variables, command-line overrides, or
service roots whose precedence is part of the product's behavior. The library
can provide roots and diagnostics, but it should not take over that policy from
one integration.

Affected flavors:

- FreeCAD owns `FREECAD_USER_HOME`, `FREECAD_USER_DATA`,
  `FREECAD_USER_TEMP`, `--user-cfg`, and `--system-cfg` precedence.
- OpenIPC Dashboard owns the service `data-root` profile and its many named
  child paths.
- qBittorrent owns the precedence between `--profile`, named configurations,
  and executable-adjacent portable profile roots.
- Amiberry owns `base_content_path` fan-out and plugin fallback order.

`0.2.0` decision: accept product-owned precedence.

Future ticket trigger: add a helper only if at least two products need the same
"one override root expands into a named child layout" contract.

### Non-Desktop Domains Are Adjacent, Not Owned

Several FlavorTests expose real platform friction that should not become
LinuxDesktop2026 API in `0.2.0`.

Affected flavors:

- SmartServoFramework needs serial access, driver installation, permissions,
  and device-profile naming.
- KickCAT needs EtherCAT interface selection, real-time behavior, simulator
  sockets, and ESI XML domain validation.
- Minifox needs Python/ComfyUI discovery, process launch, console capture,
  GPU-runtime detection, and ZLUDA replacement/restoration.
- CtrlrX needs JUCE lifecycle and plugin export policy.
- Endless Sky needs plugin metadata, validation, load order, saves, and game
  data formats.
- Walnut needs renderer startup, GPU selection, entry-point behavior, and image
  semantics.

`0.2.0` decision: keep these outside the project. `ld_process`, `ld_ipc`,
`ld_dynlib`, service lifecycle, hardware helpers, and runtime patching remain
research-only.

Future ticket trigger: open module-design work only when repeated integrations
show the same narrow seam and an existing-tool decision explains why mature
toolkit or OS APIs are not enough.

## Per-Product Review

### Notepad++

Current pain:

- Startup still spans root resolution, default hydration, validated writes,
  migration preview, and desktop registration. That is the right module split,
  but the product adapter has to orchestrate several LinuxDesktop2026 concepts.
- Public result fields still sit close to library mechanics: copied defaults,
  backup paths, dry-run imports, registration status, and activation follow-up.
- Local config needs both "requested" and "active" state; callers must not blur
  marker detection with accepted portable mode.

`0.2.0` disposition: supported proof shape. Keep the adapter product-owned and
do not add a broad Notepad++ convenience facade.

### qBittorrent

Current pain:

- Profile setup combines command-line roots, named configurations,
  executable-adjacent portable mode, settings writes, log roots, and desktop
  registration.
- Desktop registration is product-visible as one preference, but the adapter
  still has to translate staged artifacts, activation steps, unsupported
  effects, cleanup statuses, and dconf policy diagnostics.

`0.2.0` disposition: supported FlavorTest shape. Keep qBittorrent precedence in
product code; reopen desktop-builder ergonomics only if more apps repeat the
same bundle flow.

### KeePassXC

Current pain:

- The roaming/local split and local-settings root are clearer as raw
  `root::options` than as a builder chain, so KeePassXC remains an intentional
  exception to the preferred builder style.
- Portable/root diagnostics still need product wording before user prompts.
- Old local-config migration should stay a product-shaped `LocalConfigMigration`
  decision, not a public raw migration report.

`0.2.0` disposition: acceptable. KeePassXC proves the API must keep raw options
for dense product-owned root policy.

### KiCad

Current pain:

- Static named roots work for colors, toolbars, and project backups, but
  project-keyed fallback paths remain product lookup logic.
- JSON settings writes are covered by validated file writes, not by a JSON
  schema/merge helper.

`0.2.0` disposition: acceptable. Keep project-specific backup lookup and JSON
semantics in KiCad code.

### FreeCAD

Current pain:

- FreeCAD's command-line and environment precedence is too product-specific for
  `ld_paths` to simplify directly.
- Deprecated-path migration fits `ld_migration`, but product-facing state must
  remain `DeprecatedPathMigration`.
- The path resolver is useful evidence, but it does not yet feel like a
  natural FreeCAD abstraction.

`0.2.0` disposition: acceptable boundary case. Do not add environment
precedence helpers from FreeCAD alone.

### PrusaSlicer

Current pain:

- Vendor profile seeding and snapshot saves fit the existing settings APIs, but
  profile metadata, XML/INI semantics, prompt policy, and validation callbacks
  are unavoidably product-owned.
- Old datadir migration should stay behind a product-shaped
  `OldDatadirMigration` decision.

`0.2.0` disposition: acceptable. No product-facing PrusaSlicer type should
expose LinuxDesktop2026 types.

### OpenRGB

Current pain:

- JSON saves require a local adapter around `write_common_config()`.
- Autostart has many explicit inputs for a user-visible on/off setting:
  executable, arguments, working directory, enabled state, dry-run mode, and
  write permission.

`0.2.0` disposition: acceptable. Direct autostart remains preferable to a broad
desktop bundle for this product.

### OBS

Current pain:

- OBS's C-shaped persistence seam uses buffers and integer status values, so
  rich C++ reports must stay private.
- The lower-level write API fits better than the common config facade, which
  means OBS is evidence for preserving both write surfaces.

`0.2.0` disposition: acceptable. This does not prove the C ABI is broad enough
for general adoption.

### Walnut

Current pain:

- Walnut only needs executable-adjacent resources and a config root; forcing
  root topology would add framework tax.
- Renderer startup, GPU selection, entry-point mode, headless launch, and image
  path semantics remain product code.

`0.2.0` disposition: acceptable negative evidence. Use `ld_paths` directly.

### OpenIPC Dashboard

Current pain:

- The desktop profile fits `ld_paths`, but the service `data-root` branch is a
  whole product-owned profile with many child directories and security
  constraints.
- Browser-safe diagnostics, redaction, Qt/QML lifecycle, QSettings mechanics,
  and deployment policy are outside LinuxDesktop2026.

`0.2.0` disposition: acceptable. The only likely future helper is an
override-root-to-child-layout API, and only if another product repeats it.

### Gearcoleco

Current pain:

- Installed versus portable mode fits root topology, but emulator language
  around ROMs, controller databases, debug symbols, and `.uae`-style content
  stays product-owned.
- The adapter still translates LinuxDesktop2026 portable-root vocabulary into
  emulator startup wording.

`0.2.0` disposition: acceptable. This is a good portable-root evidence slice,
not a reason for emulator-specific helpers.

### CtrlrX

Current pain:

- JUCE owns lifecycle, plugin formats, host behavior, and standalone-versus-
  plugin mutation policy.
- LinuxDesktop2026 can resolve roots and plugin path sets, but CtrlrX decides
  whether an export is allowed and which target format is meaningful.

`0.2.0` disposition: acceptable. Keep plugin export policy outside
LinuxDesktop2026.

### SmartServoFramework

Current pain:

- Device-profile persistence fits config roots and writes, but serial access,
  driver installation, OS permissions, and actuator protocol behavior are not
  library responsibilities.
- Device-profile filename sanitization remains local product code.

`0.2.0` disposition: acceptable. Hardware-companion helpers stay out of scope.

### KickCAT

Current pain:

- Optional desktop tooling can use config/cache/runtime roots, but EtherCAT
  interface selection, real-time behavior, simulator lifecycle, embedded
  targets, and ESI XML validation stay product-owned.
- ESI lookup is partly resource-root shaped and partly domain-shaped, so a
  generic helper would likely hide too much.

`0.2.0` disposition: acceptable boundary challenge. Do not promote this into
process, IPC, or hardware API work without repeated evidence.

### Amiberry

Current pain:

- `base_content_path` is a bulk product root that fans out into many derived
  paths; LinuxDesktop2026 can express surrounding topology but should not own
  the fan-out.
- Plugin lookup mixes install-library, user-home, and executable fallbacks in a
  product-specific order.

`0.2.0` disposition: acceptable. Named-root handles help static declarations;
content and plugin policy remain emulator code.

### Endless Sky

Current pain:

- Bundled and user-owned plugin roots fit named roots, but component roots place
  paths under `components/<component>/...`, which is not the natural
  `plugins/` shape for Endless Sky.
- Plugin metadata, validation, load order, saves, preferences, and game data
  formats are product code.

`0.2.0` disposition: acceptable. Keep the current named-root approach and avoid
game-specific plugin abstractions.

### Minifox ComfyUI Launcher

Current pain:

- Portable package roots fit `ld_root`, but Python/ComfyUI discovery, process
  launch, console capture, GPU-runtime detection, and ZLUDA file replacement are
  outside LinuxDesktop2026.
- Tool candidate sources must remain Minifox-owned rather than exposing
  `ld_paths::candidate_source`.

`0.2.0` disposition: acceptable. This is future pressure for `ld_process`, but
not enough evidence to add that module.

## Dependency Guardrails

The desired dependency shape is:

- link `LinuxDesktop2026::ld_paths` alone for plain platform paths,
  executable/install/resource locations, path lists, plugin path sets, and
  lightweight bootstrap adapters;
- link `LinuxDesktop2026::ld_root` when the caller needs user/app-owned root
  topology, portable policy, overrides, named roots, or component roots;
- link `LinuxDesktop2026::ld_settings` when the caller needs settings
  hydration, settings layers, validated writes, or versioned whole-file commits;
- link `LinuxDesktop2026::ld_migration` when the caller needs dry-run
  application-settings copy/move/import planning;
- link `LinuxDesktop2026::ld_desktop` for desktop effects such as autostart,
  policy, and experimental desktop registration bundles.

Install-tree consumers and FlavorTests should stay module-specific enough to
catch accidental public-header or CMake interface coupling. A settings-only
consumer should not include or link desktop, paths, migration, or watch APIs
just to prove package consumption.

## Future Ticket Candidates

These are not `0.2.0` blockers:

- repeated product-result translation helpers for common save, migration,
  desktop-registration, and startup-diagnostic shapes;
- a higher-level desktop registration builder once more products need the same
  artifact/activation/cleanup flow;
- a shared override-root-to-child-layout helper if service/profile roots repeat
  beyond OpenIPC Dashboard;
- format-specific save helpers only if repeated integrations need more than the
  current validate-after-write contract;
- `ld_process`, `ld_ipc`, `ld_dynlib`, or service lifecycle design only after
  repeated source-anchored evidence and an existing-tool decision.
