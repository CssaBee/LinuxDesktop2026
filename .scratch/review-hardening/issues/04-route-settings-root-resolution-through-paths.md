# 04 — Route Settings Root Resolution Through Paths

**What to build:** `ld_settings` should use `ld_paths` for generic root and path selection while preserving the observable settings behavior callers and tests already rely on.

**Blocked by:** 03 — Define Settings Module Boundaries.

**Status:** ready-for-agent

- [ ] Settings root resolution delegates generic config/data/state/cache/resource path selection to `ld_paths`.
- [ ] Existing settings tests continue to pass or are updated only to reflect the documented boundary decision.
- [ ] Compatibility shims preserve pre-RC source compatibility where practical.
