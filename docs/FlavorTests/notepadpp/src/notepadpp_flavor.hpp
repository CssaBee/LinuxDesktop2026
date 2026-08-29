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
    bool is_local = false;
    bool command_line_override_active = false;
    bool cloud_override_active = false;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

class NppParameters {
public:
    bool load(const startup_environment& environment);
    bool loadConfigFiles();
    linuxdesktop::settings::write_report saveSession(const std::string& session_xml);

    const loaded_parameters& state() const { return state_; }
    const XmlDocument& langs() const { return langs_xml_; }
    const XmlDocument& config() const { return config_xml_; }
    const XmlDocument& stylers() const { return stylers_xml_; }
    const XmlDocument& contextMenu() const { return context_menu_xml_; }
    const XmlDocument& session() const { return session_xml_; }

private:
    bool loadXml(XmlDocument& document, const std::filesystem::path& path) const;

    loaded_parameters state_;
    XmlDocument langs_xml_;
    XmlDocument config_xml_;
    XmlDocument stylers_xml_;
    XmlDocument context_menu_xml_;
    XmlDocument session_xml_;
};

} // namespace flavor_tests::notepadpp
