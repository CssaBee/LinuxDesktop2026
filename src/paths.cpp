#include "linuxdesktop/paths.hpp"

#include <cstdlib>
#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ShlObj.h>
#include <objbase.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace linuxdesktop::paths {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

std::string sanitize_segment(std::string value, std::string fallback = {})
{
    for (char& ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
            ch = '-';
        }
    }
    return value.empty() ? fallback : value;
}

std::optional<std::string> environment_value(const resolver_options& options, const std::string& name)
{
    const auto injected = options.environment.find(name);
    if (injected != options.environment.end()) {
        if (injected->second.empty()) {
            return std::nullopt;
        }
        return injected->second;
    }

    if (!options.use_process_environment) {
        return std::nullopt;
    }

    const char* value = std::getenv(name.c_str());
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::string(value);
}

std::optional<std::string> environment_value(
    const std::map<std::string, std::string>& environment,
    bool use_process_environment,
    const std::string& name)
{
    const auto injected = environment.find(name);
    if (injected != environment.end()) {
        if (injected->second.empty()) {
            return std::nullopt;
        }
        return injected->second;
    }

    if (!use_process_environment) {
        return std::nullopt;
    }

    const char* value = std::getenv(name.c_str());
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::string(value);
}

std::filesystem::path home_directory(const resolver_options& options)
{
    if (options.home_directory) {
        return *options.home_directory;
    }

#if defined(_WIN32)
    if (auto value = environment_value(options, "USERPROFILE")) {
        return *value;
    }
    const auto drive = environment_value(options, "HOMEDRIVE");
    const auto path = environment_value(options, "HOMEPATH");
    if (drive && path) {
        return *drive + *path;
    }
#else
    if (auto value = environment_value(options, "HOME")) {
        return *value;
    }
#endif
    return {};
}

std::filesystem::path home_directory(
    const std::map<std::string, std::string>& environment,
    bool use_process_environment,
    const std::optional<std::filesystem::path>& override_home)
{
    if (override_home) {
        return *override_home;
    }

#if defined(_WIN32)
    if (auto value = environment_value(environment, use_process_environment, "USERPROFILE")) {
        return *value;
    }
    const auto drive = environment_value(environment, use_process_environment, "HOMEDRIVE");
    const auto path = environment_value(environment, use_process_environment, "HOMEPATH");
    if (drive && path) {
        return *drive + *path;
    }
#else
    if (auto value = environment_value(environment, use_process_environment, "HOME")) {
        return *value;
    }
#endif
    return {};
}

void append_report_diagnostic(resolver_report& report, diagnostic item)
{
    report.diagnostics.push_back(std::move(item));
}

void append_path_list_candidate(
    path_list_report& report,
    std::filesystem::path path,
    bool selected,
    std::vector<diagnostic> diagnostics = {},
    candidate_source source = candidate_source::environment)
{
    path_candidate candidate;
    candidate.family = path_family::plugin_search;
    candidate.source = source;
    candidate.path = std::move(path);
    candidate.selected = selected;
    candidate.diagnostics = std::move(diagnostics);

    for (const auto& item : candidate.diagnostics) {
        report.diagnostics.push_back(item);
    }
    if (candidate.selected) {
        report.paths.push_back(candidate.path);
    }
    report.candidates.push_back(std::move(candidate));
}

char default_path_list_separator()
{
#if defined(_WIN32)
    return ';';
#else
    return ':';
#endif
}

std::filesystem::path normalize_for_duplicate_check(const std::filesystem::path& path)
{
    return path.lexically_normal();
}

std::filesystem::path expand_home(const std::filesystem::path& path, const std::filesystem::path& home)
{
    const auto text = path.string();
    if (text == "~") {
        return home.empty() ? path : home;
    }
    if (text.rfind("~/", 0) == 0 || text.rfind("~\\", 0) == 0) {
        return home.empty() ? path : home / text.substr(2);
    }
    return path;
}

std::vector<plugin_path_kind> default_plugin_kinds()
{
    return {
        plugin_path_kind::ladspa,
        plugin_path_kind::dssi,
        plugin_path_kind::lv2,
        plugin_path_kind::vst2,
        plugin_path_kind::vst3,
        plugin_path_kind::clap,
        plugin_path_kind::sf2,
        plugin_path_kind::sfz,
        plugin_path_kind::jsfx,
    };
}

