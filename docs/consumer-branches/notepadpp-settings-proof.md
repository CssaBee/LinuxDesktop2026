# Notepad++ Settings Proof Branch

This is the evidence ledger for the first maintained consumer branch.

## Current Status

- Status: active maintained-consumer proof.
- External repository: private GitHub repository
  `https://github.com/CssaBee/LinuxDesktop2026-crossport-notepadpp.git`
- Local checkout: `../LinuxDesktop2026-crossport-notepadpp`
- Branch: `linuxdesktop2026-settings-proof`
- Tracking: `origin/linuxdesktop2026-settings-proof`
- Current proof commit: `a296934feedbae187fcd98981637bc45f8faceb5`
- Current observed CI: the 2026-09-05 manually dispatched
  `Notepad++ Proof Branch` workflow passed CTest 1/1 against LinuxDesktop2026
  `cf7de44f92a35b18add35529a58d8598b9c80321` on Ubuntu 24.04/GCC 13.3.
- Current maintenance posture: keep recording rebase, dependency, include/link,
  compile, and API-friction evidence here while the proof branch is maintained.

## Branch Contract

- External repository name: `LinuxDesktop2026-crossport-notepadpp`
- Local checkout: `../LinuxDesktop2026-crossport-notepadpp`
- Remote: private GitHub repository
  `https://github.com/CssaBee/LinuxDesktop2026-crossport-notepadpp.git`
- Local tracking state: `linuxdesktop2026-settings-proof` tracks
  `origin/linuxdesktop2026-settings-proof`
- Branch name: `linuxdesktop2026-settings-proof`
- Application base: upstream-shaped Notepad++ source
- Initial upstream base commit: `c057c0802`
- LinuxDesktop2026 dependency mode: normal CMake consumption
- Initial LinuxDesktop2026 modules allowed: `ld_core`, `ld_paths`,
  `ld_settings`, and `ld_migration`
- Public C ABI expansion: out of scope until release-candidate status

The branch should prove that a real Notepad++-shaped settings subsystem can use
the current libraries without moving Notepad++ policy into LinuxDesktop2026 and
without adding a product-side adapter layer that exists only to compensate for
unclear library vocabulary.

## Required Product Boundaries

Keep in Notepad++:

- XML schemas, parsing, serialization, and validation
- command-line option parsing
- portable marker policy
- cloud or sync location acceptance policy
- plugin configuration names and layout
- user-facing diagnostics
- session, shortcut, menu, and find-history models
- Windows compatibility behavior for existing Windows builds

Use LinuxDesktop2026 for:

- settings/config root discovery after Notepad++ has supplied its identity and
  accepted overrides
- directory creation diagnostics
- shipped default/model file copying
- high-value config writes with validation-before-commit
- explicit migration plans where Notepad++ has already decided the product
  migration is allowed

## Validation Gates

The branch is not considered validated until all of these are true:

- It builds from a clean checkout on the target Linux development environment.
- It can be rebased across at least one non-trivial LinuxDesktop2026 API change.
- Include paths and transitive link dependencies are supplied by the exported
  targets rather than local patching.
- At least one real settings-root flow and one validated-write flow execute
  through LinuxDesktop2026.
- API pain points are recorded below before new public vocabulary is added.

## Evidence Log

### 2026-08-30

- LinuxDesktop2026 commit: `1669268`
- Notepad++ base commit: `c057c0802`
- Dependency mode: not integrated yet
- Build result: not attempted yet
- API friction found: none yet
- Disposition: local proof repository created at
  `../LinuxDesktop2026-crossport-notepadpp` on branch
  `linuxdesktop2026-settings-proof`

The current in-tree Notepad++ FlavorTest is only a preparation signal; it does
not satisfy this ticket by itself.

### 2026-08-30 Task 15 Proof Target

- LinuxDesktop2026 commit: `83e9215`
- Cross-port branch commit: `5f6457cb5`
- Notepad++ base commit: `c057c0802`
- Dependency mode: installed/staged CMake package via
  `LinuxDesktop2026_DIR=../LinuxDesktop2026/build/task15-proof-package/lib/cmake/LinuxDesktop2026`
- Build result: `cmake --build ... --target
  linuxdesktop2026_notepadpp_settings_proof` passed on local Linux/GCC 13.3
- Test result: `ctest --test-dir build --output-on-failure` passed, 1/1
- Flows exercised: Notepad++-named settings root resolution, session root
  resolution, plugin-config root resolution, default XML model copying,
  backup-preserving validated config write, and dry-run legacy config migration
  planning
