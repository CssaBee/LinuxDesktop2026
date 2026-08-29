# Define settings module boundaries

`ld_settings` grew during the prototype phase because settings, paths,
Registry-shaped compatibility data, desktop integration effects, policy, and
migration behavior are tangled together in real Windows applications.

That prototype was useful evidence, but it is not the module boundary the
project should preserve.

## Decision

`ld_settings` owns application settings and configuration behavior:

- settings-specific root requests and reports,
- config bundle hydration from shipped model/default files,
- config layer reports and read/write ordering,
- settings-file writes, validation callbacks, backup reporting, and diagnostics,
- settings-specific named roots such as sessions, logs, profile data, plugin
  config, and component-local config/data/state roots.

Generic path/root policy belongs to `ld_paths`:

- config, data, state, cache, runtime, temp, resource, install, executable,
  document, desktop, download, media, template, public-share, and plugin search
  roots,
- platform root discovery through XDG, Windows Known Folders, environment
  variables, executable-relative paths, legacy candidates, and site defaults,
- path-list parsing, joining, and diagnostics,
- opt-in directory creation after resolution.

Desktop integration effects belong to the real `ld_desktop` extraction:

- autostart,
- desktop entries,
- icons,
- MIME/file associations,
- default applications,
- URL protocol handlers,
- shell context menus where practical,
- desktop database updates,
- managed/enforced desktop or application policy,
- Registry-equivalent behavior whose purpose is shell, policy, startup, or
  desktop/session integration.

Migration behavior belongs to the real `ld_migration` extraction:

- migration planning,
- file and directory copy/move execution,
- rollback and before/after reporting where practical,
- app-settings Registry snapshot/import/export compatibility when used to move
  application state rather than apply a desktop integration effect,
- cross-module migration orchestration once `ld_desktop` exists.

The Registry and migration APIs live in the owning modules rather than as a
standing `ld_settings` compatibility layer. Autostart and managed/enforced
policy C++ implementation lives in `ld_desktop`; migration
planning/execution and app-settings Registry snapshot/import/export
compatibility live in `ld_migration`. `ld_settings` does not expose namespace
bridges for those areas. None of those responsibilities are stable
`ld_settings` responsibilities.

Before `1.0`, C++ APIs may break when needed to correct module boundaries.
Those breaks must be documented with replacement paths. Existing C ABI entry
points remain best-effort compatible where practical, and C ABI expansion waits
until release-candidate status.

## Rationale

The original settings survey correctly found that applications need more than a
key/value store. They need root resolution, portable settings choices, config
layer explanation, first-run hydration, and careful writes.

The later prototype went further and proved that real Windows-to-Linux ports
also need Registry-shaped migration, autostart, policy, and desktop integration
operations. Those operations are important, but they do not have the same owner
as normal settings files.

Keeping everything under `ld_settings` would make the first module look useful
quickly while creating a project-killing boundary leak. Callers would learn to
reach for a settings library when they really need a path resolver, desktop
integrator, or migration engine. That would make API names, diagnostics, tests,
and future documentation increasingly misleading.

## Consequences

Task 03 is a documentation and architecture boundary decision. It does not move
code by itself.

Task 04 routes generic root resolution, including runtime, through `ld_paths`.
Task 05 records the split between settings, paths, desktop effects, and
migration, and reduces public claims where the current prototype lacks
hostile-input, rollback, permissions, Windows, or real-consumer evidence.
Task 19 introduces `ld_desktop`, moving the current C++ autostart and
managed/enforced policy implementation there. Task 20 introduces `ld_migration`,
moving the current C++ migration planning/execution and app-settings Registry
compatibility implementation there.

`ld_settings` does not keep a standing compatibility layer for those areas.
Since the project is still pre-1.0, source-breaking C++ cleanup is allowed when
a wrapper would preserve the wrong mental model.

The roadmap changes are real. `ld_desktop` and `ld_migration` exist as
extraction modules and are not speculative homes.

## Supersedes

This ADR supersedes the broad `ld_settings` ownership model in
`docs/plan/ld-settings-expanded-api.md`. That document remains useful as
prototype evidence and implementation inventory, but it is no longer the
controlling module-boundary plan.
