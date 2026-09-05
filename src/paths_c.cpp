#include "linuxdesktop/paths_c.h"

#include "linuxdesktop/paths.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

namespace ld = linuxdesktop::paths;

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

void free_diagnostic(ld_paths_diagnostic& diagnostic)
{
    std::free(diagnostic.code);
    std::free(diagnostic.message);
    std::free(diagnostic.path);
    diagnostic = {};
}

int severity_to_c(linuxdesktop::severity value)
{
    switch (value) {
    case linuxdesktop::severity::info:
        return LD_PATHS_SEVERITY_INFO;
    case linuxdesktop::severity::warning:
        return LD_PATHS_SEVERITY_WARNING;
    case linuxdesktop::severity::error:
        return LD_PATHS_SEVERITY_ERROR;
    }
    return LD_PATHS_SEVERITY_ERROR;
}

ld::path_family family_from_c(int value)
{
    if (value < LD_PATHS_FAMILY_CONFIG || value > LD_PATHS_FAMILY_RUNTIME) {
        return ld::path_family::config;
    }
    return static_cast<ld::path_family>(value);
}

int family_to_c(ld::path_family value)
{
    return static_cast<int>(value);
}

ld::location_role location_role_from_c(int value)
{
    if (value < LD_PATHS_LOCATION_EXECUTABLE || value > LD_PATHS_LOCATION_RESOURCES) {
        return ld::location_role::resources;
    }
    return static_cast<ld::location_role>(value);
}

int location_role_to_c(ld::location_role value)
{
    return static_cast<int>(value);
}

ld::candidate_source source_from_c(int value)
{
    if (value < LD_PATHS_SOURCE_EXPLICIT_OPTION || value > LD_PATHS_SOURCE_WINE_PREFIX) {
        return ld::candidate_source::fallback;
    }
    return static_cast<ld::candidate_source>(value);
}

int source_to_c(ld::candidate_source value)
{
    return static_cast<int>(value);
}

ld::plugin_path_kind plugin_kind_from_c(int value)
{
    if (value < LD_PATHS_PLUGIN_LADSPA || value > LD_PATHS_PLUGIN_JSFX) {
        return ld::plugin_path_kind::vst3;
    }
    return static_cast<ld::plugin_path_kind>(value);
}

ld::plugin_asset_path_kind plugin_asset_kind_from_c(int value)
{
    if (value < LD_PATHS_PLUGIN_ASSET_SF2 || value > LD_PATHS_PLUGIN_ASSET_SFZ) {
        return ld::plugin_asset_path_kind::sf2;
    }
    return static_cast<ld::plugin_asset_path_kind>(value);
}

int plugin_kind_to_c(ld::plugin_path_kind value)
{
    return static_cast<int>(value);
}

int plugin_asset_kind_to_c(ld::plugin_asset_path_kind value)
{
    return static_cast<int>(value);
}

int plugin_category_to_c(ld::plugin_path_category value)
{
    return static_cast<int>(value);
}

ld::plugin_path_category plugin_category_from_c(int value)
{
    if (value < LD_PATHS_PLUGIN_CATEGORY_EXECUTABLE_PLUGIN ||
        value > LD_PATHS_PLUGIN_CATEGORY_RESOURCE) {
        return ld::plugin_path_category::application_extension;
    }
    return static_cast<ld::plugin_path_category>(value);
}

template <typename T>
T* allocate_array(size_t count)
{
    if (count == 0) {
        return nullptr;
    }
    return static_cast<T*>(std::calloc(count, sizeof(T)));
}

bool fill_diagnostics(const std::vector<linuxdesktop::diagnostic>& source, ld_paths_diagnostic*& diagnostics, size_t& count)
{
    diagnostics = allocate_array<ld_paths_diagnostic>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!diagnostics) {
        count = 0;
        return false;
    }

    for (size_t i = 0; i < source.size(); ++i) {
        diagnostics[i].severity = severity_to_c(source[i].level);
        diagnostics[i].code = duplicate_string(source[i].code);
        diagnostics[i].message = duplicate_string(source[i].message);
        diagnostics[i].path = duplicate_path(source[i].path);
        if (!diagnostics[i].code || !diagnostics[i].message || !diagnostics[i].path) {
            return false;
        }
    }
    return true;
}

void free_diagnostics(ld_paths_diagnostic*& diagnostics, size_t& count)
{
    if (diagnostics) {
        for (size_t i = 0; i < count; ++i) {
            free_diagnostic(diagnostics[i]);
        }
    }
    std::free(diagnostics);
    diagnostics = nullptr;
    count = 0;
}

