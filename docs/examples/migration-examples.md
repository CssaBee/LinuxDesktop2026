# Migration Examples

These examples show the intended shape of application changes when a project adopts LinuxDesktop2026 modules.

They are not full ports and they are not copied source files. Each "before" block is a short, source-anchored reconstruction of the surrounding control flow found during the survey. Each "after" block performs the same app-level task with `ld_settings`.

The comments are part of the example style:

- `CHANGE`: replace this platform-specific policy with a LinuxDesktop2026 call.
- `KEEP`: keep this application behavior in the application.
- `DEFER`: record the edge case, but do not pretend the first module solves it yet.

## Example 1: Notepad++ Settings Root Resolution

Source anchors:

- Notepad++ initializes its executable and current-directory roots with Win32 APIs in `NppParameters::NppParameters()`: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L1130
- Notepad++ resolves its normal settings folder through `SHGetFolderPath(CSIDL_APPDATA)` and falls back to the executable directory: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L1243
- Notepad++ later applies local mode, cloud choice, and `-settingsDir` priority in `load()`: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L1271

Before, the app owns both the policy and the Windows implementation:

```cpp
// Representative shape from Notepad++ settings initialization.
// The application decides where resources, user settings, and local mode live.
NppParameters::NppParameters()
{
    wchar_t exe_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    PathRemoveFileSpecW(exe_path);
    _nppPath = exe_path;

    wchar_t current_dir[MAX_PATH] = {};
    GetCurrentDirectoryW(MAX_PATH, current_dir);
    _currentDirectory = current_dir;
}

bool NppParameters::load()
{
    std::wstring local_conf = _nppPath;
    pathAppend(local_conf, L"doLocalConf.xml");

    _isLocal = doesFileExist(local_conf.c_str());
    _userPath = getSettingsFolder();
    _sessionPath = _userPath;
    _userPluginConfDir = _userPath;
    pathAppend(_userPluginConfDir, L"plugins\\Config");

    createMissingSettingsDirectories();
    return loadConfigFiles();
}

std::wstring NppParameters::getSettingsFolder() const
{
    if (_isLocal)
        return _nppPath;

    auto app_data = getSpecialFolderLocation(CSIDL_APPDATA);
    if (app_data.empty())
        return _nppPath;

    return app_data + L"\\Notepad++";
}
```

After, the same task resolves install resources, active config, session, and plugin-config roots through `ld_settings`:

```cpp
#include "linuxdesktop/settings.hpp"

namespace ld = linuxdesktop::settings;

bool NppParameters::load()
{
    ld::app_identity identity;
    identity.organization = "Notepad-plus-plus";
    identity.application = "Notepad++";

    ld::root_options options;

    // KEEP: the app still decides where its install/resource root is.
    // CHANGE: the library turns that into a stable resources root.
    options.resource_root = detect_install_root_from_executable();

    // KEEP: Notepad++ owns the marker name and local-mode policy.
    // CHANGE: the library handles marker existence, directory selection,
    // XDG/Known Folder defaults, directory creation, and diagnostics.
    options.portable_marker = *options.resource_root / "doLocalConf.xml";

    // DEFER: Notepad++ has extra Program Files/UAC and cloud-choice rules.
    // Keep those as explicit policy checks until ld_settings grows named
    // support for privileged install roots and sync-provider overrides.
    apply_notepad_plus_plus_local_mode_guards(options);

    // KEEP: command-line parsing remains app-owned.
    // CHANGE: a valid override becomes the active config/data/state root.
    options.settings_override = getCommandLineSettingsDirIfPresent();

    const ld::root_report report = ld::resolve_app_roots(identity, options);

    // KEEP: legacy member names can stay during an incremental port.
    // CHANGE: assignments now come from portable roots instead of Win32 calls.
    _nppPath = report.roots.resources;
    _userPath = report.roots.config;
    _sessionPath = report.roots.session;
    _userPluginConfDir = report.roots.plugin_config;

    // KEEP: app logging/UI policy stays app-owned.
    for (const auto& diagnostic : report.diagnostics)
        log_platform_diagnostic(diagnostic);

    // KEEP: loading/parsing Notepad++ config files is still Notepad++ logic.
    return loadConfigFiles();
}
```

