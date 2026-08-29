# 35 - Record OpenIPC Dashboard As Reference Case

**What to build:** Add OpenIPC Dashboard to the survey/reference material as an
example of an existing cross-platform Qt application whose design can inform
LinuxDesktop2026 without being replaced by it.

**Blocked by:** None.

**Status:** proposed

- [ ] Survey notes record Dashboard as both a FlavorTest candidate and a
  reference case.
- [ ] Notes identify which seams Qt already covers well.
- [ ] Notes identify which seams still pressure LinuxDesktop2026 concepts:
  profile roots, service/headless startup, diagnostics, migration, release
  packaging, updater behavior, and desktop/server separation.
- [ ] The reference note does not imply LinuxDesktop2026 should replace Qt in a
  Qt-native application.

## Problem Statement

OpenIPC Dashboard is useful in two different ways. It can provide a concrete
FlavorTest seam, and it can also teach when LinuxDesktop2026 should recommend or
adapt to an existing toolkit instead of offering a competing abstraction.
