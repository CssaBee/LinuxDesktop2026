#pragma once

#include "linuxdesktop/core.hpp"
#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/root.hpp"
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace linuxdesktop::settings {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

struct app_identity {
    std::string organization;
    std::string application;
};

enum class portable_level {
    off,
    settings_only,
    profile,
    clean
};

enum class config_layer_kind {
    defaults,
    global,
    user,
    local,
    portable,
    managed,
    enforced
};

enum class storage_backend {
    file,
    registry,
    null_backend,
    override_values,
    app_callback
};

struct config_layer {
    config_layer_kind kind = config_layer_kind::user;
    storage_backend backend = storage_backend::file;
    std::string name;
    std::filesystem::path path;
    bool writable = false;
    bool required = false;
    bool enforced = false;
    int precedence = 0;
};

struct layer_report {
    std::vector<config_layer> candidates;
    std::vector<config_layer> active_read_order;
    std::optional<config_layer> active_write_layer;
    std::vector<diagnostic> diagnostics;
};

struct root_options {
    std::optional<std::filesystem::path> resource_root;
    std::optional<std::filesystem::path> home_directory;
    std::optional<linuxdesktop::paths::platform_path_defaults> platform_defaults;
    std::map<std::string, std::string> environment;

    // Highest priority: one absolute root for config, data, state, and cache.
    std::optional<std::filesystem::path> settings_override;

    // Moves config/plugin config only; state and sessions stay machine-local.
    std::optional<std::filesystem::path> sync_config_override;

    // Activates portable roots when the marker exists and policy allows it.
    std::optional<std::filesystem::path> portable_marker;
    std::optional<linuxdesktop::root::portable_root_request> portable_root;

    // Used when portable roots must be denied under protected install locations.
    std::vector<std::filesystem::path> privileged_install_roots;
    bool allow_portable_root = true;
    bool deny_portable_root_in_privileged_install = false;
    bool allow_sync_config_for_portable_root = false;
    bool create_directories = true;
    bool use_process_environment = true;
    portable_level portable = portable_level::settings_only;
};

struct root_report {
    linuxdesktop::root::app_roots roots;
    bool portable_requested = false;
    bool portable_active = false;
    bool settings_override_active = false;
    bool sync_config_override_active = false;
    portable_level portable = portable_level::off;
    layer_report layers;
    std::vector<diagnostic> diagnostics;
};

class root_builder {
public:
    root_builder() = default;

    explicit root_builder(app_identity identity)
        : identity_(std::move(identity))
    {
    }

    root_builder& app(std::string organization, std::string application)
    {
        identity_.organization = std::move(organization);
        identity_.application = std::move(application);
        return *this;
    }

    root_builder& resource_root(std::filesystem::path path)
    {
        options_.resource_root = std::move(path);
        return *this;
    }

    root_builder& home_directory(std::optional<std::filesystem::path> path)
    {
        options_.home_directory = std::move(path);
        return *this;
    }

    root_builder& platform_defaults(std::optional<linuxdesktop::paths::platform_path_defaults> defaults)
    {
        options_.platform_defaults = std::move(defaults);
        return *this;
    }

    root_builder& environment(std::map<std::string, std::string> values)
    {
        options_.environment = std::move(values);
        return *this;
    }

    root_builder& use_process_environment(bool enabled)
    {
        options_.use_process_environment = enabled;
        return *this;
    }

    root_builder& settings_override(std::optional<std::filesystem::path> path)
    {
        options_.settings_override = std::move(path);
        return *this;
    }

    root_builder& sync_config_override(std::optional<std::filesystem::path> path)
    {
        options_.sync_config_override = std::move(path);
        return *this;
    }

    root_builder& portable_marker(std::optional<std::filesystem::path> path)
    {
        options_.portable_marker = std::move(path);
        return *this;
    }

    root_builder& portable_root(linuxdesktop::root::portable_root_request request)
    {
        options_.portable_root = std::move(request);
        return *this;
    }

    root_builder& portable(portable_level level)
    {
        options_.portable = level;
        return *this;
    }

    root_builder& allow_portable_root(bool enabled)
    {
        options_.allow_portable_root = enabled;
        return *this;
    }

    root_builder& deny_portable_root_in_privileged_install(bool enabled)
    {
        options_.deny_portable_root_in_privileged_install = enabled;
        return *this;
    }

    root_builder& allow_sync_config_for_portable_root(bool enabled)
    {
        options_.allow_sync_config_for_portable_root = enabled;
        return *this;
    }

    root_builder& privileged_install_roots(std::vector<std::filesystem::path> roots)
    {
        options_.privileged_install_roots = std::move(roots);
        return *this;
    }

    root_builder& create_directories(bool enabled)
    {
        options_.create_directories = enabled;
        return *this;
    }

    const app_identity& identity() const { return identity_; }
    const root_options& options() const { return options_; }
    root_options build() const { return options_; }
    root_report resolve() const;

private:
    app_identity identity_;
    root_options options_;
};

struct config_file {
    std::string name;
    std::string model_name;
    bool required = true;
};

struct config_defaults_options {
    std::filesystem::path model_root;
    std::filesystem::path target_root;
    std::vector<config_file> files;
    bool create_target_root = true;
};

struct config_defaults_report {
    std::vector<std::filesystem::path> copied;
    std::vector<std::filesystem::path> skipped_existing;
    std::vector<diagnostic> diagnostics;
};

struct write_options {
    std::filesystem::path target;
    std::string content;
    bool keep_backup = true;
    bool atomic_replace = true;
    bool durable_write = false;
};

using validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

// Settings writes protect the file image from partial-write corruption through
// backup/replacement options. They do not provide interprocess lost-update
// protection for app-owned read-modify-write merges.
struct write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::optional<std::filesystem::path> temp_path;
    bool durable_write = false;
    std::vector<diagnostic> diagnostics;
};

