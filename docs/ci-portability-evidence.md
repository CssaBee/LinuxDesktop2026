# CI Portability Evidence

CI is a release-readiness signal only when it proves more than one happy-path
compiler and operating-system combination. The current matrix is intentionally
small, but each lane has a specific job.

## Main CMake Matrix

The main workflow in `.github/workflows/ci.yml` covers:

- Ubuntu latest, GCC, Debug, static libraries.
- Ubuntu latest, Clang, Release, shared libraries.
- Ubuntu 22.04 container, GCC, Release, static libraries.
- Fedora latest container, GCC, Debug, shared libraries.
- Windows latest, MSVC, Debug, static libraries.
- Windows latest, MSVC, Release, static libraries.

Shared-library builds are Linux-only for now. Windows shared libraries need an
explicit symbol-export policy and should stay out of routine CI until the
project is ready to make that decision.

The sanitizer workflow runs ASan/UBSan on Ubuntu with both GCC and Clang.

## Watcher Coverage

The default Linux lanes exercise the native inotify path. The separate libuv
workflow lane keeps the optional preferred-libuv backend buildable and tested
when libuv is available.

Windows CI runs the Windows watcher smoke target explicitly. That proves the
hosted-runner shape, not every filesystem corner case on supported Windows
versions.

## Maintained Consumer Proof

The manually dispatched `.github/workflows/notepadpp-proof.yml` workflow builds the
`CssaBee/LinuxDesktop2026-crossport-notepadpp` proof branch against the current
LinuxDesktop2026 checkout as an installed CMake package. This is the smallest
repeatable integration slice for task 15:

- LinuxDesktop2026 is installed into a workflow-local prefix.
- The Notepad++ proof branch consumes the exported package target.
- The proof target builds and runs its CTest scenario.

This lane is evidence for package consumption, include/link propagation, and
settings API ergonomics. It is not evidence for full Notepad++ native Linux UI
parity, plugin ABI compatibility, printing, shell integration, or broad file
watching.

Keep it manual until the proof branch exists on GitHub and has at least one
observed green run. After that, promote it to a scheduled or pull-request lane
if it stays stable enough to be useful.

## How To Read Failures

- A compiler-only failure means the public headers or exported target shape are
  not portable enough.
- A shared-library-only failure means the implementation or install/export
  rules are relying on static-link assumptions.
- A sanitizer failure blocks confidence in the affected module until reproduced
  or explained.
- A consumer-proof failure should be recorded in
  `docs/consumer-branches/notepadpp-settings-proof.md` before new public API
  vocabulary is added.
