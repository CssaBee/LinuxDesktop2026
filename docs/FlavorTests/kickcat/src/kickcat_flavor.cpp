#include "kickcat_flavor.hpp"

#include "kickcat/generated/platform_path_defaults.hpp"

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <utility>

namespace flavor_tests::kickcat {

namespace {

ToolDiagnostic diagnostic(
    DiagnosticCode code,
    std::string message,
    bool fatal,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

bool settings_file_is_readable(const std::filesystem::path& path, std::string& message)
{
    if (std::filesystem::exists(path)) {
        return true;
    }
    message = "KickCAT GUI settings file could not be reopened";
    return false;
}

} // namespace

ToolLayout KickcatTools::resolveLayout(const RuntimeEnvironment& environment) const
{
    linuxdesktop::paths::app_identity identity;
    identity.organization = "KickCAT";
    identity.application = "KickCAT";

    linuxdesktop::paths::resolver_options options;
    options.executable_path = environment.executable_directory / "kickui";
    options.resource_root = environment.executable_directory / "resources";
    options.home_directory = environment.home_directory;
    options.runtime_override = environment.runtime_directory;
    options.environment = environment.environment;
    options.use_process_environment = false;
    if (environment.home_directory) {
        options.platform_defaults = linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory);
    }

    const auto paths = linuxdesktop::paths::resolve_app_paths(identity, options);

    ToolLayout layout;
    layout.config_root = paths.selected.at(linuxdesktop::paths::path_family::config);
    layout.cache_root = paths.selected.at(linuxdesktop::paths::path_family::cache);
    layout.runtime_root = paths.selected.at(linuxdesktop::paths::path_family::runtime);
    layout.gui_settings_file = layout.config_root / "kickui.ini";
    layout.eeprom_workspace = layout.cache_root / "eeprom";
    layout.simulator_socket = layout.runtime_root / "kickcat-simulator.sock";
    layout.esi_search_roots = {
        paths.selected_locations.at(linuxdesktop::paths::location_role::resources) / "esi",
        layout.config_root / "esi",
    };
    for (const auto& item : paths.diagnostics) {
        layout.diagnostics.push_back(diagnostic(
            DiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
    return layout;
}

MasterLaunchPlan KickcatTools::planMasterLaunch(const ToolLayout& layout, const MasterOptions& options) const
{
    MasterLaunchPlan plan;
    plan.realtime_core = options.realtime;
    plan.interface_name = options.interface_name;
    plan.esi_file = options.esi_file;

    if (options.interface_name.empty()) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::MissingNetworkInterface,
            "EtherCAT master launch requires a network interface",
            true));
    }

    if (options.esi_file && !std::filesystem::exists(*options.esi_file)) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::MissingEsiFile,
            "requested ESI file is missing",
            true,
            *options.esi_file));
    }

    plan.start_bus = plan.diagnostics.empty();
    (void)layout;
    return plan;
}

GuiSettingsSaveResult KickcatTools::saveGuiSettings(const ToolLayout& layout, std::string content) const
{
    auto report = linuxdesktop::settings::write_common_config(
        {layout.gui_settings_file, std::move(content), true},
        settings_file_is_readable);
    return {report.ok, report.backup_path, layout.gui_settings_file};
}

} // namespace flavor_tests::kickcat
