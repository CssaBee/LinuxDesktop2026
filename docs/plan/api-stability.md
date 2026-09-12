# API And ABI Stability

LinuxDesktop2026 `0.2.0` is a public prototype release. It is not a
production-stable or release-candidate line.

## Current Promise

Before `1.0`, the public API may still change as source audits and proof integrations teach us more. Even so, changes should be deliberate, documented, and easy for humans and AI agents to detect.

The project promises:

- Public C++ headers live under `include/linuxdesktop/`.
- Public C ABI headers use plain C types and avoid C++ standard library types.
- C++ APIs are source-compatibility interfaces only; no stable C++ binary ABI is promised while public values expose standard-library types.
- Before `1.0`, C++ APIs may break when a source audit, proof integration, or module-boundary correction proves that the current shape is wrong.
- Existing C ABI entry points should remain compatible where practical, easy to bind, and deliberately versioned, but C ABI expansion and binary-stability design are postponed until release-candidate status.
- Breaking source changes or C ABI changes require a minor version bump while `major == 0`.
- Patch releases should preserve source compatibility and C ABI compatibility for documented functions.
- Removed or renamed public functions should be called out in release notes or migration docs.
- Every pre-1.0 source break or existing C ABI break requires release notes or migration guidance that names the affected module and replacement path.
- C ABI callers own no returned memory directly; they release reports through the matching free function.
- Runtime version functions should match the version macros in the installed C header.

## Version Surface

C++ consumers can read:

```cpp
linuxdesktop::severity
linuxdesktop::diagnostic
linuxdesktop::to_string(linuxdesktop::severity::warning)
linuxdesktop::settings::version_major
linuxdesktop::settings::version_minor
linuxdesktop::settings::version_patch
linuxdesktop::paths::version_major
linuxdesktop::paths::version_minor
linuxdesktop::paths::version_patch
```

Module namespaces do not re-export the shared diagnostic C++ names. Consumers
should use `linuxdesktop::severity`, `linuxdesktop::diagnostic`, and
`linuxdesktop::to_string(linuxdesktop::severity)` directly, while module-specific
enum stringification remains in each module namespace.

Most public enum stringification is already declared in headers and defined in
compiled module sources. The current inventory is:

- `ld_settings`: `portable_level`, `config_layer_kind`, and
  `storage_backend`.
- `ld_paths`: `path_family`, `location_role`, `candidate_source`,
  `directory_action`, `plugin_path_kind`, `plugin_asset_path_kind`,
  `plugin_path_category`, and `platform_support`.
- `ld_root`: `portable_root_level`, `purpose_kind`, `ownership_kind`, and
  `component_kind`.
- `ld_watch`: `event_kind`, `path_type`, `recursive_policy`,
  `overflow_policy`, `stream_state`, and `backend_kind`.
- `ld_migration`: `migration_action_kind`, `migration_action_state`,
  `migration_rollback_state`, plus Registry `hive`, `view`, and `value_type`.
- `ld_desktop`: `effect_kind`, `capability_state`, `activation_step_kind`,
  `registration_scope`, `registration_status`, and `cleanup_status`.

The exception is `ld_core`, which remains a tiny header-only interface target.
Its shared diagnostic helpers, severity/disposition stringification, diagnostic
disposition table, and product-diagnostic translation templates stay
header-defined through `0.2.0`. Moving them behind a compiled target before
release-candidate work would add link and packaging surface without evidence
that compile time, binary size, or ABI hygiene is currently a bottleneck.

Named diagnostic-code constants are header-defined only where callers have
already needed stable source vocabulary: `ld_paths::diagnostic_code` and
`ld_watch::diagnostic_code`. Keep those `inline constexpr std::string_view`
constants for `0.2.0`; they are source-level names, not a binary ABI promise or
an ownership-bearing string API. New modules should not add large diagnostic
constant tables casually. If repeated consumers need more named codes, prefer a
small module-local namespace and keep user-facing wording in diagnostics or
product adapters.

C and Rust FFI consumers can read:

```c
LD_SETTINGS_VERSION_MAJOR
LD_SETTINGS_VERSION_MINOR
LD_SETTINGS_VERSION_PATCH
ld_settings_version_major()
ld_settings_version_minor()
ld_settings_version_patch()
ld_settings_version_string()
LD_PATHS_VERSION_MAJOR
LD_PATHS_VERSION_MINOR
LD_PATHS_VERSION_PATCH
ld_paths_version_major()
ld_paths_version_minor()
ld_paths_version_patch()
ld_paths_version_string()
```

The `ld_paths` C ABI currently covers root resolution reports, location reports,
path-list parsing reports, and typed plugin path-set reports. Returned strings
and arrays are owned by the report and must be released with the matching
`ld_paths_free_*_report` function. Resolver candidates, location candidates,
path-list candidates, and plugin path candidates use distinct structs so C
bindings do not have to treat plugin search roots or executable-adjacent
locations as application path families. Do not broaden this surface until
release-candidate API evidence exists.

