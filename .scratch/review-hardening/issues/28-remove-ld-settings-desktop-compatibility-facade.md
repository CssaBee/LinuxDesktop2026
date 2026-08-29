# 28 — Remove `ld_settings` Desktop Compatibility Facade

**What to build:** Remove the remaining `ld_settings` compatibility layer for
desktop effects so `ld_desktop` is the only module that owns autostart and
managed/enforced policy behavior.

**Blocked by:** None — the desktop extraction already exists.

**Status:** ready-for-agent

- [ ] `ld_settings` no longer PUBLIC-links `ld_desktop` for compatibility.
- [ ] Callers that need desktop effects link `LinuxDesktop2026::ld_desktop`
  directly.
- [ ] No `ld_settings` header or helper presents desktop effects as a
  settings-owned surface.
- [ ] README, ADR 0012, and `docs/plan/ld-desktop-extraction.md` describe
  desktop effects as direct `ld_desktop` ownership only.
