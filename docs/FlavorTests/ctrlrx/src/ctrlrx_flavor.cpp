#include "ctrlrx_flavor.hpp"

#include "ctrlrx/generated/platform_path_defaults.hpp"

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <sstream>
#include <utility>

namespace flavor_tests::ctrlrx {

namespace {

linuxdesktop::paths::resolver_report resolve_ctrlrx_paths(const RuntimeEnvironment& environment)
{
    linuxdesktop::paths::app_identity identity;
    identity.organization = "Ctrlr";
    identity.application = "CtrlrX";

    linuxdesktop::paths::resolver_options options;
    options.executable_path = environment.executable_directory / "CtrlrX";
    options.resource_root = environment.executable_directory / "Resources";
    options.home_directory = environment.home_directory;
    options.runtime_override = environment.runtime_directory;
    options.environment = environment.environment;
    options.use_process_environment = false;
    if (environment.home_directory) {
        options.platform_defaults = linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory);
    }
    return linuxdesktop::paths::resolve_app_paths(identity, options);
}

std::string render_preferences(const Preferences& preferences)
{
    std::ostringstream output;
    output << "autoSave=" << (preferences.auto_save ? "true" : "false") << '\n';
    output << "autoSaveInterval=" << preferences.auto_save_interval_minutes << '\n';
    output << "defaultLookAndFeel=" << preferences.default_look_and_feel << '\n';
    return output.str();
}

std::filesystem::path first_path_for(
    const linuxdesktop::paths::plugin_path_report& report,
    const std::string& set_name)
{
    for (const auto& set : report.sets) {
        if (set.name == set_name && !set.paths.empty()) {
            return set.paths.front();
        }
    }
    return {};
}

} // namespace

PreferencesSaveResult CtrlrSettings::savePreferences(
    const RuntimeEnvironment& environment,
    InstanceKind kind,
    const Preferences& preferences) const
{
    const auto paths = resolve_ctrlrx_paths(environment);
    const auto preferences_file =
        paths.selected.at(linuxdesktop::paths::path_family::config) / "Ctrlr.settings";

    if (kind != InstanceKind::Standalone) {
        return {false, std::nullopt, preferences_file};
    }

    auto report = linuxdesktop::settings::write_common_config(
        {preferences_file, render_preferences(preferences), true},
        [](const std::filesystem::path& path, std::string& message) {
            if (std::filesystem::exists(path)) {
                return true;
            }
            message = "CtrlrX preferences file could not be reopened";
            return false;
        });
    return {report.ok, report.backup_path, preferences_file};
}

ResourceReloadPlan CtrlrSettings::reloadResources(const RuntimeEnvironment& environment) const
{
    const auto paths = resolve_ctrlrx_paths(environment);
    const auto resource_root = paths.selected_locations.at(linuxdesktop::paths::location_role::resources);
    return {
        resource_root,
        {resource_root / "Panels", paths.selected.at(linuxdesktop::paths::path_family::data) / "Panels"},
        {resource_root / "Lua", paths.selected.at(linuxdesktop::paths::path_family::config) / "Lua"},
    };
}

PluginExportPlan CtrlrSettings::planPluginExport(
    const RuntimeEnvironment& environment,
    PluginFormat format) const
{
    linuxdesktop::paths::plugin_path_options options;
    options.environment = environment.environment;
    options.home_directory = environment.home_directory;
    options.use_process_environment = false;
    options.kinds = {
        linuxdesktop::paths::plugin_path_kind::vst3,
        linuxdesktop::paths::plugin_path_kind::audio_unit,
        linuxdesktop::paths::plugin_path_kind::aax,
    };

    const auto plugins = linuxdesktop::paths::resolve_plugin_path_sets(options);
    const auto paths = resolve_ctrlrx_paths(environment);

    std::filesystem::path output_root;
    switch (format) {
    case PluginFormat::Vst3:
        output_root = first_path_for(plugins, "vst3");
        break;
    case PluginFormat::AudioUnit:
        output_root = first_path_for(plugins, "audio_unit");
        break;
    case PluginFormat::Aax:
        output_root = first_path_for(plugins, "aax");
        break;
    }

    return {
        format,
        output_root,
        paths.selected.at(linuxdesktop::paths::path_family::cache) / "PluginExport" / "CtrlrX.jucer",
        true,
    };
}

} // namespace flavor_tests::ctrlrx
