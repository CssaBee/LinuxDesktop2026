# Module Priority Score

Candidate modules are scored after the initial repository survey. Scores decide which capabilities deserve a focused follow-up search and which ones wait.

## Scoring Criteria

Scores use a 1 to 5 scale:

- **1**: weak or narrow evidence.
- **2**: present, but limited or not a good reusable seam yet.
- **3**: real need with meaningful caveats.
- **4**: strong need and plausible reusable seam.
- **5**: very strong need or proof-case value.

Scores measure requirement pressure, not automatic build order. A UI-heavy feature can score high while still being deferred because it belongs in toolkit adapters or application-specific port work.

| Criterion | Meaning |
|---|---|
| Frequency | How often the feature appears across surveyed repositories. |
| Coupling | How deeply the feature is embedded in application behavior. |
| Linux complexity | How difficult the Linux replacement appears to be. |
| Standalone usefulness | Whether the feature makes sense as a reusable library. |
| Notepad++ POC value | Whether the feature helps prove the Notepad++ native Linux path. |

## Module Buckets

### First Candidates

- Settings/config
- File watcher
- Process/shell
- Dynamic library loading
- Filesystem/path helpers
- Single-instance IPC

### UI-Adjacent Candidates

- GUI/windowing
- Clipboard
- Drag-and-drop
- Common dialogs/resources

### Future Work Candidates

- Printing
- Plugin ABI
- Global hotkeys/input automation
- Window discovery/control automation
- Accessibility/control automation
- Advanced theming/DPI
- Accessibility integration
- Installer/package integration
- Desktop integration effects
- Service/daemon lifecycle

## Pre-Scoring Notes

Do not assign final scores from README-level evidence alone. A module can be scored after the survey verifies:

- where the Windows feature appears in source,
- whether usage is central or incidental,
- whether existing abstractions already exist,
- and whether at least several repositories share the same requirement shape.

Do not assign final scores from source usage alone either. A source finding proves that a requirement exists; it does not prove that we should build a new module. Before scoring, run the ecosystem audit in `docs/survey/ecosystem-audit.md` to check existing ports, rewrites, abstractions, and abandoned attempts.

The first source audit batch gives enough evidence for preliminary direction, but not final totals. The next scoring pass should happen after these checks, in priority order:

1. Audit discovered ports, rewrites, and abstraction libraries enough to decide whether each is a reference, dependency candidate, warning sign, or unrelated solution.
2. Run a deeper Notepad++ source pass focused on one subsystem.
3. Search existing libraries per high-priority module and classify them as adopt, wrap, recommend, reject, or defer.
4. Run a quick issue/discussion pass for Linux-demand signals.

**First priority completed enough to continue**: ecosystem audit of discovered ports and abstractions. The first comparison set now covers Nextpad++, XerahS, AHK_X11, libuv, Qt Core, GLib/GIO, wxWidgets, Boost.Process, and Boost.DLL.

**Then selected**: deeper Notepad++ source pass focused on settings/config and the path roots required by settings.

Why this subsystem first:

- It is narrow enough to audit deeply without committing to a full UI port.
- It appears across multiple surveyed applications.
- Existing libraries solve pieces, but not the migration-specific shape we want.
- It can produce a tiny first working sample before harder modules such as file watching, plugin loading, clipboard, or drag-and-drop.

**Second priority completed enough to continue**: deeper Notepad++ settings/config audit. The pass found that the real seam is a settings root resolver plus config bundle manager, with ordered save phases, plugin-facing roots, model-file hydration, backup/restore, validation-after-write, and byte-level integrity hooks.

**Then selected**: existing-library follow-up for the first module candidate: settings/config and the path roots required by settings. This classified libraries and specifications as adopt, wrap, recommend, reject, or defer before the first sample.

**Third priority completed enough to continue**: settings/config existing-library follow-up. The classification in `docs/survey/settings-config-library-audit.md` says to adopt XDG Base Directory and Microsoft Known Folders as normative behavior, use `std::filesystem` internally, optionally wrap/recommend Boost.Nowide for Windows UTF-8 IO, and treat Qt, GLib, and wxWidgets as recommendations/adapters rather than mandatory dependencies.

