#include "openipc_dashboard_flavor.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace openipc = flavor_tests::openipc_dashboard;

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

struct deterministic_user {
    std::filesystem::path home;
    std::filesystem::path runtime;
};

enum class desktop_root_kind {
    config,
    data,
    state
};

deterministic_user make_deterministic_user(const std::filesystem::path& root)
{
    return {
        root / "home" / "alice",
        root / "run" / "user" / "1000",
    };
}

openipc::RuntimeEnvironment default_env()
{
    const auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-openipc-dashboard-flavor";
    const auto user = make_deterministic_user(root);
    openipc::RuntimeEnvironment env;
    env.home_directory = user.home;
    env.runtime_directory = user.runtime;
    env.executable_directory = root / "opt" / "openipc-dashboard";
    return env;
}

std::filesystem::path platform_default_root(
    const openipc::RuntimeEnvironment& env,
    desktop_root_kind kind)
{
    const auto& home = *env.home_directory;
    switch (kind) {
    case desktop_root_kind::config:
#if defined(_WIN32)
        return home / "AppData" / "Roaming" / "OpenIPC" / "Dashboard";
#else
        return home / ".config" / "OpenIPC" / "Dashboard";
#endif
    case desktop_root_kind::data:
#if defined(_WIN32)
        return home / "AppData" / "Roaming" / "OpenIPC" / "Dashboard";
#else
        return home / ".local" / "share" / "OpenIPC" / "Dashboard";
#endif
    case desktop_root_kind::state:
#if defined(_WIN32)
        return home / "AppData" / "Local" / "OpenIPC" / "Dashboard" / "state";
#else
        return home / ".local" / "state" / "OpenIPC" / "Dashboard";
#endif
    default:
        return {};
    }
}

void desktop_default_profile_uses_user_roots()
{
    const auto env = default_env();
    const auto profile = openipc::ApplicationProfile{}.resolve({}, env);

    expect(profile.valid, "desktop profile should be valid");
    expect(profile.profile_name == "desktop", "desktop profile keeps desktop name");
    expect(profile.config_root == platform_default_root(env, desktop_root_kind::config),
        "desktop config uses platform config root");
    expect(profile.data_root == platform_default_root(env, desktop_root_kind::data),
        "desktop data uses platform data root");
    expect(profile.state_database == platform_default_root(env, desktop_root_kind::state) / "state.sqlite3",
        "desktop state database uses platform state root");
    expect(profile.log_file == profile.data_root / "app.log", "desktop log is under data root");
    expect(!profile.qt_offscreen_required, "desktop mode does not force qt offscreen");
}

void openipc_data_root_creates_isolated_service_profile()
{
    auto env = default_env();
    const auto service_root = std::filesystem::temp_directory_path() / "linuxdesktop2026-openipc-dashboard-service";
    env.environment["OPENIPC_DATA_ROOT"] = service_root.string();

    const auto profile = openipc::ApplicationProfile{}.resolve({}, env);

    expect(profile.profile_name == "service", "environment data root selects service profile");
    expect(profile.data_root_source == openipc::DataRootSource::Environment, "environment data root source is recorded");
    expect(profile.config_root == service_root / "config", "service config root is isolated");
    expect(profile.data_root == service_root / "data", "service data root is isolated");
    expect(profile.log_file == service_root / "data" / "app.log", "service log file is under service data");
    expect(profile.state_database == service_root / "data" / "state.sqlite3", "service state database is under service data");
    expect(profile.modules_root == service_root / "data" / "modules", "service modules root is under service data");
    expect(profile.analytics_event_store == service_root / "data" / "analytics_events.sqlite", "service analytics store is under service data");
    expect(profile.evidence_snapshots_root == service_root / "evidence" / "snapshots", "service snapshots root is isolated");
    expect(profile.evidence_clips_root == service_root / "evidence" / "clips", "service clips root is isolated");
}