std::string plugin_environment_variable(plugin_path_kind kind)
{
    switch (kind) {
    case plugin_path_kind::ladspa:
        return "LADSPA_PATH";
    case plugin_path_kind::dssi:
        return "DSSI_PATH";
    case plugin_path_kind::lv2:
        return "LV2_PATH";
    case plugin_path_kind::vst2:
        return "VST_PATH";
    case plugin_path_kind::vst3:
        return "VST3_PATH";
    case plugin_path_kind::clap:
        return "CLAP_PATH";
    case plugin_path_kind::sf2:
        return "SF2_PATH";
    case plugin_path_kind::sfz:
        return "SFZ_PATH";
    case plugin_path_kind::jsfx:
        return "JSFX_PATH";
    }
    return {};
}

std::vector<std::filesystem::path> plugin_defaults(plugin_path_kind kind, const std::filesystem::path& home)
{
    const auto home_path = [&](const char* leaf) {
        return home.empty() ? std::filesystem::path{} : home / leaf;
    };

#if defined(_WIN32)
    switch (kind) {
    case plugin_path_kind::ladspa:
        return {"C:/Program Files/LADSPA"};
    case plugin_path_kind::dssi:
        return {"C:/Program Files/DSSI"};
    case plugin_path_kind::lv2:
        return {"C:/Program Files/Common Files/LV2"};
    case plugin_path_kind::vst2:
        return {"C:/Program Files/VstPlugins", "C:/Program Files/Steinberg/VstPlugins"};
    case plugin_path_kind::vst3:
        return {"C:/Program Files/Common Files/VST3"};
    case plugin_path_kind::clap:
        return {"C:/Program Files/Common Files/CLAP"};
    case plugin_path_kind::sf2:
        return {"C:/Program Files/SF2"};
    case plugin_path_kind::sfz:
        return {"C:/Program Files/SFZ"};
    case plugin_path_kind::jsfx:
        return {};
    }
#else
    switch (kind) {
    case plugin_path_kind::ladspa:
        return {home_path(".ladspa"), "/usr/local/lib/ladspa", "/usr/lib/ladspa"};
    case plugin_path_kind::dssi:
        return {home_path(".dssi"), "/usr/local/lib/dssi", "/usr/lib/dssi"};
    case plugin_path_kind::lv2:
        return {home_path(".lv2"), "/usr/local/lib/lv2", "/usr/lib/lv2"};
    case plugin_path_kind::vst2:
        return {home_path(".vst"), "/usr/local/lib/vst", "/usr/lib/vst"};
    case plugin_path_kind::vst3:
        return {home_path(".vst3"), "/usr/local/lib/vst3", "/usr/lib/vst3"};
    case plugin_path_kind::clap:
        return {home_path(".clap"), "/usr/local/lib/clap", "/usr/lib/clap"};
    case plugin_path_kind::sf2:
        return {home_path(".sounds/sf2"), "/usr/local/share/sounds/sf2", "/usr/share/sounds/sf2"};
    case plugin_path_kind::sfz:
        return {home_path(".sounds/sfz"), "/usr/local/share/sounds/sfz", "/usr/share/sounds/sfz"};
    case plugin_path_kind::jsfx:
        return {home_path(".config/REAPER/Effects")};
    }
#endif
    return {};
}

std::vector<std::filesystem::path> wine_plugin_defaults(plugin_path_kind kind, const std::filesystem::path& wine_prefix)
{
    if (wine_prefix.empty()) {
        return {};
    }

    const auto program_files = wine_prefix / "drive_c" / "Program Files";
    const auto common_files = program_files / "Common Files";
    switch (kind) {
    case plugin_path_kind::vst2:
        return {program_files / "VstPlugins", program_files / "Steinberg" / "VstPlugins"};
    case plugin_path_kind::vst3:
        return {common_files / "VST3"};
    case plugin_path_kind::clap:
        return {common_files / "CLAP"};
    default:
        return {};
    }
}

