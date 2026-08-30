# Notepad++ Settings Proof Branch

This is the evidence ledger for the first maintained consumer branch.

## Branch Contract

- External repository name: `LinuxDesktop2026-crossport-notepadpp`
- Branch name: `linuxdesktop2026-settings-proof`
- Application base: upstream-shaped Notepad++ source
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

No maintained-branch build evidence has been recorded yet. The current in-tree
Notepad++ FlavorTest is only a preparation signal; it does not satisfy this
ticket by itself.

## API Pain Log

No maintained-branch pain points have been recorded yet.
