# 83 - Run Review Blind-Spot Retrospective

**What to build:** Answer why these issues survived earlier hardening and turn
the answer into reusable review checks.

**Blocked by:** None.

**Status:** pending

- [ ] For each September 2026 finding, classify the missed question: workload
  shape, lifecycle invariant, public-claim drift, semantic naming, parser
  grammar, failure mode, or ownership boundary.
- [ ] Identify which current evidence source should have caught it: unit test,
  adversarial test, FlavorTest, maintained proof, source review, CI, or docs
  ledger.
- [ ] Add a short pre-ticket checklist for future hardening reviews so bounded
  claims, activation claims, parser scope, and public examples are challenged.
- [ ] Update ADR 0014 or adjacent hardening docs only if the retrospective
  changes the testing posture.

## Initial Hypothesis

The missed issues are not random. They cluster around claims that sounded true
at one layer but were not true end-to-end: bounded delivery vs unbounded settled
work, dconf file generation vs active policy, atomic write quality in one
module but not another, local proof vs maintained public proof, and documented
migration limits vs intuitive API names.

## Review Anchor

The user explicitly asked, "why is it that we didn't find these issues?" This
ticket makes that a first-class hardening task rather than an afterthought.
