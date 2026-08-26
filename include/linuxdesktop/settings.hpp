#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace linuxdesktop::settings {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

enum class severity {
    info,
    warning,
    error
};

struct diagnostic {
    severity level = severity::info;
    std::string code;
    std::string message;
    std::filesystem::path path;
};

struct app_identity {
    std::string organization;
    std::string application;
};

struct root_options {
    std::optional<std::filesystem::path> resource_root;

    // Highest priority: one absolute root for config, data, state, and cache.
    std::optional<std::filesystem::path> settings_override;

    // Moves config/plugin config only; state and sessions stay machine-local.
    std::optional<std::filesystem::path> sync_config_override;

    // Activates app-local roots when the marker exists and policy allows it.
    std::optional<std::filesystem::path> portable_marker;

    // Used when portable roots must be denied under protected install locations.
    std::vector<std::filesystem::path> privileged_install_roots;
    bool allow_portable_root = true;
    bool deny_portable_root_in_privileged_install = false;
    bool allow_sync_config_for_portable_root = false;
    bool create_directories = true;
};

struct app_roots {
    std::filesystem::path resources;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path state;
    std::filesystem::path cache;
    std::filesystem::path runtime;
    std::filesystem::path session;
    std::filesystem::path plugin_config;
};

struct root_report {
    app_roots roots;
    bool portable_requested = false;
    bool portable_active = false;
    bool settings_override_active = false;
    bool sync_config_override_active = false;
    std::vector<diagnostic> diagnostics;
};

struct config_file {
    std::string name;
    std::string model_name;
    bool required = true;
};

struct hydrate_options {
    std::filesystem::path model_root;
    std::filesystem::path target_root;
    std::vector<config_file> files;
    bool create_target_root = true;
};

struct hydrate_report {
    std::vector<std::filesystem::path> copied;
    std::vector<std::filesystem::path> skipped_existing;
    std::vector<diagnostic> diagnostics;
};

struct write_options {
    std::filesystem::path target;
    std::string content;
    bool keep_backup = true;
    bool atomic_replace = true;
};

struct write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::optional<std::filesystem::path> temp_path;
    std::vector<diagnostic> diagnostics;
};

using validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

root_report resolve_app_roots(const app_identity& identity, const root_options& options = {});

hydrate_report hydrate_config_bundle(const hydrate_options& options);

write_report write_with_backup(const write_options& options, validation_callback validate = {});

std::string to_string(severity value);

} // namespace linuxdesktop::settings
