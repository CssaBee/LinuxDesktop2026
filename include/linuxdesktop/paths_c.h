#pragma once

#include <stddef.h>

#ifdef _WIN32
#if defined(LD_PATHS_BUILD_SHARED)
#define LD_PATHS_API __declspec(dllexport)
#else
#define LD_PATHS_API
#endif
#else
#define LD_PATHS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LD_PATHS_VERSION_MAJOR 0
#define LD_PATHS_VERSION_MINOR 1
#define LD_PATHS_VERSION_PATCH 0

enum ld_paths_severity {
    LD_PATHS_SEVERITY_INFO = 0,
    LD_PATHS_SEVERITY_WARNING = 1,
    LD_PATHS_SEVERITY_ERROR = 2
};

enum ld_paths_path_family {
    LD_PATHS_FAMILY_CONFIG = 0,
    LD_PATHS_FAMILY_DATA = 1,
    LD_PATHS_FAMILY_STATE = 2,
    LD_PATHS_FAMILY_CACHE = 3,
    LD_PATHS_FAMILY_TEMP = 4,
    LD_PATHS_FAMILY_DOCUMENTS = 5,
    LD_PATHS_FAMILY_DESKTOP = 6,
    LD_PATHS_FAMILY_DOWNLOADS = 7,
    LD_PATHS_FAMILY_MUSIC = 8,
    LD_PATHS_FAMILY_PICTURES = 9,
    LD_PATHS_FAMILY_VIDEOS = 10,
    LD_PATHS_FAMILY_TEMPLATES = 11,
    LD_PATHS_FAMILY_PUBLIC_SHARE = 12,
    LD_PATHS_FAMILY_RUNTIME = 13
};

enum ld_paths_location_role {
    LD_PATHS_LOCATION_EXECUTABLE = 0,
    LD_PATHS_LOCATION_EXECUTABLE_DIRECTORY = 1,
    LD_PATHS_LOCATION_INSTALL_PREFIX = 2,
    LD_PATHS_LOCATION_RESOURCES = 3
};

enum ld_paths_candidate_source {
    LD_PATHS_SOURCE_EXPLICIT_OPTION = 0,
    LD_PATHS_SOURCE_ENVIRONMENT = 1,
    LD_PATHS_SOURCE_XDG_BASE_DIR = 2,
    LD_PATHS_SOURCE_XDG_USER_DIR = 3,
    LD_PATHS_SOURCE_KNOWN_FOLDER = 4,
    LD_PATHS_SOURCE_EXECUTABLE_RELATIVE = 5,
    LD_PATHS_SOURCE_LEGACY = 6,
    LD_PATHS_SOURCE_SITE_DEFAULT = 7,
    LD_PATHS_SOURCE_FALLBACK = 8,
    LD_PATHS_SOURCE_PLATFORM_DEFAULT = 9,
    LD_PATHS_SOURCE_WINE_PREFIX = 10
};

enum ld_paths_plugin_path_kind {
    LD_PATHS_PLUGIN_LADSPA = 0,
    LD_PATHS_PLUGIN_DSSI = 1,
    LD_PATHS_PLUGIN_LV2 = 2,
    LD_PATHS_PLUGIN_VST2 = 3,
    LD_PATHS_PLUGIN_VST3 = 4,
    LD_PATHS_PLUGIN_CLAP = 5,
    LD_PATHS_PLUGIN_SF2 = 6,
    LD_PATHS_PLUGIN_SFZ = 7,
    LD_PATHS_PLUGIN_JSFX = 8
};

struct ld_paths_diagnostic {
    int severity;
    char* code;
    char* message;
    char* path;
};

struct ld_paths_environment_entry {
    const char* name;
    const char* value;
};

struct ld_paths_selected_path {
    int family;
    char* path;
};

struct ld_paths_selected_location {
    int role;
    char* path;
};

