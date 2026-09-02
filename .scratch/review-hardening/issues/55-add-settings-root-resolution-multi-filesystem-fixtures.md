# 55 - Add Settings Root Resolution Multi-Filesystem Fixtures

**What to build:** Expand `ld_settings` tests so settings root resolution and
settings-owned filesystem behavior are exercised across deliberately separated
root topologies instead of assuming one friendly temporary tree.

**Blocked by:** 54 - Reconcile Review Claim And Module Boundary Docs.

**Status:** implemented

- [x] Add fixtures that place config, data, state, cache, runtime, install,
  resource, portable, settings override, and sync-config override roots in
  separate directory families.
- [x] Cover both injected platform defaults and disabled process-environment
  reads so tests can model Windows/XDG behavior deterministically.
- [x] Cover hostile or malformed roots: relative environment values,
  file-as-directory collisions, missing runtime roots, denied root creation
  where portable on the current platform, and pre-existing conflicting files.
- [x] Cover cross-device or cross-mount behavior only for operations that
  `ld_settings` actually performs; migration-grade copy/move semantics belong
  to `ld_migration`.
- [x] Assert diagnostics and root reports, not just final paths, so product
  adapters can translate failures without relying on incidental behavior.

## Domain Term

Use the glossary term **Settings root resolution multi-filesystem fixture** for
this ticket. It is a settings/root lifecycle test shape, not a generic
filesystem migration engine test.

## Implementation Notes

- `tests/settings_tests.cpp` has a `settings_root_resolution_multi_filesystem_fixture`
  that separates platform config, data, state, cache, runtime, install,
  resource, portable profile, settings override, and sync-config override
  directory families.
- Settings root resolution is covered with injected platform defaults and
  `use_process_environment = false`, including a guard that empty injected
  environment values do not force process environment reads.
- Hostile-root coverage includes relative injected environment values,
  file-as-directory sync-config override collisions, missing runtime defaults,
  and portable root policy denial under a privileged install family.
- The coverage stays on settings/root lifecycle behavior: it asserts selected
  root reports, creation outcomes, and diagnostics, and does not introduce
  migration-style cross-device copy/move semantics.

## Validation

- `cmake --build build-task54 --parallel 2`
- `ctest --test-dir build-task54 -R '^ld_settings(_c)?_tests$' --output-on-failure`
