# Settings And Registry Platform Equivalents

This document maps Windows Registry-backed behavior to Linux-native mechanisms. It is intentionally effect-oriented: the goal is not to recreate the Windows Registry on Linux, but to identify which user-visible behaviors need portable APIs.

## Platform Baseline

### Windows

The Windows Registry is a system-defined database for configuration data. Desktop applications commonly use it for preferences, app state, file associations, protocol handlers, autostart, installer state, policies, and shell integration.

The practical `ld_settings` scope includes:

- hives: HKCU and HKLM first, with room for policy hives and user classes,
- views: native, 32-bit, and 64-bit Registry views,
- operations: read, write, delete, enumerate, import, export, and subtree snapshots,
- value types: string, expandable string, multi-string, DWORD, QWORD, binary, and none/unknown,
- safety: minimum access rights, explicit elevated/global writes, dry-run plans before destructive migration,
- formats: JSON as canonical export/import format, `.reg` for Windows compatibility.

Out of first ship scope unless later survey forces them:

- remote Registry,
- transactional Registry,
- direct ACL/security descriptor editing.

Source anchors:

- https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry
- https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-functions
- https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types

### Linux

Linux does not have one universal Registry database, but it does have equivalents for many Registry effects.

Relevant first-scope mechanisms:

- XDG Base Directory for app-owned config, data, state, cache, and runtime locations.
- App-owned config files for preferences and structured state.
- dconf/GSettings-compatible keyfiles and locks for GNOME-style managed and enforced preferences, without linking GLib.
- XDG Autostart `.desktop` files for login startup.

Explicitly not implemented in `ld_settings` first ship:

- file associations,
- default apps,
- protocol handlers,
- shell context menu integration.

Users should request those as GitHub issues. They likely belong in a future `ld_desktop` module.

Source anchors:

- https://specifications.freedesktop.org/basedir/0.8/
- https://help.gnome.org/system-admin-guide/dconf-profiles.html
- https://help.gnome.org/system-admin-guide/dconf-lockdown.html
- https://specifications.freedesktop.org/autostart/latest/

## Effect Mapping

| Windows Registry-backed effect | Linux equivalent | First `ld_settings` decision |
|---|---|---|
| App preferences | XDG config file, app-owned payload, optional dconf-compatible policy files | Implement path/layer lifecycle; app owns payload parsing unless using registry value API. |
| Portable settings | App-adjacent config/data roots, explicit portable level | Implement. |
| Machine-local state | XDG state/cache/runtime roots | Implement. |
| Managed defaults | HKLM/app policy or app-owned global config | Implement Windows; implement Linux via dconf/GSettings plan and backend. |
| Enforced policy | Windows policy keys, dconf locks | Implement both, with enforced values non-overridable. |
| Startup with system | `CurrentVersion\Run` or startup folder | Implement Windows and XDG Autostart backend. |
| File associations | `Software\Classes`, ProgID, UserChoice | Not supported in first `ld_settings`; issue-requested future work. |
| Protocol handlers | URL protocol registration | Not supported in first `ld_settings`; issue-requested future work. |
| Shell context menu | Explorer shell extension/command keys | Not supported in first `ld_settings`; likely future `ld_desktop` or app-specific. |
| Recent documents / jump lists | RecentDocs/Jump Lists | Not supported in first `ld_settings`; future UI/desktop integration. |
| Environment variables | Registry environment keys | Later; likely `environment.d`/shell profile/systemd user integration after separate survey. |
| Services | Service Control Manager Registry/service APIs | Out of `ld_settings`; future process/system module if ever. |
| COM/shell extensions | COM registration | Out of scope for first-wave portable libraries. |

## Config Layers

The first public vocabulary should include every layer accepted in grilling:

| Layer | Meaning | Windows example | Linux example |
|---|---|---|---|
| `defaults` | Shipped defaults/model files | Install resource files | `/usr/share/<app>` resources |
| `global` | System-wide base config | ProgramData or HKLM app key | `/etc/xdg/<app>` |
| `user` | Normal user-editable config | HKCU app key or `%APPDATA%` | `$XDG_CONFIG_HOME/<app>` |
| `local` | Machine-specific user config/state | `%LOCALAPPDATA%` | `$XDG_STATE_HOME/<app>` or cache/state split |
| `portable` | App-adjacent portable config | INI/XML beside executable | app directory or user-selected portable root |
| `managed` | Admin-provided recommended setting | HKLM defaults, dconf system DB | `/etc/dconf/db/*` defaults |
| `enforced` | Admin-locked setting | policy Registry path | dconf locks |

Default precedence:

```text
defaults < global < user < local < portable < managed < enforced
```

Rules:

- `enforced` cannot be overridden.
- `managed` participates in normal precedence unless marked enforced by backend metadata.
- Apps may override precedence, but the report must show the default decision and the overridden decision.
- The library should report all candidate locations, not only the winner.

## Portable Levels

Portable mode should not be a boolean.

| Level | Meaning |
|---|---|
| `off` | Use platform defaults. |
| `settings_only` | Keep preferences/config near the app or selected portable root, but allow normal OS integration. |
| `profile` | Keep config/data/profile/logs portable; state/cache/runtime may stay machine-local. |
| `clean` | Avoid platform traces where practical; warn when a requested effect would touch Registry, dconf, autostart, or other system integration. |

This is necessary because Rufus-style, WinSCP-style, KeePass-style, and PortableApps-style portability do not mean the same thing.

## Autostart

Autostart is in first `ld_settings` scope.

Windows backend:

- user-level `Run` key support,
- optional machine-level support only with explicit dangerous-operation flags,
- report existing values before overwrite,
- support disable/remove.

Linux backend:

- XDG Autostart `.desktop` file in `$XDG_CONFIG_HOME/autostart`,
- support `Hidden=true` for user-level disable,
- validate executable path and generated desktop-entry fields,
- report when desktop environment policy may ignore entries.

Source anchor:

- https://specifications.freedesktop.org/autostart/latest/

## Managed And Enforced Policy

Managed/enforced policy is first-scope and must be implemented for both Windows and Linux.

Windows backend:

- support app-owned policy key paths,
- support HKLM and HKCU policy locations with explicit options,
- support value read/write/delete/enumerate,
- require explicit options for elevated/global writes.

Linux backend:

- use dconf/GSettings-compatible keyfiles and locks where a schema exists,
- support writing admin default keyfiles and lock files only when explicitly requested,
- report when no schema/backend is available,
- do not use GLib as a dependency.

Source anchors:

- https://help.gnome.org/system-admin-guide/dconf-profiles.html
- https://help.gnome.org/system-admin-guide/dconf-lockdown.html

## Unsupported Effects

The following are deliberately unsupported in first `ld_settings`:

- file associations,
- protocol handlers,
- shell context menus,
- recent documents and jump lists,
- environment variable mutation,
- install/uninstall metadata,
- service registration,
- COM registration,
- shell extensions.

Policy: users should open GitHub issues when they need these. The issue should include target OS, desktop environment, Windows Registry keys/functions currently used, and expected Linux behavior.

## Open Checks

- Survey more source anchors for Greenshot startup Registry handling.
- Survey Files default-file-manager Registry handling.
- Survey Open-Shell settings/policy source if it remains in corpus.
- Keep Linux dconf execution file-based; do not add a GLib dependency.
- Decide whether `.reg` parsing/writing belongs in a separate small translation unit or a sub-namespace.
