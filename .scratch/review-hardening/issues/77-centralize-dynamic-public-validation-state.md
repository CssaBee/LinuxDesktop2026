# 77 - Centralize Dynamic Public Validation State

**What to build:** Stop repeating fast-changing maintained-proof status across
public documents.

**Blocked by:** None.

**Status:** implemented

- [x] Make README describe maintained consumer validation as a tracked project
  status area, not as a detailed live claim.
- [x] Keep exact branch, remote, CI, and maintenance status in one status
  ledger.
- [x] Audit docs for duplicated claims about proof branches and public
  validation state.
- [x] Add a short review checklist item for public claim drift.

## Result

Current public status now says the private Notepad++ crossport exists, is
tracked locally, and has one observed green manual proof workflow. Exact branch,
remote, commit, CI, and maintenance details live in
`docs/consumer-branches/notepadpp-settings-proof.md`; README and project status
link there instead of repeating live evidence.

## Review Anchor

The newer review found a mismatch between `project-status.md` and README/public
rendering around the Notepad++ proof branch. Even if caching contributed, the
state is repeated in enough places to drift.
