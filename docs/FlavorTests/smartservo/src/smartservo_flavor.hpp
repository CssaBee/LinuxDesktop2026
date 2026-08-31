#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::smartservo {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::map<std::string, std::string> environment;
};

struct SerialLink {
    std::string port_name;
    int baud_rate = 1000000;
    bool readable = true;
    bool writable = true;
};

enum class DiagnosticCode {
    PathResolutionWarning,
    SerialAccessDenied,
    SettingsWriteFailed
};

struct GuiDiagnostic {
    DiagnosticCode code = DiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct GuiProfile {
    std::filesystem::path config_root;
    std::filesystem::path device_profiles_root;
    std::filesystem::path log_root;
    std::filesystem::path last_session_file;
    std::vector<GuiDiagnostic> diagnostics;
};

struct ScanPlan {
    std::vector<SerialLink> links_to_probe;
    std::vector<GuiDiagnostic> diagnostics;
};

struct DeviceSettingsSaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
    std::filesystem::path target_file;
    std::vector<GuiDiagnostic> diagnostics;
};

class SmartServoGui {
public:
    GuiProfile loadProfile(const RuntimeEnvironment& environment) const;
    ScanPlan planDeviceScan(const std::vector<SerialLink>& available_links) const;
    DeviceSettingsSaveResult saveDeviceSettings(
        const GuiProfile& profile,
        std::string device_name,
        std::string content) const;
};

} // namespace flavor_tests::smartservo
