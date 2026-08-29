#pragma once

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::freecad {

struct CommandLineOptions {
    std::optional<std::filesystem::path> user_cfg;
    std::optional<std::filesystem::path> system_cfg;
    std::vector<std::filesystem::path> module_paths;
    bool keep_deprecated_paths = false;
};

struct RuntimeEnvironment {
    std::filesystem::path app_home;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

struct DeprecatedPathMigration {
    bool available = false;
    bool blocked = false;
    bool should_prompt_user = false;
    bool dry_run = true;
    std::filesystem::path deprecated_user_data;
    std::filesystem::path user_app_data;
    std::string action;
};

struct ConfigurationSet {
    std::filesystem::path user_home_path;
    std::filesystem::path user_app_data;
    std::filesystem::path app_temp_path;
    std::filesystem::path user_parameter;
    std::filesystem::path system_parameter;
    std::vector<std::filesystem::path> python_search_path;
    DeprecatedPathMigration deprecated_path_migration;
};

struct SaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
};

class ApplicationConfig {
public:
    ConfigurationSet build(const RuntimeEnvironment& environment, const CommandLineOptions& options) const;
    SaveResult saveUserParameter(
        const ConfigurationSet& config,
        std::string xml) const;
};

} // namespace flavor_tests::freecad