bool fill_candidate(const ld::path_candidate& source, ld_paths_candidate& target)
{
    target.family = family_to_c(source.family);
    target.source = source_to_c(source.source);
    target.path = duplicate_path(source.path);
    target.selected = source.selected ? 1 : 0;
    return target.path && fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count);
}

bool fill_candidates(const std::vector<ld::path_candidate>& source, ld_paths_candidate*& candidates, size_t& count)
{
    candidates = allocate_array<ld_paths_candidate>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!candidates) {
        count = 0;
        return false;
    }

    for (size_t i = 0; i < source.size(); ++i) {
        if (!fill_candidate(source[i], candidates[i])) {
            return false;
        }
    }
    return true;
}

bool fill_location_candidate(const ld::location_candidate& source, ld_paths_location_candidate& target)
{
    target.role = location_role_to_c(source.role);
    target.source = source_to_c(source.source);
    target.path = duplicate_path(source.path);
    target.selected = source.selected ? 1 : 0;
    return target.path && fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count);
}

bool fill_location_candidates(
    const std::vector<ld::location_candidate>& source,
    ld_paths_location_candidate*& candidates,
    size_t& count)
{
    candidates = allocate_array<ld_paths_location_candidate>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!candidates) {
        count = 0;
        return false;
    }

    for (size_t i = 0; i < source.size(); ++i) {
        if (!fill_location_candidate(source[i], candidates[i])) {
            return false;
        }
    }
    return true;
}

bool fill_path_list_candidate(const ld::path_list_candidate& source, ld_paths_path_list_candidate& target)
{
    target.source = source_to_c(source.source);
    target.path = duplicate_path(source.path);
    target.selected = source.selected ? 1 : 0;
    return target.path && fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count);
}

bool fill_path_list_candidates(
    const std::vector<ld::path_list_candidate>& source,
    ld_paths_path_list_candidate*& candidates,
    size_t& count)
{
    candidates = allocate_array<ld_paths_path_list_candidate>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!candidates) {
        count = 0;
        return false;
    }

    for (size_t i = 0; i < source.size(); ++i) {
        if (!fill_path_list_candidate(source[i], candidates[i])) {
            return false;
        }
    }
    return true;
}

bool fill_plugin_path_candidate(const ld::plugin_path_candidate& source, ld_paths_plugin_path_candidate& target)
{
    target.set_name = duplicate_string(source.set_name);
    target.has_kind = source.kind ? 1 : 0;
    target.kind = source.kind ? plugin_kind_to_c(*source.kind) : -1;
    target.has_asset_kind = source.asset_kind ? 1 : 0;
    target.asset_kind = source.asset_kind ? plugin_asset_kind_to_c(*source.asset_kind) : -1;
    target.category = plugin_category_to_c(source.category);
    target.source = source_to_c(source.source);
    target.path = duplicate_path(source.path);
    target.selected = source.selected ? 1 : 0;
    return target.set_name && target.path && fill_diagnostics(source.diagnostics, target.diagnostics, target.diagnostic_count);
}

bool fill_plugin_path_candidates(
    const std::vector<ld::plugin_path_candidate>& source,
    ld_paths_plugin_path_candidate*& candidates,
    size_t& count)
{
    candidates = allocate_array<ld_paths_plugin_path_candidate>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!candidates) {
        count = 0;
        return false;
    }

    for (size_t i = 0; i < source.size(); ++i) {
        if (!fill_plugin_path_candidate(source[i], candidates[i])) {
            return false;
        }
    }
    return true;
}

void free_candidate(ld_paths_candidate& candidate)
{
    std::free(candidate.path);
    free_diagnostics(candidate.diagnostics, candidate.diagnostic_count);
    candidate = {};
}

void free_candidates(ld_paths_candidate*& candidates, size_t& count)
{
    if (candidates) {
        for (size_t i = 0; i < count; ++i) {
            free_candidate(candidates[i]);
        }
    }
    std::free(candidates);
    candidates = nullptr;
    count = 0;
}

void free_location_candidate(ld_paths_location_candidate& candidate)
{
    std::free(candidate.path);
    free_diagnostics(candidate.diagnostics, candidate.diagnostic_count);
    candidate = {};
}

void free_location_candidates(ld_paths_location_candidate*& candidates, size_t& count)
{
    if (candidates) {
        for (size_t i = 0; i < count; ++i) {
            free_location_candidate(candidates[i]);
        }
    }
    std::free(candidates);
    candidates = nullptr;
    count = 0;
}