struct ld_paths_candidate {
    int family;
    int source;
    char* path;
    int selected;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_location_candidate {
    int role;
    int source;
    char* path;
    int selected;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_path_list_candidate {
    int source;
    char* path;
    int selected;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_resolver_options {
    const char* organization;
    const char* application;
    const char* config_override;
    const char* data_override;
    const char* state_override;
    const char* cache_override;
    const char* temp_override;
    const char* resource_root;
    const char* install_prefix;
    const char* executable_path;
    const char* home_directory;
    const struct ld_paths_environment_entry* environment;
    size_t environment_count;
    int use_process_environment;
    const char* runtime_override;

    /* Platform defaults are lower precedence than explicit overrides,
       environment entries, process environment, and OS APIs. They are used
       only when the platform-owned root is otherwise unavailable. */
    const char* xdg_config_home_default;
    const char* xdg_data_home_default;
    const char* xdg_state_home_default;
    const char* xdg_cache_home_default;
    const char* xdg_runtime_dir_default;
    const char* windows_roaming_appdata_default;
    const char* windows_local_appdata_default;
};

struct ld_paths_resolver_report {
    struct ld_paths_selected_path* selected;
    size_t selected_count;
    struct ld_paths_selected_location* selected_locations;
    size_t selected_location_count;
    struct ld_paths_candidate* candidates;
    size_t candidate_count;
    struct ld_paths_location_candidate* location_candidates;
    size_t location_candidate_count;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_path_list_options {
    int require_absolute;
    int drop_duplicates;
    char separator;
};

struct ld_paths_path_list_report {
    char** paths;
    size_t path_count;
    struct ld_paths_path_list_candidate* candidates;
    size_t candidate_count;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_plugin_path_set {
    char* name;
    int kind;
    int has_kind;
    char** paths;
    size_t path_count;
};

struct ld_paths_plugin_path_candidate {
    char* set_name;
    int kind;
    int has_kind;
    int source;
    char* path;
    int selected;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

struct ld_paths_plugin_path_options {
    const int* kinds;
    size_t kind_count;
    const char* home_directory;
    const char* wine_prefix;
    int include_wine_prefix_defaults;
    const struct ld_paths_environment_entry* environment;
    size_t environment_count;
    int use_process_environment;
    struct ld_paths_path_list_options list_options;
};

struct ld_paths_plugin_path_report {
    struct ld_paths_plugin_path_set* sets;
    size_t set_count;
    struct ld_paths_plugin_path_candidate* candidates;
    size_t candidate_count;
    struct ld_paths_diagnostic* diagnostics;
    size_t diagnostic_count;
};

LD_PATHS_API void ld_paths_resolver_options_init(struct ld_paths_resolver_options* options);

LD_PATHS_API void ld_paths_path_list_options_init(struct ld_paths_path_list_options* options);

LD_PATHS_API void ld_paths_plugin_path_options_init(struct ld_paths_plugin_path_options* options);

LD_PATHS_API int ld_paths_resolve_app_paths(
    const struct ld_paths_resolver_options* options,
    struct ld_paths_resolver_report* report);

LD_PATHS_API void ld_paths_free_resolver_report(struct ld_paths_resolver_report* report);

LD_PATHS_API int ld_paths_parse_path_list(
    const char* value,
    const struct ld_paths_path_list_options* options,
    struct ld_paths_path_list_report* report);

LD_PATHS_API void ld_paths_free_path_list_report(struct ld_paths_path_list_report* report);

LD_PATHS_API int ld_paths_resolve_plugin_path_sets(
    const struct ld_paths_plugin_path_options* options,
    struct ld_paths_plugin_path_report* report);

LD_PATHS_API void ld_paths_free_plugin_path_report(struct ld_paths_plugin_path_report* report);

LD_PATHS_API const char* ld_paths_severity_name(int severity);

LD_PATHS_API const char* ld_paths_path_family_name(int family);

LD_PATHS_API const char* ld_paths_location_role_name(int role);

LD_PATHS_API const char* ld_paths_candidate_source_name(int source);

LD_PATHS_API const char* ld_paths_plugin_path_kind_name(int kind);

LD_PATHS_API int ld_paths_version_major(void);

LD_PATHS_API int ld_paths_version_minor(void);

LD_PATHS_API int ld_paths_version_patch(void);

LD_PATHS_API const char* ld_paths_version_string(void);

#ifdef __cplusplus
}
#endif
