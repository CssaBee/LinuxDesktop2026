# Review Hardening Ticket Order

This order is the active remaining implementation sequence after the Flavor
review round. Ticket numbers remain historical; execution order follows the
list below.

## Current Order

- `90` - Implement Versioned Settings Commit API
- `81` - Add Coverage Failure-Mode And Watch Performance Evidence
- `80` - Decompose Root CMake Before Next Module Wave
- `82` - Reduce Bus Factor And Write Governance
- `83` - Run Review Blind-Spot Retrospective
- `86` - Design Desktop Bundle Registration Surface
- `87` - Add Staged XDG Desktop Registration Artifacts
- `88` - Prove Windows Registration Capability Posture

## Implemented

- `89` - Design Versioned Settings Commit Contract
- `79` - Measure Framework Tax In Maintained Proof
- `78` - Pin FetchContent Example
- `77` - Centralize Dynamic Public Validation State
- `76` - Finish Maintained Consumer Proof Evidence
- `75` - Split Migration Internals By Responsibility
- `74` - Rename Or Split Migration Move Semantics
- `73` - Decide Settings Interprocess Write Contract
- `72` - Formally Scope Reg File Compatibility
- `71` - Replace Or Formally Scope Migration JSON Parser
- `68` - Enforce Enum String Exhaustiveness
- `70` - Make Dconf Policy Activation Honest
- `69` - Share Durable File Write Primitive With Desktop
- `64` - Protect Watch Self-Stop Ownership Invariant
- `63` - Replace Blocking Settle FIFO With Deadline Scheduler
- `62` - Bound And Coalesce Settled-File Readiness Work
- `84` - Preserve Unrelated Events On Watch Queue Overflow
- `66` - Gate Watch Test Hooks Behind CMake Option
- `65` - Scope Inotify Remove-Watch Pending State
- `67` - Reject Named-Root Path Traversal
- `85` - Update Project Status For Private Crossport Proof
- `61` - Clean Up ld_watch Settled-File Option Initializers
- `60` - Narrow And Harden Migration Filesystem Semantics
- `58` - Run ld_watch Lifecycle And Settled-File Discovery
- `59` - Add Adversarial CI And ThreadSanitizer Evidence
- `57` - Fix Watch Callback Self-Destruction Lifecycle
- `56` - Fix Watch Settle Worker Permanent Exit
- `55` - Add Settings Root Resolution Multi-Filesystem Fixtures
- `54` - Reconcile Review Claim And Module Boundary Docs
- `53` - Expand Adversarial Hardening Tests
- `52` - Adopt Invasive Hardening Test Posture
- `51` - Harden Plugin Path Kind Taxonomy
- `50` - Add Portable Root Request API
- `49` - Add Product-Owned Diagnostic Boundaries
- `48` - Contract Settings-Owned Root Builder
- `47` - Add Public Root Topology Surface
- `46` - Separate Path Families From Location Roles
- `45` - Split Path-List Candidate Vocabulary
- `44` - Audit Paths Root Overlap
- `43` - Extract Settings Root Internals
- `42` - Design Root Module Boundary
- `41` - Document Platform Path Defaults Evidence
- `40` - Remove Private FlavorTest Platform Path Helper
- `39` - Add Consumer CMake Path Default Generation
- `38` - Mirror Platform Path Defaults In C ABI
- `37` - Add Runtime Platform Path Defaults

## Maintained Consumer Evidence In Progress

- `15` - Validate APIs With One Maintained Consumer Branch
- `17` - Reopen Roadmap Only From Consumer Evidence

## Rationale

The September 3, 2026 review pass reopens the hardening lane with two inputs:
the broad technical review and the cache-busted re-review of current `main`.
Where the reviews conflict, the current-source check and the cache-busted
review win; already-addressed findings are not reopened.

