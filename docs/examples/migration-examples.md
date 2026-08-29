# Migration Examples

These examples show the intended shape of application changes when a project adopts LinuxDesktop2026 modules.

They are not full ports and they are not copied source files. Each "before" block is a short, source-anchored reconstruction of the surrounding control flow found during the survey. Each "after" block performs the same app-level task with the LinuxDesktop2026 module that owns the relevant policy.

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
    options.portable = ld::portable_level::profile;

    // CHANGE: Program Files/UAC-style portable denial is now a root policy.
    // KEEP: Notepad++ still decides which install roots count as privileged for
    // its compatibility rules and installer layout.
    options.deny_portable_root_in_privileged_install = true;
    options.privileged_install_roots = detect_privileged_install_roots();

    // KEEP: Notepad++ still owns the cloud-choice file format and validation.
    // CHANGE: once the app has an absolute sync directory, ld_settings can move
    // config/plugin config there while keeping state/session local.
    options.sync_config_override = readCloudChoiceIfPresentAndValid();

    // KEEP: command-line parsing remains app-owned.
    // CHANGE: a valid override becomes the active config/data/state root and
    // intentionally wins over portable and sync config roots.
    options.settings_override = getCommandLineSettingsDirIfPresent();

    // CHANGE: Notepad++ path families become named roots instead of repeated
    // string concatenation around _userPath/_sessionPath/_userPluginConfDir.
    options.named_roots = {
        {"logs", ld::root_purpose::logs,
            ld::persistence_class::machine_local, "logs", true},
        {"profiles", ld::root_purpose::profiles,
            ld::persistence_class::roaming, "profiles", true},
        {"backup", ld::root_purpose::backup,
            ld::persistence_class::machine_local, "backup", true},
        {"plugin-config", ld::root_purpose::plugin_config,
            ld::persistence_class::roaming, "plugins/Config", true},
    };

    // CHANGE: plugins can ask for component-scoped config/state roots without
    // hard-coding Linux or Windows directory layouts.
    ld::component_root_request compare_plugin;
    compare_plugin.name = "ComparePlugin";
    compare_plugin.kind = ld::component_kind::plugin;
    compare_plugin.roots = {
        {"config", ld::root_purpose::component_config,
            ld::persistence_class::roaming, "Config", true},
        {"state", ld::root_purpose::component_state,
            ld::persistence_class::machine_local, "State", true},
    };
    options.component_roots = {compare_plugin};

    const ld::root_report report = ld::resolve_app_roots(identity, options);

    // KEEP: legacy member names can stay during an incremental port.
    // CHANGE: assignments now come from portable roots instead of Win32 calls.
    _nppPath = report.roots.resources;
    _userPath = report.roots.config;
    _sessionPath = report.roots.session;
    if (const auto* plugin_config = ld::find_named_root(report, "plugin-config"))
        _userPluginConfDir = plugin_config->path;
    else
        _userPluginConfDir = report.roots.plugin_config;

    if (const auto* logs = ld::find_named_root(report, "logs"))
        _logPath = logs->path;

    if (const auto* plugin = ld::find_component_roots(report, "ComparePlugin")) {
        if (const auto* state = ld::find_component_named_root(*plugin, "state"))
            registerPluginStateRoot("ComparePlugin", state->path);
    }

    // CHANGE: the app can now explain the exact read/write layer order.
    for (const auto& layer : report.layers.active_read_order)
        logLayer(ld::to_string(layer.kind), ld::to_string(layer.backend), layer.path);

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

After, the same task hydrates the config bundle, loads the app-owned XML documents, and saves the same ordered files with temp-write/replace plus backup/validation:

```cpp
namespace ld = linuxdesktop::settings;
namespace ldm = linuxdesktop::migration;

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
    // CHANGE: each write goes to a same-directory temp file first, validates
    // before commit when a callback is provided, then atomically replaces the target.
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

The important boundary is that LinuxDesktop2026 does not become Notepad++'s XML engine. It handles the recurring platform-shaped operations: file family hydration, missing-file policy, ordered writes, atomic temp-write/replace, backups, validation-before-commit, and structured errors.

C ABI shape for the same Notepad++ hydration/write task:

```c
#include "linuxdesktop/settings_c.h"

