#include "notepadpp_flavor.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace flavor_tests::notepadpp {

namespace ld = linuxdesktop::settings;

namespace {

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string escape_xml(std::string value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

void append_command(std::ostringstream& output, const char* node_name, const command_shortcut& command)
{
    output << "    <" << node_name << " id=\"" << command.id << "\" name=\""
           << escape_xml(command.name) << "\" keys=\"" << escape_xml(command.keys) << "\" />\n";
}

std::string render_shortcuts_xml(const shortcut_store& store)
{
    std::ostringstream output;
    output << "<NotepadPlus>\n";
    output << "  <InternalCommands>\n";
    for (const auto& command : store.customized_shortcuts) {
        append_command(output, "Shortcut", command);
    }
    output << "  </InternalCommands>\n";

    output << "  <Macros>\n";
    for (const auto& macro : store.macros) {
        output << "    <Macro name=\"" << escape_xml(macro.name) << "\" folderName=\""
               << escape_xml(macro.folder_name) << "\" />\n";
    }
    output << "  </Macros>\n";

    output << "  <UserDefinedCommands>\n";
    for (const auto& command : store.user_commands) {
        append_command(output, "Command", command);
    }
    output << "  </UserDefinedCommands>\n";

    output << "  <PluginCommands>\n";
    for (const auto& command : store.plugin_commands) {
        append_command(output, "PluginCommand", command);
    }
    output << "  </PluginCommands>\n";

    output << "  <ScintillaKeys>\n";
    for (const auto& command : store.scintilla_keys) {
        append_command(output, "ScintKey", command);
    }
    output << "  </ScintillaKeys>\n";
    output << "</NotepadPlus>\n";
    return output.str();
}

void append_history_items(std::ostringstream& output, const char* node_name, const std::vector<std::string>& values)
{
    output << "    <" << node_name << ">\n";
    for (const auto& value : values) {
        output << "      <Item value=\"" << escape_xml(value) << "\" />\n";
    }
    output << "    </" << node_name << ">\n";
}

std::string render_find_history_xml(const find_history& history)
{
    std::ostringstream output;
    output << "<NotepadPlus>\n";
    output << "  <GUIConfigs>\n";
    output << "    <FindHistory>\n";
    append_history_items(output, "Find", history.find);
    append_history_items(output, "Replace", history.replace);
    append_history_items(output, "Paths", history.paths);
    append_history_items(output, "Filters", history.filters);
    output << "    </FindHistory>\n";
    output << "  </GUIConfigs>\n";
    output << "</NotepadPlus>\n";
    return output.str();
}

SaveResult to_save_result(const ld::write_report& report)
{
    return {report.ok, report.backup_path};
}

} // namespace

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
    state_.config_path = state_.user_path / "config.xml";
    state_.shortcuts_path = state_.user_path / "shortcuts.xml";

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
    is_all_loaded = loadXml(config_xml_, state_.config_path) && is_all_loaded;
    is_all_loaded = loadXml(stylers_xml_, state_.user_path / "stylers.xml") && is_all_loaded;
    is_all_loaded = loadXml(context_menu_xml_, state_.user_path / "contextMenu.xml") && is_all_loaded;
    is_all_loaded = loadShortcuts() && is_all_loaded;
    is_all_loaded = loadSessionWithBackupRecovery(true) && is_all_loaded;

    return is_all_loaded;
}

bool NppParameters::loadShortcuts()
{
    if (!std::filesystem::exists(state_.shortcuts_path)) {
        shortcuts_xml_ = {state_.shortcuts_path, false};
        return false;
    }

    const std::string file_content = read_text(state_.shortcuts_path);
    state_.shortcuts_on_disk_hmac_source = file_content;
    if (state_.shortcuts_xml_hmac_source_in_config.empty()) {
        state_.shortcuts_xml_hmac_source_in_config = file_content;
    }

    return loadXml(shortcuts_xml_, state_.shortcuts_path);
}

bool NppParameters::loadSessionWithBackupRecovery(bool remember_last_session)
{
    const auto session_path = state_.session_path / "session.xml";
    if (!remember_last_session) {
        session_xml_ = {session_path, false};
        return true;
    }

    if (std::filesystem::exists(session_path) && loadXml(session_xml_, session_path)) {
        return true;
    }

    const auto backup_path = session_path.string() + ".bak";
    if (!std::filesystem::exists(backup_path)) {
        session_xml_ = {session_path, false};
        return !std::filesystem::exists(session_path);
    }

    ld::write_options restore;
    restore.target = session_path;
    restore.content = read_text(backup_path);
    restore.keep_backup = false;
    restore.atomic_replace = true;
    restore.durable_write = true;

    const auto report = ld::write_with_backup(restore, [this](const std::filesystem::path& path, std::string&) {
        XmlDocument restored;
        return loadXml(restored, path);
    });
    state_.diagnostics.insert(state_.diagnostics.end(), report.diagnostics.begin(), report.diagnostics.end());

    return report.ok && loadXml(session_xml_, session_path);
}

SaveResult NppParameters::saveSession(const std::string& session_xml)
{
    ld::write_options write;
    write.target = state_.session_path / "session.xml";
    write.content = session_xml;
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    const auto report = ld::write_with_backup(write, [this](const std::filesystem::path& path, std::string&) {
        XmlDocument document;
        return loadXml(document, path);
    });
    return to_save_result(report);
}

SaveResult NppParameters::writeShortcuts(const shortcut_store& store)
{
    if (!store.any_shortcut_modified) {
        return {true, std::nullopt};
    }

    ld::write_options write;
    write.target = state_.shortcuts_path;
    write.content = render_shortcuts_xml(store);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    auto report = ld::write_with_backup(write, [this](const std::filesystem::path& path, std::string& message) {
        return validateShortcutXml(path, message);
    });

    if (report.ok) {
        state_.shortcuts_on_disk_hmac_source = write.content;
        state_.shortcuts_xml_hmac_source_in_config = write.content;
        loadXml(shortcuts_xml_, state_.shortcuts_path);
    }
    return to_save_result(report);
}

SaveResult NppParameters::writeFindHistory(const find_history& history)
{
    ld::write_options write;
    write.target = state_.config_path;
    write.content = render_find_history_xml(history);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    auto report = ld::write_with_backup(write, [this](const std::filesystem::path& path, std::string&) {
        XmlDocument document;
        return loadXml(document, path);
    });
    if (report.ok) {
        loadXml(config_xml_, state_.config_path);
    }
    return to_save_result(report);
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

bool NppParameters::validateShortcutXml(const std::filesystem::path& path, std::string& message) const
{
    const std::string bytes = read_text(path);
    const bool ok = bytes.find("<NotepadPlus") != std::string::npos &&
        bytes.find("<InternalCommands>") != std::string::npos &&
        bytes.find("<ScintillaKeys>") != std::string::npos;
    if (!ok) {
        message = "shortcuts.xml did not round-trip through the app shortcut model";
    }
    return ok;
}

} // namespace flavor_tests::notepadpp
