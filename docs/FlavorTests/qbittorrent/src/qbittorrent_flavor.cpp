#include "qbittorrent_flavor.hpp"

#include <utility>

namespace flavor_tests::qbittorrent {

namespace ld = linuxdesktop::settings;

namespace {

constexpr const char* default_portable_profile_dir = "profile";

bool validate_ini(const std::filesystem::path&, std::string& message)
{
    message.clear();
    return true;
}

} // namespace

bool Profile::init(const RuntimeEnvironment& environment, const CommandLineArgs& args)
{
    configuration_name_ = args.configuration_name;
    const auto portable_profile_path = environment.executable_dir / default_portable_profile_dir;
    portable_mode_enabled_ = !args.profile_dir.has_value() && std::filesystem::is_directory(portable_profile_path);
    relative_fastresume_paths_ = args.relative_fastresume_paths || portable_mode_enabled_;

    ld::app_identity identity;
    identity.organization = "qBittorrent";
    identity.application = "qBittorrent" + configurationSuffix();

    ld::root_options options;
    options.resource_root = environment.executable_dir;
    options.home_directory = environment.home_directory;
    options.environment = environment.variables;
    options.use_process_environment = false;
    options.portable = ld::portable_level::profile;
    options.portable_marker = portable_profile_path;
    if (args.profile_dir) {
        options.settings_override = *args.profile_dir;
    } else if (portable_mode_enabled_) {
        options.settings_override = portable_profile_path;
    }
    options.named_roots = {
        {"logs", ld::root_purpose::logs, ld::persistence_class::machine_local, "logs", true},
    };

    const auto report = ld::resolve_app_roots(identity, options);
    profile_root_ = report.roots.config;
    data_root_ = report.roots.data;
    fastresume_root_ = relative_fastresume_paths_ ? profile_root_ / "BT_backup" : data_root_ / "BT_backup";
    logs_root_ = report.roots.state / "logs";
    if (const auto* logs = ld::find_named_root(report, "logs")) {
        logs_root_ = logs->path;
    }
    return !profile_root_.empty();
}

std::filesystem::path Profile::location(SpecialFolder folder) const
{
    switch (folder) {
    case SpecialFolder::Config:
        return profile_root_;
    case SpecialFolder::Data:
        return data_root_;
    case SpecialFolder::FastResume:
        return fastresume_root_;
    case SpecialFolder::Logs:
        return logs_root_;
    }
    return {};
}

SaveResult Profile::saveFileLoggerSettings(std::string content) const
{
    ld::write_options write;
    write.target = profile_root_ / "qBittorrent.ini";
    write.content = std::move(content);
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;
    const auto report = ld::write_with_backup(write, validate_ini);
    return {report.ok, report.backup_path};
}

std::string Profile::configurationSuffix() const
{
    return configuration_name_.empty() ? std::string{} : "_" + configuration_name_;
}

} // namespace flavor_tests::qbittorrent
