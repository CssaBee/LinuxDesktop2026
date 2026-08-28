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

LD_SETTINGS_API void ld_settings_root_options_init(struct ld_settings_root_options* options);

LD_SETTINGS_API void ld_settings_effect_options_init(struct ld_settings_effect_options* options);

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

LD_SETTINGS_API const char* ld_settings_severity_name(int severity);

LD_SETTINGS_API int ld_settings_version_major(void);

LD_SETTINGS_API int ld_settings_version_minor(void);

LD_SETTINGS_API int ld_settings_version_patch(void);

LD_SETTINGS_API const char* ld_settings_version_string(void);

#ifdef __cplusplus
}
#endif
