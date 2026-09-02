# 52 - Adopt Invasive Hardening Test Posture

**What to build:** turn invasive unit and integration testing into an explicit
hardening lane for the active write/path/root/desktop/migration modules.

**Blocked by:** 50 - Add Portable Root Request API; 51 - Harden Plugin Path
Kind Taxonomy.

**Status:** implemented

- [x] Inventory the current invasive coverage for `ld_settings`, `ld_root`,
  `ld_paths`, `ld_desktop`, and `ld_migration`.
- [x] Identify public entry points whose correctness depends on internal state
  transitions, generated files, cleanup behavior, C ABI ownership, or
  cross-module report shape.
- [x] Add tests that inspect generated side effects and lifecycle transitions
  directly, not only returned success values.
- [x] Extend to integration tests where a module boundary cannot be stressed
  honestly inside a single hermetic unit test.
- [x] Keep all tests deterministic, local, and fast enough for the normal
  hardening suite.

## Implementation Notes

An invasive test is not a license to assert incidental private structure. The
target is behavior that callers depend on but ordinary black-box happy-path
tests can miss: write ordering, rollback traces, cleanup after partial failure,
ownership transfer across the C ABI, report fidelity, generated desktop/dconf
content, and root/path selection explanations.

Implemented coverage keeps the invasive lane inside the normal CTest suite.
`ld_settings` already asserts common-write backup restoration, temporary-file
cleanup, readback mismatch recovery, parent file collisions, and forwarded
`ld_paths` diagnostics. `ld_paths` asserts selected candidates, generated
location roles, path-list diagnostics, plugin path set metadata, and directory
creation effects. `ld_root` asserts named/component root topology,
portable-root transitions, creation failures, and builder option transfer.
`ld_desktop` asserts generated autostart and dconf content, dry-run/no-write
behavior, permission gates, query/remove lifecycle, lock-file cleanup, and
sibling-file isolation. `ld_migration` asserts dry-run planning, copy/move
execution traces, dangerous-action gates, overwrite before/after state,
rooted-path delegation, Registry snapshot round trips, and import permission
denials. The root C ABI test covers nested component-root ownership and
free/reset semantics, not only the top-level paths.
