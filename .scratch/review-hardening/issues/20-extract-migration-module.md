# 20 — Extract Migration Module

**What to build:** Move migration planning and execution out of `ld_settings` into a real `ld_migration` module.

**Blocked by:** None - task 05 prepared the extraction inventory.

**Status:** done

- [x] `ld_migration` owns migration planning, file/directory copy and move execution, rollback reporting, and app-settings Registry snapshot/import/export compatibility.
- [x] `ld_settings` keeps config-bundle hydration and may provide inputs to migration, but does not own the stable migration engine.
- [x] Existing migration APIs are removed, moved, or replaced with documented pre-1.0 migration guidance.
- [x] `docs/plan/ld-migration-extraction.md` is used as the implementation checklist.
