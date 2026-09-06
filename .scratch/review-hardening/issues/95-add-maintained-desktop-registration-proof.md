# 95 - Add Maintained Desktop Registration Proof

**What to build:** Validate at least one real consumer integration that uses
`ld_desktop` registration effects and records the maintenance, CI, and API
friction evidence needed before treating the desktop surface as product-ready.

**Blocked by:** 94 - Add Desktop Flavor Registration Validation.

**Status:** implemented

- [x] Choose a consumer branch whose desktop registration needs are real enough
  to exercise staged artifacts, activation follow-up, and cleanup reporting.
- [x] Record branch, remote visibility, commit, CI status, rebase/maintenance
  state, and API friction in the maintained-consumer ledger.
- [x] Keep local Desktop Flavor fixtures labeled as ergonomics evidence, not as
  a substitute for maintained proof.
- [x] Reopen the desktop bundle or individual effect design only from observed
  consumer friction, not from hypothetical shell or installer features.

## Result

The private Notepad++ crossport now has a Notepad++-owned desktop registration
adapter that links `LinuxDesktop2026::ld_desktop` explicitly and keeps
LinuxDesktop2026 bundle reports out of the product-facing header. The proof
plans, applies, queries, and removes staged XDG artifacts for a Notepad++
launcher, icon, autostart entry, `text/plain` association/default intent,
activation follow-up, and cleanup rows.

The proof was validated locally against an installed task-95 LinuxDesktop2026
package at `/tmp/linuxdesktop2026-task95-prefix`; CMake build passed and CTest
passed 1/1. The upstream-following check fetched Notepad++ `upstream/master` at
`26afde31c`; `git merge-tree HEAD upstream/master` reported a clean automatic
merge for the committed proof branch, and the task-95 proof additions are
isolated to `proof/` plus crossport-owned CMake scaffolding.

No blocking `ld_desktop` API change was found. Bundle construction remains
verbose, but the verbosity is currently useful because it keeps staged
artifacts, live activation, and cleanup explicit.

## Evidence Fit

Maintained proof should catch this: the risk is an API that looks good in local
fixtures but costs too much to keep working in a real codebase.
