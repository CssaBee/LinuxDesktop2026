# 78 - Pin FetchContent Example

**What to build:** Make README dependency examples reproducible by default.

**Blocked by:** None.

**Status:** pending

- [ ] Replace `GIT_TAG main` in the public FetchContent example with a tag or
  commit placeholder.
- [ ] Explain separately when following `main` is appropriate for development
  users.
- [ ] Check other docs/examples for mutable dependency instructions.

## Review Anchor

The newer review found the README advertises `GIT_TAG main`, which is risky for
a pre-1.0 project that intentionally allows breaking API changes.
