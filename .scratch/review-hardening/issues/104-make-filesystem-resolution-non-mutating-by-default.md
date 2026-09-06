# 104 - Make Filesystem Resolution Non-Mutating By Default

**What to build:** Make path, root, and settings resolution preview-only unless
callers explicitly request directory creation or another filesystem mutation.

**Blocked by:** None.

**Status:** pending

- [ ] Inventory public resolution APIs in `ld_paths`, `ld_root`, and
  `ld_settings` for defaults that create directories or otherwise mutate the
  filesystem.
- [ ] Keep `ld_paths` resolution non-mutating and keep `ensure_directory` as an
  explicit opt-in mutation helper.
- [ ] Change `ld_settings` root resolution defaults so resolving roots does not
  create directories unless callers opt in.
- [ ] Change `ld_root` named-root request/builder defaults so root resolution
  does not create directories unless callers opt in.
- [ ] Preserve explicit create/apply helpers for callers that want the old
  behavior, with diagnostics that distinguish preview, would-create, created,
  and failed states.
- [ ] Add focused tests proving default resolution leaves the filesystem
  untouched and opt-in creation still works.
- [ ] Document any pre-1.0 behavior break in release notes or migration
  guidance before tagging `0.2.0`.

## Evidence Fit

Adversarial tests plus public API review should catch this: the risk is a
function named like resolution unexpectedly creating user-visible directories
or other filesystem state.

## Release Gate

Blocks `0.2.0`.
