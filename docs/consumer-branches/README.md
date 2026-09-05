# Maintained Consumer Branches

Maintained consumer branches are the step after local FlavorTests. They are
buildable branches or patch series against an upstream-shaped application tree
that consume LinuxDesktop2026 as an ordinary dependency.

They are required because FlavorTests can prove call-site shape, but they do not
prove rebase cost, include propagation, dependency friction, compile-time cost,
or whether product code starts adapting around library weaknesses.

## First Target

The first maintained branch target is a narrow Notepad++ settings proof:

- default repository name: `LinuxDesktop2026-crossport-notepadpp`
- default branch name: `linuxdesktop2026-settings-proof`
- scope: settings-root selection, config-bundle hydration, validated backup
  writes, and config/session persistence only
- out of scope: GUI toolkit rewrites, plugin ABI, printing, shell integration,
  broad file watching, and complete native Linux parity

This branch must consume LinuxDesktop2026 through normal CMake integration
(`FetchContent`, `add_subdirectory`, or installed `find_package`). It should not
copy library source into the application, fork public headers locally, or add a
product-side adapter whose only job is to paper over unclear LinuxDesktop2026
types.

## Evidence Cadence

A maintained branch is useful only if it keeps moving. Record an entry in the
target evidence file whenever the branch is rebased, rebuilt, or forced to adapt
to a LinuxDesktop2026 API change.

Each entry should include:

- date
- LinuxDesktop2026 commit
- upstream application commit or branch base
- dependency mode
- build result
- API friction found
- whether the friction stayed in product code, became a library fix, or caused
  a planned library change

Do not add new broad public enums, structs, modules, or report fields merely
because the branch exposes one awkward call site. One consumer branch is enough
to reveal friction; repeated pressure from at least two real consumers is the
default threshold for new public vocabulary before release-candidate status.

## Current Evidence

- `notepadpp-settings-proof.md`: first maintained consumer target and evidence
  ledger. This is the source of truth for exact branch, remote, commit, CI, and
  maintenance state.

## CI Path

The main repository owns the smallest repeatable consumer integration path in
the manually dispatched `.github/workflows/notepadpp-proof.yml` workflow. It
checks out the current Notepad++ proof branch, installs the current
LinuxDesktop2026 tree into a workflow-local CMake package prefix, then
configures, builds, and tests the proof target against the exported package.

This lane is a release-evidence input, not a full product-port claim.

Because the crossport repository is private, the workflow should have
`LD2026_CROSSPORT_READ_TOKEN` configured with read access to the proof
repository. Keep the exact repository and branch state in
`notepadpp-settings-proof.md`.
