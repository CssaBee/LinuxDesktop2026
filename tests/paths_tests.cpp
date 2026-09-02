#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace ld = linuxdesktop::paths;

struct test_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw test_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, std::string_view code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path selected_path(const ld::resolver_report& report, ld::path_family family)
{
    const auto item = report.selected.find(family);
    if (item == report.selected.end()) {
        fail(std::string("missing selected path for ") + std::string(ld::to_string(family)));
    }
    return item->second;
}

std::filesystem::path selected_location(const ld::resolver_report& report, ld::location_role role)
{
    const auto item = report.selected_locations.find(role);
    if (item == report.selected_locations.end()) {
        fail(std::string("missing selected location for ") + std::string(ld::to_string(role)));
    }
    return item->second;
}

bool has_selected_candidate(const ld::resolver_report& report, ld::path_family family, ld::candidate_source source)
{
    for (const auto& candidate : report.candidates) {
        if (candidate.family == family && candidate.source == source && candidate.selected) {
            return true;
        }
    }
    return false;
}

bool has_candidate(const ld::resolver_report& report, ld::path_family family, ld::candidate_source source)
{
    for (const auto& candidate : report.candidates) {
        if (candidate.family == family && candidate.source == source) {
            return true;
        }
    }
    return false;
}

bool has_selected_location_candidate(const ld::resolver_report& report, ld::location_role role, ld::candidate_source source)
{
    for (const auto& candidate : report.location_candidates) {
        if (candidate.role == role && candidate.source == source && candidate.selected) {
            return true;
        }
    }
    return false;
}

bool has_path_list_diagnostic(const ld::path_list_report& report, std::string_view code)
{
    return has_diagnostic(report.diagnostics, code);
}

bool has_plugin_diagnostic(const ld::plugin_path_report& report, std::string_view code)
{
    return has_diagnostic(report.diagnostics, code);
}

const ld::plugin_path_set& plugin_set(const ld::plugin_path_report& report, std::string_view name)
{
    for (const auto& set : report.sets) {
        if (set.name == name) {
            return set;
        }
    }
    fail(std::string("missing plugin path set: ") + std::string(name));
}

ld::resolver_options deterministic_options()
{
    ld::resolver_options options;
    options.use_process_environment = false;
    const auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-paths-fixtures";
    options.home_directory = root / "home";
    options.temp_override = root / "tmp";
    options.executable_path = root / "opt" / "linuxdesktop2026" / "bin" / "paths-tests";
    return options;
}

std::filesystem::path fixture_path(std::initializer_list<std::string_view> parts)
{
    auto path = std::filesystem::temp_directory_path() / "linuxdesktop2026-paths-fixtures";
    for (const auto part : parts) {
        path /= part;
    }
    return path;
}

void exposes_cpp_version()
{
    require(ld::version_major == 0, "C++ version major should match project version");
    require(ld::version_minor == 1, "C++ version minor should match project version");
    require(ld::version_patch == 0, "C++ version patch should match project version");
}

void paths_diagnostics_use_shared_core_vocabulary()
{
    ld::diagnostic paths_diagnostic;
    paths_diagnostic.level = ld::severity::warning;
    paths_diagnostic.code = "shared-diagnostic";

    linuxdesktop::diagnostic core_diagnostic = paths_diagnostic;
    require(core_diagnostic.code == "shared-diagnostic", "paths diagnostics should alias shared diagnostics");
    require(linuxdesktop::to_string(core_diagnostic.level) == "warning", "shared severity should stringify");
    require(ld::to_string(paths_diagnostic.level) == "warning", "paths severity alias should stringify");
}

void stringifies_public_enums()
{
    require(ld::to_string(ld::path_family::config) == "config", "path family should stringify");
    require(ld::to_string(ld::path_family::public_share) == "public_share", "public share should stringify");
    require(ld::to_string(ld::path_family::templates) == "templates", "templates path family should stringify");
    require(ld::to_string(ld::path_family::runtime) == "runtime", "runtime path family should stringify");
    require(ld::to_string(ld::location_role::resources) == "resources", "location role should stringify");
    require(ld::to_string(ld::candidate_source::known_folder) == "known_folder", "candidate source should stringify");
    require(ld::to_string(ld::candidate_source::xdg_base_dir) == "xdg_base_dir", "XDG base dir should stringify");
    require(ld::to_string(ld::candidate_source::platform_default) == "platform_default", "platform default source should stringify");
    require(ld::to_string(ld::candidate_source::wine_prefix) == "wine_prefix", "Wine-prefix source should stringify");
    require(ld::to_string(ld::directory_action::would_create) == "would_create", "directory action should stringify");
    require(ld::to_string(ld::plugin_path_kind::vst3) == "vst3", "plugin path kind should stringify");
    require(ld::to_string(ld::plugin_path_kind::audio_unit) == "audio_unit", "Audio Unit plugin path kind should stringify");
    require(ld::to_string(ld::plugin_asset_path_kind::sf2) == "sf2", "plugin asset path kind should stringify");
    require(ld::to_string(ld::plugin_path_category::toolkit_plugin) == "toolkit_plugin", "plugin path category should stringify");
    require(ld::to_string(ld::platform_support::macos) == "macos", "plugin platform support should stringify");
}

