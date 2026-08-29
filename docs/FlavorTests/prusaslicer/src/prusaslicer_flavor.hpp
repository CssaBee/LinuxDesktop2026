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

struct PresetBundle {
    bool vendors_loaded = false;
    bool physical_printers_loaded = false;
    std::vector<std::string> loaded_files;
};

struct OldDatadirCheck {
    bool should_prompt_user = false;
    linuxdesktop::settings::migration_plan migration;
};

class PrusaConfigSnapshot {
public:
    bool load_config_bundle(const AppConfig& config);
    OldDatadirCheck check_old_linux_datadir(const AppConfig& config) const;

    linuxdesktop::settings::write_report save_snapshot(
        const Snapshot& snapshot,
        linuxdesktop::settings::validation_callback validate) const;
    linuxdesktop::settings::write_report save_app_config(const AppConfigStore& config) const;
    linuxdesktop::settings::write_report save_recent_projects(
        const std::filesystem::path& config_dir,
        const std::vector<RecentProject>& recent_projects) const;

    const PresetBundle& bundle() const { return bundle_; }
    const std::optional<linuxdesktop::settings::hydrate_report>& hydrateReport() const { return hydrate_report_; }

private:
    bool load_profile_file(const std::filesystem::path& path);

    PresetBundle bundle_;
    std::optional<linuxdesktop::settings::hydrate_report> hydrate_report_;
};

} // namespace flavor_tests::prusaslicer
