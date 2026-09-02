# 54 - Reconcile Review Claim And Module Boundary Docs

**What to build:** Bring public documentation, status ledgers, ADRs, package
expectations, and the actual CMake dependency graph back into agreement after
the review found that `ld_settings` still publicly links `ld_desktop` while the
docs say desktop effects moved out.

**Blocked by:** None.

**Status:** implemented

- [x] Audit README, `docs/project-status.md`, ADR 0012, extraction plans,
  examples, CMake package notes, and the ticket ledger for claims that
  `ld_settings` no longer owns or exposes desktop/migration behavior.
- [x] Remove `LinuxDesktop2026::ld_desktop` from the `ld_settings` public
  dependency graph, or explicitly document any remaining dependency as a
  temporary blocker with an owner and removal ticket.
- [x] Add a build or install-tree check that proves a minimal `ld_settings`
  consumer does not transitively require `ld_desktop` headers, targets, or
  future desktop backend dependencies.
- [x] Update the status ledger so documentation confidence reflects observable
  implementation state, not intended architecture.

## Implementation Notes

`ld_settings` now links publicly only to `ld_core` and `ld_root`; `ld_root`
continues to carry the settings/root dependency on `ld_paths`. The stale
`linuxdesktop/desktop.hpp` include was removed from `src/settings.cpp`.

The install-tree consumer fixture now builds `ld_settings_consumer` from
`linuxdesktop/settings.hpp` alone and links only
`LinuxDesktop2026::ld_settings`. Its CMake configure step also fails if
`LinuxDesktop2026::ld_settings` exposes `LinuxDesktop2026::ld_desktop` through
`INTERFACE_LINK_LIBRARIES`. Path-default generation moved to a separate
`ld_paths_consumer`, so the package still proves generated path defaults without
hiding settings dependency leaks behind unrelated linked targets.

Validation on Linux/GCC 13.3:

- `cmake -S . -B build-task54 -DLD2026_BUILD_TESTS=ON`
- `cmake --build build-task54 --parallel 2`
- `ctest --test-dir build-task54 -R '^ld_settings_install_tree_consumer$' --output-on-failure`
- `ctest --test-dir build-task54 -R '^ld_settings(_c)?_tests$' --output-on-failure`

The installed export records
`LinuxDesktop2026::ld_settings` with
`INTERFACE_LINK_LIBRARIES "LinuxDesktop2026::ld_core;LinuxDesktop2026::ld_root"`.

## Review Anchor

The external review called this a confirmed high-severity boundary leak:
`ld_settings` is documented as settings/config only, but
`target_link_libraries(ld_settings PUBLIC ... ld_desktop ...)` still makes
desktop integration part of the public consumer graph.
