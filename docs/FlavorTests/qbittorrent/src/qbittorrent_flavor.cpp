#include "qbittorrent_flavor.hpp"

#include "linuxdesktop/desktop.hpp"

#include <algorithm>
#include <utility>

namespace flavor_tests::qbittorrent {

namespace {

namespace ldd = linuxdesktop::desktop;

constexpr const char* default_portable_profile_dir = "profile";

bool validate_ini(const std::filesystem::path&, std::string& message)
{
    message.clear();
    return true;
}

DesktopRegistrationStatus to_product_status(ldd::registration_status status)
{
    switch (status) {
    case ldd::registration_status::planned:
        return DesktopRegistrationStatus::Planned;
    case ldd::registration_status::staged:
        return DesktopRegistrationStatus::Staged;
    case ldd::registration_status::present:
        return DesktopRegistrationStatus::Present;
    case ldd::registration_status::missing:
        return DesktopRegistrationStatus::Missing;
    case ldd::registration_status::unsupported:
        return DesktopRegistrationStatus::Unsupported;
    case ldd::registration_status::failed:
        return DesktopRegistrationStatus::Failed;
    case ldd::registration_status::unknown:
        break;
    }
    return DesktopRegistrationStatus::Failed;
}

bool has_activation_step(
    const ldd::desktop_bundle_report& report,
    ldd::activation_step_kind kind)
{
    return std::any_of(report.activation_plan.begin(), report.activation_plan.end(),
        [kind](const ldd::activation_step& step) {
            return step.kind == kind;
        });
}

bool has_diagnostic_code(
    const ldd::desktop_bundle_report& report,
    const std::string& code)
{
    return std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
        [&code](const linuxdesktop::diagnostic& diagnostic) {
            return diagnostic.code == code;
        });
}

bool has_effect(
    const ldd::desktop_bundle_report& report,
    ldd::effect_kind kind,
    ldd::registration_status status)
{
    return std::any_of(report.artifact_reports.begin(), report.artifact_reports.end(),
        [kind, status](const ldd::effect_report& effect) {
            return effect.kind == kind && effect.status == status;
        });
}

ldd::desktop_bundle make_desktop_bundle(const DesktopRegistrationRequest& request)
{
    ldd::desktop_bundle bundle;
    bundle.scope = ldd::registration_scope::user;

    ldd::desktop_entry_metadata entry;
    entry.id = "org.qbittorrent.qBittorrent";
    entry.display_name = "qBittorrent";
    entry.generic_name = "BitTorrent Client";
    entry.comment = "Download and share files over BitTorrent";
    entry.executable = request.executable;
    entry.arguments = request.arguments;
    entry.working_directory = request.working_directory;
    entry.categories = {"Network", "FileTransfer", "P2P"};
    entry.keywords = {"torrent", "magnet", "download"};
    bundle.entry = std::move(entry);

    if (request.register_autostart) {
        ldd::autostart_entry autostart;
        autostart.id = "org.qbittorrent.qBittorrent";
        autostart.display_name = "qBittorrent";
        autostart.executable = request.executable;
        autostart.arguments = request.arguments;
        autostart.working_directory = request.working_directory;
        autostart.enabled = true;
        bundle.autostart = std::move(autostart);
    }

    ldd::icon_reference icon;
    icon.name = "org.qbittorrent.qBittorrent";
    icon.source_path = request.icon_source;
    icon.sizes = {64, 128};
    bundle.icons = {std::move(icon)};

    bundle.mime_declarations = {{"application/x-bittorrent", "BitTorrent metainfo", {"*.torrent"}, true}};
    bundle.mime_associations = {{"application/x-bittorrent", {"org.qbittorrent.qBittorrent"}, true}};
    if (request.make_torrent_default) {
        bundle.default_applications = {{"application/x-bittorrent", "org.qbittorrent.qBittorrent", true, true}};
    }
    if (request.register_magnet_links) {
        bundle.url_scheme_handlers = {{"magnet", "org.qbittorrent.qBittorrent", true}};
    }
    if (request.include_policy_diagnostic) {
        ldd::policy_entry policy;
        policy.id = "qbittorrent-file-association";
        policy.schema_id = "org.qbittorrent.qBittorrent";
        policy.group = "org/qbittorrent/qBittorrent";
        policy.key = "desktop-integration";
        policy.value = "'managed-by-test'";
        policy.enforced = false;
        bundle.policies = {std::move(policy)};
    }

    bundle.cleanup = {{request.staging_root / "stale" / "qBittorrent.desktop", true}};
    return bundle;
}

ldd::apply_options make_desktop_options(const DesktopRegistrationRequest& request)
{
    ldd::apply_options options;
    options.dry_run = true;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;
    options.autostart_directory_override = request.staging_root / "config" / "autostart";
    options.applications_directory_override = request.staging_root / "data" / "applications";
    options.icons_directory_override = request.staging_root / "data" / "icons";
    options.mime_packages_directory_override = request.staging_root / "data" / "mime" / "packages";
    options.mimeapps_file_override = request.staging_root / "config" / "mimeapps.list";
    options.policy_defaults_directory_override = request.staging_root / "dconf" / "defaults";
    options.policy_locks_directory_override = request.staging_root / "dconf" / "locks";
    return options;
}

ldd::desktop_bundle_report run_bundle(
    DesktopRegistrationMode mode,
    const ldd::desktop_bundle& bundle,
    ldd::apply_options options)
{
    switch (mode) {
    case DesktopRegistrationMode::Plan:
        return ldd::plan_bundle(bundle, options);
    case DesktopRegistrationMode::Apply:
        options.dry_run = false;
        return ldd::apply_bundle(bundle, options);
    case DesktopRegistrationMode::Query:
        return ldd::query_bundle(bundle, options);
    case DesktopRegistrationMode::Remove:
        options.dry_run = false;
        return ldd::remove_bundle(bundle, options);
    }
    return ldd::plan_bundle(bundle, options);
}

DesktopRegistrationResult to_product_result(const ldd::desktop_bundle_report& report)
{
    DesktopRegistrationResult result;
    result.ok = report.ok;
    result.dry_run = report.dry_run;
    result.desktop_entry_ready =
        has_effect(report, ldd::effect_kind::desktop_entry, ldd::registration_status::planned) ||
        has_effect(report, ldd::effect_kind::desktop_entry, ldd::registration_status::staged) ||
        has_effect(report, ldd::effect_kind::desktop_entry, ldd::registration_status::present);
    result.torrent_mime_ready =
        has_effect(report, ldd::effect_kind::mime_association, ldd::registration_status::planned) ||
        has_effect(report, ldd::effect_kind::mime_association, ldd::registration_status::staged) ||
        has_effect(report, ldd::effect_kind::mime_association, ldd::registration_status::present);
    result.magnet_handler_ready =
        has_effect(report, ldd::effect_kind::url_protocol_handler, ldd::registration_status::planned) ||
        has_effect(report, ldd::effect_kind::url_protocol_handler, ldd::registration_status::staged) ||
        has_effect(report, ldd::effect_kind::url_protocol_handler, ldd::registration_status::present);
    result.has_cleanup_reports = !report.cleanup_reports.empty();
    result.has_desktop_database_activation =
        has_activation_step(report, ldd::activation_step_kind::refresh_desktop_database);
    result.has_mime_database_activation =
        has_activation_step(report, ldd::activation_step_kind::refresh_mime_database);
    result.has_icon_cache_activation =
        has_activation_step(report, ldd::activation_step_kind::refresh_icon_cache);
    result.has_dconf_activation_diagnostic =
        has_activation_step(report, ldd::activation_step_kind::refresh_dconf_database) ||
        has_diagnostic_code(report, "policy-dconf-activation-required");
    result.has_windows_shell_activation =
        has_activation_step(report, ldd::activation_step_kind::windows_shell_notify);
    result.has_windows_default_apps_follow_up =
        has_activation_step(report, ldd::activation_step_kind::windows_default_apps_ui);
    result.artifact_count = report.artifact_reports.size();
    result.cleanup_count = report.cleanup_reports.size();

    for (const auto& artifact : report.artifact_reports) {
        const auto status = to_product_status(artifact.status);
        if (status == DesktopRegistrationStatus::Unsupported) {
            ++result.unsupported_count;
        }
        result.statuses.push_back(status);
    }
    for (const auto& policy : report.policy_reports) {
        const auto status = to_product_status(policy.status);
        if (status == DesktopRegistrationStatus::Unsupported) {
            ++result.unsupported_count;
        }
        result.statuses.push_back(status);
    }
    for (const auto& diagnostic : report.diagnostics) {
        result.diagnostic_codes.push_back(diagnostic.code);
    }
    return result;
}

} // namespace

