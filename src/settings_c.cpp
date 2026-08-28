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

ld::portable_level portable_level_from_c(int value)
{
    switch (value) {
    case LD_SETTINGS_PORTABLE_OFF:
        return ld::portable_level::off;
    case LD_SETTINGS_PORTABLE_PROFILE:
        return ld::portable_level::profile;
    case LD_SETTINGS_PORTABLE_CLEAN:
        return ld::portable_level::clean;
    case LD_SETTINGS_PORTABLE_SETTINGS_ONLY:
    default:
        return ld::portable_level::settings_only;
    }
}

int portable_level_to_c(ld::portable_level value)
{
    switch (value) {
    case ld::portable_level::off:
        return LD_SETTINGS_PORTABLE_OFF;
    case ld::portable_level::profile:
        return LD_SETTINGS_PORTABLE_PROFILE;
    case ld::portable_level::clean:
        return LD_SETTINGS_PORTABLE_CLEAN;
    case ld::portable_level::settings_only:
        return LD_SETTINGS_PORTABLE_SETTINGS_ONLY;
    }
    return LD_SETTINGS_PORTABLE_SETTINGS_ONLY;
}

ld::root_purpose root_purpose_from_c(int value)
{
    if (value < LD_SETTINGS_ROOT_PURPOSE_RESOURCES || value > LD_SETTINGS_ROOT_PURPOSE_CUSTOM) {
        return ld::root_purpose::custom;
    }
    return static_cast<ld::root_purpose>(value);
}

int root_purpose_to_c(ld::root_purpose value)
{
    return static_cast<int>(value);
}

ld::persistence_class persistence_from_c(int value)
{
    if (value < LD_SETTINGS_PERSISTENCE_ROAMING || value > LD_SETTINGS_PERSISTENCE_ENFORCED) {
        return ld::persistence_class::roaming;
    }
    return static_cast<ld::persistence_class>(value);
}

int persistence_to_c(ld::persistence_class value)
{
    return static_cast<int>(value);
}

ld::component_kind component_kind_from_c(int value)
{
    if (value < LD_SETTINGS_COMPONENT_PLUGIN || value > LD_SETTINGS_COMPONENT_CUSTOM) {
        return ld::component_kind::custom;
    }
    return static_cast<ld::component_kind>(value);
}

int component_kind_to_c(ld::component_kind value)
{
    return static_cast<int>(value);
}

int config_layer_kind_to_c(ld::config_layer_kind value)
{
    return static_cast<int>(value);
}

int storage_backend_to_c(ld::storage_backend value)
{
    return static_cast<int>(value);
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

bool fill_diagnostics(const std::vector<ld::diagnostic>& source, ld_settings_diagnostic*& diagnostics, size_t& count);

void free_named_root(ld_settings_named_root& root)
{
    std::free(root.name);
    std::free(root.path);
    if (root.diagnostics) {
        for (size_t index = 0; index != root.diagnostic_count; ++index) {
            free_diagnostic(root.diagnostics[index]);
        }
        std::free(root.diagnostics);
    }
    root = {};
}

void free_component_roots(ld_settings_component_roots& component)
{
    std::free(component.name);
    if (component.roots) {
        for (size_t index = 0; index != component.root_count; ++index) {
            free_named_root(component.roots[index]);
        }
        std::free(component.roots);
    }
    if (component.diagnostics) {
        for (size_t index = 0; index != component.diagnostic_count; ++index) {
            free_diagnostic(component.diagnostics[index]);
        }
        std::free(component.diagnostics);
    }
    component = {};
}

void free_config_layer(ld_settings_config_layer& layer)
{
    std::free(layer.name);
    std::free(layer.path);
    layer = {};
}

void free_effect_report_fields(ld_settings_effect_report& report)
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

void free_policy_report_fields(ld_settings_policy_report& report)
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

bool fill_diagnostics(const std::vector<ld::diagnostic>& source, ld_settings_diagnostic*& diagnostics, size_t& count)
{
    diagnostics = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }

    diagnostics = static_cast<ld_settings_diagnostic*>(
        std::calloc(source.size(), sizeof(ld_settings_diagnostic)));
    if (!diagnostics) {
        return false;
    }
    count = source.size();

    for (size_t index = 0; index != source.size(); ++index) {
        const auto& diagnostic = source[index];
        diagnostics[index].severity = severity_to_c(diagnostic.level);
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

bool fill_named_root(const ld::named_root& source, ld_settings_named_root& target)
{
    target = {};
    target.purpose = root_purpose_to_c(source.purpose);
    target.persistence = persistence_to_c(source.persistence);
    target.created = source.created ? 1 : 0;
    if (!assign_string(target.name, source.name) || !assign_path(target.path, source.path) ||
        !fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        free_named_root(target);
        return false;
    }
    return true;
}

bool fill_named_roots(const std::vector<ld::named_root>& source, ld_settings_named_root*& roots, size_t& count)
{
    roots = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }
    roots = static_cast<ld_settings_named_root*>(std::calloc(source.size(), sizeof(ld_settings_named_root)));
    if (!roots) {
        return false;
    }
    count = source.size();
    for (size_t index = 0; index != source.size(); ++index) {
        if (!fill_named_root(source[index], roots[index])) {
            for (size_t free_index = 0; free_index <= index; ++free_index) {
                free_named_root(roots[free_index]);
            }
            std::free(roots);
            roots = nullptr;
            count = 0;
            return false;
        }
    }
    return true;
}

bool fill_config_layer(const ld::config_layer& source, ld_settings_config_layer& target)
{
    target = {};
    target.kind = config_layer_kind_to_c(source.kind);
    target.backend = storage_backend_to_c(source.backend);
    target.writable = source.writable ? 1 : 0;
    target.required = source.required ? 1 : 0;
    target.enforced = source.enforced ? 1 : 0;
    target.precedence = source.precedence;
    if (!assign_string(target.name, source.name) || !assign_path(target.path, source.path)) {
        free_config_layer(target);
        return false;
    }
    return true;
}

bool fill_config_layers(const std::vector<ld::config_layer>& source, ld_settings_config_layer*& layers, size_t& count)
{
    layers = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }
    layers = static_cast<ld_settings_config_layer*>(std::calloc(source.size(), sizeof(ld_settings_config_layer)));
    if (!layers) {
        return false;
    }
    count = source.size();
    for (size_t index = 0; index != source.size(); ++index) {
        if (!fill_config_layer(source[index], layers[index])) {
            for (size_t free_index = 0; free_index <= index; ++free_index) {
                free_config_layer(layers[free_index]);
            }
            std::free(layers);
            layers = nullptr;
            count = 0;
            return false;
        }
    }
    return true;
}

