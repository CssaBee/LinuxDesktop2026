# 42 - Design Root Module Boundary

**What to build:** Turn the current root-resolution friction into a precise
public `ld_root` boundary proposal before any public API is added.

**Blocked by:** 41 - Document Platform Path Defaults Evidence.

**Status:** done

- [x] `docs/FlavorTests/API_FRICTION.md` is used as the baseline evidence.
- [x] The design distinguishes `ld_paths`, proposed `ld_root`, `ld_settings`,
  and product-owned policy.
- [x] User-owned roots and app-owned roots are defined in product-facing terms.
- [x] The design names which Notepad++, qBittorrent, KiCad, Walnut, and OpenIPC
  Dashboard seams support or reject a shared root module.
- [x] The design states what must not move into `ld_root`.
- [x] No public `ld_root` API is added in this ticket.

## Problem Statement

`settings.cpp` has grown into a place where several ownership boundaries are
implemented together. The size is a signal, but the hardening problem is public
model drift: root topology could become either settings-specific forever or a
too-broad path helper unless the boundary is designed from consumer evidence.

## Result

Completed in `specs/root-module-boundary.md`. The design keeps `ld_paths`
responsible for platform path families and generated defaults, defines the
proposed public `ld_root` boundary around reusable application root topology,
keeps settings overrides/config layers/hydration/writes in `ld_settings`, and
leaves product-specific profile, project, service, cloud, diagnostic, and file
format policy in adapters.

The FlavorTest evidence is split deliberately: Notepad++, qBittorrent, and
KiCad support a shared root-topology boundary; Walnut rejects forcing simple
graphics bootstrap through that model; OpenIPC Dashboard rejects flattening an
isolated service profile into generic root helpers unless more products repeat
the same shape.
