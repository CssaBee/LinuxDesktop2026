#include "prusaslicer_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

struct test_failure : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw test_failure(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-prusaslicer-flavor";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw test_failure("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void first_run_copies_missing_vendor_profile_models()
{
    const auto root = test_root();
    const auto resources = root / "resources";
    const auto config = root / "config";
    std::filesystem::create_directories(resources);
    std::filesystem::create_directories(config);

    {
        std::ofstream(resources / "PrusaResearch.ini.model") << "vendor=PrusaResearch\n";
        std::ofstream(resources / "physical_printers.ini.model") << "printers=[]\n";
        std::ofstream(config / "physical_printers.ini") << "printers=[MK4]\n";
    }

    flavor_tests::prusaslicer::PrusaConfigSnapshot snapshot;
    flavor_tests::prusaslicer::AppConfig app_config;
    app_config.resources_dir = resources;
    app_config.config_dir = config;
    app_config.vendor_profiles = {
        {"PrusaResearch.ini", "PrusaResearch.ini.model", true},
        {"physical_printers.ini", "physical_printers.ini.model", true},
    };

    require(snapshot.load_config_bundle(app_config), "profile bundle should load");
    const auto& report = *snapshot.hydrateReport();
    require(report.copied.size() == 1, "one missing vendor profile should be copied");
    require(report.skipped_existing.size() == 1, "existing physical printer config should be kept");
    require(snapshot.bundle().vendors_loaded, "vendor profiles should be loaded into the bundle");
    require(snapshot.bundle().physical_printers_loaded, "physical printer profiles should be loaded into the bundle");
    require(snapshot.bundle().loaded_files.size() == 2, "both profile files should pass through app-owned loading");
    require(read_file(config / "PrusaResearch.ini").find("PrusaResearch") != std::string::npos,
        "copied vendor profile should contain model content");
    require(read_file(config / "physical_printers.ini").find("MK4") != std::string::npos,
        "existing user config should stay intact");
}

void snapshot_save_keeps_backup_and_validates_new_file()
{
    const auto root = test_root();
    const auto target = root / "snapshots" / "snapshot.xml";
    std::filesystem::create_directories(target.parent_path());
    std::ofstream(target) << "<snapshot version=\"old\" />\n";

    flavor_tests::prusaslicer::PrusaConfigSnapshot writer;
    flavor_tests::prusaslicer::Snapshot snapshot;
    snapshot.path = target;
    snapshot.xml = "<snapshot version=\"new\" />\n";

    const auto report = writer.save_snapshot(snapshot,
        [](const std::filesystem::path& path, std::string&) {
            return read_file(path).find("version=\"new\"") != std::string::npos;
        });

    require(report.ok, "snapshot save should succeed");
    require(report.backup_path.has_value(), "previous snapshot should be backed up");
    require(report.temp_path.has_value(), "temp file should be reported");
    require(report.durable_write, "snapshot writes should request durable mode");
    require(read_file(target).find("version=\"new\"") != std::string::npos,
        "target should contain the new snapshot");
    require(read_file(*report.backup_path).find("version=\"old\"") != std::string::npos,
        "backup should contain the old snapshot");
}

void old_linux_datadir_plans_migration_when_new_datadir_is_empty()
{
    const auto root = test_root();
    const auto resources = root / "resources";
    const auto config = root / "xdg" / "PrusaSlicer";
    const auto old_config = root / ".PrusaSlicer";
    std::filesystem::create_directories(resources);
    std::filesystem::create_directories(config);
    std::filesystem::create_directories(old_config);
    std::ofstream(old_config / "PrusaSlicer.ini") << "[app]\nlegacy=true\n";

    flavor_tests::prusaslicer::AppConfig app_config;
    app_config.resources_dir = resources;
    app_config.config_dir = config;
    app_config.old_linux_datadir = old_config;

    const flavor_tests::prusaslicer::PrusaConfigSnapshot snapshot;
    const auto check = snapshot.check_old_linux_datadir(app_config);

    require(check.should_prompt_user, "legacy datadir check should ask before silently ignoring old data");
    require(check.migration.dry_run, "legacy datadir migration should be planned as a dry run");
    require(check.migration.actions.size() == 1, "one directory copy should be planned");
    require(check.migration.actions.front().source_path == old_config,
        "migration should copy from the legacy wx datadir");
    require(check.migration.actions.front().target_path == config,
        "migration should target the XDG datadir");
}

void old_linux_datadir_check_stays_silent_when_new_datadir_has_content()
{
    const auto root = test_root();
    const auto resources = root / "resources";
    const auto config = root / "xdg" / "PrusaSlicer";
    const auto old_config = root / ".PrusaSlicer";
    std::filesystem::create_directories(resources);
    std::filesystem::create_directories(config);
    std::filesystem::create_directories(old_config);
    std::ofstream(config / "PrusaSlicer.ini") << "[app]\nnew=true\n";
    std::ofstream(old_config / "PrusaSlicer.ini") << "[app]\nlegacy=true\n";

    flavor_tests::prusaslicer::AppConfig app_config;
    app_config.resources_dir = resources;
    app_config.config_dir = config;
    app_config.old_linux_datadir = old_config;

    const flavor_tests::prusaslicer::PrusaConfigSnapshot snapshot;
    const auto check = snapshot.check_old_linux_datadir(app_config);

    require(!check.should_prompt_user, "pop-up should stay silent once the new datadir has user content");
    require(check.migration.actions.empty(), "no migration action should be planned");
}

void app_config_save_uses_backup_write_and_validates_sections()
{
    const auto root = test_root();
    const auto target = root / "config" / "PrusaSlicer.ini";
    std::filesystem::create_directories(target.parent_path());
    std::ofstream(target) << "[app]\nversion=old\n";

    const flavor_tests::prusaslicer::PrusaConfigSnapshot snapshot;
    const auto report = snapshot.save_app_config({target, "[app]\nversion=new\n"});

    require(report.ok, "app config save should succeed");
    require(report.backup_path.has_value(), "app config save should keep a backup");
    require(read_file(target).find("version=new") != std::string::npos,
        "config should contain the new app settings");
    require(read_file(*report.backup_path).find("version=old") != std::string::npos,
        "backup should contain the old app settings");
}

void recent_projects_are_written_to_prusaslicer_ini()
{
    const auto root = test_root();
    const auto config = root / "config";
    std::filesystem::create_directories(config);
    std::ofstream(config / "PrusaSlicer.ini") << "[app]\nversion=old\n";

    const flavor_tests::prusaslicer::PrusaConfigSnapshot snapshot;
    const auto report = snapshot.save_recent_projects(config, {
        {root / "models" / "case.3mf", "case-thumb"},
        {root / "models" / "fixture.stl", "fixture-thumb"},
    });

    require(report.ok, "recent project save should succeed");
    const auto config_text = read_file(config / "PrusaSlicer.ini");
    require(config_text.find("[recent_projects]") != std::string::npos,
        "recent projects section should be persisted");
    require(config_text.find("case-thumb") != std::string::npos,
        "thumbnail cache keys should be preserved");
    require(config_text.find("fixture.stl") != std::string::npos,
        "project paths should be preserved");
}

} // namespace

int main()
{
    try {
        first_run_copies_missing_vendor_profile_models();
        snapshot_save_keeps_backup_and_validates_new_file();
        old_linux_datadir_plans_migration_when_new_datadir_is_empty();
        old_linux_datadir_check_stays_silent_when_new_datadir_has_content();
        app_config_save_uses_backup_write_and_validates_sections();
        recent_projects_are_written_to_prusaslicer_ini();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
