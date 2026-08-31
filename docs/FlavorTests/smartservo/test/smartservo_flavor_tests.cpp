#include "smartservo_flavor.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace smartservo = flavor_tests::smartservo;

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

smartservo::RuntimeEnvironment default_env()
{
    const auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-smartservo-flavor";
    std::filesystem::remove_all(root);
    smartservo::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    return env;
}

void gui_profile_uses_platform_roots_for_persistent_state()
{
    const auto env = default_env();

    const auto profile = smartservo::SmartServoGui{}.loadProfile(env);

    expect(profile.config_root.filename() == "SmartServoGui", "config root keeps SmartServoGui identity");
    expect(profile.device_profiles_root.filename() == "devices", "device profiles are app data");
    expect(profile.log_root.filename() == "logs", "logs are app state");
    expect(profile.last_session_file.filename() == "last-session.json", "last session remains GUI vocabulary");
}

void serial_access_policy_stays_outside_linuxdesktop2026()
{
    const auto plan = smartservo::SmartServoGui{}.planDeviceScan({
        {"/dev/ttyUSB0", 1000000, true, true},
        {"/dev/ttyUSB1", 1000000, true, false},
    });

    expect(plan.links_to_probe.size() == 1, "only accessible serial links are probed");
    expect(plan.links_to_probe.front().port_name == "/dev/ttyUSB0", "accessible serial link is preserved");
    expect(!plan.diagnostics.empty(), "inaccessible serial link becomes GUI diagnostic");
    expect(plan.diagnostics.front().code == smartservo::DiagnosticCode::SerialAccessDenied,
        "serial diagnostic is product owned");
}

void device_settings_are_saved_under_profile_root()
{
    const auto profile = smartservo::SmartServoGui{}.loadProfile(default_env());

    const auto saved = smartservo::SmartServoGui{}.saveDeviceSettings(
        profile,
        "arm/joint-1",
        "{\"id\":1}\n");

    expect(saved.saved, "device settings are saved");
    expect(saved.target_file.filename() == "arm_joint-1.json", "device name is made path safe by product code");
    expect(std::filesystem::exists(saved.target_file), "device settings file exists");
}

} // namespace

int main()
{
    gui_profile_uses_platform_roots_for_persistent_state();
    serial_access_policy_stays_outside_linuxdesktop2026();
    device_settings_are_saved_under_profile_root();
    return failures == 0 ? 0 : 1;
}