static int validate_notepad_session(const char* path, char* message, size_t message_size, void* user_data)
{
    /* KEEP: validation calls back into Notepad++ session parsing logic. */
    struct NppParameters* npp = (struct NppParameters*)user_data;
    if (npp_can_load_session_xml(npp, path)) {
        return 1;
    }
    snprintf(message, message_size, "Notepad++ session XML did not parse");
    return 0;
}

void hydrate_and_save_config_files(struct NppParameters* npp)
{
    struct ld_settings_config_file files[] = {
        {"langs.xml", "langs.model.xml", 1},
        {"stylers.xml", "stylers.model.xml", 1},
        {"shortcuts.xml", "shortcuts.model.xml", 1},
        {"contextMenu.xml", "contextMenu.model.xml", 0},
    };

    struct ld_settings_hydrate_options hydrate;
    struct ld_settings_hydrate_report hydrated = {0};
    ld_settings_hydrate_options_init(&hydrate);

    /* KEEP: Notepad++ still chooses model and target roots. */
    /* CHANGE: copy-missing and skipped-existing reporting are reusable. */
    hydrate.model_root = npp->install_root_utf8;
    hydrate.target_root = npp->user_root_utf8;
    hydrate.files = files;
    hydrate.file_count = sizeof(files) / sizeof(files[0]);

    if (ld_settings_hydrate_config_bundle(&hydrate, &hydrated)) {
        npp_log_hydration(&hydrated);
    }
    ld_settings_free_hydrate_report(&hydrated);

    struct ld_settings_write_options write_options;
    struct ld_settings_write_report write_report = {0};
    const char* session_xml = npp_serialize_session_xml(npp);
    ld_settings_write_options_init(&write_options);

    /* KEEP: Notepad++ owns save order and XML generation. */
    /* CHANGE: temp-write, backup, replace, and callback validation are shared. */
    write_options.target = npp->session_xml_utf8;
    write_options.content = session_xml;
    write_options.content_size = strlen(session_xml);
    ld_settings_write_with_backup(&write_options, validate_notepad_session, npp, &write_report);
    npp_log_write_result(&write_report);
    ld_settings_free_write_report(&write_report);
}
```

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
namespace ldm = linuxdesktop::migration;

void Program::UpdatePersonalPath()
{
    Sandbox = CLI.IsCommandExist("sandbox");

    ld::root_options options;

    // KEEP: sandbox mode is app policy.
    // CHANGE: directory creation becomes a switch on the reusable resolver.
    options.create_directories = !Sandbox;

    // KEEP: ShareX owns the CLI spelling and marker-file convention.
    // CHANGE: portable root activation is handled by the resolver.
    if (CLI.IsCommandExist("portable", "p")) {
        options.settings_override = PortablePersonalFolder;
        options.portable = ld::portable_level::profile;
    } else {
        options.portable_marker = PortableCheckFilePath;
    }

    // KEEP: legacy registry/config migration is app-owned.
    // CHANGE: once migrated to a path, the path is just an override.
    if (!SystemOptions.PersonalPath.empty() && !CLI.IsCommandExist("portable", "p"))
        options.settings_override = SystemOptions.PersonalPath;
    else if (!CLI.IsCommandExist("portable", "p"))
        options.settings_override = ReadPersonalPathConfigIfAbsolute();

    // CHANGE: ShareX path families become named roots. This keeps the old
    // user-visible behavior but removes Windows-only folder assumptions.
    options.named_roots = {
        {"screenshots", ld::root_purpose::data,
            ld::persistence_class::roaming, "Screenshots", true},
        {"history", ld::root_purpose::state,
            ld::persistence_class::machine_local, "History", true},
        {"logs", ld::root_purpose::logs,
            ld::persistence_class::machine_local, "Logs", true},
        {"image-effects", ld::root_purpose::data,
            ld::persistence_class::roaming, "ImageEffects", true},
    };

    const ld::root_report report = ld::resolve_app_roots(
        {"ShareX", "ShareX"},
        options);

    Portable = report.portable_active;
    CustomPersonalPath = report.roots.config;
    PersonalPathDetectionMethod = explain_detection_method(report);

    // KEEP: app-specific migration can still run after root resolution.
    if (!report.settings_override_active && !report.portable_active)
        MigratePersonalPathConfig(report.roots.config);

    // CHANGE: ld_migration can now represent the file migration as a dry-run
    // plan. The app can show this before executing anything.
    const ldm::migration_plan path_migration = ldm::plan_migration({
        {
            ldm::migration_action_kind::copy_directory,
            "copy legacy personal folder",
            LegacyPersonalFolder,
            report.roots.config,
            false,
            false
        }
    });
    ShowMigrationPreview(path_migration);

    // CHANGE: ld_migration::registry can represent Registry snapshots/imports
    // through C++ calls. The pre-1.0 C ABI remains under ld_settings until
    // release-candidate cleanup.
    // DEFER: real Windows verification decides when this is safe to advertise
    // as shippable Registry migration behavior.

    // KEEP: ShareX decides which files live under the personal root.
    LoadApplicationConfig(report.roots.config / "ApplicationConfig.json");
    LoadTaskSettings(report.roots.config / "Tasks");
    if (const auto* screenshots = ld::find_named_root(report, "screenshots"))
        ScreenshotManager::SetOutputRoot(screenshots->path);
    if (const auto* history = ld::find_named_root(report, "history"))
        HistoryManager::Open(history->path);
}
```