struct file_version_read_report;
struct versioned_write_request;

class file_version_token {
public:
    file_version_token() = default;

private:
    friend struct file_version_read_report;
    friend file_version_read_report read_file_version(const std::filesystem::path& target);
    friend file_version_token missing_file_version(std::filesystem::path target);
    friend write_report write_versioned(versioned_write_request request, validation_callback validate);

    std::filesystem::path target_;
    bool valid_ = false;
    bool existed_ = false;
    std::string content_;
};

struct file_version_read_report {
    bool ok = false;
    std::filesystem::path target;
    std::string content;
    file_version_token version;
    std::vector<diagnostic> diagnostics;
};

struct versioned_write_request {
    file_version_token expected_version;
    std::filesystem::path target;
    std::string content;
    bool keep_backup = true;
    bool atomic_replace = true;
    bool durable_write = false;
};

// Narrow helper for the common validated config write path.
// It always keeps the backup + atomic replacement behavior explicit and lets
// callers opt into durable flushing without rebuilding write_options.
struct common_config_write_request {
    std::filesystem::path target;
    std::string content;
    bool durable_write = false;
};

std::string_view to_string(portable_level value);
std::string_view to_string(config_layer_kind value);
std::string_view to_string(storage_backend value);

const config_layer* find_config_layer(
    const layer_report& report,
    config_layer_kind kind,
    const std::string& name = {});

root_report resolve_settings_roots(const app_identity& identity, const root_options& options = {});

inline root_report root_builder::resolve() const
{
    return resolve_settings_roots(identity_, options_);
}

config_defaults_report ensure_config_defaults(const config_defaults_options& options);

file_version_read_report read_file_version(const std::filesystem::path& target);
file_version_token missing_file_version(std::filesystem::path target);

write_report write_with_backup(const write_options& options, validation_callback validate = {});
write_report write_common_config(common_config_write_request request, validation_callback validate = {});
write_report write_versioned(versioned_write_request request, validation_callback validate = {});

} // namespace linuxdesktop::settings
