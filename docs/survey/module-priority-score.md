# Module Priority Score

Candidate modules are scored after the initial repository survey. Scores decide which capabilities deserve a focused follow-up search and which ones wait.

## Scoring Criteria

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
- Advanced theming/DPI
- Accessibility
- Installer/package integration

## Pre-Scoring Notes

Do not assign final scores from README-level evidence alone. A module can be scored after the survey verifies:

- where the Windows feature appears in source,
- whether usage is central or incidental,
- whether existing abstractions already exist,
- and whether at least several repositories share the same requirement shape.

The first source audit batch gives enough evidence for preliminary direction, but not final totals. The next scoring pass should happen after:

- a deeper Notepad++ source pass focused on one subsystem,
- a follow-up search for existing libraries per high-priority module,
- and a quick issue/discussion pass for Linux-demand signals.

## Preliminary Evidence From Source Audit Round 1

| Candidate module | Evidence strength | Seen in source audit | Preliminary direction |
|---|---|---|---|
| File watcher | Strong | ShareX, Files, libuv, wxWidgets; likely Notepad++ follow-up needed | First focused search candidate |
| Process/shell | Strong | Notepad++, WinMerge, ShareX, Greenshot, WinSCP, Files, libuv | First focused search candidate, but split spawn vs desktop-open |
| Dynamic library loading | Strong | Notepad++, WinMerge, AutoHotkey, Rufus, WinSCP, libuv, wxWidgets | First focused search candidate, excluding plugin ABI compatibility |
| Single-instance IPC | Medium to strong | WinMerge, AutoHotkey, WinSCP, libuv; Notepad++ follow-up needed | First focused search candidate if scoped to activation/argument passing |
| Settings/config | Medium | ShareX, Greenshot, Files, Rufus; Notepad++ follow-up needed | Candidate, but split app config from OS integration |
| Filesystem/path helpers | Medium | All audited apps touch it; Files/Rufus show complex non-path semantics | Candidate only with narrow scope |
| Clipboard | Strong but UI-coupled | Notepad++, WinMerge, ShareX, Greenshot, WinSCP, Files, wxWidgets | UI-adjacent; likely toolkit adapter |
| Drag-and-drop | Medium to strong but UI-coupled | ShareX, Greenshot, Files, WinSCP, wxWidgets | UI-adjacent; likely toolkit adapter |
| GUI/windowing | Strong but too broad | Notepad++, WinMerge, AutoHotkey, Rufus, WinSCP, wxWidgets | Do not make first low-level module |
| Printing | Limited but real | Greenshot, wxWidgets; likely Notepad++/WinMerge follow-up | Future work unless POC requires it |
| Device/volume operations | Strong boundary signal | Rufus | Future/application-specific; not first library wave |

## Score Table

| Candidate module | Frequency | Coupling | Linux complexity | Standalone usefulness | Notepad++ POC value | Total | Decision |
|---|---:|---:|---:|---:|---:|---:|---|
| Settings/config | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| File watcher | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| Process/shell | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| Dynamic library loading | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| Filesystem/path helpers | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| Single-instance IPC | TBD | TBD | TBD | TBD | TBD | TBD | Pending source verification |
| GUI/windowing | TBD | TBD | TBD | TBD | TBD | TBD | UI-adjacent; pending source verification |
| Clipboard | TBD | TBD | TBD | TBD | TBD | TBD | UI-adjacent; pending source verification |
| Drag-and-drop | TBD | TBD | TBD | TBD | TBD | TBD | UI-adjacent; pending source verification |
| Common dialogs/resources | TBD | TBD | TBD | TBD | TBD | TBD | UI-adjacent; pending source verification |
| Printing | TBD | TBD | TBD | TBD | TBD | TBD | Future work unless survey raises priority |
