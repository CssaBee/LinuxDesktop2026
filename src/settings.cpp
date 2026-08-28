#include "linuxdesktop/settings.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

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

namespace linuxdesktop::settings {
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

unsigned long current_process_id()
{
#if defined(_WIN32)
    return static_cast<unsigned long>(GetCurrentProcessId());
#else
    return static_cast<unsigned long>(getpid());
#endif
}

std::filesystem::path unique_temp_path_for(const std::filesystem::path& target)
{
    const auto parent = target.parent_path();
    const auto stem = target.filename().string();
    for (int attempt = 0; attempt != 100; ++attempt) {
        auto candidate = parent / (stem + ".tmp." + std::to_string(current_process_id()) + "." + std::to_string(attempt));
        std::error_code ec;
        if (!std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return parent / (stem + ".tmp." + std::to_string(current_process_id()) + ".fallback");
}

bool replace_file(const std::filesystem::path& from, const std::filesystem::path& to, std::error_code& ec)
{
    ec.clear();
#if defined(_WIN32)
    if (MoveFileExW(from.wstring().c_str(), to.wstring().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return false;
    }
    return true;
#else
    std::filesystem::rename(from, to, ec);
    return !ec;
#endif
}

bool write_file_content(const std::filesystem::path& target, const std::string& content)
{
    std::ofstream output(target, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(output);
}

std::filesystem::path home_directory()
{
#if defined(_WIN32)
    if (const char* user_profile = std::getenv("USERPROFILE")) {
        return user_profile;
    }
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (drive && path) {
        return std::string(drive) + std::string(path);
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return home;
    }
#endif
    return {};
}

std::optional<std::filesystem::path> absolute_env_path(const char* name, std::vector<diagnostic>& diagnostics)
{
    const char* value = std::getenv(name);
    if (!value || !value[0]) {
        return std::nullopt;
    }

    std::filesystem::path path(value);
    if (!path.is_absolute()) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "relative-env-ignored",
            std::string(name) + " is relative and was ignored",
            path));
        return std::nullopt;
    }
    return path;
}

#if defined(_WIN32)
std::optional<std::filesystem::path> known_folder(REFKNOWNFOLDERID folder_id, std::vector<diagnostic>& diagnostics, const char* code)
{
    PWSTR path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_DEFAULT, nullptr, &path);
    if (FAILED(result) || !path) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            code,
            "Windows known-folder lookup failed"));
        return std::nullopt;
    }

    std::filesystem::path value(path);
    CoTaskMemFree(path);
    return value;
}
#endif

std::filesystem::path current_directory(std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    auto path = std::filesystem::current_path(ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "current-directory-failed",
            ec.message()));
        return {};
    }
    return path;
}

std::filesystem::path executable_resource_guess(std::vector<diagnostic>& diagnostics)
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
        return std::filesystem::path(buffer).parent_path();
    }
#else
    std::error_code ec;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return exe.parent_path();
    }
#endif
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        "resource-root-guessed",
        "Could not locate executable directory; using current directory as resource root"));
    return current_directory(diagnostics);
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

bool has_error(const std::vector<diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        if (item.level == severity::error) {
            return true;
        }
    }
    return false;
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
        return sanitize_segment(request.name);
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
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-name-empty",
            "Named root requires a non-empty name"));
        return result;
    }

    const auto relative = default_relative_path(request);
    if (relative.is_absolute()) {
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-relative-path-absolute",
            "Named root relative_path must be relative",
            relative));
        return result;
    }

    const auto base = base_path_for(roots, request.persistence, request.purpose);
    if (base.empty()) {
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-base-empty",
            "Named root base path could not be resolved"));
        return result;
    }

    result.path = relative.empty() ? base : base / relative;
    if (create_directories && request.create) {
        result.created = create_directory_for_root(result.path, result.diagnostics);
    }
    return result;
}

