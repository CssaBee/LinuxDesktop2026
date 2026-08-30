# 15 — Validate APIs With One Maintained Consumer Branch

**What to build:** The current APIs should be exercised by at least one actual
maintained consumer branch before the project adds more public vocabulary or
module families outside the required `ld_desktop` and `ld_migration`
extractions.

**Blocked by:** 14 — Add Adversarial Parser And Filesystem Tests.

**Status:** maintained-branch-evidence-in-progress

- [x] The repository names the first maintained consumer target, branch shape,
  validation gates, and evidence ledger outside the local FlavorTests.
- [x] A small Notepad++ proof branch or another real maintained application
  branch uses the current libraries without product code adapting around library
  weaknesses.
- [ ] The branch is built regularly enough to expose merge/rebase friction,
  include/dependency propagation, compile impact, and mundane maintenance
  changes.
- [ ] A second real consumer remains the target before release-candidate
  confidence, but it is not required before starting the first maintained
  branch.
- [x] API pain points discovered by the maintained branch are recorded before
  any new public enums, structs, or modules are added.

Implementation note:

`docs/consumer-branches/README.md` and
`docs/consumer-branches/notepadpp-settings-proof.md` now define the first
maintained branch contract, the evidence cadence, and the validation gates. This
ticket remains open until the external branch records regular build, rebase,
dependency, compile, and API-friction evidence. The local cross-port branch now
contains a passing `linuxdesktop2026_notepadpp_settings_proof` target. The first
target landed at `5f6457cb5`; the product-shaped backend rewrite landed at
`661f6f2e9`. It configures against a staged LinuxDesktop2026 CMake package,
links exported targets, and exercises root resolution, model hydration,
validated config writes, migration planning, command-line override precedence,
cloud/session separation, and protected-install local config fallback.
