# 95 - Add Maintained Desktop Registration Proof

**What to build:** Validate at least one real consumer integration that uses
`ld_desktop` registration effects and records the maintenance, CI, and API
friction evidence needed before treating the desktop surface as product-ready.

**Blocked by:** 94 - Add Desktop Flavor Registration Validation.

**Status:** ready-for-agent

- [ ] Choose a consumer branch whose desktop registration needs are real enough
  to exercise staged artifacts, activation follow-up, and cleanup reporting.
- [ ] Record branch, remote visibility, commit, CI status, rebase/maintenance
  state, and API friction in the maintained-consumer ledger.
- [ ] Keep local Desktop Flavor fixtures labeled as ergonomics evidence, not as
  a substitute for maintained proof.
- [ ] Reopen the desktop bundle or individual effect design only from observed
  consumer friction, not from hypothetical shell or installer features.

## Evidence Fit

Maintained proof should catch this: the risk is an API that looks good in local
fixtures but costs too much to keep working in a real codebase.
