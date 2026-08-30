#include "openipc_dashboard_flavor.hpp"

#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace flavor_tests::openipc_dashboard {

namespace ldp = linuxdesktop::paths;

namespace {

DashboardDiagnostic diagnostic(
    DiagnosticCode code,
    std::string message,
    bool fatal = false,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

bool has_fatal_diagnostic(const std::vector<DashboardDiagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const DashboardDiagnostic& item) {
        return item.fatal;
    });
}

std::optional<std::filesystem::path> environment_path(
    const std::map<std::string, std::string>& environment,
    const std::string& name)
{
    const auto found = environment.find(name);
    if (found == environment.end() || found->second.empty()) {
        return std::nullopt;
    }
    return std::filesystem::path(found->second);
}

std::optional<std::string> environment_value(
    const std::map<std::string, std::string>& environment,
    const std::string& name)
{
    const auto found = environment.find(name);
    if (found == environment.end() || found->second.empty()) {
        return std::nullopt;
    }
    return found->second;
}

std::vector<DashboardDiagnostic> translate_path_diagnostics(const ldp::resolver_report& report)
{
    std::vector<DashboardDiagnostic> result;
    for (const auto& item : report.diagnostics) {
        result.push_back(diagnostic(
            DiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
    return result;
}

std::filesystem::path selected_or_empty(const ldp::resolver_report& report, ldp::path_family family)
{
    const auto found = report.selected.find(family);
    return found == report.selected.end() ? std::filesystem::path{} : found->second;
}

bool is_loopback_address(std::string_view value)
{
    return value == "127.0.0.1" || value == "::1" || value == "localhost";
}

bool is_bind_address_allowed(std::string_view value)
{
    return is_loopback_address(value) || value == "0.0.0.0" || value == "::";
}

bool starts_with(std::string_view value, std::string_view prefix)
{
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool is_urlish(std::string_view value)
{
    return starts_with(value, "http://") || starts_with(value, "https://") || starts_with(value, "rtsp://");
}

bool looks_sensitive_key(std::string_view key)
{
    return key.find("password") != std::string_view::npos
        || key.find("secret") != std::string_view::npos
        || key.find("token") != std::string_view::npos
        || key.find("credential") != std::string_view::npos;
}

bool looks_local_path(std::string_view value)
{
    return starts_with(value, "/")
        || starts_with(value, "file://")
        || (value.size() > 2 && std::isalpha(static_cast<unsigned char>(value[0])) && value[1] == ':');
}

std::string percent_decode(std::string_view value)
{
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const auto hex = value.substr(i + 1, 2);
            unsigned int decoded = 0;
            std::istringstream stream{std::string(hex)};
            stream >> std::hex >> decoded;
            if (!stream.fail()) {
                result.push_back(static_cast<char>(decoded));
                i += 2;
                continue;
            }
        }
        result.push_back(static_cast<char>(value[i]));
    }
    return result;
}

std::string trim(std::string_view value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return std::string(value.substr(first, last - first));
}

} // namespace

ApplicationProfileResult ApplicationProfile::resolve(
    const CommandLineOptions& options,
    const RuntimeEnvironment& environment) const
{
    ApplicationProfileResult profile;
    profile.qt_offscreen_required = options.server_only;

    std::optional<std::filesystem::path> service_root;
    if (options.data_root_override) {
        service_root = *options.data_root_override;
        profile.data_root_source = DataRootSource::CommandLine;
    } else if (auto root = environment_path(environment.environment, "OPENIPC_DATA_ROOT")) {
        service_root = *root;
        profile.data_root_source = DataRootSource::Environment;
    }

    if (service_root) {
        profile.profile_name = "service";
        if (service_root->empty() || !service_root->is_absolute()) {
            profile.valid = false;
            profile.diagnostics.push_back(diagnostic(
                DiagnosticCode::InvalidDataRoot,
                "service data root must be absolute and non-empty",
                true,
                *service_root));
            return profile;
        }

        profile.runtime_root = *service_root;
        profile.config_root = *service_root / "config";
        profile.data_root = *service_root / "data";
        profile.log_file = profile.data_root / "app.log";
        profile.users_file = profile.config_root / "users.json";
        profile.state_database = profile.data_root / "state.sqlite3";
        profile.modules_root = profile.data_root / "modules";
        profile.analytics_event_store = profile.data_root / "analytics_events.sqlite";
        profile.evidence_snapshots_root = *service_root / "evidence" / "snapshots";
        profile.evidence_clips_root = *service_root / "evidence" / "clips";
        profile.qsettings_root = profile.config_root;
        return profile;
    }

    ldp::app_identity identity;
    identity.organization = "OpenIPC";
    identity.application = "Dashboard";

    ldp::resolver_options resolver;
    resolver.home_directory = environment.home_directory;
    resolver.executable_path = environment.executable_directory / "openipc-dashboard";
    resolver.environment = environment.environment;
    resolver.use_process_environment = false;

    const auto report = ldp::resolve_app_paths(identity, resolver);
    profile.runtime_root = selected_or_empty(report, ldp::path_family::runtime);
    profile.config_root = selected_or_empty(report, ldp::path_family::config);
    profile.data_root = selected_or_empty(report, ldp::path_family::data);
    profile.log_file = profile.data_root / "app.log";
    profile.users_file = profile.config_root / "users.json";
    profile.state_database = selected_or_empty(report, ldp::path_family::state) / "state.sqlite3";
    profile.modules_root = profile.data_root / "modules";
    profile.analytics_event_store = profile.data_root / "analytics_events.sqlite";
    profile.evidence_snapshots_root = profile.data_root / "evidence" / "snapshots";
    profile.evidence_clips_root = profile.data_root / "evidence" / "clips";
    profile.qsettings_root = profile.config_root;
    profile.diagnostics = translate_path_diagnostics(report);
    profile.valid = !has_fatal_diagnostic(profile.diagnostics);
    return profile;
}

BootstrapPlan ServerModeBootstrap::prepare(
    const ApplicationProfileResult& profile,
    const CommandLineOptions& options,
    const RuntimeEnvironment& environment) const
{
    BootstrapPlan plan;
    plan.qml_smoke_only = options.smoke_qml;
    plan.tls_self_test_only = options.self_test_tls;
    plan.start_web_server = options.server_only && profile.valid && !options.self_test_tls;
    plan.start_desktop_qml = !options.server_only && environment.desktop_auto_start && profile.valid && !options.self_test_tls;
    plan.diagnostics = profile.diagnostics;

    if (profile.qt_offscreen_required && !environment_value(environment.environment, "QT_QPA_PLATFORM")) {
        plan.environment_overrides["QT_QPA_PLATFORM"] = "offscreen";
    }

    if (options.self_test_tls && !environment.tls_available) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::CapabilityUnavailable,
            "TLS self-test failed",
            true));
    }

    if (options.initialize_admin_username) {
        plan.initialize_admin_username = *options.initialize_admin_username;
        if (!options.server_only) {
            plan.diagnostics.push_back(diagnostic(
                DiagnosticCode::AdminBootstrapInvalid,
                "administrator bootstrap requires server-only mode",
                true));
        } else if (!environment_value(environment.environment, "OPENIPC_INITIAL_ADMIN_PASSWORD_FILE")) {
            plan.diagnostics.push_back(diagnostic(
                DiagnosticCode::AdminBootstrapInvalid,
                "administrator bootstrap requires a password file",
                true));
        } else if (!environment.existing_users.empty()) {
            plan.diagnostics.push_back(diagnostic(
                DiagnosticCode::AdminBootstrapInvalid,
                "administrator bootstrap is available only before users exist",
                true));
        } else {
            plan.initialize_admin = true;
        }
    }

    if (has_fatal_diagnostic(plan.diagnostics)) {
        plan.start_desktop_qml = false;
        plan.start_web_server = false;
        plan.initialize_admin = false;
    }
    return plan;
}