void command_line_data_root_wins_over_environment()
{
    auto env = default_env();
    env.environment["OPENIPC_DATA_ROOT"] = (std::filesystem::temp_directory_path() / "linuxdesktop2026-openipc-dashboard-service").string();

    openipc::CommandLineOptions options;
    const auto command_line_root = std::filesystem::temp_directory_path() / "linuxdesktop2026-openipc-dashboard-cli";
    options.data_root_override = command_line_root;

    const auto profile = openipc::ApplicationProfile{}.resolve(options, env);

    expect(profile.data_root_source == openipc::DataRootSource::CommandLine, "command-line root source is recorded");
    expect(profile.config_root == command_line_root / "config", "command-line data root wins over environment");
}

void invalid_data_root_is_fatal_without_desktop_fallback()
{
    auto env = default_env();
    env.environment["OPENIPC_DATA_ROOT"] = "relative-service-root";

    const auto profile = openipc::ApplicationProfile{}.resolve({}, env);

    expect(!profile.valid, "relative service data root is rejected");
    expect(profile.profile_name == "service", "invalid service root does not fall back to desktop profile");
    expect(!profile.diagnostics.empty() && profile.diagnostics.front().code == openipc::DiagnosticCode::InvalidDataRoot, "invalid data root uses dashboard diagnostic");
}

void server_only_sets_offscreen_and_starts_web_without_desktop()
{
    auto env = default_env();
    env.desktop_auto_start = false;
    openipc::CommandLineOptions options;
    options.server_only = true;
    const auto profile = openipc::ApplicationProfile{}.resolve(options, env);

    const auto plan = openipc::ServerModeBootstrap{}.prepare(profile, options, env);

    expect(!plan.start_desktop_qml, "server-only does not start desktop qml");
    expect(plan.start_web_server, "server-only starts web server");
    expect(plan.environment_overrides.at("QT_QPA_PLATFORM") == "offscreen", "server-only sets qt offscreen when unset");
}

void initialize_admin_requires_safe_server_only_password_file()
{
    auto env = default_env();
    openipc::CommandLineOptions options;
    options.initialize_admin_username = "admin";

    const auto desktop_profile = openipc::ApplicationProfile{}.resolve(options, env);
    const auto desktop_plan = openipc::ServerModeBootstrap{}.prepare(desktop_profile, options, env);
    expect(!desktop_plan.initialize_admin, "admin bootstrap is rejected outside server-only");

    options.server_only = true;
    const auto no_password_plan = openipc::ServerModeBootstrap{}.prepare(desktop_profile, options, env);
    expect(!no_password_plan.initialize_admin, "admin bootstrap requires password file");

    env.environment["OPENIPC_INITIAL_ADMIN_PASSWORD_FILE"] = "/run/secrets/openipc-admin";
    env.existing_users.insert("operator");
    const auto existing_user_plan = openipc::ServerModeBootstrap{}.prepare(desktop_profile, options, env);
    expect(!existing_user_plan.initialize_admin, "admin bootstrap rejects existing users");

    env.existing_users.clear();
    const auto accepted_plan = openipc::ServerModeBootstrap{}.prepare(desktop_profile, options, env);
    expect(accepted_plan.initialize_admin, "admin bootstrap accepts first server-only user with password file");
    expect(accepted_plan.initialize_admin_username == "admin", "admin bootstrap keeps username");
    expect(accepted_plan.diagnostics.empty(), "admin bootstrap does not expose password value diagnostics");
}

void deployment_policy_preserves_profiles_and_validates_reverse_proxy()
{
    openipc::DeploymentPolicy policy;
    openipc::DashboardSettings legacy;
    legacy.allow_remote_legacy = true;
    legacy.bind_address = "0.0.0.0";
    const auto lan = policy.fromSettings(legacy);
    expect(lan.deployment_profile == "lan", "legacy allow-remote setting migrates to lan");
    expect(lan.originAllowed("http://192.168.1.10:8080"), "lan profile allows remote http origins");

    openipc::DashboardSettings reverse_proxy;
    reverse_proxy.deployment_profile = "reverse_proxy";
    reverse_proxy.bind_address = "127.0.0.1";
    reverse_proxy.public_http_url = "https://dashboard.example.test";
    reverse_proxy.public_web_socket_url = "wss://dashboard.example.test/ws";
    reverse_proxy.trusted_proxy_count = 2;
    const auto proxied = policy.fromSettings(reverse_proxy);
    expect(proxied.valid, "valid reverse proxy policy is accepted");
    expect(proxied.secure_cookies, "reverse proxy forces secure cookies");
    expect(proxied.trusted_proxy_count == 2, "trusted proxy count is exposed without peer addresses");

    reverse_proxy.public_web_socket_url.clear();
    const auto invalid_proxy = policy.fromSettings(reverse_proxy);
    expect(!invalid_proxy.valid, "reverse proxy requires public websocket url");
}