void resolves_linux_xdg_base_directories_from_injected_environment()
{
#if defined(_WIN32)
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.environment["APPDATA"] = fixture_path({"appdata", "roaming"}).string();
    options.environment["LOCALAPPDATA"] = fixture_path({"appdata", "local"}).string();

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == fixture_path({"appdata", "roaming", "LinuxDesktop2026", "paths-tests"}),
        "Windows config path should use APPDATA");
    require(selected_path(report, ld::path_family::data) == fixture_path({"appdata", "roaming", "LinuxDesktop2026", "paths-tests"}),
        "Windows data path should use APPDATA");
    require(selected_path(report, ld::path_family::state) == fixture_path({"appdata", "local", "LinuxDesktop2026", "paths-tests", "state"}),
        "Windows state path should use LOCALAPPDATA");
    require(selected_path(report, ld::path_family::cache) == fixture_path({"appdata", "local", "LinuxDesktop2026", "paths-tests", "cache"}),
        "Windows cache path should use LOCALAPPDATA");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::environment),
        "Windows config candidate should be source-labeled");
#else
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.environment["XDG_CONFIG_HOME"] = fixture_path({"xdg", "config"}).string();
    options.environment["XDG_DATA_HOME"] = fixture_path({"xdg", "data"}).string();
    options.environment["XDG_STATE_HOME"] = fixture_path({"xdg", "state"}).string();
    options.environment["XDG_CACHE_HOME"] = fixture_path({"xdg", "cache"}).string();
    options.environment["XDG_RUNTIME_DIR"] = fixture_path({"xdg", "runtime"}).string();

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == fixture_path({"xdg", "config", "LinuxDesktop2026", "paths-tests"}),
        "config path should use XDG_CONFIG_HOME");
    require(selected_path(report, ld::path_family::data) == fixture_path({"xdg", "data", "LinuxDesktop2026", "paths-tests"}),
        "data path should use XDG_DATA_HOME");
    require(selected_path(report, ld::path_family::state) == fixture_path({"xdg", "state", "LinuxDesktop2026", "paths-tests"}),
        "state path should use XDG_STATE_HOME");
    require(selected_path(report, ld::path_family::cache) == fixture_path({"xdg", "cache", "LinuxDesktop2026", "paths-tests"}),
        "cache path should use XDG_CACHE_HOME");
    require(selected_path(report, ld::path_family::runtime) == fixture_path({"xdg", "runtime", "paths-tests"}),
        "runtime path should use XDG_RUNTIME_DIR");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::xdg_base_dir),
        "XDG config candidate should be source-labeled");
#endif
}

void resolves_home_fallbacks_when_xdg_is_unset()
{
#if defined(_WIN32)
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.environment["APPDATA"] = "";
    options.environment["LOCALAPPDATA"] = "";

    const auto report = ld::resolve_app_paths(identity, options);

    const auto config = selected_path(report, ld::path_family::config);
    const auto state = selected_path(report, ld::path_family::state);
    if (has_selected_candidate(report, ld::path_family::config, ld::candidate_source::known_folder)) {
        require(has_selected_candidate(report, ld::path_family::state, ld::candidate_source::known_folder),
            "Windows state path should use Known Folder when config uses Known Folder");
    } else {
        require(config == fixture_path({"home", "AppData", "Roaming", "LinuxDesktop2026", "paths-tests"}),
            "Windows config path should fall back under HOME AppData when Known Folder is unavailable, got " + config.string());
        require(state == fixture_path({"home", "AppData", "Local", "LinuxDesktop2026", "paths-tests", "state"}),
            "Windows state path should fall back under HOME AppData when Known Folder is unavailable");
        require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::fallback),
            "Windows config fallback candidate should be source-labeled");
    }
