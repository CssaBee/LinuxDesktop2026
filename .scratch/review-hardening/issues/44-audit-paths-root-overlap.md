# 44 - Audit Paths Root Overlap

**What to build:** Audit `ld_paths` for root-topology overlap, internal split
opportunities, and unjustified merge candidates after the `ld_root` boundary is
designed.

**Blocked by:** 42 - Design Root Module Boundary.

**Status:** ready-for-agent

- [ ] The audit separates generic path-family behavior from root-topology
  behavior.
- [ ] Plugin path sets, path-list parsing, directory creation, executable roots,
  and platform defaults are each classified as stay, split internally, or
  candidate for future module interaction.
- [ ] Any proposed merge into `ld_root` cites repeated FlavorTest or
  maintained-consumer evidence.
- [ ] The audit calls out cases where no move is justified.
- [ ] `docs/plan/ld-paths-roadmap.md` is updated only where the current roadmap
  is stale or ambiguous.
- [ ] No broad `ld_paths` public redesign is performed in this ticket.

## Problem Statement

`paths.cpp` is also large, but size alone does not prove a boundary problem.
`ld_paths` already owns several distinct public surfaces: app path resolution,
directory creation, path-list parsing, plugin path sets, executable/resource
roots, and platform defaults. The project needs to know what overlaps with the
proposed `ld_root` idea and what should simply remain in `ld_paths`.

## Result

Pending.
