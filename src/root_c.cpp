#include "linuxdesktop/root_c.h"

#include "linuxdesktop/root.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <map>
#include <new>
#include <optional>
#include <string>

namespace {

namespace ld = linuxdesktop::root;

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

bool assign_string(char*& target, const std::string& value)
{
    target = duplicate_string(value);
    return target != nullptr;
}

bool assign_path(char*& target, const std::filesystem::path& value)
{
    return assign_string(target, path_to_utf8_string(value));
}

std::optional<std::filesystem::path> optional_path(const char* value)
{
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

int severity_to_c(ld::severity value)
{
    switch (value) {
    case ld::severity::info:
        return LD_ROOT_SEVERITY_INFO;
    case ld::severity::warning:
        return LD_ROOT_SEVERITY_WARNING;
    case ld::severity::error:
        return LD_ROOT_SEVERITY_ERROR;
    }
    return LD_ROOT_SEVERITY_ERROR;
}

ld::app_local_level app_local_level_from_c(int value)
{
    switch (value) {
    case LD_ROOT_APP_LOCAL_OFF:
        return ld::app_local_level::off;
    case LD_ROOT_APP_LOCAL_PROFILE:
        return ld::app_local_level::profile;
    case LD_ROOT_APP_LOCAL_CLEAN:
        return ld::app_local_level::clean;
    case LD_ROOT_APP_LOCAL_CONFIG_ONLY:
    default:
        return ld::app_local_level::config_only;
    }
}

int app_local_level_to_c(ld::app_local_level value)
{
    switch (value) {
    case ld::app_local_level::off:
        return LD_ROOT_APP_LOCAL_OFF;
    case ld::app_local_level::profile:
        return LD_ROOT_APP_LOCAL_PROFILE;
    case ld::app_local_level::clean:
        return LD_ROOT_APP_LOCAL_CLEAN;
    case ld::app_local_level::config_only:
        return LD_ROOT_APP_LOCAL_CONFIG_ONLY;
    }
    return LD_ROOT_APP_LOCAL_CONFIG_ONLY;
}

ld::purpose_kind purpose_from_c(int value)
{
    if (value < LD_ROOT_PURPOSE_RESOURCES || value > LD_ROOT_PURPOSE_CUSTOM) {
        return ld::purpose_kind::custom;
    }
    return static_cast<ld::purpose_kind>(value);
}

ld::ownership_kind ownership_from_c(int value)
{
    if (value < LD_ROOT_OWNERSHIP_USER_ROAMING || value > LD_ROOT_OWNERSHIP_ENFORCED) {
        return ld::ownership_kind::user_roaming;
    }
    return static_cast<ld::ownership_kind>(value);
}

ld::component_kind component_kind_from_c(int value)
{
    if (value < LD_ROOT_COMPONENT_PLUGIN || value > LD_ROOT_COMPONENT_CUSTOM) {
        return ld::component_kind::custom;
    }
    return static_cast<ld::component_kind>(value);
}

void free_diagnostic(ld_root_diagnostic& diagnostic)
{
    std::free(diagnostic.code);
    std::free(diagnostic.message);
    std::free(diagnostic.path);
    diagnostic = {};
}

bool fill_diagnostics(const std::vector<ld::diagnostic>& source, ld_root_diagnostic*& diagnostics, size_t& count)
{
    diagnostics = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }
    diagnostics = static_cast<ld_root_diagnostic*>(std::calloc(source.size(), sizeof(ld_root_diagnostic)));
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
            return false;
        }
    }
    return true;
}

void free_named_root(ld_root_named_root& root)
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

bool fill_named_root(const ld::named_root& source, ld_root_named_root& target)
{
    target = {};
    target.purpose = static_cast<int>(source.purpose);
    target.ownership = static_cast<int>(source.ownership);
    target.created = source.created ? 1 : 0;
    if (!assign_string(target.name, source.name) || !assign_path(target.path, source.path) ||
        !fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        free_named_root(target);
        return false;
    }
    return true;
}

bool fill_named_roots(const std::vector<ld::named_root>& source, ld_root_named_root*& roots, size_t& count)
{
    roots = nullptr;
    count = 0;
    if (source.empty()) {
        return true;
    }
    roots = static_cast<ld_root_named_root*>(std::calloc(source.size(), sizeof(ld_root_named_root)));
    if (!roots) {
        return false;
    }
    count = source.size();
    for (size_t index = 0; index != source.size(); ++index) {
        if (!fill_named_root(source[index], roots[index])) {
            return false;
        }
    }
    return true;
}

void free_component_roots(ld_root_component_roots& component)
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

std::map<std::string, std::string> environment_from_c(const ld_root_environment_entry* entries, size_t count)
{
    std::map<std::string, std::string> environment;
    if (!entries) {
        return environment;
    }
    for (size_t index = 0; index != count; ++index) {
        const auto& entry = entries[index];
        if (entry.name && entry.name[0]) {
            environment[entry.name] = entry.value ? entry.value : "";
        }
    }
    return environment;
}

