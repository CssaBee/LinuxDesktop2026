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
    std::vector<linuxdesktop::settings::config_file> vendor_profiles;
};

struct Snapshot {
    std::filesystem::path path;
    std::string xml;
};

struct PresetBundle {
    bool vendors_loaded = false;
    bool physical_printers_loaded = false;
    std::vector<std::string> loaded_files;
};

class PrusaConfigSnapshot {
public:
    bool load_config_bundle(const AppConfig& config);

    linuxdesktop::settings::write_report save_snapshot(
        const Snapshot& snapshot,
        linuxdesktop::settings::validation_callback validate) const;

    const PresetBundle& bundle() const { return bundle_; }
    const std::optional<linuxdesktop::settings::hydrate_report>& hydrateReport() const { return hydrate_report_; }

private:
    bool load_profile_file(const std::filesystem::path& path);

    PresetBundle bundle_;
    std::optional<linuxdesktop::settings::hydrate_report> hydrate_report_;
};

} // namespace flavor_tests::prusaslicer