void readiness_reports_failures_and_bounded_success()
{
    auto env = default_env();
    auto profile = openipc::ApplicationProfile{}.resolve({}, env);
    openipc::DashboardSettings settings;
    auto policy = openipc::DeploymentPolicy{}.fromSettings(settings);

    env.occupied_ports.insert(8080);
    const auto occupied = openipc::readiness_from(profile, policy, env, 42);
    expect(!occupied.ready && occupied.last_error == "dashboard port is already in use", "readiness reports occupied port");

    env.occupied_ports.clear();
    const auto ready = openipc::readiness_from(profile, policy, env, 42);
    expect(ready.ready && ready.running, "readiness reports running service");
    expect(ready.version == "openipc-dashboard-flavor", "readiness exposes bounded version");
    expect(ready.profile == "desktop", "readiness exposes bounded profile");
    expect(ready.startup_time_ms == 42, "readiness exposes startup timing");
    expect(ready.tls_available && ready.web_sockets_available && ready.webrtc_available, "readiness exposes capability flags");
}

void path_normalizer_keeps_dashboard_import_semantics()
{
    auto env = default_env();
    openipc::PathNormalizer normalizer;

    expect(normalizer.localPathFromUserInput("relative/archive.zip", env).path == "relative/archive.zip", "path normalizer preserves relative paths");
    expect(normalizer.localPathFromUserInput("file:///home/alice/Videos/cam%201.mp4", env).path == "/home/alice/Videos/cam 1.mp4", "path normalizer decodes local file urls");
    expect(normalizer.localPathFromUserInput("~/Videos/cam.mp4", env).path == *env.home_directory / "Videos" / "cam.mp4", "path normalizer expands tilde");
    expect(normalizer.localPathFromUserInput("C:\\video\\clip.mp4", env).path == "C:\\video\\clip.mp4", "path normalizer preserves windows drive paths");
    expect(normalizer.localPathFromUserInput("mnt/video/clip.mp4", env).path == "/mnt/video/clip.mp4", "path normalizer recovers likely linux absolute paths");
}

void browser_diagnostics_redact_secrets_and_local_paths()
{
    auto profile = openipc::ApplicationProfile{}.resolve({}, default_env());
    const auto readiness = openipc::readiness_from(profile, openipc::DeploymentPolicy{}.fromSettings({}), default_env(), 7);

    const auto bundle = openipc::browser_diagnostics(
        profile,
        readiness,
        {
            {"camera_password", "secret-value"},
            {"stream_url", "rtsp://user:pass@camera.local/main"},
            {"state_database", "/home/alice/.local/state/OpenIPC/Dashboard/state.sqlite3"},
            {"health", "ok"},
        },
        3);

    expect(bundle.fields.at("camera_password") == "[redacted]", "browser diagnostics redact password fields");
    expect(bundle.fields.at("stream_url") == "[redacted]", "browser diagnostics redact credential-bearing urls");
    expect(bundle.fields.at("state_database") == "[local-path]", "browser diagnostics redact local filesystem paths");
    expect(bundle.fields.at("health") == "ok", "browser diagnostics preserve safe status");
    expect(bundle.fields.at("logs") == "3", "browser diagnostics expose log count only");
}

} // namespace

int main()
{
    desktop_default_profile_uses_user_roots();
    openipc_data_root_creates_isolated_service_profile();
    command_line_data_root_wins_over_environment();
    invalid_data_root_is_fatal_without_desktop_fallback();
    server_only_sets_offscreen_and_starts_web_without_desktop();
    initialize_admin_requires_safe_server_only_password_file();
    deployment_policy_preserves_profiles_and_validates_reverse_proxy();
    readiness_reports_failures_and_bounded_success();
    path_normalizer_keeps_dashboard_import_semantics();
    browser_diagnostics_redact_secrets_and_local_paths();

    return failures == 0 ? 0 : 1;
}
