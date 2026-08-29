#include "openrgb_flavor.hpp"

#include <utility>

namespace flavor_tests::openrgb {

namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;

ResourceManager::ResourceManager() = default;

bool ResourceManager::InitializeResources(const RuntimeEnvironment& environment)
{
    detection_enabled_ = true;
    first_detection_complete_ = false;
    init_finished_ = false;

    if (!SetupConfigurationDirectory(environment)) {
        return false;
    }

    settings_manager_.LoadSettings(paths_.config_dir / "OpenRGB.json");
    log_manager_.Configure(paths_.config_dir);
    profile_manager_ = {paths_.config_dir};
    init_finished_ = true;
    return true;
}

bool ResourceManager::SetupConfigurationDirectory(const RuntimeEnvironment& environment)
{
    ldp::app_identity identity;
    identity.organization = "OpenRGB";
    identity.application = "OpenRGB";

    ldp::resolver_options options;
    options.resource_root = environment.resource_root;
    options.home_directory = environment.home_directory;
    options.environment = environment.variables;
    options.use_process_environment = false;

    report_ = ldp::resolve_app_paths(identity, options);
    paths_.resources_dir = report_.selected.at(ldp::path_family::resources);
    paths_.config_dir = report_.selected.at(ldp::path_family::config);
    paths_.profiles_dir = paths_.config_dir / "profiles";
    return true;
}

lds::effects::effect_report enable_autostart(
    const std::filesystem::path& executable,
    std::vector<std::string> arguments,
    const std::filesystem::path& working_directory,
    const std::filesystem::path& override_directory)
{
    lds::effects::autostart_entry entry;
    entry.id = "OpenRGB";
    entry.display_name = "OpenRGB";
    entry.executable = executable;
    entry.arguments = std::move(arguments);
    entry.working_directory = working_directory;
    entry.enabled = true;
    entry.user_scope = true;

    lds::effects::apply_options options;
    options.dry_run = true;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = override_directory;

    return lds::effects::apply_autostart(entry, options);
}

} // namespace flavor_tests::openrgb
