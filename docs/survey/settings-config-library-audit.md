# Settings/Config Library Follow-Up

This pass classifies existing libraries, APIs, and specifications for the first module candidate: settings/config and standard paths.

The goal is not to find a single winner. The Notepad++ audit showed a migration-shaped seam:

- standard path discovery,
- portable/local mode,
- command-line settings override,
- optional cloud/sync override,
- config/state separation,
- plugin-facing roots,
- config bundle hydration from model files,
- ordered save phases,
- backup/restore,
- validation-after-write,
- and byte-level integrity hooks.

Existing tools solve important pieces. None of the reviewed tools provides the full migration-shaped module we want as a small, CMake-friendly, toolkit-neutral library.

## Classification Summary

| Candidate | Classification | Why |
|---|---|---|
| XDG Base Directory Specification | Adopt | This should define Linux config/data/state/cache/runtime roots. |
| Microsoft Known Folders | Adopt | This should define Windows roaming/local/program-data mappings. |
| `std::filesystem` | Adopt | Good internal path and filesystem primitive; not enough policy by itself. |
| Boost.Nowide | Wrap or recommend | Useful UTF-8 bridge on Windows if dependency is acceptable. |
| Qt `QStandardPaths` / `QSettings` | Recommend or adapter | Excellent for Qt apps; too large and Qt-shaped as universal dependency. |
| GLib path/settings APIs | Recommend or adapter | Excellent for GTK/GLib apps; GLib types/main-context are not neutral. |
| wxWidgets `wxFileConfig` / standard paths | Recommend or adapter | Strong migration precedent, especially XDG migration; too wx-shaped as universal dependency. |
| PlatformFolders-style small libraries | Study/defer | Promising shape for path lookup, but not enough for bundle hydration or migration policy. |
| XDG desktop/file specs beyond basedir | Defer | Relevant for shell integration, not the first settings/config sample. |
| App sandbox APIs and portals | Defer | Important for packaging reality, but first sample can report sandbox limitations without depending on portals. |

## Adopt

### XDG Base Directory Specification

Use XDG as the Linux directory contract.

Important requirements for our module:

- ignore relative `XDG_*` paths,
- default config to `~/.config`,
- default data to `~/.local/share`,
- default state to `~/.local/state`,
- default cache to `~/.cache`,
- treat runtime as login-scoped and not persistent,
- use ordered search roots for data/config,
- create missing write directories with user-only permissions where appropriate,
- and report when a required file cannot be found or written.

Fit:

- **Adopt** as behavior.
- Do not wrap an XDG-only library as the public abstraction.
- The public API should expose named roots such as `config`, `data`, `state`, `cache`, `runtime`, `resources`, `plugin_config`, and `session`.

### Microsoft Known Folders

Use Known Folders as the Windows directory contract.

Important requirements for our module:

- use modern Known Folder concepts for new Windows code,
- map roaming application config to `FOLDERID_RoamingAppData`,
- map local machine-specific state/cache to `FOLDERID_LocalAppData`,
- keep `CSIDL_APPDATA` compatibility in mind because Notepad++ currently uses `SHGetFolderPath(CSIDL_APPDATA)`,
- return structured errors because known-folder lookup can fail,
- and avoid app code owning COM/string-freeing details.

Fit:

- **Adopt** as behavior.
- Hide Win32 details behind a tiny platform backend.
- Preserve migration vocabulary: `roaming_config`, `local_state`, `local_cache`, `program_data`, and `install_resources`.

### `std::filesystem`

Use `std::filesystem` internally for path joining, existence checks, directory creation, copying, renaming, and replacement scaffolding.

Fit:

- **Adopt** as an internal implementation default.
- Do not expose `std::filesystem::path` as the only API surface; C ABI and Rust bindings need stable string ownership.
- Keep explicit error/result objects instead of relying on exceptions only.

## Wrap Or Recommend

### Boost.Nowide

Boost.Nowide provides UTF-8-aware standard-library-like file and console APIs on Windows and aliases mostly to standard behavior on Linux.

Fit:

- **Wrap** if we choose a Boost-backed Windows implementation.
- **Recommend** if users already use Boost or need robust UTF-8 Windows file IO.
- Do not require it for the tiny first sample unless Windows UTF-8 behavior becomes a blocker.

Why not adopt as the module:

- It solves encoding/IO friction, not standard root resolution, portable mode, config bundles, save ordering, backup/restore, or migration diagnostics.

## Recommend Or Adapter

### Qt

Qt has strong APIs for standard paths and settings:

- `QStandardPaths` covers application/generic config, data, state, cache, runtime, and test mode.
- `QSettings` covers native settings and INI-style files.

Fit:

- **Recommend** for apps that already use Qt.
- **Adapter** later if we want a Qt backend or Qt-friendly facade.
- Do not require Qt for toolkit-neutral migration libraries.

Gaps for our target:

- Does not model Notepad++-style config bundle hydration from model XML files.
- Does not provide ordered save phases or byte-level post-save integrity hooks.
- Qt types and event conventions would leak into non-Qt apps.

### GLib/GIO

GLib has path helpers such as `g_get_user_config_dir()` and GIO has `GSettings` for schema-backed settings.

Fit:

- **Recommend** for GTK/GLib apps.
- **Adapter** for GTK/Nextpad++-style ports.
- Avoid GLib types in the toolkit-neutral public API.

Gaps for our target:

- `GSettings` is schema/backend-oriented and not a direct registry-to-file or XML-bundle migration tool.
- GLib path helpers are useful but cached/environment-sensitive and not enough for per-app migration policy.

### wxWidgets

wxWidgets has a strong real-world migration lesson in `wxFileConfig`: newer behavior supports XDG-compliant locations for new files and exposes migration from old dotfile locations.

Fit:

- **Recommend** for wx apps.
- **Study** as a migration reference.
- **Adapter** only if a consuming app already uses wxWidgets.

Gaps for our target:

- wx types, global app conventions, and file-config model are not neutral enough for a small general-purpose C++ library.
- It solves INI-like config migration better than Notepad++-style XML config bundles.

## Study Or Defer

### PlatformFolders-Style Small Libraries

Small cross-platform path libraries are close to our desired dependency shape.

Fit:

- **Study/defer** until we inspect source health, license, CMake shape, and maintenance.
- They may be worth wrapping or learning from for root discovery.

Gaps for our target:

- Path lookup alone does not solve model-file hydration, save lifecycle, backup/restore, validation, cloud override, command-line override, or plugin-facing roots.

### Desktop/Shell Specs

Desktop entries, MIME apps, autostart, portals, and file association specs are important, but they belong to the later shell integration module.

Fit:

- **Defer** for this module.
- Link from later process/shell and desktop integration audits.

## Design Consequences

The first module should be built, but modestly:

- build a tiny neutral library around adopted platform behavior,
- use XDG and Known Folders as the normative root contracts,
- use `std::filesystem` internally,
- optionally wrap Boost.Nowide on Windows later,
- provide adapters/recommendations for Qt, GLib, and wx rather than forcing them,
- keep XML parsing application-owned,
- return structured diagnostics,
- and prove migration behavior with one small CMake sample.

## First Sample Boundary

The sample should include:

- root resolution for Linux XDG and Windows Known Folders,
- app name/organization inputs,
- `--settings-dir` override,
- optional portable marker policy,
- config/data/state/cache/plugin/session roots,
- config bundle hydration from model files,
- ordered save plan with a post-save callback,
- backup plus validation-after-write for one session-like file,
- plain text or JSON diagnostic output,
- and tests or a repeatable CLI invocation that AI agents can run.

The sample should not include:

- Qt, GLib, or wx dependencies,
- XML parsing beyond fixture copying,
- cloud-provider integration,
- portals,
- registry editing,
- or Notepad++ fork changes.

## Proposed Decision

Proceed with a small first module after this classification:

- Working name: `ldconfig` or `ld_settings`.
- Public shape: C++ convenience API first, C-compatible API design kept visible.
- Implementation: platform backends plus neutral bundle helpers.
- Dependency posture: standard library only for the first Linux sample; add optional Windows helper dependencies only if needed.

This is enough evidence to start an ADR/API sketch before writing code.

## Source Links

- XDG Base Directory Specification: https://specifications.freedesktop.org/basedir/0.8/
- Microsoft Known Folders: https://learn.microsoft.com/en-us/windows/win32/shell/known-folders
- Microsoft `KNOWNFOLDERID`: https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid
- Microsoft `SHGetKnownFolderPath`: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
- Qt `QStandardPaths`: https://doc.qt.io/qt-6/qstandardpaths.html
- Qt `QSettings`: https://doc.qt.io/qt-6/qsettings.html
- GLib `g_get_user_config_dir`: https://docs.gtk.org/glib/func.get_user_config_dir.html
- GLib `GSettings`: https://docs.gtk.org/gio/class.Settings.html
- wxWidgets XDG config note: https://wxwidgets.org/blog/2024/01/using-xdg-compliant-config-files/
- Boost.Nowide: https://www.boost.org/library/latest/nowide/