- Include/link evidence: proof target links only exported
  `LinuxDesktop2026::ld_settings` and `LinuxDesktop2026::ld_migration`; include
  paths and transitive dependencies came from the package targets
- API friction found: no blocking friction in this first pass; the
  `ldm::plan_copy` helper was the right shape for product code that should not
  specify kind/name manually
- Disposition: satisfies the first maintained-branch existence/build gate, but
  does not yet satisfy the rebase-cadence gate

### 2026-08-30 Backend Rewrite Pass

- LinuxDesktop2026 commit: `83e9215`
- Cross-port branch commit: `661f6f2e9`
- Notepad++ base commit: `c057c0802`
- Dependency mode: installed/staged CMake package via
  `LinuxDesktop2026_DIR=../LinuxDesktop2026/build/task15-proof-package/lib/cmake/LinuxDesktop2026`
- Build result: `cmake --build ... --target
  linuxdesktop2026_notepadpp_settings_proof` passed on local Linux/GCC 13.3
- Test result: `ctest --test-dir build --output-on-failure` passed, 1/1
- Rewrite result: the raw smoke target was replaced with a Notepad++-owned
  `NotepadPlusPlusSettingsBackend` proof layer
- Scenarios exercised: normal per-user settings, command-line settings
  directory winning over cloud settings, cloud settings while session state
  remains machine-local, `doLocalConf.xml` local config under an allowed install
  tree, and `doLocalConf.xml` rejection under a protected install tree
- API friction found: no LinuxDesktop2026 API change is required from this
  pass; the product backend does need to preserve the distinction between
  "local config marker requested" and "local config active" so a protected
  install fallback can be reported without treating settings load as failed
- Disposition: stronger task-15 evidence because the fork now contains
  product-shaped code instead of only a direct API smoke test

### 2026-08-30 CI Path Added

- LinuxDesktop2026 commit: pending commit for task 18
- Cross-port branch commit: `661f6f2e9`
- Notepad++ base commit: `c057c0802`
- Dependency mode: installed/staged CMake package in a manually dispatched
  GitHub Actions workflow
- Build result: manual workflow added but not yet observed on GitHub Actions
- Test result: manual workflow added but not yet observed on GitHub Actions
- CI path: `.github/workflows/notepadpp-proof.yml` checks out the private
  `CssaBee/LinuxDesktop2026-crossport-notepadpp` proof branch, installs the
  current LinuxDesktop2026 checkout, and runs
  `linuxdesktop2026_notepadpp_settings_proof`
- API friction found: none from the CI wiring itself
- Disposition: satisfies the documented maintained-consumer CI-path requirement;
  the branch still needs observed green CI and rebase-cadence entries before
  task 15 can close

### 2026-08-30 Platform Defaults Rebase

- LinuxDesktop2026 base commit: `27c7dc6`
- LinuxDesktop2026 local change: `ld_settings::root_options` now forwards
  `platform_defaults` to `ld_paths`
- Cross-port branch base commit: `caccfc273`
- Notepad++ base commit: `c057c0802`
- Dependency mode: fresh installed/staged CMake package at
  `/tmp/linuxdesktop2026-crossport-prefix`
- Build result: `cmake --build ... --target
  linuxdesktop2026_notepadpp_settings_proof` passed on local Linux/GCC 13.3
- Test result: `ctest --test-dir build --output-on-failure` passed, 1/1
- API friction found: the maintained proof needed the new
  `ld_settings::root_options::platform_defaults` pass-through so it could use
  the generated CMake helper without dropping from `ld_settings` to `ld_paths`
- Disposition: platform-default friction became a LinuxDesktop2026 API fix; the
  proof branch now uses generated consumer code instead of private path-default
  helpers or host environment injection

### 2026-08-31 Private Remote And Topology Proof

- LinuxDesktop2026 base commit: `9cc6900`
- Cross-port branch commits: `30fcfe004`, `e1e1ea2db`, `de576a462`
- Remote state: `origin` points at private GitHub repository
  `CssaBee/LinuxDesktop2026-crossport-notepadpp`, with
  `origin/linuxdesktop2026-settings-proof` at `de576a462`
- Notepad++ base commit: `c057c0802`
- Dependency mode: normal CMake package consumption from the proof branch
- Build result: not re-run for this ledger update
- Test result: not re-run for this ledger update
- Evidence added: the proof branch now uses generated platform defaults,
  consumes the `ld_root` topology shape for Notepad++ root behavior, and keeps
  user-facing diagnostic handling in product-owned Notepad++ proof code
