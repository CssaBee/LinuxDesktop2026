#include "linuxdesktop/settings_c.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define LD_SETTINGS_PATH_SEPARATOR '\\'
#else
#include <sys/stat.h>
#define LD_SETTINGS_PATH_SEPARATOR '/'
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
    snprintf(buffer, size, "%s%c%s%c", temp_root(), LD_SETTINGS_PATH_SEPARATOR, "linuxdesktop2026-c-smoke", LD_SETTINGS_PATH_SEPARATOR);
    prefix_length = strlen(buffer);
    while (*leaf && prefix_length + 1 < size) {
        buffer[prefix_length++] = (*leaf == '/' || *leaf == '\\') ? LD_SETTINGS_PATH_SEPARATOR : *leaf;
        ++leaf;
    }
    buffer[prefix_length] = '\0';
}

static int create_directory_if_missing(const char* path)
{
#if defined(_WIN32)
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0777) == 0 || errno == EEXIST;
#endif
}

static int ensure_directory(const char* path)
{
    char buffer[1024];
    size_t i;
    size_t length;
    if (strlen(path) >= sizeof(buffer)) {
        return 0;
    }
    strcpy(buffer, path);
    length = strlen(buffer);
    for (i = 1; i < length; ++i) {
        if (buffer[i] == '/' || buffer[i] == '\\') {
            char saved = buffer[i];
            buffer[i] = '\0';
            if (strlen(buffer) > 0 && !(strlen(buffer) == 2 && buffer[1] == ':') && !create_directory_if_missing(buffer)) {
                return 0;
            }
            buffer[i] = saved;
        }
    }
    return create_directory_if_missing(buffer);
}

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
    char settings_override[512];
    char model_root[512];
    char hydrate_root[512];
    char old_root[512];
    char model_file[768];
    char old_file[768];
    char saved_file[768];

    make_path(settings_override, sizeof(settings_override), "settings");
    make_path(model_root, sizeof(model_root), "models");
    make_path(hydrate_root, sizeof(hydrate_root), "hydrated");
    make_path(old_root, sizeof(old_root), "old");
    make_path(model_file, sizeof(model_file), "models/config.model.xml");
    make_path(old_file, sizeof(old_file), "old/config.xml");
    make_path(saved_file, sizeof(saved_file), "hydrated/saved.xml");

    memset(&report, 0, sizeof(report));
    ld_settings_root_options_init(&options);

    options.organization = "LinuxDesktop2026";
    options.application = "c-smoke";
    options.settings_override = settings_override;
    options.portable_level = LD_SETTINGS_PORTABLE_PROFILE;

    if (ld_settings_version_major() != LD_SETTINGS_VERSION_MAJOR ||
        ld_settings_version_minor() != LD_SETTINGS_VERSION_MINOR ||
        ld_settings_version_patch() != LD_SETTINGS_VERSION_PATCH ||
        strcmp(ld_settings_version_string(), "0.1.0") != 0) {
        return EXIT_FAILURE;
    }

    if (!ld_settings_resolve_app_roots(&options, &report)) {
        return EXIT_FAILURE;
    }

    if (!report.config || strcmp(report.config, settings_override) != 0) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    if (report.settings_override_active != 1) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    if (report.portable_level != LD_SETTINGS_PORTABLE_PROFILE ||
        report.config_layer_count == 0 ||
        report.active_write_layer == NULL) {
        ld_settings_free_root_report(&report);
        return EXIT_FAILURE;
    }

    ld_settings_free_root_report(&report);
    if (report.config != NULL) {
        return EXIT_FAILURE;
    }

    if (!ensure_directory(model_root) || !ensure_directory(hydrate_root) || !ensure_directory(old_root)) {
        return EXIT_FAILURE;
    }
    if (!write_text(model_file, "<Config model=\"true\" />\n") ||
        !write_text(old_file, "<Config copied=\"true\" />\n") ||
        !write_text(saved_file, "<Config saved=\"old\" />\n")) {
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
    hydrate_options.model_root = model_root;
    hydrate_options.target_root = hydrate_root;
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
    write_options.target = saved_file;
    write_options.content = write_content;
    write_options.content_size = strlen(write_content);
    write_options.durable_write = 1;
    if (!ld_settings_write_with_backup(&write_options, validate_contains_saved, NULL, &write_report) ||
        write_report.ok != 1 ||
        write_report.durable_write != 1 ||
        write_report.backup_path == NULL ||
        write_report.temp_path == NULL) {
        ld_settings_free_write_report(&write_report);
        return EXIT_FAILURE;
    }
    ld_settings_free_write_report(&write_report);

    return EXIT_SUCCESS;
}
