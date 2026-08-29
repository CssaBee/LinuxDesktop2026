#include "linuxdesktop/desktop_c.h"

#include "linuxdesktop/desktop.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace desktop = linuxdesktop::desktop;

char* duplicate_string(const std::string& value)
{
    auto* result = static_cast<char*>(std::malloc(value.size() + 1));
    if (!result) {
        return nullptr;
    }
    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

std::string path_to_utf8_string(const std::filesystem::path& value)
{
    const auto text = value.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

char* duplicate_path(const std::filesystem::path& value)
{
    return duplicate_string(path_to_utf8_string(value));
}

void free_diagnostic(ld_desktop_diagnostic& diagnostic)
{
    std::free(diagnostic.code);
    std::free(diagnostic.message);
    std::free(diagnostic.path);
    diagnostic = {};
}

bool assign_string(char*& target, const std::string& value)
{
    target = duplicate_string(value);
    return target != nullptr;
}

bool assign_path(char*& target, const std::filesystem::path& value)
{
    target = duplicate_path(value);
    return target != nullptr;
}

bool fill_diagnostics(
    const std::vector<linuxdesktop::diagnostic>& source,
    ld_desktop_diagnostic*& diagnostics,
    size_t& count)
{
    diagnostics = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }

    diagnostics = static_cast<ld_desktop_diagnostic*>(std::calloc(source.size(), sizeof(ld_desktop_diagnostic)));
    if (!diagnostics) {
        return false;
    }
    count = source.size();

    for (size_t index = 0; index != source.size(); ++index) {
        const auto& diagnostic = source[index];
        diagnostics[index].severity = static_cast<int>(diagnostic.level);
        if (!assign_string(diagnostics[index].code, diagnostic.code) ||
            !assign_string(diagnostics[index].message, diagnostic.message) ||
            !assign_path(diagnostics[index].path, diagnostic.path)) {
            for (size_t free_index = 0; free_index <= index; ++free_index) {
                free_diagnostic(diagnostics[free_index]);
            }
            std::free(diagnostics);
            diagnostics = nullptr;
            count = 0;
            return false;
        }
    }
    return true;
}

void free_effect_report_fields(ld_desktop_effect_report& report)
{
    std::free(report.path);
    if (report.diagnostics) {
        for (size_t index = 0; index != report.diagnostic_count; ++index) {
            free_diagnostic(report.diagnostics[index]);
        }
        std::free(report.diagnostics);
    }
    report = {};
}

void free_policy_report_fields(ld_desktop_policy_report& report)
{
    std::free(report.path);
    std::free(report.value);
    if (report.diagnostics) {
        for (size_t index = 0; index != report.diagnostic_count; ++index) {
            free_diagnostic(report.diagnostics[index]);
        }
        std::free(report.diagnostics);
    }
    report = {};
}

desktop::apply_options effect_options_from_c(const ld_desktop_effect_options* source)
{
    desktop::apply_options options;
    if (!source) {
        return options;
    }
    options.dry_run = source->dry_run != 0;
    options.allow_global_write = source->allow_global_write != 0;
    options.allow_desktop_integration_write = source->allow_desktop_integration_write != 0;
    options.allow_policy_write = source->allow_policy_write != 0;
    if (source->autostart_directory_override) {
        options.autostart_directory_override = std::filesystem::path(source->autostart_directory_override);
    }
    if (source->policy_defaults_directory_override) {
        options.policy_defaults_directory_override = std::filesystem::path(source->policy_defaults_directory_override);
    }
    if (source->policy_locks_directory_override) {
        options.policy_locks_directory_override = std::filesystem::path(source->policy_locks_directory_override);
    }
    return options;
}

desktop::autostart_entry autostart_entry_from_c(const ld_desktop_autostart_entry* source)
{
    desktop::autostart_entry entry;
    if (!source) {
        return entry;
    }
    if (source->id) {
        entry.id = source->id;
    }
    if (source->display_name) {
        entry.display_name = source->display_name;
    }
    if (source->executable) {
        entry.executable = std::filesystem::path(source->executable);
    }
    if (source->arguments && source->argument_count > 0) {
        entry.arguments.reserve(source->argument_count);
        for (size_t index = 0; index != source->argument_count; ++index) {
            entry.arguments.emplace_back(source->arguments[index] ? source->arguments[index] : "");
        }
    }
    if (source->working_directory) {
        entry.working_directory = std::filesystem::path(source->working_directory);
    }
    entry.enabled = source->enabled != 0;
    entry.user_scope = source->user_scope != 0;
    return entry;
}

desktop::policy_entry policy_entry_from_c(const ld_desktop_policy_entry* source)
{
    desktop::policy_entry entry;
    if (!source) {
        return entry;
    }
    if (source->id) {
        entry.id = source->id;
    }
    if (source->schema_id) {
        entry.schema_id = source->schema_id;
    }
    if (source->group) {
        entry.group = source->group;
    }
    if (source->key) {
        entry.key = source->key;
    }
    if (source->value) {
        entry.value = source->value;
    }
    entry.enforced = source->enforced != 0;
    entry.user_scope = source->user_scope != 0;
    return entry;
}

bool fill_effect_report(const desktop::effect_report& source, ld_desktop_effect_report& target)
{
    target = {};
    target.ok = source.ok ? 1 : 0;
    target.dry_run = source.dry_run ? 1 : 0;
    target.enabled = source.enabled ? 1 : 0;
    if (source.path && !assign_path(target.path, *source.path)) {
        free_effect_report_fields(target);
        return false;
    }
    if (!fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        free_effect_report_fields(target);
        return false;
    }
    return true;
}

bool fill_policy_report(const desktop::policy_report& source, ld_desktop_policy_report& target)
{
    target = {};
    target.ok = source.ok ? 1 : 0;
    target.dry_run = source.dry_run ? 1 : 0;
    target.present = source.present ? 1 : 0;
    target.enforced = source.enforced ? 1 : 0;
    if (source.path && !assign_path(target.path, *source.path)) {
        free_policy_report_fields(target);
        return false;
    }
    if (source.value && !assign_string(target.value, *source.value)) {
        free_policy_report_fields(target);
        return false;
    }
    if (!fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        free_policy_report_fields(target);
        return false;
    }
    return true;
}

} // namespace

