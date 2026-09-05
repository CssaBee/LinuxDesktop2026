# 79 - Measure Framework Tax In Maintained Proof

**What to build:** Track whether LinuxDesktop2026 removes portability
complexity or replaces it with excessive project-specific vocabulary.

**Blocked by:** 76 - Finish Maintained Consumer Proof Evidence.

**Status:** implemented

- [x] Record how many LinuxDesktop2026 concepts the maintained proof consumer
  must understand to implement common settings/path/watch tasks.
- [x] Track adapter LOC, reports/options constructed, platform branches
  eliminated, and product policy kept outside the library.
- [x] Identify convenience APIs only when repeated proof friction justifies
  them.
- [x] Feed the metrics into ticket 17 before reopening the roadmap.

## Result

`docs/consumer-branches/notepadpp-settings-proof.md` now records the current
framework-tax snapshot for crossport commit `a296934f`: 252 lines of backend
implementation, 109 lines of product-shaped header, five LinuxDesktop2026
concept families, explicit report/option touchpoints, zero platform
preprocessor branches in `proof/`, and the product policies kept outside the
library. The snapshot explicitly says `ld_watch` is not part of the current
settings proof, so watcher concept tax is not measured by this branch yet.

`docs/FlavorTests/API_FRICTION.md` points to that maintained-proof metric
snapshot, and ticket 17 now records that no new convenience API should be
activated from this single proof alone.

## Review Anchor

The newer review warns that the paths/root/settings taxonomy is defensible
internally but may impose too much concept tax on consumers.
