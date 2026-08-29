#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace flavor_tests::qbittorrent {

enum class SpecialFolder {
    Config,
    Data,
    FastResume,
    Logs
};

struct CommandLineArgs {
    std::optional<std::filesystem::path> profile_dir;
    std::string configuration_name;
    bool relative_fastresume_paths = false;
};

struct RuntimeEnvironment {
    std::filesystem::path executable_dir;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

class Profile {
public:
    bool init(const RuntimeEnvironment& environment, const CommandLineArgs& args);
    std::filesystem::path location(SpecialFolder folder) const;
    linuxdesktop::settings::write_report saveFileLoggerSettings(std::string content) const;

    bool portableModeEnabled() const { return portable_mode_enabled_; }
    bool relativeFastresumePaths() const { return relative_fastresume_paths_; }
    const linuxdesktop::settings::root_report& report() const { return report_; }

private:
    std::string configurationSuffix() const;
    std::filesystem::path profile_root_;
    std::filesystem::path data_root_;
    std::filesystem::path fastresume_root_;
    std::filesystem::path logs_root_;
    std::string configuration_name_;
    bool portable_mode_enabled_ = false;
    bool relative_fastresume_paths_ = false;
    linuxdesktop::settings::root_report report_;
};

} // namespace flavor_tests::qbittorrent
