# Migration Examples

These examples show the intended shape of application changes when a project
adopts LinuxDesktop2026 modules.

They are not full ports and they are not copied source files. Each example is a
short, source-anchored reconstruction of the surrounding control flow exercised
by `docs/FlavorTests`. The goal is to show where a LinuxDesktop2026 call reads
like a natural platform-mechanism extraction, and where it should stay out of
the way.

The comments are part of the example style:

- `CHANGE`: replace repeated platform mechanics with a LinuxDesktop2026 call.
- `KEEP`: keep product behavior, file formats, UI policy, and public vocabulary
  in the application.
- `NOTE`: use the simplest natural product code when a helper would add
  framework tax.
- `DEFER`: record an edge case, but do not pretend the current module solves it.

## Reading These Examples

The FlavorTests now give stronger evidence than the early survey sketches:

- `write_common_config()` is a good fit for ordinary durable config saves where
  the app owns the target path, payload, and validation.
- `write_with_backup()` should remain available for lower-level seams such as
  OBS C APIs and Notepad++ session backup restore.
- `request_builder` is useful in Notepad++, qBittorrent, and KiCad, where
  a chain of app identity, resource root, portable marker, environment, and
  named-root declarations improves scanning.
- Direct `root_options` or `ld_paths::resolver_options` are still more natural
  for products with strong existing root policy, such as KeePassXC, FreeCAD,
  Walnut, and OpenIPC Dashboard.
- Migration plans should normally stay inside adapters. Product-facing methods
  should return product-shaped migration decisions.
- Rich LinuxDesktop2026 diagnostics should normally be translated before they
  leave the adapter boundary.

When a usage does not feel natural, do not contort the product around the tool.
That is evidence to either improve the API later or avoid the abstraction at
that seam.

## Example 1: Notepad++ Settings Roots

FlavorTest anchor:

- `docs/FlavorTests/notepadpp/src/notepadpp_flavor.cpp`

Before, the app owns both policy and Windows implementation:

```cpp
bool NppParameters::load()
{
    _isLocal = doesFileExist((_nppPath + L"\\doLocalConf.xml").c_str());
    _userPath = _isLocal ? _nppPath : getSettingsFolderFromAppData();
    _sessionPath = _userPath;
    _userPluginConfDir = _userPath + L"\\plugins\\Config";
    createMissingSettingsDirectories();
    return loadConfigFiles();
}
```

After, the startup seam declares product policy and lets `ld_settings` resolve
roots, layers, directories, and diagnostics:

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;

