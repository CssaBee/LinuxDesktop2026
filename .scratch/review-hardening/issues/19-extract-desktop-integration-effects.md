# 19 — Extract Desktop Integration Effects

**What to build:** Move desktop integration effects out of `ld_settings` into a real `ld_desktop` module.

**Blocked by:** None - task 05 prepared the extraction inventory.

**Status:** done

- [ ] `ld_desktop` owns autostart, desktop entries, icons, MIME/file associations, default applications, URL protocol handlers, shell-equivalent behavior, desktop database updates, and managed/enforced policy.
- [ ] Existing `ld_settings::effects` behavior is removed, moved, or replaced with documented pre-1.0 migration guidance.
- [ ] Tests cover Linux and Windows-shaped backends or capability diagnostics for every moved effect.
- [ ] `docs/plan/ld-desktop-extraction.md` is used as the implementation checklist.
