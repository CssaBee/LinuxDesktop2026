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
    struct ld_settings_config_layer* config_layers;
    size_t config_layer_count;
    struct ld_settings_config_layer* active_read_order;
    size_t active_read_order_count;
    struct ld_settings_config_layer* active_write_layer;
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
    int durable_write;
};

struct ld_settings_write_report {
    int ok;
    char* backup_path;
    char* temp_path;
    int durable_write;
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_SETTINGS_API void ld_settings_root_options_init(struct ld_settings_root_options* options);

LD_SETTINGS_API void ld_settings_hydrate_options_init(struct ld_settings_hydrate_options* options);

LD_SETTINGS_API void ld_settings_write_options_init(struct ld_settings_write_options* options);

LD_SETTINGS_API int ld_settings_resolve_app_roots(
    const struct ld_settings_root_options* options,
    struct ld_settings_root_report* report);

LD_SETTINGS_API void ld_settings_free_root_report(struct ld_settings_root_report* report);

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

LD_SETTINGS_API const char* ld_settings_severity_name(int severity);

LD_SETTINGS_API int ld_settings_version_major(void);

LD_SETTINGS_API int ld_settings_version_minor(void);

LD_SETTINGS_API int ld_settings_version_patch(void);

LD_SETTINGS_API const char* ld_settings_version_string(void);

#ifdef __cplusplus
}
#endif