bool Profile::init(const RuntimeEnvironment& environment, const CommandLineArgs& args)
{
    configuration_name_ = args.configuration_name;
    const auto portable_profile_path = environment.executable_dir / default_portable_profile_dir;
    portable_mode_enabled_ = !args.profile_dir.has_value() && std::filesystem::is_directory(portable_profile_path);
    relative_fastresume_paths_ = args.relative_fastresume_paths || portable_mode_enabled_;

    linuxdesktop::root::portable_root_request portable_root;
    portable_root.root = portable_profile_path;
    portable_root.marker = portable_profile_path;
    portable_root.level = linuxdesktop::root::portable_root_level::profile;

    auto builder = linuxdesktop::root::request_builder()
        .app("qBittorrent", "qBittorrent" + configurationSuffix())
        .resource_root(environment.executable_dir)
        .home_directory(environment.home_directory)
        .environment(environment.variables)
        .use_process_environment(false)
        .portable_root(portable_root)
        .named_root(linuxdesktop::root::make_log_root_request(
            "logs",
            linuxdesktop::root::ownership_kind::user_local,
            "logs"));
    if (args.profile_dir) {
        builder.app_root_override(*args.profile_dir);
    }

    const auto report = builder.resolve();
    profile_root_ = report.roots.config;
    data_root_ = report.roots.data;
    fastresume_root_ = relative_fastresume_paths_ ? profile_root_ / "BT_backup" : data_root_ / "BT_backup";
    logs_root_ = report.roots.state / "logs";
    if (const auto* logs = linuxdesktop::root::find_named_root(report, "logs")) {
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
    const auto report = linuxdesktop::settings::write_common_config({profile_root_ / "qBittorrent.ini", std::move(content), true},
        validate_ini);
    return {report.ok, report.backup_path};
}

std::string Profile::configurationSuffix() const
{
    return configuration_name_.empty() ? std::string{} : "_" + configuration_name_;
}

std::vector<DesktopFlavorScenario> desktop_flavor_scenarios()
{
    return {
        {"xdg_full_gnome", "ubuntu:GNOME", false},
        {"xdg_full_kde", "KDE", false},
        {"xdg_light_xfce", "XFCE", false},
        {"xdg_minimal_bare_wm", "i3", false},
        {"windows_registration", "Windows", true},
    };
}

DesktopRegistrationResult run_desktop_registration(
    const DesktopRegistrationRequest& request,
    const DesktopFlavorScenario& scenario,
    DesktopRegistrationMode mode)
{
    (void)scenario;
    auto options = make_desktop_options(request);
    const auto bundle = make_desktop_bundle(request);
    return to_product_result(run_bundle(mode, bundle, std::move(options)));
}

} // namespace flavor_tests::qbittorrent
