# ld_paths Roadmap

`ld_paths` is the next planned module after the current `ld_settings` and `ld_watch` work. It exists because the extended survey found repeated path-resolution code in Notepad++, OpenRGB, PrusaSlicer, OpenSCAD, FreeCAD, Carla, NUT, Project Island, and smaller challenge candidates.

The goal is a community-facing prototype that feels useful, boring, and honest: broad enough to hand to other developers, but scoped enough that it does not pretend to be a settings engine, desktop integrator, process launcher, or plugin host.

## Design Position

`ld_paths` should resolve path families and explain how it resolved them.

It should not own application payloads. A caller may use `ld_paths` to find `config`, `cache`, `state`, `documents`, `resources`, or `vst3` search roots, but parsing JSON/XML/INI, copying model files, migrating old settings, registering file types, or loading plugins remains outside this module.

## Platform Promise

First prototype:

- Windows 10/11.
- Ubuntu LTS.
- Other Linux distributions best-effort when they follow XDG behavior.
- No macOS promise in phase one, but avoid API choices that make later macOS support awkward.

## Milestones

### Milestone 0: Survey And API Sketch

Status: completed enough to start implementation.

Deliverables:

- `docs/survey/ld-paths-application-audit.md`.
- `docs/plan/ld-paths-roadmap.md`.
- updated README and library roadmap positioning.
- migration examples that show both application adoption and internal extraction from `ld_settings`.

Exit bar:

- path terminology is recorded in `CONTEXT.md`,
- first API surface is explicitly scoped,
- and non-goals are documented before implementation begins.

### Milestone 1: Resolver Core

Status: implemented for the Linux resolver core, deterministic test hooks, deterministic Windows environment tests, and hosted-runner Known Folder selection; deeper Windows verification remains pending.

Implement:

- `ld_core` diagnostics reused from existing modules,
- `ld_paths` CMake library target,
- `LinuxDesktop2026::ld_paths` namespaced target,
- app identity,
- resolver options,
- standard root report for config, data, state, cache, temp, documents, desktop, downloads, music, pictures, videos, templates, and public share,
- executable path and executable directory,
- resource root and install prefix options,
- source-labeled candidate reporting,
- Linux XDG Base Directory behavior,
- Windows Known Folder behavior in the public model,
- and deterministic test hooks.

Exit bar:

- Linux unit tests cover XDG env variables, unset variables, relative invalid overrides, `$HOME` fallback, missing home diagnostics, XDG user-dir parsing, malformed/relative user-dir diagnostics, executable roots, resource roots, install prefix derivation, temp roots, and source-labeled candidates.
- Windows public model includes environment and Known Folder sources. CI covers deterministic APPDATA/LOCALAPPDATA behavior and hosted-runner Known Folder selection; Windows tests or a verification checklist still need to cover executable path, UTF-8 paths, unavailable-folder diagnostics, and fallback behavior when Known Folders are unavailable.
- The demo prints a resolver report without touching real user config.

### Milestone 2: Directory Creation And Path Lists

Status: implemented for the C++ prototype.

Implement:

- opt-in `ensure_directory` helpers that operate on explicit paths or resolved path families,
- parent creation diagnostics,
- dry-run directory creation preview,
- platform path-list separator parsing,
- path-list joining,
- invalid/relative entry filtering policy,
- duplicate normalization where possible,
- and environment override reports through source-labeled path-list candidates.

Exit bar:

- no path is created unless the caller asks for creation,
- path-list behavior is testable without mutating the real environment,
- and diagnostics identify rejected relative, empty, duplicate, file-in-place, missing-parent, and creation-failure cases.

### Milestone 3: User Dirs And Legacy Fallbacks

Status: implemented for the C++ and C resolver reports on Linux; real Windows 10/11 verification remains pending.

Implement:

- XDG `user-dirs.dirs` parsing for Documents, Desktop, Downloads, Music, Pictures, Videos, Templates, and Public,
- fallback behavior when `user-dirs.dirs` is missing, malformed, or points to relative paths,
- Windows equivalent folder resolution,
- legacy fallback chain modeling for configured absolute legacy config files,
- explicit config directory candidate chains shaped by NUT and OpenRGB evidence through XDG user, legacy, and site-default candidates,
- and source labels such as explicit option, environment variable, XDG user dir, known folder, legacy file, site default, executable relative, and fallback.

Exit bar:

- OpenSCAD-style document path logic can be represented without app-specific branches,
- NUT-style explicit/user/legacy/site search order can be reported,
- and the report lets callers show users why a path won.

### Milestone 4: Typed Plugin Path Sets

Status: implemented for the C++ prototype; platform-default verification beyond Ubuntu-style paths is still pending.

Implement:

- typed path sets for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX,
- platform defaults for Linux and Windows,
- environment variable overrides,
- Wine-prefix-aware defaults where relevant,
- path-list diagnostics,
- and custom named path sets for app-defined ecosystems.

Exit bar:

- Carla-style plugin search roots can be represented without hard-coding Carla itself,
- unsupported plugin ecosystems can be represented as caller-defined sets,
- and plugin path discovery remains separate from plugin loading and plugin ABI.

