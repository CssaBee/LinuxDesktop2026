# 75 - Split Migration Internals By Responsibility

**What to build:** Keep the public `ld_migration` facade while separating
planning, filesystem execution, Registry compatibility, and rollback internals.

**Blocked by:** 71 - Replace Or Formally Scope Migration JSON Parser; 72 -
Formally Scope Reg File Compatibility.

**Status:** pending

- [ ] Move migration planning code into a focused internal source/unit.
- [ ] Move filesystem action execution and rollback reporting into focused
  internal sources.
- [ ] Move Registry snapshot/import/export compatibility into focused internal
  sources.
- [ ] Preserve the public C++ API and current tests during the internal split.

## Review Anchor

The newer review flags `migration.cpp` as nearly 2,000 lines spanning both
filesystem migration and Registry serialization/mutation, which are different
engineering domains.
