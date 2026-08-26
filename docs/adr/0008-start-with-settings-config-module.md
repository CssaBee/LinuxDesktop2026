# Start with a settings/config module

The first implementation module will target settings/config and standard paths.

## Decision

Build a small toolkit-neutral C++ module that resolves application roots and manages simple config bundles.

The first sample should prove:

- Linux XDG root resolution,
- Windows Known Folder root resolution,
- app name and organization inputs,
- command-line settings override,
- optional portable marker policy,
- optional privileged-install denial for portable roots,
- optional sync/config override that keeps state and sessions local,
- separate config/data/state/cache/session/plugin roots,
- hydration of missing config files from model files,
- ordered save phases,
- backup plus validation-after-write for one session-like file,
- structured diagnostics,
- and CMake consumption.

## Rationale

The Notepad++ source pass showed that settings/config is not just a key/value store. The useful seam is a settings root resolver plus config bundle manager.

Existing libraries solve pieces:

- XDG Base Directory and Microsoft Known Folders define the root behavior we should adopt.
- `std::filesystem` is good enough for internal path and file operations.
- Qt, GLib, and wxWidgets are good answers for apps already using those frameworks.
- Boost.Nowide may be useful for Windows UTF-8 IO if the standard library path is not enough.

None of them provides the exact small migration-shaped library we want: portable/local mode, model-file hydration, ordered save phases, plugin-facing roots, validation-after-write, and agent-friendly diagnostics without forcing a GUI framework.

## Initial Public Shape

Names are provisional, but the first code sample should orbit these concepts:

```cpp
struct app_identity {
    std::string organization;
    std::string application;
};

struct root_options {
    std::optional<std::filesystem::path> settings_override;
    std::optional<std::filesystem::path> sync_config_override;
    std::optional<std::filesystem::path> portable_marker;
    std::vector<std::filesystem::path> privileged_install_roots;
    bool allow_portable_root = true;
    bool deny_portable_root_in_privileged_install = false;
    bool allow_sync_config_for_portable_root = false;
};

struct app_roots {
    std::filesystem::path resources;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path state;
    std::filesystem::path cache;
    std::filesystem::path session;
    std::filesystem::path plugin_config;
};

struct config_file {
    std::string name;
    std::string model_name;
    bool required;
};
```

Expected first operations:

- resolve roots,
- create required directories,
- hydrate missing config files,
- write a file with backup,
- validate written contents through an app callback,
- and return a report suitable for CLI output, tests, and GUI messages.

## Deferred

Do not include these in the first sample:

- Qt, GLib, or wxWidgets adapters,
- XML parser ownership,
- cloud provider integration beyond app-provided absolute sync roots,
- portals,
- registry editing,
- Notepad++ fork changes,
- file watching,
- plugin ABI compatibility,
- or UI clipboard/drag-and-drop behavior.

## Consequences

This module gives the project a small executable proof before harder Linux desktop seams. It also creates a reusable foundation for the Notepad++ proof case without committing us to a full Notepad++ port strategy yet.
