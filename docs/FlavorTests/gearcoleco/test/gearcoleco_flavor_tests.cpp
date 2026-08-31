#include "gearcoleco_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace gearcoleco = flavor_tests::gearcoleco;

namespace {

int failures = 0;

void expect(bool condition, const std::string& name)
{
    if (condition) {
        std::cout << "ok " << name << '\n';
    } else {
        std::cout << "not ok " << name << '\n';
        ++failures;
    }
}

void write_file(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << content;
}

gearcoleco::RuntimeEnvironment default_env(const std::string& name)
{
    const auto root = std::filesystem::temp_directory_path() / ("linuxdesktop2026-" + name);
    std::filesystem::remove_all(root);
    gearcoleco::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    env.executable_directory = root / "opt" / "gearcoleco";
    write_file(env.executable_directory / "gearcoleco.ini", "fullscreen=false\n");
    write_file(env.executable_directory / "gamecontrollerdb.txt", "controller\n");
    return env;
}

void installed_mode_uses_user_config_and_executable_resources()
{
    const auto env = default_env("gearcoleco-installed");

    const auto plan = gearcoleco::DesktopFrontend{}.prepare(env, {});

#if defined(_WIN32)
    const auto expected_config = *env.home_directory / "AppData" / "Roaming" / "Gearcoleco" / "Gearcoleco";
#else
    const auto expected_config = *env.home_directory / ".config" / "Gearcoleco" / "Gearcoleco";
#endif
    expect(!plan.portable_active, "installed mode does not activate portable roots");
    expect(plan.config_root == expected_config, "installed mode uses platform config root");
    expect(plan.controller_database == env.executable_directory / "gamecontrollerdb.txt",
        "controller database stays executable-relative");
    expect(std::filesystem::exists(plan.settings_file), "settings model is hydrated into config root");
    expect(plan.copied_defaults.size() == 1, "first run copies default gearcoleco.ini");
}

void portable_marker_moves_settings_beside_executable()
{
    auto env = default_env("gearcoleco-marker");
    write_file(env.executable_directory / "portable.ini", "");

    const auto plan = gearcoleco::DesktopFrontend{}.prepare(env, {});

    expect(plan.portable_requested, "portable.ini requests portable mode");
    expect(plan.portable_active, "portable.ini activates executable-adjacent roots");
    expect(plan.config_root == env.executable_directory, "portable config root is executable directory");
    expect(plan.settings_file == env.executable_directory / "gearcoleco.ini", "portable settings stay beside executable");
}

void command_line_portable_mode_does_not_need_marker_file()
{
    const auto env = default_env("gearcoleco-cli-portable");
    gearcoleco::LaunchOptions options;
    options.portable = true;

    const auto plan = gearcoleco::DesktopFrontend{}.prepare(env, options);

    expect(plan.portable_requested, "command-line portable option is recorded");
    expect(plan.portable_active, "command-line portable option activates portable roots");
    expect(plan.config_root == env.executable_directory, "command-line portable uses executable directory");
}

void rom_symbol_resolution_prefers_existing_sidecar()
{
    const auto env = default_env("gearcoleco-symbols");
    const auto rom = *env.home_directory / "roms" / "demo.rom";
    write_file(rom, "rom");
    write_file(*env.home_directory / "roms" / "demo.sym", "symbols");

    gearcoleco::LaunchOptions options;
    options.rom_file = rom;
    const auto plan = gearcoleco::DesktopFrontend{}.prepare(env, options);

    expect(plan.symbols.automatic.size() == 2, "rom load prepares sym and noi candidates");
    expect(plan.symbols.selected == (*env.home_directory / "roms" / "demo.sym"),
        "existing symbol sidecar is selected");
}

} // namespace

int main()
{
    installed_mode_uses_user_config_and_executable_resources();
    portable_marker_moves_settings_beside_executable();
    command_line_portable_mode_does_not_need_marker_file();
    rom_symbol_resolution_prefers_existing_sidecar();
    return failures == 0 ? 0 : 1;
}