void append_unique_name_diagnostics(std::vector<named_root>& roots)
{
    std::vector<std::string> seen;
    for (auto& root : roots) {
        if (std::find(seen.begin(), seen.end(), root.name) != seen.end()) {
            root.diagnostics.push_back(make_diagnostic(
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
    const auto organization = sanitize_segment(identity.organization);
    const auto application = sanitize_segment(identity.application, "application");
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

std::string read_text(const std::filesystem::path& path, std::error_code& ec)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        ec = std::make_error_code(std::errc::io_error);
        return {};
    }

    ec.clear();
    return buffer.str();
}

} // namespace

root_report resolve_app_roots(const app_identity& identity, const root_options& options)
{
    root_report report;
    const auto organization = sanitize_segment(identity.organization);
    const auto application = sanitize_segment(identity.application, "application");
    const auto app_leaf = organization.empty() ? application : organization + "/" + application;

    report.portable = options.portable;

    report.roots.resources = options.resource_root.value_or(executable_resource_guess(report.diagnostics));

    if (options.settings_override) {
        if (options.settings_override->is_absolute()) {
            report.settings_override_active = true;
            report.roots.config = *options.settings_override;
            report.roots.data = *options.settings_override;
            report.roots.state = *options.settings_override;
            report.roots.cache = *options.settings_override / "cache";
            report.roots.runtime = std::filesystem::path{};
        } else {
            report.diagnostics.push_back(make_diagnostic(
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
                    report.diagnostics.push_back(make_diagnostic(
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
                report.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "portable-denied",
                    "Portable marker exists, but portable roots are disabled",
                    marker));
            }
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "portable-marker-missing",
                "Portable marker was requested but does not exist",
                marker));
        }
    }

    if (report.roots.config.empty() && !has_error(report.diagnostics)) {
#if defined(_WIN32)
        auto roaming = known_folder(FOLDERID_RoamingAppData, report.diagnostics, "known-folder-roaming-failed");
        auto local = known_folder(FOLDERID_LocalAppData, report.diagnostics, "known-folder-local-failed");
        const auto fallback_home = home_directory();

        if (!roaming && !fallback_home.empty()) {
            roaming = fallback_home / "AppData" / "Roaming";
        }
        if (!local && !fallback_home.empty()) {
            local = fallback_home / "AppData" / "Local";
        }

        report.roots.config = roaming.value_or(current_directory(report.diagnostics)) / app_leaf;
        report.roots.data = report.roots.config;
        report.roots.state = local.value_or(report.roots.config) / app_leaf / "state";
        report.roots.cache = local.value_or(report.roots.config) / app_leaf / "cache";
        report.roots.runtime = std::filesystem::path{};
#else
        const auto home = home_directory();
        auto config_home = absolute_env_path("XDG_CONFIG_HOME", report.diagnostics).value_or(home / ".config");
        auto data_home = absolute_env_path("XDG_DATA_HOME", report.diagnostics).value_or(home / ".local" / "share");
        auto state_home = absolute_env_path("XDG_STATE_HOME", report.diagnostics).value_or(home / ".local" / "state");
        auto cache_home = absolute_env_path("XDG_CACHE_HOME", report.diagnostics).value_or(home / ".cache");
        auto runtime_dir = absolute_env_path("XDG_RUNTIME_DIR", report.diagnostics).value_or(std::filesystem::path{});

        report.roots.config = config_home / app_leaf;
        report.roots.data = data_home / app_leaf;
        report.roots.state = state_home / app_leaf;
        report.roots.cache = cache_home / app_leaf;
        report.roots.runtime = runtime_dir.empty() ? runtime_dir : runtime_dir / application;
#endif
    }

    if (options.sync_config_override) {
        if (report.settings_override_active) {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "sync-config-override-ignored",
                "Sync config override was ignored because settings override is active",
                *options.sync_config_override));
        } else if (report.portable_active && !options.allow_sync_config_for_portable_root) {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "sync-config-override-ignored-portable",
                "Sync config override was ignored because portable root is active",
                *options.sync_config_override));
        } else if (options.sync_config_override->is_absolute()) {
            report.sync_config_override_active = true;
            report.roots.config = *options.sync_config_override;
        } else {
            report.diagnostics.push_back(make_diagnostic(
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
            component.diagnostics.push_back(make_diagnostic(
                severity::error,
                "component-root-name-empty",
                "Component root requires a non-empty name"));
        }

        const auto component_leaf = sanitize_segment(request.name, "component");
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

    if (options.create_directories && !has_error(report.diagnostics)) {
        create_directory_if_needed(report.roots.config, report.diagnostics);
        create_directory_if_needed(report.roots.data, report.diagnostics);
        create_directory_if_needed(report.roots.state, report.diagnostics);
        create_directory_if_needed(report.roots.cache, report.diagnostics);
        create_directory_if_needed(report.roots.session, report.diagnostics);
        create_directory_if_needed(report.roots.plugin_config, report.diagnostics);
        if (!report.roots.runtime.empty()) {
            create_directory_if_needed(report.roots.runtime, report.diagnostics);
        }
    }

    return report;
}

hydrate_report hydrate_config_bundle(const hydrate_options& options)
{
    hydrate_report report;
    if (options.create_target_root) {
        create_directory_if_needed(options.target_root, report.diagnostics);
    }

    if (has_error(report.diagnostics)) {
        return report;
    }

    for (const auto& file : options.files) {
        const auto target = options.target_root / file.name;
        const auto model = options.model_root / file.model_name;

        std::error_code ec;
        if (std::filesystem::exists(target, ec)) {
            report.skipped_existing.push_back(target);
            continue;
        }

        if (!std::filesystem::exists(model, ec)) {
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-model-missing" : "optional-model-missing",
                "Model file does not exist",
                model));
            continue;
        }

        std::filesystem::copy_file(model, target, std::filesystem::copy_options::none, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-copy-failed" : "optional-copy-failed",
                ec.message(),
                target));
            continue;
        }

        report.copied.push_back(target);
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "config-hydrated",
            "Copied model file into config bundle",
            target));
    }

    return report;
}

