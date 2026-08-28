#pragma once

#include <stddef.h>

#ifdef _WIN32
#if defined(LD_SETTINGS_BUILD_SHARED)
#define LD_SETTINGS_API __declspec(dllexport)
#else
#define LD_SETTINGS_API
#endif
#else
#define LD_SETTINGS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LD_SETTINGS_VERSION_MAJOR 0
#define LD_SETTINGS_VERSION_MINOR 1
#define LD_SETTINGS_VERSION_PATCH 0

enum ld_settings_severity {
    LD_SETTINGS_SEVERITY_INFO = 0,
    LD_SETTINGS_SEVERITY_WARNING = 1,
    LD_SETTINGS_SEVERITY_ERROR = 2
};

enum ld_settings_portable_level {
    LD_SETTINGS_PORTABLE_SETTINGS_ONLY = 0,
    LD_SETTINGS_PORTABLE_OFF = 1,
    LD_SETTINGS_PORTABLE_PROFILE = 2,
    LD_SETTINGS_PORTABLE_CLEAN = 3
};

enum ld_settings_root_purpose {
    LD_SETTINGS_ROOT_PURPOSE_RESOURCES = 0,
    LD_SETTINGS_ROOT_PURPOSE_CONFIG = 1,
    LD_SETTINGS_ROOT_PURPOSE_DATA = 2,
    LD_SETTINGS_ROOT_PURPOSE_STATE = 3,
    LD_SETTINGS_ROOT_PURPOSE_CACHE = 4,
    LD_SETTINGS_ROOT_PURPOSE_RUNTIME = 5,
    LD_SETTINGS_ROOT_PURPOSE_SESSION = 6,
    LD_SETTINGS_ROOT_PURPOSE_PLUGIN_CONFIG = 7,
    LD_SETTINGS_ROOT_PURPOSE_LOGS = 8,
    LD_SETTINGS_ROOT_PURPOSE_PROFILES = 9,
    LD_SETTINGS_ROOT_PURPOSE_BACKUP = 10,
    LD_SETTINGS_ROOT_PURPOSE_TEMP = 11,
    LD_SETTINGS_ROOT_PURPOSE_COMPONENT_CONFIG = 12,
    LD_SETTINGS_ROOT_PURPOSE_COMPONENT_DATA = 13,
    LD_SETTINGS_ROOT_PURPOSE_COMPONENT_STATE = 14,
    LD_SETTINGS_ROOT_PURPOSE_MANAGED_CONFIG = 15,
    LD_SETTINGS_ROOT_PURPOSE_ENFORCED_CONFIG = 16,
    LD_SETTINGS_ROOT_PURPOSE_CUSTOM = 17
};

enum ld_settings_persistence_class {
    LD_SETTINGS_PERSISTENCE_ROAMING = 0,
    LD_SETTINGS_PERSISTENCE_MACHINE_LOCAL = 1,
    LD_SETTINGS_PERSISTENCE_PORTABLE = 2,
    LD_SETTINGS_PERSISTENCE_EPHEMERAL = 3,
    LD_SETTINGS_PERSISTENCE_MANAGED = 4,
    LD_SETTINGS_PERSISTENCE_ENFORCED = 5
};

enum ld_settings_component_kind {
    LD_SETTINGS_COMPONENT_PLUGIN = 0,
    LD_SETTINGS_COMPONENT_EMBEDDED_TOOL = 1,
    LD_SETTINGS_COMPONENT_PROFILE = 2,
    LD_SETTINGS_COMPONENT_LANGUAGE_PACK = 3,
    LD_SETTINGS_COMPONENT_EXTENSION = 4,
    LD_SETTINGS_COMPONENT_CUSTOM = 5
};

enum ld_settings_config_layer_kind {
    LD_SETTINGS_CONFIG_LAYER_DEFAULTS = 0,
    LD_SETTINGS_CONFIG_LAYER_GLOBAL = 1,
    LD_SETTINGS_CONFIG_LAYER_USER = 2,
    LD_SETTINGS_CONFIG_LAYER_LOCAL = 3,
    LD_SETTINGS_CONFIG_LAYER_PORTABLE = 4,
    LD_SETTINGS_CONFIG_LAYER_MANAGED = 5,
    LD_SETTINGS_CONFIG_LAYER_ENFORCED = 6
};

