# Root Module Boundary Spec

## Problem Statement

`src/settings.cpp` is now large enough to hide multiple responsibilities behind
one implementation file. The current file contains settings root resolution,
portable and sync overlays, named and component roots, config-layer reporting,
default hydration, common write and backup mechanics, diagnostics glue, and
assorted filesystem helpers. Some of that belongs to `ld_settings`, some has
already moved conceptually to `ld_paths`, and some may deserve a public
`ld_root` module if repeated consumer evidence says applications need root
topology without the rest of the settings lifecycle.

This is not a tech-debt cleanup ticket. A split that only makes files smaller
would leave the public model as muddy as before. The hardening question is
whether LinuxDesktop2026 needs a root module that can express user-owned and
app-owned root layouts without forcing callers through either generic path
families or settings/config vocabulary.

`docs/FlavorTests/API_FRICTION.md` is the baseline. Notepad++, qBittorrent, and
KiCad show that a settings-oriented root helper can reduce mechanical setup.
KeePassXC and FreeCAD show that product-owned XDG/environment policy may read
better as direct requests. Walnut and OpenIPC Dashboard are negative evidence:
some products only need `ld_paths`, while service or web profile roots should
remain product-owned unless a repeated root-topology seam appears.

## Solution

Design `ld_root` as a proposed public boundary before extracting code into it.
The design must define the difference between:

- generic path families owned by `ld_paths`,
- application root topology potentially owned by `ld_root`,
- settings-specific root overlays and config lifecycle owned by `ld_settings`,
- product policy that must stay in adapters or applications.

Only after that boundary is documented should implementation move. The first
implementation pass should split `ld_settings` internals along those ownership
lines while preserving public behavior. A later public `ld_root` API should be
added only if the extracted boundary proves reusable across FlavorTests or a
maintained cross-port branch.

## User Stories

1. As an application developer, I want user-owned and app-owned roots to have a
   clear vocabulary, so that my product layout does not have to masquerade as
   settings configuration.
2. As an application developer, I want ordinary platform path discovery to stay
   in `ld_paths`, so that simple apps like Walnut do not need a heavier root
   topology API.
3. As an application developer, I want settings config lifecycle to stay in
   `ld_settings`, so that default hydration and safe writes remain discoverable
   from the settings module.
4. As an application developer, I want service/profile roots to remain product
   policy unless multiple products show the same reusable topology, so that
   OpenIPC-style deployment constraints are not flattened into a generic helper.
5. As a Notepad++ integrator, I want install roots, per-user config roots,
   session roots, plugin config roots, local config markers, command-line
   overrides, and cloud choices to remain easy to compose without exposing raw
   platform path details.
6. As a maintainer, I want the `settings.cpp` split to make ownership visible,
   so that future changes do not accidentally grow `ld_settings` again.
7. As a maintainer, I want a public `ld_root` module only if it has a distinct
   job from `ld_paths` and `ld_settings`, so that LinuxDesktop2026 does not add
   a module-shaped synonym.
8. As a maintainer, I want the `ld_paths` audit to distinguish merge candidates
   from unjustified expansion, so that generic path APIs stay small and honest.
9. As a FlavorTest reviewer, I want API friction notes to say when
   `root_request_builder` is positive evidence and when direct `ld_paths`
   remains the better fit.
10. As a future C or Rust binding author, I want root topology concepts to be
    flat and explicit if they become public, so that they do not depend on C++
    builder-only ergonomics.

## Implementation Decisions

- Treat `ld_root` as a proposed public module boundary, not a private cleanup
  name.
- Do not add a public `ld_root` API before the boundary spec and extraction
  evidence are complete.
- Preserve current public `ld_settings` behavior while splitting internals.
- Keep generic config/data/state/cache/runtime/resource placement and platform
  defaults in `ld_paths`.
- Keep settings override, sync-config override, portable marker handling,
  config layers, hydration, and settings writes in `ld_settings`.
- Consider `ld_root` only for reusable application root topology: named roots,
  component roots, user-owned roots, app-owned roots, install-adjacent roots,
  and root ownership reporting.
- Do not move service deployment policy, web profile layout, cloud prompt
  policy, file format ownership, or plugin ABI behavior into `ld_root`.
- Audit `ld_paths` separately for internal organization and unjustified overlap,
  but do not merge plugin path sets, path-list parsing, or directory creation
  into `ld_root` without repeated evidence.
- Use `API_FRICTION.md`, in-tree FlavorTests, and the Notepad++ cross-port proof
  as the evidence baseline.

## Testing Decisions

- Start with characterization tests around public `ld_settings` root behavior
  before moving implementation files.
- Preserve Notepad++, qBittorrent, KiCad, Walnut, and OpenIPC Dashboard
  FlavorTest behavior while changing internals.
- If `ld_root` becomes public, add public API tests before migrating
  FlavorTests to the new surface.
- Keep `ld_paths` tests focused on path-family resolution, candidate sources,
  platform defaults, path lists, plugin path sets, and directory creation.
- Do not treat a smaller `settings.cpp` line count as test evidence.

## Out of Scope

- Implementing `ld_root` public headers in the first design ticket.
- Moving settings hydration or safe writes into `ld_root`.
- Moving plugin search roots wholesale out of `ld_paths`.
- Replacing product-owned service/profile deployment rules.
- C ABI expansion before a public `ld_root` API is justified.
- Broad `ld_paths` public redesign without new FlavorTest or maintained
  consumer evidence.

## Further Notes

This spec follows the API friction baseline after the platform-defaults pass.
The accepted direction is public-boundary quality with no tech-debt module:
design the root boundary, split implementation seams only where ownership
becomes clearer, and let public API promotion wait for evidence.
