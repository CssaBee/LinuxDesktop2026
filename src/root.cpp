#include "root_internal.hpp"

#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace linuxdesktop::root {
namespace {

namespace ld_paths = linuxdesktop::paths;

std::filesystem::path selected_path_or_empty(
    const ld_paths::resolver_report& report,
    ld_paths::path_family family)
{
    const auto item = report.selected.find(family);
    return item == report.selected.end() ? std::filesystem::path{} : item->second;
}

std::filesystem::path selected_location_or_empty(
    const ld_paths::resolver_report& report,
    ld_paths::location_role role)
{
    const auto item = report.selected_locations.find(role);
    return item == report.selected_locations.end() ? std::filesystem::path{} : item->second;
}

ld_paths::resolver_options path_options_from_options(const options& options)
{
    ld_paths::resolver_options result;
    result.resource_root = options.resource_root;
    result.home_directory = options.home_directory;
    result.platform_defaults = options.platform_defaults;
    result.environment = options.environment;
    result.use_process_environment = options.use_process_environment;
    return result;
}

void apply_default_roots_from_paths(report& report, const ld_paths::resolver_report& paths)
{
    report.roots.config = selected_path_or_empty(paths, ld_paths::path_family::config);
    report.roots.data = selected_path_or_empty(paths, ld_paths::path_family::data);
    report.roots.state = selected_path_or_empty(paths, ld_paths::path_family::state);
    report.roots.cache = selected_path_or_empty(paths, ld_paths::path_family::cache);
    report.roots.resources = selected_location_or_empty(paths, ld_paths::location_role::resources);
    report.roots.runtime = selected_path_or_empty(paths, ld_paths::path_family::runtime);
}

std::filesystem::path base_path_for(const app_roots& roots, ownership_kind ownership, purpose_kind purpose)
{
    switch (ownership) {
    case ownership_kind::user_local:
        if (purpose == purpose_kind::cache || purpose == purpose_kind::temp) {
            return roots.cache;
        }
        if (purpose == purpose_kind::runtime) {
            return roots.runtime;
        }
        return roots.state;
    case ownership_kind::app_local:
        return roots.config;
    case ownership_kind::ephemeral:
        return roots.cache;
    case ownership_kind::managed:
        return roots.config / "managed";
    case ownership_kind::enforced:
        return roots.config / "enforced";
    case ownership_kind::user_roaming:
        break;
    }

    switch (purpose) {
    case purpose_kind::resources:
        return roots.resources;
    case purpose_kind::config:
    case purpose_kind::plugin_config:
    case purpose_kind::profiles:
    case purpose_kind::backup:
    case purpose_kind::component_config:
    case purpose_kind::managed_config:
    case purpose_kind::enforced_config:
    case purpose_kind::custom:
        return roots.config;
    case purpose_kind::data:
    case purpose_kind::component_data:
        return roots.data;
    case purpose_kind::state:
    case purpose_kind::session:
    case purpose_kind::logs:
    case purpose_kind::component_state:
        return roots.state;
    case purpose_kind::cache:
    case purpose_kind::temp:
        return roots.cache;
    case purpose_kind::runtime:
        return roots.runtime;
    }
    return roots.config;
}

std::filesystem::path default_relative_path(const named_root_request& request)
{
    if (!request.relative_path.empty()) {
        return request.relative_path;
    }
    if (!request.name.empty()) {
        return detail::sanitize_segment(request.name);
    }

    switch (request.purpose) {
    case purpose_kind::logs:
        return "logs";
    case purpose_kind::profiles:
        return "profiles";
    case purpose_kind::backup:
        return "backups";
    case purpose_kind::temp:
        return "temp";
    case purpose_kind::plugin_config:
        return std::filesystem::path{"plugins"} / "Config";
    case purpose_kind::component_config:
        return "config";
    case purpose_kind::component_data:
        return "data";
    case purpose_kind::component_state:
        return "state";
    case purpose_kind::managed_config:
        return "managed";
    case purpose_kind::enforced_config:
        return "enforced";
    default:
        return {};
    }
}

named_root resolve_named_root(const named_root_request& request, const app_roots& roots, bool create_directories)
{
    named_root result;
    result.name = request.name;
    result.purpose = request.purpose;
    result.ownership = request.ownership;

    if (request.name.empty()) {
        result.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "named-root-name-empty",
            "Named root requires a non-empty name"));
        return result;
    }

    const auto relative = default_relative_path(request);
    if (relative.is_absolute()) {
        result.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "named-root-relative-path-absolute",
            "Named root relative_path must be relative",
            relative));
        return result;
    }

    const auto base = base_path_for(roots, request.ownership, request.purpose);
    if (base.empty()) {
        result.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "named-root-base-empty",
            "Named root base path could not be resolved"));
        return result;
    }

    result.path = relative.empty() ? base : base / relative;
    if (create_directories && request.create) {
        result.created = detail::create_directory_for_root(result.path, result.diagnostics);
    }
    return result;
}

