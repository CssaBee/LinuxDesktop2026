# 46 - Separate Path Families From Location Roles

**What to build:** Application path reports should distinguish platform path
families from executable, install-prefix, and resource locations, so consumers
can ask for ordinary user roots without pretending that every location is the
same kind of root. This ticket takes the necessary pre-1.0 break so the final
API has no compatibility-shaped debt.

**Blocked by:** 45 - Split Path-List Candidate Vocabulary.

**Status:** implemented

- [x] Config, data, state, cache, runtime, temp, and user directories remain the
  ordinary platform path families.
- [x] Executable path, executable directory, install prefix, and resources are
  reported as location/provenance values with source-labeled candidates.
- [x] Those location/provenance values are removed from the ordinary public
  path-family enum/result map once examples and tests use the clearer result
  shape.
- [x] Walnut and OpenIPC Dashboard continue to read naturally through direct
  path resolution for their simple desktop/default profile cases.
- [x] Notepad++ and qBittorrent can pass install-adjacent/resource locations on
  to root topology without duplicating platform discovery.
- [x] The C ABI is updated to the same model without old aliases, duplicate
  selected-path entries, or compatibility shims.
- [x] Docs describe the minimal dependency rule: executable/resource discovery
  remains available from `ld_paths` alone, while named app-owned child roots are
  resolved by `ld_root` only when callers need that topology.

## Breaking Change

Remove executable, executable-directory, install-prefix, and resource entries
from the ordinary path-family result model. Replace them with explicit location
roles or an equivalent public shape that preserves source-labeled diagnostics
without calling them user path families.

## Dependency Boundary

After this ticket, users who only need platform path families or
executable/resource locations link only `ld_paths`. `ld_root` must consume
these values as inputs later; it must not become the only way to discover an
install-adjacent resource directory.