enum ld_settings_storage_backend {
    LD_SETTINGS_STORAGE_FILE = 0,
    LD_SETTINGS_STORAGE_REGISTRY = 1,
    LD_SETTINGS_STORAGE_NULL = 2,
    LD_SETTINGS_STORAGE_OVERRIDE_VALUES = 3,
    LD_SETTINGS_STORAGE_APP_CALLBACK = 4
};

enum ld_settings_migration_action_kind {
    LD_SETTINGS_MIGRATION_COPY_FILE = 0,
    LD_SETTINGS_MIGRATION_MOVE_FILE = 1,
    LD_SETTINGS_MIGRATION_COPY_DIRECTORY = 2,
    LD_SETTINGS_MIGRATION_MOVE_DIRECTORY = 3,
    LD_SETTINGS_MIGRATION_IMPORT_REGISTRY = 4,
    LD_SETTINGS_MIGRATION_EXPORT_REGISTRY = 5,
    LD_SETTINGS_MIGRATION_WRITE_REGISTRY_VALUE = 6,
    LD_SETTINGS_MIGRATION_DELETE_REGISTRY_KEY = 7,
    LD_SETTINGS_MIGRATION_WRITE_AUTOSTART = 8,
    LD_SETTINGS_MIGRATION_WRITE_POLICY = 9
};

enum ld_settings_registry_hive {
    LD_SETTINGS_REGISTRY_CURRENT_USER = 0,
    LD_SETTINGS_REGISTRY_LOCAL_MACHINE = 1,
    LD_SETTINGS_REGISTRY_CLASSES_ROOT = 2,
    LD_SETTINGS_REGISTRY_USERS = 3,
    LD_SETTINGS_REGISTRY_CURRENT_CONFIG = 4
};

enum ld_settings_registry_view {
    LD_SETTINGS_REGISTRY_VIEW_NATIVE = 0,
    LD_SETTINGS_REGISTRY_VIEW_32 = 1,
    LD_SETTINGS_REGISTRY_VIEW_64 = 2
};

enum ld_settings_registry_value_type {
    LD_SETTINGS_REGISTRY_VALUE_NONE = 0,
    LD_SETTINGS_REGISTRY_VALUE_STRING = 1,
    LD_SETTINGS_REGISTRY_VALUE_EXPANDABLE_STRING = 2,
    LD_SETTINGS_REGISTRY_VALUE_MULTI_STRING = 3,
    LD_SETTINGS_REGISTRY_VALUE_DWORD = 4,
    LD_SETTINGS_REGISTRY_VALUE_QWORD = 5,
    LD_SETTINGS_REGISTRY_VALUE_BINARY = 6,
    LD_SETTINGS_REGISTRY_VALUE_UNKNOWN = 7
};

struct ld_settings_diagnostic {
    int severity;
    char* code;
    char* message;
    char* path;
};

struct ld_settings_named_root_request {
    const char* name;
    int purpose;
    int persistence;
    const char* relative_path;
    int create;
};

struct ld_settings_component_root_request {
    const char* name;
    int kind;
    const struct ld_settings_named_root_request* roots;
    size_t root_count;
};

