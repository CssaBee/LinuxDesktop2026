# Root Module Boundary Spec

## Problem Statement

`src/settings.cpp` is now large enough to hide multiple responsibilities behind
one implementation file. The current file contains settings root resolution,
portable and sync overlays, named and component roots, config-layer reporting,
default hydration, common write and backup mechanics, diagnostics glue, and
assorted filesystem helpers. Some of that belongs to `ld_settings`, some has
already moved conceptually to `ld_paths`, and some may deserve a public
`ld_root` module if repeated consumer evidence says applications need root
topology without the rest of the settings lifecycle.

This is not a tech-debt cleanup ticket. A split that only makes files smaller
would leave the public model as muddy as before. The hardening question is
whether LinuxDesktop2026 needs a root module that can express user-owned and
app-owned root layouts without forcing callers through either generic path
families or settings/config vocabulary.

`docs/FlavorTests/API_FRICTION.md` is the baseline. Notepad++, qBittorrent, and
KiCad show that a root-topology helper can reduce mechanical setup.
KeePassXC and FreeCAD show that product-owned XDG/environment policy may read
better as direct requests. Walnut and OpenIPC Dashboard are negative evidence:
some products only need `ld_paths`, while service or web profile roots should
remain product-owned unless a repeated root-topology seam appears.

## Solution

`ld_root` is now the public boundary for reusable root topology. The design
defines the difference between:

- generic path families owned by `ld_paths`,
- application root topology potentially owned by `ld_root`,
- settings-specific root overlays and config lifecycle owned by `ld_settings`,
- product policy that must stay in adapters or applications.

The implementation moved generic named/component topology into
`linuxdesktop::root` and contracted `linuxdesktop::settings` to settings
lifecycle behavior. This was a deliberate pre-1.0 break: settings no longer
keeps duplicated generic root-builder structs or aliases.

## Boundary Design

`ld_paths` answers "where does this platform normally put a path family for
this app?" It owns XDG/Known Folder/environment/default precedence, generated
platform defaults, executable-relative resource discovery, path-list parsing,
plugin search path sets, and directory creation reports. It should remain the
right API for products that only need config/data/state/cache/runtime/resource
families and then apply their own layout.

The `ld_root` boundary answers "how should an application organize the
roots it owns after the base platform families are known?" It may own a public
request/report vocabulary for base application roots, named roots, component
roots, app-local roots, install-adjacent resource roots, user-owned root
selection, app-owned child root derivation, root source reporting, and root
creation diagnostics. It should call or share internals with `ld_paths`; it
should not duplicate platform path discovery.

`ld_settings` answers "how does a settings/config lifecycle use roots?" It owns
settings overrides, sync-config overrides, portable settings decisions, config
layers, default hydration, backup/atomic/durable writes, and settings-specific
diagnostics. It now consumes an `ld_root` result, but it should continue
to be where callers look for config defaults and safe settings writes.

Product-owned policy answers "what layout does this product promise to users,
plugins, projects, services, or existing upstream code?" Product adapters keep
command-line precedence when it is product-specific, cloud prompt policy,
service profile contracts, project-keyed backup paths, UI diagnostic wording,
file formats, plugin ABI expectations, environment variables unique to the
product, and Qt/renderer/browser lifecycle.

## Root Ownership Terms

A user-owned root is selected by the person or deployment environment using
ordinary app-user vocabulary: home directory, command-line settings directory,
cloud/sync directory, XDG/AppData defaults, local portable profile marker, or a
product-specific override. The product must be able to explain this root to a
user as "your profile", "your settings directory", "your cloud settings", or
"your local profile" without exposing LinuxDesktop2026 internals.

An app-owned root is derived by the application from a selected base root. It is
where the product decides child layout: `plugins/Config`, `BT_backup`, `logs`,
`colors`, `toolbars`, `project-backups`, QSettings files, module caches, or
service databases. `ld_root` helps when these children are ordinary
named/component roots with ownership classes. It must not take over children
whose names, validation, security model, or migration behavior are part of the
product contract.

## Flavor Evidence

