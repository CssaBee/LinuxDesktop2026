#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

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

struct SaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
};

enum class DesktopRegistrationMode {
    Plan,
    Apply,
    Query,
    Remove
};

enum class DesktopRegistrationStatus {
    Planned,
    Staged,
    Present,
    Missing,
    Unsupported,
    Failed
};

struct DesktopFlavorScenario {
    std::string name;
    std::string current_desktop;
    bool windows_shaped = false;
};

struct DesktopRegistrationRequest {
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::filesystem::path icon_source;
    std::filesystem::path staging_root;
    bool register_autostart = false;
    bool make_torrent_default = false;
    bool register_magnet_links = true;
    bool include_policy_diagnostic = false;
};

struct DesktopRegistrationResult {
    bool ok = false;
    bool dry_run = true;
    bool desktop_entry_ready = false;
    bool torrent_mime_ready = false;
    bool magnet_handler_ready = false;
    bool has_cleanup_reports = false;
    bool has_desktop_database_activation = false;
    bool has_mime_database_activation = false;
    bool has_icon_cache_activation = false;
    bool has_dconf_activation_diagnostic = false;
    bool has_windows_shell_activation = false;
    bool has_windows_default_apps_follow_up = false;
    std::size_t artifact_count = 0;
    std::size_t cleanup_count = 0;
    std::size_t unsupported_count = 0;
    std::vector<DesktopRegistrationStatus> statuses;
    std::vector<std::string> diagnostic_codes;
};

class Profile {
public:
    bool init(const RuntimeEnvironment& environment, const CommandLineArgs& args);
    std::filesystem::path location(SpecialFolder folder) const;
    SaveResult saveFileLoggerSettings(std::string content) const;

    bool portableModeEnabled() const { return portable_mode_enabled_; }
    bool relativeFastresumePaths() const { return relative_fastresume_paths_; }

private:
    std::string configurationSuffix() const;
    std::filesystem::path profile_root_;
    std::filesystem::path data_root_;
    std::filesystem::path fastresume_root_;
    std::filesystem::path logs_root_;
    std::string configuration_name_;
    bool portable_mode_enabled_ = false;
    bool relative_fastresume_paths_ = false;
};

std::vector<DesktopFlavorScenario> desktop_flavor_scenarios();
DesktopRegistrationResult run_desktop_registration(
    const DesktopRegistrationRequest& request,
    const DesktopFlavorScenario& scenario,
    DesktopRegistrationMode mode);

} // namespace flavor_tests::qbittorrent
