# 27 — Record FlavorTest API Friction Notes

**What to build:** Add lightweight per-flavor notes that record what still feels
unnatural after the first ergonomics pass, so passing tests do not masquerade as
adoption evidence.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 25 — Add Clear Config Defaults Alias; 26 — Add Ergonomic Migration Action Helpers.

**Status:** ready-for-agent

- [ ] Notepad++, Audacity, qBittorrent, KeePassXC, KiCad, FreeCAD, PrusaSlicer,
  OpenRGB, and OBS each record the remaining LinuxDesktop2026 concepts visible
  at their product seam.
- [ ] Each note distinguishes acceptable platform-mechanism vocabulary from
  product-boundary leakage that should drive a future API change.
- [ ] The notes call out where convenience helpers made code shorter but did not
  improve local reasoning.
- [ ] README or FlavorTests docs point maintainers to these notes before they
  treat a passing FlavorTest as integration-readiness evidence.
