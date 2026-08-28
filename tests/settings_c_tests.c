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
    options.portable_level = LD_SETTINGS_PORTABLE_PROFILE;

    struct ld_settings_named_root_request roots[1];
    memset(roots, 0, sizeof(roots));
    roots[0].name = "logs";
    roots[0].purpose = LD_SETTINGS_ROOT_PURPOSE_LOGS;
    roots[0].persistence = LD_SETTINGS_PERSISTENCE_MACHINE_LOCAL;
    roots[0].relative_path = "Logs";
    roots[0].create = 1;
    options.named_roots = roots;
    options.named_root_count = 1;

    if (ld_settings_version_major() != LD_SETTINGS_VERSION_MAJOR ||
        ld_settings_version_minor() != LD_SETTINGS_VERSION_MINOR ||
        ld_settings_version_patch() != LD_SETTINGS_VERSION_PATCH ||
        strcmp(ld_settings_version_string(), "0.1.0") != 0) {
        return EXIT_FAILURE;
    }

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

    if (report.portable_level != LD_SETTINGS_PORTABLE_PROFILE ||
        report.named_root_count != 1 ||
        !report.named_roots ||
        strcmp(report.named_roots[0].name, "logs") != 0 ||
        !report.named_roots[0].path ||
        report.config_layer_count == 0 ||
        report.active_write_layer == NULL) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    ld_settings_free_root_report(&report);
    return report.config == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
