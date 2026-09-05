# 74 - Rename Or Split Migration Move Semantics

**What to build:** Align migration action names with the now-narrow filesystem
contract.

**Blocked by:** 60 - Narrow And Harden Migration Filesystem Semantics.

**Status:** implemented

- [x] Decide whether pre-1.0 `move_file` should become an explicitly atomic
  rename action name.
- [x] Decide whether semantic cross-device move belongs as a separate future
  action with copy/verify/remove behavior.
- [x] Update helper names, examples, FlavorTests, and migration docs if the
  action taxonomy changes.
- [x] Add source-break guidance for any pre-1.0 callers using the old names.

## Implementation Notes

- Added `migration_action_kind::rename_file` and `plan_rename_file()` as the
  canonical file rename API.
- Removed `migration_action_kind::move_file` and `plan_move_file()` as an
  intentional pre-1.0 source break. Callers should use `rename_file` and
  `plan_rename_file()`.
- Renamed the file diagnostics to `migration-file-rename-atomic-only` and
  `migration-file-rename-failed`.
- Did not add semantic cross-device file move behavior. If consumer evidence
  later needs copy/verify/remove semantics, it should be a separate action kind
  instead of hidden fallback under atomic rename.

## Review Anchor

The newer review says migration is now honest about atomic rename-only file
moves, but the public `move_file` name still suggests broader semantic moving
than the contract provides.
