#include "linuxdesktop/desktop_c.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define LD_DESKTOP_PATH_SEPARATOR '\\'
#else
#include <sys/stat.h>
#define LD_DESKTOP_PATH_SEPARATOR '/'
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
    snprintf(buffer, size, "%s%c%s%c", temp_root(), LD_DESKTOP_PATH_SEPARATOR, "linuxdesktop2026-desktop-c-smoke", LD_DESKTOP_PATH_SEPARATOR);
    prefix_length = strlen(buffer);
    while (*leaf && prefix_length + 1 < size) {
        buffer[prefix_length++] = (*leaf == '/' || *leaf == '\\') ? LD_DESKTOP_PATH_SEPARATOR : *leaf;
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

static int has_diagnostic(const struct ld_desktop_diagnostic* diagnostics, size_t count, const char* code)
{
    size_t index;
    for (index = 0; index != count; ++index) {
        if (diagnostics[index].code && strcmp(diagnostics[index].code, code) == 0) {
            return 1;
        }
    }
    return 0;
}

static int read_text(const char* path, char* buffer, size_t size)
{
    FILE* file = fopen(path, "rb");
    size_t read_count;
    if (!file) {
        return 0;
    }
    read_count = fread(buffer, 1, size - 1, file);
    fclose(file);
    buffer[read_count] = '\0';
    return 1;
}

static struct ld_desktop_autostart_entry autostart_entry(void)
{
    struct ld_desktop_autostart_entry entry;
    static const char* arguments[] = {"--profile", "Default"};
    memset(&entry, 0, sizeof(entry));
    entry.id = "linuxdesktop2026-desktop-c-smoke";
    entry.display_name = "LinuxDesktop2026 Desktop C Smoke";
    entry.executable = "/usr/bin/ld-desktop-c-smoke";
    entry.arguments = arguments;
    entry.argument_count = 2;
    entry.enabled = 1;
    entry.user_scope = 1;
    return entry;
}

static struct ld_desktop_policy_entry policy_entry(void)
{
    struct ld_desktop_policy_entry entry;
    memset(&entry, 0, sizeof(entry));
    entry.id = "linuxdesktop2026-desktop-c-policy";
    entry.schema_id = "org.linuxdesktop2026.desktop-c-smoke";
    entry.key = "theme";
    entry.value = "'dark'";
    entry.user_scope = 1;
    return entry;
}

int main(void)
{
    char autostart_root[512];
    char policy_root[512];
    struct ld_desktop_effect_options options;

    make_path(autostart_root, sizeof(autostart_root), "autostart");
    make_path(policy_root, sizeof(policy_root), "policy/defaults");

    if (ld_desktop_version_major() != LD_DESKTOP_VERSION_MAJOR ||
        ld_desktop_version_minor() != LD_DESKTOP_VERSION_MINOR ||
        ld_desktop_version_patch() != LD_DESKTOP_VERSION_PATCH ||
        strcmp(ld_desktop_version_string(), "0.1.0") != 0) {
        return EXIT_FAILURE;
    }

    ld_desktop_effect_options_init(&options);
    options.allow_desktop_integration_write = 1;
    options.autostart_directory_override = autostart_root;

    {
        struct ld_desktop_autostart_entry entry = autostart_entry();
        struct ld_desktop_effect_report report;
        memset(&report, 0, sizeof(report));
        if (!ld_desktop_apply_autostart(&entry, &options, &report) ||
            report.ok != 1 ||
            report.dry_run != 1 ||
            report.path == NULL ||
            !has_diagnostic(report.diagnostics, report.diagnostic_count, "autostart-dry-run")) {
            ld_desktop_free_effect_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_effect_report(&report);
    }

#if !defined(_WIN32)
    {
        char content[1024];
        struct ld_desktop_autostart_entry entry = autostart_entry();
        struct ld_desktop_effect_report report;
        ld_desktop_effect_options_init(&options);
        options.dry_run = 0;
        options.allow_desktop_integration_write = 1;
        options.autostart_directory_override = autostart_root;
        memset(&report, 0, sizeof(report));

        if (!ld_desktop_apply_autostart(&entry, &options, &report) ||
            report.ok != 1 ||
            report.path == NULL ||
            !read_text(report.path, content, sizeof(content)) ||
            strstr(content, "[Desktop Entry]") == NULL ||
            strstr(content, "Name=LinuxDesktop2026 Desktop C Smoke") == NULL) {
            ld_desktop_free_effect_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_effect_report(&report);

        memset(&report, 0, sizeof(report));
        if (!ld_desktop_query_autostart(&entry, &options, &report) ||
            report.ok != 1 ||
            report.enabled != 1) {
            ld_desktop_free_effect_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_effect_report(&report);

        memset(&report, 0, sizeof(report));
        if (!ld_desktop_remove_autostart(&entry, &options, &report) ||
            report.ok != 1) {
            ld_desktop_free_effect_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_effect_report(&report);
    }

    {
        struct ld_desktop_policy_entry entry = policy_entry();
        struct ld_desktop_policy_report report;
        ld_desktop_effect_options_init(&options);
        options.dry_run = 0;
        options.allow_policy_write = 1;
        options.policy_defaults_directory_override = policy_root;

        if (!ensure_directory(policy_root)) {
            return EXIT_FAILURE;
        }

        memset(&report, 0, sizeof(report));
        if (!ld_desktop_apply_policy(&entry, &options, &report) ||
            report.ok != 1 ||
            report.path == NULL) {
            ld_desktop_free_policy_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_policy_report(&report);

        memset(&report, 0, sizeof(report));
        if (!ld_desktop_query_policy(&entry, &options, &report) ||
            report.ok != 1 ||
            report.present != 1) {
            ld_desktop_free_policy_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_policy_report(&report);

        memset(&report, 0, sizeof(report));
        if (!ld_desktop_remove_policy(&entry, &options, &report) ||
            report.ok != 1) {
            ld_desktop_free_policy_report(&report);
            return EXIT_FAILURE;
        }
        ld_desktop_free_policy_report(&report);
    }
#endif

    return EXIT_SUCCESS;
}