**Fourth priority completed enough to continue**: ADR/API sketch for the settings/config module. ADR 0008 defines the first sample boundary and explicitly defers framework adapters, cloud integration, portals, registry editing, and Notepad++ fork changes.

**Fifth priority completed enough to continue**: first executable `ld_settings` sample. The CMake build and CLI demo validate root resolution, model-file hydration, ordered writes, backups, and validation-after-write on Ubuntu.

**Then selected**: finish the `ld_settings` consumption path before starting a new module. The first module needs to be usable by GitHub users and AI agents through ordinary CMake flows, not only runnable inside this repository.

**Sixth priority completed enough to continue**: installed CMake package support and an install-tree consumer smoke test for `LinuxDesktop2026::ld_settings`.

**Seventh priority completed enough to continue**: atomic temp-write/replace for `write_with_backup`. The default path now writes a same-directory temporary file, validates it before commit, backs up the previous target when requested, and replaces the target only after the pending file is acceptable.

**Picked next**: Windows verification for `ld_settings`, especially Known Folder lookup and atomic replace behavior. The verification checklist now lives in `docs/plan/ld-settings-windows-verification.md`.

**Eighth priority completed enough to continue**: first C ABI seed for `ld_settings` root resolution. This keeps future Rust binding work realistic without exposing C++ ABI details.

**Ninth priority completed enough to continue**: first API/ABI stability policy and version surface. C++ exposes version constants; the C ABI exposes matching macros and runtime functions.

**Picked next**: prepare the Notepad++ fork integration plan for the standalone `ld_settings` surface.

**Tenth priority completed enough to continue**: Notepad++ integration prep. The POC plan now defines a narrow first fork patch that maps settings/config behavior to `ld_settings` while keeping XML parsing, UI, plugin ABI, and Windows compatibility app-owned.

**Eleventh priority completed enough to continue**: shared diagnostics extraction before watcher work. ADR 0009 captures the decision, and `LinuxDesktop2026::ld_core` now provides a tiny C++ diagnostic vocabulary while preserving `ld_settings` aliases.

**Twelfth priority completed enough to continue**: focused and broader file watcher evidence pass. The application audits in `docs/survey/file-watcher-audit.md` and `docs/survey/file-watcher-application-audit.md` found a real migration seam: single-file and directory watching, save-by-replace behavior, debounce and settled-file workflows, dirty-path refresh pressure, overflow/rescan diagnostics, and recursive-policy honesty. The library follow-up in `docs/survey/file-watcher-library-audit.md` now completes the build/wrap/recommend classification.

**Current watcher direction**: the broad `ld_watch` prototype now exists, with native Linux `inotify` as the default Ubuntu backend, native Windows `ReadDirectoryChangesW` implemented behind the backend seam, and optional libuv verified as a preferred backend when selected through CMake. Public C++ API stabilization has started with backend capability identity, named diagnostic-code constants, `watch_id` equality, timeout-capable pull delivery, and optional settled-file timeout policy. Recommend libuv directly for apps that already own a libuv loop and only need coarse change/rename notifications; keep `ld_watch` for migration-shaped paths, diagnostics, recursive-policy honesty, and settled-file behavior. Windows watcher smoke coverage is wired into CI, Windows CI is green, and the first recursive/save-by-replace/remove-rename/resource-limit/settled-file hardening pass is implemented; keep native Windows verification green before C ABI or capability schema expansion.

**Thirteenth priority started**: expanded `ld_settings` settings/Registry survey. The expanded app and platform-equivalent audits show that `ld_settings` is not shippable yet. It must grow named roots, component roots, portable levels, all config layers, migration plans, Windows Registry support, Linux equivalents for relevant Registry effects, autostart, and managed/enforced policy before release.

**Current `ld_settings` direction**: update examples and then implement expanded root/layer API before moving deeper into Registry execution. Keep file-watcher work parked until the user chooses to commit or resume it.