struct ld_settings_named_root {
    char* name;
    int purpose;
    int persistence;
    char* path;
    int created;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_component_roots {
    char* name;
    int kind;
    struct ld_settings_named_root* roots;
    size_t root_count;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_config_layer {
    int kind;
    int backend;
    char* name;
    char* path;
    int writable;
    int required;
    int enforced;
    int precedence;
};

struct ld_settings_environment_entry {
    const char* name;
    const char* value;
};

struct ld_settings_root_options {
    const char* organization;
    const char* application;
    const char* resource_root;
    const char* settings_override;
    const char* sync_config_override;
    const char* portable_marker;
    const char* const* privileged_install_roots;
    size_t privileged_install_root_count;
    int allow_portable_root;
    int deny_portable_root_in_privileged_install;
    int allow_sync_config_for_portable_root;
    int create_directories;
    int portable_level;
    const struct ld_settings_named_root_request* named_roots;
    size_t named_root_count;
    const struct ld_settings_component_root_request* component_roots;
    size_t component_root_count;
    const char* home_directory;
    const struct ld_settings_environment_entry* environment;
    size_t environment_count;
    int use_process_environment;
};

struct ld_settings_root_report {
    char* resources;
    char* config;
    char* data;
    char* state;
    char* cache;
    char* runtime;
    char* session;
    char* plugin_config;
    int portable_requested;
    int portable_active;
    int settings_override_active;
    int sync_config_override_active;
    int portable_level;
    struct ld_settings_named_root* named_roots;
    size_t named_root_count;
    struct ld_settings_component_roots* component_roots;
    size_t component_root_count;
    struct ld_settings_config_layer* config_layers;
    size_t config_layer_count;
    struct ld_settings_config_layer* active_read_order;
    size_t active_read_order_count;
    struct ld_settings_config_layer* active_write_layer;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_effect_options {
    int dry_run;
    int allow_global_write;
    int allow_desktop_integration_write;
    int allow_policy_write;
    const char* autostart_directory_override;
    const char* policy_defaults_directory_override;
    const char* policy_locks_directory_override;
};

struct ld_settings_autostart_entry {
    const char* id;
    const char* display_name;
    const char* executable;
    const char* const* arguments;
    size_t argument_count;
    const char* working_directory;
    int enabled;
    int user_scope;
};

struct ld_settings_effect_report {
    int ok;
    int dry_run;
    int enabled;
    char* path;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_policy_entry {
    const char* id;
    const char* schema_id;
    const char* group;
    const char* key;
    const char* value;
    int enforced;
    int user_scope;
};

struct ld_settings_policy_report {
    int ok;
    int dry_run;
    int present;
    int enforced;
    char* path;
    char* value;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_config_file {
    const char* name;
    const char* model_name;
    int required;
};

struct ld_settings_hydrate_options {
    const char* model_root;
    const char* target_root;
    const struct ld_settings_config_file* files;
    size_t file_count;
    int create_target_root;
};

struct ld_settings_hydrate_report {
    char** copied;
    size_t copied_count;
    char** skipped_existing;
    size_t skipped_existing_count;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

typedef int (*ld_settings_validate_file_callback)(
    const char* path,
    char* message,
    size_t message_size,
    void* user_data);

struct ld_settings_write_options {
    const char* target;
    const char* content;
    size_t content_size;
    int keep_backup;
    int atomic_replace;
};

struct ld_settings_write_report {
    int ok;
    char* backup_path;
    char* temp_path;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_migration_action {
    int kind;
    const char* name;
    const char* source_path;
    const char* target_path;
    int dangerous;
    int requires_elevation;
};

struct ld_settings_migration_options {
    int dry_run;
    int allow_dangerous;
    int allow_elevation;
    int create_parent_directories;
    int overwrite_existing;
};

struct ld_settings_migration_action_result {
    struct ld_settings_migration_action action;
    int planned;
    int executed;
    int skipped;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_migration_report {
    int ok;
    int dry_run;
    struct ld_settings_migration_action* actions;
    size_t action_count;
    struct ld_settings_migration_action_result* results;
    size_t result_count;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_registry_key {
    int hive;
    const char* subkey;
    int view;
};

struct ld_settings_registry_options {
    int allow_hklm_write;
    int allow_policy_write;
    int allow_recursive_delete;
    int allow_import;
    int dry_run;
};

struct ld_settings_registry_value {
    const char* key_path;
    const char* name;
    int type;
    const unsigned char* bytes;
    size_t byte_count;
};

struct ld_settings_registry_operation_report {
    int ok;
    int dry_run;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_settings_registry_format_report {
    int ok;
    char* content;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_SETTINGS_API void ld_settings_root_options_init(struct ld_settings_root_options* options);

LD_SETTINGS_API void ld_settings_effect_options_init(struct ld_settings_effect_options* options);

LD_SETTINGS_API void ld_settings_hydrate_options_init(struct ld_settings_hydrate_options* options);

LD_SETTINGS_API void ld_settings_write_options_init(struct ld_settings_write_options* options);

LD_SETTINGS_API void ld_settings_migration_options_init(struct ld_settings_migration_options* options);

LD_SETTINGS_API void ld_settings_registry_options_init(struct ld_settings_registry_options* options);

LD_SETTINGS_API int ld_settings_resolve_app_roots(
    const struct ld_settings_root_options* options,
    struct ld_settings_root_report* report);

LD_SETTINGS_API void ld_settings_free_root_report(struct ld_settings_root_report* report);

LD_SETTINGS_API int ld_settings_apply_autostart(
    const struct ld_settings_autostart_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_effect_report* report);

LD_SETTINGS_API int ld_settings_remove_autostart(
    const struct ld_settings_autostart_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_effect_report* report);

LD_SETTINGS_API int ld_settings_query_autostart(
    const struct ld_settings_autostart_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_effect_report* report);

LD_SETTINGS_API void ld_settings_free_effect_report(struct ld_settings_effect_report* report);

LD_SETTINGS_API int ld_settings_apply_policy(
    const struct ld_settings_policy_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_policy_report* report);

LD_SETTINGS_API int ld_settings_remove_policy(
    const struct ld_settings_policy_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_policy_report* report);

LD_SETTINGS_API int ld_settings_query_policy(
    const struct ld_settings_policy_entry* entry,
    const struct ld_settings_effect_options* options,
    struct ld_settings_policy_report* report);

LD_SETTINGS_API void ld_settings_free_policy_report(struct ld_settings_policy_report* report);

LD_SETTINGS_API int ld_settings_hydrate_config_bundle(
    const struct ld_settings_hydrate_options* options,
    struct ld_settings_hydrate_report* report);

LD_SETTINGS_API void ld_settings_free_hydrate_report(struct ld_settings_hydrate_report* report);

LD_SETTINGS_API int ld_settings_write_with_backup(
    const struct ld_settings_write_options* options,
    ld_settings_validate_file_callback validate,
    void* user_data,
    struct ld_settings_write_report* report);

LD_SETTINGS_API void ld_settings_free_write_report(struct ld_settings_write_report* report);

LD_SETTINGS_API int ld_settings_plan_migration(
    const struct ld_settings_migration_action* actions,
    size_t action_count,
    const struct ld_settings_migration_options* options,
    struct ld_settings_migration_report* report);

LD_SETTINGS_API int ld_settings_execute_migration_plan(
    const struct ld_settings_migration_action* actions,
    size_t action_count,
    const struct ld_settings_migration_options* plan_options,
    const struct ld_settings_migration_options* execute_options,
    struct ld_settings_migration_report* report);

LD_SETTINGS_API void ld_settings_free_migration_report(struct ld_settings_migration_report* report);

LD_SETTINGS_API int ld_settings_registry_serialize_json(
    const struct ld_settings_registry_key* root,
    const struct ld_settings_registry_value* values,
    size_t value_count,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_parse_json(
    const char* content,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_serialize_reg(
    const struct ld_settings_registry_key* root,
    const struct ld_settings_registry_value* values,
    size_t value_count,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_parse_reg(
    const char* content,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_export_tree_json(
    const struct ld_settings_registry_key* root,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_import_tree_json(
    const struct ld_settings_registry_key* root,
    const char* content,
    const struct ld_settings_registry_options* options,
    struct ld_settings_registry_operation_report* report);

LD_SETTINGS_API int ld_settings_registry_export_tree_reg(
    const struct ld_settings_registry_key* root,
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API int ld_settings_registry_import_tree_reg(
    const struct ld_settings_registry_key* root,
    const char* content,
    const struct ld_settings_registry_options* options,
    struct ld_settings_registry_operation_report* report);

LD_SETTINGS_API void ld_settings_free_registry_operation_report(
    struct ld_settings_registry_operation_report* report);

LD_SETTINGS_API void ld_settings_free_registry_format_report(
    struct ld_settings_registry_format_report* report);

LD_SETTINGS_API const char* ld_settings_severity_name(int severity);

LD_SETTINGS_API int ld_settings_version_major(void);

LD_SETTINGS_API int ld_settings_version_minor(void);

LD_SETTINGS_API int ld_settings_version_patch(void);

LD_SETTINGS_API const char* ld_settings_version_string(void);

#ifdef __cplusplus
}
#endif
