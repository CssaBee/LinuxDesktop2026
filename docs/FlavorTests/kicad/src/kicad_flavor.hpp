#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::kicad {

enum class SettingsLocation {
    User,
    Project,
    Colors,
    Toolbars
};

enum class BackupLocation {
    ProjectDir,
    UserDir
};

struct Project {
    std::filesystem::path full_name;
    std::string name;
};

struct JsonSettings {
    std::string file_name;
    SettingsLocation location = SettingsLocation::User;
    std::string json = "{}";
    const Project* owning_project = nullptr;
};

struct SaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
};

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

class SETTINGS_MANAGER {
public:
    explicit SETTINGS_MANAGER(RuntimeEnvironment environment);

    bool SettingsDirectoryValid() const;
    std::filesystem::path GetPathForSettingsFile(const JsonSettings& settings) const;
    std::filesystem::path GetToolbarSettingsPath() const;
    std::filesystem::path GetBackupRootForProject(const Project* project = nullptr) const;
    SaveResult Save(const JsonSettings& settings) const;

    void SetProject(Project project) { project_ = std::move(project); }
    void SetBackupLocation(BackupLocation location) { backup_location_ = location; }

private:
    const Project& resolveProject(const Project* project) const;
    std::string projectKeySuffix(const Project& project) const;

    RuntimeEnvironment environment_;
    Project project_;
    BackupLocation backup_location_ = BackupLocation::ProjectDir;
    std::filesystem::path user_settings_root_;
    std::filesystem::path state_root_;
    std::filesystem::path color_settings_root_;
    std::filesystem::path toolbar_settings_root_;
    std::filesystem::path user_backup_root_;
};

} // namespace flavor_tests::kicad
