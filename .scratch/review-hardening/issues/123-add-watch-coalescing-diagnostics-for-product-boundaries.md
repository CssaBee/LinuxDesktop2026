# 123 - Add Watch Coalescing Diagnostics For Product Boundaries

**What to build:** Decide whether `ld_watch` should expose aggregate
coalescing and overload diagnostics that product adapters can translate
without logging one discarded native event per source notification.

**Blocked by:** None; unblocked by implemented ticket `122`.

**Status:** implemented

- [x] Review existing `ld_watch` diagnostics and performance counters against
  the large-tree probe results.
- [x] If existing reports already expose the needed signal, document the
  product-owned diagnostic translation pattern instead of adding API.
- [x] If signal is missing, add narrow aggregate counters or diagnostics such
  as received, coalesced, delivered, dropped, overflowed, settle-timeout, and
  validation-call counts.
- [x] Keep diagnostics aggregate-first for noisy paths; avoid creating a
  product burden to log one library diagnostic per discarded backend event.
- [x] Add tests proving diagnostics remain bounded during repeated notifications
  for the same path and during queue saturation.

## Implementation Notes

The large-tree probe already exposes aggregate received, coalesced, delivered,
overflow, dropped, pending, and validation-call measurements for validation
runs, while the public `ld_watch` contract exposes bounded delivery, settled
coalescing, and overflow/rescan diagnostics. Task `123` therefore does not add a
public watcher API.

The Nextcloud-shaped FlavorTest now records the product-owned aggregate batch
shape: raw events observed, overflow events observed, candidates retained,
candidates dropped for rescan, candidates validated, sync work, ignored
spurious candidates, and summary log lines. Tests prove repeated same-path
noise and candidate saturation stay bounded and summarized instead of creating
per-event validation or log chatter.

## Review Anchor

The Nextcloud-shaped problem is not just receiving many events. It is letting
per-event filtering and logging become the workload. LinuxDesktop2026 should
help products explain overload and coalescing as bounded aggregate status when
that evidence is needed.

## Evidence Fit

Unit, adversarial, and product-boundary evidence should catch this: diagnostics
are part of the workload and must not reintroduce the chatter that coalescing
removed.

## Release Gate

Does not block `0.2.1`. Promote only if ticket `122` shows current diagnostics
hide material watcher overload behavior or force product adapters into
per-event chatter.