bool NppParameters::load()
{
    const ld::root_report report = ld::request_builder()
        .app("Notepad-plus-plus", "Notepad++")
        .resource_root(detect_install_root_from_executable())
        .portable_marker(detect_install_root_from_executable() / "doLocalConf.xml")
        .portable(ld::portable_level::profile)
        .deny_portable_root_in_privileged_install(true)
        .privileged_install_roots(detect_privileged_install_roots())
        .sync_config_override(read_cloud_choice_if_present_and_valid())
        .settings_override(get_command_line_settings_dir_if_present())
        .named_root(ld::make_plugin_config_root_request(
            "plugin-config", ld::ownership_kind::user_roaming, "plugins/Config"))
        .named_root(ld::make_log_root_request(
            "logs", ld::ownership_kind::user_local, "logs"))
        .resolve();

    // KEEP: legacy member names can stay during an incremental port.
    _nppPath = report.roots.resources;
    _userPath = report.roots.config;
    _sessionPath = report.roots.session;

    if (const auto* plugin_config = ld::find_named_root(report, "plugin-config"))
        _userPluginConfDir = plugin_config->path;

    // KEEP: diagnostics are translated to Notepad++ startup logging/warnings.
    for (const auto& diagnostic : report.diagnostics)
        log_startup_warning(to_notepad_warning(diagnostic));

    return loadConfigFiles();
}
```

The useful abstraction is not "replace `SHGetFolderPath`." It is "resolve the
active settings root from install resources, portable marker, command-line
override, cloud config, plugin config, state, and diagnostics."

## Example 2: Notepad++ Config Bundle And Saves

FlavorTest anchor:

- `docs/FlavorTests/notepadpp/src/notepadpp_flavor.cpp`

Before, each config file repeats model fallback, copy/generate behavior, parse,
backup, and replace mechanics:

```cpp
bool NppParameters::loadConfigFiles()
{
    ensureModelOrGenerate(_userPath / "shortcuts.xml",
        _nppPath / "shortcuts.model.xml");
    ensureModelOrGenerate(_userPath / "contextMenu.xml",
        _nppPath / "contextMenu.model.xml");

    if (!load_shortcuts_xml(_userPath / "shortcuts.xml"))
        return false;

    if (!load_session_xml(_sessionPath / "session.xml"))
        restore_session_backup_or_start_empty(_sessionPath / "session.xml");

    return true;
}
```

After, hydration and common saves are shared, while XML semantics remain
Notepad++ code:

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;

bool NppParameters::loadConfigFiles()
{
    const auto hydrated = settings::ensure_config_defaults({
        _nppPath,
        _userPath,
        {
            {"langs.xml", "langs.model.xml", true},
            {"stylers.xml", "stylers.model.xml", true},
            {"shortcuts.xml", "shortcuts.model.xml", true},
            {"contextMenu.xml", "contextMenu.model.xml", false},
        },
    });

    log_hydration(to_notepad_hydration(hydrated));

    // KEEP: parsing, merging, shortcuts, and session semantics are product code.
    load_langs_xml(_userPath / "langs.xml");
    load_stylers_xml(_userPath / "stylers.xml");
    load_shortcuts_xml(_userPath / "shortcuts.xml");

    if (!load_session_xml(_sessionPath / "session.xml"))
        restore_session_backup_or_start_empty(_sessionPath / "session.xml");

    return true;
}

bool NppParameters::saveShortcuts()
{
    auto validate = [](const std::filesystem::path& path, std::string& error) {
        return notepad_can_parse_shortcuts(path, error);
    };

    const auto saved = settings::write_common_config(
        {_userPath / "shortcuts.xml", render_shortcuts_xml(), true},
        validate);

    return saved.ok;
}
```

`write_common_config()` is the natural helper for normal Notepad++ saves.

```cpp
void NppParameters::restoreSessionFromBackup()
{
    // NOTE: keep the lower-level API here. This branch deliberately restores
    // from an existing .bak file and should not create another backup through
    // the common save facade.
    ld::write_options options;
    options.target = _sessionPath / "session.xml";
    options.content = read_backup_session();
    options.keep_backup = false;

    (void)ld::write_with_backup(options, can_load_session_xml);
}
```

## Example 3: Audacity FileConfig Flush

FlavorTest anchor:

- `docs/FlavorTests/audacity/src/audacity_flavor.cpp`

Audacity is the cleanest positive save example. Its `FileConfig::Flush()` method
is already a persistence boundary, so the common helper names exactly what is
being replaced:

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;

FlushResult FileConfig::Flush()
{
    // KEEP: Audacity owns dirty-state checks and config serialization.
    if (!dirty_)
        return {true, {}};

    auto validate = [](const std::filesystem::path& path, std::string& error) {
        return config_stream_is_readable(path, error);
    };

    const auto report = settings::write_common_config(
        {local_filename_, serialize(), true},
        validate);

    // KEEP: callers see Audacity vocabulary, not a LinuxDesktop2026 report.
    if (report.ok)
        dirty_ = false;

    return to_flush_result(report);
}
```

This is the shape to prefer for ordinary validated config writes: product path
and product validation in, product result out.

## Example 4: qBittorrent Profile Roots

FlavorTest anchor:

- `docs/FlavorTests/qbittorrent/src/qbittorrent_flavor.cpp`

qBittorrent is a good root-builder example because its profile rules are still
visible after the refactor:

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;

void Profile::init(const RuntimeEnvironment& environment)
{
    if (command_line_profile_root_) {
        // KEEP: qBittorrent policy says --profile wins outright.
        setProfileRoot(*command_line_profile_root_);
        return;
    }

    const auto executable_root = environment.executable_path.parent_path();
    const auto portable_marker = executable_root / "profile";

    const ld::root_report report = ld::request_builder()
        .app("qBittorrent", configuration_name())
        .resource_root(executable_root)
        .home_directory(environment.home)
        .environment(environment.variables)
        .portable_marker(portable_marker)
        .portable(ld::portable_level::profile)
        .named_root(ld::make_log_root_request(
            "logs", ld::ownership_kind::user_local, "logs"))
        .resolve();

    setProfileRoot(report.roots.config);
    setFastResumeRoot(report.roots.data / "BT_backup");

    if (const auto* logs = ld::find_named_root(report, "logs"))
        setLogRoot(logs->path);
}
```