void free_path_list_candidate(ld_paths_path_list_candidate& candidate)
{
    std::free(candidate.path);
    free_diagnostics(candidate.diagnostics, candidate.diagnostic_count);
    candidate = {};
}

void free_path_list_candidates(ld_paths_path_list_candidate*& candidates, size_t& count)
{
    if (candidates) {
        for (size_t i = 0; i < count; ++i) {
            free_path_list_candidate(candidates[i]);
        }
    }
    std::free(candidates);
    candidates = nullptr;
    count = 0;
}

void free_plugin_path_candidate(ld_paths_plugin_path_candidate& candidate)
{
    std::free(candidate.set_name);
    std::free(candidate.path);
    free_diagnostics(candidate.diagnostics, candidate.diagnostic_count);
    candidate = {};
}

void free_plugin_path_candidates(ld_paths_plugin_path_candidate*& candidates, size_t& count)
{
    if (candidates) {
        for (size_t i = 0; i < count; ++i) {
            free_plugin_path_candidate(candidates[i]);
        }
    }
    std::free(candidates);
    candidates = nullptr;
    count = 0;
}

std::map<std::string, std::string> environment_from_c(
    const ld_paths_environment_entry* entries,
    size_t count)
{
    std::map<std::string, std::string> environment;
    if (!entries) {
        return environment;
    }
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].name && entries[i].value) {
            environment[entries[i].name] = entries[i].value;
        }
    }
    return environment;
}

