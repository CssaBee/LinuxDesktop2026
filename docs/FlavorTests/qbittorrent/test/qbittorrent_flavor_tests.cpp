#include "qbittorrent_flavor.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

struct test_failure : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw test_failure(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-qbittorrent-flavor";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw test_failure("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool has_status(
    const flavor_tests::qbittorrent::DesktopRegistrationResult& result,
    flavor_tests::qbittorrent::DesktopRegistrationStatus status)
{
    return std::find(result.statuses.begin(), result.statuses.end(), status) != result.statuses.end();
}

flavor_tests::qbittorrent::DesktopRegistrationRequest registration_request(const std::filesystem::path& root)
{
    const auto icon = root / "icons" / "qbittorrent.png";
    std::filesystem::create_directories(icon.parent_path());
    std::ofstream(icon, std::ios::binary) << "png-ish";

    flavor_tests::qbittorrent::DesktopRegistrationRequest request;
    request.executable = root / "bin" / "qbittorrent";
    request.arguments = {"--profile=" + (root / "profile").string()};
    request.working_directory = root;
    request.icon_source = icon;
    request.staging_root = root / "desktop";
    request.register_autostart = true;
    request.make_torrent_default = true;
    request.register_magnet_links = true;
    request.include_policy_diagnostic = true;
    return request;
}

void explicit_profile_dir_wins_over_default_roots()
{
    const auto root = test_root();
    const auto profile = root / "custom-profile";
    std::filesystem::create_directories(profile);

    flavor_tests::qbittorrent::Profile app_profile;
    flavor_tests::qbittorrent::RuntimeEnvironment environment;
    environment.executable_dir = root / "bin";
    environment.home_directory = root / "home";
    environment.variables["XDG_CONFIG_HOME"] = (root / "xdg-config").string();
    environment.variables["XDG_DATA_HOME"] = (root / "xdg-data").string();
    std::filesystem::create_directories(environment.executable_dir);

    require(app_profile.init(environment, {profile, "nightly", false}), "profile init should succeed");
    require(app_profile.location(flavor_tests::qbittorrent::SpecialFolder::Config) == profile,
        "explicit profile dir should become config root");
    require(!app_profile.portableModeEnabled(), "explicit profile dir should not be auto portable mode");
}

void profile_marker_enables_portable_mode_and_relative_fastresume()
{
    const auto root = test_root();
    flavor_tests::qbittorrent::RuntimeEnvironment environment;
    environment.executable_dir = root / "bin";
    std::filesystem::create_directories(environment.executable_dir / "profile");

    flavor_tests::qbittorrent::Profile app_profile;
    require(app_profile.init(environment, {{}, {}, false}), "profile init should succeed");

    require(app_profile.portableModeEnabled(), "profile folder beside executable should enable portable mode");
    require(app_profile.relativeFastresumePaths(), "portable mode should imply relative fastresume");
    require(app_profile.location(flavor_tests::qbittorrent::SpecialFolder::Config) ==
            environment.executable_dir / "profile",
        "portable config should use executable profile folder");
    require(app_profile.location(flavor_tests::qbittorrent::SpecialFolder::FastResume) ==
            environment.executable_dir / "profile" / "BT_backup",
        "portable fastresume should be relative to profile");
}

void configuration_name_separates_default_profiles()
{
    const auto root = test_root();
    flavor_tests::qbittorrent::RuntimeEnvironment environment;
    environment.executable_dir = root / "bin";
    environment.home_directory = root / "home";
    environment.variables["XDG_CONFIG_HOME"] = (root / "xdg-config").string();
    std::filesystem::create_directories(environment.executable_dir);

    flavor_tests::qbittorrent::Profile app_profile;
    require(app_profile.init(environment, {{}, "test", true}), "profile init should succeed");

    require(app_profile.location(flavor_tests::qbittorrent::SpecialFolder::Config).filename() == "qBittorrent_test",
        "configuration name should suffix the profile leaf");
    require(app_profile.relativeFastresumePaths(), "command line relative-fastresume should be preserved");
}

void file_logger_settings_use_common_config_write()
{
    const auto root = test_root();
    const auto profile = root / "profile";
    std::filesystem::create_directories(profile);
    std::ofstream(profile / "qBittorrent.ini") << "[Application]\nFileLogger\\MaxSizeBytes=1\n";

    flavor_tests::qbittorrent::Profile app_profile;
    require(app_profile.init({root / "bin", {}, {}}, {profile, {}, false}), "profile init should succeed");
    const auto result = app_profile.saveFileLoggerSettings("[Application]\nFileLogger\\MaxSizeBytes=1024\n");

    require(result.saved, "file logger settings save should succeed");
    require(result.backup_file.has_value(), "file logger settings save should keep backup");
    require(read_file(profile / "qBittorrent.ini").find("1024") != std::string::npos,
        "new logger setting should be persisted");
    require(read_file(*result.backup_file).find("MaxSizeBytes=1") != std::string::npos,
        "backup should contain old logger setting");
}

void desktop_registration_flavors_plan_product_bundle_without_live_shell_claims()
{
    const auto root = test_root();
    for (const auto& scenario : flavor_tests::qbittorrent::desktop_flavor_scenarios()) {
        const auto request = registration_request(root / scenario.name);
        const auto result = flavor_tests::qbittorrent::run_desktop_registration(
            request,
            scenario,
            flavor_tests::qbittorrent::DesktopRegistrationMode::Plan);

        require(result.dry_run, "desktop registration plan should remain dry-run");
        require(result.artifact_count >= 6, "desktop registration plan should cover launcher, icon, MIME, default, protocol, and autostart");
        require(result.has_cleanup_reports, "desktop registration plan should include uninstall cleanup rows");

#if defined(_WIN32)
        require(result.ok, "Windows-shaped registration plan should complete as advisory dry-run");
        require(result.unsupported_count > 0, "Windows-shaped registration should expose limited shell and Registry mappings");
        require(result.has_windows_shell_activation, "Windows-shaped registration should keep shell refresh as follow-up");
        require(result.has_windows_default_apps_follow_up, "Windows-shaped default-app registration should remain user mediated");
#else
        require(!scenario.windows_shaped || result.ok,
            "Linux-hosted Windows flavor fixture should still use the same product bundle vocabulary");
        require(result.ok, "XDG desktop registration plan should validate the product bundle");
        require(result.desktop_entry_ready, "qBittorrent launcher should be represented as a bundle artifact");
        require(result.torrent_mime_ready, "qBittorrent torrent file handling should be represented as MIME artifacts");
        require(result.magnet_handler_ready, "qBittorrent magnet handling should be represented as protocol artifacts");
        require(result.has_desktop_database_activation,
            "desktop registration plan should report desktop database follow-up");
        require(result.has_mime_database_activation,
            "desktop registration plan should report MIME database follow-up");
        require(result.has_icon_cache_activation,
            "desktop registration plan should report icon cache follow-up");
        require(result.has_dconf_activation_diagnostic,
            "managed-policy fixture should report dconf activation instead of claiming live policy state");
#endif
    }
}

void desktop_registration_apply_query_and_remove_staged_xdg_artifacts()
{
#if !defined(_WIN32)
    const auto root = test_root() / "xdg-apply";
    const auto scenario = flavor_tests::qbittorrent::DesktopFlavorScenario{"xdg_full_kde", "KDE", false};
    const auto request = registration_request(root);

    const auto applied = flavor_tests::qbittorrent::run_desktop_registration(
        request,
        scenario,
        flavor_tests::qbittorrent::DesktopRegistrationMode::Apply);
    require(applied.ok, "desktop registration apply should stage qBittorrent artifacts");
    require(has_status(applied, flavor_tests::qbittorrent::DesktopRegistrationStatus::Staged),
        "desktop registration apply should report staged artifacts");
    require(applied.cleanup_count >= applied.artifact_count,
        "desktop registration apply should expose cleanup rows for generated artifacts");

    const auto launcher = request.staging_root / "data" / "applications" / "org.qbittorrent.qBittorrent.desktop";
    const auto mimeapps = request.staging_root / "config" / "mimeapps.list";
    require(std::filesystem::exists(launcher), "desktop registration apply should write the launcher file");
    require(read_file(launcher).find("x-scheme-handler/magnet") != std::string::npos,
        "launcher should declare the magnet protocol through bundle associations");
    require(read_file(mimeapps).find("application/x-bittorrent=org.qbittorrent.qBittorrent.desktop;") != std::string::npos,
        "mimeapps should make qBittorrent the torrent default");
    require(read_file(mimeapps).find("x-scheme-handler/magnet=org.qbittorrent.qBittorrent.desktop;") != std::string::npos,
        "mimeapps should register qBittorrent for magnet links");

    const auto queried = flavor_tests::qbittorrent::run_desktop_registration(
        request,
        scenario,
        flavor_tests::qbittorrent::DesktopRegistrationMode::Query);
    require(queried.ok, "desktop registration query should read staged artifacts");
    require(has_status(queried, flavor_tests::qbittorrent::DesktopRegistrationStatus::Present),
        "desktop registration query should report present artifacts");

    const auto removed = flavor_tests::qbittorrent::run_desktop_registration(
        request,
        scenario,
        flavor_tests::qbittorrent::DesktopRegistrationMode::Remove);
    require(removed.ok, "desktop registration remove should clean generated artifacts");
    require(has_status(removed, flavor_tests::qbittorrent::DesktopRegistrationStatus::Missing),
        "desktop registration remove should report missing artifacts after cleanup");
    require(!std::filesystem::exists(launcher), "desktop registration remove should delete the launcher file");
#endif
}

} // namespace

int main()
{
    try {
        explicit_profile_dir_wins_over_default_roots();
        profile_marker_enables_portable_mode_and_relative_fastresume();
        configuration_name_separates_default_profiles();
        file_logger_settings_use_common_config_write();
        desktop_registration_flavors_plan_product_bundle_without_live_shell_claims();
        desktop_registration_apply_query_and_remove_staged_xdg_artifacts();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
