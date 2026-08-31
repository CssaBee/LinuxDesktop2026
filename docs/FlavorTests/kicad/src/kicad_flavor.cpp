#include "kicad_flavor.hpp"

#include <filesystem>
#include <functional>
#include <sstream>
#include <utility>

namespace flavor_tests::kicad {

namespace {

std::string hex_suffix(std::size_t value)
{
    std::ostringstream output;
    output << std::hex << value;
    return output.str().substr(0, 12);
}

bool json_object_shape(const std::filesystem::path&, std::string& message)
{
    message.clear();
    return true;
}

} // namespace

SETTINGS_MANAGER::SETTINGS_MANAGER(RuntimeEnvironment environment)
    : environment_(std::move(environment))
{
    auto builder = linuxdesktop::root::request_builder()
        .app("KiCad", "kicad")
        .home_directory(environment_.home_directory)
        .environment(environment_.variables)
        .use_process_environment(false)
        .named_root(linuxdesktop::root::make_component_config_root_request(
            "colors",
            linuxdesktop::root::ownership_kind::user_roaming,
            "colors"))
        .named_root(linuxdesktop::root::make_component_config_root_request(
            "toolbars",
            linuxdesktop::root::ownership_kind::user_roaming,
            "toolbars"))
        .named_root(linuxdesktop::root::make_named_root_request(
            "project-backups",
            linuxdesktop::root::purpose_kind::backup,
            linuxdesktop::root::ownership_kind::user_roaming,
            "project-backups"));
    if (const auto config_home = environment_.variables.find("XDG_CONFIG_HOME");
        config_home != environment_.variables.end()) {
        builder.app_root_override(std::filesystem::path(config_home->second) / "KiCad" / "kicad");
    }
    const auto report = builder.resolve();
    user_settings_root_ = report.roots.config;
    state_root_ = report.roots.state;
    color_settings_root_ = report.roots.config / "colors";
    toolbar_settings_root_ = report.roots.config / "toolbars";
    user_backup_root_ = report.roots.state / "project-backups";
    if (const auto* colors = linuxdesktop::root::find_named_root(report, "colors")) {
        color_settings_root_ = colors->path;
    }
    if (const auto* toolbars = linuxdesktop::root::find_named_root(report, "toolbars")) {
        toolbar_settings_root_ = toolbars->path;
    }
    if (const auto* backups = linuxdesktop::root::find_named_root(report, "project-backups")) {
        user_backup_root_ = backups->path;
    }
}

bool SETTINGS_MANAGER::SettingsDirectoryValid() const
{
    return !user_settings_root_.empty() && std::filesystem::is_directory(user_settings_root_);
}

std::filesystem::path SETTINGS_MANAGER::GetPathForSettingsFile(const JsonSettings& settings) const
{
    switch (settings.location) {
    case SettingsLocation::User:
        return user_settings_root_ / settings.file_name;
    case SettingsLocation::Project:
        return resolveProject(settings.owning_project).full_name.parent_path() / settings.file_name;
    case SettingsLocation::Colors:
        return color_settings_root_ / settings.file_name;
    case SettingsLocation::Toolbars:
        return GetToolbarSettingsPath() / settings.file_name;
    }
    return {};
}

std::filesystem::path SETTINGS_MANAGER::GetToolbarSettingsPath() const
{
    return toolbar_settings_root_;
}

std::filesystem::path SETTINGS_MANAGER::GetBackupRootForProject(const Project* project) const
{
    const auto& resolved = resolveProject(project);
    if (backup_location_ == BackupLocation::ProjectDir) {
        return resolved.full_name.parent_path() / (resolved.name + "-backups");
    }
    return user_backup_root_ / projectKeySuffix(resolved);
}

SaveResult SETTINGS_MANAGER::Save(const JsonSettings& settings) const
{
    const auto report = linuxdesktop::settings::write_common_config({GetPathForSettingsFile(settings), settings.json, true},
        json_object_shape);
    return {report.ok, report.backup_path};
}

const Project& SETTINGS_MANAGER::resolveProject(const Project* project) const
{
    return project ? *project : project_;
}

std::string SETTINGS_MANAGER::projectKeySuffix(const Project& project) const
{
    return project.name + "-" + hex_suffix(std::hash<std::string>{}(project.full_name.string()));
}

} // namespace flavor_tests::kicad
