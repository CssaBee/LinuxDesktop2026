# 03 — Define Settings Module Boundaries

**What to build:** The project should have a concrete boundary decision for `ld_settings`: what remains core settings/config behavior, what moves to `ld_paths`, and what must be extracted to `ld_desktop` and `ld_migration`.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] `ld_settings` has a documented core responsibility centered on settings roots and config-bundle behavior.
- [x] Generic path/root policy is assigned to `ld_paths`.
- [x] Registry, autostart, policy, and migration execution are classified as temporary implementation locations with required extraction to `ld_desktop` and `ld_migration`.
- [x] The temporary `linuxdesktop::settings::registry` and `linuxdesktop::settings::effects` namespace bridges have been removed.