bool fill_report(const ld::root_report& source, ld_settings_root_report& target)
{
    target = {};
    target.portable_requested = source.portable_requested ? 1 : 0;
    target.portable_active = source.portable_active ? 1 : 0;
    target.settings_override_active = source.settings_override_active ? 1 : 0;
    target.sync_config_override_active = source.sync_config_override_active ? 1 : 0;
    target.portable_level = portable_level_to_c(source.portable);

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

    if (!fill_named_roots(source.named_roots, target.named_roots, target.named_root_count) ||
        !fill_config_layers(source.layers.candidates, target.config_layers, target.config_layer_count) ||
        !fill_config_layers(source.layers.active_read_order, target.active_read_order, target.active_read_order_count) ||
        !fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        ld_settings_free_root_report(&target);
        return false;
    }

    if (source.layers.active_write_layer) {
        target.active_write_layer = static_cast<ld_settings_config_layer*>(
            std::calloc(1, sizeof(ld_settings_config_layer)));
        if (!target.active_write_layer || !fill_config_layer(*source.layers.active_write_layer, *target.active_write_layer)) {
            ld_settings_free_root_report(&target);
            return false;
        }
    }

    if (!source.component_roots.empty()) {
        target.component_roots = static_cast<ld_settings_component_roots*>(
            std::calloc(source.component_roots.size(), sizeof(ld_settings_component_roots)));
        if (!target.component_roots) {
            ld_settings_free_root_report(&target);
            return false;
        }
        target.component_root_count = source.component_roots.size();
        for (size_t index = 0; index != source.component_roots.size(); ++index) {
            const auto& component = source.component_roots[index];
            auto& target_component = target.component_roots[index];
            target_component.kind = component_kind_to_c(component.kind);
            if (!assign_string(target_component.name, component.name) ||
                !fill_named_roots(component.roots, target_component.roots, target_component.root_count) ||
                !fill_diagnostics(component.diagnostics, target_component.diagnostics, target_component.diagnostic_count)) {
                ld_settings_free_root_report(&target);
                return false;
            }
        }
    }

    return true;
}

bool fill_effect_report(const ld::effects::effect_report& source, ld_settings_effect_report& target)
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

bool fill_policy_report(const ld::effects::policy_report& source, ld_settings_policy_report& target)
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