The now-completed ledger-truth fix taught the maintained-consumer proof state to
distinguish "private remote exists" from "public proof evidence" so future
review does not reason from stale status. The remaining order starts with
correctness and security items: defects that can corrupt state, erase unrelated
watcher backend or delivery state, or expose test-only behavior in ordinary
builds. The settled-file scheduler follows because it is now the largest live
`ld_watch` design risk: raw delivery is bounded, and settled-file readiness now
has bounded path coalescing plus deadline scheduling; remaining watcher work
focuses on lifecycle invariants.

Desktop write durability and dconf activation are ordered together because they
are both end-to-end desktop-effect truth problems: generating a file is not the
same thing as durably installing or activating an effect. Enum exhaustiveness
comes next as a cheap guardrail against future silent diagnostic drift.

Parser scope, settings interprocess semantics, and migration naming come after
the immediate data-loss/security defects because they mostly decide contracts.
Those contracts should be settled before internally splitting `ld_migration`,
otherwise the split risks preserving unclear behavior in nicer files.

Maintained-consumer evidence and public validation-state cleanup follow because
they answer the project's central strategic question: whether these APIs lower
real integration cost over time. FetchContent pinning is small but public-facing
and should happen in the same documentation honesty pass. Framework-tax metrics
then give ticket 17 real evidence instead of intuition. Task 89 followed that
evidence pass and accepted a narrow versioned settings commit contract for
high-impact files like Notepad++ session and shortcut state. Task 90 now
implements that pre-1.0 C++ promise before broader coverage/performance work
continues.

Coverage, failure-mode, and performance evidence are intentionally after the
watcher scheduler work so measurements target the intended shape. CMake
decomposition and bus-factor/governance are real sustainability work, but they
should not displace immediate correctness fixes. The retrospective stays in the
batch because the user asked the hard question directly: the process needs to
explain why boundedness, activation, parser scope, and public-claim drift
survived previous review.

The older FlavorTest/root-path rationale below is retained as historical context
for implemented tickets. Migration result translation, the common write audit,
the experimental root-request builder pass, and the maintained-branch contract
are complete; the active order above is now governed by the September 3 review
batch.

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

The path-defaults batch is active because it hardens an existing `ld_paths`
promise instead of opening a new platform module. Walnut and OpenIPC Dashboard
exposed a specific API gap: `docs/FlavorTests/support/platform_paths.hpp` made
user-root resolution look ergonomic while real consumers could not take that
helper with them. The sequence closes that gap through supported runtime
defaults, mirrors the capability for C callers, proves consumer-target CMake
generation through the install-tree path, removes the private FlavorTest helper,
and records the evidence before treating the improved ergonomics as user-ready.

The root-topology batch now has its boundary and overlap audit. The accepted
constraint remains no tech-debt module: `settings.cpp` being large is a symptom,
not the reason to publish a new API. Task 42 designed the public boundary, task
43 extracted `ld_settings` root internals only along that boundary, and task 44
audited `ld_paths` for overlap without broad redesign. The audit found that
`ld_paths` should keep generic path families, platform defaults, path lists,
directory creation, plugin search-root sets, and executable/resource discovery,
but the current public result shape is not clean enough to feed a future
`ld_root` API.

The next four tickets intentionally take the necessary pre-1.0 breaking changes
instead of leaving compatibility scaffolding behind. Task 45 removes the
synthetic plugin-search path-family vocabulary from path-list and plugin-set
reports. Task 46 separates ordinary path families from
executable/install/resource location roles. Task 47 can then add public root
topology for the repeated Notepad++, qBittorrent, and KiCad seams without
inheriting `ld_paths` ambiguity. Task 48 removes or reshapes the duplicated
settings-owned generic root-builder surface so `ld_settings` returns to settings
lifecycle ownership.

By the end of task 48, the hardening lane should have no known root/path API
tech debt left by design: `ld_paths` is enough for path families, locations,
path lists, directory helpers, and plugin search roots; `ld_root` is enough for
user-owned and app-owned root topology; `ld_settings` is only needed for
settings/config lifecycle. Dependency edges should follow that same order and
users should not bring in a higher-level module to get lower-level behavior.

