#include "linuxdesktop/root_c.h"

#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct ld_root_options options;
    struct ld_root_report report;
    struct ld_root_named_root_request named;

    ld_root_options_init(&options);
    memset(&report, 0, sizeof(report));
    memset(&named, 0, sizeof(named));

    options.organization = "LinuxDesktop2026";
    options.application = "c-root-tests";
    options.app_root_override = "/tmp/linuxdesktop2026-c-root-tests";
    named.name = "logs";
    named.purpose = LD_ROOT_PURPOSE_LOGS;
    named.ownership = LD_ROOT_OWNERSHIP_USER_LOCAL;
    named.relative_path = "Logs";
    named.create = 1;
    options.named_roots = &named;
    options.named_root_count = 1;

    if (!ld_root_resolve_app_roots(&options, &report)) {
        return EXIT_FAILURE;
    }
    if (!report.config || strcmp(report.config, options.app_root_override) != 0 ||
        report.app_root_override_active != 1 ||
        report.named_root_count != 1 ||
        !report.named_roots ||
        strcmp(report.named_roots[0].name, "logs") != 0 ||
        !report.named_roots[0].path) {
        ld_root_free_report(&report);
        return EXIT_FAILURE;
    }
    if (strcmp(ld_root_ownership_name(LD_ROOT_OWNERSHIP_USER_LOCAL), "user_local") != 0 ||
        strcmp(ld_root_purpose_name(LD_ROOT_PURPOSE_LOGS), "logs") != 0) {
        ld_root_free_report(&report);
        return EXIT_FAILURE;
    }
    ld_root_free_report(&report);
    return report.config == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
