# Review Hardening Ticket Order

This order is the active remaining implementation sequence after the Flavor
review round. Ticket numbers remain historical; execution order follows the
list below.

## Current Order

1. `17` - Reopen Roadmap Only From Consumer Evidence

## Maintained Consumer Evidence In Progress

- `15` - Validate APIs With One Maintained Consumer Branch

## Rationale

The next pass resolves remaining FlavorTest product-boundary leaks before
treating the current APIs as ready for maintained consumer validation. Migration
result translation, the common write audit, the experimental root-request
builder pass, and the maintained-branch contract are complete, so the sequence
can move to broader portability evidence while the first external branch is
created and kept building.

Walnut and OpenIPC Dashboard now stay concrete without a separate intake
template gate: each candidate ticket must name its own source-anchored seam
before code is added. OpenIPC is tracked twice: as a FlavorTest candidate for
data-root/profile separation and as a reference case for when Qt already solves
a seam well enough that LinuxDesktop2026 should document, recommend, or adapt
instead of replacing it. OBS completed the first cross-port review pilot;
cross-port reviews use source anchors and paraphrased lessons, not copied
upstream code snippets.

The project now has an expanded CI portability matrix, a documented
maintained-consumer proof workflow, and an explicit watcher backend decision:
keep native Linux and Windows backends as owned pre-1.0 behavior, keep libuv
optional/recommended for libuv-shaped apps, and reopen wrapping only from CI,
stress-test, or maintained-consumer evidence. Task 15 has its first local
cross-port proof target plus a product-shaped Notepad++ settings backend
rewrite, but stays open until the Notepad++ settings proof branch records
observed CI, regular build, rebase, dependency, compile, and API-friction
evidence; local FlavorTests are not enough to close it.
