#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace linuxdesktop::settings {

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
    std::optional<std::filesystem::path> settings_override;
    std::optional<std::filesystem::path> portable_marker;
    bool allow_portable_root = true;
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
};

struct write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::vector<diagnostic> diagnostics;
};

using validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

root_report resolve_app_roots(const app_identity& identity, const root_options& options = {});

hydrate_report hydrate_config_bundle(const hydrate_options& options);

write_report write_with_backup(const write_options& options, validation_callback validate = {});

std::string to_string(severity value);

} // namespace linuxdesktop::settings
