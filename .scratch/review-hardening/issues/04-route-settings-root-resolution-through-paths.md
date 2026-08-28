# 04 — Route Settings Root Resolution Through Paths

**What to build:** `ld_settings` should use `ld_paths` for generic root and path selection while preserving the observable settings behavior callers and tests already rely on.

**Blocked by:** None - ADR 0012 settled the module boundary.

**Status:** done

- [x] Settings root resolution delegates generic config/data/state/cache/resource/runtime path selection to `ld_paths`.
- [x] Existing settings tests continue to pass or are updated only to reflect the documented boundary decision.
- [x] Pre-1.0 C++ breakage is allowed when needed to remove the wrong module boundary; document replacement guidance for any source break.

## Problem Statement

`ld_settings` still contains generic root and path selection logic even though
ADR 0012 makes `ld_paths` the owner of generic path policy. That duplication
keeps the wrong module boundary alive: settings callers can get useful path
behavior from the settings module, but the implementation hides where that
policy really belongs.

## Solution

Route the generic root selection used by `ld_settings` through `ld_paths` while
preserving the settings-facing root report and current settings-specific
behavior. `ld_settings` remains responsible for portable settings overlays,
sync-config overrides, session roots, plugin config roots, named roots,
component roots, config layers, and the decision to create directories.
Runtime is a first-class generic path family in `ld_paths`, not extraction debt
left behind in settings.

The public settings root API should continue to work for this ticket. The
implementation change should make `ld_paths` diagnostics visible through
settings reports so callers can see which module produced path-resolution
facts.

## User Stories

1. As an application developer, I want `ld_settings` root resolution to follow
   the same generic path policy as `ld_paths`, so that config/data/state/cache
   placement does not drift between modules.
2. As an application developer, I want my existing settings root calls to keep
   working, so that a boundary cleanup does not force an unrelated migration.
3. As an application developer, I want settings override behavior to remain
   settings-specific, so that one explicit settings root can still control the
   settings bundle.
4. As an application developer, I want portable marker behavior to remain
   settings-specific, so that portable settings continue to work while generic
   path policy moves out.
5. As an application developer, I want sync-config overrides to keep state and
   sessions machine-local, so that roaming config does not accidentally move
   runtime state.
6. As an application developer, I want session and plugin config roots derived
   from the final settings roots, so that existing root reports remain useful.
7. As an application developer, I want named roots and component roots to keep
   resolving from the settings root report, so that higher-level settings
   layouts do not need to learn `ld_paths` internals.
8. As a maintainer, I want `ld_paths` diagnostics to pass through settings
   reports unchanged, so that ownership of generic path facts is observable.
9. As a maintainer, I want directory creation to use the generic directory
   helper where practical, so that filesystem mutation semantics do not drift.
10. As a maintainer, I want focused tests around the public settings resolver,
    so that refactoring internals does not freeze the wrong implementation
    shape.
11. As a maintainer, I want adversarial environment or filesystem cases
    included when useful, so that relative paths, missing homes, and
    path-as-file failures are checked through public behavior.
12. As a future `ld_desktop` or `ld_migration` implementer, I want settings
    root resolution to depend on `ld_paths`, so that later module extraction
    starts from a cleaner boundary.
13. As an application developer, I want runtime roots to be resolved by
    `ld_paths`, so that runtime does not remain a settings-owned special case.
14. As a test author, I want settings root resolution to accept injected home
    and environment values, so that tests can exercise path policy without
    mutating process-global state.

## Implementation Decisions

- `ld_settings` will consume `ld_paths` for default config, data, state, cache,
  resource, and runtime root selection.
- `ld_paths` will add runtime as a path family with an explicit runtime
  override. C enum values should append runtime rather than renumber existing
  families.
- `ld_settings` will preserve its public root API for this ticket.
- `ld_settings` root options will add settings-owned environment injection
  controls: home directory, environment map, and process-environment opt-in.
  The C ABI mirrors those controls with settings-owned C structs rather than
  exposing `ld_paths` C types.
- Settings-specific overlays remain in `ld_settings`: settings override,
  portable marker handling, sync-config override handling, session root,
  plugin config root, named roots, component roots, and layer reporting.
- `settings_override` remains a settings overlay and does not feed `ld_paths`
  as a generic override.
- `ld_paths` diagnostics should be passed through unchanged rather than
  translated into settings-only codes.
- Directory creation remains controlled by the settings `create_directories`
  option, but generic directory creation should reuse the `ld_paths` directory
  helper where practical.
- `ld_settings` should gain only the dependency it needs on `ld_paths`; because
  the settings headers do not expose `ld_paths` types for this ticket, that
  dependency should be private.
- Pre-1.0 C++ source breakage remains allowed by policy, but this ticket should
  avoid it unless preserving the API would keep the wrong module boundary.

## Testing Decisions

- Test the highest existing seam: the public settings root resolver.
- Keep existing settings root tests passing unless a test encodes the old module
  ownership rather than intended behavior.
- Add focused coverage showing that a generic path-resolution diagnostic from
  `ld_paths` is visible through the settings root report.
- Include adversarial tests when they prove external behavior through the public
  API, especially relative environment paths, missing home-directory behavior,
  or directory creation failures.
- Add runtime coverage in `ld_paths` tests and settings delegation coverage for
  injected home/environment values.
- Use the existing settings and paths resolver tests as prior art.

## Out of Scope

- Extracting desktop integration effects into `ld_desktop`.
- Extracting migration planning or execution into `ld_migration`.
- Exposing `ld_paths` types in the settings public API.
- Broad portability matrix expansion beyond tests needed for this task.
- Stable C ABI redesign.

## Further Notes

This ticket is the code follow-through for ADR 0012's boundary decision. It
should make the ownership change real without making task 04 carry the full
weight of the later `ld_desktop` and `ld_migration` extraction work.
