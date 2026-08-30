#pragma once

#include "linuxdesktop/paths.hpp"

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>

namespace flavor_tests::support {

struct fake_user_environment {
    std::filesystem::path home;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path state;
    std::filesystem::path cache;
    std::filesystem::path runtime;
    std::filesystem::path appdata_roaming;
    std::filesystem::path appdata_local;
    std::map<std::string, std::string> variables;
};

inline fake_user_environment make_fake_user_environment(const std::filesystem::path& root)
{
    fake_user_environment environment;
    environment.home = root / "home" / "alice";
    environment.config = environment.home / ".config";
    environment.data = environment.home / ".local" / "share";
    environment.state = environment.home / ".local" / "state";
    environment.cache = environment.home / ".cache";
    environment.runtime = root / "run" / "user" / "1000";
    environment.appdata_roaming = root / "appdata" / "roaming";
    environment.appdata_local = root / "appdata" / "local";
    environment.variables = {
        {"XDG_CONFIG_HOME", environment.config.string()},
        {"XDG_DATA_HOME", environment.data.string()},
        {"XDG_STATE_HOME", environment.state.string()},
        {"XDG_CACHE_HOME", environment.cache.string()},
        {"XDG_RUNTIME_DIR", environment.runtime.string()},
        {"APPDATA", environment.appdata_roaming.string()},
        {"LOCALAPPDATA", environment.appdata_local.string()},
    };
    return environment;
}

inline std::filesystem::path resolved_user_root(
    const linuxdesktop::paths::app_identity& identity,
    const fake_user_environment& environment,
    linuxdesktop::paths::path_family family,
    const std::filesystem::path& executable_path = {})
{
    linuxdesktop::paths::resolver_options options;
    options.home_directory = environment.home;
    options.executable_path = executable_path;
    options.environment = environment.variables;
    options.use_process_environment = false;

    const auto report = linuxdesktop::paths::resolve_app_paths(identity, options);
    const auto found = report.selected.find(family);
    if (found == report.selected.end()) {
        throw std::runtime_error("ld_paths did not select the requested user root");
    }
    return found->second;
}

} // namespace flavor_tests::support