The useful abstraction is not "replace `SHGetFolderPath`." It is "resolve the active settings root from install resources, portable marker, command-line override, per-user config, plugin config, state, cache, and diagnostics."

## Example 2: Notepad++ Config Bundle Hydration And Ordered Saves

Source anchors:

- Notepad++ creates or copies per-user XML files such as context menu config before loading them: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L1691
- Notepad++ treats `session.xml` specially and restores a backup when the session file cannot be loaded: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L1731
- Notepad++ copies first-run cloud settings only when destination files are missing: https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/Parameters.cpp#L4135

Before, each config file repeats path construction, model-file fallback, copy/generate behavior, parse, and recovery:

```cpp
// Representative shape from the Notepad++ XML config family.
std::wstring context_menu = _userPath;
pathAppend(context_menu, L"contextMenu.xml");

if (!doesFileExist(context_menu.c_str()))
{
    std::wstring model = _nppPath;
    pathAppend(model, L"contextMenu.xml.model");

    if (doesFileExist(model.c_str()))
        CopyFileW(model.c_str(), context_menu.c_str(), TRUE);
    else
        generateXmlFromScratch(context_menu.c_str(), default_context_menu_xml);
}

auto* document = new NppXml::NewDocument();
if (!NppXml::loadFileContextMenu(document, context_menu.c_str()))
{
    delete document;
    document = nullptr;
}

std::wstring session = _sessionPath;
pathAppend(session, L"session.xml");
if (!loadSession(session))
    restoreBackupOrStartEmptySession(session);
```

After, the same task hydrates the config bundle, loads the app-owned XML documents, and saves the same ordered files with backup/validation:

```cpp
namespace ld = linuxdesktop::settings;

bool NppParameters::loadConfigFiles()
{
    ld::hydrate_options hydrate;

    // KEEP: Notepad++ decides which model files ship with the app.
    // CHANGE: the library copies missing files and reports skipped/errors.
    hydrate.model_root = _nppPath;
    hydrate.target_root = _userPath;
    hydrate.files = {
        {"langs.xml", "langs.model.xml", true},
        {"stylers.xml", "stylers.model.xml", true},
        {"shortcuts.xml", "shortcuts.model.xml", true},
        {"contextMenu.xml", "contextMenu.model.xml", false},
    };

    const ld::hydrate_report hydrated = ld::hydrate_config_bundle(hydrate);
    log_hydration(hydrated);

    // KEEP: XML parsing and in-memory menu/shortcut models stay in Notepad++.
    // CHANGE: missing-file copy/generate policy is no longer repeated per file.
    if (!load_langs_xml(_userPath / "langs.xml"))
        return false;
    if (!load_stylers_xml(_userPath / "stylers.xml"))
        return false;
    if (!load_shortcuts_xml(_userPath / "shortcuts.xml"))
        return false;
    load_optional_context_menu_xml(_userPath / "contextMenu.xml");

    // KEEP: session recovery semantics are app-owned because Notepad++ knows
    // what a valid session means.
    // CHANGE: the session path comes from state/session roots, not AppData.
    if (!load_session_xml(_sessionPath / "session.xml"))
        restore_session_backup_or_start_empty(_sessionPath / "session.xml");

    return true;
}

bool NppParameters::saveConfigFiles()
{
    // KEEP: the save order is app behavior. The survey found that order matters.
    const auto shortcuts = ld::write_with_backup({
        _userPath / "shortcuts.xml",
        serialize_shortcuts_xml(),
        true
    });

    const auto config = ld::write_with_backup({
        _userPath / "config.xml",
        serialize_config_xml(),
        true
    });

    auto validate_session = [](const std::filesystem::path& path, std::string& error) {
        // KEEP: validation calls back into app parsing logic.
        return app_can_load_session_xml(path, error);
    };

    const auto session = ld::write_with_backup({
        _sessionPath / "session.xml",
        serialize_session_xml(),
        true
    }, validate_session);

    return shortcuts.ok && config.ok && session.ok;
}
```

