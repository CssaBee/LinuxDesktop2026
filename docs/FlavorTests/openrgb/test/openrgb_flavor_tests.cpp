#include "openrgb_flavor.hpp"

#include "linuxdesktop/paths.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

namespace ldp = linuxdesktop::paths;

struct test_failure : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw test_failure(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-openrgb-flavor";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw test_failure("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void resource_manager_uses_xdg_config_for_settings_and_profiles()
{
    const auto root = test_root();
    const auto resources = root / "share" / "OpenRGB";
    const auto config_home = root / "xdg-config";
    std::filesystem::create_directories(resources);

    flavor_tests::openrgb::RuntimeEnvironment environment;
    environment.resource_root = resources;
    environment.variables["XDG_CONFIG_HOME"] = config_home.string();

    flavor_tests::openrgb::ResourceManager manager;
    require(manager.InitializeResources(environment), "resource manager start should succeed");

    const auto& paths = manager.paths();
    require(paths.resources_dir == resources, "resources should come from the runtime environment");
    require(paths.config_dir == config_home / "OpenRGB" / "OpenRGB",
        "config should resolve from XDG_CONFIG_HOME");
    require(paths.profiles_dir == config_home / "OpenRGB" / "OpenRGB" / "profiles",
        "profiles should stay below OpenRGB config");
    require(manager.report().selected.at(ldp::path_family::config) == paths.config_dir,
        "path report should expose selected config root");
    require(manager.settingsManager().loaded_settings_file == paths.config_dir / "OpenRGB.json",
        "settings manager should load OpenRGB.json from the resolved config directory");
    require(manager.logManager().configured_directory == paths.config_dir,
        "log manager should use the resolved config directory");
    require(manager.profileManager().configuration_directory == paths.config_dir,
        "profile manager should keep OpenRGB's config-root convention");
    require(manager.profileManager().profile_directory == paths.config_dir / "profiles",
        "profile directory should be created below the config root");
    require(manager.profileManager().profile_list_reloaded,
        "profile manager should refresh after receiving the config directory");
}

void resource_manager_can_switch_configuration_directory_after_startup()
{
    const auto root = test_root();
    const auto resources = root / "share" / "OpenRGB";
    const auto config_home = root / "xdg-config";
    const auto alternate_config = root / "portable-config";
    std::filesystem::create_directories(resources);

    flavor_tests::openrgb::RuntimeEnvironment environment;
    environment.resource_root = resources;
    environment.variables["XDG_CONFIG_HOME"] = config_home.string();

    flavor_tests::openrgb::ResourceManager manager;
    require(manager.InitializeResources(environment), "resource manager start should succeed");
    require(manager.SetConfigurationDirectory(alternate_config), "config directory switch should succeed");

    require(manager.paths().config_dir == alternate_config, "runtime paths should follow the new config dir");
    require(manager.paths().profiles_dir == alternate_config / "profiles",
        "runtime profile path should follow the new config dir");
    require(manager.settingsManager().loaded_settings_file == alternate_config / "OpenRGB.json",
        "settings manager should reload settings from the new config dir");
    require(manager.logManager().configured_directory == alternate_config,
        "log manager should reconfigure against the new config dir");
    require(manager.profileManager().configuration_directory == alternate_config,
        "profile manager should adopt the new config dir");
    require(std::filesystem::is_directory(alternate_config / "profiles"),
        "profile manager should create the profile directory");
}

void settings_save_uses_validated_backup_write()
{
    const auto root = test_root();
    const auto resources = root / "share" / "OpenRGB";
    const auto config_home = root / "xdg-config";
    std::filesystem::create_directories(resources);

    flavor_tests::openrgb::RuntimeEnvironment environment;
    environment.resource_root = resources;
    environment.variables["XDG_CONFIG_HOME"] = config_home.string();

    flavor_tests::openrgb::ResourceManager manager;
    require(manager.InitializeResources(environment), "resource manager start should succeed");
    std::ofstream(manager.paths().config_dir / "OpenRGB.json") << "{\"old\": true}\n";

    const auto report = manager.SaveSettings("{\"client\": {\"start_minimized\": true}}\n");

    require(report.ok, "settings save should succeed");
    require(report.backup_path.has_value(), "settings save should keep a backup");
    require(manager.settingsManager().settings_json.find("start_minimized") != std::string::npos,
        "settings manager should retain the saved settings JSON");
    require(read_file(manager.paths().config_dir / "OpenRGB.json").find("client") != std::string::npos,
        "OpenRGB.json should contain the new settings");
    require(read_file(*report.backup_path).find("old") != std::string::npos,
        "backup should contain the previous settings JSON");
}

void controller_configuration_is_saved_under_profile_manager_root()
{
    const auto root = test_root();
    const auto resources = root / "share" / "OpenRGB";
    const auto config_home = root / "xdg-config";
    std::filesystem::create_directories(resources);

    flavor_tests::openrgb::RuntimeEnvironment environment;
    environment.resource_root = resources;
    environment.variables["XDG_CONFIG_HOME"] = config_home.string();

    flavor_tests::openrgb::ResourceManager manager;
    require(manager.InitializeResources(environment), "resource manager start should succeed");

    const auto report = manager.SaveConfiguration({
        {"Keyboard", "Underglow"},
        {"Mouse", "Logo"},
    });

    require(report.ok, "controller configuration save should succeed");
    const auto configuration = read_file(manager.paths().config_dir / "Configuration.json");
    require(configuration.find("Keyboard") != std::string::npos,
        "configuration should contain controller names");
    require(configuration.find("Underglow") != std::string::npos,
        "configuration should contain controller zones");
}

void linux_autostart_is_planned_as_a_desktop_effect()
{
    const auto root = test_root();
    const auto autostart_root = root / "autostart";

    const auto report = flavor_tests::openrgb::enable_autostart(
        root / "bin" / "OpenRGB",
        {"--startminimized", "--profile", "daily"},
        root,
        autostart_root);

    require(report.ok, "autostart planning should succeed");
    require(report.dry_run, "autostart should be dry-run in tests");
    require(report.path.has_value(), "autostart target should be reported");
    require(*report.path == autostart_root / "OpenRGB.desktop",
        "desktop file path should follow the override directory");
    require(!std::filesystem::exists(*report.path), "dry-run should not write the desktop file");
}

void linux_autostart_disable_is_the_same_product_seam()
{
    const auto root = test_root();
    const auto autostart_root = root / "autostart";

    const auto report = flavor_tests::openrgb::set_autostart_enabled(
        root / "bin" / "OpenRGB",
        {"--startminimized"},
        root,
        autostart_root,
        false);

    require(report.ok, "autostart disable planning should succeed");
    require(report.dry_run, "autostart disable should be dry-run in tests");
    require(!report.enabled, "autostart report should reflect the disabled product setting");
    require(report.path.has_value(), "disabled autostart target should still be reported");
    require(*report.path == autostart_root / "OpenRGB.desktop",
        "disabled autostart should use the same desktop file path");
}

} // namespace

int main()
{
    try {
        resource_manager_uses_xdg_config_for_settings_and_profiles();
        resource_manager_can_switch_configuration_directory_after_startup();
        settings_save_uses_validated_backup_write();
        controller_configuration_is_saved_under_profile_manager_root();
        linux_autostart_is_planned_as_a_desktop_effect();
        linux_autostart_disable_is_the_same_product_seam();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