| Flavor | Evidence for `ld_root` | Boundary decision |
| --- | --- | --- |
| Notepad++ | Strong positive evidence. Both the in-tree FlavorTest and maintained proof branch compose install resources, user config, session roots, plugin config, command-line settings, cloud choice, local-config marker, and privileged-install denial. | Shared topology is useful, but config default hydration, XML validation, backup restore, plugin ABI, cloud prompt wording, and diagnostic translation remain `ld_settings` or product-owned. |
| qBittorrent | Positive evidence. `Profile::init()` uses `request_builder` for app identity, executable resources, environment/default roots, portable profile activation, and a machine-local log named root. | `ld_root` can own the reusable request/report mechanics. qBittorrent keeps profile-dir override precedence, executable-adjacent `profile` activation policy, `SpecialFolder` naming, and fastresume layout. |
| KiCad | Positive evidence with limits. Named roots for colors, toolbars, and project backups fit a root topology API better than generic path families. | Component/named roots belong in `ld_root`. Project-keyed backup fallback, `GetPathForSettingsFile()`, and project ownership remain KiCad policy. |
| Walnut | Negative evidence. Walnut only needs executable-adjacent resources and ordinary config roots before renderer/bootstrap decisions. Direct `ld_paths::resolve_app_paths()` is clearer than root topology. | Do not require `ld_root` for simple graphics/bootstrap apps. Keep platform defaults and resource/config family selection in `ld_paths`. |
| OpenIPC Dashboard | Negative evidence with a narrow future watch point. The desktop profile uses ordinary `ld_paths`, while `--data-root`/`OPENIPC_DATA_ROOT` selects an isolated service profile with many app-owned children. | Do not generalize service roots yet. Reopen only if more FlavorTests need "one absolute root selects a named service profile with app-owned child layout" and can share vocabulary without hiding security/deployment policy. |

KeePassXC and FreeCAD are mixed evidence, not promotion blockers. Their
roaming/local and app-specific environment precedence rules still read more
honestly as direct requests until more evidence justifies additional
`ld_root` vocabulary.

## Do Not Move Into `ld_root`

- Generic platform path-family discovery, platform defaults, path lists, plugin
  search path sets, or directory-only helpers from `ld_paths`.
- Config-layer modeling, storage backends, managed/enforced settings layers,
  default hydration, settings writes, validation callbacks, or backup recovery
  from `ld_settings`.
- Product-specific command-line semantics, cloud/sync prompts, service profile
  security, project-keyed roots, renderer/Qt/browser lifecycle, file formats,
  migration prompts, or user-facing diagnostic vocabulary.
- Settings lifecycle behavior. The public root API exists, but it must not grow
  settings hydration, writes, or storage-layer vocabulary.

## `ld_paths` Overlap Audit

Task 44 audited the current `ld_paths` public and implementation surface against
the `ld_root` boundary.

| Area | Classification | Decision |
| --- | --- | --- |
| Config, data, state, cache, runtime, temp, and XDG/Known Folder user directories | Generic path-family behavior | Stay in `ld_paths`. These answer "where does this platform normally place this family?" and are the base input `ld_root` consumes. |
| Explicit overrides, environment/default precedence, generated platform defaults, and source-labeled resolver candidates | Generic path-family behavior | Stay in `ld_paths`. The platform-default work exists specifically so users can pass deterministic OS roots without private FlavorTest helpers. |
| Legacy and site config candidates | Generic path-family behavior with config-specific vocabulary | Stay in `ld_paths` for now. NUT/OpenRGB-style search chains justify reporting legacy/site candidates, but no FlavorTest shows this becoming reusable root topology. |
| Directory creation helpers | Generic filesystem mechanics | Stay public in `ld_paths`; `ld_root` reports creation per named/app-owned root through the shared directory helper behavior. |
| Path-list parsing and joining | Generic path-list behavior | Stay in `ld_paths`. This supports environment-shaped search lists and should not move to root topology. |
| Typed plugin path sets and Wine-prefix-aware plugin defaults | Domain-specific path-set behavior | Stay in `ld_paths`. Carla-style evidence supports search-root discovery, not plugin loading, plugin ABI, or app-owned root topology. |
| Custom plugin path sets | Path-set behavior, not root topology | Stay in `ld_paths`. A caller-defined plugin ecosystem is still a search list; it should not be confused with named app roots such as Notepad++ plugin config. |
| Executable path, executable directory, install prefix, and resources | Path-location behavior with root interaction point | Stay in `ld_paths` as explicit location roles, separate from ordinary path families. Walnut and PrusaSlicer need executable-adjacent resources directly; Notepad++ and qBittorrent feed install-adjacent inputs into `ld_root`. |
| Path-list and plugin path-set candidates | Path-list/path-set behavior | Stay in `ld_paths` with direct candidate vocabulary. A plugin search set is not a selected application path family, and C/Rust bindings no longer inherit a fake family just because reports need diagnostics. |
| Settings named roots, component roots, portable/local overlays, config layers, hydration, and writes | Root topology or settings lifecycle | Do not move into `ld_paths`. Positive `ld_root` evidence comes from Notepad++, qBittorrent, and KiCad; settings lifecycle remains in `ld_settings`. |
| Service/data-root profiles and project-keyed roots | Product-owned policy | Do not move into `ld_paths` or `ld_root` yet. OpenIPC Dashboard and KiCad project backups remain negative or limited evidence. |

The audit does not justify merging broad `ld_paths` behavior into `ld_root`.
Tasks 45 and 46 completed the pre-public cleanup of the `ld_paths` result
shape: ordinary path families, executable/install/resource locations, path-list
candidates, and plugin path-set candidates are distinct public concepts before
a public `ld_root` module is added.

## User Stories