#else
    std::error_code ec;
    std::filesystem::remove_all(fixture_path({"home", ".config", "user-dirs.dirs"}), ec);

    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto report = ld::resolve_app_paths(identity, deterministic_options());

    const auto config = selected_path(report, ld::path_family::config);
    require(config == fixture_path({"home", ".config", "LinuxDesktop2026", "paths-tests"}),
        "config path should fall back under HOME, got " + config.string());
    require(selected_path(report, ld::path_family::data) == fixture_path({"home", ".local", "share", "LinuxDesktop2026", "paths-tests"}),
        "data path should fall back under HOME");
    require(selected_path(report, ld::path_family::state) == fixture_path({"home", ".local", "state", "LinuxDesktop2026", "paths-tests"}),
        "state path should fall back under HOME");
    require(selected_path(report, ld::path_family::cache) == fixture_path({"home", ".cache", "LinuxDesktop2026", "paths-tests"}),
        "cache path should fall back under HOME");
    require(selected_path(report, ld::path_family::documents) == fixture_path({"home", "Documents"}),
        "documents path should use stable HOME fallback when XDG user-dirs is missing");
    require(selected_path(report, ld::path_family::templates) == fixture_path({"home", "Templates"}),
        "templates path should use stable HOME fallback when XDG user-dirs is missing");
#endif
}

void resolves_app_paths_from_public_platform_defaults()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.home_directory.reset();
    options.platform_defaults = ld::platform_path_defaults::xdg(
        fixture_path({"platform", "home"}),
        fixture_path({"platform", "runtime"}));

    const auto report = ld::resolve_app_paths(identity, options);

#if defined(_WIN32)
    require(options.platform_defaults->windows_roaming_appdata == std::nullopt,
        "XDG factory should not inject Windows defaults");
#else
    require(selected_path(report, ld::path_family::config) == fixture_path({"platform", "home", ".config", "LinuxDesktop2026", "paths-tests"}),
        "config path should use public XDG platform defaults");
    require(selected_path(report, ld::path_family::data) == fixture_path({"platform", "home", ".local", "share", "LinuxDesktop2026", "paths-tests"}),
        "data path should use public XDG platform defaults");
    require(selected_path(report, ld::path_family::state) == fixture_path({"platform", "home", ".local", "state", "LinuxDesktop2026", "paths-tests"}),
        "state path should use public XDG platform defaults");
    require(selected_path(report, ld::path_family::cache) == fixture_path({"platform", "home", ".cache", "LinuxDesktop2026", "paths-tests"}),
        "cache path should use public XDG platform defaults");
    require(selected_path(report, ld::path_family::runtime) == fixture_path({"platform", "runtime", "paths-tests"}),
        "runtime path should use public XDG platform defaults");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::platform_default),
        "default-derived config candidate should be observable");
    require(has_selected_candidate(report, ld::path_family::runtime, ld::candidate_source::platform_default),
        "default-derived runtime candidate should be observable");
    require(!has_diagnostic(report.diagnostics, ld::diagnostic_code::home_missing),
        "public platform defaults should not be reported as a missing-home resolution");
#endif
}

void platform_defaults_do_not_override_explicit_or_environment_paths()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.config_override = fixture_path({"override", "config"});
    options.environment["XDG_DATA_HOME"] = fixture_path({"env", "data"}).string();
    options.platform_defaults = ld::platform_path_defaults::xdg(
        fixture_path({"platform", "home"}),
        fixture_path({"platform", "runtime"}));

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == fixture_path({"override", "config"}),
        "explicit config override should win over platform defaults");
#if defined(_WIN32)
    (void)report;
#else
    require(selected_path(report, ld::path_family::data) == fixture_path({"env", "data", "LinuxDesktop2026", "paths-tests"}),
        "environment data root should win over platform defaults");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::explicit_option),
        "explicit config candidate should remain selected");
    require(has_selected_candidate(report, ld::path_family::data, ld::candidate_source::xdg_base_dir),
        "environment data candidate should remain selected");
#endif
}

void windows_platform_defaults_precede_known_folders()
{
#if defined(_WIN32)
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.environment["APPDATA"] = "";
    options.environment["LOCALAPPDATA"] = "";
    options.platform_defaults = ld::platform_path_defaults::windows(fixture_path({"platform", "home"}));

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == fixture_path({"platform", "home", "AppData", "Roaming", "LinuxDesktop2026", "paths-tests"}),
        "Windows config path should use injected platform default before Known Folder");
    require(selected_path(report, ld::path_family::data) == fixture_path({"platform", "home", "AppData", "Roaming", "LinuxDesktop2026", "paths-tests"}),
        "Windows data path should use injected platform default before Known Folder");
    require(selected_path(report, ld::path_family::state) == fixture_path({"platform", "home", "AppData", "Local", "LinuxDesktop2026", "paths-tests", "state"}),
        "Windows state path should use injected platform default before Known Folder");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::platform_default),
        "Windows injected config default should be source-labeled");
