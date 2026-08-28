# 05 — Prepare Settings Effect Extraction

**What to build:** Settings-adjacent effects should no longer look like stable `ld_settings` features. Registry-equivalent desktop/system behavior, autostart, and policy must be prepared for extraction to `ld_desktop`; migration planning/execution and app-settings Registry migration compatibility must be prepared for extraction to `ld_migration`.

**Blocked by:** None - ADR 0012 settled the module boundary.

**Status:** ready-for-agent

- [ ] Registry, autostart, policy, and migration execution docs identify current `ld_settings` APIs as temporary implementation locations.
- [ ] `ld_desktop` extraction requirements cover autostart, desktop entries, icons, MIME/file associations, default applications, URL protocol handlers, shell-equivalent behavior, desktop database updates, and managed/enforced policy.
- [ ] `ld_migration` extraction requirements cover migration planning/execution, file/directory moves, rollback reporting, app-settings Registry snapshot/import/export compatibility, and later cross-module orchestration.
- [ ] Public claims are reduced where current tests do not cover hostile input, rollback, permissions, or real application integration.
- [ ] Any retained effect behavior has focused tests and clear diagnostics for unsupported platforms or incomplete backends.
