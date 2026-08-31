#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::kickcat {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::filesystem::path executable_directory;
    std::map<std::string, std::string> environment;
};

struct MasterOptions {
    std::string interface_name;
    bool realtime = false;
    std::optional<std::filesystem::path> esi_file;
};

enum class DiagnosticCode {
    PathResolutionWarning,
    MissingNetworkInterface,
    MissingEsiFile,
    ToolConfigWriteFailed
};

struct ToolDiagnostic {
    DiagnosticCode code = DiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct ToolLayout {
    std::filesystem::path config_root;
    std::filesystem::path cache_root;
    std::filesystem::path runtime_root;
    std::filesystem::path gui_settings_file;
    std::filesystem::path eeprom_workspace;
    std::filesystem::path simulator_socket;
    std::vector<std::filesystem::path> esi_search_roots;
    std::vector<ToolDiagnostic> diagnostics;
};

struct MasterLaunchPlan {
    bool start_bus = false;
    bool realtime_core = false;
    std::string interface_name;
    std::optional<std::filesystem::path> esi_file;
    std::vector<ToolDiagnostic> diagnostics;
};

struct GuiSettingsSaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
    std::filesystem::path target_file;
};

class KickcatTools {
public:
    ToolLayout resolveLayout(const RuntimeEnvironment& environment) const;
    MasterLaunchPlan planMasterLaunch(const ToolLayout& layout, const MasterOptions& options) const;
    GuiSettingsSaveResult saveGuiSettings(const ToolLayout& layout, std::string content) const;
};

} // namespace flavor_tests::kickcat