void add_candidate(
    resolver_report& report,
    path_family family,
    candidate_source source,
    std::filesystem::path path,
    bool selected,
    std::vector<diagnostic> diagnostics = {})
{
    path_candidate candidate;
    candidate.family = family;
    candidate.source = source;
    candidate.path = std::move(path);
    candidate.selected = selected;
    candidate.diagnostics = std::move(diagnostics);

    if (candidate.selected) {
        report.selected[candidate.family] = candidate.path;
    }

    for (const auto& item : candidate.diagnostics) {
        report.diagnostics.push_back(item);
    }

    report.candidates.push_back(std::move(candidate));
}

bool select_absolute_override(
    resolver_report& report,
    path_family family,
    const std::optional<std::filesystem::path>& override_path)
{
    if (!override_path) {
        return false;
    }
    if (override_path->is_absolute()) {
        add_candidate(report, family, candidate_source::explicit_option, *override_path, true);
        return true;
    }

    add_candidate(
        report,
        family,
        candidate_source::explicit_option,
        *override_path,
        false,
        {make_diagnostic(
            severity::error,
            std::string(diagnostic_code::override_relative_ignored),
            "Explicit path override must be absolute",
            *override_path)});
    return false;
}

std::optional<std::filesystem::path> absolute_environment_path(
    const resolver_options& options,
    resolver_report& report,
    const std::string& name,
    path_family family)
{
    const auto value = environment_value(options, name);
    if (!value) {
        return std::nullopt;
    }

    std::filesystem::path path(*value);
    if (path.is_absolute()) {
        return path;
    }

    add_candidate(
        report,
        family,
        candidate_source::environment,
        path,
        false,
        {make_diagnostic(
            severity::warning,
            std::string(diagnostic_code::environment_relative_ignored),
            name + " is relative and was ignored",
            path)});
    return std::nullopt;
}

void select_base_directory(
    resolver_report& report,
    const resolver_options& options,
    path_family family,
    const std::string& env_name,
    const std::filesystem::path& fallback_base,
    const std::filesystem::path& app_leaf)
{
    if (report.selected.find(family) != report.selected.end()) {
        return;
    }

    if (auto base = absolute_environment_path(options, report, env_name, family)) {
        add_candidate(report, family, candidate_source::xdg_base_dir, *base / app_leaf, true);
        return;
    }

    if (!fallback_base.empty()) {
        add_candidate(report, family, candidate_source::fallback, fallback_base / app_leaf, true);
    }
}

void select_user_directory(
    resolver_report& report,
    path_family family,
    const std::filesystem::path& home,
    const std::filesystem::path& leaf)
{
    if (!home.empty()) {
        add_candidate(report, family, candidate_source::fallback, home / leaf, true);
    }
}

std::filesystem::path temp_directory(std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    auto path = std::filesystem::temp_directory_path(ec);
    if (!ec) {
        return path;
    }

    diagnostics.push_back(make_diagnostic(
        severity::warning,
        std::string(diagnostic_code::temp_directory_unavailable),
        ec.message()));
    return {};
}

std::optional<std::filesystem::path> actual_executable_path(std::vector<diagnostic>& diagnostics)
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size > 0) {
        buffer.resize(size);
        return std::filesystem::path(buffer);
    }
#else
    std::error_code ec;
    auto path = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return path;
    }
#endif
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        std::string(diagnostic_code::executable_unavailable),
        "Could not resolve current executable path"));
    return std::nullopt;
}

#if defined(_WIN32)
std::optional<std::filesystem::path> known_folder(REFKNOWNFOLDERID folder_id, resolver_report& report, path_family family)
{
    PWSTR path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_DEFAULT, nullptr, &path);
    if (FAILED(result) || !path) {
        add_candidate(
            report,
            family,
            candidate_source::known_folder,
            {},
            false,
            {make_diagnostic(
                severity::warning,
                "paths.known_folder.unavailable",
                "Windows known-folder lookup failed")});
        return std::nullopt;
    }

    std::filesystem::path value(path);
    CoTaskMemFree(path);
    return value;
}
#endif

} // namespace

