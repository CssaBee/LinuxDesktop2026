# Notepad++ Settings Proof Branch

This is the evidence ledger for the first maintained consumer branch.

## Branch Contract

- External repository name: `LinuxDesktop2026-crossport-notepadpp`
- Local checkout: `../LinuxDesktop2026-crossport-notepadpp`
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
- shipped default/model file hydration
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
  resolution, plugin-config root resolution, default XML model hydration,
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

## API Pain Log

No blocking maintained-branch pain points have been recorded yet. The first
proof pass did reinforce that path-kind inference helpers such as
`ldm::plan_copy` are important for consumer ergonomics; forcing Notepad++ code
to fill action kind/name fields directly would have created avoidable product
adapter code.

The backend rewrite added one non-blocking product-mapping rule: consumers need
both requested and active state for portable/local settings. LinuxDesktop2026
already exposes this through `portable_requested` and `portable_active`, so the
fork records the rule in its backend rather than changing the library.
