#include "linuxdesktop/paths_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define LD_PATHS_SEPARATOR ';'
#else
#include <sys/stat.h>
#define LD_PATHS_SEPARATOR ':'
#endif

static const char* temp_root(void)
{
#if defined(_WIN32)
    const char* value = getenv("TEMP");
    return value ? value : "C:\\Temp";
#else
    return "/tmp";
#endif
}

static void make_path(char* buffer, size_t size, const char* leaf)
{
    size_t prefix_length;
    char separator;
#if defined(_WIN32)
    separator = '\\';
    snprintf(buffer, size, "%s\\linuxdesktop2026-c-paths\\", temp_root());
#else
    separator = '/';
    snprintf(buffer, size, "%s/linuxdesktop2026-c-paths/", temp_root());
#endif
    prefix_length = strlen(buffer);
    while (*leaf && prefix_length + 1 < size) {
        buffer[prefix_length++] = (*leaf == '/' || *leaf == '\\') ? separator : *leaf;
        ++leaf;
    }
    buffer[prefix_length] = '\0';
}

static const char* selected_path(const struct ld_paths_resolver_report* report, int family)
{
    size_t i;
    for (i = 0; i < report->selected_count; ++i) {
        if (report->selected[i].family == family) {
            return report->selected[i].path;
        }
    }
    return NULL;
}

static int has_candidate(
    const struct ld_paths_resolver_report* report,
    int family,
    int source,
    int selected)
{
    size_t i;
    for (i = 0; i < report->candidate_count; ++i) {
        if (report->candidates[i].family == family &&
            report->candidates[i].source == source &&
            report->candidates[i].selected == selected) {
            return 1;
        }
    }
    return 0;
}

static int has_diagnostic_code(
    const struct ld_paths_resolver_report* report,
    const char* code)
{
    size_t i;
    for (i = 0; i < report->diagnostic_count; ++i) {
        if (report->diagnostics[i].code && strcmp(report->diagnostics[i].code, code) == 0) {
            return 1;
        }
    }
    return 0;
}

static const struct ld_paths_plugin_path_set* plugin_set(
    const struct ld_paths_plugin_path_report* report,
    const char* name)
{
    size_t i;
    for (i = 0; i < report->set_count; ++i) {
        if (report->sets[i].name && strcmp(report->sets[i].name, name) == 0) {
            return &report->sets[i];
        }
    }
    return NULL;
}