void ld_desktop_effect_options_init(ld_desktop_effect_options* options)
{
    if (!options) {
        return;
    }
    *options = {};
    options->dry_run = 1;
}

int ld_desktop_apply_autostart(
    const ld_desktop_autostart_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
                   desktop::apply_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_desktop_remove_autostart(
    const ld_desktop_autostart_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
                   desktop::remove_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_desktop_query_autostart(
    const ld_desktop_autostart_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
                   desktop::query_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

void ld_desktop_free_effect_report(ld_desktop_effect_report* report)
{
    if (!report) {
        return;
    }
    free_effect_report_fields(*report);
}

int ld_desktop_apply_policy(
    const ld_desktop_policy_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
                   desktop::apply_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_desktop_remove_policy(
    const ld_desktop_policy_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
                   desktop::remove_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_desktop_query_policy(
    const ld_desktop_policy_entry* entry,
    const ld_desktop_effect_options* options,
    ld_desktop_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
                   desktop::query_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
                   *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

void ld_desktop_free_policy_report(ld_desktop_policy_report* report)
{
    if (!report) {
        return;
    }
    free_policy_report_fields(*report);
}

int ld_desktop_version_major(void)
{
    return LD_DESKTOP_VERSION_MAJOR;
}

int ld_desktop_version_minor(void)
{
    return LD_DESKTOP_VERSION_MINOR;
}

int ld_desktop_version_patch(void)
{
    return LD_DESKTOP_VERSION_PATCH;
}

const char* ld_desktop_version_string(void)
{
    return "0.1.0";
}
