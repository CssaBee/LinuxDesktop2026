#include "linuxdesktop/paths_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_selected_path(
    const struct ld_paths_resolver_report* report,
    int family)
{
    size_t i;
    for (i = 0; i < report->selected_count; ++i) {
        if (report->selected[i].family == family) {
            printf("%s: %s\n", ld_paths_path_family_name(family), report->selected[i].path);
            return;
        }
    }
}

int main(int argc, char** argv)
{
    const char* application = argc > 1 ? argv[1] : "paths-c-demo";

    struct ld_paths_resolver_options options;
    struct ld_paths_resolver_report report;
    memset(&report, 0, sizeof(report));
    ld_paths_resolver_options_init(&options);
    options.organization = "LinuxDesktop2026";
    options.application = application;
    options.home_directory = "/tmp/linuxdesktop2026-paths-c-demo/home";
    options.temp_override = "/tmp/linuxdesktop2026-paths-c-demo/tmp";
    options.executable_path = "/opt/linuxdesktop2026/bin/paths-c-demo";
    options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&options, &report)) {
        fprintf(stderr, "ld_paths_resolve_app_paths failed\n");
        return EXIT_FAILURE;
    }

    printf("LinuxDesktop2026 ld_paths C demo\n");
    print_selected_path(&report, LD_PATHS_FAMILY_CONFIG);
    print_selected_path(&report, LD_PATHS_FAMILY_DATA);
    print_selected_path(&report, LD_PATHS_FAMILY_CACHE);
    print_selected_path(&report, LD_PATHS_FAMILY_RESOURCES);
    ld_paths_free_resolver_report(&report);

    int kinds[2] = {LD_PATHS_PLUGIN_LV2, LD_PATHS_PLUGIN_VST3};
    struct ld_paths_plugin_path_options plugin_options;
    struct ld_paths_plugin_path_report plugin_report;
    memset(&plugin_report, 0, sizeof(plugin_report));
    ld_paths_plugin_path_options_init(&plugin_options);
    plugin_options.kinds = kinds;
    plugin_options.kind_count = 2;
    plugin_options.home_directory = "/tmp/linuxdesktop2026-paths-c-demo/home";
    plugin_options.use_process_environment = 0;

    if (!ld_paths_resolve_plugin_path_sets(&plugin_options, &plugin_report)) {
        fprintf(stderr, "ld_paths_resolve_plugin_path_sets failed\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < plugin_report.set_count; ++i) {
        printf("%s paths: %zu\n", plugin_report.sets[i].name, plugin_report.sets[i].path_count);
    }
    ld_paths_free_plugin_path_report(&plugin_report);

    return EXIT_SUCCESS;
}