This is why the first module keeps diagnostics as first-class output. A migrated app needs to explain whether a path came from CLI, a portable marker, a legacy settings file, XDG defaults, Windows Known Folders, or an ignored invalid override.

C ABI shape for the same ShareX-style migration preview and explicit execution:

```c
#include "linuxdesktop/settings_c.h"

void preview_or_apply_personal_path_migration(const char* legacy_root, const char* new_root, int user_confirmed)
{
    struct ld_settings_migration_action actions[1] = {0};
    actions[0].kind = LD_SETTINGS_MIGRATION_COPY_DIRECTORY;
    actions[0].name = "copy legacy personal folder";
    actions[0].source_path = legacy_root;
    actions[0].target_path = new_root;

    struct ld_settings_migration_options options;
    struct ld_settings_migration_report report = {0};
    ld_settings_migration_options_init(&options);

    /* CHANGE: first call is a dry-run plan for UI/CLI review. */
    ld_settings_plan_migration(actions, 1, &options, &report);
    sharex_show_migration_preview(&report);
    ld_settings_free_migration_report(&report);

    if (!user_confirmed) {
        return;
    }

    /* KEEP: ShareX decides when user confirmation is enough. */
    /* CHANGE: execution uses the same action list and reports per-action state. */
    options.dry_run = 0;
    report = (struct ld_settings_migration_report){0};
    ld_settings_execute_migration_plan(actions, 1, NULL, &options, &report);
    sharex_log_migration_execution(&report);
    ld_settings_free_migration_report(&report);
}
```

## Example 4: WinSCP-Style Storage Selection

Source anchors:

- WinSCP documents two major configuration stores: the Windows Registry and INI files: https://winscp.net/eng/docs/config
- WinSCP exposes storage selection in preferences, including automatic/registry/INI modes: https://winscp.net/eng/docs/ui_pref_storage

Before, the application branches directly on Windows storage mechanisms:

```cpp
// Representative shape from an app with WinSCP-like storage choices.
ConfigStorage OpenConfigurationStorage()
{
    if (StorageMode == StorageMode::Registry)
        return RegistryStorage(HKEY_CURRENT_USER, L"Software\\Vendor\\App");

    if (StorageMode == StorageMode::IniFile)
        return IniStorage(GetExecutableDirectory() / L"winscp.ini");

    if (StorageMode == StorageMode::Nul)
        return NullStorage();

    return RegistryStorage(HKEY_CURRENT_USER, L"Software\\Vendor\\App");
}
```

After, the same task asks `ld_settings` for the active layers and keeps payload parsing/storage semantics explicit:

