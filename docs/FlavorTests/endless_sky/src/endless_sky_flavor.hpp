#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::endless_sky {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::filesystem::path executable_directory;
    std::filesystem::path resource_directory;
    std::map<std::string, std::string> environment;
};

struct PluginRoot {
    std::string name;
    std::filesystem::path path;
    bool user_writable = false;
};

struct StartupPaths {
    std::filesystem::path config_root;
    std::filesystem::path save_root;
    std::filesystem::path preferences_file;
    std::vector<PluginRoot> plugin_roots;
    std::filesystem::path downloadable_plugin_root;
    std::filesystem::path plugin_errors_file;
};

class Files {
public:
    StartupPaths initialize(const RuntimeEnvironment& environment) const;
};

} // namespace flavor_tests::endless_sky
