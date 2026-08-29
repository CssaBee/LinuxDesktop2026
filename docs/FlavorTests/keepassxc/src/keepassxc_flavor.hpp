#pragma once

#include "linuxdesktop/migration.hpp"
#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace flavor_tests::keepassxc {

struct RuntimeEnvironment {
    std::filesystem::path app_dir;
    std::optional<std::filesystem::path> portable_config_dir;
    std::optional<std::filesystem::path> old_cache_config_file;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

struct ConfigFiles {
    std::filesystem::path roaming;
    std::filesystem::path local;
    bool portable = false;
};

struct ExportResult {
    bool exported = false;
    std::optional<std::filesystem::path> backup_file;
};

class Config {
public:
    bool open(const RuntimeEnvironment& environment);
    bool importSettings(const std::filesystem::path& file_name);
    ExportResult exportSettings(const std::filesystem::path& file_name) const;
    linuxdesktop::migration::migration_plan migrateOldLocalConfig(const RuntimeEnvironment& environment) const;

    void set(std::string key, std::string value, bool local = false);
    std::optional<std::string> get(const std::string& key) const;
    const ConfigFiles& files() const { return files_; }

private:
    ConfigFiles files_;
    std::map<std::string, std::string> roaming_values_;
    std::map<std::string, std::string> local_values_;
};

} // namespace flavor_tests::keepassxc