#endif
}

void rejects_relative_platform_defaults()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    ld::platform_path_defaults defaults;
    defaults.xdg_config_home = "relative-config";
    options.platform_defaults = defaults;

    const auto report = ld::resolve_app_paths(identity, options);

#if defined(_WIN32)
    require(!has_diagnostic(report.diagnostics, ld::diagnostic_code::platform_default_relative_ignored),
        "unused XDG defaults should not affect Windows resolution");
#else
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::platform_default_relative_ignored),
        "relative platform default should be diagnosed");
    require(has_candidate(report, ld::path_family::config, ld::candidate_source::platform_default),
        "relative platform default should be visible as a rejected candidate");
    require(selected_path(report, ld::path_family::config) == fixture_path({"home", ".config", "LinuxDesktop2026", "paths-tests"}),
        "relative platform default should fall through to built-in fallback");
#endif
}

void resolves_xdg_user_dirs_from_config_file()
{
#if defined(_WIN32)
    return;
#else
    const auto base = std::filesystem::temp_directory_path() / "linuxdesktop2026-xdg-user-dirs-tests";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(base / "config");
    {
        std::ofstream file(base / "config" / "user-dirs.dirs");
        file << "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n";
        file << "XDG_DESKTOP_DIR=\"" << (base / "desktop-root").string() << "\"\n";
        file << "XDG_DOWNLOAD_DIR=\"${HOME}/Incoming\"\n";
        file << "XDG_MUSIC_DIR=\"$HOME/Audio\"\n";
        file << "XDG_PICTURES_DIR=\"$HOME/Images\"\n";
        file << "XDG_VIDEOS_DIR=\"$HOME/Movies\"\n";
        file << "XDG_TEMPLATES_DIR=\"$HOME/Document Templates\"\n";
        file << "XDG_PUBLICSHARE_DIR=\"$HOME/Shared\"\n";
    }

    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.home_directory = base / "home";
    options.environment["XDG_CONFIG_HOME"] = (base / "config").string();

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::documents) == base / "home" / "Docs",
        "documents should use XDG user-dirs value");
    require(selected_path(report, ld::path_family::desktop) == base / "desktop-root",
        "absolute XDG user-dirs values should be accepted");
    require(selected_path(report, ld::path_family::downloads) == base / "home" / "Incoming",
        "braced HOME in XDG user-dirs should expand");
    require(selected_path(report, ld::path_family::templates) == base / "home" / "Document Templates",
        "templates should be resolved from XDG user-dirs");
    require(has_selected_candidate(report, ld::path_family::documents, ld::candidate_source::xdg_user_dir),
        "XDG user-dir candidate should be source-labeled");

    std::filesystem::remove_all(base, ec);
#endif
}

void reports_user_dir_legacy_and_site_fallback_candidates()
{
#if defined(_WIN32)
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.legacy_config_files = {fixture_path({"etc", "nut", "ups.conf"}), "relative-legacy.conf"};

    const auto report = ld::resolve_app_paths(identity, options);

    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::legacy_path_relative_ignored),
        "relative legacy paths should be diagnosed");
    require(has_candidate(report, ld::path_family::config, ld::candidate_source::legacy),
        "legacy config file candidates should be reported");
#else
    const auto base = std::filesystem::temp_directory_path() / "linuxdesktop2026-xdg-user-dirs-diagnostics";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(base / "config");
    {
        std::ofstream file(base / "config" / "user-dirs.dirs");
        file << "XDG_DOCUMENTS_DIR=relative-documents\n";
        file << "not a valid line\n";
    }

    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.home_directory = base / "home";
    options.environment["XDG_CONFIG_HOME"] = (base / "config").string();
    options.legacy_config_files = {fixture_path({"etc", "nut", "ups.conf"}), "relative-legacy.conf"};
#if !defined(_WIN32)
    options.environment["XDG_CONFIG_DIRS"] = fixture_path({"site", "xdg"}).string() + ":" + fixture_path({"opt", "xdg"}).string();