The important boundary is that LinuxDesktop2026 does not become Notepad++'s XML engine. It handles the recurring platform-shaped operations: file family hydration, missing-file policy, ordered writes, backups, validation-after-write, and structured errors.

## Example 3: ShareX Personal Path Selection

Source anchor:

- ShareX selects a personal folder from sandbox mode, portable CLI flag, portable marker file, registry-backed setting, migrated config file, and directory creation: https://github.com/ShareX/ShareX/blob/develop/ShareX/Program.cs#L447

Before, the app mixes user policy, Windows configuration sources, portable mode, migration, and filesystem creation in one startup function:

```csharp
// Representative shape from ShareX personal path selection.
static void UpdatePersonalPath()
{
    Sandbox = CLI.IsCommandExist("sandbox");

    if (Sandbox)
        return;

    if (CLI.IsCommandExist("portable", "p"))
    {
        Portable = true;
        CustomPersonalPath = PortablePersonalFolder;
        PersonalPathDetectionMethod = "Portable CLI flag";
    }
    else if (File.Exists(PortableCheckFilePath))
    {
        Portable = true;
        CustomPersonalPath = PortablePersonalFolder;
        PersonalPathDetectionMethod = "Portable file";
    }
    else if (!string.IsNullOrEmpty(SystemOptions.PersonalPath))
    {
        CustomPersonalPath = SystemOptions.PersonalPath;
        PersonalPathDetectionMethod = "Registry";
    }
    else
    {
        MigratePersonalPathConfig();
        CustomPersonalPath = ReadPersonalPathConfig();
    }

    Directory.CreateDirectory(PersonalFolder);
}
```

After, the same startup task keeps ShareX-style policy decisions visible while moving root resolution and directory creation into `ld_settings`:

```cpp
namespace ld = linuxdesktop::settings;

void Program::UpdatePersonalPath()
{
    Sandbox = CLI.IsCommandExist("sandbox");

    ld::root_options options;

    // KEEP: sandbox mode is app policy.
    // CHANGE: directory creation becomes a switch on the reusable resolver.
    options.create_directories = !Sandbox;

    // KEEP: ShareX owns the CLI spelling and marker-file convention.
    // CHANGE: portable root activation is handled by the resolver.
    if (CLI.IsCommandExist("portable", "p"))
        options.portable_marker = PortablePersonalFolder / ".force-portable";
    else
        options.portable_marker = PortableCheckFilePath;

    // KEEP: legacy registry/config migration is app-owned.
    // CHANGE: once migrated to a path, the path is just an override.
    if (!SystemOptions.PersonalPath.empty())
        options.settings_override = SystemOptions.PersonalPath;
    else
        options.settings_override = ReadPersonalPathConfigIfAbsolute();

    const ld::root_report report = ld::resolve_app_roots(
        {"ShareX", "ShareX"},
        options);

    Portable = report.portable_active;
    CustomPersonalPath = report.roots.config;
    PersonalPathDetectionMethod = explain_detection_method(report);

    // KEEP: app-specific migration can still run after root resolution.
    if (!report.settings_override_active && !report.portable_active)
        MigratePersonalPathConfig(report.roots.config);

    // KEEP: ShareX decides which files live under the personal root.
    LoadApplicationConfig(report.roots.config / "ApplicationConfig.json");
    LoadTaskSettings(report.roots.config / "Tasks");
}
```

This is why the first module keeps diagnostics as first-class output. A migrated app needs to explain whether a path came from CLI, a portable marker, a legacy settings file, XDG defaults, Windows Known Folders, or an ignored invalid override.

## What These Examples Prove

- The first module should expose policy-level operations, not raw wrappers around `GetModuleFileName`, `SHGetFolderPath`, `CreateDirectory`, `CopyFile`, or XDG environment parsing.
- Users should see small application changes: declare identity, declare overrides/portable markers, hydrate a bundle, write with backup/validation, then keep app-specific parsing in the app.
- AI agents should have enough surrounding code to recognize the migration seam and propose a safe incremental patch instead of trying to port an entire GUI at once. Tiny bites. No heroic yak rodeo.
