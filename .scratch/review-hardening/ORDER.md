# Review Hardening Ticket Order

This order is the active remaining implementation sequence after the Flavor
review round. Ticket numbers remain historical; execution order follows the
list below.

## Current Order

1. `34` — Add OpenIPC Dashboard FlavorTest Candidate
2. `35` — Record OpenIPC Dashboard As Reference Case
3. `36` — Add OBS Cross-Port Flavor Review
4. `15` — Validate APIs With One Maintained Consumer Branch
5. `18` — Expand CI Into Portability Evidence
6. `16` — Decide Keep, Wrap, Or Retire Native Watch Backends
7. `17` — Reopen Roadmap Only From Consumer Evidence

## Rationale

The next pass resolves remaining FlavorTest product-boundary leaks before
treating the current APIs as ready for maintained consumer validation. Migration
result translation, the common write audit, and the experimental root-request
builder pass are complete, so the sequence can move to new candidate evidence.

Walnut and OpenIPC Dashboard now stay concrete without a separate intake
template gate: each candidate ticket must name its own source-anchored seam
before code is added. OpenIPC is tracked twice: as a FlavorTest candidate for
data-root/profile separation and as a reference case for when Qt already solves
a seam well enough that LinuxDesktop2026 should document, recommend, or adapt
instead of replacing it. OBS is the first cross-port review pilot; cross-port
reviews use source anchors and paraphrased lessons, not copied upstream code
snippets.

Only after that cleanup should the project return to maintained consumer
validation, broader portability evidence, watch-backend decisions, and roadmap
reopening from consumer evidence.
