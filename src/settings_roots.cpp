#include "settings_internal.hpp"

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

namespace linuxdesktop::settings {
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

ld_paths::resolver_options path_options_from_root_options(const root_options& options)
{
    ld_paths::resolver_options result;
    result.resource_root = options.resource_root;
    result.home_directory = options.home_directory;
    result.platform_defaults = options.platform_defaults;
    result.environment = options.environment;
    result.use_process_environment = options.use_process_environment;
    return result;
}

void apply_default_roots_from_paths(root_report& report, const ld_paths::resolver_report& paths)
{
    report.roots.config = selected_path_or_empty(paths, ld_paths::path_family::config);
    report.roots.data = selected_path_or_empty(paths, ld_paths::path_family::data);
    report.roots.state = selected_path_or_empty(paths, ld_paths::path_family::state);
    report.roots.cache = selected_path_or_empty(paths, ld_paths::path_family::cache);
    report.roots.resources = selected_location_or_empty(paths, ld_paths::location_role::resources);
    report.roots.runtime = selected_path_or_empty(paths, ld_paths::path_family::runtime);
}

int default_precedence(config_layer_kind kind)
{
    switch (kind) {
    case config_layer_kind::defaults:
        return 10;
    case config_layer_kind::global:
        return 20;
    case config_layer_kind::user:
        return 30;
    case config_layer_kind::local:
        return 40;
    case config_layer_kind::portable:
        return 50;
    case config_layer_kind::managed:
        return 60;
    case config_layer_kind::enforced:
        return 70;
    }
    return 0;
}

std::filesystem::path base_path_for(const app_roots& roots, persistence_class persistence, root_purpose purpose)
{
    switch (persistence) {
    case persistence_class::machine_local:
        if (purpose == root_purpose::cache || purpose == root_purpose::temp) {
            return roots.cache;
        }
        if (purpose == root_purpose::runtime) {
            return roots.runtime;
        }
        return roots.state;
    case persistence_class::portable:
        return roots.config;
    case persistence_class::ephemeral:
        return roots.cache;
    case persistence_class::managed:
        return roots.config / "managed";
    case persistence_class::enforced:
        return roots.config / "enforced";
    case persistence_class::roaming:
        break;
    }

    switch (purpose) {
    case root_purpose::resources:
        return roots.resources;
    case root_purpose::config:
    case root_purpose::plugin_config:
    case root_purpose::profiles:
    case root_purpose::backup:
    case root_purpose::component_config:
    case root_purpose::managed_config:
    case root_purpose::enforced_config:
    case root_purpose::custom:
        return roots.config;
    case root_purpose::data:
    case root_purpose::component_data:
        return roots.data;
    case root_purpose::state:
    case root_purpose::session:
    case root_purpose::logs:
    case root_purpose::component_state:
        return roots.state;
    case root_purpose::cache:
    case root_purpose::temp:
        return roots.cache;
    case root_purpose::runtime:
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
    case root_purpose::logs:
        return "logs";
    case root_purpose::profiles:
        return "profiles";
    case root_purpose::backup:
        return "backups";
    case root_purpose::temp:
        return "temp";
    case root_purpose::plugin_config:
        return std::filesystem::path{"plugins"} / "Config";
    case root_purpose::component_config:
        return "config";
    case root_purpose::component_data:
        return "data";
    case root_purpose::component_state:
        return "state";
    case root_purpose::managed_config:
        return "managed";
    case root_purpose::enforced_config:
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
    result.persistence = request.persistence;

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

    const auto base = base_path_for(roots, request.persistence, request.purpose);
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

layer_report build_layer_report(const app_identity& identity, const root_report& report)
{
    layer_report layers;
    const auto organization = detail::sanitize_segment(identity.organization);
    const auto application = detail::sanitize_segment(identity.application, "application");
    const auto app_leaf = organization.empty() ? application : organization + "/" + application;

    const auto add_layer = [&](config_layer_kind kind,
                               storage_backend backend,
                               std::string name,
                               std::filesystem::path path,
                               bool writable,
                               bool required = false,
                               bool enforced = false) {
        config_layer layer;
        layer.kind = kind;
        layer.backend = backend;
        layer.name = std::move(name);
        layer.path = std::move(path);
        layer.writable = writable;
        layer.required = required;
        layer.enforced = enforced;
        layer.precedence = default_precedence(kind);
        layers.candidates.push_back(layer);
    };

    add_layer(config_layer_kind::defaults, storage_backend::file, "defaults", report.roots.resources, false, false, false);
#if defined(_WIN32)
    add_layer(config_layer_kind::global, storage_backend::registry, "global", "HKLM/Software/" + app_leaf, false, false, false);
    add_layer(config_layer_kind::user, storage_backend::registry, "user", "HKCU/Software/" + app_leaf, true, false, false);
    add_layer(config_layer_kind::local, storage_backend::file, "local", report.roots.state, true, false, false);
    add_layer(config_layer_kind::managed, storage_backend::registry, "managed", "HKLM/Software/Policies/" + app_leaf, false, false, false);
    add_layer(config_layer_kind::enforced, storage_backend::registry, "enforced", "HKLM/Software/Policies/" + app_leaf, false, false, true);
#else
    add_layer(config_layer_kind::global, storage_backend::file, "global", std::filesystem::path{"/etc/xdg"} / app_leaf, false, false, false);
    add_layer(config_layer_kind::user, storage_backend::file, "user", report.roots.config, true, false, false);
    add_layer(config_layer_kind::local, storage_backend::file, "local", report.roots.state, true, false, false);
    add_layer(config_layer_kind::managed, storage_backend::file, "managed", std::filesystem::path{"/etc/dconf/db"} / application / "defaults", false, false, false);
    add_layer(config_layer_kind::enforced, storage_backend::file, "enforced", std::filesystem::path{"/etc/dconf/db"} / application / "locks", false, false, true);
#endif
    if (report.portable_active) {
        add_layer(config_layer_kind::portable, storage_backend::file, "portable", report.roots.config, true, false, false);
    }
    if (report.settings_override_active) {
        add_layer(config_layer_kind::user, storage_backend::override_values, "settings_override", report.roots.config, true, false, false);
    }
    if (report.sync_config_override_active) {
        add_layer(config_layer_kind::user, storage_backend::override_values, "sync_config_override", report.roots.config, true, false, false);
    }

    layers.active_read_order = layers.candidates;
    std::sort(layers.active_read_order.begin(), layers.active_read_order.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.precedence > rhs.precedence;
    });

    for (const auto& layer : layers.active_read_order) {
        if (layer.writable) {
            layers.active_write_layer = layer;
            break;
        }
    }

    return layers;
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

bool is_under_privileged_install_root(const std::filesystem::path& path, const root_options& options)
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

const named_root* find_named_root(const root_report& report, const std::string& name)
{
    const auto it = std::find_if(report.named_roots.begin(), report.named_roots.end(), [&](const auto& root) {
        return root.name == name;
    });
    return it == report.named_roots.end() ? nullptr : &*it;
}

const component_root_group* find_component_roots(const root_report& report, const std::string& name)
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

const config_layer* find_config_layer(const layer_report& report, config_layer_kind kind, const std::string& name)
{
    const auto it = std::find_if(report.candidates.begin(), report.candidates.end(), [&](const auto& layer) {
        return layer.kind == kind && (name.empty() || layer.name == name);
    });
    return it == report.candidates.end() ? nullptr : &*it;
}

root_report resolve_app_roots(const app_identity& identity, const root_options& options)
{
    root_report report;

    ld_paths::app_identity path_identity;
    path_identity.organization = identity.organization;
    path_identity.application = identity.application;
    const auto path_report = ld_paths::resolve_app_paths(path_identity, path_options_from_root_options(options));
    report.diagnostics.insert(report.diagnostics.end(), path_report.diagnostics.begin(), path_report.diagnostics.end());
    report.roots.resources = selected_location_or_empty(path_report, ld_paths::location_role::resources);

    report.portable = options.portable;

    if (options.settings_override) {
        if (options.settings_override->is_absolute()) {
            report.settings_override_active = true;
            report.roots.config = *options.settings_override;
            report.roots.data = *options.settings_override;
            report.roots.state = *options.settings_override;
            report.roots.cache = *options.settings_override / "cache";
            report.roots.runtime = std::filesystem::path{};
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "settings-override-relative",
                "Settings override must be absolute",
                *options.settings_override));
        }
    }

    report.portable_requested = options.portable_marker.has_value();
    if (!report.settings_override_active && options.portable_marker) {
        const auto marker = *options.portable_marker;
        std::error_code ec;
        if (std::filesystem::exists(marker, ec)) {
            if (options.allow_portable_root && options.portable != portable_level::off) {
                const auto portable_root = marker.parent_path();
                const auto privileged_path = !report.roots.resources.empty() ? report.roots.resources : portable_root;
                if (options.deny_portable_root_in_privileged_install &&
                    is_under_privileged_install_root(privileged_path, options)) {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::warning,
                        "portable-denied-privileged-install",
                        "Portable marker exists, but install root is privileged",
                        privileged_path));
                } else {
                    report.portable_active = true;
                    report.roots.config = portable_root;
                    report.roots.data = portable_root;
                    report.roots.state = portable_root;
                    report.roots.cache = portable_root / "cache";
                    report.roots.runtime = std::filesystem::path{};
                }
            } else {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::warning,
                    "portable-denied",
                    "Portable marker exists, but portable roots are disabled",
                    marker));
            }
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "portable-marker-missing",
                "Portable marker was requested but does not exist",
                marker));
        }
    }

    if (report.roots.config.empty() && !detail::has_error(report.diagnostics)) {
        apply_default_roots_from_paths(report, path_report);
    }

    if (options.sync_config_override) {
        if (report.settings_override_active) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "sync-config-override-ignored",
                "Sync config override was ignored because settings override is active",
                *options.sync_config_override));
        } else if (report.portable_active && !options.allow_sync_config_for_portable_root) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::info,
                "sync-config-override-ignored-portable",
                "Sync config override was ignored because portable root is active",
                *options.sync_config_override));
        } else if (options.sync_config_override->is_absolute()) {
            report.sync_config_override_active = true;
            report.roots.config = *options.sync_config_override;
        } else {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "sync-config-override-relative",
                "Sync config override must be absolute",
                *options.sync_config_override));
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

    report.layers = build_layer_report(identity, report);

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

} // namespace linuxdesktop::settings
