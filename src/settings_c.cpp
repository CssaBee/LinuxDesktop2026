#include "linuxdesktop/settings_c.h"

#include "linuxdesktop/settings.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <new>
#include <optional>
#include <string>

namespace {

namespace ld = linuxdesktop::settings;

char* duplicate_string(const std::string& value)
{
    auto* result = static_cast<char*>(std::malloc(value.size() + 1));
    if (!result) {
        return nullptr;
    }
    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

char* duplicate_path(const std::filesystem::path& value)
{
    return duplicate_string(value.u8string());
}

void free_diagnostic(ld_settings_diagnostic& diagnostic)
{
    std::free(diagnostic.code);
    std::free(diagnostic.message);
    std::free(diagnostic.path);
    diagnostic = {};
}

int severity_to_c(ld::severity value)
{
    switch (value) {
    case ld::severity::info:
        return LD_SETTINGS_SEVERITY_INFO;
    case ld::severity::warning:
        return LD_SETTINGS_SEVERITY_WARNING;
    case ld::severity::error:
        return LD_SETTINGS_SEVERITY_ERROR;
    }
    return LD_SETTINGS_SEVERITY_ERROR;
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

bool fill_report(const ld::root_report& source, ld_settings_root_report& target)
{
    target = {};
    target.portable_requested = source.portable_requested ? 1 : 0;
    target.portable_active = source.portable_active ? 1 : 0;
    target.settings_override_active = source.settings_override_active ? 1 : 0;
    target.sync_config_override_active = source.sync_config_override_active ? 1 : 0;

    if (!assign_path(target.resources, source.roots.resources) ||
        !assign_path(target.config, source.roots.config) ||
        !assign_path(target.data, source.roots.data) ||
        !assign_path(target.state, source.roots.state) ||
        !assign_path(target.cache, source.roots.cache) ||
        !assign_path(target.runtime, source.roots.runtime) ||
        !assign_path(target.session, source.roots.session) ||
        !assign_path(target.plugin_config, source.roots.plugin_config)) {
        ld_settings_free_root_report(&target);
        return false;
    }

    if (!source.diagnostics.empty()) {
        target.diagnostics = static_cast<ld_settings_diagnostic*>(
            std::calloc(source.diagnostics.size(), sizeof(ld_settings_diagnostic)));
        if (!target.diagnostics) {
            ld_settings_free_root_report(&target);
            return false;
        }
        target.diagnostic_count = source.diagnostics.size();

        for (size_t index = 0; index != source.diagnostics.size(); ++index) {
            const auto& diagnostic = source.diagnostics[index];
            target.diagnostics[index].severity = severity_to_c(diagnostic.level);
            if (!assign_string(target.diagnostics[index].code, diagnostic.code) ||
                !assign_string(target.diagnostics[index].message, diagnostic.message) ||
                !assign_path(target.diagnostics[index].path, diagnostic.path)) {
                ld_settings_free_root_report(&target);
                return false;
            }
        }
    }

    return true;
}

std::optional<std::filesystem::path> optional_path(const char* value)
{
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

} // namespace

extern "C" {

void ld_settings_root_options_init(ld_settings_root_options* options)
{
    if (!options) {
        return;
    }

    *options = {};
    options->allow_portable_root = 1;
    options->create_directories = 1;
}

int ld_settings_resolve_app_roots(const ld_settings_root_options* options, ld_settings_root_report* report)
{
    if (!options || !report || !options->organization || !options->application) {
        return 0;
    }

    try {
        ld::app_identity identity;
        identity.organization = options->organization;
        identity.application = options->application;

        ld::root_options root_options;
        root_options.resource_root = optional_path(options->resource_root);
        root_options.settings_override = optional_path(options->settings_override);
        root_options.sync_config_override = optional_path(options->sync_config_override);
        root_options.portable_marker = optional_path(options->portable_marker);
        root_options.allow_portable_root = options->allow_portable_root != 0;
        root_options.deny_portable_root_in_privileged_install =
            options->deny_portable_root_in_privileged_install != 0;
        root_options.allow_sync_config_for_portable_root =
            options->allow_sync_config_for_portable_root != 0;
        root_options.create_directories = options->create_directories != 0;

        for (size_t index = 0; index != options->privileged_install_root_count; ++index) {
            const char* path = options->privileged_install_roots ? options->privileged_install_roots[index] : nullptr;
            if (path && path[0]) {
                root_options.privileged_install_roots.emplace_back(path);
            }
        }

        return fill_report(ld::resolve_app_roots(identity, root_options), *report) ? 1 : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

void ld_settings_free_root_report(ld_settings_root_report* report)
{
    if (!report) {
        return;
    }

    std::free(report->resources);
    std::free(report->config);
    std::free(report->data);
    std::free(report->state);
    std::free(report->cache);
    std::free(report->runtime);
    std::free(report->session);
    std::free(report->plugin_config);

    if (report->diagnostics) {
        for (size_t index = 0; index != report->diagnostic_count; ++index) {
            free_diagnostic(report->diagnostics[index]);
        }
        std::free(report->diagnostics);
    }

    *report = {};
}

const char* ld_settings_severity_name(int severity)
{
    switch (severity) {
    case LD_SETTINGS_SEVERITY_INFO:
        return "info";
    case LD_SETTINGS_SEVERITY_WARNING:
        return "warning";
    case LD_SETTINGS_SEVERITY_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

int ld_settings_version_major(void)
{
    return LD_SETTINGS_VERSION_MAJOR;
}

int ld_settings_version_minor(void)
{
    return LD_SETTINGS_VERSION_MINOR;
}

int ld_settings_version_patch(void)
{
    return LD_SETTINGS_VERSION_PATCH;
}

const char* ld_settings_version_string(void)
{
    return "0.1.0";
}

} // extern "C"
