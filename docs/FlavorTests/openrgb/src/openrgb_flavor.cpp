#include "openrgb_flavor.hpp"

#include "linuxdesktop/paths.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace flavor_tests::openrgb {

namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;
namespace ldd = linuxdesktop::desktop;

namespace {

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string escape_json(std::string value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

bool looks_like_json_object(const std::string& bytes)
{
    const auto first = bytes.find_first_not_of(" \t\r\n");
    const auto last = bytes.find_last_not_of(" \t\r\n");
    return first != std::string::npos && bytes[first] == '{' && bytes[last] == '}';
}

std::string render_configuration_json(const std::vector<ControllerConfiguration>& controllers)
{
    std::ostringstream output;
    output << "{\n  \"controllers\": [\n";
    for (size_t index = 0; index < controllers.size(); ++index) {
        const auto& controller = controllers[index];
        output << "    {\"name\": \"" << escape_json(controller.name)
               << "\", \"zone\": \"" << escape_json(controller.zone) << "\"}";
        if (index + 1 < controllers.size()) {
            output << ',';
        }
        output << '\n';
    }
    output << "  ]\n}\n";
    return output.str();
}

std::string sanitize_desktop_id(std::string value)
{
    for (char& ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
            ch = '-';
        }
    }
    return value.empty() ? "application.desktop" : value + ".desktop";
}

lds::write_report write_json_file(const std::filesystem::path& path, std::string content)
{
    lds::write_options write;
    write.target = path;
    write.content = std::move(content);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;

    return lds::write_with_backup(write, [](const std::filesystem::path& candidate, std::string& message) {
        if (looks_like_json_object(read_text(candidate))) {
            return true;
        }
        message = "OpenRGB JSON did not round-trip as an object";
        return false;
    });
}

SaveResult to_save_result(const lds::write_report& report)
{
    return {report.ok, report.backup_path};
}

AutostartUpdate to_autostart_update(
    const ldd::effect_report& report,
    const ldd::autostart_entry& entry,
    const std::filesystem::path& override_directory)
{
    return {
        report.ok,
        report.dry_run,
        entry.enabled,
        override_directory / sanitize_desktop_id(entry.id),
    };
}

} // namespace

void ProfileManager::SetConfigurationDirectory(const std::filesystem::path& directory)
{
    configuration_directory = directory;
    profile_directory = configuration_directory / "profiles";
    std::filesystem::create_directories(profile_directory);
    profile_list_reloaded = true;
}

ResourceManager::ResourceManager() = default;

bool ResourceManager::InitializeResources(const RuntimeEnvironment& environment)
{
    detection_enabled_ = true;
    first_detection_complete_ = false;
    init_finished_ = false;

    if (!SetupConfigurationDirectory(environment)) {
        return false;
    }

    settings_manager_.LoadSettings(paths_.config_dir / "OpenRGB.json");
    log_manager_.Configure(paths_.config_dir);
    profile_manager_.SetConfigurationDirectory(paths_.config_dir);
    init_finished_ = true;
    return true;
}

bool ResourceManager::SetConfigurationDirectory(const std::filesystem::path& directory)
{
    paths_.config_dir = directory;
    paths_.profiles_dir = directory / "profiles";
    settings_manager_.LoadSettings(directory / "OpenRGB.json");
    log_manager_.Configure(directory);
    profile_manager_.SetConfigurationDirectory(directory);
    return true;
}

SaveResult ResourceManager::SaveSettings(std::string settings_json)
{
    auto report = write_json_file(paths_.config_dir / "OpenRGB.json", std::move(settings_json));
    if (report.ok) {
        settings_manager_.settings_json = read_text(paths_.config_dir / "OpenRGB.json");
    }
    return to_save_result(report);
}

SaveResult ResourceManager::SaveConfiguration(
    const std::vector<ControllerConfiguration>& controllers)
{
    return to_save_result(write_json_file(profile_manager_.configuration_directory / "Configuration.json",
        render_configuration_json(controllers)));
}

bool ResourceManager::SetupConfigurationDirectory(const RuntimeEnvironment& environment)
{
    ldp::app_identity identity;
    identity.organization = "OpenRGB";
    identity.application = "OpenRGB";

    ldp::resolver_options options;
    options.resource_root = environment.resource_root;
    options.home_directory = environment.home_directory;
    options.environment = environment.variables;
    options.use_process_environment = false;
    if (const auto config_home = environment.variables.find("XDG_CONFIG_HOME"); config_home != environment.variables.end()) {
        options.config_override = std::filesystem::path(config_home->second) / "OpenRGB" / "OpenRGB";
    }

    const auto report = ldp::resolve_app_paths(identity, options);
    paths_.resources_dir = report.selected.at(ldp::path_family::resources);
    paths_.config_dir = report.selected.at(ldp::path_family::config);
    paths_.profiles_dir = paths_.config_dir / "profiles";
    return true;
}

AutostartUpdate enable_autostart(
    const std::filesystem::path& executable,
    std::vector<std::string> arguments,
    const std::filesystem::path& working_directory,
    const std::filesystem::path& override_directory)
{
    return set_autostart_enabled(executable, std::move(arguments), working_directory, override_directory, true);
}

AutostartUpdate set_autostart_enabled(
    const std::filesystem::path& executable,
    std::vector<std::string> arguments,
    const std::filesystem::path& working_directory,
    const std::filesystem::path& override_directory,
    bool enabled)
{
    ldd::autostart_entry entry;
    entry.id = "OpenRGB";
    entry.display_name = "OpenRGB";
    entry.executable = executable;
    entry.arguments = std::move(arguments);
    entry.working_directory = working_directory;
    entry.enabled = enabled;
    entry.user_scope = true;

    ldd::apply_options options;
    options.dry_run = true;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = override_directory;

    const auto report = ldd::apply_autostart(entry, options);
    return to_autostart_update(report, entry, override_directory);
}

} // namespace flavor_tests::openrgb
