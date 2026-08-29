#include "openrgb_flavor.hpp"

#include "linuxdesktop/paths.hpp"

#include <filesystem>
#include <iostream>
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

} // namespace

int main()
{
    try {
        resource_manager_uses_xdg_config_for_settings_and_profiles();
        linux_autostart_is_planned_as_a_desktop_effect();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