```cpp
namespace ld = linuxdesktop::settings;

ConfigStorage OpenConfigurationStorage()
{
    ld::root_options options;

    // KEEP: UI preference names and compatibility modes remain app-owned.
    const auto storage_mode = ReadStorageModeFromCommandLineOrPreferences();

    if (storage_mode == StorageMode::IniFile)
        options.settings_override = GetExecutableDirectory();
    if (storage_mode == StorageMode::Portable)
        options.portable_marker = GetExecutableDirectory() / "winscp.ini";

    const ld::root_report report =
        ld::resolve_app_roots({"WinSCP", "WinSCP"}, options);

    // CHANGE: read precedence is now visible and cross-platform.
    for (const auto& layer : report.layers.active_read_order)
        TraceConfigLayer(ld::to_string(layer.kind), ld::to_string(layer.backend), layer.path);

    if (storage_mode == StorageMode::Nul)
        return NullStorage();

    if (storage_mode == StorageMode::IniFile) {
        const auto* write_layer = report.layers.active_write_layer
            ? &*report.layers.active_write_layer
            : ld::find_config_layer(report.layers, ld::config_layer_kind::user);
        if (write_layer == nullptr)
            throw ConfigError("No writable configuration layer");
        return IniStorage(write_layer->path / "winscp.ini");
    }

    // CHANGE: raw Registry and snapshot operations now have C++ and C ABI
    // entry points.
    // DEFER: Windows verification is still required before we call the
    // registry backend shippable.
    // Linux builds can select file-backed layers instead of pretending HKCU exists.
    return OpenPlatformRegistryOrFileStorage(report.layers);
}
```

This example keeps us honest: `ld_settings` should model storage layers and precedence, but it should not force every app into one universal config parser.

C ABI shape for the same WinSCP-style Registry snapshot/export seam:

```c
#include "linuxdesktop/settings_c.h"

void export_winscp_style_registry_snapshot(void)
{
    struct ld_settings_registry_key root = {
        LD_SETTINGS_REGISTRY_CURRENT_USER,
        "Software\\Vendor\\App",
        LD_SETTINGS_REGISTRY_VIEW_NATIVE,
    };
    struct ld_settings_registry_format_report exported = {0};

    /* CHANGE: the C ABI exposes JSON and .reg snapshot entry points.
       KEEP: the app still decides when Registry storage is active. */
    if (ld_settings_registry_export_tree_json(&root, &exported) && exported.ok) {
        write_portable_snapshot_file(exported.content);
    } else {
        log_registry_snapshot_diagnostics(&exported);
    }
    ld_settings_free_registry_format_report(&exported);
}
```

## Example 5: KeePassXC And PortableApps-Style Roots

Source anchors:

- KeePassXC uses Qt-backed config and has to split config-like data from cache/state-like data: https://github.com/keepassxreboot/keepassxc/blob/develop/src/core/Config.cpp
- PortableApps Launcher has declarative registry key moving support: https://portableapps.com/manuals/PortableApps.comLauncher/ref/launcher.ini/registry.html

Before, a cross-platform app or portable launcher often handles roots and migration in separate ad-hoc systems:

```cpp
// Representative shape from apps that already have some abstraction.
auto settings = QSettings(QSettings::IniFormat, QSettings::UserScope, org, app);
auto cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
auto data_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

PortableRegistry::MoveKeyBeforeRun("HKCU\\Software\\Vendor\\App");
RunApplication();
PortableRegistry::MoveKeyAfterRun("HKCU\\Software\\Vendor\\App");
```

After, the same task uses general-purpose roots and keeps portable Registry snapshots behind explicit preview/execution gates:

```cpp
namespace ld = linuxdesktop::settings;

void BootstrapConfigAndPortableState()
{
    ld::root_options options;
    options.portable_marker = executableDir() / "portable.txt";
    options.portable = ld::portable_level::profile;

    // CHANGE: roaming config and machine-local state/cache are named in one
    // report, even when the app keeps Qt, INI, JSON, or XML for payloads.
    options.named_roots = {
        {"database-settings", ld::root_purpose::config,
            ld::persistence_class::roaming, "config", true},
        {"thumbnails", ld::root_purpose::cache,
            ld::persistence_class::ephemeral, "thumbnails", true},
        {"crash-state", ld::root_purpose::state,
            ld::persistence_class::machine_local, "crash-state", true},
    };

    const ld::root_report report =
        ld::resolve_app_roots({"KeePassXC", "KeePassXC"}, options);

    // KEEP: existing settings engine remains app-owned.
    if (const auto* config = ld::find_named_root(report, "database-settings"))
        OpenExistingConfigEngine(config->path / "keepassxc.ini");

    if (const auto* thumbnails = ld::find_named_root(report, "thumbnails"))
        ThumbnailCache::SetRoot(thumbnails->path);

    // CHANGE: the dry-run migration API can carry the dangerous operation as
    // an inspectable plan instead of running Registry movement implicitly.
    const ldm::migration_plan registry_plan = ldm::plan_migration({
        {
            ldm::migration_action_kind::export_registry,
            "snapshot app registry before portable run",
            {},
            report.roots.state / "registry-snapshot.json",
            true,
            false
        }
    });
    ShowMigrationPreview(registry_plan);

    // CHANGE: JSON/.reg snapshot formats and C ABI entry points now exist, so
    // app-specific before/after-run policy can be wired around explicit execution:
    //
    //   ldm::options execute_options;
    //   execute_options.dry_run = false;
    //   execute_options.allow_dangerous = true;
    //   auto executed = ldm::execute_migration_plan(registry_plan, execute_options);
    //
    // DEFER: Windows verification and rollback evidence are still required
    // before PortableApps-style registry run wrappers are shippable.
    // The current API intentionally does not fake this with path roots.
}
```

