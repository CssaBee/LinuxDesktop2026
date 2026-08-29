#include "keepassxc_flavor.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace flavor_tests::keepassxc {

namespace ld = linuxdesktop::settings;
namespace ldm = linuxdesktop::migration;

namespace {

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::map<std::string, std::string> parse_ini(const std::string& bytes)
{
    std::map<std::string, std::string> values;
    std::istringstream input(bytes);
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos || line.empty() || line.front() == '[') {
            continue;
        }
        values[line.substr(0, separator)] = line.substr(separator + 1);
    }
    return values;
}

std::string render_ini(const std::map<std::string, std::string>& values)
{
    std::ostringstream output;
    output << "[General]\n";
    for (const auto& item : values) {
        output << item.first << '=' << item.second << '\n';
    }
    return output.str();
}

bool looks_like_ini(const std::filesystem::path& path, std::string& message)
{
    const auto bytes = read_text(path);
    const bool ok = bytes.find('=') != std::string::npos || bytes.find("[General]") != std::string::npos;
    if (!ok) {
        message = "KeePassXC settings export did not contain INI-shaped content";
    }
    return ok;
}

ExportResult to_export_result(const ld::write_report& report)
{
    return {report.ok, report.backup_path};
}

LocalConfigMigration to_local_config_migration(const ldm::migration_plan& plan)
{
    LocalConfigMigration result;
    result.dry_run = plan.dry_run;
    if (!plan.actions.empty()) {
        result.planned = true;
        result.source = plan.actions.front().source_path;
        result.target = plan.actions.front().target_path;
    }
    return result;
}

} // namespace

bool Config::open(const RuntimeEnvironment& environment)
{
    ld::app_identity identity;
    identity.organization = "keepassxc";
    identity.application = "keepassxc";

    ld::root_options options;
    options.resource_root = environment.app_dir;
    options.home_directory = environment.home_directory;
    options.environment = environment.variables;
    options.use_process_environment = false;
    options.portable = ld::portable_level::profile;
    if (environment.portable_config_dir) {
        options.settings_override = *environment.portable_config_dir;
    }
    options.named_roots = {
        ld::make_state_root_request("local-settings", ld::persistence_class::machine_local),
    };

    const auto roots = ld::resolve_app_roots(identity, options);
    files_.portable = roots.settings_override_active;
    files_.roaming = roots.roots.config / "keepassxc.ini";
    files_.local = roots.roots.state / "keepassxc_local.ini";
    if (const auto config_home = environment.variables.find("XDG_CONFIG_HOME");
        !files_.portable && config_home != environment.variables.end()) {
        files_.roaming = std::filesystem::path(config_home->second) / "keepassxc" / "keepassxc.ini";
    }
    if (const auto state_home = environment.variables.find("XDG_STATE_HOME");
        !files_.portable && state_home != environment.variables.end()) {
        files_.local = std::filesystem::path(state_home->second) / "keepassxc" / "keepassxc_local.ini";
    }
    if (files_.portable) {
        files_.local = roots.roots.config / "keepassxc_local.ini";
    }
    std::filesystem::create_directories(files_.roaming.parent_path());
    std::filesystem::create_directories(files_.local.parent_path());

    if (std::filesystem::exists(files_.roaming)) {
        roaming_values_ = parse_ini(read_text(files_.roaming));
    }
    if (std::filesystem::exists(files_.local)) {
        local_values_ = parse_ini(read_text(files_.local));
    }
    return !roots.roots.config.empty();
}

bool Config::importSettings(const std::filesystem::path& file_name)
{
    const auto imported = parse_ini(read_text(file_name));
    if (imported.empty()) {
        return false;
    }
    roaming_values_.clear();
    for (const auto& item : imported) {
        if (item.first.rfind("Local/", 0) != 0) {
            roaming_values_[item.first] = item.second;
        }
    }
    return !roaming_values_.empty();
}

ExportResult Config::exportSettings(const std::filesystem::path& file_name) const
{
    ld::write_options write;
    write.target = file_name;
    write.content = render_ini(roaming_values_);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;
    return to_export_result(ld::write_with_backup(write, looks_like_ini));
}

LocalConfigMigration Config::migrateOldLocalConfig(const RuntimeEnvironment& environment) const
{
    if (!environment.old_cache_config_file || std::filesystem::exists(files_.local) ||
        !std::filesystem::exists(*environment.old_cache_config_file)) {
        return {};
    }

    ldm::options options;
    options.allow_dangerous = true;
    return to_local_config_migration(ldm::plan_move_file(*environment.old_cache_config_file, files_.local, options));
}

void Config::set(std::string key, std::string value, bool local)
{
    if (local) {
        local_values_[std::move(key)] = std::move(value);
    } else {
        roaming_values_[std::move(key)] = std::move(value);
    }
}

std::optional<std::string> Config::get(const std::string& key) const
{
    if (const auto local = local_values_.find(key); local != local_values_.end()) {
        return local->second;
    }
    if (const auto roaming = roaming_values_.find(key); roaming != roaming_values_.end()) {
        return roaming->second;
    }
    return std::nullopt;
}

} // namespace flavor_tests::keepassxc
