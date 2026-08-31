# 42 - Design Root Module Boundary

**What to build:** Turn the current root-resolution friction into a precise
public `ld_root` boundary proposal before any public API is added.

**Blocked by:** 41 - Document Platform Path Defaults Evidence.

**Status:** ready-for-agent

- [ ] `docs/FlavorTests/API_FRICTION.md` is used as the baseline evidence.
- [ ] The design distinguishes `ld_paths`, proposed `ld_root`, `ld_settings`,
  and product-owned policy.
- [ ] User-owned roots and app-owned roots are defined in product-facing terms.
- [ ] The design names which Notepad++, qBittorrent, KiCad, Walnut, and OpenIPC
  Dashboard seams support or reject a shared root module.
- [ ] The design states what must not move into `ld_root`.
- [ ] No public `ld_root` API is added in this ticket.

## Problem Statement

`settings.cpp` has grown into a place where several ownership boundaries are
implemented together. The size is a signal, but the hardening problem is public
model drift: root topology could become either settings-specific forever or a
too-broad path helper unless the boundary is designed from consumer evidence.

## Result

Pending.
