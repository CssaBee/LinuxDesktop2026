#include "keepassxc_flavor.hpp"

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-keepassxc-flavor";
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

void default_config_splits_roaming_and_local_state()
{
    const auto root = test_root();
    flavor_tests::keepassxc::RuntimeEnvironment environment;
    environment.app_dir = root / "app";
    environment.home_directory = root / "home";
    environment.variables["XDG_CONFIG_HOME"] = (root / "xdg-config").string();
    environment.variables["XDG_STATE_HOME"] = (root / "xdg-state").string();
    std::filesystem::create_directories(environment.app_dir);

    flavor_tests::keepassxc::Config config;
    require(config.open(environment), "config should open");

    require(config.files().roaming == root / "xdg-config" / "keepassxc" / "keepassxc.ini",
        "roaming config should follow XDG_CONFIG_HOME");
    require(config.files().local == root / "xdg-state" / "keepassxc" / "keepassxc_local.ini",
        "local config should follow XDG_STATE_HOME");
}

void portable_config_collapses_local_and_roaming_files()
{
    const auto root = test_root();
    const auto portable = root / "portable";
    std::filesystem::create_directories(portable);

    flavor_tests::keepassxc::RuntimeEnvironment environment;
    environment.app_dir = root / "app";
    environment.portable_config_dir = portable;

    flavor_tests::keepassxc::Config config;
    require(config.open(environment), "portable config should open");
    require(config.files().portable, "portable mode should be active");
    require(config.files().roaming == portable / "keepassxc.ini", "roaming settings should be portable");
    require(config.files().local == portable / "keepassxc_local.ini", "local settings should be portable");
}

void import_exports_only_roaming_settings_with_backup_write()
{
    const auto root = test_root();
    const auto portable = root / "portable";
    const auto imported = root / "import.ini";
    const auto exported = root / "export.ini";
    std::filesystem::create_directories(portable);
    std::ofstream(imported) << "[General]\nSingleInstance=false\nLocal/LastDatabases=secret.kdbx\n";
    std::ofstream(exported) << "[General]\nSingleInstance=true\n";

    flavor_tests::keepassxc::Config config;
    require(config.open({root / "app", portable, {}, {}, {}}), "config should open");
    require(config.importSettings(imported), "valid import should succeed");

    const auto result = config.exportSettings(exported);
    require(result.exported, "settings export should succeed");
    require(result.backup_file.has_value(), "settings export should keep a backup");
    require(read_file(exported).find("SingleInstance=false") != std::string::npos,
        "roaming imported settings should be exported");
    require(read_file(exported).find("Local/LastDatabases") == std::string::npos,
        "local settings should not leak into roaming export");
}

void legacy_cache_local_settings_are_planned_as_migration()
{
    const auto root = test_root();
    const auto old_file = root / "cache" / "keepassxc" / "keepassxc_local.ini";
    std::filesystem::create_directories(old_file.parent_path());
    std::ofstream(old_file) << "[General]\nLastDatabases=legacy.kdbx\n";

    flavor_tests::keepassxc::RuntimeEnvironment environment;
    environment.app_dir = root / "app";
    environment.home_directory = root / "home";
    environment.old_cache_config_file = old_file;
    environment.variables["XDG_CONFIG_HOME"] = (root / "xdg-config").string();
    environment.variables["XDG_STATE_HOME"] = (root / "xdg-state").string();

    flavor_tests::keepassxc::Config config;
    require(config.open(environment), "config should open");
    const auto migration = config.migrateOldLocalConfig(environment);

    require(migration.dry_run, "legacy local settings migration should be planned first");
    require(migration.actions.size() == 1, "one local settings move should be planned");
    require(migration.actions.front().source_path == old_file, "migration should source old cache file");
    require(migration.actions.front().target_path == config.files().local, "migration should target local state file");
}

} // namespace

int main()
{
    try {
        default_config_splits_roaming_and_local_state();
        portable_config_collapses_local_and_roaming_files();
        import_exports_only_roaming_settings_with_backup_write();
        legacy_cache_local_settings_are_planned_as_migration();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
