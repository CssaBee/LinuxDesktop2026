# 123 - Add Watch Coalescing Diagnostics For Product Boundaries

**What to build:** Decide whether `ld_watch` should expose aggregate
coalescing and overload diagnostics that product adapters can translate
without logging one discarded native event per source notification.

**Blocked by:** None; unblocked by implemented ticket `122`.

**Status:** proposed

- [ ] Review existing `ld_watch` diagnostics and performance counters against
  the large-tree probe results.
- [ ] If existing reports already expose the needed signal, document the
  product-owned diagnostic translation pattern instead of adding API.
- [ ] If signal is missing, add narrow aggregate counters or diagnostics such
  as received, coalesced, delivered, dropped, overflowed, settle-timeout, and
  validation-call counts.
- [ ] Keep diagnostics aggregate-first for noisy paths; avoid creating a
  product burden to log one library diagnostic per discarded backend event.
- [ ] Add tests proving diagnostics remain bounded during repeated notifications
  for the same path and during queue saturation.

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
