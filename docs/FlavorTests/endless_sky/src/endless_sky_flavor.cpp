#include "endless_sky_flavor.hpp"

#include "endless_sky/generated/platform_path_defaults.hpp"

#include "linuxdesktop/root.hpp"

namespace flavor_tests::endless_sky {

StartupPaths Files::initialize(const RuntimeEnvironment& environment) const
{
    const auto bundled_plugins_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "bundled-plugins",
            linuxdesktop::root::purpose_kind::resources,
            linuxdesktop::root::ownership_kind::user_roaming,
            "plugins"));
    const auto save_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "save",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::user_roaming,
            "saves"));
    const auto local_plugins_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "local-plugins",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::user_roaming,
            "plugins"));

    auto builder = linuxdesktop::root::request_builder()
        .app("endless-sky", "endless-sky")
        .resource_root(environment.resource_directory)
        .home_directory(environment.home_directory)
        .environment(environment.environment)
        .use_process_environment(false)
        .named_root(bundled_plugins_root)
        .named_root(save_root)
        .named_root(local_plugins_root);

    if (environment.home_directory) {
        builder.platform_defaults(linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory));
    }

    const auto report = builder.resolve();

    StartupPaths paths;
    paths.config_root = report.roots.data;
    paths.save_root = paths.config_root / "saves";
    paths.preferences_file = paths.config_root / "preferences.txt";
    paths.downloadable_plugin_root = paths.config_root / "plugins";
    paths.plugin_errors_file = paths.downloadable_plugin_root / "errors.txt";

    if (const auto* save = linuxdesktop::root::find_named_root(report, save_root)) {
        paths.save_root = save->path;
    }
    if (const auto* bundled = linuxdesktop::root::find_named_root(report, bundled_plugins_root)) {
        paths.plugin_roots.push_back({"bundled", bundled->path, false});
    }
    if (const auto* local = linuxdesktop::root::find_named_root(report, local_plugins_root)) {
        paths.plugin_roots.push_back({"local", local->path, true});
        paths.downloadable_plugin_root = local->path;
        paths.plugin_errors_file = local->path / "errors.txt";
    }

    return paths;
}

} // namespace flavor_tests::endless_sky
