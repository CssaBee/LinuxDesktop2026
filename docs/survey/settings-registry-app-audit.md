# Settings, Registry, And Portable Config App Audit

This survey expands the original `ld_settings` pass. The goal is to test whether the current API is enough for real Windows-heavy applications and cross-platform references.

Conclusion: the current `ld_settings` sample is a useful seed, but it is not shippable yet. Real applications need named roots, component roots, config layers, migration plans, autostart support, and full Windows Registry support with safe execution controls. ADR 0012 keeps the settings/config subset in `ld_settings` and moves desktop effects to `ld_desktop` and migration behavior to `ld_migration`.

## Requirement Summary

| Requirement | Evidence | API implication |
|---|---|---|
| Standard user/config/data/state/cache roots | Notepad++, ShareX, KeePassXC, Qt, wxWidgets, XDG | Keep fixed roots, but add layered read candidates and named/component roots. |
| Portable settings beside the app | Notepad++, ShareX, System Informer, Rufus, WinSCP, PortableApps | Replace boolean portable mode with explicit portable levels and safety diagnostics. |
| Roaming/shared vs machine-local settings | KeePassXC, Qt, Windows AppData, WinSCP | Add first-class config layers and root persistence metadata. |
| Registry-backed preferences | WinSCP, Greenshot, Files, Rufus, Windows desktop apps | Keep as prototype evidence; app-settings Registry migration compatibility belongs to `ld_migration`, while desktop/system Registry-equivalent behavior belongs to `ld_desktop`. |
| Registry-to-file portability | WinSCP, PortableApps, Rufus, System Informer | Move registry export/import and migration plans with JSON canonical format plus `.reg` compatibility to `ld_migration`. |
| Admin-managed and enforced settings | WinSCP docs, dconf profiles/locks, Windows policy conventions | Add managed and enforced layers, with enforced values non-overridable. |
| Startup/autostart integration | Greenshot/ShareX category, Windows `Run` keys, XDG Autostart | Move autostart effect planning and backend execution to `ld_desktop`. |
| File associations and protocol handlers | WinSCP setup, Windows shell registry, freedesktop MIME/Desktop Entry specs | Move to `ld_desktop`. |
| Migration from legacy locations | ShareX, KeePassXC, wxWidgets | Move `migration_plan` and execution behavior to `ld_migration`; keep hydration separate in `ld_settings`. |
| Test mode / isolated roots | Qt `QStandardPaths`, CI needs | Add explicit test/sandbox root override options. |

## Application Findings

### Notepad++

Notepad++ remains the proof case for path and config-bundle behavior:

- executable/resource root detection,
- `doLocalConf.xml` portable marker,
- command-line `-settingsDir`,
- cloud-choice config override,
- user/session/plugin config paths,
- model XML hydration,
- backup/restore for session files,
- validation-after-write.

Current `ld_settings` fit:

- Root resolution, portable marker, config-only sync override, plugin config root, hydration, and safe writes fit the observed Notepad++ settings seam.

Gaps:

- No named roots for additional plugin/profile/log directories.
- No config-layer report for defaults/global/user/local/portable/managed/enforced.
- No registry compatibility for future Windows-side migration tools.
- No formal migration plan API separate from hydration.

Source anchors:

- https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp
- https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/winmain.cpp

### ShareX

ShareX selects a personal data root from sandbox mode, portable CLI flag, portable marker file, registry-backed system option, migrated path config, and directory creation.

Observed behavior:

- `UpdatePersonalPath()` prefers portable command-line mode, then a portable marker file, then a registry-backed system option, then a migrated/read `PersonalPath.cfg`.
- Personal roots include history, logs, screenshots, image effects, and models.
- Startup also registers extensions when not portable.

Current `ld_settings` fit:

- `settings_override`, portable marker policy, directory creation, diagnostics, and named roots can simplify this flow.

Gaps:

- Needs named roots for `history`, `logs`, `screenshots`, `image_effects`, and `models`.
- Needs legacy migration from previous personal path config location.
- Needs registry-backed source support to represent `SystemOptions.PersonalPath`.
- Needs effect boundary so extension registration is not silently treated as ordinary settings.

Source anchor:

- https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/Program.cs

### WinSCP

WinSCP is the strongest surveyed case for a real registry/file configuration storage abstraction.

Observed behavior from project documentation:

- Configuration can be stored in the Windows Registry or an INI file.
- Installed builds default to the Registry; portable versions prefer INI files when possible.
- `/ini=nul` means no persistent configuration.
- Custom INI file paths and automatic storage transfer are supported.
- Administrator restrictions and enforcements remain stored in the Registry.
- Auto-selection checks INI files first, then Registry keys in HKCU/HKLM.
- UAC VirtualStore can affect where INI files are found.

Current `ld_settings` fit:

- Root resolution and safe file writes are useful, but insufficient.

Gaps:

- Needs a `storage_backend` choice: registry, INI/file, null, override.
- Needs layer precedence with read-only and enforced layers.
- Needs Registry HKCU/HKLM and 32/64-bit view support.
- Needs migration/export/import between Registry and INI/file storage.
- Needs UAC/privileged-root diagnostics.

Source anchors:

- https://winscp.net/eng/docs/config
- https://winscp.net/eng/docs/ui_pref_storage
- https://github.com/winscp/winscp/blob/master/deployment/winscpsetup.iss

### KeePassXC

KeePassXC is a useful reference because it is already cross-platform and still keeps separate config classes.

Observed behavior from source search:

