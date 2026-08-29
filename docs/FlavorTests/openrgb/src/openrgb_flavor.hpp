#pragma once

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::openrgb {

struct SettingsManager {
    std::filesystem::path loaded_settings_file;

    void LoadSettings(const std::filesystem::path& path) { loaded_settings_file = path; }
};

struct LogManager {
    std::filesystem::path configured_directory;

    void Configure(const std::filesystem::path& directory) { configured_directory = directory; }
};

struct ProfileManager {
    std::filesystem::path configuration_directory;
};

struct RuntimeEnvironment {
    std::filesystem::path resource_root;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

struct RuntimePaths {
    std::filesystem::path config_dir;
    std::filesystem::path profiles_dir;
    std::filesystem::path resources_dir;
};

class ResourceManager {
public:
    ResourceManager();

    bool InitializeResources(const RuntimeEnvironment& environment);

    const RuntimePaths& paths() const { return paths_; }
    const linuxdesktop::paths::resolver_report& report() const { return report_; }
    const SettingsManager& settingsManager() const { return settings_manager_; }
    const LogManager& logManager() const { return log_manager_; }
    const ProfileManager& profileManager() const { return profile_manager_; }

private:
    bool SetupConfigurationDirectory(const RuntimeEnvironment& environment);

    RuntimePaths paths_;
    SettingsManager settings_manager_;
    LogManager log_manager_;
    ProfileManager profile_manager_;
    linuxdesktop::paths::resolver_report report_;
    bool detection_enabled_ = true;
    bool first_detection_complete_ = false;
    bool init_finished_ = false;
};

linuxdesktop::settings::effects::effect_report enable_autostart(
    const std::filesystem::path& executable,
    std::vector<std::string> arguments,
    const std::filesystem::path& working_directory,
    const std::filesystem::path& override_directory);

} // namespace flavor_tests::openrgb
