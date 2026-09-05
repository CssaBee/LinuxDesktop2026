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
The watcher ThreadSanitizer lane is a separate Ubuntu/Clang job that builds only
the deterministic `ld_watch_tests` hardening target with the optional libuv
backend disabled. That lane is race evidence for the portable watcher
lifecycle, queue, callback, and settled-file code; it does not replace the
ASan/UBSan suite.

## Watcher Coverage

The default Linux lanes exercise the native inotify path. The separate libuv
workflow lane keeps the optional preferred-libuv backend buildable and tested
when libuv is available.

Windows CI runs the Windows watcher smoke target explicitly. That proves the
hosted-runner shape, not every filesystem corner case on supported Windows
versions.

## Adversarial Watcher Evidence

The ordinary `ld_watch_tests` CTest target is the first adversarial watcher
hardening target. It uses the simulated backend so CI can deterministically run
callback and lifecycle cases without relying on platform event timing.

| Case | Ordinary CTest | ThreadSanitizer CI | Backend CI |
| --- | --- | --- | --- |
| Callback calls `stop` and `remove_watch` | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Callback releases the last watcher facade owner | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Callback replacement affects later events | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Callback exception fallback to queued error event | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Pull queue overflow emits degraded rescan guidance | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Raw delivery while settled-file work is pending | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Remove-watch cancels pending settled-file delivery | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Stop cancels pending settled-file delivery | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Repeated add/remove/start leaves watcher usable | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Removed settled watch does not stop future settlement | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Stale settled generation does not stop future settlement | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Large settled-file batch preserves every path | `ld_watch_tests` | `ld_watch_tests` | Not backend-specific |
| Native Linux event mapping, recursion, replace, churn | `ld_watch_inotify_tests` on Linux | Not isolated yet | Main Linux matrix |
| Preferred libuv backend smoke | `ld_watch_libuv_tests` when libuv is preferred | Not isolated yet | `watcher-libuv` |
| Native Windows watcher smoke | `ld_watch_windows_tests` on Windows | Not available in hosted CI | Main Windows matrix |

Still-discovery cases from task 58 remain outside the sanitizer lane until they
are deterministic enough for CI: stop while a real backend is shutting down,
queue overflow during concurrent watch removal, repeated add/remove/start stress
across real backends, pull/callback mode switching under backend churn, and
head-of-line blocking when one file never stabilizes.

## Maintained Consumer Proof

The manually dispatched `.github/workflows/notepadpp-proof.yml` workflow builds the
`CssaBee/LinuxDesktop2026-crossport-notepadpp` proof branch against the current
LinuxDesktop2026 checkout as an installed CMake package. This is the smallest
repeatable integration slice for task 15:

- LinuxDesktop2026 is installed into a workflow-local prefix.
- The Notepad++ proof branch consumes the exported package target.
- The proof target builds and runs its CTest scenario.

The proof repository is private. Configure `LD2026_CROSSPORT_READ_TOKEN` with
read access to `CssaBee/LinuxDesktop2026-crossport-notepadpp` before treating
this workflow as observable CI evidence.

This lane is evidence for package consumption, include/link propagation, and
settings API ergonomics. It is not evidence for full Notepad++ native Linux UI
parity, plugin ABI compatibility, printing, shell integration, or broad file
watching.

The 2026-09-05 manually dispatched run is the first observed green proof run:
it checked out LinuxDesktop2026 `cf7de44f92a35b18add35529a58d8598b9c80321` and
crossport `a296934feedbae187fcd98981637bc45f8faceb5`, installed the package,
built the proof target, and passed CTest 1/1 on Ubuntu 24.04/GCC 13.3.

Keep it manual while the proof branch is private. Promote it to a scheduled or
pull-request lane if it stays stable enough to be useful.

## How To Read Failures

- A compiler-only failure means the public headers or exported target shape are
  not portable enough.
- A shared-library-only failure means the implementation or install/export
  rules are relying on static-link assumptions.
- A sanitizer failure blocks confidence in the affected module until reproduced
  or explained.
- A watcher ThreadSanitizer failure blocks confidence in callback, settlement,
  queueing, or shutdown lifecycle behavior until reproduced or explained.
- A consumer-proof failure should be recorded in
  `docs/consumer-branches/notepadpp-settings-proof.md` before new public API
  vocabulary is added.
