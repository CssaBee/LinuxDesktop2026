# 77 - Centralize Dynamic Public Validation State

**What to build:** Stop repeating fast-changing maintained-proof status across
public documents.

**Blocked by:** None.

**Status:** pending

- [ ] Make README describe maintained consumer validation as a tracked project
  status area, not as a detailed live claim.
- [ ] Keep exact branch, remote, CI, and maintenance status in one status
  ledger.
- [ ] Audit docs for duplicated claims about proof branches and public
  validation state.
- [ ] Add a short review checklist item for public claim drift.

## Review Anchor

The newer review found a mismatch between `project-status.md` and README/public
rendering around the Notepad++ proof branch. Even if caching contributed, the
state is repeated in enough places to drift.