**Fourteenth priority completed enough to continue**: extended watchlist fit audit. The pass sampled OpenRGB, sample-cpp-plugin, dylib, PrusaSlicer, OpenSCAD, FreeCAD, Carla, Project Island, NUT, and GTR_Framework. The audit keeps `ld_settings` and `ld_watch` pointed in the same broad direction, but promotes `ld_paths`, `ld_process`, and `ld_ipc` as separate planning lanes, and names desktop integration plus service/daemon lifecycle as future boundaries.

**Fifteenth priority completed enough to continue**: focused `ld_paths` planning. The survey in `docs/survey/ld-paths-application-audit.md` and roadmap in `docs/plan/ld-paths-roadmap.md` select `ld_paths` as the next planned module. The accepted shape is resolver-first, but broad enough for a community-facing prototype: standard user paths, executable/resource/install roots, source-labeled candidate reports, path-list parsing, typed plugin path sets, Wine-prefix-aware defaults where needed, opt-in directory creation, C++17 first, and a small C ABI before public prototype announcement.

## Preliminary Evidence From Source Audit Round 1

| Candidate module | Evidence strength | Seen in source audit | Preliminary direction |
|---|---|---|---|
| File watcher | Strong | Notepad++, ShareX, Files, libuv, Qt, GLib/GIO, wxWidgets, .NET, Watchman, fswatch, efsw, e-dant/watcher, Panoptes, Project Island | Prototype implemented; libuv preferred-backend smoke verified, Windows backend still needs real Windows verification |
| Process/shell | Strong | Notepad++, WinMerge, ShareX, Greenshot, WinSCP, Files, libuv, PrusaSlicer, NUT, OpenRGB | Focused planning candidate; split argv-safe spawn, shell command mode, desktop-open/default-app behavior, and daemon readiness handshakes |
| Dynamic library loading | Strong | Notepad++, WinMerge, AutoHotkey, Rufus, WinSCP, libuv, wxWidgets, sample-cpp-plugin, dylib, Project Island, Carla, PrusaSlicer, FreeCAD | First focused search candidate; decide adopt/wrap/recommend/reject for `dylib` before implementing |
| Single-instance IPC | Strong | WinMerge, AutoHotkey, WinSCP, libuv, PrusaSlicer, FreeCAD, NUT; Notepad++ follow-up needed | Promote follow-up if scoped to lock ownership, stale recovery, local transport, and activation forwarding |
| Settings/config | Strong | ShareX, Greenshot, Files, Rufus, Notepad++ deep pass, library follow-up, executable sample, OpenRGB, PrusaSlicer, OpenSCAD, FreeCAD, NUT, Carla | First implementation candidate; add environment override, legacy fallback, and config-layer candidate diagnostics before ship |
| Filesystem/path helpers | Strong | All audited apps touch it; Files/Rufus show complex non-path semantics; Notepad++, OpenSCAD, FreeCAD, Carla, OpenRGB, PrusaSlicer, NUT, and Project Island show reusable path-root pressure | Selected as next planned module, `ld_paths`; build resolver-first with standard user dirs, executable/resource roots, candidate reports, path lists, and typed plugin path sets |
| Desktop integration effects | Medium to strong | PrusaSlicer, OpenRGB, platform-equivalent survey | Future module/effects package for desktop entries, autostart files, icons, MIME types, URL protocols, and desktop database updates |
| Clipboard | Strong but UI-coupled | Notepad++, WinMerge, ShareX, Greenshot, WinSCP, Files, wxWidgets | UI-adjacent; likely toolkit adapter |
| Drag-and-drop | Medium to strong but UI-coupled | ShareX, Greenshot, Files, WinSCP, wxWidgets | UI-adjacent; likely toolkit adapter |
| GUI/windowing | Strong but too broad | Notepad++, WinMerge, AutoHotkey, Rufus, WinSCP, wxWidgets | Do not make first low-level module |
| Printing | Limited but real | Greenshot, wxWidgets; likely Notepad++/WinMerge follow-up | Future work unless POC requires it |
| Device/volume operations | Strong boundary signal | Rufus | Future/application-specific; not first library wave |

