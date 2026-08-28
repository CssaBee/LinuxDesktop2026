#include "linuxdesktop/paths.hpp"

#include <cstdlib>
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

void append_report_diagnostic(resolver_report& report, diagnostic item)
{
    report.diagnostics.push_back(std::move(item));
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

} // namespace linuxdesktop::paths
