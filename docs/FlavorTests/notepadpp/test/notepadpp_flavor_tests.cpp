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
    require(parameters.langs().loaded, "langs.xml should be copied from defaults and loaded");
    require(parameters.config().loaded, "config.xml should be copied from defaults and loaded");
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

    const auto result = parameters.saveSession("<Session new=\"1\" />\n");
    require(result.saved, "session save should succeed");
    require(result.backup_file.has_value(), "session save should keep a backup");
    require(read_file(settings_root / "session.xml").find("new=\"1\"") != std::string::npos,
        "session should contain the new XML");
    require(read_file(*result.backup_file).find("old=\"1\"") != std::string::npos,
        "backup should contain the old XML");
}

void corrupt_session_is_restored_from_backup()
{
    const auto root = test_root();
    const auto install_root = root / "npp-install";
    const auto settings_root = root / "npp-settings";
    std::filesystem::create_directories(install_root);
    std::filesystem::create_directories(settings_root);
    write_models(install_root);
    std::ofstream(settings_root / "session.xml") << "<broken>\n";
    std::ofstream(settings_root / "session.xml.bak") << "<Session restored=\"1\" />\n";

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, settings_root, {}, {}, false}), "load should recover session");

    require(parameters.session().loaded, "restored session should parse");
    require(read_file(settings_root / "session.xml").find("restored=\"1\"") != std::string::npos,
        "session.xml should be restored from the backup file");
}

void write_shortcuts_uses_backup_write_and_refreshes_hmac_source()
{
    const auto root = test_root();
    const auto install_root = root / "npp-install";
    const auto settings_root = root / "npp-settings";
    std::filesystem::create_directories(install_root);
    std::filesystem::create_directories(settings_root);
    write_models(install_root);
    std::ofstream(settings_root / "shortcuts.xml")
        << "<NotepadPlus><InternalCommands /><ScintillaKeys /></NotepadPlus>\n";

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, settings_root, {}, {}, false}), "load should succeed");
    const auto original_hmac_source = parameters.state().shortcuts_on_disk_hmac_source;

    flavor_tests::notepadpp::shortcut_store store;
    store.any_shortcut_modified = true;
    store.customized_shortcuts.push_back({41002, "Open", "Ctrl+O"});
    store.macros.push_back({"Trim trailing space", "Text cleanup"});
    store.user_commands.push_back({42001, "Run linter", "Ctrl+Alt+L"});
    store.plugin_commands.push_back({43001, "Plugin action", "Ctrl+Shift+P"});
    store.scintilla_keys.push_back({2400, "SCI_LINEDELETE", "Ctrl+Shift+L"});

    const auto result = parameters.writeShortcuts(store);

    require(result.saved, "shortcut write should succeed");
    require(result.backup_file.has_value(), "shortcut write should keep a backup");
    require(parameters.shortcuts().loaded, "shortcut XML should reload after save");
    require(parameters.state().shortcuts_on_disk_hmac_source != original_hmac_source,
        "shortcut HMAC source should be recomputed from the new file content");
    require(parameters.state().shortcuts_xml_hmac_source_in_config == parameters.state().shortcuts_on_disk_hmac_source,
        "config HMAC source should follow a successful shortcut save");
    require(read_file(settings_root / "shortcuts.xml").find("SCI_LINEDELETE") != std::string::npos,
        "shortcut file should contain the rendered Scintilla shortcut");
}

void write_find_history_uses_config_backup_write()
{
    const auto root = test_root();
    const auto install_root = root / "npp-install";
    const auto settings_root = root / "npp-settings";
    std::filesystem::create_directories(install_root);
    std::filesystem::create_directories(settings_root);
    write_models(install_root);
    std::ofstream(settings_root / "config.xml") << "<NotepadPlus><GUIConfigs /></NotepadPlus>\n";

    flavor_tests::notepadpp::NppParameters parameters;
    require(parameters.load({install_root, settings_root, {}, {}, false}), "load should succeed");

    flavor_tests::notepadpp::find_history history;
    history.find = {"needle", "ampersand & value"};
    history.replace = {"replacement"};
    history.paths = {"/tmp/project"};
    history.filters = {"*.cpp"};

    const auto result = parameters.writeFindHistory(history);

    require(result.saved, "find-history config write should succeed");
    require(result.backup_file.has_value(), "config write should keep a backup");
    const auto config = read_file(settings_root / "config.xml");
    require(config.find("<FindHistory>") != std::string::npos, "config should contain FindHistory");
    require(config.find("ampersand &amp; value") != std::string::npos, "find values should be XML escaped");
    require(config.find("*.cpp") != std::string::npos, "filter history should be persisted");
}

} // namespace

int main()
{
    try {
        command_line_settings_dir_wins_over_cloud_and_local_mode();
        portable_marker_moves_settings_beside_the_install_root();
        save_session_uses_backup_write_and_app_validation();
        corrupt_session_is_restored_from_backup();
        write_shortcuts_uses_backup_write_and_refreshes_hmac_source();
        write_find_history_uses_config_backup_write();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