The split matters: the builder removes request boilerplate, but it does not
swallow qBittorrent's command-line precedence or `SpecialFolder` vocabulary.

## Example 5: KeePassXC Local Config Migration

FlavorTest anchor:

- `docs/FlavorTests/keepassxc/src/keepassxc_flavor.cpp`

Before, a migration check can be tempted to return the generic plan directly:

```cpp
linuxdesktop::migration::migration_plan Config::migrateOldLocalConfig()
{
    return linuxdesktop::migration::plan_move_file(old_cache_ini, new_state_ini);
}
```

That passes mechanics through the product boundary. The FlavorTest now keeps the
raw plan private and returns a KeePassXC-shaped decision:

```cpp
namespace ldm = linuxdesktop::migration;

LocalConfigMigration Config::migrateOldLocalConfig()
{
    const auto plan = ldm::plan_move_file(old_cache_ini(), local_state_ini());

    LocalConfigMigration result;
    result.available = !plan.actions.empty();
    result.dry_run = plan.dry_run;
    result.source = old_cache_ini();
    result.target = local_state_ini();
    result.action_name = "move KeePassXC local settings";
    result.prompt_user = result.available && target_is_empty();
    result.blocked = has_error(plan.diagnostics);
    return result;
}
```

`plan_move_file()` is useful here, but only inside the adapter. Callers should
not need to understand `migration_action_kind` to decide whether to show a
KeePassXC prompt.

## Example 6: PrusaSlicer Config Defaults And Old Datadir

FlavorTest anchor:

- `docs/FlavorTests/prusaslicer/src/prusaslicer_flavor.cpp`

PrusaSlicer has two natural LinuxDesktop2026 seams: shipped config hydration and
dry-run migration planning.

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;
namespace ldm = linuxdesktop::migration;

LoadBundleResult load_config_bundle(const AppConfig& app_config)
{
    settings::hydrate_options defaults;
    defaults.model_root = app_config.resources / "profiles";
    defaults.target_root = app_config.config_dir;
    defaults.files = {
        {"vendor.ini", "vendor.ini", true},
        {"print.ini", "print.ini", true},
        {"filament.ini", "filament.ini", false},
    };

    const auto hydrated = settings::ensure_config_defaults(defaults);

    // KEEP: profile parsing, merging, and vendor metadata stay PrusaSlicer code.
    return parse_prusa_profiles(app_config.config_dir, hydrated);
}

OldDatadirMigration check_old_linux_datadir(const AppConfig& config)
{
    ldm::options options;
    options.dry_run = true;
    options.overwrite_existing = false;

    const auto plan =
        ldm::plan_copy_directory(config.old_linux_datadir, config.config_dir, options);

    // KEEP: translate to a PrusaSlicer prompt decision before returning.
    return to_old_datadir_migration(plan, "copy PrusaSlicer legacy config");
}
```

The helper earns its place because it removes repeated copy/backup mechanics
without owning profile content or prompt policy.

## Example 7: KiCad Named Roots, But Project Backups Stay KiCad

FlavorTest anchor:

- `docs/FlavorTests/kicad/src/kicad_flavor.cpp`

KiCad naturally uses named roots for colors, toolbars, and ordinary settings:

```cpp
namespace ld = linuxdesktop::root;
namespace settings = linuxdesktop::settings;

