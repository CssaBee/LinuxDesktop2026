#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::notepadpp {

struct XmlDocument {
    std::filesystem::path path;
    bool loaded = false;
};

struct startup_environment {
    std::filesystem::path install_root;
    std::optional<std::filesystem::path> command_line_settings_dir;
    std::optional<std::filesystem::path> cloud_choice_dir;
    std::vector<std::filesystem::path> privileged_install_roots;
    bool allow_cloud_for_local_config = false;
};

struct loaded_parameters {
    std::filesystem::path npp_path;
    std::filesystem::path user_path;
    std::filesystem::path session_path;
    std::filesystem::path user_plugin_config_dir;
    std::filesystem::path shortcuts_path;
    std::filesystem::path config_path;
    std::string shortcuts_on_disk_hmac_source;
    std::string shortcuts_xml_hmac_source_in_config;
    bool is_local = false;
    bool command_line_override_active = false;
    bool cloud_override_active = false;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

struct command_shortcut {
    int id = 0;
    std::string name;
    std::string keys;
};

struct macro_shortcut {
    std::string name;
    std::string folder_name;
};

struct shortcut_store {
    bool any_shortcut_modified = false;
    std::vector<command_shortcut> customized_shortcuts;
    std::vector<macro_shortcut> macros;
    std::vector<command_shortcut> user_commands;
    std::vector<command_shortcut> plugin_commands;
    std::vector<command_shortcut> scintilla_keys;
};

struct find_history {
    std::vector<std::string> find;
    std::vector<std::string> replace;
    std::vector<std::string> paths;
    std::vector<std::string> filters;
};

class NppParameters {
public:
    bool load(const startup_environment& environment);
    bool loadConfigFiles();
    bool loadShortcuts();
    bool loadSessionWithBackupRecovery(bool remember_last_session);
    linuxdesktop::settings::write_report saveSession(const std::string& session_xml);
    linuxdesktop::settings::write_report writeShortcuts(const shortcut_store& store);
    linuxdesktop::settings::write_report writeFindHistory(const find_history& history);

    const loaded_parameters& state() const { return state_; }
    const XmlDocument& langs() const { return langs_xml_; }
    const XmlDocument& config() const { return config_xml_; }
    const XmlDocument& stylers() const { return stylers_xml_; }
    const XmlDocument& contextMenu() const { return context_menu_xml_; }
    const XmlDocument& shortcuts() const { return shortcuts_xml_; }
    const XmlDocument& session() const { return session_xml_; }

private:
    bool loadXml(XmlDocument& document, const std::filesystem::path& path) const;
    bool validateShortcutXml(const std::filesystem::path& path, std::string& message) const;

    loaded_parameters state_;
    XmlDocument langs_xml_;
    XmlDocument config_xml_;
    XmlDocument stylers_xml_;
    XmlDocument context_menu_xml_;
    XmlDocument shortcuts_xml_;
    XmlDocument session_xml_;
};

} // namespace flavor_tests::notepadpp