std::string_view to_string(path_family value)
{
    switch (value) {
    case path_family::config:
        return "config";
    case path_family::data:
        return "data";
    case path_family::state:
        return "state";
    case path_family::cache:
        return "cache";
    case path_family::temp:
        return "temp";
    case path_family::documents:
        return "documents";
    case path_family::desktop:
        return "desktop";
    case path_family::downloads:
        return "downloads";
    case path_family::music:
        return "music";
    case path_family::pictures:
        return "pictures";
    case path_family::videos:
        return "videos";
    case path_family::public_share:
        return "public_share";
    case path_family::executable:
        return "executable";
    case path_family::executable_directory:
        return "executable_directory";
    case path_family::install_prefix:
        return "install_prefix";
    case path_family::resources:
        return "resources";
    case path_family::plugin_search:
        return "plugin_search";
    }
    return "unknown";
}

std::string_view to_string(candidate_source value)
{
    switch (value) {
    case candidate_source::explicit_option:
        return "explicit_option";
    case candidate_source::environment:
        return "environment";
    case candidate_source::xdg_base_dir:
        return "xdg_base_dir";
    case candidate_source::xdg_user_dir:
        return "xdg_user_dir";
    case candidate_source::known_folder:
        return "known_folder";
    case candidate_source::executable_relative:
        return "executable_relative";
    case candidate_source::legacy:
        return "legacy";
    case candidate_source::site_default:
        return "site_default";
    case candidate_source::fallback:
        return "fallback";
    }
    return "unknown";
}

std::string_view to_string(directory_action value)
{
    switch (value) {
    case directory_action::already_exists:
        return "already_exists";
    case directory_action::would_create:
        return "would_create";
    case directory_action::created:
        return "created";
    case directory_action::failed:
        return "failed";
    }
    return "unknown";
}

std::string_view to_string(plugin_path_kind value)
{
    switch (value) {
    case plugin_path_kind::ladspa:
        return "ladspa";
    case plugin_path_kind::dssi:
        return "dssi";
    case plugin_path_kind::lv2:
        return "lv2";
    case plugin_path_kind::vst2:
        return "vst2";
    case plugin_path_kind::vst3:
        return "vst3";
    case plugin_path_kind::clap:
        return "clap";
    case plugin_path_kind::sf2:
        return "sf2";
    case plugin_path_kind::sfz:
        return "sfz";
    case plugin_path_kind::jsfx:
        return "jsfx";
    }
    return "unknown";
}