#endif

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::documents) == base / "home" / "Documents",
        "relative XDG user-dir should fall back to HOME leaf");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::xdg_user_dir_relative_ignored),
        "relative XDG user-dir should be diagnosed");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::xdg_user_dir_malformed),
        "malformed XDG user-dirs lines should be diagnosed");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::legacy_path_relative_ignored),
        "relative legacy paths should be diagnosed");
    require(has_candidate(report, ld::path_family::config, ld::candidate_source::legacy),
        "legacy config file candidates should be reported");
#if !defined(_WIN32)
    require(has_candidate(report, ld::path_family::config, ld::candidate_source::site_default),
        "site default config candidates should be reported");
#endif

    std::filesystem::remove_all(base, ec);
#endif
}

void rejects_relative_overrides_and_environment_values()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.config_override = "relative-config";
    options.environment["XDG_DATA_HOME"] = "relative-data";

    const auto report = ld::resolve_app_paths(identity, options);

    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::override_relative_ignored),
        "relative explicit override should be diagnosed");
#if defined(_WIN32)
    require(selected_path(report, ld::path_family::config) != std::filesystem::path("relative-config"),
        "relative config override should be ignored");
#else
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::environment_relative_ignored),
        "relative environment path should be diagnosed");
    require(selected_path(report, ld::path_family::config) == fixture_path({"home", ".config", "LinuxDesktop2026", "paths-tests"}),
        "relative config override should be ignored");
    require(selected_path(report, ld::path_family::data) == fixture_path({"home", ".local", "share", "LinuxDesktop2026", "paths-tests"}),
        "relative data environment should be ignored");
#endif
}

void reports_missing_home_without_selecting_user_scoped_fallbacks()
{
#if defined(_WIN32)
    return;
#else
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.home_directory.reset();

    const auto report = ld::resolve_app_paths(identity, options);

    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::home_missing),
        "missing home should be diagnosed");
    require(report.selected.find(ld::path_family::config) == report.selected.end(),
        "config should not be guessed without HOME or XDG_CONFIG_HOME");
    require(report.selected.find(ld::path_family::documents) == report.selected.end(),
        "documents should not be guessed without HOME");
#endif
}

void resolves_executable_install_resource_and_temp_paths()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto report = ld::resolve_app_paths(identity, deterministic_options());

    require(selected_path(report, ld::path_family::temp) == fixture_path({"tmp"}),
        "temp override should be selected");
    require(selected_location(report, ld::location_role::executable) == fixture_path({"opt", "linuxdesktop2026", "bin", "paths-tests"}),
        "injected executable path should be selected");
    require(selected_location(report, ld::location_role::executable_directory) == fixture_path({"opt", "linuxdesktop2026", "bin"}),
        "executable directory should derive from executable path");
    require(selected_location(report, ld::location_role::install_prefix) == fixture_path({"opt", "linuxdesktop2026"}),
        "install prefix should derive from a bin executable directory");
    require(selected_location(report, ld::location_role::resources) == fixture_path({"opt", "linuxdesktop2026", "share", "paths-tests"}),
        "resource root should derive from install prefix");
    require(!report.selected.empty() && !report.selected_locations.empty(),
        "ordinary path-family selections and locations should be reported separately");
}

void honors_absolute_explicit_options()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.config_override = fixture_path({"override", "config"});
    options.runtime_override = fixture_path({"override", "runtime"});
    options.resource_root = fixture_path({"override", "resources"});
    options.install_prefix = fixture_path({"override", "prefix"});

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == fixture_path({"override", "config"}),
        "absolute config override should win");
    require(selected_path(report, ld::path_family::runtime) == fixture_path({"override", "runtime"}),
        "absolute runtime override should win");
    require(selected_location(report, ld::location_role::resources) == fixture_path({"override", "resources"}),
        "absolute resource root should win");
    require(selected_location(report, ld::location_role::install_prefix) == fixture_path({"override", "prefix"}),
        "absolute install prefix should win");
    require(has_selected_location_candidate(report, ld::location_role::resources, ld::candidate_source::explicit_option),
        "explicit resource candidate should be source-labeled");
}

