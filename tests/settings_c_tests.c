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
    if (report.config != NULL) {
        return EXIT_FAILURE;
    }

    struct ld_settings_effect_options effect_options;
    ld_settings_effect_options_init(&effect_options);
    effect_options.allow_desktop_integration_write = 1;
    effect_options.autostart_directory_override = "/tmp/linuxdesktop2026-c-smoke-autostart";

    const char* arguments[] = {"--profile", "Default"};
    struct ld_settings_autostart_entry autostart;
    memset(&autostart, 0, sizeof(autostart));
    autostart.id = "linuxdesktop2026-c-smoke";
    autostart.display_name = "LinuxDesktop2026 C Smoke";
    autostart.executable = "/usr/bin/ld-settings-c-smoke";
    autostart.arguments = arguments;
    autostart.argument_count = 2;
    autostart.enabled = 1;
    autostart.user_scope = 1;

    struct ld_settings_effect_report effect_report;
    memset(&effect_report, 0, sizeof(effect_report));
    if (!ld_settings_apply_autostart(&autostart, &effect_options, &effect_report) ||
        effect_report.ok != 1 ||
        effect_report.dry_run != 1) {
        ld_settings_free_effect_report(&effect_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_effect_report(&effect_report);
    if (effect_report.path != NULL) {
        return EXIT_FAILURE;
    }

    struct ld_settings_policy_entry policy;
    memset(&policy, 0, sizeof(policy));
    policy.id = "linuxdesktop2026-c-policy";
    policy.schema_id = "org.linuxdesktop2026.c-smoke";
    policy.key = "theme";
    policy.value = "'dark'";
    policy.user_scope = 1;

    effect_options.allow_policy_write = 1;
    effect_options.policy_defaults_directory_override = "/tmp/linuxdesktop2026-c-smoke-policy/defaults";

    struct ld_settings_policy_report policy_report;
    memset(&policy_report, 0, sizeof(policy_report));
    if (!ld_settings_apply_policy(&policy, &effect_options, &policy_report) ||
        policy_report.ok != 1 ||
        policy_report.dry_run != 1 ||
        policy_report.present != 1 ||
        !policy_report.value ||
        strcmp(policy_report.value, "'dark'") != 0) {
        ld_settings_free_policy_report(&policy_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_policy_report(&policy_report);

    return policy_report.value == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
