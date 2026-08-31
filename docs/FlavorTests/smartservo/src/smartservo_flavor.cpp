#include "smartservo_flavor.hpp"

#include "smartservo/generated/platform_path_defaults.hpp"

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <algorithm>
#include <utility>

namespace flavor_tests::smartservo {

namespace {

GuiDiagnostic diagnostic(
    DiagnosticCode code,
    std::string message,
    bool fatal,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

bool settings_file_is_readable(const std::filesystem::path& path, std::string& message)
{
    if (std::filesystem::exists(path)) {
        return true;
    }
    message = "SmartServoGui device settings file could not be reopened";
    return false;
}

std::string safe_device_filename(std::string value)
{
    std::replace(value.begin(), value.end(), '/', '_');
    std::replace(value.begin(), value.end(), '\\', '_');
    return value.empty() ? "unnamed-device" : value;
}

} // namespace

GuiProfile SmartServoGui::loadProfile(const RuntimeEnvironment& environment) const
{
    linuxdesktop::paths::app_identity identity;
    identity.organization = "Inria";
    identity.application = "SmartServoGui";

    linuxdesktop::paths::resolver_options options;
    options.home_directory = environment.home_directory;
    options.runtime_override = environment.runtime_directory;
    options.environment = environment.environment;
    options.use_process_environment = false;
    if (environment.home_directory) {
        options.platform_defaults = linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory);
    }

    const auto paths = linuxdesktop::paths::resolve_app_paths(identity, options);

    GuiProfile profile;
    profile.config_root = paths.selected.at(linuxdesktop::paths::path_family::config);
    profile.device_profiles_root = paths.selected.at(linuxdesktop::paths::path_family::data) / "devices";
    profile.log_root = paths.selected.at(linuxdesktop::paths::path_family::state) / "logs";
    profile.last_session_file = profile.config_root / "last-session.json";
    for (const auto& item : paths.diagnostics) {
        profile.diagnostics.push_back(diagnostic(
            DiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
    return profile;
}

ScanPlan SmartServoGui::planDeviceScan(const std::vector<SerialLink>& available_links) const
{
    ScanPlan plan;
    for (const auto& link : available_links) {
        if (link.readable && link.writable) {
            plan.links_to_probe.push_back(link);
        } else {
            plan.diagnostics.push_back(diagnostic(
                DiagnosticCode::SerialAccessDenied,
                "serial link is not readable and writable",
                false));
        }
    }
    return plan;
}

DeviceSettingsSaveResult SmartServoGui::saveDeviceSettings(
    const GuiProfile& profile,
    std::string device_name,
    std::string content) const
{
    const auto target = profile.device_profiles_root / (safe_device_filename(std::move(device_name)) + ".json");
    auto report = linuxdesktop::settings::write_common_config(
        {target, std::move(content), true},
        settings_file_is_readable);

    DeviceSettingsSaveResult result;
    result.saved = report.ok;
    result.backup_file = report.backup_path;
    result.target_file = target;
    for (const auto& item : report.diagnostics) {
        result.diagnostics.push_back(diagnostic(
            DiagnosticCode::SettingsWriteFailed,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
    return result;
}

} // namespace flavor_tests::smartservo