The pattern is the same across surveyed repos: keep the app's data format and compatibility policy, but move repeated platform placement, root families, and layer reporting into LinuxDesktop2026 modules.

## Example 6: Notepad++ Root Placement With `ld_paths`

Source anchors:

- Notepad++ initializes executable/current-directory roots and then uses them as resource and fallback settings roots in `PowerEditor/src/Parameters.cpp`.
- The earlier `ld_settings` example keeps XML parsing and config hydration in `ld_settings`, but the reusable path placement now belongs in `ld_paths`.

Before, the same constructor-level path code tends to mix executable discovery, resource roots, settings fallback, and portable markers:

```cpp
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
    _isLocal = doesFileExist((_nppPath + L"\\doLocalConf.xml").c_str());
    _userPath = _isLocal ? _nppPath : getSettingsFolder();
    _sessionPath = _userPath;
    _userPluginConfDir = _userPath + L"\\plugins\\Config";
    return loadConfigFiles();
}
```

After, `ld_paths` owns placement and `ld_settings` owns config files:

```cpp
#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;

bool NppParameters::load()
{
    ldp::resolver_options paths;
    paths.resource_root = DetectInstallRootFromExecutable();
    paths.legacy_config_files = {paths.resource_root.value() / "doLocalConf.xml"};
    paths.config_override = GetCommandLineSettingsDirIfPresent();

    const ldp::resolver_report resolved =
        ldp::resolve_app_paths({"Notepad-plus-plus", "Notepad++"}, paths);

    // CHANGE: executable/resource/config/state/plugin-config placement comes
    // from a path report that can explain every candidate and fallback.
    _nppPath = resolved.selected.at(ldp::path_family::resources);
    _userPath = resolved.selected.at(ldp::path_family::config);
    _sessionPath = resolved.selected.at(ldp::path_family::state);
    _userPluginConfDir = _userPath / "plugins" / "Config";

    // KEEP: settings payload behavior stays in ld_settings and Notepad++.
    lds::hydrate_options hydrate;
    hydrate.model_root = _nppPath;
    hydrate.target_root = _userPath;
    hydrate.files = NotepadConfigModels();
    LogPathCandidates(resolved.candidates);
    LogHydration(lds::hydrate_config_bundle(hydrate));

    return loadConfigFiles();
}
```

`ld_paths` makes the path decision inspectable. Under ADR 0012,
`ld_settings` keeps config bundle hydration and writes; migration plans and
Registry snapshot portability move to `ld_migration`; autostart and policy
effects move to `ld_desktop`.

## Example 7: OpenRGB XDG Config And Autostart Split

Source anchors:

- OpenRGB resolves `APPDATA`, `XDG_CONFIG_HOME`, `$HOME/.config`, and the `OpenRGB` app config directory in `ResourceManager.cpp`.
- OpenRGB writes Linux XDG autostart entries in `AutoStart/AutoStart-Linux.cpp`.

Before, config roots and autostart roots can look like one platform helper even though they are different policies:

