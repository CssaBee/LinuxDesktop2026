#include "linuxdesktop/settings_c.h"

#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct ld_settings_root_options options;
    struct ld_settings_root_report report;

    memset(&report, 0, sizeof(report));
    ld_settings_root_options_init(&options);

    options.organization = "LinuxDesktop2026";
    options.application = "c-smoke";
    options.settings_override = "/tmp/linuxdesktop2026-c-smoke";

    if (!ld_settings_resolve_app_roots(&options, &report)) {
        return EXIT_FAILURE;
    }

    if (!report.config || strcmp(report.config, "/tmp/linuxdesktop2026-c-smoke") != 0) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    if (report.settings_override_active != 1) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    ld_settings_free_root_report(&report);
    return report.config == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
