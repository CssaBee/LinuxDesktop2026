#include "kicad_flavor.hpp"

#include <filesystem>
#include <functional>
#include <sstream>
#include <utility>

namespace flavor_tests::kicad {

namespace ld = linuxdesktop::settings;

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
    ld::app_identity identity;
    identity.organization = "KiCad";
    identity.application = "kicad";

    ld::root_options options;
    options.home_directory = environment_.home_directory;
    options.environment = environment_.variables;
    options.use_process_environment = false;
    if (const auto config_home = environment_.variables.find("XDG_CONFIG_HOME");
        config_home != environment_.variables.end()) {
        options.settings_override = std::filesystem::path(config_home->second) / "KiCad" / "kicad";
    }
    options.named_roots = {
        {"colors", ld::root_purpose::component_config, ld::persistence_class::roaming, "colors", true},
        {"toolbars", ld::root_purpose::component_config, ld::persistence_class::roaming, "toolbars", true},
        {"project-backups", ld::root_purpose::backup, ld::persistence_class::roaming, "project-backups", true},
    };
    const auto report = ld::resolve_app_roots(identity, options);
    user_settings_root_ = report.roots.config;
    state_root_ = report.roots.state;
    color_settings_root_ = report.roots.config / "colors";
    toolbar_settings_root_ = report.roots.config / "toolbars";
    user_backup_root_ = report.roots.state / "project-backups";
    if (const auto* colors = ld::find_named_root(report, "colors")) {
        color_settings_root_ = colors->path;
    }
    if (const auto* toolbars = ld::find_named_root(report, "toolbars")) {
        toolbar_settings_root_ = toolbars->path;
    }
    if (const auto* backups = ld::find_named_root(report, "project-backups")) {
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
    ld::write_options write;
    write.target = GetPathForSettingsFile(settings);
    write.content = settings.json;
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;
    const auto report = ld::write_with_backup(write, json_object_shape);
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