void append_unique_name_diagnostics(std::vector<named_root>& roots)
{
    std::vector<std::string> seen;
    for (auto& root : roots) {
        if (std::find(seen.begin(), seen.end(), root.name) != seen.end()) {
            root.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "named-root-duplicate",
                "Named root names must be unique within the same scope"));
        } else {
            seen.push_back(root.name);
        }
    }
}

std::filesystem::path comparable_path(const std::filesystem::path& path)
{
    std::error_code ec;
    auto absolute = path.is_absolute() ? path : std::filesystem::absolute(path, ec);
    if (ec) {
        absolute = path;
    }
    return absolute.lexically_normal();
}

bool path_is_at_or_under(const std::filesystem::path& candidate, const std::filesystem::path& root)
{
    const auto normalized_candidate = comparable_path(candidate);
    const auto normalized_root = comparable_path(root);
    auto candidate_part = normalized_candidate.begin();
    auto root_part = normalized_root.begin();

    for (; root_part != normalized_root.end(); ++root_part, ++candidate_part) {
        if (candidate_part == normalized_candidate.end() || *candidate_part != *root_part) {
            return false;
        }
    }
    return true;
}

std::vector<std::filesystem::path> default_privileged_install_roots()
{
#if defined(_WIN32)
    std::vector<std::filesystem::path> roots;
    if (const char* program_files = std::getenv("ProgramFiles")) {
        roots.emplace_back(program_files);
    }
    if (const char* program_files_x86 = std::getenv("ProgramFiles(x86)")) {
        roots.emplace_back(program_files_x86);
    }
    if (roots.empty()) {
        roots.emplace_back("C:\\Program Files");
        roots.emplace_back("C:\\Program Files (x86)");
    }
    return roots;
#else
    return {"/usr", "/opt", "/app"};
#endif
}

bool is_under_privileged_install_root(const std::filesystem::path& path, const options& options)
{
    const auto roots = options.privileged_install_roots.empty()
        ? default_privileged_install_roots()
        : options.privileged_install_roots;

    for (const auto& root : roots) {
        if (!root.empty() && path_is_at_or_under(path, root)) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace detail {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path)
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

std::string sanitize_segment(std::string value, std::string fallback)
{
    for (char& ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
            ch = '-';
        }
    }
    return value.empty() ? fallback : value;
}

std::error_code system_error_code()
{
#if defined(_WIN32)
    return std::error_code(static_cast<int>(GetLastError()), std::system_category());
#else
    return std::error_code(errno, std::generic_category());
#endif
}

void create_directory_if_needed(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    if (path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "empty-directory",
            "Cannot create an empty directory path"));
        return;
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        if (!std::filesystem::is_directory(path, ec)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "path-not-directory",
                "Path exists but is not a directory",
                path));
        }
        return;
    }

    if (std::filesystem::create_directories(path, ec)) {
        diagnostics.push_back(make_diagnostic(
            severity::info,
            "directory-created",
            "Created directory",
            path));
        return;
    }

    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "directory-create-failed",
            ec.message(),
            path));
    }
}

bool create_directory_for_root(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    const auto before = diagnostics.size();
    const auto existed = !path.empty() && std::filesystem::is_directory(path);
    create_directory_if_needed(path, diagnostics);
    for (auto index = before; index != diagnostics.size(); ++index) {
        if (diagnostics[index].level == severity::error) {
            return false;
        }
    }
    return !existed && !path.empty() && std::filesystem::is_directory(path);
}

void ensure_root_directory(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    ld_paths::ensure_directory_options options;
    options.dry_run = false;
    const auto report = ld_paths::ensure_directory(path, options);
    diagnostics.insert(diagnostics.end(), report.diagnostics.begin(), report.diagnostics.end());
}

bool has_error(const std::vector<diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        if (item.level == severity::error) {
            return true;
        }
    }
    return false;
}

} // namespace detail

const named_root* find_named_root(const report& report, const std::string& name)
{
    const auto it = std::find_if(report.named_roots.begin(), report.named_roots.end(), [&](const auto& root) {
        return root.name == name;
    });
    return it == report.named_roots.end() ? nullptr : &*it;
}

const component_root_group* find_component_roots(const report& report, const std::string& name)
{
    const auto it = std::find_if(report.component_roots.begin(), report.component_roots.end(), [&](const auto& component) {
        return component.name == name;
    });
    return it == report.component_roots.end() ? nullptr : &*it;
}

const named_root* find_component_named_root(const component_root_group& component, const std::string& name)
{
    const auto it = std::find_if(component.roots.begin(), component.roots.end(), [&](const auto& root) {
        return root.name == name;
    });
    return it == component.roots.end() ? nullptr : &*it;
}

