# Notepad++ Settings And Config Source Audit

This pass focuses on one Notepad++ subsystem: settings/config discovery, model-file hydration, user data locations, and config persistence. It is intentionally narrow so we can decide whether a first reusable module can be proven without taking on the full Notepad++ UI port.

## Scope

Repository inspected:

- `/tmp/linuxdesktop2026-survey/notepad-plus-plus`

Primary files inspected:

- `PowerEditor/src/Parameters.cpp`
- `PowerEditor/src/Parameters.h`
- `PowerEditor/src/winmain.cpp`
- `PowerEditor/src/NppBigSwitch.cpp`
- `PowerEditor/src/NppXml.h`
- `PowerEditor/src/MISC/Common/Common.cpp`
- `PowerEditor/src/MISC/Common/Common.h`
- `PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h`
- `PowerEditor/src/MISC/hmac/hmac.h`
- `PowerEditor/installer/nsisInclude/mainSectionFuncs.nsh`
- `PowerEditor/installer/nsisInclude/tools.nsh`
- `PowerEditor/installer/nsisInclude/uninstall.nsh`

## Core Finding

Notepad++ does not just need a key/value settings store. Its actual seam is a **settings root resolver plus config file family manager**:

- resolve install path,
- resolve current working directory,
- detect portable/local mode,
- choose per-user settings root,
- allow command-line settings override,
- allow a cloud settings override,
- keep session storage out of cloud in some cases,
- create required user/plugin config directories,
- hydrate missing user config files from model XML files,
- recover corrupted or empty files,
- save several XML-backed settings families,
- expose active settings path to plugins,
- and keep compatibility with existing Notepad++ file names and behavior.

This is a better first module candidate than a generic `QSettings`-style abstraction.

## Path Selection Behavior

### Install And Startup Paths

`NppParameters::NppParameters()` initializes:

- `_nppPath` from `GetModuleFileName(NULL, ...)` and `PathRemoveFileSpec`.
- `_currentDirectory` from `GetCurrentDirectory`.
- `_asNotepadStyle` from an install-dir sentinel file.
- current Windows version and system code page.

Linux implication:

- We need an `application_paths` layer that can resolve executable directory, invocation working directory, user config directory, user data directory, state directory, cache directory, and optional app-local portable directory without leaking Win32 path APIs into app code.

### Portable/Local Mode

`NppParameters::load()` detects `doLocalConf.xml` in the install directory. If it exists, Notepad++ treats the install directory as the settings root, except it disables local mode on Vista-or-newer when installed under Program Files.

Installer scripts expose the same concept to users as "Don't use %APPDATA%" for USB/portable usage.

Linux implication:

- Portable mode should be explicit and policy-driven, not just "write beside executable." On Linux, writing beside the executable may fail for packaged apps, AppImage, Flatpak, system installs, or read-only locations.
- A first module should report `portable_requested`, `portable_available`, and `portable_denied_reason`.

### Default Per-User Mode

When not local, Notepad++ uses `SHGetFolderPath(CSIDL_APPDATA)` and appends `Notepad++`. It creates:

- the user settings directory,
- `plugins`,
- and `plugins/Config`.

It also keeps `_pluginRootDir` under the installation `plugins` directory and `_userPluginConfDir` under per-user settings.

Linux implication:

- Our module should distinguish app-shipped resources from user-writable config.
- It should provide named roots, not just one directory:
  - install resource root,
  - user config root,
  - user data root,
  - plugin install root,
  - plugin config root,
  - session/state root.

### Cloud Settings Override

Notepad++ checks a `cloud/choice` file under the normal settings folder. If it exists and points to an existing directory, `_userPath` is replaced with that cloud directory and `_nppGUI._cloudPath` is initialized.

`setCloudChoice()` writes that choice file as UTF-8. `removeCloudChoice()` deletes it. `writeSettingsFilesOnCloudForThe1stTime()` copies selected existing settings files to a cloud directory if they do not already exist.

Notably, `_sessionPath` is set before cloud override, with the comment that the session stores absolute file paths and should never be on cloud. If `-settingsDir` is provided later, `_sessionPath` is reset to the custom settings directory.

Linux implication:

- "Settings root" may not equal "state/session root."
- A useful module should model root kinds separately and allow user-selected sync/cloud directories for config without forcing volatile or machine-local state to follow.

### Command-Line Settings Override

`winmain.cpp` parses `-settingsDir=...` and calls `NppParameters::setCmdSettingsDir()`. In `load()`, a valid `_cmdSettingsDir` overrides both default user path and cloud settings. An invalid directory is ignored after a message box.

Linux implication:

- A first sample can include `--settings-dir` as a portable, testable behavior.
- The resolver should return diagnostics rather than directly showing UI, so GUI and CLI apps can decide how to report invalid paths.

## Config File Families

Notepad++ manages a family of XML files under the active user path:

- `langs.xml`
- `config.xml`
- `stylers.xml`
- `userDefineLang.xml`
- `userDefineLangs/*.xml`
- `nativeLang.xml`
- `toolbarButtonsConf.xml`
- `shortcuts.xml`
- `contextMenu.xml`
- `tabContextMenu.xml`
- `session.xml`

Several files have model/source defaults under the installation directory:

- `langs.model.xml`
- `config.model.xml`
- `stylers.model.xml`
- `shortcuts.xml` model name through `SHORTCUTSXML_MODEL_FILENAME`
- `contextMenu.xml` model name through `CONTEXTMENUXML_MODEL_FILENAME`

Missing files are copied from model files or generated from embedded XML strings. Some files are optional and failure to load does not always fail the entire config load.

Linux implication:

- The module should not be limited to scalar settings.
- We likely need a `config_bundle` concept:
  - declare expected files,
  - declare source model file,
  - declare required/optional,
  - copy or generate on first run,
  - validate parse/load result through app-provided callbacks,
  - return a structured load report.

## Save Lifecycle

Settings are not saved as an unordered bag. During shutdown, `NppBigSwitch.cpp` performs a deliberate sequence:

- notify plugins with `NPPN_SHUTDOWN`,
- save Scintilla zoom,
- save `shortcuts.xml`,
- save GUI parameters, find history, and recent file history into `config.xml`,
- save `config.xml`,
- save user-defined languages,
- save `session.xml` if last-session restore is enabled,
- optionally save the current session into a separately loaded session file,
- and finally copy first-run cloud settings files if the cloud path changed.

The order matters. `shortcuts.xml` is saved first, then read back as bytes to compute an HMAC. That HMAC is stored in the in-memory GUI/config state and then persisted when `config.xml` is saved.

Linux implication:

- A reusable module should not provide only `save_all(files)` without ordering.
- The bundle layer should allow explicit phases or dependencies such as "save file A, post-process file A, update in-memory config, then save file B."
- The module should return save reports rather than show UI directly, because shutdown, GUI, CLI, and tests need different reporting behavior.

## Recovery And Integrity

Important recovery behavior:

- Empty `langs.xml` can trigger a recovery copy from `langs.model.xml`.
- Invalid `langs.xml` marks load failure after user-facing prompt logic.
- Invalid `stylers.xml`, user-defined languages, shortcuts, context menus, and session files have separate failure handling.
- `session.xml` has a backup swap path using `ReplaceFile` or `MoveFileEx`.
- `shortcuts.xml` gets an HMAC stored in `config.xml`; the key comes from Windows `MachineGuid` in the registry, and the hash uses Windows BCrypt APIs.
- Session saving removes read-only attributes, copies the previous session to a backup file, writes the new XML, loads it back, validates it into a `Session` object, and restores the backup if validation fails.
- Session startup recovery can swap a `.bak` backup into place when the primary `session.xml` is missing or invalid.

Linux implication:

- Config migration may include integrity metadata, machine identity, and backup/replace semantics.
- We should not clone the `MachineGuid` behavior blindly. A portable design needs an explicit `machine_id_provider` or an application-provided secret/key source.
- Atomic replace and backup restore should be part of the file-helper surface or documented as a required policy.
- Validation-after-write is a real requirement, especially for high-value state like open editor sessions.
- The first module can keep integrity support minimal, but it should reserve a hook for "derive metadata from saved file bytes."

## File Helper Semantics

Notepad++ helper behavior is compact but strongly Win32-shaped:

