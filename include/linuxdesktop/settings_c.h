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

struct ld_settings_diagnostic {
    int severity;
    char* code;
    char* message;
    char* path;
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
    struct ld_settings_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_SETTINGS_API void ld_settings_root_options_init(struct ld_settings_root_options* options);

LD_SETTINGS_API int ld_settings_resolve_app_roots(
    const struct ld_settings_root_options* options,
    struct ld_settings_root_report* report);

LD_SETTINGS_API void ld_settings_free_root_report(struct ld_settings_root_report* report);

LD_SETTINGS_API const char* ld_settings_severity_name(int severity);

LD_SETTINGS_API int ld_settings_version_major(void);

LD_SETTINGS_API int ld_settings_version_minor(void);

LD_SETTINGS_API int ld_settings_version_patch(void);

LD_SETTINGS_API const char* ld_settings_version_string(void);

#ifdef __cplusplus
}
#endif
