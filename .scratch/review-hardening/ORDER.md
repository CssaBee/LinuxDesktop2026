# Review Hardening Ticket Order

This order is the active remaining implementation sequence after the Flavor
review round. Ticket numbers remain historical; execution order follows the
list below.

## Current Order

1. `30` — Translate Migration Plans At Product Boundaries
2. `31` — Finish Common Config Write Audit
3. `32` — Prototype Root Request Builder
4. `33` — Add Walnut FlavorTest Candidate
5. `34` — Add OpenIPC Dashboard FlavorTest Candidate
6. `35` — Record OpenIPC Dashboard As Reference Case
7. `36` — Add OBS Cross-Port Flavor Review
8. `15` — Validate APIs With One Maintained Consumer Branch
9. `18` — Expand CI Into Portability Evidence
10. `16` — Decide Keep, Wrap, Or Retire Native Watch Backends
11. `17` — Reopen Roadmap Only From Consumer Evidence

## Rationale

The next pass resolves FlavorTest product-boundary leaks before treating the
current APIs as ready for maintained consumer validation. Migration result
translation comes first because it is the clearest public seam leak. The common
write audit and root-request builder follow because they decide whether current
helpers are good enough or still charge too much framework tax.

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
