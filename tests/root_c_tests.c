#include "linuxdesktop/root_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* test_temp_dir(void)
{
    const char* value = getenv("TMPDIR");
    if (value && value[0]) {
        return value;
    }
    value = getenv("TEMP");
    if (value && value[0]) {
        return value;
    }
    value = getenv("TMP");
    if (value && value[0]) {
        return value;
    }
#if defined(_WIN32)
    return "C:\\Windows\\Temp";
#else
    return "/tmp";
#endif
}

static void join_path(char* output, size_t output_size, const char* base, const char* leaf)
{
    const size_t base_length = strlen(base);
    const char separator =
        base_length > 0 && (base[base_length - 1] == '/' || base[base_length - 1] == '\\') ? '\0' :
#if defined(_WIN32)
            '\\';
#else
            '/';
#endif

    if (separator) {
        snprintf(output, output_size, "%s%c%s", base, separator, leaf);
    } else {
        snprintf(output, output_size, "%s%s", base, leaf);
    }
}

int main(void)
{
    struct ld_root_options options;
    struct ld_root_report report;
    struct ld_root_named_root_request named;
    struct ld_root_named_root_request component_roots[2];
    struct ld_root_component_root_request component;
    char app_root_override[1024];

    ld_root_options_init(&options);
    memset(&report, 0, sizeof(report));
    memset(&named, 0, sizeof(named));
    memset(component_roots, 0, sizeof(component_roots));
    memset(&component, 0, sizeof(component));
    join_path(app_root_override, sizeof(app_root_override), test_temp_dir(), "linuxdesktop2026-c-root-tests");

    options.organization = "LinuxDesktop2026";
    options.application = "c-root-tests";
    options.app_root_override = app_root_override;
    named.name = "logs";
    named.purpose = LD_ROOT_PURPOSE_LOGS;
    named.ownership = LD_ROOT_OWNERSHIP_USER_LOCAL;
    named.relative_path = "Logs";
    named.create = 1;
    options.named_roots = &named;
    options.named_root_count = 1;
    component_roots[0].name = "config";
    component_roots[0].purpose = LD_ROOT_PURPOSE_COMPONENT_CONFIG;
    component_roots[0].ownership = LD_ROOT_OWNERSHIP_USER_ROAMING;
    component_roots[0].relative_path = "Config";
    component_roots[0].create = 1;
    component_roots[1].name = "state";
    component_roots[1].purpose = LD_ROOT_PURPOSE_COMPONENT_STATE;
    component_roots[1].ownership = LD_ROOT_OWNERSHIP_USER_LOCAL;
    component_roots[1].relative_path = "State";
    component_roots[1].create = 1;
    component.name = "spellcheck";
    component.kind = LD_ROOT_COMPONENT_PLUGIN;
    component.roots = component_roots;
    component.root_count = 2;
    options.component_roots = &component;
    options.component_root_count = 1;

    if (!ld_root_resolve_app_roots(&options, &report)) {
        return EXIT_FAILURE;
    }
    if (!report.config || strcmp(report.config, options.app_root_override) != 0 ||
        report.app_root_override_active != 1 ||
        report.named_root_count != 1 ||
        !report.named_roots ||
        strcmp(report.named_roots[0].name, "logs") != 0 ||
        !report.named_roots[0].path ||
        report.component_root_count != 1 ||
        !report.component_roots ||
        strcmp(report.component_roots[0].name, "spellcheck") != 0 ||
        report.component_roots[0].kind != LD_ROOT_COMPONENT_PLUGIN ||
        report.component_roots[0].root_count != 2 ||
        !report.component_roots[0].roots ||
        strcmp(report.component_roots[0].roots[0].name, "config") != 0 ||
        report.component_roots[0].roots[0].purpose != LD_ROOT_PURPOSE_COMPONENT_CONFIG ||
        strcmp(report.component_roots[0].roots[1].name, "state") != 0 ||
        report.component_roots[0].roots[1].ownership != LD_ROOT_OWNERSHIP_USER_LOCAL) {
        ld_root_free_report(&report);
        return EXIT_FAILURE;
    }
    if (strcmp(ld_root_ownership_name(LD_ROOT_OWNERSHIP_USER_LOCAL), "user_local") != 0 ||
        strcmp(ld_root_purpose_name(LD_ROOT_PURPOSE_LOGS), "logs") != 0 ||
        strcmp(ld_root_component_kind_name(LD_ROOT_COMPONENT_PLUGIN), "plugin") != 0) {
        ld_root_free_report(&report);
        return EXIT_FAILURE;
    }
    ld_root_free_report(&report);
    return report.config == NULL &&
            report.named_roots == NULL &&
            report.named_root_count == 0 &&
            report.component_roots == NULL &&
            report.component_root_count == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
