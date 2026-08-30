#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace flavor_tests::openipc_dashboard {

struct CommandLineOptions {
    bool server_only = false;
    std::optional<std::string> initialize_admin_username;
    std::optional<std::filesystem::path> data_root_override;
    bool smoke_qml = false;
    bool self_test_tls = false;
    std::vector<std::string> arguments;
};

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::filesystem::path executable_directory;
    std::map<std::string, std::string> environment;
    std::string platform_name = "linux";
    bool tls_available = true;
    bool web_sockets_available = true;
    bool webrtc_available = true;
    bool desktop_auto_start = true;
    std::set<std::string> existing_users;
    std::set<int> occupied_ports;
};

enum class DataRootSource {
    DesktopDefaults,
    Environment,
    CommandLine
};

enum class DiagnosticCode {
    PathResolutionWarning,
    InvalidDataRoot,
    AdminBootstrapInvalid,
    CapabilityUnavailable,
    InvalidDeploymentProfile,
    InvalidBindAddress,
    OccupiedPort,
    SecretRedacted,
    LocalPathRedacted
};

struct DashboardDiagnostic {
    DiagnosticCode code = DiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct ApplicationProfileResult {
    bool valid = true;
    std::string profile_name = "desktop";
    DataRootSource data_root_source = DataRootSource::DesktopDefaults;
    bool qt_offscreen_required = false;
    std::filesystem::path runtime_root;
    std::filesystem::path config_root;
    std::filesystem::path data_root;
    std::filesystem::path log_file;
    std::filesystem::path users_file;
    std::filesystem::path state_database;
    std::filesystem::path modules_root;
    std::filesystem::path analytics_event_store;
    std::filesystem::path evidence_snapshots_root;
    std::filesystem::path evidence_clips_root;
    std::filesystem::path qsettings_root;
    std::vector<DashboardDiagnostic> diagnostics;
};

class ApplicationProfile {
public:
    ApplicationProfileResult resolve(
        const CommandLineOptions& options,
        const RuntimeEnvironment& environment) const;
};

struct BootstrapPlan {
    bool start_desktop_qml = false;
    bool start_web_server = false;
    bool qml_smoke_only = false;
    bool tls_self_test_only = false;
    bool initialize_admin = false;
    std::string initialize_admin_username;
    std::map<std::string, std::string> environment_overrides;
    std::vector<DashboardDiagnostic> diagnostics;
};

class ServerModeBootstrap {
public:
    BootstrapPlan prepare(
        const ApplicationProfileResult& profile,
        const CommandLineOptions& options,
        const RuntimeEnvironment& environment) const;
};

struct DashboardSettings {
    std::string deployment_profile = "localhost";
    bool allow_remote_legacy = false;
    std::string bind_address = "127.0.0.1";
    int http_port = 8080;
    int web_socket_port = 8081;
    std::string public_http_url;
    std::string public_web_socket_url;
    bool secure_cookies = false;
    int trusted_proxy_count = 0;
};

struct DeploymentPolicyResult {
    bool valid = true;
    std::string deployment_profile;
    std::string bind_address;
    int http_port = 0;
    int web_socket_port = 0;
    std::string local_http_url;
    std::string public_http_url;
    std::string public_web_socket_url;
    bool secure_cookies = false;
    int trusted_proxy_count = 0;
    std::vector<DashboardDiagnostic> diagnostics;

    bool originAllowed(std::string_view origin) const;
};

class DeploymentPolicy {
public:
    DeploymentPolicyResult fromSettings(const DashboardSettings& settings) const;
};

struct ReadinessStatus {
    bool ready = false;
    bool running = false;
    std::string version;
    std::string profile;
    int startup_time_ms = 0;
    bool tls_available = false;
    bool web_sockets_available = false;
    bool webrtc_available = false;
    bool bootstrap_required = false;
    std::string last_error;
};

ReadinessStatus readiness_from(
    const ApplicationProfileResult& profile,
    const DeploymentPolicyResult& policy,
    const RuntimeEnvironment& environment,
    int startup_time_ms);

struct NormalizedPath {
    std::filesystem::path path;
    bool changed = false;
};

class PathNormalizer {
public:
    NormalizedPath localPathFromUserInput(
        std::string_view input,
        const RuntimeEnvironment& environment) const;
};

struct BrowserDiagnosticBundle {
    std::map<std::string, std::string> fields;
    std::vector<DashboardDiagnostic> diagnostics;
};

BrowserDiagnosticBundle browser_diagnostics(
    const ApplicationProfileResult& profile,
    const ReadinessStatus& readiness,
    const std::map<std::string, std::string>& internal_status,
    int log_count);

} // namespace flavor_tests::openipc_dashboard