request_builder& request_builder::app(std::string organization, std::string application)
{
    identity_.organization = std::move(organization);
    identity_.application = std::move(application);
    return *this;
}

request_builder& request_builder::resource_root(std::filesystem::path path)
{
    options_.resource_root = std::move(path);
    return *this;
}

request_builder& request_builder::home_directory(std::optional<std::filesystem::path> path)
{
    options_.home_directory = std::move(path);
    return *this;
}

request_builder& request_builder::platform_defaults(
    std::optional<linuxdesktop::paths::platform_path_defaults> defaults)
{
    options_.platform_defaults = std::move(defaults);
    return *this;
}

request_builder& request_builder::environment(std::map<std::string, std::string> values)
{
    options_.environment = std::move(values);
    return *this;
}

request_builder& request_builder::use_process_environment(bool enabled)
{
    options_.use_process_environment = enabled;
    return *this;
}

request_builder& request_builder::app_root_override(std::optional<std::filesystem::path> path)
{
    options_.app_root_override = std::move(path);
    return *this;
}

request_builder& request_builder::user_config_override(std::optional<std::filesystem::path> path)
{
    options_.user_config_override = std::move(path);
    return *this;
}

request_builder& request_builder::app_local_marker(std::optional<std::filesystem::path> path)
{
    options_.app_local_marker = std::move(path);
    return *this;
}

request_builder& request_builder::app_local(app_local_level level)
{
    options_.app_local = level;
    return *this;
}

request_builder& request_builder::allow_app_local_root(bool enabled)
{
    options_.allow_app_local_root = enabled;
    return *this;
}

request_builder& request_builder::deny_app_local_root_in_privileged_install(bool enabled)
{
    options_.deny_app_local_root_in_privileged_install = enabled;
    return *this;
}

request_builder& request_builder::allow_user_config_for_app_local_root(bool enabled)
{
    options_.allow_user_config_for_app_local_root = enabled;
    return *this;
}

request_builder& request_builder::privileged_install_roots(std::vector<std::filesystem::path> roots)
{
    options_.privileged_install_roots = std::move(roots);
    return *this;
}

request_builder& request_builder::create_directories(bool enabled)
{
    options_.create_directories = enabled;
    return *this;
}

request_builder& request_builder::named_root(named_root_request request)
{
    options_.named_roots.push_back(std::move(request));
    return *this;
}

request_builder& request_builder::component_roots(component_root_request request)
{
    options_.component_roots.push_back(std::move(request));
    return *this;
}

std::string_view to_string(app_local_level value)
{
    switch (value) {
    case app_local_level::off:
        return "off";
    case app_local_level::config_only:
        return "config_only";
    case app_local_level::profile:
        return "profile";
    case app_local_level::clean:
        return "clean";
    }
    return "unknown";
}

std::string_view to_string(purpose_kind value)
{
    switch (value) {
    case purpose_kind::resources:
        return "resources";
    case purpose_kind::config:
        return "config";
    case purpose_kind::data:
        return "data";
    case purpose_kind::state:
        return "state";
    case purpose_kind::cache:
        return "cache";
    case purpose_kind::runtime:
        return "runtime";
    case purpose_kind::session:
        return "session";
    case purpose_kind::plugin_config:
        return "plugin_config";
    case purpose_kind::logs:
        return "logs";
    case purpose_kind::profiles:
        return "profiles";
    case purpose_kind::backup:
        return "backup";
    case purpose_kind::temp:
        return "temp";
    case purpose_kind::component_config:
        return "component_config";
    case purpose_kind::component_data:
        return "component_data";
    case purpose_kind::component_state:
        return "component_state";
    case purpose_kind::managed_config:
        return "managed_config";
    case purpose_kind::enforced_config:
        return "enforced_config";
    case purpose_kind::custom:
        return "custom";
    }
    return "unknown";
}

std::string_view to_string(ownership_kind value)
{
    switch (value) {
    case ownership_kind::user_roaming:
        return "user_roaming";
    case ownership_kind::user_local:
        return "user_local";
    case ownership_kind::app_local:
        return "app_local";
    case ownership_kind::ephemeral:
        return "ephemeral";
    case ownership_kind::managed:
        return "managed";
    case ownership_kind::enforced:
        return "enforced";
    }
    return "unknown";
}

std::string_view to_string(component_kind value)
{
    switch (value) {
    case component_kind::plugin:
        return "plugin";
    case component_kind::embedded_tool:
        return "embedded_tool";
    case component_kind::profile:
        return "profile";
    case component_kind::language_pack:
        return "language_pack";
    case component_kind::extension:
        return "extension";
    case component_kind::custom:
        return "custom";
    }
    return "unknown";
}

