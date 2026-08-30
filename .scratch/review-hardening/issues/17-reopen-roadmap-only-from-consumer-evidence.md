# 17 — Reopen Roadmap Only From Consumer Evidence

**What to build:** Future modules beyond the ADR 0012 extraction work should move from research notes to active roadmap items only when real integrations prove repeated demand and a sustainable abstraction boundary.

**Blocked by:** 15 — Validate APIs With One Maintained Consumer Branch; 16 — Decide Keep, Wrap, Or Retire Native Watch Backends.

**Status:** implemented-awaiting-consumer-evidence

**Implementation note:** The roadmap and README now define activation gates for
future modules. `ld_process`, `ld_ipc`, `ld_dynlib`, service lifecycle, and
UI-adjacent helpers are research-only until repeated source/consumer evidence
and an existing-tool decision justify activation. `ld_desktop` and
`ld_migration` remain allowed because they extract already-implemented behavior
from the wrong module.

- [x] Later modules remain explicitly research-only until backed by real consumer evidence, except for the required `ld_desktop` and `ld_migration` extractions from `ld_settings`.
- [x] Any newly activated module has a documented existing-tool decision before implementation.
- [x] Public API design for new modules waits until at least two integrations show the same concept is shared.

This ticket should close only after task 15 records enough maintained-consumer
evidence to prove that the gate works against a real branch, not just local
FlavorTests.
