#include "notepadpp_flavor.hpp"

#include <fstream>
#include <iterator>
#include <utility>

namespace flavor_tests::notepadpp {

namespace ld = linuxdesktop::settings;

bool NppParameters::load(const startup_environment& environment)
{
    ld::app_identity identity;
    identity.organization = "notepad-plus-plus";
    identity.application = "Notepad++";

    ld::root_options roots;
    roots.resource_root = environment.install_root;
    roots.settings_override = environment.command_line_settings_dir;
    roots.sync_config_override = environment.cloud_choice_dir;
    roots.portable_marker = environment.install_root / "doLocalConf.xml";
    roots.portable = ld::portable_level::profile;
    roots.allow_sync_config_for_portable_root = environment.allow_cloud_for_local_config;
    roots.deny_portable_root_in_privileged_install = true;
    roots.privileged_install_roots = environment.privileged_install_roots;
    roots.named_roots = {
        {"plugin-config", ld::root_purpose::plugin_config, ld::persistence_class::roaming, "plugins/Config", true},
    };

    const ld::root_report report = ld::resolve_app_roots(identity, roots);

    state_.npp_path = report.roots.resources;
    state_.user_path = report.roots.config;
    state_.session_path = report.roots.session;
    if (report.settings_override_active || report.portable_active) {
        state_.session_path = report.roots.config;
    }
    state_.user_plugin_config_dir = report.roots.plugin_config;
    if (const auto* plugin_config = ld::find_named_root(report, "plugin-config")) {
        state_.user_plugin_config_dir = plugin_config->path;
    }
    state_.is_local = report.portable_active;
    state_.command_line_override_active = report.settings_override_active;
    state_.cloud_override_active = report.sync_config_override_active;
    state_.diagnostics = report.diagnostics;

    return loadConfigFiles();
}

bool NppParameters::loadConfigFiles()
{
    ld::hydrate_options hydrate;
    hydrate.model_root = state_.npp_path;
    hydrate.target_root = state_.user_path;
    hydrate.files = {
        {"langs.xml", "langs.model.xml", true},
        {"config.xml", "config.model.xml", true},
        {"stylers.xml", "stylers.model.xml", true},
        {"shortcuts.xml", "shortcuts.xml.model", true},
        {"contextMenu.xml", "contextMenu.xml.model", true},
    };

    const ld::hydrate_report hydration = ld::hydrate_config_bundle(hydrate);
    state_.diagnostics.insert(
        state_.diagnostics.end(),
        hydration.diagnostics.begin(),
        hydration.diagnostics.end());

    bool is_all_loaded = true;
    is_all_loaded = loadXml(langs_xml_, state_.user_path / "langs.xml") && is_all_loaded;
    is_all_loaded = loadXml(config_xml_, state_.user_path / "config.xml") && is_all_loaded;
    is_all_loaded = loadXml(stylers_xml_, state_.user_path / "stylers.xml") && is_all_loaded;
    is_all_loaded = loadXml(context_menu_xml_, state_.user_path / "contextMenu.xml") && is_all_loaded;

    const auto session_path = state_.session_path / "session.xml";
    if (std::filesystem::exists(session_path)) {
        is_all_loaded = loadXml(session_xml_, session_path) && is_all_loaded;
    } else {
        session_xml_ = {session_path, false};
    }

    return is_all_loaded;
}

ld::write_report NppParameters::saveSession(const std::string& session_xml)
{
    ld::write_options write;
    write.target = state_.session_path / "session.xml";
    write.content = session_xml;
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    return ld::write_with_backup(write, [this](const std::filesystem::path& path, std::string&) {
        XmlDocument document;
        return loadXml(document, path);
    });
}

bool NppParameters::loadXml(XmlDocument& document, const std::filesystem::path& path) const
{
    document.path = path;
    document.loaded = false;

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    const std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    document.loaded = bytes.find("<NotepadPlus") != std::string::npos ||
        bytes.find("<Session") != std::string::npos;
    return document.loaded;
}

} // namespace flavor_tests::notepadpp