report resolve_app_roots(const app_identity& identity, const options& options)
{
    report report;

    ld_paths::app_identity path_identity;
    path_identity.organization = identity.organization;
    path_identity.application = identity.application;
    const auto path_report = ld_paths::resolve_app_paths(path_identity, path_options_from_options(options));
    report.diagnostics.insert(report.diagnostics.end(), path_report.diagnostics.begin(), path_report.diagnostics.end());
    report.roots.resources = selected_location_or_empty(path_report, ld_paths::location_role::resources);

    report.app_local = options.app_local;

    if (options.app_root_override && !options.app_root_override->empty()) {
        if (options.app_root_override->is_absolute()) {
            report.app_root_override_active = true;
            report.roots.config = *options.app_root_override;
            report.roots.data = *options.app_root_override;
            report.roots.state = *options.app_root_override;
            report.roots.cache = *options.app_root_override / "cache";
            report.roots.runtime = std::filesystem::path{};
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "app-root-override-relative",
                "App root override must be absolute",
                *options.app_root_override));
        }
    }

    report.app_local_requested = options.app_local_marker.has_value() && !options.app_local_marker->empty();
    if (!report.app_root_override_active && report.app_local_requested) {
        const auto marker = *options.app_local_marker;
        std::error_code ec;
        if (std::filesystem::exists(marker, ec)) {
            if (options.allow_app_local_root && options.app_local != app_local_level::off) {
                const auto app_local_root = marker.parent_path();
                const auto privileged_path = !report.roots.resources.empty() ? report.roots.resources : app_local_root;
                if (options.deny_app_local_root_in_privileged_install &&
                    is_under_privileged_install_root(privileged_path, options)) {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::warning,
                        "app_local-denied-privileged-install",
                        "App-local marker exists, but install root is privileged",
                        privileged_path));
                } else {
                    report.app_local_active = true;
                    report.roots.config = app_local_root;
                    report.roots.data = app_local_root;
                    report.roots.state = app_local_root;
                    report.roots.cache = app_local_root / "cache";
                    report.roots.runtime = std::filesystem::path{};
                }
            } else {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::warning,
                    "app_local-denied",
                    "App-local marker exists, but app-local roots are disabled",
                    marker));
            }
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "app_local-marker-missing",
                "App-local marker was requested but does not exist",
                marker));
        }
    }

    if (report.roots.config.empty() && !detail::has_error(report.diagnostics)) {
        apply_default_roots_from_paths(report, path_report);
    }

    if (options.user_config_override && !options.user_config_override->empty()) {
        if (report.app_root_override_active) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "user-config-override-ignored",
                "User config override was ignored because app root override is active",
                *options.user_config_override));
        } else if (report.app_local_active && !options.allow_user_config_for_app_local_root) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "user-config-override-ignored-app-local",
                "User config override was ignored because app-local root is active",
                *options.user_config_override));
        } else if (options.user_config_override->is_absolute()) {
            report.user_config_override_active = true;
            report.roots.config = *options.user_config_override;
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "user-config-override-relative",
                "User config override must be absolute",
                *options.user_config_override));
        }
    }

    report.roots.session = report.roots.state / "sessions";
    report.roots.plugin_config = report.roots.config / "plugins" / "Config";

    for (const auto& request : options.named_roots) {
        report.named_roots.push_back(resolve_named_root(request, report.roots, options.create_directories));
    }
    append_unique_name_diagnostics(report.named_roots);

    for (const auto& request : options.component_roots) {
        component_root_group component;
        component.name = request.name;
        component.kind = request.kind;

        if (request.name.empty()) {
            component.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "component-root-name-empty",
                "Component root requires a non-empty name"));
        }

        const auto component_leaf = detail::sanitize_segment(request.name, "component");
        for (const auto& root_request : request.roots) {
            auto scoped_request = root_request;
            const auto relative = default_relative_path(scoped_request);
            scoped_request.relative_path = std::filesystem::path{"components"} / component_leaf / relative;
            component.roots.push_back(resolve_named_root(scoped_request, report.roots, options.create_directories));
        }
        append_unique_name_diagnostics(component.roots);
        report.component_roots.push_back(std::move(component));
    }

    if (options.create_directories && !detail::has_error(report.diagnostics)) {
        detail::ensure_root_directory(report.roots.config, report.diagnostics);
        detail::ensure_root_directory(report.roots.data, report.diagnostics);
        detail::ensure_root_directory(report.roots.state, report.diagnostics);
        detail::ensure_root_directory(report.roots.cache, report.diagnostics);
        detail::create_directory_if_needed(report.roots.session, report.diagnostics);
        detail::create_directory_if_needed(report.roots.plugin_config, report.diagnostics);
        if (!report.roots.runtime.empty()) {
            detail::ensure_root_directory(report.roots.runtime, report.diagnostics);
        }
    }

    return report;
}

} // namespace linuxdesktop::root
