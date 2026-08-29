#include "prusaslicer_flavor.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace flavor_tests::prusaslicer {

namespace ld = linuxdesktop::settings;
namespace ldm = linuxdesktop::migration;

namespace {

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string escape_ini_value(std::string value)
{
    for (char& ch : value) {
        if (ch == '\n' || ch == '\r') {
            ch = ' ';
        }
    }
    return value;
}

std::string render_recent_projects(const std::vector<RecentProject>& recent_projects)
{
    std::ostringstream output;
    output << "[recent_projects]\n";
    for (size_t index = 0; index < recent_projects.size(); ++index) {
        output << "project_" << index << "=" << escape_ini_value(recent_projects[index].path.string()) << "\n";
        output << "thumbnail_" << index << "=" << escape_ini_value(recent_projects[index].thumbnail_cache_key) << "\n";
    }
    return output.str();
}

bool directory_has_user_content(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) {
        return false;
    }

    return std::filesystem::directory_iterator(path, ec) != std::filesystem::directory_iterator();
}

SaveResult to_save_result(const ld::write_report& report)
{
    return {report.ok, report.backup_path, report.temp_path, report.durable_write};
}

PlannedDirectoryCopy to_planned_directory_copy(const ldm::migration_plan& plan)
{
    PlannedDirectoryCopy result;
    result.dry_run = plan.dry_run;
    if (!plan.actions.empty()) {
        result.planned = true;
        result.source = plan.actions.front().source_path;
        result.target = plan.actions.front().target_path;
    }
    return result;
}

} // namespace

bool PrusaConfigSnapshot::load_config_bundle(const AppConfig& config)
{
    ld::hydrate_options hydrate;
    hydrate.model_root = config.resources_dir;
    hydrate.target_root = config.config_dir;
    hydrate.files = config.vendor_profiles;
    const auto hydrate_report = ld::hydrate_config_bundle(hydrate);
    load_result_ = {
        false,
        hydrate_report.copied.size(),
        hydrate_report.skipped_existing.size(),
    };

    bool loaded = true;
    for (const auto& profile : config.vendor_profiles) {
        loaded = load_profile_file(config.config_dir / profile.name) && loaded;
    }
    load_result_.loaded = loaded;
    return loaded;
}

OldDatadirCheck PrusaConfigSnapshot::check_old_linux_datadir(const AppConfig& config) const
{
    OldDatadirCheck check;
    if (config.old_linux_datadir.empty() || config.old_linux_datadir == config.config_dir) {
        return check;
    }

    const bool new_datadir_empty = !directory_has_user_content(config.config_dir);
    const bool old_datadir_present = directory_has_user_content(config.old_linux_datadir);
    check.should_prompt_user = new_datadir_empty && old_datadir_present;
    if (!check.should_prompt_user) {
        return check;
    }

    ldm::migration_action action;
    action.kind = ldm::migration_action_kind::copy_directory;
    action.name = "Copy legacy PrusaSlicer datadir into the XDG datadir";
    action.source_path = config.old_linux_datadir;
    action.target_path = config.config_dir;

    ldm::options options;
    options.dry_run = true;
    options.overwrite_existing = false;
    check.migration = to_planned_directory_copy(ldm::plan_migration({action}, options));
    return check;
}

SaveResult PrusaConfigSnapshot::save_snapshot(
    const Snapshot& snapshot,
    ld::validation_callback validate) const
{
    return to_save_result(ld::write_common_config({snapshot.path, snapshot.xml, true}, std::move(validate)));
}

SaveResult PrusaConfigSnapshot::save_app_config(const AppConfigStore& config) const
{
    return to_save_result(ld::write_common_config({config.path, config.ini, true},
        [](const std::filesystem::path& path, std::string& message) {
        const std::string bytes = read_text(path);
        const bool ok = bytes.find("[app]") != std::string::npos || bytes.find("[recent_projects]") != std::string::npos;
        if (!ok) {
            message = "PrusaSlicer config did not contain a known section";
        }
        return ok;
    }));
}

SaveResult PrusaConfigSnapshot::save_recent_projects(
    const std::filesystem::path& config_dir,
    const std::vector<RecentProject>& recent_projects) const
{
    return save_app_config({config_dir / "PrusaSlicer.ini", render_recent_projects(recent_projects)});
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
