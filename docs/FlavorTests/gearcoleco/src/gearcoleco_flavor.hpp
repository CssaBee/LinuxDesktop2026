#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::gearcoleco {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::filesystem::path executable_directory;
    std::map<std::string, std::string> environment;
};

struct LaunchOptions {
    bool portable = false;
    std::optional<std::filesystem::path> rom_file;
    std::optional<std::filesystem::path> symbol_file;
};

enum class DiagnosticCode {
    PathResolutionWarning,
    ConfigHydrationWarning,
    SymbolFileMissing
};

struct EmulatorDiagnostic {
    DiagnosticCode code = DiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct SymbolCandidates {
    std::optional<std::filesystem::path> requested;
    std::vector<std::filesystem::path> automatic;
    std::optional<std::filesystem::path> selected;
};

struct StartupPlan {
    bool portable_requested = false;
    bool portable_active = false;
    std::filesystem::path config_root;
    std::filesystem::path data_root;
    std::filesystem::path controller_database;
    std::filesystem::path settings_file;
    SymbolCandidates symbols;
    std::vector<std::filesystem::path> copied_defaults;
    std::vector<EmulatorDiagnostic> diagnostics;
};

class DesktopFrontend {
public:
    StartupPlan prepare(const RuntimeEnvironment& environment, const LaunchOptions& options) const;
};

} // namespace flavor_tests::gearcoleco