void ensures_directories_only_when_requested()
{
    const auto base = std::filesystem::temp_directory_path() / "linuxdesktop2026-paths-ensure-tests";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);

    const auto target = base / "parent" / "leaf";

    auto dry_run = ld::ensure_directory(target);
    require(dry_run.action == ld::directory_action::would_create, "default directory ensure should be dry-run only");
    require(!std::filesystem::exists(target), "dry-run should not create directories");

    ld::ensure_directory_options no_parents;
    no_parents.dry_run = false;
    no_parents.create_parents = false;
    auto missing_parent = ld::ensure_directory(target, no_parents);
    require(missing_parent.action == ld::directory_action::failed,
        "directory ensure should fail when parent creation is disabled");
    require(has_diagnostic(missing_parent.diagnostics, ld::diagnostic_code::directory_parent_missing),
        "missing parent should be diagnosed");

    ld::ensure_directory_options create;
    create.dry_run = false;
    auto created = ld::ensure_directory(target, create);
    require(created.action == ld::directory_action::created, "directory ensure should create when requested");
    require(std::filesystem::is_directory(target), "requested directory should exist");

    auto exists = ld::ensure_directory(target, create);
    require(exists.action == ld::directory_action::already_exists, "existing directory should be reported");

    const auto file_path = base / "file";
    {
        std::ofstream file(file_path);
        file << "not a directory";
    }
    auto file = ld::ensure_directory(file_path, create);
    require(file.action == ld::directory_action::failed, "file path should not be treated as directory");
    require(has_diagnostic(file.diagnostics, ld::diagnostic_code::directory_exists_as_file),
        "existing file should be diagnosed");

    std::filesystem::remove_all(base, ec);
}

void ensures_directory_from_resolver_family()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto resolved = ld::resolve_app_paths(identity, deterministic_options());
    const auto config = selected_path(resolved, ld::path_family::config);

    auto report = ld::ensure_directory(resolved, ld::path_family::config);
    require(report.action == ld::directory_action::would_create,
        "resolver-family directory ensure should use selected path");
    require(report.path == config,
        "resolver-family directory ensure should preserve selected path");

    ld::resolver_report empty;
    auto missing = ld::ensure_directory(empty, ld::path_family::config);
    require(missing.action == ld::directory_action::failed, "unresolved family should fail");
    require(has_diagnostic(missing.diagnostics, "paths.directory.family_unresolved"),
        "unresolved family should be diagnosed");
}

void parses_and_joins_path_lists_with_diagnostics()
{
    ld::path_list_options options;
    options.separator = ';';
    const auto first = fixture_path({"one"});
    const auto second = fixture_path({"two", "..", "two"});
    const auto normalized_second = fixture_path({"two"});

    const auto report = ld::parse_path_list(
        first.string() + ";" + second.string() + ";relative;;" + first.string(),
        options);

    require(report.paths.size() == 2, "path list should keep absolute unique entries");
    require(report.paths[0] == first, "first path should be preserved");
    require(report.paths[1] == normalized_second, "second path should be normalized");
    require(report.candidates.size() == 5, "path-list entries should have direct candidates");
    require(report.candidates[0].selected && report.candidates[0].source == ld::candidate_source::environment,
        "path-list candidates should not carry application path-family vocabulary");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_relative_ignored),
        "relative path-list entries should be diagnosed");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_empty_entry_ignored),
        "empty path-list entries should be diagnosed");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_duplicate_ignored),
        "duplicate path-list entries should be diagnosed");
    require(ld::join_path_list(report.paths, options) == first.string() + ";" + normalized_second.string(),
        "path-list join should use the selected separator");
}