### Milestone 5: Public Prototype Polish

Status: implemented for packaging, examples, C ABI smoke coverage, and docs; still blocked from broad community announcement by Milestone 3 depth and real Windows verification.

Implement:

- small C ABI for root reports, candidate reports, path-list parsing, and plugin path sets,
- C++ and C examples,
- install/export package files,
- install-tree consumer test for `LinuxDesktop2026::ld_paths`,
- README quick-start section,
- API stability notes for `ld_paths`,
- and a compatibility matrix for Windows 10/11 and Ubuntu LTS.

Exit bar:

- a downstream CMake project can link `LinuxDesktop2026::ld_paths`,
- C callers can allocate, inspect, and free reports through documented ownership rules,
- examples do not depend on a developer's real home directory,
- and unsupported platform behavior is reported, not hidden.

Remaining before public prototype announcement:

- run the Windows 10/11 UTF-8 path, executable-root, unavailable-folder, Known Folder fallback, and plugin-default verification checklist,
- and decide whether custom plugin path sets need C ABI exposure in the first public cut.

Windows compatibility rule: keep callers on `ld_paths` root families, source labels, and owned C reports. Do not make tests, demos, `ld_settings`, or future `ld_watch` code depend on XDG-only variables, slash-separated raw strings, or Windows Known Folder paths directly when an `ld_paths` concept can express the same requirement.

## Proposed API Shape

Sketch only:

```cpp
namespace linuxdesktop::paths {

struct app_identity {
    std::string organization;
    std::string application;
};

enum class path_family {
    config,
    data,
    state,
    cache,
    temp,
    documents,
    desktop,
    downloads,
    music,
    pictures,
    videos,
    templates,
    public_share,
    executable,
    install_prefix,
    resources,
    plugin_search,
    runtime
};

enum class candidate_source {
    explicit_option,
    environment,
    xdg_base_dir,
    xdg_user_dir,
    known_folder,
    executable_relative,
    legacy,
    site_default,
    fallback
};

struct path_candidate {
    path_family family;
    candidate_source source;
    std::filesystem::path path;
    bool selected;
    std::vector<diagnostic> diagnostics;
};

struct resolver_options {
    std::optional<std::filesystem::path> config_override;
    std::optional<std::filesystem::path> data_override;
    std::optional<std::filesystem::path> state_override;
    std::optional<std::filesystem::path> cache_override;
    std::optional<std::filesystem::path> temp_override;
    std::optional<std::filesystem::path> runtime_override;
    std::optional<std::filesystem::path> resource_root;
    std::optional<std::filesystem::path> install_prefix;
    std::optional<std::filesystem::path> executable_path;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> environment;
    std::vector<std::filesystem::path> legacy_config_files;
    bool use_process_environment;
};

struct resolver_report {
    std::map<path_family, std::filesystem::path> selected;
    std::vector<path_candidate> candidates;
    std::vector<diagnostic> diagnostics;
};

resolver_report resolve_app_paths(app_identity identity, resolver_options options);

}
```

The final API does not need to use these exact names. The important commitments are source-labeled candidates, selected paths by family, explicit diagnostics, and no hidden filesystem mutation.

## Relationship To Existing Modules

`ld_settings` kept its internal root resolver until `ld_paths` had tests,
examples, and install-tree consumer coverage. Task 04 performs the extraction:

- `ld_paths` resolves config/data/state/cache/resource/runtime roots.
- `ld_settings` consumes those roots to hydrate config bundles and write settings safely. Migration planning moves to `ld_migration`.
- `ld_desktop` owns autostart and policy effects; `ld_settings::effects` remains only as a pre-1.0 compatibility facade.
- shared diagnostics remain in `ld_core`.

`ld_watch` should accept normal path values produced by `ld_paths`, but it should not depend on `ld_paths` for core watcher behavior.

Future `ld_process`, `ld_ipc`, `ld_dynlib`, and desktop integration modules can reuse path families, path lists, executable roots, resource roots, and plugin path sets without redefining them.

## Risks

- Too much scope: plugin path sets can drag the module toward plugin hosting. Keep the boundary at search roots.
- Too little scope: config/cache/state only would duplicate `ld_settings` and miss OpenSCAD, FreeCAD, and Carla evidence.
- Silent platform differences: every fallback or unavailable folder must be visible in reports.
- Early refactor risk: making `ld_settings` depend on `ld_paths` before `ld_paths` is tested would destabilize the working sample.

## First Implementation Checklist

- Add `include/linuxdesktop/paths.hpp`.
- Add `src/paths.cpp`.
- Add `tests/paths_tests.cpp`.
- Add `examples/paths_demo.cpp`.
- Add `ld_paths` and `LinuxDesktop2026::ld_paths` to CMake.
- Reuse `ld_core` diagnostics.
- Add deterministic environment/executable/user-dir test seams.
- Add install-tree consumer coverage.
- Maintain the existing C ABI reports where practical, but defer further C ABI
  expansion until release-candidate status.
