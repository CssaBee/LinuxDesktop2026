# API And ABI Stability

LinuxDesktop2026 is currently `0.1.0`.

## Current Promise

Before `1.0`, the public API may still change as source audits and proof integrations teach us more. Even so, changes should be deliberate, documented, and easy for humans and AI agents to detect.

The project promises:

- Public C++ headers live under `include/linuxdesktop/`.
- Public C ABI headers use plain C types and avoid C++ standard library types.
- Breaking source or ABI changes require a minor version bump while `major == 0`.
- Patch releases should preserve source compatibility and C ABI compatibility for documented functions.
- Removed or renamed public functions should be called out in release notes or migration docs.
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

`linuxdesktop::settings::severity`, `linuxdesktop::settings::diagnostic`, `linuxdesktop::settings::to_string`, `linuxdesktop::paths::severity`, `linuxdesktop::paths::diagnostic`, and `linuxdesktop::paths::to_string` remain source-compatible aliases for the shared C++ diagnostic vocabulary.

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

The `ld_paths` C ABI currently covers root resolution reports, candidate reports, path-list parsing reports, and typed plugin path-set reports. Returned strings and arrays are owned by the report and must be released with the matching `ld_paths_free_*_report` function.

`ld_watch` intentionally has no C ABI yet. Its C ABI design is postponed until the release-preview pass so callback, queue, ownership, settled-file, and `watch_path` semantics can settle in C++ first.

## Pre-1.0 Rules

Allowed in `0.x` minor releases:

- rename provisional types or functions,
- add fields to C++ structs,
- add C ABI functions,
- change diagnostics when behavior becomes more accurate,
- and tighten validation around unsafe or ambiguous inputs.

Avoid unless strongly justified:

- changing C ABI struct field order,
- changing ownership rules,
- removing C ABI functions,
- changing default root-resolution precedence,
- or silently weakening write-safety guarantees.

If one of those happens before `1.0`, document it as a breaking change.

## Deferred

- Symbol visibility policy for shared-library builds.
- Stable ABI negotiation beyond version functions.
- Rust crate semver policy.
- Per-module versioning if the monorepo grows multiple independent release tracks.