void resolves_typed_plugin_path_sets()
{
    ld::plugin_path_options options;
    options.use_process_environment = false;
    options.home_directory = fixture_path({"home"});
    options.kinds = {
        ld::plugin_path_kind::vst3,
        ld::plugin_path_kind::lv2,
        ld::plugin_path_kind::audio_unit,
        ld::plugin_path_kind::aax,
    };
    options.asset_kinds = {ld::plugin_asset_path_kind::sf2, ld::plugin_asset_path_kind::sfz};
    options.list_options.separator = ';';
    options.environment["VST3_PATH"] = fixture_path({"vendor", "vst3"}).string() + ";" +
        fixture_path({"vendor", "vst3", "..", "vst3"}).string() + ";relative";
    options.environment["AU_PATH"] = fixture_path({"vendor", "components"}).string();
    options.environment["AAX_PATH"] = fixture_path({"vendor", "aax"}).string();

    const auto report = ld::resolve_plugin_path_sets(options);

    const auto& vst3 = plugin_set(report, "vst3");
    require(vst3.kind && *vst3.kind == ld::plugin_path_kind::vst3,
        "typed plugin set should preserve its kind");
    require(!vst3.asset_kind, "executable plugin set should not claim an asset kind");
    require(vst3.category == ld::plugin_path_category::executable_plugin,
        "typed plugin set should report executable plugin category");
    require(std::find(vst3.extensions.begin(), vst3.extensions.end(), ".vst3") != vst3.extensions.end(),
        "typed plugin set should expose expected extension metadata");
    require(vst3.paths.size() == 2 || vst3.paths.size() == 4,
        "VST3 should include env path and platform defaults");
    require(vst3.paths[0] == fixture_path({"vendor", "vst3"}), "environment plugin path should win ordering");
    require(report.candidates[0].set_name == "vst3" && report.candidates[0].kind &&
            *report.candidates[0].kind == ld::plugin_path_kind::vst3 &&
            report.candidates[0].source == ld::candidate_source::environment,
        "plugin candidates should carry direct set vocabulary");
#if defined(_WIN32)
    require(vst3.paths[1] == "C:/Program Files/Common Files/VST3", "VST3 should include Windows default");
#else
    require(vst3.paths[1] == fixture_path({"home", ".vst3"}), "VST3 should include home default");
#endif
    require(has_plugin_diagnostic(report, ld::diagnostic_code::path_list_relative_ignored),
        "relative plugin environment entries should be diagnosed");
    require(has_plugin_diagnostic(report, ld::diagnostic_code::path_list_duplicate_ignored),
        "duplicate plugin entries should be diagnosed");

    const auto& lv2 = plugin_set(report, "lv2");
#if defined(_WIN32)
    require(lv2.paths[0] == "C:/Program Files/Common Files/LV2", "LV2 should include Windows default");
#else
    require(lv2.paths[0] == fixture_path({"home", ".lv2"}), "LV2 should include home default");
    require(lv2.paths[1] == "/usr/local/lib/lv2", "LV2 should include local system default");
#endif

    const auto& audio_unit = plugin_set(report, "audio_unit");
    require(audio_unit.kind && *audio_unit.kind == ld::plugin_path_kind::audio_unit,
        "Audio Unit should be a first-class executable plugin kind");
    require(audio_unit.category == ld::plugin_path_category::executable_plugin,
        "Audio Unit should report executable plugin category");
    require(audio_unit.paths[0] == fixture_path({"vendor", "components"}),
        "Audio Unit environment path should resolve when provided");

    const auto& aax = plugin_set(report, "aax");
    require(aax.kind && *aax.kind == ld::plugin_path_kind::aax,
        "AAX should be a first-class executable plugin kind");
    require(aax.paths[0] == fixture_path({"vendor", "aax"}),
        "AAX environment path should resolve when provided");

    const auto& sf2 = plugin_set(report, "sf2");
    require(!sf2.kind && sf2.asset_kind && *sf2.asset_kind == ld::plugin_asset_path_kind::sf2,
        "SF2 should resolve as an asset-library path kind");
    require(sf2.category == ld::plugin_path_category::asset_library,
        "SF2 should report asset-library category");

    const auto& sfz = plugin_set(report, "sfz");
    require(!sfz.kind && sfz.asset_kind && *sfz.asset_kind == ld::plugin_asset_path_kind::sfz,
        "SFZ should resolve as an asset-library path kind");
}

void resolves_wine_and_custom_plugin_path_sets()
{
    ld::plugin_path_options options;
    options.use_process_environment = false;
    options.home_directory = fixture_path({"home"});
    options.wine_prefix = fixture_path({"wine", "prefix"});
    options.include_wine_prefix_defaults = true;
    options.kinds = {ld::plugin_path_kind::vst2, ld::plugin_path_kind::clap};
    options.include_default_asset_kinds = false;
    options.list_options.separator = ';';

    ld::named_plugin_path_set sampler_request;
    sampler_request.name = "sampler-bank";
    sampler_request.environment_variable = "SAMPLER_BANK_PATH";
    sampler_request.defaults = {fixture_path({"opt", "sampler", "banks"})};
    sampler_request.category = ld::plugin_path_category::asset_library;
    sampler_request.extensions = {".sf2", ".sfz"};
    sampler_request.platforms = {ld::platform_support::any};
    options.named_sets = {sampler_request};
    options.environment["SAMPLER_BANK_PATH"] = fixture_path({"library", "banks"}).string();

    const auto report = ld::resolve_plugin_path_sets(options);

    const auto& vst2 = plugin_set(report, "vst2");
    require(std::find(vst2.paths.begin(), vst2.paths.end(), fixture_path({"wine", "prefix", "drive_c", "Program Files", "VstPlugins"})) != vst2.paths.end(),
        "VST2 should include Wine-prefix default when requested");

    const auto& clap = plugin_set(report, "clap");
    require(std::find(clap.paths.begin(), clap.paths.end(), fixture_path({"wine", "prefix", "drive_c", "Program Files", "Common Files", "CLAP"})) != clap.paths.end(),
        "CLAP should include Wine-prefix default when requested");
    require(std::find_if(report.candidates.begin(), report.candidates.end(), [](const ld::plugin_path_candidate& candidate) {
                return candidate.set_name == "clap" && candidate.source == ld::candidate_source::wine_prefix && candidate.selected;
            }) != report.candidates.end(),
        "Wine-prefix plugin candidates should be source-labeled");

    const auto& sampler = plugin_set(report, "sampler-bank");
    require(!sampler.kind, "custom plugin set should not claim a built-in kind");
    require(!sampler.asset_kind, "named plugin set should not claim a built-in asset kind");
    require(sampler.category == ld::plugin_path_category::asset_library,
        "named plugin set should preserve product-owned category");
    require(sampler.extensions.size() == 2 && sampler.extensions[0] == ".sf2",
        "named plugin set should preserve extension metadata");
    require(sampler.paths.size() == 2, "custom plugin set should include environment and defaults");
    require(sampler.paths[0] == fixture_path({"library", "banks"}), "custom environment path should be first");
    require(sampler.paths[1] == fixture_path({"opt", "sampler", "banks"}), "custom default path should follow");
}

