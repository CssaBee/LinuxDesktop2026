# `ld_desktop` Extraction Requirements

Status: initial C++ extraction in progress; full ship-candidate coverage still
required.

`ld_desktop` owns platform actions that register an application with the
desktop, shell, session, or managed-policy environment. The current C++
autostart and managed/enforced policy implementation lives in
`linuxdesktop::desktop`; callers should use `ld_desktop` directly for those
effects. Registry-equivalent desktop/system behavior belongs in `ld_desktop`.
The matching C ABI lives in `linuxdesktop/desktop_c.h` and the
`ld_desktop` C library surface. `ld_settings` does not publicly depend on
`ld_desktop`.

## Scope

`ld_desktop` supports desktop integration by standards-backed registration
artifacts rather than by separate GNOME, KDE, Xfce, bare-window-manager, or
Windows-shell public APIs. GNOME, KDE Plasma, Xfce, bare window-manager
sessions, and the Windows shell are Desktop Flavors for validation, not separate
platform promises.

The extracted module must cover these responsibility groups before
`ld_desktop` can be treated as the stable desktop integration surface:

- autostart entries,
- desktop entries,
- icon installation and lookup metadata,
- MIME and file associations,
- default applications,
- URL protocol handlers,
- desktop database update and activation plans,
- uninstall cleanup reporting,
- managed and enforced desktop or application policy,
- Registry-equivalent behavior whose purpose is shell, startup, policy,
  session, or desktop integration.

Runtime shell actions such as opening a path or URL, revealing a file in a file
manager, and forwarding a later invocation to an already-running process are
outside this expansion. They should be handled by a later process/shell or IPC
design pass unless repeated consumer evidence proves they belong here.

The preferred C++ surface for app registration should be a coherent desktop
bundle that can plan, dry-run, apply, query, and remove the normal registration
set together. Individual effect calls remain useful for advanced callers,
tests, and partial integrations, but they should not create per-desktop public
APIs.

## Required API Posture

- Use explicit capability reports for every backend.
- Keep filesystem and Registry mutation behind explicit permission flags.
- Default mutating operations to dry-run where practical.
- Separate staged file/Registry artifact writes from live activation commands
  such as `update-desktop-database`, `update-mime-database`, icon-cache
  refreshes, `dconf update`, or Windows system-default updates.
- Report unsupported, sandbox-limited, permission-denied, backend-missing, and
  externally-updated-database states as diagnostics.
- Keep desktop/session concepts separate from settings payload concepts.
- Route generic path selection through `ld_paths` instead of embedding XDG,
  Known Folder, or home-directory policy.
- Treat Windows `CurrentVersion\Run`, `Software\Policies`, shell classes, and
  protocol handler equivalents as desktop/system effects, not settings storage.

## Current Implementation

- `include/linuxdesktop/desktop.hpp` exposes the C++ `ld_desktop` API.
- `include/linuxdesktop/desktop_c.h` exposes the C ABI `ld_desktop` surface.
- `ld_desktop` reports capabilities for autostart, desktop entries, icons,
  MIME/file associations, default applications, URL protocol handlers,
  shell-equivalent integration, desktop database updates, and managed policy.
  Managed policy is reported as backend-limited on Linux because the current
  implementation generates dconf-compatible source files but does not run
  `dconf update` or otherwise verify active dconf database state.
- Linux autostart and managed/enforced policy use the same dry-run-first file
  behavior proven by the earlier prototype. Policy reports include diagnostics
  that generated files still require system dconf installation and activation.
- Windows autostart and policy currently report backend-missing capability
  diagnostics from `ld_desktop`; a non-cyclic Registry/system layer is required
  before those writes should move into this module.
- Desktop Flavor variance is currently covered by hermetic capability and XDG
  path tests. Live desktop-session consumption is not yet release evidence.
- New C++ callers should include `linuxdesktop/desktop.hpp` and link
  `LinuxDesktop2026::ld_desktop`.
- New C callers should include `linuxdesktop/desktop_c.h` and link
  `LinuxDesktop2026::ld_desktop`.

## Validation Required

Before `ld_desktop` is a ship candidate, tests and examples must cover:

- Linux XDG Autostart write, query, disable, and remove paths,
- Linux `.desktop` field escaping and invalid-field rejection,
- Linux MIME/default-app/protocol registration as dry-run and staged file
  generation before any live database update,
- Desktop Flavor scenarios for `xdg_full_gnome`, `xdg_full_kde`,
  `xdg_light_xfce`, `xdg_minimal_bare_wm`, and `windows_registration`, with
  assertions focused on honest capability reporting and correct staged
  artifacts rather than live desktop consumption,
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

When this module grows, keep C++ and C desktop-effect entry points under the
`ld_desktop` headers and library target. Do not leave callers believing that
`ld_settings` owns desktop integration.

The old `ld_settings` desktop-effect facade has been removed; there are no
`ld_settings_*` desktop-effect C ABI entry points in the current code.
