# 94 - Add Desktop Flavor Registration Validation

**What to build:** Prove the registration model against hermetic Desktop Flavor
fixtures for full XDG desktops, lighter XDG sessions, and Windows-shaped
registration without claiming live desktop consumption.

**Blocked by:** 92 - Wire Desktop Bundle To Staged Registration Effects; 93 - Add Desktop Registration Cleanup Reports.

**Status:** implemented

- [x] Flavor coverage exercises GNOME-like, KDE-like, Xfce-like, bare
  window-manager, and Windows-shaped registration scenarios through the same
  product-facing bundle vocabulary.
- [x] Assertions focus on staged artifacts, activation plans, capability
  reporting, cleanup reports, and diagnostics rather than live shell behavior.
- [x] The validation keeps runtime shell-open, reveal-in-file-manager, and
  single-instance activation outside `ld_desktop` unless new consumer evidence
  reopens that boundary.
- [x] API friction notes record where the bundle path reduces or adds framework
  tax for product-shaped callers.

## Implementation Note

The Desktop Flavor registration validation completed before task 95's maintained
desktop proof. The coverage stays hermetic: it validates staged artifacts,
activation plans, capability reports, cleanup reports, diagnostics, and API
friction without claiming live desktop-session consumption.

## Evidence Fit

FlavorTests should catch this: the risk is a bundle model that passes unit tests
but feels wrong or misleading at real product call sites.