bool DeploymentPolicyResult::originAllowed(std::string_view origin) const
{
    if (!valid) {
        return false;
    }
    if (deployment_profile == "localhost") {
        return starts_with(origin, "http://127.0.0.1")
            || starts_with(origin, "http://localhost")
            || starts_with(origin, "https://localhost");
    }
    if (deployment_profile == "lan" || deployment_profile == "vpn") {
        return starts_with(origin, "http://") || starts_with(origin, "https://");
    }
    return !public_http_url.empty() && origin == public_http_url;
}

DeploymentPolicyResult DeploymentPolicy::fromSettings(const DashboardSettings& settings) const
{
    DeploymentPolicyResult policy;
    policy.deployment_profile = settings.allow_remote_legacy ? "lan" : settings.deployment_profile;
    policy.bind_address = settings.bind_address;
    policy.http_port = settings.http_port;
    policy.web_socket_port = settings.web_socket_port;
    policy.local_http_url = "http://" + settings.bind_address + ":" + std::to_string(settings.http_port);
    policy.public_http_url = settings.public_http_url;
    policy.public_web_socket_url = settings.public_web_socket_url;
    policy.secure_cookies = settings.secure_cookies;
    policy.trusted_proxy_count = settings.trusted_proxy_count;

    const bool known_profile = policy.deployment_profile == "localhost"
        || policy.deployment_profile == "lan"
        || policy.deployment_profile == "vpn"
        || policy.deployment_profile == "reverse_proxy";
    if (!known_profile) {
        policy.valid = false;
        policy.diagnostics.push_back(diagnostic(
            DiagnosticCode::InvalidDeploymentProfile,
            "deployment profile is not supported",
            true));
    }

    if (!is_bind_address_allowed(policy.bind_address)) {
        policy.valid = false;
        policy.diagnostics.push_back(diagnostic(
            DiagnosticCode::InvalidBindAddress,
            "bind address is not allowed for dashboard service",
            true));
    }

    if (policy.deployment_profile == "localhost" && !is_loopback_address(policy.bind_address)) {
        policy.valid = false;
        policy.diagnostics.push_back(diagnostic(
            DiagnosticCode::InvalidBindAddress,
            "localhost profile must bind to loopback",
            true));
    }

    if (policy.deployment_profile == "reverse_proxy") {
        if (policy.public_http_url.empty() || policy.public_web_socket_url.empty()) {
            policy.valid = false;
            policy.diagnostics.push_back(diagnostic(
                DiagnosticCode::InvalidDeploymentProfile,
                "reverse proxy profile requires public HTTP and WebSocket URLs",
                true));
        }
        policy.secure_cookies = true;
    }

    return policy;
}

