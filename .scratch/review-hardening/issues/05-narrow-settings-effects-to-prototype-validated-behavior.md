# 05 — Prepare Settings Effect Extraction

**What to build:** Settings-adjacent effects should no longer look like stable `ld_settings` features. Registry-equivalent desktop/system behavior, autostart, and policy must be prepared for extraction to `ld_desktop`; migration planning/execution and app-settings Registry migration compatibility must be prepared for extraction to `ld_migration`.

**Blocked by:** None - ADR 0012 settled the module boundary.

**Status:** done

- [x] Registry, autostart, policy, and migration execution docs identify current `ld_settings` APIs as temporary implementation locations.
- [x] `ld_desktop` extraction requirements cover autostart, desktop entries, icons, MIME/file associations, default applications, URL protocol handlers, shell-equivalent behavior, desktop database updates, and managed/enforced policy.
- [x] `ld_migration` extraction requirements cover migration planning/execution, file/directory moves, rollback reporting, app-settings Registry snapshot/import/export compatibility, and later cross-module orchestration.
- [x] Public claims are reduced where current tests do not cover hostile input, rollback, permissions, or real application integration.
- [x] Any retained effect behavior has focused tests and clear diagnostics for unsupported platforms or incomplete backends.

Evidence:

- Public headers and roadmap docs now label the current migration, Registry,
  autostart, and policy APIs as temporary pre-1.0 implementation locations.
- `docs/plan/ld-desktop-extraction.md` and
  `docs/plan/ld-migration-extraction.md` define the required extraction scope,
  API posture, and validation before ship-candidate status.
- Existing retained behavior remains covered by focused `ld_settings_tests` and
  `ld_settings_c_tests` checks for dry-run behavior, permission denial, Linux
  effect paths, Registry import denial, and non-Windows Registry unsupported
  diagnostics.