bool fill_report(const ld::report& source, ld_root_report& target)
{
    target = {};
    target.app_local_requested = source.app_local_requested ? 1 : 0;
    target.app_local_active = source.app_local_active ? 1 : 0;
    target.app_root_override_active = source.app_root_override_active ? 1 : 0;
    target.user_config_override_active = source.user_config_override_active ? 1 : 0;
    target.app_local_level = app_local_level_to_c(source.app_local);
    if (!assign_path(target.resources, source.roots.resources) ||
        !assign_path(target.config, source.roots.config) ||
        !assign_path(target.data, source.roots.data) ||
        !assign_path(target.state, source.roots.state) ||
        !assign_path(target.cache, source.roots.cache) ||
        !assign_path(target.runtime, source.roots.runtime) ||
        !assign_path(target.session, source.roots.session) ||
        !assign_path(target.plugin_config, source.roots.plugin_config) ||
        !fill_named_roots(source.named_roots, target.named_roots, target.named_root_count) ||
        !fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count)) {
        ld_root_free_report(&target);
        return false;
    }
    if (!source.component_roots.empty()) {
        target.component_roots = static_cast<ld_root_component_roots*>(
            std::calloc(source.component_roots.size(), sizeof(ld_root_component_roots)));
        if (!target.component_roots) {
            ld_root_free_report(&target);
            return false;
        }
        target.component_root_count = source.component_roots.size();
        for (size_t index = 0; index != source.component_roots.size(); ++index) {
            const auto& component = source.component_roots[index];
            auto& target_component = target.component_roots[index];
            target_component.kind = static_cast<int>(component.kind);
            if (!assign_string(target_component.name, component.name) ||
                !fill_named_roots(component.roots, target_component.roots, target_component.root_count) ||
                !fill_diagnostics(component.diagnostics, target_component.diagnostics, target_component.diagnostic_count)) {
                ld_root_free_report(&target);
                return false;
            }
        }
    }
    return true;
}

} // namespace

extern "C" {

void ld_root_options_init(ld_root_options* options)
{
    if (!options) {
        return;
    }
    *options = {};
    options->allow_app_local_root = 1;
    options->create_directories = 1;
    options->use_process_environment = 1;
    options->app_local_level = LD_ROOT_APP_LOCAL_CONFIG_ONLY;
}

int ld_root_resolve_app_roots(const ld_root_options* options, ld_root_report* report)
{
    if (!options || !report || !options->organization || !options->application) {
        return 0;
    }
    try {
        ld::app_identity identity{options->organization, options->application};
        ld::options root_options;
        root_options.resource_root = optional_path(options->resource_root);
        root_options.app_root_override = optional_path(options->app_root_override);
        root_options.user_config_override = optional_path(options->user_config_override);
        root_options.app_local_marker = optional_path(options->app_local_marker);
        root_options.home_directory = optional_path(options->home_directory);
        root_options.environment = environment_from_c(options->environment, options->environment_count);
        root_options.allow_app_local_root = options->allow_app_local_root != 0;
        root_options.deny_app_local_root_in_privileged_install =
            options->deny_app_local_root_in_privileged_install != 0;
        root_options.allow_user_config_for_app_local_root =
            options->allow_user_config_for_app_local_root != 0;
        root_options.create_directories = options->create_directories != 0;
        root_options.use_process_environment = options->use_process_environment != 0;
        root_options.app_local = app_local_level_from_c(options->app_local_level);
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
            request.purpose = purpose_from_c(source.purpose);
            request.ownership = ownership_from_c(source.ownership);
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
                request.purpose = purpose_from_c(root_source.purpose);
                request.ownership = ownership_from_c(root_source.ownership);
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

void ld_root_free_report(ld_root_report* report)
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
    if (report->diagnostics) {
        for (size_t index = 0; index != report->diagnostic_count; ++index) {
            free_diagnostic(report->diagnostics[index]);
        }
        std::free(report->diagnostics);
    }
    *report = {};
}

const char* ld_root_severity_name(int severity)
{
    switch (severity) {
    case LD_ROOT_SEVERITY_INFO:
        return "info";
    case LD_ROOT_SEVERITY_WARNING:
        return "warning";
    case LD_ROOT_SEVERITY_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char* ld_root_app_local_level_name(int level)
{
    return ld::to_string(app_local_level_from_c(level)).data();
}

const char* ld_root_purpose_name(int purpose)
{
    return ld::to_string(purpose_from_c(purpose)).data();
}

const char* ld_root_ownership_name(int ownership)
{
    return ld::to_string(ownership_from_c(ownership)).data();
}

const char* ld_root_component_kind_name(int kind)
{
    return ld::to_string(component_kind_from_c(kind)).data();
}

} // extern "C"