resolver_report resolve_app_paths(const app_identity& identity, const resolver_options& options)
{
    resolver_report report;

    const auto organization = sanitize_segment(identity.organization);
    const auto application = sanitize_segment(identity.application, "application");
    if (identity.application.empty()) {
        append_report_diagnostic(report, make_diagnostic(
            severity::warning,
            std::string(diagnostic_code::application_missing),
            "Application identity was empty; using fallback application segment"));
    }
    const std::filesystem::path app_leaf = organization.empty()
        ? std::filesystem::path(application)
        : std::filesystem::path(organization) / application;

    select_absolute_override(report, path_family::config, options.config_override);
    select_absolute_override(report, path_family::data, options.data_override);
    select_absolute_override(report, path_family::state, options.state_override);
    select_absolute_override(report, path_family::cache, options.cache_override);
    select_absolute_override(report, path_family::temp, options.temp_override);
    select_absolute_override(report, path_family::resources, options.resource_root);
    select_absolute_override(report, path_family::install_prefix, options.install_prefix);
    select_absolute_override(report, path_family::executable, options.executable_path);

#if defined(_WIN32)
    const auto home = home_directory(options);
    auto roaming = absolute_environment_path(options, report, "APPDATA", path_family::config);
    auto roaming_source = candidate_source::environment;
    if (!roaming) {
        roaming = known_folder(FOLDERID_RoamingAppData, report, path_family::config);
        roaming_source = candidate_source::known_folder;
    }
    auto local = absolute_environment_path(options, report, "LOCALAPPDATA", path_family::state);
    auto local_source = candidate_source::environment;
    if (!local) {
        local = known_folder(FOLDERID_LocalAppData, report, path_family::state);
        local_source = candidate_source::known_folder;
    }

    if (report.selected.find(path_family::config) == report.selected.end()) {
        if (roaming) {
            add_candidate(report, path_family::config, roaming_source, *roaming / app_leaf, true);
        } else if (!home.empty()) {
            add_candidate(report, path_family::config, candidate_source::fallback, home / "AppData" / "Roaming" / app_leaf, true);
        }
    }
    if (report.selected.find(path_family::data) == report.selected.end()) {
        if (roaming) {
            add_candidate(report, path_family::data, roaming_source, *roaming / app_leaf, true);
        } else if (!home.empty()) {
            add_candidate(report, path_family::data, candidate_source::fallback, home / "AppData" / "Roaming" / app_leaf, true);
        }
    }
    if (report.selected.find(path_family::state) == report.selected.end()) {
        if (local) {
            add_candidate(report, path_family::state, local_source, *local / app_leaf / "state", true);
        } else if (!home.empty()) {
            add_candidate(report, path_family::state, candidate_source::fallback, home / "AppData" / "Local" / app_leaf / "state", true);
        }
    }
    if (report.selected.find(path_family::cache) == report.selected.end()) {
        if (local) {
            add_candidate(report, path_family::cache, local_source, *local / app_leaf / "cache", true);
        } else if (!home.empty()) {
            add_candidate(report, path_family::cache, candidate_source::fallback, home / "AppData" / "Local" / app_leaf / "cache", true);
        }
    }

    if (!home.empty()) {
        select_user_directory(report, path_family::documents, home, "Documents");
        select_user_directory(report, path_family::desktop, home, "Desktop");
        select_user_directory(report, path_family::downloads, home, "Downloads");
        select_user_directory(report, path_family::music, home, "Music");
        select_user_directory(report, path_family::pictures, home, "Pictures");
        select_user_directory(report, path_family::videos, home, "Videos");
        select_user_directory(report, path_family::public_share, home, "Public");
    }
#else
    const auto home = home_directory(options);
    if (home.empty()) {
        append_report_diagnostic(report, make_diagnostic(
            severity::error,
            std::string(diagnostic_code::home_missing),
            "Cannot resolve user-scoped paths without HOME or an injected home directory"));
    }

    select_base_directory(report, options, path_family::config, "XDG_CONFIG_HOME", home.empty() ? std::filesystem::path{} : home / ".config", app_leaf);
    select_base_directory(report, options, path_family::data, "XDG_DATA_HOME", home.empty() ? std::filesystem::path{} : home / ".local" / "share", app_leaf);
    select_base_directory(report, options, path_family::state, "XDG_STATE_HOME", home.empty() ? std::filesystem::path{} : home / ".local" / "state", app_leaf);
    select_base_directory(report, options, path_family::cache, "XDG_CACHE_HOME", home.empty() ? std::filesystem::path{} : home / ".cache", app_leaf);

    select_user_directory(report, path_family::documents, home, "Documents");
    select_user_directory(report, path_family::desktop, home, "Desktop");
    select_user_directory(report, path_family::downloads, home, "Downloads");
    select_user_directory(report, path_family::music, home, "Music");
    select_user_directory(report, path_family::pictures, home, "Pictures");
    select_user_directory(report, path_family::videos, home, "Videos");
    select_user_directory(report, path_family::public_share, home, "Public");
#endif

    if (report.selected.find(path_family::temp) == report.selected.end()) {
        auto temp_diagnostics = std::vector<diagnostic>{};
        const auto temp = temp_directory(temp_diagnostics);
        if (!temp.empty()) {
            add_candidate(report, path_family::temp, candidate_source::fallback, temp, true, std::move(temp_diagnostics));
        } else {
            for (auto& item : temp_diagnostics) {
                append_report_diagnostic(report, std::move(item));
            }
        }
    }

    if (report.selected.find(path_family::executable) == report.selected.end()) {
        auto executable_diagnostics = std::vector<diagnostic>{};
        if (auto executable = actual_executable_path(executable_diagnostics)) {
            add_candidate(report, path_family::executable, candidate_source::executable_relative, *executable, true, std::move(executable_diagnostics));
        } else {
            for (auto& item : executable_diagnostics) {
                append_report_diagnostic(report, std::move(item));
            }
        }
    }

    const auto executable = report.selected.find(path_family::executable);
    if (executable != report.selected.end()) {
        add_candidate(report, path_family::executable_directory, candidate_source::executable_relative, executable->second.parent_path(), true);
    }

    const auto executable_directory = report.selected.find(path_family::executable_directory);
    if (report.selected.find(path_family::install_prefix) == report.selected.end() && executable_directory != report.selected.end()) {
        const auto directory = executable_directory->second;
        const auto prefix = directory.filename() == "bin" ? directory.parent_path() : directory;
        add_candidate(report, path_family::install_prefix, candidate_source::executable_relative, prefix, true);
    }

    if (report.selected.find(path_family::resources) == report.selected.end()) {
        const auto install_prefix = report.selected.find(path_family::install_prefix);
        if (install_prefix != report.selected.end()) {
            add_candidate(report, path_family::resources, candidate_source::executable_relative, install_prefix->second / "share" / application, true);
        } else if (executable_directory != report.selected.end()) {
            add_candidate(report, path_family::resources, candidate_source::executable_relative, executable_directory->second, true);
        }
    }

    return report;
}

