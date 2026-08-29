#include "notepadpp_flavor.hpp"

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-notepadpp-flavor";
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

void write_models(const std::filesystem::path& install_root)
{
    std::ofstream(install_root / "langs.model.xml") << "<NotepadPlus><Languages /></NotepadPlus>\n";
    std::ofstream(install_root / "config.model.xml") << "<NotepadPlus><GUIConfigs /></NotepadPlus>\n";
    std::ofstream(install_root / "stylers.model.xml") << "<NotepadPlus><LexerStyles /></NotepadPlus>\n";
    std::ofstream(install_root / "shortcuts.xml.model") << "<NotepadPlus><InternalCommands /></NotepadPlus>\n";
    std::ofstream(install_root / "contextMenu.xml.model") << "<NotepadPlus><ScintillaContextMenu /></NotepadPlus>\n";
}

void command_line_settings_dir_wins_over_cloud_and_local_mode()
{
    const auto root = test_root();
    const auto install_root = root / "npp-install";
    const auto command_line_root = root / "cmd-settings";
    const auto cloud_root = root / "cloud-settings";
    std::filesystem::create_directories(install_root);
    std::filesystem::create_directories(command_line_root);
    std::filesystem::create_directories(cloud_root);
    write_models(install_root);

    {
        std::ofstream marker(install_root / "doLocalConf.xml");
        marker << "<localConf />\n";
    }

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, command_line_root, cloud_root, {}, false}), "load should succeed");

    const auto& state = parameters.state();
    require(state.command_line_override_active, "-settingsDir equivalent should be active");
    require(!state.cloud_override_active, "cloud choice should not win over command line settings");
    require(state.user_path == command_line_root, "user settings root should come from command line");
    require(state.session_path == command_line_root, "session follows command line settings");
    require(state.user_plugin_config_dir == command_line_root / "plugins" / "Config",
        "plugin config should stay under active settings root");
    require(parameters.langs().loaded, "langs.xml should be hydrated and loaded");
    require(parameters.config().loaded, "config.xml should be hydrated and loaded");
}

void portable_marker_moves_settings_beside_the_install_root()
{
    const auto root = test_root();
    const auto install_root = root / "npp-portable";
    std::filesystem::create_directories(install_root);
    write_models(install_root);

    {
        std::ofstream marker(install_root / "doLocalConf.xml");
        marker << "<localConf />\n";
    }

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, {}, {}, {}, false}), "load should succeed");

    const auto& state = parameters.state();
    require(state.is_local, "portable marker should activate local mode");
    require(state.npp_path == install_root, "resource path should remain the install root");
    require(state.user_path == install_root, "user settings should be app-local");
    require(state.session_path == install_root, "session should be app-local");
    require(state.user_plugin_config_dir == install_root / "plugins" / "Config",
        "plugin config should be app-local");
    require(parameters.contextMenu().loaded, "context menu XML should load from the portable root");
}

void save_session_uses_backup_write_and_app_validation()
{
    const auto root = test_root();
    const auto install_root = root / "npp-install";
    const auto settings_root = root / "npp-settings";
    std::filesystem::create_directories(install_root);
    std::filesystem::create_directories(settings_root);
    write_models(install_root);
    std::ofstream(settings_root / "session.xml") << "<Session old=\"1\" />\n";

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, settings_root, {}, {}, false}), "load should succeed");

    const auto report = parameters.saveSession("<Session new=\"1\" />\n");
    require(report.ok, "session save should succeed");
    require(report.backup_path.has_value(), "session save should keep a backup");
    require(read_file(settings_root / "session.xml").find("new=\"1\"") != std::string::npos,
        "session should contain the new XML");
    require(read_file(*report.backup_path).find("old=\"1\"") != std::string::npos,
        "backup should contain the old XML");
}

} // namespace

int main()
{
    try {
        command_line_settings_dir_wins_over_cloud_and_local_mode();
        portable_marker_moves_settings_beside_the_install_root();
        save_session_uses_backup_write_and_app_validation();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