ReadinessStatus readiness_from(
    const ApplicationProfileResult& profile,
    const DeploymentPolicyResult& policy,
    const RuntimeEnvironment& environment,
    int startup_time_ms)
{
    ReadinessStatus status;
    status.version = "openipc-dashboard-flavor";
    status.profile = profile.profile_name;
    status.startup_time_ms = std::max(0, startup_time_ms);
    status.tls_available = environment.tls_available;
    status.web_sockets_available = environment.web_sockets_available;
    status.webrtc_available = environment.webrtc_available;
    status.bootstrap_required = environment.existing_users.empty();

    if (!profile.valid || !policy.valid) {
        status.last_error = "dashboard configuration is invalid";
        return status;
    }

    if (environment.occupied_ports.count(policy.http_port) != 0
        || environment.occupied_ports.count(policy.web_socket_port) != 0) {
        status.last_error = "dashboard port is already in use";
        return status;
    }

    status.ready = true;
    status.running = true;
    return status;
}

NormalizedPath PathNormalizer::localPathFromUserInput(
    std::string_view input,
    const RuntimeEnvironment& environment) const
{
    const auto value = trim(input);
    if (starts_with(value, "file://")) {
        auto without_scheme = value.substr(7);
        if (starts_with(without_scheme, "localhost/")) {
            without_scheme = without_scheme.substr(9);
        }
        return {std::filesystem::path(percent_decode(without_scheme)), true};
    }

    if (value == "~") {
        return {environment.home_directory.value_or(std::filesystem::path{"~"}), environment.home_directory.has_value()};
    }
    if (starts_with(value, "~/") || starts_with(value, "~\\")) {
        if (environment.home_directory) {
            return {*environment.home_directory / value.substr(2), true};
        }
        return {std::filesystem::path(value), false};
    }

    if (starts_with(value, "mnt/") || starts_with(value, "media/")) {
        return {std::filesystem::path("/") / value, true};
    }

    return {std::filesystem::path(value), false};
}

BrowserDiagnosticBundle browser_diagnostics(
    const ApplicationProfileResult& profile,
    const ReadinessStatus& readiness,
    const std::map<std::string, std::string>& internal_status,
    int log_count)
{
    BrowserDiagnosticBundle bundle;
    bundle.fields["profile"] = readiness.profile;
    bundle.fields["ready"] = readiness.ready ? "true" : "false";
    bundle.fields["running"] = readiness.running ? "true" : "false";
    bundle.fields["version"] = readiness.version;
    bundle.fields["logs"] = std::to_string(std::max(0, log_count));
    bundle.fields["storage"] = profile.profile_name;

    for (const auto& item : internal_status) {
        if (looks_sensitive_key(item.first) || is_urlish(item.second)) {
            bundle.fields[item.first] = "[redacted]";
            bundle.diagnostics.push_back(diagnostic(
                DiagnosticCode::SecretRedacted,
                "sensitive dashboard diagnostic value was redacted"));
        } else if (looks_local_path(item.second)) {
            bundle.fields[item.first] = "[local-path]";
            bundle.diagnostics.push_back(diagnostic(
                DiagnosticCode::LocalPathRedacted,
                "local filesystem path was redacted from browser diagnostics"));
        } else {
            bundle.fields[item.first] = item.second;
        }
    }

    return bundle;
}

} // namespace flavor_tests::openipc_dashboard