SETTINGS_MANAGER::SETTINGS_MANAGER(RuntimeEnvironment environment)
{
    const auto report = ld::request_builder()
        .app("KiCad", "KiCad")
        .home_directory(environment.home)
        .environment(environment.variables)
        .named_root(ld::make_config_root_request(
            "colors", ld::ownership_kind::user_roaming, "colors"))
        .named_root(ld::make_config_root_request(
            "toolbars", ld::ownership_kind::user_roaming, "toolbars"))
        .named_root(ld::make_named_root_request(
            "project-backups", ld::purpose_kind::backup,
            ld::ownership_kind::user_local, "backups"))
        .resolve();

    user_root_ = report.roots.config;
    colors_root_ = required_named_root(report, "colors");
    toolbars_root_ = required_named_root(report, "toolbars");
    backup_root_ = required_named_root(report, "project-backups");
}
```

Project-scoped backups are different:

```cpp
std::filesystem::path SETTINGS_MANAGER::GetBackupRootForProject(const Project& project) const
{
    // NOTE: do not add a generic LinuxDesktop2026 helper just for this.
    // KiCad owns project identity, project-file location, backup-location
    // preferences, and disambiguation rules.
    if (backup_location_ == BackupLocation::ProjectDir)
        return project.file.parent_path() / "backups";

    return backup_root_ / project.stable_backup_key();
}
```

This is a counter-example against overuse: use named roots for repeated platform
placement, but keep product-shaped project policy in product code unless more
flavors repeat the same shape.

## Example 8: OpenRGB Config Roots, JSON Saves, And Autostart

FlavorTest anchor:

- `docs/FlavorTests/openrgb/src/openrgb_flavor.cpp`

OpenRGB has three distinct seams that should stay distinct:

```cpp
namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;
namespace ldd = linuxdesktop::desktop;

void ResourceManager::SetupConfigurationDirectory(RuntimeEnvironment environment)
{
    ldp::resolver_options options;
    options.home_directory = environment.home;
    options.environment = environment.variables;

    const auto paths = ldp::resolve_app_paths({"OpenRGB", "OpenRGB"}, options);

    configuration_directory_ = paths.selected.at(ldp::path_family::config);
    profile_directory_ = configuration_directory_ / "profiles";

    log_candidates(to_openrgb_candidates(paths.candidates));
}

SaveResult write_json_file(std::filesystem::path path, std::string content)
{
    auto validate_json = [](const std::filesystem::path& path, std::string& error) {
        return openrgb_json_object_is_valid(path, error);
    };

    return to_openrgb_save_result(
        lds::write_common_config({path, std::move(content), true}, validate_json));
}

AutostartUpdate set_autostart_enabled(bool enabled)
{
    ldd::autostart_entry entry;
    entry.id = "org.openrgb.OpenRGB";
    entry.display_name = "OpenRGB";
    entry.executable = current_executable();
    entry.arguments = {"--startminimized"};
    entry.enabled = enabled;

    ldd::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;

    return to_openrgb_autostart_update(ldd::apply_autostart(entry, options));
}
```

`write_json_file()` is useful product glue. If JSON object saves repeat across
more products, add a JSON-oriented helper later. Do not push JSON parsing into
`ld_settings` just because this wrapper is short.

## Example 9: OBS C-Shaped Boundaries

FlavorTest anchor:

- `docs/FlavorTests/obs/src/obs_flavor.cpp`

OBS is the strongest counter-example against leaking C++ reports through a
product's public seam:

```cpp
namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;

int os_get_config_path(char* dst, size_t size, const char* name)
{
    const auto root = resolve_config_root_private();
    const auto path = root / name;

    // KEEP: OBS callers still use caller-owned buffers and integer status.
    return copy_to_c_buffer(dst, size, path.string()) ? 0 : -1;
}