std::optional<std::filesystem::path> optional_path(const char* value)
{
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

ld::effects::apply_options effect_options_from_c(const ld_settings_effect_options* source)
{
    ld::effects::apply_options options;
    if (!source) {
        return options;
    }
    options.dry_run = source->dry_run != 0;
    options.allow_global_write = source->allow_global_write != 0;
    options.allow_desktop_integration_write = source->allow_desktop_integration_write != 0;
    options.allow_policy_write = source->allow_policy_write != 0;
    options.autostart_directory_override = optional_path(source->autostart_directory_override);
    options.policy_defaults_directory_override = optional_path(source->policy_defaults_directory_override);
    options.policy_locks_directory_override = optional_path(source->policy_locks_directory_override);
    return options;
}

ld::effects::autostart_entry autostart_entry_from_c(const ld_settings_autostart_entry* source)
{
    ld::effects::autostart_entry entry;
    if (!source) {
        return entry;
    }
    entry.id = source->id ? source->id : "";
    entry.display_name = source->display_name ? source->display_name : "";
    entry.executable = optional_path(source->executable).value_or(std::filesystem::path{});
    for (size_t index = 0; index != source->argument_count; ++index) {
        const char* argument = source->arguments ? source->arguments[index] : nullptr;
        entry.arguments.emplace_back(argument ? argument : "");
    }
    entry.working_directory = optional_path(source->working_directory).value_or(std::filesystem::path{});
    entry.enabled = source->enabled != 0;
    entry.user_scope = source->user_scope != 0;
    return entry;
}

ld::effects::policy_entry policy_entry_from_c(const ld_settings_policy_entry* source)
{
    ld::effects::policy_entry entry;
    if (!source) {
        return entry;
    }
    entry.id = source->id ? source->id : "";
    entry.schema_id = source->schema_id ? source->schema_id : "";
    entry.group = source->group ? source->group : "";
    entry.key = source->key ? source->key : "";
    entry.value = source->value ? source->value : "";
    entry.enforced = source->enforced != 0;
    entry.user_scope = source->user_scope != 0;
    return entry;
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
    options->portable_level = LD_SETTINGS_PORTABLE_SETTINGS_ONLY;
}

void ld_settings_effect_options_init(ld_settings_effect_options* options)
{
    if (!options) {
        return;
    }

    *options = {};
    options->dry_run = 1;
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
        root_options.portable = portable_level_from_c(options->portable_level);

        for (size_t index = 0; index != options->privileged_install_root_count; ++index) {
            const char* path = options->privileged_install_roots ? options->privileged_install_roots[index] : nullptr;
            if (path && path[0]) {
                root_options.privileged_install_roots.emplace_back(path);
            }
        }

        for (size_t index = 0; index != options->named_root_count; ++index) {
            const auto& source = options->named_roots[index];
            ld::named_root_request request;
            request.name = source.name ? source.name : "";
            request.purpose = root_purpose_from_c(source.purpose);
            request.persistence = persistence_from_c(source.persistence);
            request.relative_path = optional_path(source.relative_path).value_or(std::filesystem::path{});
            request.create = source.create != 0;
            root_options.named_roots.push_back(std::move(request));
        }

        for (size_t index = 0; index != options->component_root_count; ++index) {
            const auto& source = options->component_roots[index];
            ld::component_root_request component;
            component.name = source.name ? source.name : "";
            component.kind = component_kind_from_c(source.kind);
            for (size_t root_index = 0; root_index != source.root_count; ++root_index) {
                const auto& root_source = source.roots[root_index];
                ld::named_root_request request;
                request.name = root_source.name ? root_source.name : "";
                request.purpose = root_purpose_from_c(root_source.purpose);
                request.persistence = persistence_from_c(root_source.persistence);
                request.relative_path = optional_path(root_source.relative_path).value_or(std::filesystem::path{});
                request.create = root_source.create != 0;
                component.roots.push_back(std::move(request));
            }
            root_options.component_roots.push_back(std::move(component));
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

    if (report->named_roots) {
        for (size_t index = 0; index != report->named_root_count; ++index) {
            free_named_root(report->named_roots[index]);
        }
        std::free(report->named_roots);
    }

    if (report->component_roots) {
        for (size_t index = 0; index != report->component_root_count; ++index) {
            free_component_roots(report->component_roots[index]);
        }
        std::free(report->component_roots);
    }

    if (report->config_layers) {
        for (size_t index = 0; index != report->config_layer_count; ++index) {
            free_config_layer(report->config_layers[index]);
        }
        std::free(report->config_layers);
    }

    if (report->active_read_order) {
        for (size_t index = 0; index != report->active_read_order_count; ++index) {
            free_config_layer(report->active_read_order[index]);
        }
        std::free(report->active_read_order);
    }

    if (report->active_write_layer) {
        free_config_layer(*report->active_write_layer);
        std::free(report->active_write_layer);
    }

    if (report->diagnostics) {
        for (size_t index = 0; index != report->diagnostic_count; ++index) {
            free_diagnostic(report->diagnostics[index]);
        }
        std::free(report->diagnostics);
    }

    *report = {};
}

int ld_settings_apply_autostart(
    const ld_settings_autostart_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
            ld::effects::apply_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_settings_remove_autostart(
    const ld_settings_autostart_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
            ld::effects::remove_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_settings_query_autostart(
    const ld_settings_autostart_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_effect_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_effect_report(
            ld::effects::query_autostart(autostart_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

void ld_settings_free_effect_report(ld_settings_effect_report* report)
{
    if (!report) {
        return;
    }
    free_effect_report_fields(*report);
}

int ld_settings_apply_policy(
    const ld_settings_policy_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
            ld::effects::apply_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_settings_remove_policy(
    const ld_settings_policy_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
            ld::effects::remove_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

int ld_settings_query_policy(
    const ld_settings_policy_entry* entry,
    const ld_settings_effect_options* options,
    ld_settings_policy_report* report)
{
    if (!entry || !report) {
        return 0;
    }
    try {
        return fill_policy_report(
            ld::effects::query_policy(policy_entry_from_c(entry), effect_options_from_c(options)),
            *report)
            ? 1
            : 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (const std::exception&) {
        return 0;
    }
}

void ld_settings_free_policy_report(ld_settings_policy_report* report)
{
    if (!report) {
        return;
    }
    free_policy_report_fields(*report);
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
