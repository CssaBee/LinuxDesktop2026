#include "linuxdesktop/paths_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        strcmp(ld_paths_candidate_source_name(LD_PATHS_SOURCE_XDG_BASE_DIR), "xdg_base_dir") != 0 ||
        strcmp(ld_paths_plugin_path_kind_name(LD_PATHS_PLUGIN_VST3), "vst3") != 0) {
        return EXIT_FAILURE;
    }

    struct ld_paths_environment_entry env[1];
    env[0].name = "XDG_CONFIG_HOME";
    env[0].value = "/tmp/linuxdesktop2026-c-paths/config";

    struct ld_paths_resolver_options resolver_options;
    struct ld_paths_resolver_report resolver_report;
    memset(&resolver_report, 0, sizeof(resolver_report));
    ld_paths_resolver_options_init(&resolver_options);
    resolver_options.organization = "LinuxDesktop2026";
    resolver_options.application = "c-paths";
    resolver_options.home_directory = "/tmp/linuxdesktop2026-c-paths/home";
    resolver_options.executable_path = "/opt/linuxdesktop2026/bin/c-paths";
    resolver_options.temp_override = "/tmp/linuxdesktop2026-c-paths/tmp";
    resolver_options.environment = env;
    resolver_options.environment_count = 1;
    resolver_options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&resolver_options, &resolver_report)) {
        return EXIT_FAILURE;
    }
    if (!selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG) ||
        strcmp(selected_path(&resolver_report, LD_PATHS_FAMILY_CONFIG),
            "/tmp/linuxdesktop2026-c-paths/config/LinuxDesktop2026/c-paths") != 0 ||
        !selected_path(&resolver_report, LD_PATHS_FAMILY_RESOURCES) ||
        strcmp(selected_path(&resolver_report, LD_PATHS_FAMILY_RESOURCES),
            "/opt/linuxdesktop2026/share/c-paths") != 0 ||
        resolver_report.candidate_count == 0) {
        ld_paths_free_resolver_report(&resolver_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_resolver_report(&resolver_report);
    if (resolver_report.selected != NULL || resolver_report.candidate_count != 0) {
        return EXIT_FAILURE;
    }

    struct ld_paths_path_list_options list_options;
    struct ld_paths_path_list_report list_report;
    memset(&list_report, 0, sizeof(list_report));
    ld_paths_path_list_options_init(&list_options);
    list_options.separator = ':';
    if (!ld_paths_parse_path_list("/one:/two/../two:relative:/one", &list_options, &list_report) ||
        list_report.path_count != 2 ||
        strcmp(list_report.paths[0], "/one") != 0 ||
        strcmp(list_report.paths[1], "/two") != 0 ||
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
    plugin_env[0].value = "/vendor/vst3";

    struct ld_paths_plugin_path_options plugin_options;
    struct ld_paths_plugin_path_report plugin_report;
    memset(&plugin_report, 0, sizeof(plugin_report));
    ld_paths_plugin_path_options_init(&plugin_options);
    plugin_options.kinds = kinds;
    plugin_options.kind_count = 2;
    plugin_options.home_directory = "/home/tester";
    plugin_options.environment = plugin_env;
    plugin_options.environment_count = 1;
    plugin_options.use_process_environment = 0;
    plugin_options.list_options.separator = ':';

    if (!ld_paths_resolve_plugin_path_sets(&plugin_options, &plugin_report)) {
        return EXIT_FAILURE;
    }
    const struct ld_paths_plugin_path_set* vst3 = plugin_set(&plugin_report, "vst3");
    const struct ld_paths_plugin_path_set* lv2 = plugin_set(&plugin_report, "lv2");
    if (!vst3 || vst3->has_kind != 1 || vst3->kind != LD_PATHS_PLUGIN_VST3 ||
        vst3->path_count == 0 || strcmp(vst3->paths[0], "/vendor/vst3") != 0 ||
        !lv2 || lv2->path_count == 0 || strcmp(lv2->paths[0], "/home/tester/.lv2") != 0) {
        ld_paths_free_plugin_path_report(&plugin_report);
        return EXIT_FAILURE;
    }
    ld_paths_free_plugin_path_report(&plugin_report);
    if (plugin_report.sets != NULL) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
