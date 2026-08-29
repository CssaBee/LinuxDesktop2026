# 26 — Add Ergonomic Migration Action Helpers

**What to build:** Add ergonomic C++ helpers for common `ld_migration` plans so
normal file/directory migration does not require callers to specify an action
kind or name by hand.

**Blocked by:** 22 — Translate Library Reports At Product Boundaries.

**Status:** done

- [x] Callers can express common copy migration with a path pair, for example
  `copy_path(source, target)` or `plan_copy(source, target)`, without setting
  `migration_action_kind` or `migration_action::name`.
- [x] The API offers explicit helpers for ambiguous or missing-source dry-run
  cases, such as copy-file and copy-directory helpers.
- [x] Source-kind inference is available when the source exists; missing or
  ambiguous sources produce diagnostics instead of silent wrong plans.
- [x] Destructive helpers such as move/delete make dangerous intent explicit and
  set or require the appropriate dangerous-action metadata.
- [x] Helpers do not make overwrite behavior or parent-directory creation
  surprising; those policies remain visible through options or named helpers.
- [x] Helper-created actions treat `name` as optional metadata. Generated labels
  belong in reports and diagnostics only.
- [x] PrusaSlicer, KeePassXC, and FreeCAD FlavorTests prefer the helper layer
  where it removes mechanical migration-action setup.
