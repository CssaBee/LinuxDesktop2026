# 03 — Define Settings Module Boundaries

**What to build:** The project should have a concrete boundary decision for `ld_settings`: what remains core settings/config behavior, what moves to `ld_paths`, and what is postponed or split into later effect/migration modules.

**Blocked by:** None — can start immediately.

**Status:** ready-for-agent

- [ ] `ld_settings` has a documented core responsibility centered on settings roots and config-bundle behavior.
- [ ] Generic path/root policy is assigned to `ld_paths`.
- [ ] Registry, autostart, policy, and migration execution are classified as prototype-only, future extracted modules, or explicitly postponed behavior.