The C resolver options expose platform defaults as flat borrowed-string fields:
`xdg_config_home_default`, `xdg_data_home_default`, `xdg_state_home_default`,
`xdg_cache_home_default`, `xdg_runtime_dir_default`,
`windows_roaming_appdata_default`, and `windows_local_appdata_default`. These
fields do not transfer ownership to LinuxDesktop2026 and do not create hidden
resolver state. Their precedence is explicit override, injected environment,
process environment or OS APIs, platform defaults, then built-in fallback. The
selected report uses `LD_PATHS_SOURCE_PLATFORM_DEFAULT` when a default-derived
candidate wins.

Runtime root selection belongs to `ld_paths`; settings-owned home/environment
injection controls live in `ld_settings` root options. The path API uses direct
candidate vocabulary for resolver results, path-list parsing, plugin path sets,
and executable/resource/install locations instead of treating every result as
an ordinary application path family.

Autostart and managed/enforced policy belong to `ld_desktop`. Migration
planning/execution and app-settings Registry compatibility belong to
`ld_migration`. `ld_settings` no longer exposes those compatibility helpers.
Existing C ABI entry points remain best-effort-compatible where practical until
release-candidate status.

For `0.2.0`, `ld_root::options::create_directories`,
`ld_settings::root_options::create_directories`,
`ld_root_named_root_request::create`, and the C `create_directories` initializer
defaults changed from create-on-resolution to preview-only. Callers that relied
on resolution to create directories should set `create_directories = true` and,
for named/component roots, set the individual request `create` flag to true.
`ld_paths` keeps resolution non-mutating and exposes directory creation through
`ensure_directory()`, whose default report remains a dry-run `would_create`.

For `0.2.0`, `ld_root` keeps string-key named-root lookup for dynamic maps and
adds `named_root_handle` as a C++ convenience for static named roots. A handle
owns the caller-provided request label, can be passed to `request_builder`, and
can be reused to find the resolved root from a report without repeating the
string key. The helper does not add product-specific root kinds.

The recommended `0.2.0` C++ construction style for ordinary `ld_root` topology
is `request_builder`: app identity, install/resource roots, portable markers,
root overrides, named roots, and component roots should read as one request
chain when that improves scanning. Raw `ld_root::options` remains source-stable
prototype surface and is still the clearer integration point when a product
already has a dense root-policy object. Plain `ld_paths::resolver_options`
remains the lighter API for path-family and resource-location lookup that does
not need root topology.

`ld_watch` intentionally has no C ABI yet. Its C ABI design is postponed until release-candidate status so callback, queue, ownership, settled-file, and `watch_path` semantics can settle in C++ first.

## Pre-1.0 Rules

Allowed in `0.x` minor releases:

- rename provisional types or functions,
- remove or move C++ APIs that belong to a different module boundary,
- add fields to C++ structs,
- change diagnostics when behavior becomes more accurate,
- and tighten validation around unsafe or ambiguous inputs.

Avoid unless strongly justified:

- changing C ABI struct field order,
- changing ownership rules,
- removing C ABI functions,
- changing default root-resolution precedence,
- changing filesystem mutation defaults,
- or silently weakening write-safety guarantees.

If one of those happens before `1.0`, document it as a breaking change.

At release-candidate status, revisit whether the C ABI should use opaque
handles or versioned/size-tagged structs for long-lived objects and large
reports. Plain C structs are easier to bind than C++ values, but they can
still freeze layout too early. Until then, existing C ABI maintenance is a
best-effort compatibility practice, not a promise to keep expanding binary
interfaces during prototype work.

For `0.2.0`, the solid C ABI line is the already-covered subset, not a broader
ABI surface. Desktop registration C callers can use the existing autostart and
policy calls with matching reset-after-free reports; the desktop bundle,
activation plans, cleanup reports, and staged entry/icon/MIME/default-app/
protocol registration stay C++-only experimental APIs until release-candidate
evidence supports an opaque-handle or builder-style C design. Watching still has
no C ABI. The release support matrix in `docs/project-status.md` is the current
source of truth for which modules are supported prototypes, experimental
extractions, best-effort, or excluded.

Diagnostic stringification cleanup is deferred to release-candidate hardening.
That pass may move `ld_core` non-template helpers or diagnostic disposition
tables out of headers, generate diagnostic-code tables from a single source, or
add stronger exhaustiveness checks. It should preserve the existing
module-level enum `to_string` declarations and provide source migration notes if
any header-defined helper moves.

## Deferred

- Symbol visibility policy for shared-library builds.
- Stable ABI negotiation beyond version functions.
- Rust crate semver policy.
- Per-module versioning if the monorepo grows multiple independent release tracks.
- Release-candidate diagnostic stringification/table cleanup if measured
  compile-time, binary-size, or API hygiene costs justify it.
