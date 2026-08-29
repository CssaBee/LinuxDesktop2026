#include "freecad_flavor.hpp"

#include "linuxdesktop/migration.hpp"
#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <utility>

namespace flavor_tests::freecad {

namespace ldp = linuxdesktop::paths;
namespace ldm = linuxdesktop::migration;
namespace lds = linuxdesktop::settings;

namespace {

std::optional<std::filesystem::path> absolute_env(
    const std::map<std::string, std::string>& variables,
    const std::string& name)
{
    const auto found = variables.find(name);
    if (found == variables.end()) {
        return std::nullopt;
    }
    std::filesystem::path path(found->second);
    return path.is_absolute() ? std::optional<std::filesystem::path>(path) : std::nullopt;
}

bool xml_like(const std::filesystem::path&, std::string& message)
{
    message.clear();
    return true;
}

} // namespace

ConfigurationSet ApplicationConfig::build(
    const RuntimeEnvironment& environment,
    const CommandLineOptions& command_line) const
{
    ConfigurationSet config;
    if (auto user_home = absolute_env(environment.variables, "FREECAD_USER_HOME")) {
        config.user_home_path = *user_home;
    } else if (environment.home_directory) {
        config.user_home_path = *environment.home_directory;
    }

    config.user_app_data = absolute_env(environment.variables, "FREECAD_USER_DATA")
        .value_or(config.user_home_path / ".FreeCAD");
    config.app_temp_path = absolute_env(environment.variables, "FREECAD_USER_TEMP")
        .value_or(config.user_home_path / "temp");

    ldp::app_identity identity;
    identity.organization = "FreeCAD";
    identity.application = "FreeCAD";

    ldp::resolver_options path_options;
    path_options.home_directory = config.user_home_path;
    path_options.resource_root = environment.app_home;
    path_options.config_override = config.user_app_data;
    path_options.temp_override = config.app_temp_path;
    path_options.environment = environment.variables;
    path_options.use_process_environment = false;
    (void)ldp::resolve_app_paths(identity, path_options);

    config.user_parameter = command_line.user_cfg.value_or(config.user_app_data / "user.cfg");
    config.system_parameter = command_line.system_cfg.value_or(config.user_app_data / "system.cfg");
    config.python_search_path = command_line.module_paths;

    const auto deprecated = config.user_home_path / ".FreeCAD";
    if (!command_line.keep_deprecated_paths && deprecated != config.user_app_data && std::filesystem::exists(deprecated)) {
        const auto plan = ldm::plan_copy_directory(deprecated, config.user_app_data);
        config.deprecated_path_migration.dry_run = plan.dry_run;
        if (!plan.actions.empty()) {
            config.deprecated_path_migration.planned = true;
            config.deprecated_path_migration.source = plan.actions.front().source_path;
            config.deprecated_path_migration.target = plan.actions.front().target_path;
        }
    }

    return config;
}

SaveResult ApplicationConfig::saveUserParameter(
    const ConfigurationSet& config,
    std::string xml) const
{
    lds::write_options write;
    write.target = config.user_parameter;
    write.content = std::move(xml);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;
    const auto report = lds::write_with_backup(write, xml_like);
    return {report.ok, report.backup_path};
}

} // namespace flavor_tests::freecad
