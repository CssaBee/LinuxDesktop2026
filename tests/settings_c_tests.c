#include "linuxdesktop/settings_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int validate_contains_saved(const char* path, char* message, size_t message_size, void* user_data)
{
    (void)user_data;
    FILE* file = fopen(path, "rb");
    char buffer[128];
    size_t read_count;
    if (!file) {
        snprintf(message, message_size, "cannot open validation target");
        return 0;
    }
    read_count = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[read_count] = '\0';
    if (strstr(buffer, "saved") == NULL) {
        snprintf(message, message_size, "missing saved marker");
        return 0;
    }
    return 1;
}

static int write_text(const char* path, const char* content)
{
    FILE* file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    if (fwrite(content, 1, strlen(content), file) != strlen(content)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

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

    if (policy_report.value != NULL) {
        return EXIT_FAILURE;
    }

    if (system("mkdir -p /tmp/linuxdesktop2026-c-smoke-models /tmp/linuxdesktop2026-c-smoke-hydrated /tmp/linuxdesktop2026-c-smoke-old") != 0) {
        return EXIT_FAILURE;
    }
    if (!write_text("/tmp/linuxdesktop2026-c-smoke-models/config.model.xml", "<Config model=\"true\" />\n") ||
        !write_text("/tmp/linuxdesktop2026-c-smoke-old/config.xml", "<Config copied=\"true\" />\n") ||
        !write_text("/tmp/linuxdesktop2026-c-smoke-hydrated/saved.xml", "<Config saved=\"old\" />\n")) {
        return EXIT_FAILURE;
    }

    struct ld_settings_config_file files[1];
    memset(files, 0, sizeof(files));
    files[0].name = "config.xml";
    files[0].model_name = "config.model.xml";
    files[0].required = 1;

    struct ld_settings_hydrate_options hydrate_options;
    struct ld_settings_hydrate_report hydrate_report;
    memset(&hydrate_report, 0, sizeof(hydrate_report));
    ld_settings_hydrate_options_init(&hydrate_options);
    hydrate_options.model_root = "/tmp/linuxdesktop2026-c-smoke-models";
    hydrate_options.target_root = "/tmp/linuxdesktop2026-c-smoke-hydrated";
    hydrate_options.files = files;
    hydrate_options.file_count = 1;
    if (!ld_settings_hydrate_config_bundle(&hydrate_options, &hydrate_report) ||
        (hydrate_report.copied_count != 1 && hydrate_report.skipped_existing_count != 1)) {
        ld_settings_free_hydrate_report(&hydrate_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_hydrate_report(&hydrate_report);

    struct ld_settings_write_options write_options;
    struct ld_settings_write_report write_report;
    const char* write_content = "<Config saved=\"new\" />\n";
    memset(&write_report, 0, sizeof(write_report));
    ld_settings_write_options_init(&write_options);
    write_options.target = "/tmp/linuxdesktop2026-c-smoke-hydrated/saved.xml";
    write_options.content = write_content;
    write_options.content_size = strlen(write_content);
    if (!ld_settings_write_with_backup(&write_options, validate_contains_saved, NULL, &write_report) ||
        write_report.ok != 1 ||
        write_report.backup_path == NULL ||
        write_report.temp_path == NULL) {
        ld_settings_free_write_report(&write_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_write_report(&write_report);

    struct ld_settings_migration_action actions[1];
    struct ld_settings_migration_options migration_options;
    struct ld_settings_migration_report migration_report;
    memset(actions, 0, sizeof(actions));
    actions[0].kind = LD_SETTINGS_MIGRATION_COPY_FILE;
    actions[0].name = "copy legacy config";
    actions[0].source_path = "/tmp/linuxdesktop2026-c-smoke-old/config.xml";
    actions[0].target_path = "/tmp/linuxdesktop2026-c-smoke-hydrated/migrated.xml";
    ld_settings_migration_options_init(&migration_options);
    migration_options.overwrite_existing = 1;
    memset(&migration_report, 0, sizeof(migration_report));
    if (!ld_settings_plan_migration(actions, 1, &migration_options, &migration_report) ||
        migration_report.dry_run != 1 ||
        migration_report.action_count != 1 ||
        migration_report.actions == NULL ||
        strcmp(migration_report.actions[0].name, "copy legacy config") != 0) {
        ld_settings_free_migration_report(&migration_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_migration_report(&migration_report);

    migration_options.dry_run = 0;
    memset(&migration_report, 0, sizeof(migration_report));
    if (!ld_settings_execute_migration_plan(actions, 1, &migration_options, &migration_options, &migration_report) ||
        migration_report.ok != 1 ||
        migration_report.dry_run != 0 ||
        migration_report.result_count != 1 ||
        migration_report.results == NULL ||
        migration_report.results[0].executed != 1) {
        ld_settings_free_migration_report(&migration_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_migration_report(&migration_report);

    unsigned char value_bytes[] = {'A', 'l', 'i', 'c', 'e'};
    struct ld_settings_registry_key registry_key;
    struct ld_settings_registry_value registry_values[1];
    struct ld_settings_registry_format_report format_report;
    memset(&registry_key, 0, sizeof(registry_key));
    registry_key.hive = LD_SETTINGS_REGISTRY_CURRENT_USER;
    registry_key.subkey = "Software\\LinuxDesktop2026\\c-smoke";
    registry_key.view = LD_SETTINGS_REGISTRY_VIEW_NATIVE;
    memset(registry_values, 0, sizeof(registry_values));
    registry_values[0].key_path = "Profiles";
    registry_values[0].name = "Name";
    registry_values[0].type = LD_SETTINGS_REGISTRY_VALUE_STRING;
    registry_values[0].bytes = value_bytes;
    registry_values[0].byte_count = sizeof(value_bytes);

    memset(&format_report, 0, sizeof(format_report));
    if (!ld_settings_registry_serialize_json(&registry_key, registry_values, 1, &format_report) ||
        format_report.ok != 1 ||
        format_report.content == NULL ||
        strstr(format_report.content, "linuxdesktop.settings.registry.snapshot.v1") == NULL) {
        ld_settings_free_registry_format_report(&format_report);
        return EXIT_FAILURE;
    }

    struct ld_settings_registry_format_report parsed_json;
    memset(&parsed_json, 0, sizeof(parsed_json));
    if (!ld_settings_registry_parse_json(format_report.content, &parsed_json) ||
        parsed_json.ok != 1 ||
        parsed_json.content == NULL ||
        strstr(parsed_json.content, "Profiles") == NULL) {
        ld_settings_free_registry_format_report(&format_report);
        ld_settings_free_registry_format_report(&parsed_json);
        return EXIT_FAILURE;
    }
    ld_settings_free_registry_format_report(&format_report);
    ld_settings_free_registry_format_report(&parsed_json);

    memset(&format_report, 0, sizeof(format_report));
    if (!ld_settings_registry_serialize_reg(&registry_key, registry_values, 1, &format_report) ||
        format_report.ok != 1 ||
        format_report.content == NULL ||
        strstr(format_report.content, "Windows Registry Editor Version 5.00") == NULL) {
        ld_settings_free_registry_format_report(&format_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_registry_format_report(&format_report);

    struct ld_settings_registry_options registry_options;
    struct ld_settings_registry_operation_report operation_report;
    ld_settings_registry_options_init(&registry_options);
    memset(&operation_report, 0, sizeof(operation_report));
    if (!ld_settings_registry_import_tree_json(&registry_key,
            "{ \"root\": { \"hive\": \"current_user\", \"subkey\": \"Software\\\\LinuxDesktop2026\\\\c-smoke\", \"view\": \"native\" }, \"values\": [] }",
            &registry_options,
            &operation_report) ||
        operation_report.ok != 0 ||
        operation_report.diagnostic_count == 0) {
        ld_settings_free_registry_operation_report(&operation_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_registry_operation_report(&operation_report);

    return EXIT_SUCCESS;
}
