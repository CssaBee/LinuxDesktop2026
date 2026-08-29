#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::prusaslicer {

struct AppConfig {
    std::filesystem::path resources_dir;
    std::filesystem::path config_dir;
    std::filesystem::path old_linux_datadir;
    std::vector<linuxdesktop::settings::config_file> vendor_profiles;
};

struct Snapshot {
    std::filesystem::path path;
    std::string xml;
};

struct AppConfigStore {
    std::filesystem::path path;
    std::string ini;
};

struct RecentProject {
    std::filesystem::path path;
    std::string thumbnail_cache_key;
};

struct SaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
    std::optional<std::filesystem::path> temporary_file;
    bool durable = false;
};

struct BundleLoadResult {
    bool loaded = false;
    std::size_t copied_defaults = 0;
    std::size_t preserved_existing = 0;
};

struct PresetBundle {
    bool vendors_loaded = false;
    bool physical_printers_loaded = false;
    std::vector<std::string> loaded_files;
};

struct OldDatadirMigration {
    bool available = false;
    bool blocked = false;
    bool dry_run = true;
    std::filesystem::path old_datadir;
    std::filesystem::path config_dir;
    std::string action;
};

struct OldDatadirCheck {
    bool should_prompt_user = false;
    OldDatadirMigration migration;
};

class PrusaConfigSnapshot {
public:
    bool load_config_bundle(const AppConfig& config);
    OldDatadirCheck check_old_linux_datadir(const AppConfig& config) const;

    SaveResult save_snapshot(
        const Snapshot& snapshot,
        linuxdesktop::settings::validation_callback validate) const;
    SaveResult save_app_config(const AppConfigStore& config) const;
    SaveResult save_recent_projects(
        const std::filesystem::path& config_dir,
        const std::vector<RecentProject>& recent_projects) const;

    const PresetBundle& bundle() const { return bundle_; }
    const BundleLoadResult& lastLoadResult() const { return load_result_; }

private:
    bool load_profile_file(const std::filesystem::path& path);

    PresetBundle bundle_;
    BundleLoadResult load_result_;
};

} // namespace flavor_tests::prusaslicer
