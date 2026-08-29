# 29 — Remove `ld_settings` Migration Compatibility Facade

**What to build:** Remove the remaining `ld_settings` compatibility layer for
migration so `ld_migration` is the only module that owns planning, execution,
and Registry-shaped migration compatibility.

**Blocked by:** None — the migration extraction already exists.

**Expiry:** release-candidate cleanup. After that point, any remaining `ld_settings -> ld_migration` compatibility link is a defect, not a supported transition.

**Status:** ready-for-agent

- [ ] `ld_settings` no longer PUBLIC-links `ld_migration` for compatibility.
- [ ] Callers that need migration behavior link `LinuxDesktop2026::ld_migration`
  directly.
- [ ] No `ld_settings` header or helper presents migration as a
  settings-owned surface.
- [ ] README, ADR 0012, and `docs/plan/ld-migration-extraction.md` describe
  migration ownership as direct `ld_migration` ownership only.