- API friction found: the proof continued to move through LinuxDesktop2026 API
  reshaping without needing product-side replacement headers or private path
  helpers; observed CI remains unproven
- Disposition: the remote is no longer missing, but it is private. This
  satisfies private crossport existence evidence only; public release evidence
  still requires an observed green workflow run and later maintenance entries.

### 2026-09-05 Current Main API Drift Sync

- LinuxDesktop2026 base commit: `344b45c` plus local pre-1.0 compatibility
  cleanup working-tree changes
- Cross-port branch base commit: `de576a462`
- Cross-port branch commit: `a296934fe`
- Notepad++ base commit: `c057c0802`
- Dependency mode: freshly staged CMake package at
  `/tmp/linuxdesktop2026-current-main-prefix`
- Build result: `cmake --build build/current-main-proof --target
  linuxdesktop2026_notepadpp_settings_proof` passed on local Linux/GCC 13.3
- Test result: `ctest --test-dir build/current-main-proof
  --output-on-failure` passed, 1/1
- Drift fixed: the proof now uses the current `ld_root`
  `portable_root_request` API, reads `portable_root_requested` and
  `portable_root_active`, expects the current
  `portable-denied-privileged-install` diagnostic code, and uses
  config-default copying vocabulary instead of the removed hydration vocabulary.
- Disposition: local proof evidence is aligned with current main package
  consumption again. Observed green workflow evidence is still outstanding.

### 2026-09-05 Task 76 Maintained Proof Pass

- LinuxDesktop2026 commit: `af518aa`
- Cross-port branch commit: `a296934fe`
- Local tracking state: cross-port branch
  `linuxdesktop2026-settings-proof` is clean and tracks
  `origin/linuxdesktop2026-settings-proof`; the local remote-tracking ref is
  also at `a296934fe`.
- Remote state: `origin` is the private GitHub repository
  `https://github.com/CssaBee/LinuxDesktop2026-crossport-notepadpp.git`.
- Notepad++ base commit: `c057c0802`
- Dependency mode: freshly staged installed CMake package at
  `/tmp/linuxdesktop2026-task76-prefix`
- Build result: `cmake --build build/task76-proof --target
  linuxdesktop2026_notepadpp_settings_proof` passed on local Linux/GCC 13.3
- Test result: `ctest --test-dir build/task76-proof --output-on-failure`
  passed, 1/1
- Rebase/maintenance evidence: the proof has now survived later
  LinuxDesktop2026 API movement across platform defaults, root topology,
  portable-root request naming, diagnostic-code cleanup, config-default
  vocabulary, and the `ld_migration` internal split without local header
  patching or source vendoring.
- Include/link friction: none observed in this pass. The proof continued to
  consume exported CMake package targets.
- Dependency friction: none observed locally. The main-repo manual workflow now
  documents the token path for checking out the private crossport repository
  through `LD2026_CROSSPORT_READ_TOKEN`.
- Adapter churn: no proof adapter changes were needed for the task-75
  migration split; the split preserved the public package surface used by the
  proof.
- CI status: observed green in the 2026-09-05 manually dispatched
  `Notepad++ Proof Branch` GitHub Actions workflow. The run checked out
  LinuxDesktop2026 `cf7de44f92a35b18add35529a58d8598b9c80321` and crossport
  `a296934feedbae187fcd98981637bc45f8faceb5`, installed LinuxDesktop2026 as a
  Release CMake package on Ubuntu 24.04/GCC 13.3, built
  `linuxdesktop2026_notepadpp_settings_proof`, and passed CTest 1/1.
- Disposition: satisfies the stable private proof repository, observed CI, and
  later maintenance-pass evidence for task 76.

## API Pain Log

No blocking maintained-branch pain points have been recorded yet. The first
proof pass did reinforce that path-kind inference helpers such as
`ldm::plan_copy` are important for consumer ergonomics; forcing Notepad++ code
to fill action kind/name fields directly would have created avoidable product
adapter code.

The backend rewrite added one non-blocking product-mapping rule: consumers need
both requested and active state for portable/local settings. LinuxDesktop2026
now exposes this through `portable_root_requested` and `portable_root_active`,
so the fork records the rule in its backend rather than changing the library.

The platform-default rebase added one API fix: `ld_settings` now accepts
`platform_path_defaults` and passes them to `ld_paths`. This keeps consumers on
the settings-level API while still allowing CMake-generated OS defaults.

## Current Framework-Tax Snapshot

Measured against crossport commit
`a296934feedbae187fcd98981637bc45f8faceb5`.