## First Scoring Pass

This is a provisional scoring pass after:

- source audit round 1,
- ecosystem audit round 1,
- the Notepad++ settings/config deep pass,
- the settings/config existing-library follow-up,
- ADR 0008,
- and the first executable `ld_settings` sample.

The scores are good enough to guide the next work item. They are not final project totals because the larger issue/discussion pass and remaining reference-source audits are still open.

| Candidate module | Frequency | Coupling | Linux complexity | Standalone usefulness | Notepad++ POC value | Total | Decision |
|---|---:|---:|---:|---:|---:|---:|---|
| Settings/config | 5 | 4 | 3 | 5 | 5 | 22 | First implementation sample; consumption path in progress |
| File watcher | 4 | 4 | 4 | 5 | 4 | 21 | Prototype implemented; C++ API stabilization started; verify Windows next |
| Process/shell | 5 | 4 | 4 | 5 | 3 | 21 | Follow-up soon; split raw process spawning from desktop-open/default-app behavior |
| Filesystem/path helpers | 5 | 3 | 3 | 5 | 5 | 21 | Next planned module as `ld_paths`; resolver-first public prototype with C++ API, C ABI, path families, candidate reports, path lists, and typed plugin path sets |
| Dynamic library loading | 4 | 4 | 3 | 4 | 4 | 19 | Follow-up later; likely policy layer over existing loaders |
| Clipboard | 5 | 4 | 5 | 3 | 4 | 21 | UI-adjacent; design as toolkit/session adapter, not first neutral core |
| Common dialogs/resources | 4 | 5 | 5 | 2 | 4 | 20 | UI-adjacent; likely migration tooling/docs rather than first library |
| GUI/windowing | 5 | 5 | 5 | 2 | 5 | 22 | High pressure but too broad; defer to toolkit strategy/Notepad++ POC design |
| Drag-and-drop | 4 | 4 | 5 | 3 | 2 | 18 | UI-adjacent; toolkit adapter |
| Single-instance IPC | 3 | 4 | 3 | 4 | 3 | 17 | First-candidate follow-up if scoped to detect vs forward activation |
| Printing | 2 | 3 | 4 | 2 | 1 | 12 | Future work unless Notepad++ POC or issue pass raises priority |
| Desktop integration effects | 3 | 3 | 4 | 4 | 2 | 16 | Future effects/module; keep MIME/protocol/desktop entry behavior out of first `ld_settings` |
| Service/daemon lifecycle | 2 | 4 | 4 | 3 | 1 | 14 | Future work after `ld_process` and `ld_ipc` mature |

## Score Consequences

- `Settings/config` remains the right first sample because it combines high score, low enough implementation risk, and direct Notepad++ proof-case value.
- `GUI/windowing` and `clipboard` score high as requirements but stay out of first-wave neutral core because Linux behavior is toolkit-, compositor-, and session-dependent.
- `File watcher` remains a strong prototype, but `ld_paths` is now the next planned module because the extended watchlist exposed a reusable resolver need across OpenSCAD, FreeCAD, Carla, OpenRGB, PrusaSlicer, NUT, Project Island, and the Notepad++ proof case.
- `ld_paths` should not remain scoped under `ld_settings`. The focused plan splits path resolution from settings payload behavior and adds standard user dirs, executable/resource roots, candidate reports, path lists, typed plugin path sets, and opt-in directory creation.
- `Process/shell` and `Single-instance IPC` should receive focused design passes before implementation. PrusaSlicer, FreeCAD, and NUT show that spawn, shell command, readiness, lock ownership, stale recovery, and activation forwarding need separate concepts.
- `Dynamic library loading` should emphasize loader policy rather than reimplementing `dlopen`/`LoadLibrary` primitives, and should classify `dylib` before any build decision.
- Desktop integration and service/daemon lifecycle are real but should stay future work. They are too cross-cutting to absorb into `ld_settings`.