int config_save_safe(const char* path, const char* data)
{
    lds::write_options options;
    options.target = path;
    options.content = data;
    options.keep_backup = true;
    options.atomic_replace = true;

    // NOTE: lower-level write_with_backup is intentional. This slice tests
    // whether C-shaped callers can keep their conventions while the private
    // implementation delegates platform mechanics.
    return lds::write_with_backup(options).ok ? 0 : -1;
}
```

The migration lesson is boundary preservation: LinuxDesktop2026 can be a private
implementation detail even when the public API remains plain C.

## Example 10: FreeCAD Environment Overrides

FlavorTest anchor:

- `docs/FlavorTests/freecad/src/freecad_flavor.cpp`

FreeCAD already has strong application vocabulary for startup directories:

```cpp
namespace ldp = linuxdesktop::paths;
namespace ldm = linuxdesktop::migration;

ConfigurationSet ApplicationConfig::build(
    const RuntimeEnvironment& environment,
    const CommandLineOptions& options)
{
    ConfigurationSet config;

    // KEEP: FreeCAD owns this precedence. The environment names are product
    // compatibility, not generic LinuxDesktop2026 policy.
    config.user_home = first_absolute(options.user_home,
        environment.value("FREECAD_USER_HOME"),
        qt_user_home_fallback());
    config.user_app_data = first_absolute(options.user_data,
        environment.value("FREECAD_USER_DATA"),
        qt_app_data_fallback());
    config.user_temp = first_absolute(options.user_temp,
        environment.value("FREECAD_USER_TEMP"),
        system_temp_fallback());
    config.user_parameter = options.user_cfg.value_or(
        config.user_app_data / "user.cfg");

    // NOTE: this resolver call is useful as compatibility evidence only if it
    // improves diagnostics or candidate reporting. Do not replace the natural
    // FreeCAD precedence code with a denser generic request object.
    ldp::resolver_options path_options;
    path_options.config_override = config.user_home;
    path_options.data_override = config.user_app_data;
    path_options.temp_override = config.user_temp;
    (void)ldp::resolve_app_paths({"FreeCAD", "FreeCAD"}, path_options);

    config.deprecated_path_migration =
        to_deprecated_path_migration(ldm::plan_copy_directory(
            deprecated_freecad_path(), config.user_app_data));

    return config;
}
```

`write_common_config()` is still natural for `saveUserParameter()`, because that
method is an ordinary validated XML write. The startup root selection itself is
more readable when FreeCAD's own precedence remains direct.

## Example 11: Walnut Lightweight App Bootstrap

FlavorTest anchor:

- `docs/FlavorTests/walnut/src/walnut_flavor.cpp`

Walnut uses LinuxDesktop2026 only where it needs ordinary paths:

```cpp
namespace ldp = linuxdesktop::paths;

BootstrapPlan ApplicationBootstrap::prepare(
    ApplicationSpecification specification,
    RuntimeEnvironment environment,
    LaunchOptions launch)
{
    BootstrapPlan plan;
    plan.entry_point = select_entry_point(specification, launch);
    plan.renderer = inspect_renderer_capabilities(environment);

    ldp::resolver_options options;
    options.executable_path = environment.executable_path;
    options.resource_root = environment.executable_path.parent_path();
    options.environment = environment.variables;
    options.platform_defaults = generated::platform_path_defaults_for_home(
        environment.home,
        environment.runtime);

    const auto paths = ldp::resolve_app_paths({"Walnut", specification.name}, options);

    plan.resource_root = paths.selected_locations.at(ldp::location_role::resources);
    plan.config_root = paths.selected.at(ldp::path_family::config);
    plan.diagnostics = to_walnut_diagnostics(paths.diagnostics);
    return plan;
}
```

This is negative evidence for `request_builder`. A graphics app bootstrap
with executable-adjacent resources reads more naturally with direct `ld_paths`
options than with settings-oriented root vocabulary.

## Example 12: OpenIPC Dashboard Service Profile

FlavorTest anchor:

- `docs/FlavorTests/openipc_dashboard/src/openipc_dashboard_flavor.cpp`

Dashboard can use `ld_paths` for ordinary desktop defaults:

```cpp
namespace ldp = linuxdesktop::paths;