std::optional<std::filesystem::path> optional_path(const char* value)
{
    if (!value || !value[0]) {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

std::optional<ld::platform_path_defaults> platform_defaults_from_c(
    const ld_paths_resolver_options& options)
{
    ld::platform_path_defaults defaults;
    bool has_default = false;

    auto assign_path = [&has_default](std::optional<std::filesystem::path>& target, const char* value) {
        target = optional_path(value);
        has_default = has_default || target.has_value();
    };

    assign_path(defaults.xdg_config_home, options.xdg_config_home_default);
    assign_path(defaults.xdg_data_home, options.xdg_data_home_default);
    assign_path(defaults.xdg_state_home, options.xdg_state_home_default);
    assign_path(defaults.xdg_cache_home, options.xdg_cache_home_default);
    assign_path(defaults.xdg_runtime_dir, options.xdg_runtime_dir_default);
    assign_path(defaults.windows_roaming_appdata, options.windows_roaming_appdata_default);
    assign_path(defaults.windows_local_appdata, options.windows_local_appdata_default);

    if (!has_default) {
        return std::nullopt;
    }
    return defaults;
}

ld::path_list_options path_list_options_from_c(const ld_paths_path_list_options& options)
{
    ld::path_list_options result;
    result.require_absolute = options.require_absolute != 0;
    result.drop_duplicates = options.drop_duplicates != 0;
    if (options.separator != '\0') {
        result.separator = options.separator;
    }
    return result;
}

bool fill_string_array(
    const std::vector<std::filesystem::path>& source,
    char**& paths,
    size_t& count)
{
    paths = allocate_array<char*>(source.size());
    count = source.size();
    if (source.empty()) {
        return true;
    }
    if (!paths) {
        count = 0;
        return false;
    }
    for (size_t i = 0; i < source.size(); ++i) {
        paths[i] = duplicate_path(source[i]);
        if (!paths[i]) {
            return false;
        }
    }
    return true;
}

void free_string_array(char**& paths, size_t& count)
{
    if (paths) {
        for (size_t i = 0; i < count; ++i) {
            std::free(paths[i]);
        }
    }
    std::free(paths);
    paths = nullptr;
    count = 0;
}

void free_plugin_set(ld_paths_plugin_path_set& set)
{
    std::free(set.name);
    free_string_array(set.paths, set.path_count);
    set = {};
}

} // namespace

extern "C" {

void ld_paths_resolver_options_init(ld_paths_resolver_options* options)
{
    if (!options) {
        return;
    }
    *options = {};
    options->use_process_environment = 1;
}

void ld_paths_path_list_options_init(ld_paths_path_list_options* options)
{
    if (!options) {
        return;
    }
    *options = {};
    options->require_absolute = 1;
    options->drop_duplicates = 1;
}

void ld_paths_plugin_path_options_init(ld_paths_plugin_path_options* options)
{
    if (!options) {
        return;
    }
    *options = {};
    options->use_process_environment = 1;
    options->include_default_kinds = 1;
    options->include_default_asset_kinds = 1;
    ld_paths_path_list_options_init(&options->list_options);
}

int ld_paths_resolve_app_paths(
    const ld_paths_resolver_options* options,
    ld_paths_resolver_report* report)
{
    if (!options || !report) {
        return 0;
    }
    ld_paths_free_resolver_report(report);

    try {
        ld::app_identity identity;
        identity.organization = options->organization ? options->organization : "";
        identity.application = options->application ? options->application : "";

        ld::resolver_options resolver_options;
        resolver_options.config_override = optional_path(options->config_override);
        resolver_options.data_override = optional_path(options->data_override);
        resolver_options.state_override = optional_path(options->state_override);
        resolver_options.cache_override = optional_path(options->cache_override);
        resolver_options.temp_override = optional_path(options->temp_override);
        resolver_options.runtime_override = optional_path(options->runtime_override);
        resolver_options.resource_root = optional_path(options->resource_root);
        resolver_options.install_prefix = optional_path(options->install_prefix);
        resolver_options.executable_path = optional_path(options->executable_path);
        resolver_options.home_directory = optional_path(options->home_directory);
        resolver_options.platform_defaults = platform_defaults_from_c(*options);
        resolver_options.environment = environment_from_c(options->environment, options->environment_count);
        resolver_options.use_process_environment = options->use_process_environment != 0;

        const auto resolved = ld::resolve_app_paths(identity, resolver_options);

        report->selected = allocate_array<ld_paths_selected_path>(resolved.selected.size());
        report->selected_count = resolved.selected.size();
        if (resolved.selected.size() && !report->selected) {
            return 0;
        }
        size_t index = 0;
        for (const auto& item : resolved.selected) {
            report->selected[index].family = family_to_c(item.first);
            report->selected[index].path = duplicate_path(item.second);
            if (!report->selected[index].path) {
                return 0;
            }
            ++index;
        }

        report->selected_locations = allocate_array<ld_paths_selected_location>(resolved.selected_locations.size());
        report->selected_location_count = resolved.selected_locations.size();
        if (resolved.selected_locations.size() && !report->selected_locations) {
            return 0;
        }
        index = 0;
        for (const auto& item : resolved.selected_locations) {
            report->selected_locations[index].role = location_role_to_c(item.first);
            report->selected_locations[index].path = duplicate_path(item.second);
            if (!report->selected_locations[index].path) {
                return 0;
            }
            ++index;
        }

        return fill_candidates(resolved.candidates, report->candidates, report->candidate_count) &&
            fill_location_candidates(resolved.location_candidates, report->location_candidates, report->location_candidate_count) &&
            fill_diagnostics(resolved.diagnostics, report->diagnostics, report->diagnostic_count);
    } catch (const std::exception&) {
        ld_paths_free_resolver_report(report);
        return 0;
    }
}

void ld_paths_free_resolver_report(ld_paths_resolver_report* report)
{
    if (!report) {
        return;
    }
    if (report->selected) {
        for (size_t i = 0; i < report->selected_count; ++i) {
            std::free(report->selected[i].path);
        }
    }
    std::free(report->selected);
    if (report->selected_locations) {
        for (size_t i = 0; i < report->selected_location_count; ++i) {
            std::free(report->selected_locations[i].path);
        }
    }
    std::free(report->selected_locations);
    free_candidates(report->candidates, report->candidate_count);
    free_location_candidates(report->location_candidates, report->location_candidate_count);
    free_diagnostics(report->diagnostics, report->diagnostic_count);
    *report = {};
}

int ld_paths_parse_path_list(
    const char* value,
    const ld_paths_path_list_options* options,
    ld_paths_path_list_report* report)
{
    if (!value || !report) {
        return 0;
    }
    ld_paths_free_path_list_report(report);

    try {
        ld_paths_path_list_options defaults;
        ld_paths_path_list_options_init(&defaults);
        const auto list_options = path_list_options_from_c(options ? *options : defaults);
        const auto parsed = ld::parse_path_list(value, list_options);
        return fill_string_array(parsed.paths, report->paths, report->path_count) &&
            fill_path_list_candidates(parsed.candidates, report->candidates, report->candidate_count) &&
            fill_diagnostics(parsed.diagnostics, report->diagnostics, report->diagnostic_count);
    } catch (const std::exception&) {
        ld_paths_free_path_list_report(report);
        return 0;
    }
}

void ld_paths_free_path_list_report(ld_paths_path_list_report* report)
{
    if (!report) {
        return;
    }
    free_string_array(report->paths, report->path_count);
    free_path_list_candidates(report->candidates, report->candidate_count);
    free_diagnostics(report->diagnostics, report->diagnostic_count);
    *report = {};
}

int ld_paths_resolve_plugin_path_sets(
    const ld_paths_plugin_path_options* options,
    ld_paths_plugin_path_report* report)
{
    if (!report) {
        return 0;
    }
    ld_paths_free_plugin_path_report(report);

    try {
        ld_paths_plugin_path_options defaults;
        ld_paths_plugin_path_options_init(&defaults);
        const auto& input = options ? *options : defaults;

        ld::plugin_path_options plugin_options;
        plugin_options.home_directory = optional_path(input.home_directory);
        plugin_options.wine_prefix = optional_path(input.wine_prefix);
        plugin_options.include_wine_prefix_defaults = input.include_wine_prefix_defaults != 0;
        plugin_options.environment = environment_from_c(input.environment, input.environment_count);
        plugin_options.use_process_environment = input.use_process_environment != 0;
        plugin_options.include_default_kinds = input.include_default_kinds != 0;
        plugin_options.include_default_asset_kinds = input.include_default_asset_kinds != 0;
        plugin_options.list_options = path_list_options_from_c(input.list_options);
        for (size_t i = 0; i < input.kind_count; ++i) {
            plugin_options.kinds.push_back(plugin_kind_from_c(input.kinds[i]));
        }
        for (size_t i = 0; i < input.asset_kind_count; ++i) {
            plugin_options.asset_kinds.push_back(plugin_asset_kind_from_c(input.asset_kinds[i]));
        }

        const auto resolved = ld::resolve_plugin_path_sets(plugin_options);
        report->sets = allocate_array<ld_paths_plugin_path_set>(resolved.sets.size());
        report->set_count = resolved.sets.size();
        if (resolved.sets.size() && !report->sets) {
            return 0;
        }
        for (size_t i = 0; i < resolved.sets.size(); ++i) {
            report->sets[i].name = duplicate_string(resolved.sets[i].name);
            report->sets[i].has_kind = resolved.sets[i].kind ? 1 : 0;
            report->sets[i].kind = resolved.sets[i].kind ? plugin_kind_to_c(*resolved.sets[i].kind) : -1;
            report->sets[i].has_asset_kind = resolved.sets[i].asset_kind ? 1 : 0;
            report->sets[i].asset_kind = resolved.sets[i].asset_kind
                ? plugin_asset_kind_to_c(*resolved.sets[i].asset_kind)
                : -1;
            report->sets[i].category = plugin_category_to_c(resolved.sets[i].category);
            if (!report->sets[i].name ||
                !fill_string_array(resolved.sets[i].paths, report->sets[i].paths, report->sets[i].path_count)) {
                return 0;
            }
        }

        return fill_plugin_path_candidates(resolved.candidates, report->candidates, report->candidate_count) &&
            fill_diagnostics(resolved.diagnostics, report->diagnostics, report->diagnostic_count);
    } catch (const std::exception&) {
        ld_paths_free_plugin_path_report(report);
        return 0;
    }
}

void ld_paths_free_plugin_path_report(ld_paths_plugin_path_report* report)
{
    if (!report) {
        return;
    }
    if (report->sets) {
        for (size_t i = 0; i < report->set_count; ++i) {
            free_plugin_set(report->sets[i]);
        }
    }
    std::free(report->sets);
    free_plugin_path_candidates(report->candidates, report->candidate_count);
    free_diagnostics(report->diagnostics, report->diagnostic_count);
    *report = {};
}

const char* ld_paths_severity_name(int severity)
{
    switch (severity) {
    case LD_PATHS_SEVERITY_INFO:
        return "info";
    case LD_PATHS_SEVERITY_WARNING:
        return "warning";
    case LD_PATHS_SEVERITY_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char* ld_paths_path_family_name(int family)
{
    return ld::to_string(family_from_c(family)).data();
}

const char* ld_paths_location_role_name(int role)
{
    return ld::to_string(location_role_from_c(role)).data();
}

const char* ld_paths_candidate_source_name(int source)
{
    return ld::to_string(source_from_c(source)).data();
}

const char* ld_paths_plugin_path_kind_name(int kind)
{
    return ld::to_string(plugin_kind_from_c(kind)).data();
}

const char* ld_paths_plugin_asset_path_kind_name(int kind)
{
    return ld::to_string(plugin_asset_kind_from_c(kind)).data();
}

const char* ld_paths_plugin_path_category_name(int category)
{
    return ld::to_string(plugin_category_from_c(category)).data();
}

int ld_paths_version_major(void)
{
    return ld::version_major;
}

int ld_paths_version_minor(void)
{
    return ld::version_minor;
}

int ld_paths_version_patch(void)
{
    return ld::version_patch;
}

const char* ld_paths_version_string(void)
{
    return "0.1.0";
}

} // extern "C"
