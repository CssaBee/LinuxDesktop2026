#include "kickcat_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace kickcat = flavor_tests::kickcat;

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

kickcat::RuntimeEnvironment default_env()
{
    const auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-kickcat-flavor";
    std::filesystem::remove_all(root);
    kickcat::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    env.executable_directory = root / "opt" / "kickcat";
    return env;
}

void tool_layout_uses_desktop_roots_without_touching_realtime_core()
{
    const auto layout = kickcat::KickcatTools{}.resolveLayout(default_env());

    expect(layout.gui_settings_file.filename() == "kickui.ini", "GUI settings keep tool vocabulary");
    expect(layout.eeprom_workspace.filename() == "eeprom", "EEPROM workspace is a cache child");
    expect(layout.simulator_socket.filename() == "kickcat-simulator.sock", "simulator socket is runtime state");
    expect(layout.esi_search_roots.size() == 2, "ESI lookup has shipped and user roots");
}

void master_launch_keeps_network_and_realtime_policy_in_kickcat()
{
    const auto env = default_env();
    const auto layout = kickcat::KickcatTools{}.resolveLayout(env);
    const auto esi = *env.home_directory / "devices" / "slave.xml";
    write_file(esi, "<EtherCATInfo />\n");

    kickcat::MasterOptions options;
    options.interface_name = "enp8s0";
    options.realtime = true;
    options.esi_file = esi;

    const auto plan = kickcat::KickcatTools{}.planMasterLaunch(layout, options);

    expect(plan.start_bus, "valid master launch starts bus");
    expect(plan.realtime_core, "realtime flag remains KickCAT launch policy");
    expect(plan.interface_name == "enp8s0", "network interface remains product-owned");
    expect(plan.esi_file == esi, "ESI file is carried into launch plan");
}

void invalid_master_launch_returns_product_diagnostics()
{
    const auto layout = kickcat::KickcatTools{}.resolveLayout(default_env());
    kickcat::MasterOptions options;
    options.esi_file = "/missing/slave.xml";

    const auto plan = kickcat::KickcatTools{}.planMasterLaunch(layout, options);

    expect(!plan.start_bus, "invalid master launch does not start bus");
    expect(plan.diagnostics.size() == 2, "missing interface and ESI are both reported");
    expect(plan.diagnostics.front().code == kickcat::DiagnosticCode::MissingNetworkInterface,
        "network diagnostic is product-owned");
}

void gui_settings_use_common_config_write()
{
    const auto layout = kickcat::KickcatTools{}.resolveLayout(default_env());

    const auto saved = kickcat::KickcatTools{}.saveGuiSettings(layout, "theme=dark\n");

    expect(saved.saved, "GUI settings are saved");
    expect(saved.target_file == layout.gui_settings_file, "GUI settings target stays in layout");
    expect(std::filesystem::exists(saved.target_file), "GUI settings file exists");
}

} // namespace

int main()
{
    tool_layout_uses_desktop_roots_without_touching_realtime_core();
    master_launch_keeps_network_and_realtime_policy_in_kickcat();
    invalid_master_launch_returns_product_diagnostics();
    gui_settings_use_common_config_write();
    return failures == 0 ? 0 : 1;
}