- Adapter size: `proof/notepadpp_settings_backend.cpp` is 252 lines and
  `proof/notepadpp_settings_backend.hpp` is 109 lines. The proof harness adds
  258 lines, but it is test/evidence code rather than product adapter surface.
- LinuxDesktop2026 exposure: the product-facing header uses Notepad++
  vocabulary only. LinuxDesktop2026 headers and namespaces are confined to the
  backend implementation and CMake target wiring.
- Concept families the consumer currently has to understand: 5
  (`ld_core` diagnostics, CMake-generated platform defaults, `ld_root`
  topology, `ld_settings` default/write lifecycle, and `ld_migration` dry-run
  planning). `ld_watch` is not part of the current Notepad++ settings proof, so
  this snapshot does not measure watcher concept tax.
- Concrete LinuxDesktop2026 touchpoints in the adapter: `root::options`,
  `portable_root_request`, `portable_root_level`, `ownership_kind`,
  `app_identity`, named-root request helpers, `resolve_app_roots`,
  `find_named_root`, root report booleans/diagnostics,
  `settings::config_file`, `ensure_config_defaults`, `write_common_config`,
  config-write validation callback, `migration::plan_copy`, migration actions,
  `diagnostic`, `diagnostic_handling`, and classified product-diagnostic
  helpers.
- Reports/options constructed or consumed: one root options object, one
  portable-root request, one app identity, three named-root requests, three
  settings file descriptors in the current proof scenario, one defaults report,
  one write report, one migration plan, the top-level root report, and three
  named-root subreports.
- Platform branches in adapter code: 0 `#if` or platform-specific source
  branches under `proof/`. The generated platform defaults are the only
  platform-default integration point.
- Product policy kept outside LinuxDesktop2026: command-line `-settingsDir`
  acceptance, cloud-directory acceptance, `doLocalConf.xml` marker detection,
  protected-install policy, Notepad++ XML root validation, user-facing
  diagnostic vocabulary, and legacy-import intent.

Current conclusion: the concept tax is acceptable for a settings/root/migration
proof because it remains private to one backend and removes platform branching
from the product-shaped surface. Do not add broader convenience APIs from this
single proof alone. Reopen helper design only if another maintained proof also
duplicates named-root lookup names, root-option assembly, or report translation
in a comparable way.

## Versioned Settings Commit Evidence

Task 89 used the maintained Notepad++ proof and in-tree flavor tests as the
first real evidence for a versioned settings commit contract. Task 90
implemented that contract in `ld_settings` and updated the maintained
crossport proof to exercise the new API directly.

- `session.xml` is the first proof target. The proof already uses
  `read_file_version()` followed by `write_versioned()` with backup, durable
  write, and validation-after-write for session saves. Its stale-write proof
  confirms a newer independent `session.xml` image remains untouched and the
  validation callback is not entered for stale input. Losing or replacing a
  newer session image has high user impact, but merging session XML is Notepad++
  policy.
- `shortcuts.xml` is the second proof target. The proof writes shortcuts through
  `read_file_version()` followed by `write_versioned()`, validates the XML, and
  only refreshes the shortcut HMAC source after a successful commit. Its stale
  proof confirms rejection happens before HMAC-source refresh. Replacing a newer
  shortcuts file would make the subsequent HMAC bookkeeping describe the wrong
  overwrite, but shortcut parsing and HMAC policy are still product-owned.

Current implementation status: `ld_settings` now exposes the C++-only
`file_version_token`, `read_file_version()`, `missing_file_version()`, and
`write_versioned()` path. `write_with_backup()` and `write_common_config()`
remain unversioned helpers and still report
`settings-interprocess-lost-update-not-protected`.

Task 90 validation on 2026-09-05:

- LinuxDesktop2026: `cmake --build build --target ld_settings_tests` passed on
  local Linux/GCC 13.3.
- LinuxDesktop2026: `./build/ld_settings_tests` passed, including deterministic
  versioned-writer, missing-file token, stale `session.xml`, and stale
  `shortcuts.xml` cases.
- Crossport proof: staged LinuxDesktop2026 into
  `/tmp/linuxdesktop2026-task90-prefix`, configured
  `LinuxDesktop2026-crossport-notepadpp/build/task90-proof` against that
  package, built `linuxdesktop2026_notepadpp_settings_proof`, and passed CTest
  1/1.

Conclusion: the implemented proof supports stale-write rejection for
participating whole-file settings commits. It still does not justify library
merge callbacks, automatic retry/reread behavior, XML-specific merging, or a
general settings transaction system.