- `getFileContent()` checks existence, opens with `_wfopen(..., "rb")`, reads raw bytes, and optionally reports failure through a boolean pointer.
- `writeFileContent()` wraps `Win32_IO_File` and writes string bytes if the file opens.
- `pathAppend()` is backslash-oriented and mutates the destination string.
- `doesFileExist()` and `doesDirectoryExist()` use Windows file attributes and support an optional timeout path.
- Directory creation is mostly direct `CreateDirectory()` calls with limited structured error propagation.
- XML IO is handled through a thin `NppXml` wrapper around pugixml. Different config file families use different parse/save flags.

Linux implication:

- The abstraction should separate "path composition" from "Windows-compatible path migration."
- The core should prefer `std::filesystem::path` internally but expose stable string APIs for C and Rust friendliness.
- File reads/writes should return typed results, including not found, permission denied, invalid directory, parse failure, validation failure, and write failure.
- XML should remain application-owned. Our module can hydrate/copy/save bytes and run callbacks, but should not force a specific XML library on adopters.
- Timeout-aware existence checks are a hint for network-drive behavior, but likely not part of the first tiny sample unless another repository raises it again.

## Plugin-Facing Config Roots

Two plugin-facing paths matter:

- `NPPM_GETPLUGINSCONFIGDIR` returns the per-user plugin config directory.
- `NPPM_GETNPPSETTINGSDIRPATH` returns the active settings directory selected by `-settingsDir`, cloud, AppData, or install-dir local mode.

Linux implication:

- The resolver should name the difference between host settings, plugin config, plugin install/home, shipped resources, and session/state.
- A reusable module must be able to answer "where should a plugin write its config?" separately from "where did the host load its shipped resources?"
- A Notepad++ POC can initially stub plugin compatibility, but the path API shape should be designed before the fork touches plugins.
- The Notepad++ POC will need this path API before plugin compatibility can be evaluated.

## Module Requirements Suggested By This Audit

A first `settings/config` module should likely provide:

- path discovery for install, config, data, state, cache, runtime, and working directory,
- ordered root resolution with named policies,
- portable/local mode detection and explicit denial reasons,
- command-line override support,
- user-selected sync/cloud override support,
- separation between config root and session/state root,
- directory creation with structured diagnostics,
- model-file hydration for config bundles,
- optional versus required config file reporting,
- ordered save phases and post-save callbacks,
- backup and atomic replacement helpers,
- validation-after-write hooks,
- byte-level post-processing hooks for integrity metadata,
- simple UTF-8 text read/write helpers,
- path normalization that tolerates trailing slash differences,
- and C ABI/Rust-friendly ownership boundaries later.

## Smallest Useful First API Shape

The first code sample should prove these concepts without pretending to solve every Notepad++ setting:

- `ld_paths_resolve_app()` or C++ equivalent:
  resolves install/resource root, config root, data root, state/session root, cache root, plugin config root, and working directory.
- `ld_config_bundle_hydrate()`:
  takes file descriptors such as `{name, model_name, required, target_root, source_root}` and creates missing files.
- `ld_config_bundle_report`:
  lists selected roots, created directories, copied files, skipped existing files, warnings, and failures.
- `ld_config_write_with_backup()`:
  writes or replaces one high-value file with optional backup and validation callback.

Keep XML parsing outside the library in the first sample. Use XML-looking fixture files only to prove hydration and save lifecycle behavior.

## Preliminary Decision

Treat `settings/config and standard paths` as the first implementation candidate after the survey phase.

Suggested first proof sample:

- CMake project with one tiny library and one CLI example.
- On Linux, resolve an XDG config root such as `$XDG_CONFIG_HOME/linuxdesktop2026-demo` or `~/.config/linuxdesktop2026-demo`.
- On Windows, resolve a roaming app-data root.
- Support `--settings-dir`.
- Support an optional portable marker file.
- Hydrate a small `config.xml` from `config.model.xml`.
- Hydrate a second file whose save order is declared before `config.xml`.
- Demonstrate a validation-after-write callback for a session-like file.
- Print a structured diagnostic report showing selected backend, selected root paths, created directories, copied defaults, skipped optional files, and warnings.

This would be a credible "smallest useful thing" before any Notepad++ fork work.
