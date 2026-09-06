#include "amiberry_flavor.hpp"

#include "amiberry/generated/platform_path_defaults.hpp"

#include "linuxdesktop/root.hpp"

#include <utility>

namespace flavor_tests::amiberry {

namespace {

std::optional<std::filesystem::path> env_path(
    const RuntimeEnvironment& environment,
    const std::string& name)
{
    const auto it = environment.environment.find(name);
    if (it == environment.environment.end() || it->second.empty()) {
        return std::nullopt;
    }
    return std::filesystem::path(it->second);
}

std::filesystem::path amiberry_home(const RuntimeEnvironment& environment)
{
    if (const auto override = env_path(environment, "AMIBERRY_HOME_DIR")) {
        return *override;
    }
    if (environment.home_directory) {
        return *environment.home_directory / "Amiberry";
    }
    return environment.executable_directory;
}

std::filesystem::path first_existing_or_first(const std::vector<std::filesystem::path>& candidates)
{
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.empty() ? std::filesystem::path{} : candidates.front();
}

} // namespace

RootTopology PathManager::resolve(const RuntimeEnvironment& environment, const PathOptions& options) const
{
    linuxdesktop::root::portable_root_request portable;
    portable.root = environment.executable_directory;
    portable.marker = environment.executable_directory / "amiberry.portable";
    portable.requested = options.portable_requested;
    portable.level = linuxdesktop::root::portable_root_level::profile;
    portable.deny_in_privileged_install = true;

    auto builder = linuxdesktop::root::request_builder()
        .app("BlitterStudio", "amiberry")
        .resource_root(environment.executable_directory)
        .home_directory(environment.home_directory)
        .environment(environment.environment)
        .use_process_environment(false)
        .portable_root(portable)
        .named_root(linuxdesktop::root::make_named_root_request(
            "whdboot",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::user_roaming,
            "whdboot"))
        .named_root(linuxdesktop::root::make_named_root_request(
            "controllers",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::user_roaming,
            "controllers"))
        .named_root(linuxdesktop::root::make_named_root_request(
            "savestates",
            linuxdesktop::root::purpose_kind::state,
            linuxdesktop::root::ownership_kind::user_local,
            "savestates"))
        .named_root(linuxdesktop::root::make_named_root_request(
            "screenshots",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::user_roaming,
            "screenshots"));

    if (environment.home_directory) {
        builder.platform_defaults(linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory));
    }

    const auto report = builder.resolve();
    const auto home_root = amiberry_home(environment);
    const auto content_root = options.base_content_path.value_or(home_root);

    RootTopology topology;
    topology.portable_requested = report.portable_root_requested;
    topology.portable_active = report.portable_root_active;
    topology.bootstrap_config_file = report.roots.config / "amiberry.conf";
    topology.bundled_data_root = environment.executable_directory;

    if (const auto data = env_path(environment, "AMIBERRY_DATA_DIR")) {
        if (std::filesystem::is_directory(*data)) {
            topology.bundled_data_root = *data;
            topology.data_root_override_active = true;
        } else {
            topology.diagnostics.push_back("AMIBERRY_DATA_DIR ignored because it does not point to an existing directory");
        }
    }

    if (const auto config = env_path(environment, "AMIBERRY_CONFIG_DIR")) {
        topology.configuration_files = *config;
    } else if (topology.portable_active) {
        topology.configuration_files = environment.executable_directory / "conf";
    } else if (options.base_content_path) {
        topology.configuration_files = content_root / "conf";
    } else {
        topology.configuration_files = home_root / "conf";
    }

    topology.plugins = env_path(environment, "AMIBERRY_PLUGINS_DIR").value_or(first_existing_or_first({
        environment.install_library_directory / "amiberry",
        home_root / "plugins",
        environment.executable_directory / "plugins",
    }));

    topology.whdboot = content_root / "whdboot";
    topology.controllers = content_root / "controllers";
    topology.roms = content_root / "roms";
    topology.savestates = content_root / "savestates";
    topology.screenshots = content_root / "screenshots";
    topology.log_file = home_root / "amiberry.log";

    if (const auto* whdboot = linuxdesktop::root::find_named_root(report, "whdboot");
        whdboot && !topology.portable_active && !options.base_content_path) {
        topology.whdboot = whdboot->path;
    }
    if (const auto* controllers = linuxdesktop::root::find_named_root(report, "controllers");
        controllers && !topology.portable_active && !options.base_content_path) {
        topology.controllers = controllers->path;
    }
    if (const auto* savestates = linuxdesktop::root::find_named_root(report, "savestates");
        savestates && !topology.portable_active && !options.base_content_path) {
        topology.savestates = savestates->path;
    }
    if (const auto* screenshots = linuxdesktop::root::find_named_root(report, "screenshots");
        screenshots && !topology.portable_active && !options.base_content_path) {
        topology.screenshots = screenshots->path;
    }

    topology.plugin_search_roots = {
        topology.plugins,
        home_root / "plugins",
        topology.bundled_data_root / "plugins",
    };
    return topology;
}

} // namespace flavor_tests::amiberry
