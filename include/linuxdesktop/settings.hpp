#pragma once

#include "linuxdesktop/core.hpp"
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

using ::linuxdesktop::diagnostic;
using ::linuxdesktop::severity;
using ::linuxdesktop::to_string;

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

enum class root_purpose {
    resources,
    config,
    data,
    state,
    cache,
    runtime,
    session,
    plugin_config,
    logs,
    profiles,
    backup,
    temp,
    component_config,
    component_data,
    component_state,
    managed_config,
    enforced_config,
    custom
};

enum class persistence_class {
    roaming,
    machine_local,
    portable,
    ephemeral,
    managed,
    enforced
};

enum class component_kind {
    plugin,
    embedded_tool,
    profile,
    language_pack,
    extension,
    custom
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

struct named_root_request {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path relative_path;
    bool create = true;
};

struct named_root {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path path;
    bool created = false;
    std::vector<diagnostic> diagnostics;
};

struct component_root_request {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root_request> roots;
};

struct component_root_group {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root> roots;
    std::vector<diagnostic> diagnostics;
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
    std::map<std::string, std::string> environment;

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
    bool use_process_environment = true;
    portable_level portable = portable_level::settings_only;
    std::vector<named_root_request> named_roots;
    std::vector<component_root_request> component_roots;
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
    portable_level portable = portable_level::off;
    std::vector<named_root> named_roots;
    std::vector<component_root_group> component_roots;
    layer_report layers;
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
    bool durable_write = false;
};

struct write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::optional<std::filesystem::path> temp_path;
    bool durable_write = false;
    std::vector<diagnostic> diagnostics;
};

using validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

std::string_view to_string(portable_level value);
std::string_view to_string(root_purpose value);
std::string_view to_string(persistence_class value);
std::string_view to_string(component_kind value);
std::string_view to_string(config_layer_kind value);
std::string_view to_string(storage_backend value);

const named_root* find_named_root(const root_report& report, const std::string& name);
const component_root_group* find_component_roots(const root_report& report, const std::string& name);
const named_root* find_component_named_root(const component_root_group& component, const std::string& name);
const config_layer* find_config_layer(
    const layer_report& report,
    config_layer_kind kind,
    const std::string& name = {});

root_report resolve_app_roots(const app_identity& identity, const root_options& options = {});

hydrate_report hydrate_config_bundle(const hydrate_options& options);

write_report write_with_backup(const write_options& options, validation_callback validate = {});

} // namespace linuxdesktop::settings