1. As an application developer, I want user-owned and app-owned roots to have a
   clear vocabulary, so that my product layout does not have to masquerade as
   settings configuration.
2. As an application developer, I want ordinary platform path discovery to stay
   in `ld_paths`, so that simple apps like Walnut do not need a heavier root
   topology API.
3. As an application developer, I want settings config lifecycle to stay in
   `ld_settings`, so that default hydration and safe writes remain discoverable
   from the settings module.
4. As an application developer, I want service/profile roots to remain product
   policy unless multiple products show the same reusable topology, so that
   OpenIPC-style deployment constraints are not flattened into a generic helper.
5. As a Notepad++ integrator, I want install roots, per-user config roots,
   session roots, plugin config roots, local config markers, command-line
   overrides, and cloud choices to remain easy to compose without exposing raw
   platform path details.
6. As a maintainer, I want the `settings.cpp` split to make ownership visible,
   so that future changes do not accidentally grow `ld_settings` again.
7. As a maintainer, I want a public `ld_root` module only if it has a distinct
   job from `ld_paths` and `ld_settings`, so that LinuxDesktop2026 does not add
   a module-shaped synonym.
8. As a maintainer, I want the `ld_paths` audit to distinguish merge candidates
   from unjustified expansion, so that generic path APIs stay small and honest.
9. As a FlavorTest reviewer, I want API friction notes to say when
   `request_builder` is positive evidence and when direct `ld_paths`
   remains the better fit.
10. As a future C or Rust binding author, I want root topology concepts to be
    flat and explicit if they become public, so that they do not depend on C++
    builder-only ergonomics.

## Implementation Decisions

- Treat `ld_root` as a public module boundary, not a private cleanup name.
- Add `linuxdesktop::root` and `ld_root_c.h` as the public generic root
  topology APIs.
- Contract `ld_settings` with a deliberate pre-1.0 break instead of preserving
  duplicate generic root topology APIs.
- Keep generic config/data/state/cache/runtime/resource placement and platform
  defaults in `ld_paths`.
- Keep settings override, sync-config override, portable marker handling,
  config layers, hydration, and settings writes in `ld_settings`.
- Use `ld_root` only for reusable application root topology: named roots,
  component roots, user-owned roots, app-owned roots, install-adjacent roots,
  and root ownership reporting.
- Do not move service deployment policy, web profile layout, cloud prompt
  policy, file format ownership, or plugin ABI behavior into `ld_root`.
- Audit `ld_paths` separately for internal organization and unjustified overlap,
  but do not merge plugin path sets, path-list parsing, or directory creation
  into `ld_root` without repeated evidence.
- Keep plugin path-set reports separate from selected application root-family
  candidates.
- Keep executable/resource/install location discovery in `ld_paths` through
  explicit location roles before asking `ld_root` to consume those values.
- Use `API_FRICTION.md`, in-tree FlavorTests, and the Notepad++ cross-port proof
  as the evidence baseline.

## Testing Decisions

- Start with characterization tests around public `ld_settings` root behavior
  before moving implementation files.
- Preserve Notepad++, qBittorrent, KiCad, Walnut, and OpenIPC Dashboard
  FlavorTest behavior while changing internals.
- Keep public `ld_root` API tests beside the module and migrate positive
  FlavorTests to the new surface.
- Keep `ld_paths` tests focused on path-family resolution, candidate sources,
  platform defaults, path lists, plugin path sets, and directory creation.
- Do not treat a smaller `settings.cpp` line count as test evidence.

## Out of Scope

- Moving settings hydration or safe writes into `ld_root`.
- Moving plugin search roots wholesale out of `ld_paths`.
- Replacing product-owned service/profile deployment rules.
- Additional root C ABI beyond the topology surface implemented in
  `ld_root_c.h`.
- Broad `ld_paths` public redesign without new FlavorTest or maintained
  consumer evidence.

The task 44 audit removed preserve-before-break caution for the root/path
hardening tickets. Tasks 45 and 46 took the direct breaking changes needed to
make those concepts honest while the module is still pre-public-prototype.
Tasks 47 and 48 should now finish the public `ld_root` and contracted
`ld_settings` boundary without leaving deprecated aliases,
duplicated public structs, or temporary adapter layers behind.

The intended final dependency shape is:

- `ld_paths` for platform families, executable/resource locations, path lists,
  directory helpers, and plugin search roots.
- `ld_root` for reusable user-owned and app-owned root topology, depending on
  `ld_paths` without depending on `ld_settings`.
- `ld_settings` for config layers, hydration, settings writes, backups,
  portable/settings overlays, and settings diagnostics, with no generic
  root-topology API exposed as settings vocabulary.

## Further Notes

This spec follows the API friction baseline after the platform-defaults pass.
The accepted direction is public-boundary quality with no tech-debt module:
design the root boundary, split implementation seams only where ownership
becomes clearer, and let public API promotion wait for evidence.
