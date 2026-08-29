#include "freecad_flavor.hpp"

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-freecad-flavor";
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

void freecad_environment_overrides_home_data_and_temp()
{
    const auto root = test_root();
    flavor_tests::freecad::ApplicationConfig app_config;
    flavor_tests::freecad::RuntimeEnvironment environment;
    environment.app_home = root / "opt" / "FreeCAD";
    environment.home_directory = root / "home";
    environment.variables["FREECAD_USER_HOME"] = (root / "portable-home").string();
    environment.variables["FREECAD_USER_DATA"] = (root / "portable-data").string();
    environment.variables["FREECAD_USER_TEMP"] = (root / "portable-temp").string();

    const auto config = app_config.build(environment, {});
    require(config.user_home_path == root / "portable-home", "FREECAD_USER_HOME should override home");
    require(config.user_app_data == root / "portable-data", "FREECAD_USER_DATA should override app data");
    require(config.app_temp_path == root / "portable-temp", "FREECAD_USER_TEMP should override temp");
    require(config.user_parameter == root / "portable-data" / "user.cfg", "default user config should follow app data");
}

void command_line_cfg_paths_win_over_defaults()
{
    const auto root = test_root();
    flavor_tests::freecad::ApplicationConfig app_config;
    flavor_tests::freecad::RuntimeEnvironment environment;
    environment.app_home = root / "opt" / "FreeCAD";
    environment.home_directory = root / "home";
    environment.variables["FREECAD_USER_DATA"] = (root / "data").string();

    flavor_tests::freecad::CommandLineOptions options;
    options.user_cfg = root / "profiles" / "user.cfg";
    options.system_cfg = root / "profiles" / "system.cfg";
    options.module_paths = {root / "Mod" / "Sketcher", root / "Mod" / "PartDesign"};

    const auto config = app_config.build(environment, options);
    require(config.user_parameter == *options.user_cfg, "--user-cfg should win over default user.cfg");
    require(config.system_parameter == *options.system_cfg, "--system-cfg should win over default system.cfg");
    require(config.python_search_path.size() == 2, "--module-path entries should be preserved");
}

void deprecated_path_migration_is_planned_unless_kept()
{
    const auto root = test_root();
    const auto deprecated = root / "home" / ".FreeCAD";
    std::filesystem::create_directories(deprecated);

    flavor_tests::freecad::ApplicationConfig app_config;
    flavor_tests::freecad::RuntimeEnvironment environment;
    environment.app_home = root / "opt" / "FreeCAD";
    environment.home_directory = root / "home";
    environment.variables["FREECAD_USER_DATA"] = (root / "xdg" / "FreeCAD").string();

    const auto config = app_config.build(environment, {});
    require(config.deprecated_path_migration.actions.size() == 1,
        "deprecated FreeCAD path should be planned as a migration");

    flavor_tests::freecad::CommandLineOptions keep;
    keep.keep_deprecated_paths = true;
    require(app_config.build(environment, keep).deprecated_path_migration.actions.empty(),
        "--keep-deprecated-paths should suppress migration planning");
}

void user_parameter_save_uses_backup_write()
{
    const auto root = test_root();
    flavor_tests::freecad::ApplicationConfig app_config;
    flavor_tests::freecad::RuntimeEnvironment environment;
    environment.app_home = root / "opt" / "FreeCAD";
    environment.home_directory = root / "home";
    environment.variables["FREECAD_USER_DATA"] = (root / "data").string();

    const auto config = app_config.build(environment, {});
    std::filesystem::create_directories(config.user_parameter.parent_path());
    std::ofstream(config.user_parameter) << "<FCParameters old=\"true\" />\n";

    const auto result = app_config.saveUserParameter(config, "<FCParameters old=\"false\" />\n");
    require(result.saved, "FreeCAD user parameter save should succeed");
    require(result.backup_file.has_value(), "FreeCAD user parameter save should keep backup");
    require(read_file(config.user_parameter).find("false") != std::string::npos,
        "FreeCAD user parameter should be updated");
}

} // namespace

int main()
{
    try {
        freecad_environment_overrides_home_data_and_temp();
        command_line_cfg_paths_win_over_defaults();
        deprecated_path_migration_is_planned_unless_kept();
        user_parameter_save_uses_backup_write();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
