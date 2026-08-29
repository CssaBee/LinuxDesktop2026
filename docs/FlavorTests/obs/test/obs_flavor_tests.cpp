#include "obs_flavor.hpp"

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-obs-flavor";
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

void c_buffer_config_path_uses_xdg_config_home()
{
    const auto root = test_root();
    flavor_tests::obs::Platform platform({root / "home", {{"XDG_CONFIG_HOME", (root / "xdg-config").string()}}});

    char buffer[1024] = {};
    const int size = platform.os_get_config_path(buffer, sizeof(buffer), "basic.ini");

    require(size > 0, "OBS config path should fit buffer");
    require(std::filesystem::path(buffer) == root / "xdg-config" / "obs-studio" / "basic.ini",
        "OBS config path should follow XDG_CONFIG_HOME without exposing C++ reports");
}

void c_buffer_config_path_reports_small_buffer()
{
    const auto root = test_root();
    flavor_tests::obs::Platform platform({root / "home", {{"XDG_CONFIG_HOME", (root / "xdg-config").string()}}});

    char buffer[4] = {};
    require(platform.os_get_config_path(buffer, sizeof(buffer), "basic.ini") == -1,
        "OBS config path should preserve C-style small-buffer failure");
}

void module_config_path_stays_under_plugin_config()
{
    const auto root = test_root();
    flavor_tests::obs::Platform platform({root / "home", {{"XDG_CONFIG_HOME", (root / "xdg-config").string()}}});

    const auto path = std::filesystem::path(platform.obs_module_get_config_path("decklink-output", "settings.json"));

    require(path == root / "xdg-config" / "obs-studio" / "plugin_config" / "decklink-output" / "settings.json",
        "OBS module config path should keep module-owned naming");
}

void config_save_safe_keeps_c_return_shape()
{
    const auto root = test_root();
    const auto target = root / "obs-studio" / "basic.ini";
    std::filesystem::create_directories(target.parent_path());
    std::ofstream(target) << "[General]\nOld=true\n";

    flavor_tests::obs::Platform platform({root / "home", {{"XDG_CONFIG_HOME", (root / "xdg-config").string()}}});
    require(platform.config_save_safe(target, "[General]\nOld=false\n") == 0,
        "OBS safe config save should retain C-style success result");
    require(read_file(target).find("Old=false") != std::string::npos,
        "OBS safe config save should replace target");
}

} // namespace

int main()
{
    try {
        c_buffer_config_path_uses_xdg_config_home();
        c_buffer_config_path_reports_small_buffer();
        module_config_path_stays_under_plugin_config();
        config_save_safe_keeps_c_return_shape();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
