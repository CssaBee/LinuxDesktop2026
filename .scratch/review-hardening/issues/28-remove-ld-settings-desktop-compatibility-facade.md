# 28 — Remove `ld_settings` Desktop Compatibility Facade

**What to build:** Remove the remaining `ld_settings` compatibility layer for
desktop effects so `ld_desktop` is the only module that owns autostart and
managed/enforced policy behavior.

**Blocked by:** None — the desktop extraction already exists.

**Expiry:** release-candidate cleanup. After that point, any remaining `ld_settings -> ld_desktop` compatibility link is a defect, not a supported transition.

**Status:** done

- [x] `ld_settings` no longer PUBLIC-links `ld_desktop` for compatibility.
- [x] Callers that need desktop effects link `LinuxDesktop2026::ld_desktop`
  directly.
- [x] No `ld_settings` header or helper presents desktop effects as a
  settings-owned surface.
- [x] README, ADR 0012, and `docs/plan/ld-desktop-extraction.md` describe
  desktop effects as direct `ld_desktop` ownership only.
