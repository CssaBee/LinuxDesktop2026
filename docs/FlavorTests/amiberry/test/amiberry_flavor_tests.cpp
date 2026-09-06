#include "amiberry_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace amiberry = flavor_tests::amiberry;

namespace {

int failures = 0;

enum class root_kind {
    config,
    data
};

void expect(bool condition, const std::string& name)
{
    if (condition) {
        std::cout << "ok " << name << '\n';
    } else {
        std::cout << "not ok " << name << '\n';
        ++failures;
    }
}

amiberry::RuntimeEnvironment default_env(const std::string& name)
{
    const auto root = std::filesystem::temp_directory_path() / ("linuxdesktop2026-" + name);
    std::filesystem::remove_all(root);
    amiberry::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    env.executable_directory = root / "opt" / "amiberry";
    env.install_library_directory = root / "usr" / "lib";
    return env;
}

std::filesystem::path platform_root(const amiberry::RuntimeEnvironment& env, root_kind kind)
{
    const auto& home = *env.home_directory;
    switch (kind) {
    case root_kind::config:
#if defined(_WIN32)
        return home / "AppData" / "Roaming" / "BlitterStudio" / "amiberry";
#else
        return home / ".config" / "BlitterStudio" / "amiberry";
#endif
    case root_kind::data:
#if defined(_WIN32)
        return home / "AppData" / "Roaming" / "BlitterStudio" / "amiberry";
#else
        return home / ".local" / "share" / "BlitterStudio" / "amiberry";
#endif
    }
    return {};
}

void linux_split_layout_keeps_bootstrap_config_in_xdg_and_content_under_data_or_home()
{
    const auto env = default_env("amiberry-split");

    const auto topology = amiberry::PathManager{}.resolve(env, {});

    expect(topology.bootstrap_config_file == platform_root(env, root_kind::config) / "amiberry.conf",
        "bootstrap config uses platform config root");
    expect(topology.whdboot == platform_root(env, root_kind::data) / "whdboot",
        "whdboot follows platform data root");
    expect(topology.controllers == platform_root(env, root_kind::data) / "controllers",
        "controllers follow platform data root");
    expect(topology.roms == *env.home_directory / "Amiberry" / "roms",
        "ROMs remain under Amiberry home content");
    expect(topology.configuration_files == *env.home_directory / "Amiberry" / "conf",
        "configuration files keep Amiberry conf leaf");
}

void base_content_path_becomes_authoritative_for_managed_directories()
{
    const auto env = default_env("amiberry-base-content");
    amiberry::PathOptions options;
    options.base_content_path = *env.home_directory / "Emulation" / "Amiberry";

    const auto topology = amiberry::PathManager{}.resolve(env, options);

    expect(topology.configuration_files == *options.base_content_path / "conf",
        "base content path moves configuration files");
    expect(topology.whdboot == *options.base_content_path / "whdboot",
        "base content path moves whdboot");
    expect(topology.savestates == *options.base_content_path / "savestates",
        "base content path moves savestates");
    expect(topology.log_file == *env.home_directory / "Amiberry" / "amiberry.log",
        "base content path does not move bootstrap log policy");
}

void environment_overrides_config_and_plugin_roots()
{
    auto env = default_env("amiberry-env");
    const auto data_root = *env.home_directory / "BundledData";
    std::filesystem::create_directories(data_root);
    env.environment["AMIBERRY_HOME_DIR"] = (*env.home_directory / "Retro" / "Amiga").string();
    env.environment["AMIBERRY_DATA_DIR"] = data_root.string();
    env.environment["AMIBERRY_CONFIG_DIR"] = (*env.home_directory / "Configs").string();
    env.environment["AMIBERRY_PLUGINS_DIR"] = (*env.home_directory / "PluginLibs").string();

    const auto topology = amiberry::PathManager{}.resolve(env, {});

    expect(topology.data_root_override_active, "existing AMIBERRY_DATA_DIR is selected");
    expect(topology.bundled_data_root == data_root, "AMIBERRY_DATA_DIR selects bundled data root");
    expect(topology.configuration_files == *env.home_directory / "Configs",
        "AMIBERRY_CONFIG_DIR overrides configuration files");
    expect(topology.plugins == *env.home_directory / "PluginLibs",
        "AMIBERRY_PLUGINS_DIR overrides plugin root");
    expect(topology.roms == *env.home_directory / "Retro" / "Amiga" / "roms",
        "AMIBERRY_HOME_DIR changes home content roots");
}

void missing_data_override_is_diagnosed_without_becoming_selected()
{
    auto env = default_env("amiberry-missing-data");
    env.environment["AMIBERRY_DATA_DIR"] = (*env.home_directory / "MissingData").string();

    const auto topology = amiberry::PathManager{}.resolve(env, {});

    expect(!topology.data_root_override_active, "missing AMIBERRY_DATA_DIR is not selected");
    expect(topology.bundled_data_root == env.executable_directory,
        "missing AMIBERRY_DATA_DIR falls back to executable data root");
    expect(topology.diagnostics.size() == 1, "missing AMIBERRY_DATA_DIR is diagnosed");
}

void portable_marker_moves_writable_topology_beside_executable()
{
    auto env = default_env("amiberry-portable");
    std::filesystem::create_directories(env.executable_directory);
    std::ofstream(env.executable_directory / "amiberry.portable");

    const auto topology = amiberry::PathManager{}.resolve(env, {});

    expect(topology.portable_requested, "portable marker requests portable topology");
    expect(topology.portable_active, "portable marker activates portable topology");
    expect(topology.bootstrap_config_file == env.executable_directory / "amiberry.conf",
        "portable bootstrap config stays executable-adjacent");
    expect(topology.configuration_files == env.executable_directory / "conf",
        "portable configuration files stay executable-adjacent");
}

} // namespace

int main()
{
    linux_split_layout_keeps_bootstrap_config_in_xdg_and_content_under_data_or_home();
    base_content_path_becomes_authoritative_for_managed_directories();
    environment_overrides_config_and_plugin_roots();
    missing_data_override_is_diagnosed_without_becoming_selected();
    portable_marker_moves_writable_topology_beside_executable();
    return failures == 0 ? 0 : 1;
}