- Config entries are tagged as local or roaming.
- Portable mode stores config files near the app.
- Linux local state uses `XDG_STATE_HOME` or `~/.local/state`.
- The app migrates an old Linux local config path from cache to state.
- Import/export intentionally filters which settings are allowed.

Current `ld_settings` fit:

- XDG roots, portable roots, and migration diagnostics are aligned with the need.

Gaps:

- Needs explicit roaming/shared vs local-machine config categories.
- Needs `migration_plan` for old-path moves.
- Needs import/export validation hooks so apps decide which settings are legal to import.

Source anchor:

- https://github.com/keepassxreboot/keepassxc/blob/develop/src/core/Config.cpp

### PortableApps Launcher

PortableApps is a reference for making registry and filesystem changes portable without rewriting the target app.

Observed behavior:

- Registry keys can be saved into `.reg` files and restored around app execution.
- Registry values can be written before launch.
- Registry keys/values can be backed up, deleted, cleaned if empty, or forcibly cleaned.
- Files and directories can be moved into/out of a portable data directory.
- Existing target files/directories are backed up and restored.

Current `ld_settings` fit:

- Safe writes and backups are aligned, but the current module does not model run-scoped migration.

Gaps:

- Needs registry snapshot/restore operations.
- Needs run-scoped cleanup semantics.
- Needs dry-run plans and explicit dangerous-operation flags.
- Needs canonical JSON export in addition to `.reg` compatibility.

Source anchors:

- https://portableapps.com/manuals/PortableApps.comLauncher/ref/launcher.ini/registry.html
- https://portableapps.com/manuals/PortableApps.comLauncher/ref/launcher.ini/filesystem.html

### Rufus

Rufus is a boundary case for portability and Registry use.

Observed behavior:

- Rufus is intentionally Windows-specific because device, setup, and volume APIs are central.
- Registry writes appear in setup/support code.
- Rufus FAQ and support discussions distinguish portable behavior from complete absence of Windows integration.

Current `ld_settings` fit:

- Useful only for settings/portable mechanics, not for the core device product.

Gaps:

- The module must not claim that portable mode means "no registry ever" unless a strict portable level is selected.
- Device and install/setup behavior should remain outside `ld_settings`.

Source anchors:

- https://github.com/pbatard/rufus/blob/master/res/setup/setup.c
- https://rufus.ie/en/

### System Informer

System Informer documents a simple portable marker behavior.

Observed behavior:

- Creating `SystemInformer.exe.settings.xml` beside the executable makes settings live with the app, useful for USB/portable use.

Current `ld_settings` fit:

- Portable marker and resource-root handling fit this case well.

Gaps:

- Needs named roots and migration diagnostics if the app later grows more settings files.

Source anchor:

- https://github.com/winsiderss/systeminformer/blob/master/README.md

### Qt

Qt is a reference, not a dependency target.

Observed behavior:

- `QSettings` offers platform-independent settings with native Windows Registry support, INI support, user/system scopes, fallback lookup, atomic sync, and custom formats.
- On Windows, native settings use Registry paths under HKCU/HKLM; on Unix, native settings use XDG-style config files.
- `QStandardPaths` provides standard locations and a test mode that avoids touching real user config.

API implications:

- `ld_settings` needs user/system layer vocabulary.
- Test mode should be explicit.
- Atomic writes and multi-process safety need further design.
- Avoid Qt types in the neutral core; provide adapter guidance later.

Source anchors:

- https://doc.qt.io/qt-6/qsettings.html
- https://doc.qt.io/qt-6/qstandardpaths.html

### wxWidgets

wxWidgets is a migration reference for old home-directory config files.

Observed behavior:

- New wxWidgets behavior can prefer XDG-compliant config paths.
- Existing dotfile configs can be migrated into the XDG layout with a dedicated migration function.

API implications:

- Migration should be first-class, separate from hydration.
- Migration reports should include old path, new path, and error diagnostics.

Source anchor:

- https://wxwidgets.org/blog/2024/01/using-xdg-compliant-config-files/

## API Consequences

The current `ld_settings` API should be extended before we call it shippable:

- Add `portable_level` instead of relying on boolean portable state.
- Add config layers: `defaults`, `global`, `user`, `local`, `portable`, `managed`, and `enforced`.
- Add default precedence: `defaults < global < user < local < portable < managed < enforced`, with enforced non-overridable.
- Add named roots in the first C++ and C API.
- Add component root helpers in the first API.
- Add a storage backend model: file, registry, null, override, app_callback.
- Add full Windows Registry support: HKCU/HKLM, 32/64-bit views, read/write/delete/enumerate, import/export, JSON canonical export, `.reg` compatibility, registry value types, and policy hives.
- Add Linux equivalents for relevant Registry-backed effects: XDG config files, dconf/GSettings defaults and locks, and XDG Autostart.
- Do not implement file associations or protocol handlers now; document them as issue-requested future scope.
- Add `migration_plan` with dry-run default and explicit execute step.
- Require explicit dangerous-operation flags for HKLM, policy hives, recursive delete, import, and autostart changes.

## Decision

`ld_settings` is not ready to ship. It is ready to grow deliberately.

The next design/code path should be:

1. Document the expanded platform equivalents.
2. Document the expanded API shape.
3. Implement named roots, component roots, config layers, and portable levels.
4. Implement Windows Registry support.
5. Implement autostart and managed/enforced policy execution.
6. Add migration examples from Notepad++, ShareX, WinSCP, KeePassXC, and PortableApps-style workflows.