write_report write_with_backup(const write_options& options, validation_callback validate)
{
    write_report report;
    create_directory_if_needed(options.target.parent_path(), report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

    std::error_code ec;
    const auto write_target = options.atomic_replace ? unique_temp_path_for(options.target) : options.target;
    if (options.atomic_replace) {
        report.temp_path = write_target;
    }

    const auto backup = options.target.string() + ".bak";
    if (!options.atomic_replace && options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "backup-copy-failed",
                ec.message(),
                backup));
            return report;
        }
        report.backup_path = backup;
    }

    if (!write_file_content(write_target, options.content)) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            options.atomic_replace ? "temp-write-failed" : "write-failed",
            options.atomic_replace ? "Could not write temporary target content" : "Could not write target content",
            write_target));
        if (options.atomic_replace) {
            std::filesystem::remove(write_target, ec);
        }
        return report;
    }

    if (validate) {
        std::string validation_message;
        if (!validate(write_target, validation_message)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "validation-failed",
                validation_message.empty() ? "Written file failed validation" : validation_message,
                write_target));

            if (options.atomic_replace) {
                std::filesystem::remove(write_target, ec);
                if (ec) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        "temp-cleanup-failed",
                        ec.message(),
                        write_target));
                } else {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::info,
                        "temp-cleaned",
                        "Removed invalid temporary file",
                        write_target));
                }
            } else if (report.backup_path) {
                std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::error,
                        "backup-restore-failed",
                        ec.message(),
                        options.target));
                } else {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        "backup-restored",
                        "Restored previous target from backup",
                        options.target));
                }
            }
            return report;
        }
    }

    if (options.atomic_replace && options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "backup-copy-failed",
                ec.message(),
                backup));
            if (options.atomic_replace) {
                std::filesystem::remove(write_target, ec);
            }
            return report;
        }
        report.backup_path = backup;
    }

    if (options.atomic_replace) {
        if (!replace_file(write_target, options.target, ec)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "atomic-replace-failed",
                ec.message(),
                options.target));
            std::filesystem::remove(write_target, ec);
            return report;
        }
    }

    std::error_code read_ec;
    static_cast<void>(read_text(options.target, read_ec));
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "write-readback-failed",
            read_ec.message(),
            options.target));
        return report;
    }

    report.ok = true;
    report.diagnostics.push_back(make_diagnostic(
        severity::info,
        "write-ok",
        "Wrote target file",
        options.target));
    return report;
}

} // namespace linuxdesktop::settings
