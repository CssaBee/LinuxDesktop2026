# 80 - Decompose Root CMake Before Next Module Wave

**What to build:** Split build definitions by module before the top-level CMake
file becomes the architecture registry.

**Blocked by:** None.

**Status:** pending

- [ ] Move library target definitions into module-focused CMake files.
- [ ] Move tests into a focused tests CMake file.
- [ ] Keep package/export orchestration and global options at the repository
  root.
- [ ] Preserve install-tree consumer behavior and CI target names.

## Review Anchor

The newer review says the root CMake file is manageable now, but will become a
maintenance problem if module count or test count keeps growing in one file.