int main(void)
{
    if (ld_paths_version_major() != LD_PATHS_VERSION_MAJOR ||
        ld_paths_version_minor() != LD_PATHS_VERSION_MINOR ||
        ld_paths_version_patch() != LD_PATHS_VERSION_PATCH ||
        strcmp(ld_paths_version_string(), "0.1.0") != 0 ||
        strcmp(ld_paths_severity_name(LD_PATHS_SEVERITY_WARNING), "warning") != 0 ||
        strcmp(ld_paths_path_family_name(LD_PATHS_FAMILY_CONFIG), "config") != 0 ||
        strcmp(ld_paths_path_family_name(LD_PATHS_FAMILY_TEMPLATES), "templates") != 0 ||
        strcmp(ld_paths_path_family_name(LD_PATHS_FAMILY_RUNTIME), "runtime") != 0 ||
        strcmp(ld_paths_candidate_source_name(LD_PATHS_SOURCE_XDG_BASE_DIR), "xdg_base_dir") != 0 ||
        strcmp(ld_paths_candidate_source_name(LD_PATHS_SOURCE_PLATFORM_DEFAULT), "platform_default") != 0 ||
        strcmp(ld_paths_plugin_path_kind_name(LD_PATHS_PLUGIN_VST3), "vst3") != 0) {
        fprintf(stderr,
            "name smoke failed: config=%s templates=%s source=%s plugin=%s\n",
            ld_paths_path_family_name(LD_PATHS_FAMILY_CONFIG),
            ld_paths_path_family_name(LD_PATHS_FAMILY_TEMPLATES),
            ld_paths_candidate_source_name(LD_PATHS_SOURCE_XDG_BASE_DIR),
            ld_paths_plugin_path_kind_name(LD_PATHS_PLUGIN_VST3));
        return EXIT_FAILURE;
    }

    char home[512];
    char config[512];
    char executable[512];
    char temp[512];
    char platform_home[512];
    char platform_runtime[512];
    char expected_config[768];
    char expected_data[768];
    char expected_state[768];
    char expected_cache[768];
    char expected_runtime[768];
    char expected_resources[768];
    char list_input[2048];
    char list_one[512];
    char list_two[512];
    char list_two_dirty[512];
    char plugin_home[512];
    char plugin_vendor[512];
    char plugin_vendor_dirty[512];
    char plugin_env_value[1024];
    make_path(home, sizeof(home), "home");
    make_path(config, sizeof(config), "config");
    make_path(executable, sizeof(executable), "opt\\linuxdesktop2026\\bin\\c-paths");
    make_path(temp, sizeof(temp), "tmp");
    make_path(platform_home, sizeof(platform_home), "platform-home");
    make_path(platform_runtime, sizeof(platform_runtime), "platform-runtime");
    make_path(list_one, sizeof(list_one), "one");
    make_path(list_two, sizeof(list_two), "two");
    make_path(list_two_dirty, sizeof(list_two_dirty), "two\\..\\two");
    make_path(plugin_home, sizeof(plugin_home), "plugin-home");
    make_path(plugin_vendor, sizeof(plugin_vendor), "vendor\\vst3");
    make_path(plugin_vendor_dirty, sizeof(plugin_vendor_dirty), "vendor\\vst3\\..\\vst3");

    struct ld_paths_resolver_options resolver_options;
    struct ld_paths_resolver_report resolver_report;
    memset(&resolver_report, 0, sizeof(resolver_report));
    ld_paths_resolver_options_init(&resolver_options);
    resolver_options.organization = "LinuxDesktop2026";
    resolver_options.application = "c-paths";
    resolver_options.config_override = config;
    resolver_options.home_directory = home;
    resolver_options.executable_path = executable;
    resolver_options.temp_override = temp;
    resolver_options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&resolver_options, &resolver_report)) {
        return EXIT_FAILURE;
    }
    snprintf(expected_config, sizeof(expected_config), "%s", config);
    snprintf(expected_resources, sizeof(expected_resources), "%s%clinuxdesktop2026-c-paths%copt%clinuxdesktop2026%cshare%cc-paths", temp_root(),
#if defined(_WIN32)
        '\\', '\\', '\\', '\\', '\\'
#else
        '/', '/', '/', '/', '/'
#endif
    );
    if (!selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG) ||
        strcmp(selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG), expected_config) != 0 ||
        !selected_path(&resolver_report, LD_PATHS_FAMILY_RESOURCES) ||
        strcmp(selected_path(&resolver_report, LD_PATHS_FAMILY_RESOURCES), expected_resources) != 0 ||
        resolver_report.candidate_count == 0) {
        fprintf(stderr,
            "resolver smoke failed: config=%s expected=%s resources=%s expected=%s candidates=%zu\n",
            selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG),
            expected_config,
            selected_path(&resolver_report, LD_PATHS_FAMILY_RESOURCES),
            expected_resources,
            resolver_report.candidate_count);
        ld_paths_free_resolver_report(&resolver_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_resolver_report(&resolver_report);
    if (resolver_report.selected != NULL || resolver_report.candidate_count != 0) {
        return EXIT_FAILURE;
    }

    memset(&resolver_report, 0, sizeof(resolver_report));
    ld_paths_resolver_options_init(&resolver_options);
    resolver_options.organization = "LinuxDesktop2026";
    resolver_options.application = "c-paths";
    resolver_options.xdg_config_home_default = platform_home;
    resolver_options.xdg_data_home_default = platform_home;
    resolver_options.xdg_state_home_default = platform_home;
    resolver_options.xdg_cache_home_default = platform_home;
    resolver_options.xdg_runtime_dir_default = platform_runtime;
    resolver_options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&resolver_options, &resolver_report)) {
        return EXIT_FAILURE;
    }
#if !defined(_WIN32)
    const char* resolved_config;
    const char* resolved_data;
    const char* resolved_state;
    const char* resolved_cache;
    const char* resolved_runtime;
    snprintf(expected_config, sizeof(expected_config), "%s%cLinuxDesktop2026%cc-paths", platform_home, '/', '/');
    snprintf(expected_data, sizeof(expected_data), "%s%cLinuxDesktop2026%cc-paths", platform_home, '/', '/');
    snprintf(expected_state, sizeof(expected_state), "%s%cLinuxDesktop2026%cc-paths", platform_home, '/', '/');
    snprintf(expected_cache, sizeof(expected_cache), "%s%cLinuxDesktop2026%cc-paths", platform_home, '/', '/');
    snprintf(expected_runtime, sizeof(expected_runtime), "%s%cc-paths", platform_runtime, '/');
    resolved_config = selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG);
    resolved_data = selected_path(&resolver_report, LD_PATHS_FAMILY_DATA);
    resolved_state = selected_path(&resolver_report, LD_PATHS_FAMILY_STATE);
    resolved_cache = selected_path(&resolver_report, LD_PATHS_FAMILY_CACHE);
    resolved_runtime = selected_path(&resolver_report, LD_PATHS_FAMILY_RUNTIME);
    if (!resolved_config || strcmp(resolved_config, expected_config) != 0 ||
        !resolved_data || strcmp(resolved_data, expected_data) != 0 ||
        !resolved_state || strcmp(resolved_state, expected_state) != 0 ||
        !resolved_cache || strcmp(resolved_cache, expected_cache) != 0 ||
        !resolved_runtime || strcmp(resolved_runtime, expected_runtime) != 0 ||
        !has_candidate(&resolver_report, LD_PATHS_FAMILY_CONFIG, LD_PATHS_SOURCE_PLATFORM_DEFAULT, 1) ||
        !has_candidate(&resolver_report, LD_PATHS_FAMILY_RUNTIME, LD_PATHS_SOURCE_PLATFORM_DEFAULT, 1)) {
        fprintf(stderr,
            "platform defaults failed: config=%s expected=%s runtime=%s expected=%s\n",
            resolved_config ? resolved_config : "(null)",
            expected_config,
            resolved_runtime ? resolved_runtime : "(null)",
            expected_runtime);
        ld_paths_free_resolver_report(&resolver_report);
        return EXIT_FAILURE;
    }