ensure_directory_report ensure_directory(const std::filesystem::path& path, const ensure_directory_options& options)
{
    ensure_directory_report report;
    report.path = path;

    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        if (!ec && std::filesystem::is_directory(path, ec) && !ec) {
            report.action = directory_action::already_exists;
            return report;
        }

        report.action = directory_action::failed;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            std::string(diagnostic_code::directory_exists_as_file),
            "Path exists but is not a directory",
            path));
        return report;
    }

    const auto parent = path.parent_path();
    if (!options.create_parents && !parent.empty() && !std::filesystem::exists(parent, ec)) {
        report.action = directory_action::failed;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            std::string(diagnostic_code::directory_parent_missing),
            "Parent directory is missing and parent creation is disabled",
            parent));
        return report;
    }

    if (options.dry_run) {
        report.action = directory_action::would_create;
        return report;
    }

    const bool created = options.create_parents
        ? std::filesystem::create_directories(path, ec)
        : std::filesystem::create_directory(path, ec);
    if (!ec || std::filesystem::is_directory(path)) {
        report.action = created ? directory_action::created : directory_action::already_exists;
        return report;
    }

    report.action = directory_action::failed;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        std::string(diagnostic_code::directory_create_failed),
        "Directory creation failed: " + ec.message(),
        path));
    return report;
}

ensure_directory_report ensure_directory(
    const resolver_report& source_report,
    path_family family,
    const ensure_directory_options& options)
{
    const auto selected = source_report.selected.find(family);
    if (selected == source_report.selected.end()) {
        ensure_directory_report report;
        report.action = directory_action::failed;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "paths.directory.family_unresolved",
            "Cannot ensure an unresolved path family"));
        return report;
    }

    return ensure_directory(selected->second, options);
}

path_list_report parse_path_list(std::string_view value, const path_list_options& options)
{
    path_list_report report;
    const char separator = options.separator.value_or(default_path_list_separator());
    std::set<std::filesystem::path> seen;

    std::string current;
    auto flush = [&]() {
        if (current.empty()) {
            append_path_list_candidate(
                report,
                {},
                false,
                {make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::path_list_empty_entry_ignored),
                    "Empty path-list entry was ignored")});
            return;
        }

        std::filesystem::path path(current);
        current.clear();
        if (options.require_absolute && !path.is_absolute()) {
            append_path_list_candidate(
                report,
                path,
                false,
                {make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::path_list_relative_ignored),
                    "Relative path-list entry was ignored",
                    path)});
            return;
        }

        const auto normalized = normalize_for_duplicate_check(path);
        if (options.drop_duplicates && !seen.insert(normalized).second) {
            append_path_list_candidate(
                report,
                path,
                false,
                {make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::path_list_duplicate_ignored),
                    "Duplicate path-list entry was ignored",
                    path)});
            return;
        }

        append_path_list_candidate(report, normalized, true);
    };

    for (const char ch : value) {
        if (ch == separator) {
            flush();
        } else {
            current.push_back(ch);
        }
    }
    flush();

    return report;
}

