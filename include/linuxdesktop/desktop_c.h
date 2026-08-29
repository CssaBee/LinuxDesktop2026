#pragma once

#include <stddef.h>

#ifdef _WIN32
#if defined(LD_DESKTOP_BUILD_SHARED)
#define LD_DESKTOP_API __declspec(dllexport)
#else
#define LD_DESKTOP_API
#endif
#else
#define LD_DESKTOP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LD_DESKTOP_VERSION_MAJOR 0
#define LD_DESKTOP_VERSION_MINOR 1
#define LD_DESKTOP_VERSION_PATCH 0

struct ld_desktop_diagnostic {
    int severity;
    char* code;
    char* message;
    char* path;
};

/*
 * `ld_desktop` owns desktop integration effects directly.
 * The C ABI is intentionally separate from `ld_settings`.
 */
struct ld_desktop_effect_options {
    int dry_run;
    int allow_global_write;
    int allow_desktop_integration_write;
    int allow_policy_write;
    const char* autostart_directory_override;
    const char* policy_defaults_directory_override;
    const char* policy_locks_directory_override;
};

struct ld_desktop_autostart_entry {
    const char* id;
    const char* display_name;
    const char* executable;
    const char* const* arguments;
    size_t argument_count;
    const char* working_directory;
    int enabled;
    int user_scope;
};

struct ld_desktop_effect_report {
    int ok;
    int dry_run;
    int enabled;
    char* path;
    struct ld_desktop_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_desktop_policy_entry {
    const char* id;
    const char* schema_id;
    const char* group;
    const char* key;
    const char* value;
    int enforced;
    int user_scope;
};

struct ld_desktop_policy_report {
    int ok;
    int dry_run;
    int present;
    int enforced;
    char* path;
    char* value;
    struct ld_desktop_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_DESKTOP_API void ld_desktop_effect_options_init(struct ld_desktop_effect_options* options);

LD_DESKTOP_API int ld_desktop_apply_autostart(
    const struct ld_desktop_autostart_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_effect_report* report);

LD_DESKTOP_API int ld_desktop_remove_autostart(
    const struct ld_desktop_autostart_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_effect_report* report);

LD_DESKTOP_API int ld_desktop_query_autostart(
    const struct ld_desktop_autostart_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_effect_report* report);

LD_DESKTOP_API void ld_desktop_free_effect_report(struct ld_desktop_effect_report* report);

LD_DESKTOP_API int ld_desktop_apply_policy(
    const struct ld_desktop_policy_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_policy_report* report);

LD_DESKTOP_API int ld_desktop_remove_policy(
    const struct ld_desktop_policy_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_policy_report* report);

LD_DESKTOP_API int ld_desktop_query_policy(
    const struct ld_desktop_policy_entry* entry,
    const struct ld_desktop_effect_options* options,
    struct ld_desktop_policy_report* report);

LD_DESKTOP_API void ld_desktop_free_policy_report(struct ld_desktop_policy_report* report);

LD_DESKTOP_API int ld_desktop_version_major(void);
LD_DESKTOP_API int ld_desktop_version_minor(void);
LD_DESKTOP_API int ld_desktop_version_patch(void);
LD_DESKTOP_API const char* ld_desktop_version_string(void);

#ifdef __cplusplus
}
#endif
