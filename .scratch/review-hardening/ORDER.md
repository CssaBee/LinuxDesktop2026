# Review Hardening Ticket Order

This order is the active remaining implementation sequence after the Flavor
review round. Ticket numbers remain historical; execution order follows the
list below.

## Current Order

- `56` - Fix Watch Settle Worker Permanent Exit
- `57` - Fix Watch Callback Self-Destruction Lifecycle
- `58` - Run ld_watch Lifecycle And Settlement Discovery
- `59` - Add Adversarial CI And ThreadSanitizer Evidence
- `55` - Add Settings Root Resolution Multi-Filesystem Fixtures
- `60` - Narrow And Harden Migration Filesystem Semantics

## Implemented

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

The next pass resolves remaining FlavorTest product-boundary leaks before
treating the current APIs as ready for maintained consumer validation. Migration
result translation, the common write audit, the experimental root-request
builder pass, and the maintained-branch contract are complete, so the sequence
can move to broader portability evidence while the first external branch is
created and kept building.

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

## Review-Derived Hardening Batch

The September 2026 external review opened a new hardening batch after the
original FlavorTest/root-path sequence. The batch keeps confirmed defects ahead
of broad cleanup: first reconcile the documentation and dependency graph so the
ledger is trustworthy, then fix the two live watcher lifecycle defects, then
run the wider watcher discovery needed before adding concurrency CI evidence.

The settings fixture work is deliberately after the watcher lifecycle fixes. It
is important, but it depends on the module-boundary cleanup more than it blocks
the immediate process-termination and lost-settlement risks. The migration
ticket stays in the same batch because the review found a real promise gap, but
it is ordered after the watcher and settings evidence unless current consumers
make migration semantics urgent.