std::string join_path_list(const std::vector<std::filesystem::path>& paths, const path_list_options& options)
{
    const char separator = options.separator.value_or(default_path_list_separator());
    std::ostringstream joined;
    bool first = true;
    for (const auto& path : paths) {
        if (!first) {
            joined << separator;
        }
        first = false;
        joined << path.string();
    }
    return joined.str();
}

plugin_path_report resolve_plugin_path_sets(const plugin_path_options& options)
{
    plugin_path_report report;
    const auto home = home_directory(options.environment, options.use_process_environment, options.home_directory);

    auto append_set = [&](std::string name,
                          std::optional<plugin_path_kind> kind,
                          const std::optional<std::string>& environment_variable,
                          std::vector<std::filesystem::path> defaults) {
        path_list_report combined;

        if (environment_variable) {
            if (const auto value = environment_value(options.environment, options.use_process_environment, *environment_variable)) {
                auto parsed = parse_path_list(*value, options.list_options);
                for (auto& candidate : parsed.candidates) {
                    candidate.source = candidate_source::environment;
                    combined.candidates.push_back(std::move(candidate));
                }
                combined.paths.insert(combined.paths.end(), parsed.paths.begin(), parsed.paths.end());
            }
        }

        for (auto& path : defaults) {
            path = expand_home(path, home);
            if (path.empty()) {
                continue;
            }
            path_candidate candidate;
            candidate.family = path_family::plugin_search;
            candidate.source = candidate_source::fallback;
            candidate.path = path.lexically_normal();
            candidate.selected = !options.list_options.require_absolute || candidate.path.is_absolute();
            if (!candidate.selected) {
                candidate.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::path_list_relative_ignored),
                    "Relative default path-list entry was ignored",
                    candidate.path));
            }
            combined.candidates.push_back(std::move(candidate));
        }

        if (kind && options.include_wine_prefix_defaults) {
            auto wine_prefix = options.wine_prefix;
            if (!wine_prefix) {
                if (auto env_prefix = environment_value(options.environment, options.use_process_environment, "WINEPREFIX")) {
                    wine_prefix = std::filesystem::path(*env_prefix);
                } else if (!home.empty()) {
                    wine_prefix = home / ".wine";
                }
            }
            for (const auto& path : wine_plugin_defaults(*kind, wine_prefix.value_or(std::filesystem::path{}))) {
                path_candidate candidate;
                candidate.family = path_family::plugin_search;
                candidate.source = candidate_source::fallback;
                candidate.path = path.lexically_normal();
                candidate.selected = !options.list_options.require_absolute || candidate.path.is_absolute();
                combined.candidates.push_back(std::move(candidate));
            }
        }

        std::set<std::filesystem::path> seen;
        plugin_path_set set;
        set.name = std::move(name);
        set.kind = kind;
        for (auto& candidate : combined.candidates) {
            const auto normalized = normalize_for_duplicate_check(candidate.path);
            if (candidate.selected && options.list_options.drop_duplicates && !seen.insert(normalized).second) {
                candidate.selected = false;
                candidate.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::path_list_duplicate_ignored),
                    "Duplicate plugin search path was ignored",
                    candidate.path));
            }
            if (candidate.selected) {
                candidate.path = normalized;
                set.paths.push_back(candidate.path);
            }
            for (const auto& item : candidate.diagnostics) {
                combined.diagnostics.push_back(item);
            }
            report.candidates.push_back(std::move(candidate));
        }

        report.diagnostics.insert(report.diagnostics.end(), combined.diagnostics.begin(), combined.diagnostics.end());
        report.sets.push_back(std::move(set));
    };

    const auto kinds = options.kinds.empty() ? default_plugin_kinds() : options.kinds;
    for (const auto kind : kinds) {
        append_set(
            std::string(to_string(kind)),
            kind,
            plugin_environment_variable(kind),
            plugin_defaults(kind, home));
    }

    for (const auto& custom : options.custom_sets) {
        append_set(custom.name, std::nullopt, custom.environment_variable, custom.defaults);
    }

    return report;
}

} // namespace linuxdesktop::paths
