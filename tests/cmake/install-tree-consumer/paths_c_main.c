#include "linuxdesktop/paths_c.h"

#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct ld_paths_resolver_options options;
    struct ld_paths_resolver_report report;
    memset(&report, 0, sizeof(report));
    ld_paths_resolver_options_init(&options);
    options.organization = "LinuxDesktop2026";
    options.application = "install-c-consumer";
    options.home_directory = "/tmp/linuxdesktop2026-install-c-consumer";
    options.use_process_environment = 0;

    if (!ld_paths_resolve_app_paths(&options, &report) ||
        report.selected_count == 0 ||
        strcmp(ld_paths_path_family_name(LD_PATHS_FAMILY_CONFIG), "config") != 0) {
        ld_paths_free_resolver_report(&report);
        return EXIT_FAILURE;
    }

    ld_paths_free_resolver_report(&report);
    return report.selected == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