#endif
    ld_paths_free_resolver_report(&resolver_report);

    memset(&resolver_report, 0, sizeof(resolver_report));
    ld_paths_resolver_options_init(&resolver_options);
    resolver_options.organization = "LinuxDesktop2026";
    resolver_options.application = "c-paths";
    resolver_options.config_override = config;
    resolver_options.xdg_config_home_default = platform_home;
    resolver_options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&resolver_options, &resolver_report)) {
        return EXIT_FAILURE;
    }
    const char* override_config = selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG);
    if (!override_config || strcmp(override_config, config) != 0 ||
        !has_candidate(&resolver_report, LD_PATHS_FAMILY_CONFIG, LD_PATHS_SOURCE_EXPLICIT_OPTION, 1)) {
        ld_paths_free_resolver_report(&resolver_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_resolver_report(&resolver_report);

    memset(&resolver_report, 0, sizeof(resolver_report));
    ld_paths_resolver_options_init(&resolver_options);
    resolver_options.organization = "LinuxDesktop2026";
    resolver_options.application = "c-paths";
    resolver_options.home_directory = home;
    resolver_options.xdg_config_home_default = "relative-config-default";
    resolver_options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&resolver_options, &resolver_report)) {
        return EXIT_FAILURE;
    }
#if !defined(_WIN32)
    if (!has_diagnostic_code(&resolver_report, "paths.platform_default.relative_ignored") ||
        !has_candidate(&resolver_report, LD_PATHS_FAMILY_CONFIG, LD_PATHS_SOURCE_PLATFORM_DEFAULT, 0)) {
        ld_paths_free_resolver_report(&resolver_report);
        return EXIT_FAILURE;
    }
#endif
    ld_paths_free_resolver_report(&resolver_report);

    struct ld_paths_path_list_options list_options;
    struct ld_paths_path_list_report list_report;
    memset(&list_report, 0, sizeof(list_report));
    ld_paths_path_list_options_init(&list_options);
    list_options.separator = LD_PATHS_SEPARATOR;
    snprintf(list_input, sizeof(list_input), "%s%c%s%crelative%c%s",
        list_one,
        LD_PATHS_SEPARATOR,
        list_two_dirty,
        LD_PATHS_SEPARATOR,
        LD_PATHS_SEPARATOR,
        list_one);
    if (!ld_paths_parse_path_list(list_input, &list_options, &list_report) ||
        list_report.path_count != 2 ||
        strcmp(list_report.paths[0], list_one) != 0 ||
        strcmp(list_report.paths[1], list_two) != 0 ||
        list_report.diagnostic_count < 2) {
        ld_paths_free_path_list_report(&list_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_path_list_report(&list_report);

    int kinds[2];
    kinds[0] = LD_PATHS_PLUGIN_VST3;
    kinds[1] = LD_PATHS_PLUGIN_LV2;
    struct ld_paths_environment_entry plugin_env[1];
    plugin_env[0].name = "VST3_PATH";
    plugin_env[0].value = plugin_env_value;

    struct ld_paths_plugin_path_options plugin_options;
    struct ld_paths_plugin_path_report plugin_report;
    memset(&plugin_report, 0, sizeof(plugin_report));
    ld_paths_plugin_path_options_init(&plugin_options);
    plugin_options.kinds = kinds;
    plugin_options.kind_count = 2;
    plugin_options.home_directory = plugin_home;
    plugin_options.environment = plugin_env;
    plugin_options.environment_count = 1;
    plugin_options.use_process_environment = 0;
    plugin_options.list_options.separator = LD_PATHS_SEPARATOR;
    snprintf(plugin_env_value, sizeof(plugin_env_value), "%s%c%s", plugin_vendor, LD_PATHS_SEPARATOR, plugin_vendor_dirty);

    if (!ld_paths_resolve_plugin_path_sets(&plugin_options, &plugin_report)) {
        return EXIT_FAILURE;
    }
    const struct ld_paths_plugin_path_set* vst3 = plugin_set(&plugin_report, "vst3");
    const struct ld_paths_plugin_path_set* lv2 = plugin_set(&plugin_report, "lv2");
    if (!vst3 || vst3->has_kind != 1 || vst3->kind != LD_PATHS_PLUGIN_VST3 ||
        vst3->path_count == 0 || strcmp(vst3->paths[0], plugin_vendor) != 0 ||
        !lv2 || lv2->path_count == 0) {
        ld_paths_free_plugin_path_report(&plugin_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_plugin_path_report(&plugin_report);
    if (plugin_report.sets != NULL) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