ApplicationProfile ApplicationProfile::desktop(RuntimeEnvironment environment)
{
    ldp::resolver_options resolver;
    resolver.home_directory = environment.home;
    resolver.environment = environment.variables;
    resolver.platform_defaults = generated::platform_path_defaults_for_home(
        environment.home,
        environment.runtime);

    const auto paths = ldp::resolve_app_paths(
        {"OpenIPC", "OpenIPC Dashboard"},
        resolver);

    ApplicationProfile profile;
    profile.kind = ProfileKind::Desktop;
    profile.config_root = paths.selected.at(ldp::path_family::config);
    profile.data_root = paths.selected.at(ldp::path_family::data);
    profile.diagnostics = to_dashboard_diagnostics(paths.diagnostics);
    return profile;
}
```

The service profile should stay product-owned:

```cpp
ApplicationProfile ApplicationProfile::service(ServiceOptions options)
{
    const auto root = require_absolute_data_root(options.data_root);

    // NOTE: do not flatten this into a generic "data override" helper.
    // One absolute root selects a whole Dashboard service profile with browser,
    // administrator-bootstrap, evidence, analytics, user, module, state, and
    // logging contracts.
    ApplicationProfile profile;
    profile.kind = ProfileKind::Service;
    profile.config_root = root / "config";
    profile.data_root = root / "data";
    profile.evidence_root = root / "evidence";
    profile.analytics_root = root / "analytics";
    profile.user_root = root / "users";
    profile.module_root = root / "modules";
    profile.log_root = root / "logs";
    return profile;
}
```

As a reference case, Dashboard says LinuxDesktop2026 should adapt around Qt
application lifecycle, QSettings mechanics, QML startup, web routing, and event
loop ownership instead of competing with them.

## Example 13: Carla Plugin Path Sets

Survey anchor:

- `docs/survey/ld-paths-application-audit.md`

Some path work is not settings work at all. Plugin hosts need typed search
roots, not config files:

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

    // KEEP: scanning, binary loading, plugin ABI, sandboxing, and host policy
    // remain Carla or future plugin-host work.
}
```

This is why `ld_paths` cannot stop at config/cache/state roots. It also shows
the limit: resolving paths is not the same as loading plugins.

## What These Examples Prove

- LinuxDesktop2026 modules should expose policy-level operations, not raw
  wrappers around `GetModuleFileName`, `SHGetFolderPath`, `CreateDirectory`,
  `CopyFile`, Qt, XDG environment parsing, or desktop-file text.
- `ld_paths` should own path families, candidate reports, executable/resource
  roots, user directories, explicit platform defaults, path lists, and typed
  plugin path sets.
- `ld_settings` should own config-default hydration and config writes. Generic
  root placement increasingly belongs to `ld_paths`; migration behavior belongs
  to `ld_migration`; autostart, policy, and desktop effects belong to
  `ld_desktop`.
- `write_common_config()` has enough positive FlavorTest evidence to be the
  default recommendation for ordinary validated config saves.
- `request_builder` remains experimental. Use it where it clarifies a
  cluster of settings-root mechanics, and avoid it where direct product code is
  clearer.
- Raw migration plans and rich diagnostics should usually stay internal to
  adapters. Product-facing methods should return product-shaped save results,
  migration prompts, warnings, or logs.
- Counter-examples matter. OBS, Walnut, FreeCAD, KiCad project backups, and
  OpenIPC Dashboard service profiles all show places where the simplest natural
  solution is to keep product code in charge and use LinuxDesktop2026 narrowly,
  or not at all.
- FlavorTests must use public consumer mechanisms for deterministic roots.
  Runtime `platform_path_defaults`, C flat default fields, and generated CMake
  target helpers are acceptable; private test-only platform-path helpers are
  misleading integration evidence.
- AI agents should have enough surrounding code to recognize the migration seam
  and propose a safe incremental patch instead of trying to port an entire GUI
  or replace a toolkit-owned lifecycle.
