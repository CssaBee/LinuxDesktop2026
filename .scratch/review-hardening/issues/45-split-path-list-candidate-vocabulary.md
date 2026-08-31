# 45 - Split Path-List Candidate Vocabulary

**What to build:** Path-list parsing and plugin search-root reports should use
candidate vocabulary that describes path-list entries and plugin sets directly,
instead of reusing an application path-family candidate with a synthetic plugin
search family. This ticket takes the necessary pre-1.0 breaking change instead
of carrying compatibility scaffolding.

**Blocked by:** 44 - Audit Paths Root Overlap.

**Status:** ready-for-agent

- [ ] Path-list parse results expose entry candidates without requiring callers
  to treat them as selected application path families.
- [ ] Plugin path-set results expose rejected, duplicate, default, environment,
  and Wine-prefix entries without relying on a fake selected path family.
- [ ] `plugin_search` is removed from public application path-family
  vocabularies wherever it only existed to label path-list/plugin-set
  candidates.
- [ ] C and C++ callers retain clear owned-report/freeing rules after the result
  vocabulary changes.
- [ ] Existing examples and FlavorTests that inspect path-list or plugin-set
  diagnostics are migrated to the new vocabulary.
- [ ] No transitional duplicate fields, aliases, or compatibility adapters are
  left behind after tests and examples are migrated.
- [ ] API stability notes and friction notes call out the intentional break and
  the reason: plugin search roots are path sets, not application path families.

## Breaking Change

Remove the synthetic plugin-search path family from the public path-family
model and replace reused resolver candidates in path-list/plugin-set reports
with path-list or path-set candidate vocabulary. Because `ld_paths` is still
pre-public-prototype, this should be a direct cleanup, not expand-contract
compatibility debt.

## Dependency Boundary

After this ticket, users who only need path-list parsing or plugin search-root
sets bring in `ld_paths` only. They do not need `ld_root` or `ld_settings`, and
they do not have to understand application root topology to inspect path-list
diagnostics.
