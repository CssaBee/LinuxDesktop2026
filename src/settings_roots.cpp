#include "settings_internal.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace linuxdesktop::settings {
namespace {

namespace ld_root = linuxdesktop::root;

ld_root::app_local_level to_root_app_local_level(portable_level value)
{
    switch (value) {
    case portable_level::off:
        return ld_root::app_local_level::off;
    case portable_level::profile:
        return ld_root::app_local_level::profile;
    case portable_level::clean:
        return ld_root::app_local_level::clean;
    case portable_level::settings_only:
        return ld_root::app_local_level::config_only;
    }
    return ld_root::app_local_level::config_only;
}

ld_root::options to_options(const root_options& options)
{
    ld_root::options result;
    result.resource_root = options.resource_root;
    result.home_directory = options.home_directory;
    result.platform_defaults = options.platform_defaults;
    result.environment = options.environment;
    result.app_root_override = options.settings_override;
    result.user_config_override = options.sync_config_override;
    result.app_local_marker = options.portable_marker;
    result.privileged_install_roots = options.privileged_install_roots;
    result.allow_app_local_root = options.allow_portable_root;
    result.deny_app_local_root_in_privileged_install = options.deny_portable_root_in_privileged_install;
    result.allow_user_config_for_app_local_root = options.allow_sync_config_for_portable_root;
    result.create_directories = options.create_directories;
    result.use_process_environment = options.use_process_environment;
    result.app_local = to_root_app_local_level(options.portable);
    return result;
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
    linuxdesktop::paths::ensure_directory_options options;
    options.dry_run = false;
    const auto report = linuxdesktop::paths::ensure_directory(path, options);
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

const config_layer* find_config_layer(const layer_report& report, config_layer_kind kind, const std::string& name)
{
    const auto it = std::find_if(report.candidates.begin(), report.candidates.end(), [&](const auto& layer) {
        return layer.kind == kind && (name.empty() || layer.name == name);
    });
    return it == report.candidates.end() ? nullptr : &*it;
}

root_report resolve_settings_roots(const app_identity& identity, const root_options& options)
{
    root_report result;

    ld_root::app_identity root_identity;
    root_identity.organization = identity.organization;
    root_identity.application = identity.application;
    const auto root_result = ld_root::resolve_app_roots(root_identity, to_options(options));

    result.roots = root_result.roots;
    result.portable_requested = root_result.app_local_requested;
    result.portable_active = root_result.app_local_active;
    result.settings_override_active = root_result.app_root_override_active;
    result.sync_config_override_active = root_result.user_config_override_active;
    result.portable = options.portable;
    result.diagnostics = root_result.diagnostics;

    for (auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == "app-root-override-relative") {
            diagnostic.code = "settings-override-relative";
            diagnostic.message = "Settings override must be absolute";
        } else if (diagnostic.code == "user-config-override-relative") {
            diagnostic.code = "sync-config-override-relative";
            diagnostic.message = "Sync config override must be absolute";
        } else if (diagnostic.code == "user-config-override-ignored") {
            diagnostic.code = "sync-config-override-ignored";
            diagnostic.message = "Sync config override was ignored because settings override is active";
        } else if (diagnostic.code == "user-config-override-ignored-app-local") {
            diagnostic.code = "sync-config-override-ignored-portable";
            diagnostic.message = "Sync config override was ignored because portable root is active";
        } else if (diagnostic.code == "app_local-denied-privileged-install") {
            diagnostic.code = "portable-denied-privileged-install";
            diagnostic.message = "Portable marker exists, but install root is privileged";
        } else if (diagnostic.code == "app_local-denied") {
            diagnostic.code = "portable-denied";
            diagnostic.message = "Portable marker exists, but portable roots are disabled";
        } else if (diagnostic.code == "app_local-marker-missing") {
            diagnostic.code = "portable-marker-missing";
            diagnostic.message = "Portable marker was requested but does not exist";
        }
    }

    result.layers = build_layer_report(identity, result);
    return result;
}

} // namespace linuxdesktop::settings
