# `ld_desktop` Extraction Requirements

Status: required extraction before ship-candidate status.

`ld_desktop` owns platform actions that register an application with the
desktop, shell, session, or managed-policy environment. The current
`linuxdesktop::settings::effects` APIs and Registry-equivalent desktop/system
behavior are temporary pre-1.0 implementation locations in `ld_settings`.

## Scope

The extracted module must cover these responsibility groups before the current
`ld_settings` effect surface can be treated as resolved:

- autostart entries,
- desktop entries,
- icon installation and lookup metadata,
- MIME and file associations,
- default applications,
- URL protocol handlers,
- shell-equivalent behavior where practical,
- desktop database updates,
- managed and enforced desktop or application policy,
- Registry-equivalent behavior whose purpose is shell, startup, policy,
  session, or desktop integration.

## Required API Posture

- Use explicit capability reports for every backend.
- Keep filesystem and Registry mutation behind explicit permission flags.
- Default mutating operations to dry-run where practical.
- Report unsupported, sandbox-limited, permission-denied, backend-missing, and
  externally-updated-database states as diagnostics.
- Keep desktop/session concepts separate from settings payload concepts.
- Route generic path selection through `ld_paths` instead of embedding XDG,
  Known Folder, or home-directory policy.
- Treat Windows `CurrentVersion\Run`, `Software\Policies`, shell classes, and
  protocol handler equivalents as desktop/system effects, not settings storage.

## Validation Required

Before `ld_desktop` is a ship candidate, tests and examples must cover:

- Linux XDG Autostart write, query, disable, and remove paths,
- Linux `.desktop` field escaping and invalid-field rejection,
- Linux MIME/default-app/protocol registration as dry-run and staged file
  generation before any live database update,
- managed/enforced Linux policy diagnostics for missing schemas, user-vs-global
  scope, and lock/default file behavior,
- Windows-shaped autostart and policy diagnostics, even when CI cannot mutate
  machine-wide state,
- hostile input for desktop entry IDs, command arguments, paths, MIME names,
  protocol names, and policy values,
- permission denial for global writes,
- rollback or uninstall reporting for generated files where practical,
- at least one real consumer integration that exercises desktop effects.

## Extraction Rule

When this module is introduced, remove, move, or provide documented pre-1.0
migration guidance for `linuxdesktop::settings::effects` and matching C ABI
entry points. Do not leave stable callers believing that `ld_settings` owns
desktop integration.