```cpp
std::filesystem::path GetConfigurationDirectory()
{
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"))
        return std::filesystem::path(xdg) / "OpenRGB";

    if (const char* home = std::getenv("HOME"))
        return std::filesystem::path(home) / ".config" / "OpenRGB";

    return std::filesystem::current_path();
}

void EnableAutostart()
{
    auto autostart = GetConfigurationDirectory().parent_path() / "autostart";
    WriteDesktopFile(autostart / "OpenRGB.desktop", EscapedExecPath());
}
```

After, `ld_paths` resolves user roots while desktop registration stays a separate effect:

```cpp
namespace ldp = linuxdesktop::paths;
namespace ldm = linuxdesktop::migration;
namespace lds = linuxdesktop::settings;

void StartOpenRGB()
{
    ldp::resolver_options paths;
    const ldp::resolver_report resolved =
        ldp::resolve_app_paths({"OpenRGB", "OpenRGB"}, paths);

    const auto config_root = resolved.selected.at(ldp::path_family::config);
    const auto profile_root = config_root / "profiles";

    // CHANGE: XDG_CONFIG_HOME, HOME fallback, invalid env values, and selected
    // roots are visible in the candidate report.
    LogPathCandidates(resolved.candidates);

    LoadJson(config_root / "OpenRGB.json");
    LoadProfiles(profile_root);

    // DEFER: XDG Autostart file writing is a desktop integration effect.
    // ld_settings may keep autostart temporarily, but ld_paths only gives
    // the data/config roots needed by that later effect.
    lds::write_autostart(BuildOpenRGBAutostartEffect(resolved));
}
```

The split prevents `ld_paths` from becoming a hidden desktop-entry generator.

## Example 8: FreeCAD Environment Overrides

Source anchors:

- FreeCAD resolves `FREECAD_USER_HOME`, `FREECAD_USER_DATA`, `FREECAD_USER_TEMP`, Qt standard locations, executable paths, and install roots in `src/App/ApplicationDirectories.cpp`.

Before, application directory code has to interleave environment overrides, runtime roots, and migration compatibility:

```cpp
ApplicationDirectories dirs;
dirs.user_home = getenvPath("FREECAD_USER_HOME");
dirs.user_data = getenvPath("FREECAD_USER_DATA");
dirs.user_temp = getenvPath("FREECAD_USER_TEMP");

if (dirs.user_data.empty())
    dirs.user_data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

dirs.executable = FindExecutablePath();
dirs.resource_root = GuessResourceRoot(dirs.executable);
MigrateOldConfigIfNeeded(dirs);
```

After, `ld_paths` turns overrides and fallback roots into a report, while migration remains separate:

```cpp
namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;

void InitializeFreeCADDirectories()
{
    ldp::resolver_options paths;
    paths.environment_overrides = {
        {"FREECAD_USER_HOME", ldp::path_family::config},
        {"FREECAD_USER_DATA", ldp::path_family::data},
        {"FREECAD_USER_TEMP", ldp::path_family::temp},
    };
    paths.resource_root = InferFreeCADResourceRoot();

    const auto resolved = ldp::resolve_app_paths({"FreeCAD", "FreeCAD"}, paths);

    App::SetUserConfigRoot(resolved.selected.at(ldp::path_family::config));
    App::SetUserDataRoot(resolved.selected.at(ldp::path_family::data));
    App::SetTempRoot(resolved.selected.at(ldp::path_family::temp));
    App::SetResourceRoot(resolved.selected.at(ldp::path_family::resources));

    // KEEP: versioned migration rules stay with settings/app code.
    const auto migration = ldm::plan_migration(BuildFreeCADMigration(resolved));
    ShowMigrationPreview(migration);
}
```

The important part is not replacing Qt. It is making environment-selected roots, fallback choices, and partial failures visible to users and support logs.

## Example 9: Carla Plugin Path Sets

Source anchors:

- Carla defines platform-specific plugin paths for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX in `source/frontend/carla_shared.py` and `source/frontend/pluginlist/pluginlistdialog.cpp`.

Before, every plugin family tends to carry its own default list and environment parsing:

```cpp
std::vector<std::filesystem::path> vst3Paths()
{
    auto paths = SplitEnv("VST3_PATH");
    paths.push_back("~/.vst3");
    paths.push_back("/usr/lib/vst3");
    paths.push_back("/usr/local/lib/vst3");
    AddWinePrefixVst3Paths(paths);
    return Normalize(paths);
}
```

