# 36 - Add OBS Cross-Port Flavor Review

**What to build:** Add a cross-port review pass for the OBS FlavorTest that
compares the product concept across OBS's Windows and Linux implementations
without copying upstream code into the repository.

**Blocked by:** 27 - Record FlavorTest API Friction Notes.

**Status:** proposed

- [ ] `docs/FlavorTests/SOURCES.md` labels OBS as the first cross-port review
  pilot.
- [ ] The review uses links, source anchors, and short paraphrased notes rather
  than commented upstream code snippets.
- [ ] `docs/FlavorTests/API_FRICTION.md` records whether LinuxDesktop2026
  matches OBS's shared product concept or merely mirrors one platform backend.
- [ ] The review identifies at least one keep/change/defer lesson for C-shaped
  path and config-save seams.

## Decision

Do not add commented upstream code snippets to FlavorTests. The side-by-side
idea is dropped because source copying can create review noise and licensing or
copyright concerns. Cross-port comparison should happen through anchors and
paraphrased notes instead.
