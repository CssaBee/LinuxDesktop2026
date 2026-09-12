# Review Hardening Retrospective

This retrospective answers why the September 2026 findings survived earlier
hardening and turns the answer into repeatable review checks.

## Pattern

The misses were not mostly hidden algorithms. They were claims that were true
at one layer and false, incomplete, or unproven at the boundary users care
about:

- a bounded raw queue did not mean bounded settled-file work;
- a generated desktop or dconf artifact did not mean the effect was activated;
- an atomic replace did not mean concurrent edits were protected;
- a local FlavorTest did not mean a maintained consumer proof was current;
- parser behavior that was useful for app-settings fixtures was not a general
  file-format promise;
- names that were implementation-honest still sounded broader than the
  contract.

## Missed Questions

| Finding | Missed question | Evidence source that should have caught it |
| --- | --- | --- |
| `54` module-boundary claim drift | Public-claim drift: does the docs claim match the actual dependency graph? | Docs ledger plus source review |
| `55` settings root fixture gap | Ownership boundary: do tests cross real filesystem/device shapes that users hit? | Adversarial test |
| `56` settle worker permanent exit | Lifecycle invariant: can one invalid path retire future work? | Unit test plus adversarial test |
| `57` callback self-destruction | Lifecycle invariant: what happens if a callback mutates its owner? | Adversarial test |
| `58` watcher discovery pass | Workload shape: do focused backend runs cover lifecycle and settled-file behavior? | CI plus source review |
| `59` sanitizer and adversarial CI evidence | Failure mode: are race-shaped lifecycle cases in the right lane? | CI |
| `60` migration filesystem semantics | Semantic naming: do move/copy words imply metadata or topology guarantees? | Source review plus docs ledger |
| `61` settled option initializer warnings | Semantic naming: are tests readable enough to expose wrong option fields? | Unit test plus source review |
| `62` settled-file readiness growth | Workload shape: can settled work grow without the raw delivery queue growing? | Adversarial test plus CI |
| `63` blocking settle FIFO | Workload shape: can slow readiness for one path block unrelated ready paths? | Adversarial test plus CI |
| `64` self-stop ownership invariant | Lifecycle invariant: is the lifetime proof explicit after self-stop returns? | Unit test plus source review |
| `65` inotify remove-watch pending state | Ownership boundary: does backend cleanup erase only state owned by that watch? | Unit test plus source review |
| `66` watch test hooks exposed | Ownership boundary: can test-only backend affordances enter normal builds? | CI plus source review |
| `67` named-root traversal | Failure mode: are relative paths hostile even when not absolute? | Adversarial test |
| `68` enum string fallback | Failure mode: do new enum values fail visibly instead of silently degrading? | Unit test |
| `69` desktop durable writes | Failure mode: do all mutating modules use the same durable write quality? | Source review plus unit test |
| `70` dconf activation honesty | Public-claim drift: does writing an artifact activate policy, or only stage it? | Docs ledger plus adversarial test |
| `71` migration JSON parser scope | Parser grammar: is this a scoped fixture parser or a general JSON promise? | Adversarial test plus docs ledger |
| `72` `.reg` compatibility scope | Parser grammar: which Registry syntax is actually promised? | Adversarial test plus docs ledger |
| `73` settings interprocess writes | Failure mode: does atomic replacement prevent corruption but still lose edits? | Maintained proof plus source review |
| `74` migration move naming | Semantic naming: does API text imply broader filesystem preservation than exists? | Docs ledger plus source review |
| `75` migration file size | Ownership boundary: can reviewers see parser, planner, and filesystem behavior separately? | Source review |
| `76` maintained proof evidence | Public-claim drift: does proof status include remote, CI, rebase, and maintenance facts? | Maintained proof plus docs ledger |
| `77` public validation state | Public-claim drift: is live proof status repeated in multiple public docs? | Docs ledger |
| `78` FetchContent example pin | Public-claim drift: does public sample code teach a stable dependency boundary? | Source review plus docs ledger |
| `79` framework tax metric | Ownership boundary: are helper layers hiding integration cost? | Maintained proof plus FlavorTest |
| `80` root CMake growth | Ownership boundary: can new module owners find their build surface? | Source review |
| `81` coverage, failure mode, watch performance evidence | Failure mode: do tests prove the risky states reviewers keep naming? | CI plus unit test |
| `82` bus factor and governance | Ownership boundary: who reviews each module and where is help needed? | Docs ledger plus source review |
| `84` watch queue overflow erase | Ownership boundary: does overflow handling drop only the overloaded stream? | Adversarial test plus source review |
| `85` private crossport status | Public-claim drift: did status say the proof did not exist after it had moved private? | Docs ledger plus maintained proof |
| `86` desktop bundle registration surface | Ownership boundary: is registration modeled as staged artifacts, plans, and effects? | FlavorTest plus source review |
| `87` staged XDG artifacts | Parser grammar: are IDs, MIME names, protocols, and activation plans explicit? | Adversarial test plus docs ledger |
| `88` Windows registration posture | Public-claim drift: does Windows capability reporting distinguish artifacts from live system effects? | CI plus docs ledger |
| `89` versioned settings contract | Failure mode: what write contract handles stale user-edited state? | Maintained proof plus source review |
| `90` versioned settings implementation | Failure mode: does the accepted contract reject stale and mismatched writes? | Maintained proof plus unit test |
| `122` large-tree watch noise | Workload shape: do raw queue, settled pending work, and product validation counts stay distinguishable under noisy large-tree pressure? | Performance probe plus docs ledger |

## Pre-Ticket Checklist

Before accepting a future hardening ticket as scoped, challenge these questions:

- **Boundedness:** Which queue, map, worker, retry path, or deferred task can
  grow independently of the bounded structure already under test?
- **Activation:** Does the operation stage artifacts, request activation, or
  verify an active system effect? Name the boundary in capability reports and
  docs.
- **Parser scope:** Is the parser a real format implementation, a schema-bound
  importer, or a fixture helper? Tests and public names must match that scope.
- **Public claims:** Is every branch, CI, proof, support, and validation claim
  sourced from one current ledger rather than repeated prose?
- **Ownership:** Can cleanup, overflow, or backend teardown erase unrelated
  user, watch, pending, or generated state?
- **Naming:** Would a reasonable caller infer a stronger contract from the API
  name than the implementation intends to provide?
- **Evidence fit:** Is the claim best caught by a unit test, adversarial test,
  FlavorTest, maintained proof, source review, CI, or docs ledger? Add the
  missing evidence source before treating the ticket as done.

## Process Change

Future review-derived tickets should carry a one-line "evidence fit" in their
acceptance notes. A ticket that changes a public claim, parser contract,
activation boundary, or write/concurrency guarantee is not complete until the
matching evidence source above has either been added or explicitly rejected as
out of scope.
