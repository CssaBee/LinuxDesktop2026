#pragma once

#include <stddef.h>

#ifdef _WIN32
#if defined(LD_ROOT_BUILD_SHARED)
#define LD_ROOT_API __declspec(dllexport)
#else
#define LD_ROOT_API
#endif
#else
#define LD_ROOT_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LD_ROOT_VERSION_MAJOR 0
#define LD_ROOT_VERSION_MINOR 1
#define LD_ROOT_VERSION_PATCH 0

enum ld_root_severity {
    LD_ROOT_SEVERITY_INFO = 0,
    LD_ROOT_SEVERITY_WARNING = 1,
    LD_ROOT_SEVERITY_ERROR = 2
};

enum ld_root_app_local_level {
    LD_ROOT_APP_LOCAL_CONFIG_ONLY = 0,
    LD_ROOT_APP_LOCAL_OFF = 1,
    LD_ROOT_APP_LOCAL_PROFILE = 2,
    LD_ROOT_APP_LOCAL_CLEAN = 3
};

enum ld_root_purpose {
    LD_ROOT_PURPOSE_RESOURCES = 0,
    LD_ROOT_PURPOSE_CONFIG = 1,
    LD_ROOT_PURPOSE_DATA = 2,
    LD_ROOT_PURPOSE_STATE = 3,
    LD_ROOT_PURPOSE_CACHE = 4,
    LD_ROOT_PURPOSE_RUNTIME = 5,
    LD_ROOT_PURPOSE_SESSION = 6,
    LD_ROOT_PURPOSE_PLUGIN_CONFIG = 7,
    LD_ROOT_PURPOSE_LOGS = 8,
    LD_ROOT_PURPOSE_PROFILES = 9,
    LD_ROOT_PURPOSE_BACKUP = 10,
    LD_ROOT_PURPOSE_TEMP = 11,
    LD_ROOT_PURPOSE_COMPONENT_CONFIG = 12,
    LD_ROOT_PURPOSE_COMPONENT_DATA = 13,
    LD_ROOT_PURPOSE_COMPONENT_STATE = 14,
    LD_ROOT_PURPOSE_MANAGED_CONFIG = 15,
    LD_ROOT_PURPOSE_ENFORCED_CONFIG = 16,
    LD_ROOT_PURPOSE_CUSTOM = 17
};

enum ld_root_ownership {
    LD_ROOT_OWNERSHIP_USER_ROAMING = 0,
    LD_ROOT_OWNERSHIP_USER_LOCAL = 1,
    LD_ROOT_OWNERSHIP_APP_LOCAL = 2,
    LD_ROOT_OWNERSHIP_EPHEMERAL = 3,
    LD_ROOT_OWNERSHIP_MANAGED = 4,
    LD_ROOT_OWNERSHIP_ENFORCED = 5
};

enum ld_root_component_kind {
    LD_ROOT_COMPONENT_PLUGIN = 0,
    LD_ROOT_COMPONENT_EMBEDDED_TOOL = 1,
    LD_ROOT_COMPONENT_PROFILE = 2,
    LD_ROOT_COMPONENT_LANGUAGE_PACK = 3,
    LD_ROOT_COMPONENT_EXTENSION = 4,
    LD_ROOT_COMPONENT_CUSTOM = 5
};

struct ld_root_diagnostic {
    int severity;
    char* code;
    char* message;
    char* path;
};

struct ld_root_named_root_request {
    const char* name;
    int purpose;
    int ownership;
    const char* relative_path;
    int create;
};

struct ld_root_component_root_request {
    const char* name;
    int kind;
    const struct ld_root_named_root_request* roots;
    size_t root_count;
};

struct ld_root_named_root {
    char* name;
    int purpose;
    int ownership;
    char* path;
    int created;
    struct ld_root_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_root_component_roots {
    char* name;
    int kind;
    struct ld_root_named_root* roots;
    size_t root_count;
    struct ld_root_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_root_environment_entry {
    const char* name;
    const char* value;
};

struct ld_root_options {
    const char* organization;
    const char* application;
    const char* resource_root;
    const char* app_root_override;
    const char* user_config_override;
    const char* app_local_marker;
    const char* const* privileged_install_roots;
    size_t privileged_install_root_count;
    int allow_app_local_root;
    int deny_app_local_root_in_privileged_install;
    int allow_user_config_for_app_local_root;
    int create_directories;
    int app_local_level;
    const struct ld_root_named_root_request* named_roots;
    size_t named_root_count;
    const struct ld_root_component_root_request* component_roots;
    size_t component_root_count;
    const char* home_directory;
    const struct ld_root_environment_entry* environment;
    size_t environment_count;
    int use_process_environment;
};

struct ld_root_report {
    char* resources;
    char* config;
    char* data;
    char* state;
    char* cache;
    char* runtime;
    char* session;
    char* plugin_config;
    int app_local_requested;
    int app_local_active;
    int app_root_override_active;
    int user_config_override_active;
    int app_local_level;
    struct ld_root_named_root* named_roots;
    size_t named_root_count;
    struct ld_root_component_roots* component_roots;
    size_t component_root_count;
    struct ld_root_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_ROOT_API void ld_root_options_init(struct ld_root_options* options);
LD_ROOT_API int ld_root_resolve_app_roots(
    const struct ld_root_options* options,
    struct ld_root_report* report);
LD_ROOT_API void ld_root_free_report(struct ld_root_report* report);
LD_ROOT_API const char* ld_root_severity_name(int severity);
LD_ROOT_API const char* ld_root_app_local_level_name(int level);
LD_ROOT_API const char* ld_root_purpose_name(int purpose);
LD_ROOT_API const char* ld_root_ownership_name(int ownership);
LD_ROOT_API const char* ld_root_component_kind_name(int kind);

#ifdef __cplusplus
}
#endif
