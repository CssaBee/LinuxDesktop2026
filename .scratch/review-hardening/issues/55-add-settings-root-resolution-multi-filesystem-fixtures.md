# 55 - Add Settings Root Resolution Multi-Filesystem Fixtures

**What to build:** Expand `ld_settings` tests so settings root resolution and
settings-owned filesystem behavior are exercised across deliberately separated
root topologies instead of assuming one friendly temporary tree.

**Blocked by:** 54 - Reconcile Review Claim And Module Boundary Docs.

**Status:** proposed

- [ ] Add fixtures that place config, data, state, cache, runtime, install,
  resource, portable, settings override, and sync-config override roots in
  separate directory families.
- [ ] Cover both injected platform defaults and disabled process-environment
  reads so tests can model Windows/XDG behavior deterministically.
- [ ] Cover hostile or malformed roots: relative environment values,
  file-as-directory collisions, missing runtime roots, denied root creation
  where portable on the current platform, and pre-existing conflicting files.
- [ ] Cover cross-device or cross-mount behavior only for operations that
  `ld_settings` actually performs; migration-grade copy/move semantics belong
  to `ld_migration`.
- [ ] Assert diagnostics and root reports, not just final paths, so product
  adapters can translate failures without relying on incidental behavior.

## Domain Term

Use the glossary term **Settings root resolution multi-filesystem fixture** for
this ticket. It is a settings/root lifecycle test shape, not a generic
filesystem migration engine test.