void named_plugin_path_sets_cover_product_and_toolkit_owned_ecosystems()
{
    ld::plugin_path_options options;
    options.use_process_environment = false;
    options.home_directory = fixture_path({"home"});
    options.kinds = {};
    options.asset_kinds = {};
    options.include_default_kinds = false;
    options.include_default_asset_kinds = false;
    options.list_options.separator = ';';

    ld::named_plugin_path_set obs;
    obs.name = "obs-module";
    obs.environment_variable = "OBS_PLUGIN_PATH";
    obs.defaults = {fixture_path({"home", ".config", "obs-studio", "plugins"})};
    obs.category = ld::plugin_path_category::application_extension;
    obs.extensions = {".so", ".dll", ".dylib"};
    obs.platforms = {ld::platform_support::any};

    ld::named_plugin_path_set qt;
    qt.name = "qt-platforms";
    qt.defaults = {fixture_path({"app", "plugins", "platforms"})};
    qt.category = ld::plugin_path_category::toolkit_plugin;
    qt.platforms = {ld::platform_support::any};

    options.named_sets = {obs, qt};
    options.environment["OBS_PLUGIN_PATH"] = fixture_path({"vendor", "obs"}).string() + ";" +
        fixture_path({"vendor", "obs"}).string();

    const auto report = ld::resolve_plugin_path_sets(options);

    const auto& obs_set = plugin_set(report, "obs-module");
    require(obs_set.paths.size() == 2, "named OBS set should include env and default paths");
    require(obs_set.category == ld::plugin_path_category::application_extension,
        "named OBS set should keep product-owned extension category");
    require(obs_set.extensions.size() == 3, "named OBS set should carry extension metadata");
    require(has_plugin_diagnostic(report, ld::diagnostic_code::path_list_duplicate_ignored),
        "named plugin sets should reuse path-list duplicate diagnostics");

    const auto& qt_set = plugin_set(report, "qt-platforms");
    require(qt_set.category == ld::plugin_path_category::toolkit_plugin,
        "Qt counterexample should stay toolkit-owned metadata, not a built-in enum");
    require(!qt_set.kind && !qt_set.asset_kind,
        "Qt counterexample should not claim LinuxDesktop2026 built-in semantics");
}

} // namespace

int main()
{
    try {
        exposes_cpp_version();
        paths_diagnostics_use_shared_core_vocabulary();
        stringifies_public_enums();
        resolves_linux_xdg_base_directories_from_injected_environment();
        resolves_home_fallbacks_when_xdg_is_unset();
        resolves_app_paths_from_public_platform_defaults();
        platform_defaults_do_not_override_explicit_or_environment_paths();
        windows_platform_defaults_precede_known_folders();
        rejects_relative_platform_defaults();
        resolves_xdg_user_dirs_from_config_file();
        reports_user_dir_legacy_and_site_fallback_candidates();
        rejects_relative_overrides_and_environment_values();
        reports_missing_home_without_selecting_user_scoped_fallbacks();
        resolves_executable_install_resource_and_temp_paths();
        honors_absolute_explicit_options();
        ensures_directories_only_when_requested();
        ensures_directory_from_resolver_family();
        parses_and_joins_path_lists_with_diagnostics();
        resolves_typed_plugin_path_sets();
        resolves_wine_and_custom_plugin_path_sets();
        named_plugin_path_sets_cover_product_and_toolkit_owned_ecosystems();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
