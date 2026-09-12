# Review Hardening Backlog

This is the current hardening queue. It tracks live work only. Completed review
paths are compressed in `HISTORY.md` and in the public evidence docs.

## Release Gate

No review-hardening ticket currently blocks `0.2.1`.

Promote a ticket to the release gate only when current code or docs expose a
correctness, boundedness, mutation-safety, parser-scope, activation, or
public-claim problem.

## Current Order

1. `issues/122-measure-large-tree-watch-noise-and-coalescing.md`
2. `issues/123-add-watch-coalescing-diagnostics-for-product-boundaries.md`
3. `issues/124-document-ld-watch-nextcloud-evidence.md`
4. `issues/82-reduce-bus-factor-and-write-governance.md`
5. `issues/15-validate-apis-with-one-maintained-consumer-branch.md`
6. `issues/17-reopen-roadmap-only-from-consumer-evidence.md`

## Evidence Cadence

- Keep the Notepad++ maintained proof ledger current in
  `docs/consumer-branches/notepadpp-settings-proof.md`; it remains the source
  for rebase, dependency, compile, API-friction, and CI observations.
- Keep future-module activation gated by repeated real-consumer evidence plus
  an existing-tool decision. `ld_process`, `ld_ipc`, `ld_dynlib`, service
  lifecycle, and UI-adjacent helpers remain research-only.
- Keep `docs/project-status.md`, `docs/validation-evidence.md`, and
  `docs/review-hardening-retrospective.md` as the durable public-state ledgers.

## Intake Rule

New review-derived tickets must fit on one screen:

- problem, current code/doc anchor, and why it matters;
- required evidence source: unit/adversarial test, FlavorTest, maintained
  proof, CI, source review, or docs ledger;
- acceptance checks;
- release-gate disposition.

Do not compress not-implemented tickets. Keep proposed, pending, blocked, or
evidence-in-progress work as individual files and list it here.

Do not open a new ticket for historical explanation. Put completed context in
`HISTORY.md`, the retrospective, or the relevant evidence ledger.
