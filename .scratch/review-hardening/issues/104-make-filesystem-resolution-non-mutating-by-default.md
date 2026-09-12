# 104 - Make Filesystem Resolution Non-Mutating By Default

**What to build:** Make path, root, and settings resolution preview-only unless
callers explicitly request directory creation or another filesystem mutation.

**Blocked by:** None.

**Status:** implemented

- [x] Inventory public resolution APIs in `ld_paths`, `ld_root`, and
  `ld_settings` for defaults that create directories or otherwise mutate the
  filesystem.
- [x] Keep `ld_paths` resolution non-mutating and keep `ensure_directory` as an
  explicit opt-in mutation helper.
- [x] Change `ld_settings` root resolution defaults so resolving roots does not
  create directories unless callers opt in.
- [x] Change `ld_root` named-root request/builder defaults so root resolution
  does not create directories unless callers opt in.
- [x] Preserve explicit create/apply helpers for callers that want the old
  behavior, with diagnostics that distinguish preview, would-create, created,
  and failed states.
- [x] Add focused tests proving default resolution leaves the filesystem
  untouched and opt-in creation still works.
- [x] Document any pre-1.0 behavior break in release notes or migration
  guidance before tagging `0.2.0`.

## Implementation Notes

`ld_paths` already resolved without mutation and kept directory effects behind
`ensure_directory()`. `ld_root` and `ld_settings` now default
`create_directories` to false in C++ and C option initializers, and named-root
factory helpers default their per-root `create` flag to false. Callers that want
the previous behavior can set `create_directories = true` and opt in named roots
with `create = true`. Focused C++ tests cover preview defaults and explicit
creation.

## Evidence Fit

Adversarial tests plus public API review should catch this: the risk is a
function named like resolution unexpectedly creating user-visible directories
or other filesystem state.

## Release Gate

Resolved for `0.2.0`.
