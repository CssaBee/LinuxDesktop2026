#include "qbittorrent_flavor.hpp"

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

void file_logger_settings_use_backup_write()
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

} // namespace

int main()
{
    try {
        explicit_profile_dir_wins_over_default_roots();
        profile_marker_enables_portable_mode_and_relative_fastresume();
        configuration_name_separates_default_profiles();
        file_logger_settings_use_backup_write();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
