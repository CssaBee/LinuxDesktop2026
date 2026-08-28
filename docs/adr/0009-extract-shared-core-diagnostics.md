# Extract shared core diagnostics before file watching

Before starting the file watcher module, the project will extract the existing diagnostic vocabulary into a tiny shared core surface.

## Decision

Create a minimal `ld_core` C++ surface for:

- `linuxdesktop::severity`,
- `linuxdesktop::diagnostic`,
- and `linuxdesktop::to_string(severity)`.

`ld_settings` should keep source-compatible aliases such as `linuxdesktop::settings::severity` and `linuxdesktop::settings::diagnostic`, but those names should point at the shared vocabulary. The C ABI can keep `LD_SETTINGS_SEVERITY_*` and settings-local struct names for now.

The first implementation should expose `LinuxDesktop2026::ld_core` as a header-only/interface CMake target so later modules, especially `ld_watch`, can depend on shared diagnostics without depending on `ld_settings`.

## Rationale

`ld_watch` will need diagnostics for backend capability reporting, overflow/lost-event recovery, recursive-watch limits, unsupported filesystem behavior, and degraded watch state. Reusing `settings::diagnostic` would make the watcher depend semantically on the settings module. Duplicating the type would make cross-module reports harder to consume.

The shared core should stay intentionally small. Path abstractions, result/report base types, and versioning helpers need more evidence before becoming shared code.

## Compatibility

Keep the current `ld_settings` C++ names as aliases. Do not force existing C++ consumers to rename diagnostic types during this pre-1.0 cleanup.

The C ABI should not be renamed yet. C ownership and naming rules are more expensive to change, and there is not a second C module to prove the shared shape.

## Verification

Because this touches public headers and package consumption, implementation should be checked with:

- existing C++ tests,
- existing C ABI tests,
- and the install-tree consumer smoke test.

## Deferred

- Do not extract a shared path type in this step.
- Do not refactor `ld_settings` path returns unless the watcher design proves a shared path value is needed.
- Do not create a broad `ld_paths` module yet.
- Do not change C ABI names until a second C-facing module needs shared ABI vocabulary.

## Related Docs

- `docs/plan/library-roadmap.md`
- `docs/survey/file-watcher-application-audit.md`
- `docs/survey/file-watcher-library-audit.md`
- `docs/adr/0010-design-file-watcher-module.md`
