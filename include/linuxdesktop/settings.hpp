#pragma once

#include "linuxdesktop/core.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <string>
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
};

struct write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::optional<std::filesystem::path> temp_path;
    std::vector<diagnostic> diagnostics;
};

using validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

enum class migration_action_kind {
    copy_file,
    move_file,
    copy_directory,
    move_directory,
    import_registry,
    export_registry,
    write_registry_value,
    delete_registry_key,
    write_autostart,
    write_policy
};

struct migration_action {
    migration_action_kind kind = migration_action_kind::copy_file;
    std::string name;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
    bool dangerous = false;
    bool requires_elevation = false;
};

struct migration_options {
    bool dry_run = true;
    bool allow_dangerous = false;
    bool allow_elevation = false;
    bool create_parent_directories = true;
    bool overwrite_existing = false;
};

struct migration_plan {
    std::vector<migration_action> actions;
    bool dry_run = true;
    std::vector<diagnostic> diagnostics;
};

struct migration_action_result {
    migration_action action;
    bool planned = false;
    bool executed = false;
    bool skipped = false;
    std::vector<diagnostic> diagnostics;
};

struct migration_execution_report {
    bool ok = false;
    bool dry_run = true;
    std::vector<migration_action_result> actions;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(portable_level value);
std::string_view to_string(root_purpose value);
std::string_view to_string(persistence_class value);
std::string_view to_string(component_kind value);
std::string_view to_string(config_layer_kind value);
std::string_view to_string(storage_backend value);
std::string_view to_string(migration_action_kind value);

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

migration_plan plan_migration(std::vector<migration_action> actions, const migration_options& options = {});

migration_execution_report execute_migration_plan(
    const migration_plan& plan,
    const migration_options& options = {});

namespace registry {

enum class hive {
    current_user,
    local_machine,
    classes_root,
    users,
    current_config
};

enum class view {
    native,
    registry_32,
    registry_64
};

enum class value_type {
    none,
    string,
    expandable_string,
    multi_string,
    dword,
    qword,
    binary,
    unknown
};

struct key {
    hive root = hive::current_user;
    std::string subkey;
    view registry_view = view::native;
};

struct value {
    std::string name;
    value_type type = value_type::none;
    std::vector<std::byte> bytes;
};

struct options {
    bool allow_hklm_write = false;
    bool allow_policy_write = false;
    bool allow_recursive_delete = false;
    bool allow_import = false;
    bool dry_run = true;
};

struct operation_report {
    bool ok = false;
    bool dry_run = false;
    std::vector<diagnostic> diagnostics;
};

struct value_report {
    bool ok = false;
    std::optional<value> item;
    std::vector<diagnostic> diagnostics;
};

struct values_report {
    bool ok = false;
    std::vector<value> values;
    std::vector<diagnostic> diagnostics;
};

struct subkeys_report {
    bool ok = false;
    std::vector<std::string> names;
    std::vector<diagnostic> diagnostics;
};

struct snapshot_value {
    std::string key_path;
    value item;
};

struct snapshot {
    key root;
    std::vector<snapshot_value> values;
};

struct snapshot_report {
    bool ok = false;
    std::optional<snapshot> item;
    std::vector<diagnostic> diagnostics;
};

struct format_report {
    bool ok = false;
    std::string content;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(hive value);
std::string_view to_string(view value);
std::string_view to_string(value_type value);

value_report read_value(const key& key, const std::string& name);
operation_report write_value(const key& key, const value& value, const options& options = {});
operation_report delete_value(const key& key, const std::string& name, const options& options = {});
operation_report delete_key(const key& key, const options& options = {});
values_report enumerate_values(const key& key);
subkeys_report enumerate_subkeys(const key& key);
format_report serialize_snapshot_json(const snapshot& snapshot);
snapshot_report parse_snapshot_json(std::string_view content);
format_report serialize_snapshot_reg(const snapshot& snapshot);
snapshot_report parse_snapshot_reg(std::string_view content);
format_report export_tree_json(const key& key);
operation_report import_tree_json(const key& key, std::string_view content, const options& options = {});
format_report export_tree_reg(const key& key);
operation_report import_tree_reg(const key& key, std::string_view content, const options& options = {});

} // namespace registry

namespace effects {

struct autostart_entry {
    std::string id;
    std::string display_name;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    bool enabled = true;
    bool user_scope = true;
};

struct apply_options {
    bool dry_run = true;
    bool allow_global_write = false;
    bool allow_desktop_integration_write = false;
    bool allow_policy_write = false;
    std::optional<std::filesystem::path> autostart_directory_override;
    std::optional<std::filesystem::path> policy_defaults_directory_override;
    std::optional<std::filesystem::path> policy_locks_directory_override;
};

struct effect_report {
    bool ok = false;
    bool dry_run = false;
    bool enabled = false;
    std::optional<std::filesystem::path> path;
    std::vector<diagnostic> diagnostics;
};

struct policy_entry {
    std::string id;
    std::string schema_id;
    std::string group;
    std::string key;
    std::string value;
    bool enforced = false;
    bool user_scope = false;
};

struct policy_report {
    bool ok = false;
    bool dry_run = false;
    bool present = false;
    bool enforced = false;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> value;
    std::vector<diagnostic> diagnostics;
};

effect_report apply_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report remove_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report query_autostart(const autostart_entry& entry, const apply_options& options = {});

policy_report apply_policy(const policy_entry& entry, const apply_options& options = {});
policy_report remove_policy(const policy_entry& entry, const apply_options& options = {});
policy_report query_policy(const policy_entry& entry, const apply_options& options = {});

} // namespace effects

} // namespace linuxdesktop::settings
