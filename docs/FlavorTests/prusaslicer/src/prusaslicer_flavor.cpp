#include "prusaslicer_flavor.hpp"

#include <fstream>
#include <iterator>
#include <utility>

namespace flavor_tests::prusaslicer {

namespace ld = linuxdesktop::settings;

bool PrusaConfigSnapshot::load_config_bundle(const AppConfig& config)
{
    ld::hydrate_options hydrate;
    hydrate.model_root = config.resources_dir;
    hydrate.target_root = config.config_dir;
    hydrate.files = config.vendor_profiles;
    hydrate_report_ = ld::hydrate_config_bundle(hydrate);

    bool loaded = true;
    for (const auto& profile : config.vendor_profiles) {
        loaded = load_profile_file(config.config_dir / profile.name) && loaded;
    }
    return loaded;
}

ld::write_report PrusaConfigSnapshot::save_snapshot(
    const Snapshot& snapshot,
    ld::validation_callback validate) const
{
    ld::write_options write;
    write.target = snapshot.path;
    write.content = snapshot.xml;
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    return ld::write_with_backup(write, std::move(validate));
}

bool PrusaConfigSnapshot::load_profile_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    const std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    bundle_.loaded_files.push_back(path.filename().string());
    if (path.filename() == "physical_printers.ini") {
        bundle_.physical_printers_loaded = !bytes.empty();
    } else {
        bundle_.vendors_loaded = !bytes.empty() || bundle_.vendors_loaded;
    }
    return !bytes.empty();
}

} // namespace flavor_tests::prusaslicer
