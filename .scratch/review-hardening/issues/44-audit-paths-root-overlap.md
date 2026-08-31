# 44 - Audit Paths Root Overlap

**What to build:** Audit `ld_paths` for root-topology overlap, internal split
opportunities, and unjustified merge candidates after the `ld_root` boundary is
designed.

**Blocked by:** 42 - Design Root Module Boundary.

**Status:** done

- [x] The audit separates generic path-family behavior from root-topology
  behavior.
- [x] Plugin path sets, path-list parsing, directory creation, executable roots,
  and platform defaults are each classified as stay, split internally, or
  candidate for future module interaction.
- [x] Any proposed merge into `ld_root` cites repeated FlavorTest or
  maintained-consumer evidence.
- [x] The audit calls out cases where no move is justified.
- [x] `docs/plan/ld-paths-roadmap.md` is updated only where the current roadmap
  is stale or ambiguous.
- [x] No broad `ld_paths` public redesign is performed in this ticket.

## Problem Statement

`paths.cpp` is also large, but size alone does not prove a boundary problem.
`ld_paths` already owns several distinct public surfaces: app path resolution,
directory creation, path-list parsing, plugin path sets, executable/resource
roots, and platform defaults. The project needs to know what overlaps with the
proposed `ld_root` idea and what should simply remain in `ld_paths`.

## Result

Completed in `specs/root-module-boundary.md` and reflected in the `ld_paths`
roadmap. The audit keeps generic path families, platform defaults, path-list
parsing, directory creation, typed plugin path sets, legacy/site candidates,
and executable/resource discovery in `ld_paths`. It does not justify merging
those surfaces wholesale into `ld_root`.

The justified breaking-change plan is narrower: the current public path result
shape uses `path_family::plugin_search` as a synthetic label for path-list and
plugin-set candidates even though plugin search sets are not selected
application path families. The roadmap now treats that as a pre-public cleanup
before C/Rust consumers or a future `ld_root` API inherit the muddled shape.
Executable, install-prefix, and resource values should also be clarified as
location/provenance values that `ld_root` can consume, not user-owned root
families.

Follow-up tickets were added for the expand-contract/breaking cleanup and
public root-topology promotion path.