Task 49 adds the shared core affordance for product diagnostics and keeps those
diagnostics on the product side of the maintained Notepad++ evidence:
LinuxDesktop2026 reports remain internal adapter inputs, while public cross-port
headers expose Notepad++-owned diagnostic severity, handling flags, status
codes, operation summaries, and migration/write/default-hydration decisions.
`ld_core` owns the library-code handling hints, so the maintained consumer
branch does not teach users to bring LinuxDesktop2026 report types or diagnostic
catalogs into product APIs just to surface warnings, prompts, or errors.

Task 50 gives `ld_root` one portable-root request shape for app-owned roots
selected by explicit product policy, command-line switches, or marker files.
`ld_settings` forwards that request only as a convenience for settings lifecycle
call sites. C++ users no longer have to model a product's portable mode as a
generic app-root override plus separate marker policy, while the C ABI keeps its
existing names and maps them into the C++ request internally.

Task 51 splits executable plugin ecosystems from plugin-adjacent asset/library
ecosystems. `ld_paths` owns built-in executable plugin kinds for LADSPA, DSSI,
LV2, VST2, VST3, CLAP, Audio Unit, AAX, and JSFX; SF2 and SFZ move to asset
path kinds. Product-owned, toolkit-owned, and investigation-only ecosystems use
named plugin path sets with category, extension, platform, environment, and
default-root metadata instead of growing the built-in enum. CtrlrX now uses the
settled VST3/Audio Unit/AAX path API, and tests include a Qt side-by-side
counterexample.

Task 52 makes invasive hardening tests an explicit posture for the active
modules. The normal suite treats generated file content, directory lifecycle,
cleanup after partial failures, C ABI nested ownership, cross-module diagnostic
propagation, root/path selection explanation, and migration before/after action
traces as caller-visible behavior. The posture is recorded in ADR 0014 and
stays local, deterministic, and fast enough for normal CTest runs.

Task 53 completes the adversarial follow-through for that posture. The normal
suite now exercises malformed root requests, hostile product-owned plugin path
sets, desktop integration identifier sanitization, malformed integration output
roots, migration destination collisions, and hostile Registry snapshot imports.
Rejected or degraded operations keep diagnostics so product adapters can
translate failures instead of reverse-engineering silent normalization.

Spec: `specs/platform-path-defaults.md`
Spec: `specs/root-module-boundary.md`
ADR: `docs/adr/0014-adopt-invasive-and-adversarial-hardening-tests.md`

## Review Checklist

- Public-facing status claims should point to one current ledger instead of
  repeating branch, remote, CI, or maintenance details in multiple docs.

## Review-Derived Hardening Batch

The September 2026 external review opened a new hardening batch after the
original FlavorTest/root-path sequence. The batch keeps confirmed defects ahead
of broad cleanup: first reconcile the documentation and dependency graph so the
ledger is trustworthy, then fix the two live watcher lifecycle defects, then
run the wider watcher discovery needed before adding concurrency CI evidence.

The settings fixture work is deliberately after the watcher lifecycle fixes. It
is important, but it depends on the module-boundary cleanup more than it blocks
the immediate process-termination and lost settled-file readiness risks. The
migration ticket stays in the same batch because the review found a real
promise gap, but it is ordered after the watcher and settings evidence unless
current consumers make migration semantics urgent.

Task 58 completed the focused watcher lifecycle and settled-file readiness
discovery pass. Fresh debug builds covered deterministic simulated-backend
tests, native Linux `inotify` smoke tests, and libuv-preferred watcher tests.
All focused watcher tests passed locally. The pass did not find a blocking
runtime defect, but it did expose repeated settled-file option aggregate
initializer warnings in `tests/watch_tests.cpp`; task 61 tracks that cleanup
after the currently ordered migration hardening work.