After, `ld_paths` resolves typed search roots without claiming to load plugins:

```cpp
namespace ldp = linuxdesktop::paths;

void RefreshCarlaPluginSearchRoots()
{
    ldp::plugin_path_options options;
    options.include_wine_prefix_defaults = true;
    options.kinds = {
        ldp::plugin_path_kind::ladspa,
        ldp::plugin_path_kind::dssi,
        ldp::plugin_path_kind::lv2,
        ldp::plugin_path_kind::vst2,
        ldp::plugin_path_kind::vst3,
        ldp::plugin_path_kind::clap,
        ldp::plugin_path_kind::sf2,
        ldp::plugin_path_kind::sfz,
        ldp::plugin_path_kind::jsfx,
    };

    const ldp::plugin_path_report report =
        ldp::resolve_plugin_path_sets(options);

    for (const auto& set : report.sets)
        PluginScanner::SetSearchRoots(set.kind, set.paths);

    LogPathCandidates(report.candidates);

    // KEEP: scanning, binary loading, plugin ABI, sandboxing, and host policy
    // remain Carla or future ld_dynlib/plugin-host work.
}
```

This is the clearest reason `ld_paths` cannot stop at config/cache/state roots.

## Example 10: Extract `ld_settings` Root Logic Into `ld_paths`

Source anchors:

- The current LinuxDesktop2026 `ld_settings` sample resolves roots directly before hydrating bundles, writing config, and planning migrations.
- The `ld_paths` roadmap says to keep that resolver until `ld_paths` has tests and install-tree consumer coverage.

Before extraction, `ld_settings` owns both placement and settings behavior:

```cpp
namespace linuxdesktop::settings {

root_report resolve_app_roots(app_identity identity, root_options options)
{
    root_report report;
    report.roots.config = ResolveXdgOrKnownFolderConfig(identity, options);
    report.roots.data = ResolveXdgOrKnownFolderData(identity, options);
    report.roots.state = ResolveStateRoot(identity, options);
    report.roots.cache = ResolveCacheRoot(identity, options);
    report.roots.resources = ResolveResourceRoot(options);
    report.layers = BuildConfigLayers(report.roots, options);
    return report;
}

}
```

After extraction, `ld_settings` asks `ld_paths` for placement and keeps settings-specific work:

```cpp
namespace linuxdesktop::settings {

root_report resolve_app_roots(app_identity identity, root_options options)
{
    paths::resolver_options path_options;
    path_options.config_override = options.settings_override;
    path_options.resource_root = options.resource_root;
    path_options.legacy_config_files = BuildLegacyMarkers(options);

    const paths::resolver_report paths =
        paths::resolve_app_paths(ToPathIdentity(identity), path_options);

    root_report report;
    report.roots.config = paths.selected.at(paths::path_family::config);
    report.roots.data = paths.selected.at(paths::path_family::data);
    report.roots.state = paths.selected.at(paths::path_family::state);
    report.roots.cache = paths.selected.at(paths::path_family::cache);
    report.roots.resources = paths.selected.at(paths::path_family::resources);
    report.diagnostics = MergeDiagnostics(paths.diagnostics, SettingsDiagnostics(options));
    report.layers = BuildConfigLayers(report.roots, options);
    return report;
}

}
```

The extraction is deliberately delayed until `ld_paths` has its own resolver tests, demo, CMake target, and install-tree consumer test.

## What These Examples Prove

- LinuxDesktop2026 modules should expose policy-level operations, not raw wrappers around `GetModuleFileName`, `SHGetFolderPath`, `CreateDirectory`, `CopyFile`, or XDG environment parsing.
- `ld_paths` should own path families, path lists, executable/resource roots, candidate reports, and typed plugin path sets.
- `ld_settings` should own config bundles and writes. Generic root policy belongs to `ld_paths`; migration behavior and app-settings Registry portability belong to `ld_migration`; autostart, policy, and other desktop integration effects belong to `ld_desktop`.
- Users should see small application changes: declare identity, declare overrides/portable markers, inspect path and layer reports, hydrate a bundle, write with backup/validation, then keep app-specific parsing in the app.
- AI agents should have enough surrounding code to recognize the migration seam and propose a safe incremental patch instead of trying to port an entire GUI at once.
