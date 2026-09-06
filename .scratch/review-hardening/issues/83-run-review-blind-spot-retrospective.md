# 83 - Run Review Blind-Spot Retrospective

**What to build:** Answer why these issues survived earlier hardening and turn
the answer into reusable review checks.

**Blocked by:** None.

**Status:** implemented

- [x] For each September 2026 finding, classify the missed question: workload
  shape, lifecycle invariant, public-claim drift, semantic naming, parser
  grammar, failure mode, or ownership boundary.
- [x] Identify which current evidence source should have caught it: unit test,
  adversarial test, FlavorTest, maintained proof, source review, CI, or docs
  ledger.
- [x] Add a short pre-ticket checklist for future hardening reviews so bounded
  claims, activation claims, parser scope, and public examples are challenged.
- [x] Update ADR 0014 or adjacent hardening docs only if the retrospective
  changes the testing posture.

## Result

Added `docs/review-hardening-retrospective.md` with a concise classification
of the September 2026 findings by missed question and matching evidence source.
The pre-ticket checklist now lives in both that retrospective and the active
review order. ADR 0014 now says review-derived tickets must name the evidence
source that fits the claim, because adversarial input tests alone would not
have caught public status drift, activation drift, maintained-proof drift, or
some workload-shape bugs.

## Initial Hypothesis

The missed issues are not random. They cluster around claims that sounded true
at one layer but were not true end-to-end: bounded delivery vs unbounded settled
work, dconf file generation vs active policy, atomic write quality in one
module but not another, local proof vs maintained public proof, and documented
migration limits vs intuitive API names.

## Review Anchor

The user explicitly asked, "why is it that we didn't find these issues?" This
ticket makes that a first-class hardening task rather than an afterthought.
