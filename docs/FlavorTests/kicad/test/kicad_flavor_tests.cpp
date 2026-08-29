#include "kicad_flavor.hpp"

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-kicad-flavor";
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

flavor_tests::kicad::SETTINGS_MANAGER make_manager(const std::filesystem::path& root)
{
    return flavor_tests::kicad::SETTINGS_MANAGER({root / "home", {{"XDG_CONFIG_HOME", (root / "xdg-config").string()}}});
}

void user_color_and_toolbar_settings_resolve_to_named_roots()
{
    const auto root = test_root();
    auto manager = make_manager(root);

    require(manager.SettingsDirectoryValid(), "KiCad settings directory should be valid");
    require(manager.GetPathForSettingsFile({"eeschema.json", flavor_tests::kicad::SettingsLocation::User}) ==
            root / "xdg-config" / "KiCad" / "kicad" / "eeschema.json",
        "user settings should stay under KiCad config root");
    require(manager.GetPathForSettingsFile({"dark.json", flavor_tests::kicad::SettingsLocation::Colors}) ==
            root / "xdg-config" / "KiCad" / "kicad" / "colors" / "dark.json",
        "color settings should use the colors named root");
    require(manager.GetPathForSettingsFile({"pcbnew.tb", flavor_tests::kicad::SettingsLocation::Toolbars}) ==
            manager.GetToolbarSettingsPath() / "pcbnew.tb",
        "toolbar settings should use the toolbar path helper");
}

void project_settings_resolve_beside_the_project()
{
    const auto root = test_root();
    auto manager = make_manager(root);
    flavor_tests::kicad::Project project{root / "boards" / "clock" / "clock.kicad_pro", "clock"};

    require(manager.GetPathForSettingsFile({"fp-lib-table", flavor_tests::kicad::SettingsLocation::Project, "{}", &project}) ==
            project.full_name.parent_path() / "fp-lib-table",
        "project settings should remain project-owned");
}

void backup_root_honors_project_dir_policy()
{
    const auto root = test_root();
    auto manager = make_manager(root);
    flavor_tests::kicad::Project project{root / "boards" / "clock" / "clock.kicad_pro", "clock"};

    require(manager.GetBackupRootForProject(&project) == root / "boards" / "clock" / "clock-backups",
        "project-dir backup policy should preserve legacy project backup root");
}

void backup_root_honors_user_dir_policy_with_project_disambiguation()
{
    const auto root = test_root();
    auto manager = make_manager(root);
    manager.SetBackupLocation(flavor_tests::kicad::BackupLocation::UserDir);
    flavor_tests::kicad::Project project{root / "boards" / "clock" / "clock.kicad_pro", "clock"};

    const auto backup_root = manager.GetBackupRootForProject(&project);
    require(backup_root.parent_path() == root / "xdg-config" / "KiCad" / "kicad" / "project-backups",
        "user-dir backup policy should use the backup named root");
    require(backup_root.filename().string().find("clock-") == 0,
        "user-dir backup policy should disambiguate same-named projects");
}

void save_settings_uses_backup_write_without_changing_product_api()
{
    const auto root = test_root();
    auto manager = make_manager(root);
    flavor_tests::kicad::JsonSettings settings{"common.json", flavor_tests::kicad::SettingsLocation::User, "{\"old\": false}\n"};
    std::filesystem::create_directories(manager.GetPathForSettingsFile(settings).parent_path());
    std::ofstream(manager.GetPathForSettingsFile(settings)) << "{\"old\": true}\n";

    const auto result = manager.Save(settings);
    require(result.saved, "KiCad settings save should succeed");
    require(result.backup_file.has_value(), "KiCad settings save should keep backup");
    require(read_file(manager.GetPathForSettingsFile(settings)).find("false") != std::string::npos,
        "KiCad settings save should update JSON file");
}

} // namespace

int main()
{
    try {
        user_color_and_toolbar_settings_resolve_to_named_roots();
        project_settings_resolve_beside_the_project();
        backup_root_honors_project_dir_policy();
        backup_root_honors_user_dir_policy_with_project_disambiguation();
        save_settings_uses_backup_write_without_changing_product_api();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
