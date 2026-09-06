#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::amiberry {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::filesystem::path executable_directory;
    std::filesystem::path install_library_directory;
    std::map<std::string, std::string> environment;
};

struct PathOptions {
    std::optional<std::filesystem::path> base_content_path;
    bool portable_requested = false;
};

struct RootTopology {
    bool portable_requested = false;
    bool portable_active = false;
    bool data_root_override_active = false;
    std::filesystem::path bootstrap_config_file;
    std::filesystem::path bundled_data_root;
    std::filesystem::path configuration_files;
    std::filesystem::path plugins;
    std::filesystem::path whdboot;
    std::filesystem::path controllers;
    std::filesystem::path roms;
    std::filesystem::path savestates;
    std::filesystem::path screenshots;
    std::filesystem::path log_file;
    std::vector<std::filesystem::path> plugin_search_roots;
    std::vector<std::string> diagnostics;
};

class PathManager {
public:
    RootTopology resolve(const RuntimeEnvironment& environment, const PathOptions& options) const;
};

} // namespace flavor_tests::amiberry
