# 124 - Document ld_watch Nextcloud Evidence

**What to build:** Record the Nextcloud watcher overload scan as non-blocking
`ld_watch` evidence and update release-facing docs only after the probe or
measurements produce concrete results.

**Blocked by:** None; unblocked for source-anchor notes by implemented ticket
`121` and for measurement claims by implemented ticket `122`.

**Status:** proposed

- [ ] Add the Nextcloud Desktop issue `#7873` source anchor to the appropriate
  watcher survey or validation-evidence document.
- [ ] Record that qBittorrent `#24444` remains a watch item for possible
  `ld_desktop` reveal-folder behavior and is not current `ld_watch` evidence.
- [ ] After ticket `122`, summarize what the large-tree workload proves and
  what remains unproven.
- [ ] Update `docs/plan/library-roadmap.md` only if the evidence changes the
  `ld_watch` direction, capability fields, wrap decision, or release posture.
- [ ] Avoid public claims that LinuxDesktop2026 solves Nextcloud's upstream
  issue unless a maintained proof branch or upstream-shaped patch demonstrates
  that integration.

## Review Anchor

The scan is useful because it is concrete external pressure on the `ld_watch`
contract. It should become durable evidence without turning into an inflated
adoption claim.

## Evidence Fit

Docs-ledger evidence should catch this: release notes and roadmap text must
distinguish candidate signal, synthetic proof, maintained proof, and upstream
contribution readiness.

## Release Gate

Does not block `0.2.1`. Promote only if documentation starts making claims
that are stronger than the completed proof evidence.
