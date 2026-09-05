# 78 - Pin FetchContent Example

**What to build:** Make README dependency examples reproducible by default.

**Blocked by:** None.

**Status:** implemented

- [x] Replace `GIT_TAG main` in the public FetchContent example with a tag or
  commit placeholder.
- [x] Explain separately when following `main` is appropriate for development
  users.
- [x] Check other docs/examples for mutable dependency instructions.

## Result

The README FetchContent example now uses `<release-tag-or-commit-sha>` instead
of `main`, and the surrounding text says ordinary dependency builds should pin a
release tag or commit SHA. It separately scopes `main` tracking to active
LinuxDesktop2026 development, proof branches, or integration work ready to
absorb pre-1.0 breaking changes immediately.

`rg` found no other public `GIT_TAG main` instructions in README, docs, cmake,
examples, tests, or the active issue set.

## Review Anchor

The newer review found the README advertises `GIT_TAG main`, which is risky for
a pre-1.0 project that intentionally allows breaking API changes.
