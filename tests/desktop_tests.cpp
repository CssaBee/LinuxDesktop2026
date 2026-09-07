#include "linuxdesktop/desktop.hpp"

#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ld = linuxdesktop::desktop;

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-desktop-tests";
    std::filesystem::create_directories(root);
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool has_diagnostic(const std::vector<linuxdesktop::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

bool has_activation_step(const ld::effect_report& report, ld::activation_step_kind kind)
{
    for (const auto& step : report.activation_plan) {
        if (step.kind == kind) {
            return true;
        }
    }
    return false;
}

bool has_activation_step(const ld::desktop_bundle_report& report, ld::activation_step_kind kind)
{
    for (const auto& step : report.activation_plan) {
        if (step.kind == kind) {
            return true;
        }
    }
    return false;
}

bool has_temp_sibling(const std::filesystem::path& target)
{
    std::error_code ec;
    const auto parent = target.parent_path();
    if (!std::filesystem::is_directory(parent, ec)) {
        return false;
    }
    const auto prefix = target.filename().string() + ".tmp.";
    for (const auto& entry : std::filesystem::directory_iterator(parent, ec)) {
        if (entry.path().filename().string().rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

const ld::capability* find_capability(const ld::capability_report& report, ld::effect_kind kind)
{
    for (const auto& capability : report.effects) {
        if (capability.kind == kind) {
            return &capability;
        }
    }
    return nullptr;
}

const ld::effect_report* find_effect_report(const ld::desktop_bundle_report& report, ld::effect_kind kind)
{
    for (const auto& effect : report.artifact_reports) {
        if (effect.kind == kind) {
            return &effect;
        }
    }
    return nullptr;
}

bool has_effect_status(const ld::desktop_bundle_report& report, ld::effect_kind kind, ld::registration_status status)
{
    return std::any_of(report.artifact_reports.begin(), report.artifact_reports.end(), [&](const ld::effect_report& effect) {
        return effect.kind == kind && effect.status == status;
    });
}

const ld::cleanup_report* find_cleanup_report(const ld::desktop_bundle_report& report, const std::filesystem::path& path)
{
    for (const auto& cleanup : report.cleanup_reports) {
        if (cleanup.rule.path == path) {
            return &cleanup;
        }
    }
    return nullptr;
}

bool has_cleanup_status(const ld::desktop_bundle_report& report, const std::filesystem::path& path, ld::cleanup_status status)
{
    const auto* cleanup = find_cleanup_report(report, path);
    return cleanup != nullptr && cleanup->status == status;
}

void require_contains(const std::string& content, const std::string& needle, const char* message)
{
    require(content.find(needle) != std::string::npos, message);
}

#if !defined(_WIN32)
class scoped_env_var {
public:
    scoped_env_var(std::string name, std::string value)
        : name_(std::move(name))
    {
        if (const char* current = std::getenv(name_.c_str())) {
            original_ = current;
        }
        setenv(name_.c_str(), value.c_str(), 1);
    }

    ~scoped_env_var()
    {
        if (original_) {
            setenv(name_.c_str(), original_->c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }

    scoped_env_var(const scoped_env_var&) = delete;
    scoped_env_var& operator=(const scoped_env_var&) = delete;

private:
    std::string name_;
    std::optional<std::string> original_;
};
#endif

ld::autostart_entry autostart_entry_for_tests()
{
    ld::autostart_entry entry;
    entry.id = "linuxdesktop2026-desktop-tests";
    entry.display_name = "LinuxDesktop2026 Desktop Tests";
    entry.executable = "/usr/bin/ld-desktop-test";
    entry.arguments = {"--profile", "Default User"};
    return entry;
}

ld::policy_entry policy_entry_for_tests()
{
    ld::policy_entry entry;
    entry.id = "desktop-tests-theme";
    entry.schema_id = "org.linuxdesktop2026.desktop-tests";
    entry.key = "theme";
    entry.value = "'dark'";
    entry.user_scope = true;
    return entry;
}

ld::desktop_entry desktop_entry_for_tests()
{
    ld::desktop_entry entry;
    entry.id = "org.linuxdesktop2026.DesktopTests";
    entry.display_name = "LinuxDesktop2026 Desktop Tests";
    entry.comment = "Desktop\nregistration";
    entry.executable = "/usr/bin/ld-desktop-test";
    entry.arguments = {"--profile", "Default User"};
    entry.categories = {"Utility", "Development"};
    entry.keywords = {"LinuxDesktop2026", "Desktop"};
    entry.mime_types = {"text/x-linuxdesktop2026-test", "x-scheme-handler/ld2026"};
    return entry;
}

ld::desktop_entry desktop_entry_for_tests(const std::filesystem::path& executable)
{
    auto entry = desktop_entry_for_tests();
    entry.executable = executable;
    return entry;
}

ld::icon_entry icon_entry_for_tests(const std::filesystem::path& source)
{
    ld::icon_entry entry;
    entry.name = "org.linuxdesktop2026.DesktopTests";
    entry.source_path = source;
    entry.size = 64;
    return entry;
}

ld::mime_declaration mime_declaration_for_tests()
{
    ld::mime_declaration declaration;
    declaration.name = "text/x-linuxdesktop2026-test";
    declaration.comment = "LinuxDesktop2026 Test Document";
    declaration.glob_patterns = {"*.ld2026"};
    return declaration;
}

ld::desktop_bundle desktop_bundle_for_tests(const std::filesystem::path& icon_source, const std::filesystem::path& root)
{
    ld::desktop_bundle bundle;
    bundle.scope = ld::registration_scope::user;
    bundle.autostart = autostart_entry_for_tests();
    bundle.autostart->executable = root / "ld-desktop-test";

    ld::desktop_entry_metadata metadata;
    const auto entry = desktop_entry_for_tests(root / "ld-desktop-test");
    metadata.id = entry.id;
    metadata.display_name = entry.display_name;
    metadata.generic_name = entry.generic_name;
    metadata.comment = entry.comment;
    metadata.executable = entry.executable;
    metadata.arguments = entry.arguments;
    metadata.working_directory = entry.working_directory;
    metadata.categories = entry.categories;
    metadata.keywords = entry.keywords;
    metadata.terminal = entry.terminal;
    bundle.entry = metadata;

    ld::icon_reference icon;
    icon.name = "org.linuxdesktop2026.DesktopTests";
    icon.source_path = icon_source;
    icon.sizes = {64};
    bundle.icons = {icon};
    bundle.mime_declarations = {mime_declaration_for_tests()};
    bundle.mime_associations = {{"text/x-linuxdesktop2026-test", {"org.linuxdesktop2026.DesktopTests"}, true}};
    bundle.default_applications = {{"text/x-linuxdesktop2026-test", "org.linuxdesktop2026.DesktopTests", true, true}};
    bundle.url_scheme_handlers = {{"ld2026", "org.linuxdesktop2026.DesktopTests", true}};
    bundle.policies = {policy_entry_for_tests()};
    bundle.cleanup = {{root / "explicit-cleanup.tmp", true}};
    return bundle;
}

void capability_report_covers_extraction_scope()
{
    ld::apply_options options;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;

    const auto report = ld::query_capabilities(options);
    require(report.effects.size() == 9, "desktop capability report should cover every extraction group");
    require(find_capability(report, ld::effect_kind::autostart) != nullptr, "capabilities should include autostart");
    require(find_capability(report, ld::effect_kind::desktop_entry) != nullptr, "capabilities should include desktop entries");
    require(find_capability(report, ld::effect_kind::icon) != nullptr, "capabilities should include icons");
    require(find_capability(report, ld::effect_kind::mime_association) != nullptr, "capabilities should include MIME associations");
    require(find_capability(report, ld::effect_kind::default_application) != nullptr, "capabilities should include default applications");
    require(find_capability(report, ld::effect_kind::url_protocol_handler) != nullptr, "capabilities should include protocol handlers");
    require(find_capability(report, ld::effect_kind::shell_integration) != nullptr, "capabilities should include shell integration");
    require(find_capability(report, ld::effect_kind::desktop_database) != nullptr, "capabilities should include desktop database updates");
    require(find_capability(report, ld::effect_kind::managed_policy) != nullptr, "capabilities should include managed policy");
}

void desktop_bundle_vocabulary_covers_current_registration_groups()
{
    const ld::effect_kind effects[] = {
        ld::effect_kind::autostart,
        ld::effect_kind::desktop_entry,
        ld::effect_kind::icon,
        ld::effect_kind::mime_association,
        ld::effect_kind::default_application,
        ld::effect_kind::url_protocol_handler,
        ld::effect_kind::shell_integration,
        ld::effect_kind::desktop_database,
        ld::effect_kind::managed_policy,
    };
    for (const auto kind : effects) {
        require(ld::to_string(kind) != "unknown", "desktop effect vocabulary should have stable string names");
    }

    const ld::activation_step_kind activation_steps[] = {
        ld::activation_step_kind::refresh_desktop_database,
        ld::activation_step_kind::refresh_mime_database,
        ld::activation_step_kind::refresh_icon_cache,
        ld::activation_step_kind::refresh_dconf_database,
        ld::activation_step_kind::windows_shell_notify,
        ld::activation_step_kind::windows_default_apps_ui,
    };
    for (const auto kind : activation_steps) {
        require(ld::to_string(kind) != "unknown", "desktop activation vocabulary should have stable string names");
    }

    require(ld::to_string(ld::registration_scope::user) == "user", "bundle scope should name user registration");
    require(ld::to_string(ld::registration_scope::global) == "global", "bundle scope should name global registration");
    require(ld::to_string(ld::registration_status::planned) == "planned", "bundle reports should name planned artifact status");
    require(ld::to_string(ld::registration_status::staged) == "staged", "bundle reports should name staged artifact status");
    require(ld::to_string(ld::registration_status::present) == "present", "bundle reports should name present artifact status");
    require(ld::to_string(ld::registration_status::missing) == "missing", "bundle reports should name missing artifact status");
    require(ld::to_string(ld::registration_status::unsupported) == "unsupported", "bundle reports should name unsupported artifact status");
    require(ld::to_string(ld::registration_status::failed) == "failed", "bundle reports should name failed artifact status");
    require(ld::to_string(ld::cleanup_status::planned) == "planned", "bundle cleanup reports should name planned cleanup");
    require(ld::to_string(ld::cleanup_status::removed) == "removed", "bundle cleanup reports should name removed cleanup");
    require(ld::to_string(ld::cleanup_status::already_absent) == "already_absent", "bundle cleanup reports should name absent cleanup");
    require(ld::to_string(ld::cleanup_status::skipped_duplicate) == "skipped_duplicate", "bundle cleanup reports should name duplicate cleanup");
    require(ld::to_string(ld::cleanup_status::manual_follow_up) == "manual_follow_up", "bundle cleanup reports should name manual cleanup");
    require(ld::to_string(ld::cleanup_status::failed) == "failed", "bundle cleanup reports should name failed cleanup");

    const auto root = test_root() / "bundle-vocabulary";
    const auto icon_source = root / "icon.png";
    std::filesystem::create_directories(root);
    {
        std::ofstream file(icon_source, std::ios::binary);
        file << "png-ish";
    }

    ld::apply_options options;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;
    options.autostart_directory_override = root / "autostart";
    options.applications_directory_override = root / "applications";
    options.icons_directory_override = root / "icons";
    options.mime_packages_directory_override = root / "mime" / "packages";
    options.mimeapps_file_override = root / "mimeapps.list";
    options.policy_defaults_directory_override = root / "dconf" / "defaults";
    options.policy_locks_directory_override = root / "dconf" / "locks";

    auto bundle = desktop_bundle_for_tests(icon_source, root);
    const auto planned = ld::plan_bundle(bundle, options);
#if defined(_WIN32)
    require(planned.ok, "Windows desktop bundle plan should complete as advisory dry-run");
#else
    require(planned.ok, "desktop bundle plan should validate every registration artifact");
#endif
    require(planned.dry_run, "desktop bundle plan should stay dry-run");
    require(planned.artifact_reports.size() == 7, "desktop bundle plan should report every artifact group");
    require(planned.policy_reports.size() == 1, "desktop bundle plan should report policy groups separately");
    require(!planned.cleanup_reports.empty(), "desktop bundle plan should report cleanup outcomes");
#if defined(_WIN32)
    require(has_effect_status(planned, ld::effect_kind::desktop_entry, ld::registration_status::unsupported),
        "Windows bundle plan should mark app identity registration as unsupported");
    require(planned.policy_reports.front().kind == ld::effect_kind::managed_policy,
        "desktop bundle policy reports should identify the managed policy effect");
    require(planned.policy_reports.front().status == ld::registration_status::unsupported,
        "Windows bundle plan should mark policy registration as unsupported");
#else
    require(has_effect_status(planned, ld::effect_kind::desktop_entry, ld::registration_status::planned),
        "desktop bundle plan should mark staged artifacts as planned");
    require(planned.policy_reports.front().kind == ld::effect_kind::managed_policy,
        "desktop bundle policy reports should identify the managed policy effect");
    require(planned.policy_reports.front().status == ld::registration_status::planned,
        "desktop bundle plan should mark policy artifacts as planned");
#endif
#if defined(_WIN32)
    require(has_activation_step(planned, ld::activation_step_kind::windows_shell_notify),
        "Windows bundle plan should keep shell activation explicit");
    require(has_activation_step(planned, ld::activation_step_kind::windows_default_apps_ui),
        "Windows bundle plan should keep default-app user choice explicit");
#else
    require(has_activation_step(planned, ld::activation_step_kind::refresh_desktop_database),
        "Linux bundle plan should keep desktop database activation explicit");
    require(has_activation_step(planned, ld::activation_step_kind::refresh_mime_database),
        "Linux bundle plan should keep MIME database activation explicit");
    require(has_activation_step(planned, ld::activation_step_kind::refresh_icon_cache),
        "Linux bundle plan should keep icon cache activation explicit");
    require(has_activation_step(planned, ld::activation_step_kind::refresh_dconf_database),
        "Linux bundle plan should keep dconf activation explicit");
#endif
}

ld::apply_options desktop_bundle_options_for_tests(const std::filesystem::path& root)
{
    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;
    options.autostart_directory_override = root / "autostart";
    options.applications_directory_override = root / "applications";
    options.icons_directory_override = root / "icons";
    options.mime_packages_directory_override = root / "mime" / "packages";
    options.mimeapps_file_override = root / "mimeapps.list";
    options.policy_defaults_directory_override = root / "dconf" / "defaults";
    options.policy_locks_directory_override = root / "dconf" / "locks";
    return options;
}

void desktop_bundle_applies_queries_and_removes_staged_artifacts()
{
#if !defined(_WIN32)
    const auto root = test_root() / "bundle-apply-query-remove";
    std::filesystem::remove_all(root);
    const auto icon_source = root / "source.png";
    std::filesystem::create_directories(root);
    {
        std::ofstream file(icon_source, std::ios::binary);
        file << "png-ish";
    }

    auto bundle = desktop_bundle_for_tests(icon_source, root);
    auto options = desktop_bundle_options_for_tests(root);

    const auto applied = ld::apply_bundle(bundle, options);
    require(applied.ok, "desktop bundle apply should stage every supported artifact");
    require(!applied.dry_run, "desktop bundle apply should report a real write");
    require(has_effect_status(applied, ld::effect_kind::desktop_entry, ld::registration_status::staged),
        "desktop bundle apply should mark desktop entry as staged");
    require(has_effect_status(applied, ld::effect_kind::icon, ld::registration_status::staged),
        "desktop bundle apply should mark icon as staged");
    require(applied.policy_reports.front().status == ld::registration_status::staged,
        "desktop bundle apply should mark policy as staged");
    require(has_activation_step(applied, ld::activation_step_kind::refresh_desktop_database),
        "desktop bundle apply should report desktop database activation separately");
    require(has_activation_step(applied, ld::activation_step_kind::refresh_dconf_database),
        "desktop bundle apply should report dconf activation separately");

    const auto desktop = find_effect_report(applied, ld::effect_kind::desktop_entry);
    require(desktop != nullptr && desktop->path.has_value() && std::filesystem::exists(*desktop->path),
        "desktop bundle apply should write the desktop entry through the staged effect path");
    require(has_cleanup_status(applied, *desktop->path, ld::cleanup_status::planned),
        "desktop bundle apply should include generated desktop entry in cleanup reports");
    require(read_file(*desktop->path).find("MimeType=text/x-linuxdesktop2026-test;x-scheme-handler/ld2026;") != std::string::npos,
        "desktop bundle apply should merge MIME and URL scheme metadata into the desktop entry");

    const auto queried = ld::query_bundle(bundle, options);
    require(queried.ok, "desktop bundle query should succeed after apply");
    require(has_effect_status(queried, ld::effect_kind::desktop_entry, ld::registration_status::present),
        "desktop bundle query should report present desktop entry");
    require(has_effect_status(queried, ld::effect_kind::autostart, ld::registration_status::present),
        "desktop bundle query should report present autostart entry");
    require(queried.policy_reports.front().status == ld::registration_status::present,
        "desktop bundle query should report present policy artifact");

    const auto removed = ld::remove_bundle(bundle, options);
    require(removed.ok, "desktop bundle remove should remove staged artifacts");
    require(has_effect_status(removed, ld::effect_kind::desktop_entry, ld::registration_status::missing),
        "desktop bundle remove should mark desktop entry as missing");
    require(removed.policy_reports.front().status == ld::registration_status::missing,
        "desktop bundle remove should mark policy as missing");
    require(has_cleanup_status(removed, *desktop->path, ld::cleanup_status::removed),
        "desktop bundle remove should report generated desktop entry removal");

    const auto queried_after_remove = ld::query_bundle(bundle, options);
    require(queried_after_remove.ok, "desktop bundle query should succeed after remove");
    require(has_effect_status(queried_after_remove, ld::effect_kind::desktop_entry, ld::registration_status::missing),
        "desktop bundle query after remove should report missing desktop entry");
    require(queried_after_remove.policy_reports.front().status == ld::registration_status::missing,
        "desktop bundle query after remove should report missing policy artifact");
    require(has_cleanup_status(queried_after_remove, *desktop->path, ld::cleanup_status::already_absent),
        "desktop bundle query after remove should report generated desktop entry as already absent");
#endif
}

void desktop_bundle_cleanup_reports_are_idempotent_and_parent_safe()
{
#if !defined(_WIN32)
    const auto root = test_root() / "bundle-explicit-cleanup";
    std::filesystem::remove_all(root);
    const auto generated_parent = root / "generated-parent";
    const auto generated = generated_parent / "generated.tmp";
    const auto sibling = generated_parent / "keep.txt";
    const auto absent = root / "already-absent.tmp";
    const auto directory = root / "not-a-file";
    std::filesystem::create_directories(directory);
    std::filesystem::create_directories(generated_parent);
    {
        std::ofstream file(generated);
        file << "generated";
    }
    {
        std::ofstream file(sibling);
        file << "user";
    }

    ld::desktop_bundle bundle;
    bundle.cleanup = {
        {generated, true},
        {generated, true},
        {absent, true},
        {directory, true},
    };

    auto options = desktop_bundle_options_for_tests(root);
    const auto queried = ld::query_bundle(bundle, options);
    require(queried.ok, "desktop bundle cleanup query should succeed");
    require(has_cleanup_status(queried, generated, ld::cleanup_status::planned),
        "desktop bundle cleanup query should report existing generated artifact as removable");
    require(has_cleanup_status(queried, absent, ld::cleanup_status::already_absent),
        "desktop bundle cleanup query should report missing artifact as already absent");
    require(has_cleanup_status(queried, directory, ld::cleanup_status::planned),
        "desktop bundle cleanup query should not delete directories");

    const auto removed = ld::remove_bundle(bundle, options);
    require(!std::filesystem::exists(generated), "desktop bundle cleanup remove should remove explicit generated file");
    require(std::filesystem::exists(sibling), "desktop bundle cleanup remove should preserve unrelated sibling files");
    require(std::filesystem::exists(generated_parent), "desktop bundle cleanup remove should not remove non-empty parents");
    require(std::filesystem::exists(directory), "desktop bundle cleanup remove should preserve directories for manual follow-up");
    require(has_cleanup_status(removed, generated, ld::cleanup_status::removed),
        "desktop bundle cleanup remove should report removed explicit artifact");
    require(has_cleanup_status(removed, absent, ld::cleanup_status::already_absent),
        "desktop bundle cleanup remove should report missing explicit artifact as already absent");
    require(has_cleanup_status(removed, directory, ld::cleanup_status::manual_follow_up),
        "desktop bundle cleanup remove should report directory cleanup as manual follow-up");
    require(has_diagnostic(removed.diagnostics, "cleanup-duplicate-skipped"),
        "desktop bundle cleanup remove should report duplicate cleanup rules");
    require(has_diagnostic(removed.diagnostics, "cleanup-parent-not-empty"),
        "desktop bundle cleanup remove should report parent cleanup outcome");

    ld::desktop_bundle global_bundle;
    global_bundle.scope = ld::registration_scope::global;
    global_bundle.cleanup = {{generated_parent / "global.tmp", false}};
    {
        std::ofstream file(global_bundle.cleanup.front().path);
        file << "global";
    }
    options.allow_global_write = false;
    const auto denied = ld::remove_bundle(global_bundle, options);
    require(!denied.ok, "global cleanup should require explicit global write permission");
    require(has_diagnostic(denied.diagnostics, "cleanup-global-write-denied"),
        "global cleanup denial should be specific enough for uninstall UI");
#endif
}

void desktop_bundle_partial_failure_preserves_per_effect_diagnostics()
{
#if !defined(_WIN32)
    const auto root = test_root() / "bundle-partial-failure";
    std::filesystem::remove_all(root);
    const auto icon_source = root / "missing.png";
    std::filesystem::create_directories(root);

    auto bundle = desktop_bundle_for_tests(icon_source, root);
    auto options = desktop_bundle_options_for_tests(root);

    const auto report = ld::apply_bundle(bundle, options);
    require(!report.ok, "desktop bundle apply should fail when one staged artifact fails");
    require(has_effect_status(report, ld::effect_kind::desktop_entry, ld::registration_status::staged),
        "desktop bundle partial failure should keep successful artifact status");
    require(has_effect_status(report, ld::effect_kind::icon, ld::registration_status::failed),
        "desktop bundle partial failure should mark the failed artifact");
    require(has_diagnostic(report.diagnostics, "icon-source-path-not-file"),
        "desktop bundle partial failure should aggregate failed child diagnostics");
    const auto icon = find_effect_report(report, ld::effect_kind::icon);
    require(icon != nullptr && has_diagnostic(icon->diagnostics, "icon-source-path-not-file"),
        "desktop bundle partial failure should preserve per-effect icon diagnostics");
    require(report.policy_reports.front().status == ld::registration_status::staged,
        "desktop bundle partial failure should keep successful policy status");
    require(has_activation_step(report, ld::activation_step_kind::refresh_desktop_database),
        "desktop bundle partial failure should still report activation follow-up for staged artifacts");
#endif
}

void managed_policy_capability_reports_dconf_activation_limit()
{
#if !defined(_WIN32)
    ld::apply_options options;
    options.allow_policy_write = true;
    options.allow_global_write = true;

    const auto report = ld::query_capabilities(options);
    const auto* capability = find_capability(report, ld::effect_kind::managed_policy);
    require(capability != nullptr, "capabilities should include managed policy");
    require(capability->state == ld::capability_state::backend_limited,
        "managed policy should report dconf activation as backend-limited");
    require(capability->can_query, "managed policy should still expose generated-file query support");
    require(capability->can_write_user, "managed policy should still allow staged user policy writes");
    require(capability->can_write_global, "managed policy should still allow staged global policy writes with permission");
    require(has_diagnostic(capability->diagnostics, "policy-dconf-activation-required"),
        "managed policy capability should diagnose manual dconf activation");
#endif
}

void windows_registration_capabilities_report_limited_native_mapping()
{
#if defined(_WIN32)
    ld::apply_options options;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;
    options.allow_global_write = true;

    const auto report = ld::query_capabilities(options);

    const ld::effect_kind registry_backed[] = {
        ld::effect_kind::autostart,
        ld::effect_kind::desktop_entry,
        ld::effect_kind::mime_association,
        ld::effect_kind::url_protocol_handler,
        ld::effect_kind::managed_policy,
    };
    for (const auto kind : registry_backed) {
        const auto* capability = find_capability(report, kind);
        require(capability != nullptr, "Windows capability report should include every Registry-backed registration effect");
        require(capability->state == ld::capability_state::backend_limited,
            "Windows Registry-backed registration effects should be backend-limited until the shared Registry layer exists");
        require(capability->can_dry_run, "Windows registration effects should support dry-run proof");
        require(!capability->can_write_user && !capability->can_write_global,
            "Windows registration effects should not promise writes before the shared Registry layer exists");
    }

    const auto* defaults = find_capability(report, ld::effect_kind::default_application);
    require(defaults != nullptr, "Windows capability report should include default applications");
    require(defaults->state == ld::capability_state::backend_limited,
        "Windows default applications should be backend-limited rather than forceable");
    require(has_diagnostic(defaults->diagnostics, "desktop.windows.default-apps.user-choice-required"),
        "Windows default applications should diagnose the user-choice contract");

    const auto* shell = find_capability(report, ld::effect_kind::shell_integration);
    require(shell != nullptr, "Windows capability report should include shell integration");
    require(shell->state == ld::capability_state::backend_missing,
        "Windows runtime shell integration should remain backend-missing");
#endif
}

void windows_registration_dry_runs_report_limits_without_mutation()
{
#if defined(_WIN32)
    ld::apply_options options;
    options.allow_desktop_integration_write = true;
    options.allow_policy_write = true;

    const auto autostart = ld::apply_autostart(autostart_entry_for_tests(), options);
    require(autostart.ok && autostart.dry_run, "Windows autostart dry-run should succeed without Registry mutation");
    require(has_diagnostic(autostart.diagnostics, "desktop.windows.autostart.registry-layer-required"),
        "Windows autostart dry-run should name the Registry-layer dependency");

    ld::mime_association association;
    association.mime_type = "text/x-linuxdesktop2026-test";
    association.desktop_entry_ids = {"org.linuxdesktop2026.DesktopTests"};
    const auto association_report = ld::apply_mime_association(association, options);
    require(association_report.ok, "Windows file association dry-run should succeed without Registry mutation");
    require(has_diagnostic(association_report.diagnostics, "desktop.windows.file-association.registry-layer-required"),
        "Windows file association dry-run should name the Registry-layer dependency");
    require(has_activation_step(association_report, ld::activation_step_kind::windows_shell_notify),
        "Windows file association dry-run should report shell notification activation");

    ld::default_application_intent defaults;
    defaults.mime_type_or_scheme = "text/x-linuxdesktop2026-test";
    defaults.desktop_entry_id = "org.linuxdesktop2026.DesktopTests";
    defaults.make_default = true;
    const auto default_report = ld::apply_default_application(defaults, options);
    require(default_report.ok, "Windows default application dry-run should succeed as guidance");
    require(has_diagnostic(default_report.diagnostics, "desktop.windows.default-apps.user-choice-required"),
        "Windows default application dry-run should diagnose that UserChoice is not forced");
    require(has_activation_step(default_report, ld::activation_step_kind::windows_default_apps_ui),
        "Windows default application dry-run should point to Default Apps settings");

    ld::url_scheme_handler handler;
    handler.scheme = "ld2026";
    handler.desktop_entry_id = "org.linuxdesktop2026.DesktopTests";
    const auto protocol_report = ld::apply_url_scheme_handler(handler, options);
    require(protocol_report.ok, "Windows URL protocol dry-run should succeed without Registry mutation");
    require(has_diagnostic(protocol_report.diagnostics, "desktop.windows.url-protocol.registry-layer-required"),
        "Windows URL protocol dry-run should name the Registry-layer dependency");

    auto global_entry = desktop_entry_for_tests(std::filesystem::temp_directory_path() / "ld-desktop-test");
    global_entry.user_scope = false;
    const auto denied = ld::apply_desktop_entry(global_entry, options);
    require(!denied.ok, "Windows global app identity writes should require explicit global permission");
    require(has_diagnostic(denied.diagnostics, "desktop-entry-global-write-denied"),
        "Windows global app identity denial should be reported before any backend write path");
#endif
}

void autostart_dry_run_does_not_write()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart-dry-run";
    const auto entry = autostart_entry_for_tests();

    ld::apply_options options;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto report = ld::apply_autostart(entry, options);
    require(report.ok, "desktop autostart dry-run should succeed");
    require(report.dry_run, "desktop autostart dry-run should report dry_run");
    require(report.path.has_value(), "desktop autostart dry-run should report target path");
    require(!std::filesystem::exists(*report.path), "desktop autostart dry-run should not write a file");
    require(has_diagnostic(report.diagnostics, "autostart-dry-run"), "desktop autostart dry-run should include a diagnostic");
#endif
}

void autostart_reports_sanitized_ids_and_escaped_arguments()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart-sanitized";
    auto entry = autostart_entry_for_tests();
    entry.id = "linuxdesktop2026/desktop:tests";
    entry.display_name = "LinuxDesktop2026\nDesktop Tests";
    entry.arguments = {"--profile", "O'Brien"};

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto report = ld::apply_autostart(entry, options);

    require(report.ok, "desktop autostart should accept sanitized ids");
    require(report.path.has_value(), "desktop autostart should report sanitized path");
    require(report.path->filename() == "linuxdesktop2026-desktop-tests.desktop",
        "desktop autostart should sanitize path separators in ids");
    require(has_diagnostic(report.diagnostics, "autostart-id-sanitized"),
        "desktop autostart should diagnose sanitized ids");
    const auto content = read_file(*report.path);
    require(content.find("Name=LinuxDesktop2026\\nDesktop Tests") != std::string::npos,
        "desktop autostart should escape desktop-file newlines");
    require(content.find("Exec=/usr/bin/ld-desktop-test --profile \"O'Brien\"") != std::string::npos,
        "desktop autostart should use desktop-entry quotes for arguments with apostrophes");
#endif
}

void autostart_exec_uses_freedesktop_argument_grammar()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart-exec-grammar";
    auto entry = autostart_entry_for_tests();
    entry.executable = root / "Program Files" / "ld desktop test 日本語";
    entry.arguments = {
        "--plain",
        "Default User",
        "O'Brien",
        "he said \"hi\"",
        R"(C:\Users\Test)",
        "$HOME",
        "`date`",
        "100%",
        "one;two",
        "日本語",
        "",
    };

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root / "autostart";

    const auto report = ld::apply_autostart(entry, options);
    require(report.ok, "desktop autostart should write entry with freedesktop Exec tokens");
    require(report.path.has_value(), "desktop autostart should report path for Exec grammar test");

    const auto content = read_file(*report.path);
    const std::vector<std::string> expected_tokens = {
        "Exec=\"" + entry.executable.string() + "\"",
        "--plain",
        "\"Default User\"",
        "\"O'Brien\"",
        "\"he said \\\"hi\\\"\"",
        R"("C:\\Users\\Test")",
        R"("\$HOME")",
        R"("\`date\`")",
        "100%%",
        "\"one;two\"",
        "日本語",
        "\"\"",
    };
    for (const auto& token : expected_tokens) {
        require_contains(content, token, "desktop autostart Exec should format every adversarial token");
    }
    require(content.find("'Default User'") == std::string::npos,
        "desktop autostart Exec should not use shell-style single quotes");
#endif
}

void autostart_rejects_relative_or_file_backed_output_directory()
{
#if !defined(_WIN32)
    auto entry = autostart_entry_for_tests();

    ld::apply_options relative;
    relative.dry_run = false;
    relative.allow_desktop_integration_write = true;
    relative.autostart_directory_override = "relative-autostart";

    const auto relative_report = ld::apply_autostart(entry, relative);
    require(!relative_report.ok, "desktop autostart should reject relative output directories");
    require(has_diagnostic(relative_report.diagnostics, "autostart-directory-relative"),
        "desktop autostart should diagnose relative output directories");

    const auto root = test_root() / "autostart-output-file";
    std::filesystem::create_directories(root.parent_path());
    {
        std::ofstream file(root);
        file << "not a directory\n";
    }

    ld::apply_options file_backed;
    file_backed.dry_run = false;
    file_backed.allow_desktop_integration_write = true;
    file_backed.autostart_directory_override = root;

    const auto file_report = ld::apply_autostart(entry, file_backed);
    require(!file_report.ok, "desktop autostart should reject file-backed output directories");
    require(has_diagnostic(file_report.diagnostics, "autostart-create-directory-failed"),
        "desktop autostart should diagnose file-backed output directories");
#endif
}

void autostart_linux_writes_queries_and_removes_desktop_file()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart";
    const auto entry = autostart_entry_for_tests();

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto applied = ld::apply_autostart(entry, options);
    require(applied.ok, "desktop Linux autostart write should succeed");
    require(applied.path.has_value(), "desktop Linux autostart write should report path");
    const auto content = read_file(*applied.path);
    require(content.find("[Desktop Entry]") != std::string::npos, "desktop autostart file should be a desktop entry");
    require(content.find("Name=LinuxDesktop2026 Desktop Tests") != std::string::npos, "desktop autostart file should include display name");
    require(content.find("Exec=/usr/bin/ld-desktop-test --profile \"Default User\"") != std::string::npos, "desktop autostart file should quote Exec arguments");

    auto queried = ld::query_autostart(entry, options);
    require(queried.ok, "desktop Linux autostart query should succeed");
    require(queried.enabled, "desktop Linux autostart query should report enabled file");

    auto disabled_entry = entry;
    disabled_entry.enabled = false;
    const auto disabled = ld::apply_autostart(disabled_entry, options);
    require(disabled.ok, "desktop Linux disabled autostart write should succeed");
    queried = ld::query_autostart(entry, options);
    require(queried.ok, "desktop Linux disabled autostart query should succeed");
    require(!queried.enabled, "desktop Hidden=true autostart file should query as disabled");

    const auto removed = ld::remove_autostart(entry, options);
    require(removed.ok, "desktop Linux autostart remove should succeed");
    require(!std::filesystem::exists(*applied.path), "desktop Linux autostart remove should delete the file");
#endif
}

void autostart_linux_removal_only_deletes_generated_entry()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart-isolation";
    const auto entry = autostart_entry_for_tests();

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto applied = ld::apply_autostart(entry, options);
    require(applied.ok, "desktop Linux autostart write should succeed before isolated removal");
    require(applied.path.has_value(), "desktop Linux autostart write should report generated path");

    const auto sibling = root / "other-product.desktop";
    {
        std::ofstream file(sibling);
        file << "[Desktop Entry]\nName=Other Product\n";
    }

    const auto removed = ld::remove_autostart(entry, options);
    require(removed.ok, "desktop Linux autostart isolated remove should succeed");
    require(!std::filesystem::exists(*applied.path), "desktop Linux autostart remove should delete generated entry");
    require(read_file(sibling).find("Other Product") != std::string::npos,
        "desktop Linux autostart remove should leave sibling desktop entries untouched");
#endif
}

void autostart_atomic_write_cleans_temp_after_replace_failure()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart-atomic-replace-failure";
    const auto entry = autostart_entry_for_tests();
    const auto target = root / "linuxdesktop2026-desktop-tests.desktop";
    std::filesystem::create_directories(target);

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto report = ld::apply_autostart(entry, options);
    require(!report.ok, "desktop autostart should fail when the target path is a directory");
    require(has_diagnostic(report.diagnostics, "atomic-replace-failed"),
        "desktop autostart should surface atomic replace failures");
    require(has_diagnostic(report.diagnostics, "autostart-write-failed"),
        "desktop autostart should keep its effect-specific write diagnostic");
    require(std::filesystem::is_directory(target), "failed autostart replace should preserve existing target");
    require(!has_temp_sibling(target), "failed autostart replace should clean temporary files");
#endif
}

void autostart_linux_routes_config_home_through_paths()
{
#if !defined(_WIN32)
    const auto config_home = test_root() / "xdg-config-home";
    scoped_env_var config_home_env("XDG_CONFIG_HOME", config_home.string());

    const auto entry = autostart_entry_for_tests();
    ld::apply_options options;
    const auto queried = ld::query_autostart(entry, options);

    require(queried.ok, "desktop autostart query should resolve an XDG config home path");
    require(queried.path.has_value(), "desktop autostart query should report the resolved path");
    require(
        *queried.path == config_home / "autostart" / "linuxdesktop2026-desktop-tests.desktop",
        "desktop autostart query should route XDG config-home selection through ld_paths");
#endif
}

void desktop_flavors_share_standard_xdg_autostart_contract()
{
#if !defined(_WIN32)
    struct flavor_case {
        const char* name;
        const char* xdg_current_desktop;
    };

    const flavor_case flavors[] = {
        {"xdg_full_gnome", "ubuntu:GNOME"},
        {"xdg_full_kde", "KDE"},
        {"xdg_light_xfce", "XFCE"},
        {"xdg_minimal_bare_wm", "i3"},
    };

    for (const auto& flavor : flavors) {
        const auto config_home = test_root() / "desktop-flavors" / flavor.name / "config";
        scoped_env_var config_home_env("XDG_CONFIG_HOME", config_home.string());
        scoped_env_var desktop_env("XDG_CURRENT_DESKTOP", flavor.xdg_current_desktop);

        const auto entry = autostart_entry_for_tests();
        const auto queried = ld::query_autostart(entry);

        require(queried.ok, "Desktop Flavor autostart query should resolve the XDG autostart artifact");
        require(queried.path.has_value(), "Desktop Flavor autostart query should report the artifact path");
        require(
            *queried.path == config_home / "autostart" / "linuxdesktop2026-desktop-tests.desktop",
            "Desktop Flavors should share the standard XDG autostart artifact location");
    }
#endif
}

void desktop_flavor_capabilities_do_not_create_per_desktop_backends()
{
#if !defined(_WIN32)
    struct flavor_case {
        const char* name;
        const char* xdg_current_desktop;
    };

    const flavor_case flavors[] = {
        {"xdg_full_gnome", "ubuntu:GNOME"},
        {"xdg_full_kde", "KDE"},
        {"xdg_light_xfce", "XFCE"},
        {"xdg_minimal_bare_wm", "i3"},
    };

    const ld::effect_kind registration_artifacts[] = {
        ld::effect_kind::desktop_entry,
        ld::effect_kind::icon,
        ld::effect_kind::mime_association,
        ld::effect_kind::default_application,
        ld::effect_kind::url_protocol_handler,
        ld::effect_kind::desktop_database,
    };

    for (const auto& flavor : flavors) {
        scoped_env_var desktop_env("XDG_CURRENT_DESKTOP", flavor.xdg_current_desktop);
        const auto report = ld::query_capabilities();

        for (const auto kind : registration_artifacts) {
            const auto* capability = find_capability(report, kind);
            require(capability != nullptr, "Desktop Flavor capability report should include every registration artifact");
            require(capability->state == ld::capability_state::supported,
                "Staged registration artifacts should report standards-backed Linux support");
            require(!capability->can_write_user,
                "Staged registration artifacts should still require explicit write permission");
        }

        const auto* shell = find_capability(report, ld::effect_kind::shell_integration);
        require(shell != nullptr, "Desktop Flavor capability report should include shell integration");
        require(shell->state == ld::capability_state::backend_missing,
            "Runtime shell integration should stay outside the registration-artifact tranche");
    }
#endif
}

void xdg_desktop_entry_writes_queries_removes_and_reports_activation_plan()
{
#if !defined(_WIN32)
    const auto root = test_root() / "desktop-entry";
    const auto entry = desktop_entry_for_tests();

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.applications_directory_override = root / "applications";

    const auto applied = ld::apply_desktop_entry(entry, options);
    require(applied.ok, "desktop entry write should succeed");
    require(applied.present, "desktop entry write should report a present artifact");
    require(applied.path.has_value(), "desktop entry write should report path");
    require(!applied.activation_plan.empty(), "desktop entry write should return activation plan");
    require(applied.activation_plan.front().kind == ld::activation_step_kind::refresh_desktop_database,
        "desktop entry write should plan desktop database refresh");

    const auto content = read_file(*applied.path);
    require(content.find("Name=LinuxDesktop2026 Desktop Tests") != std::string::npos, "desktop entry should include name");
    require(content.find("Comment=Desktop\\nregistration") != std::string::npos, "desktop entry should escape newlines");
    require(content.find("MimeType=text/x-linuxdesktop2026-test;x-scheme-handler/ld2026;") != std::string::npos,
        "desktop entry should include MIME metadata");

    const auto queried = ld::query_desktop_entry(entry, options);
    require(queried.ok && queried.present, "desktop entry query should find staged file");

    const auto removed = ld::remove_desktop_entry(entry, options);
    require(removed.ok, "desktop entry remove should succeed");
    require(!std::filesystem::exists(*applied.path), "desktop entry remove should delete staged file");
#endif
}

void xdg_registration_rejects_hostile_ids_names_paths_and_global_writes()
{
#if !defined(_WIN32)
    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.applications_directory_override = test_root() / "hostile" / "applications";

    auto hostile_entry = desktop_entry_for_tests();
    hostile_entry.id = "../bad";
    hostile_entry.working_directory = "relative";
    const auto entry_report = ld::apply_desktop_entry(hostile_entry, options);
    require(!entry_report.ok, "desktop entry should reject hostile ids and relative paths");
    require(has_diagnostic(entry_report.diagnostics, "desktop-entry-id-invalid"), "desktop entry should diagnose hostile id");
    require(has_diagnostic(entry_report.diagnostics, "desktop-entry-working-directory-relative"),
        "desktop entry should diagnose relative working directory");

    auto global_entry = desktop_entry_for_tests();
    global_entry.user_scope = false;
    const auto global_report = ld::apply_desktop_entry(global_entry, options);
    require(!global_report.ok, "desktop entry global write should require explicit permission");
    require(has_diagnostic(global_report.diagnostics, "desktop-entry-global-write-denied"),
        "desktop entry global write should diagnose missing global permission");

    ld::mime_declaration hostile_mime = mime_declaration_for_tests();
    hostile_mime.name = "bad/../name";
    hostile_mime.glob_patterns = {"../*.bad"};
    const auto mime_report = ld::apply_mime_declaration(hostile_mime, options);
    require(!mime_report.ok, "MIME declaration should reject hostile names and path-like globs");
    require(has_diagnostic(mime_report.diagnostics, "mime-name-invalid"), "MIME declaration should diagnose hostile name");
    require(has_diagnostic(mime_report.diagnostics, "mime-glob-invalid"), "MIME declaration should diagnose path-like glob");

    ld::url_scheme_handler handler;
    handler.scheme = "1bad";
    handler.desktop_entry_id = "org.linuxdesktop2026.DesktopTests";
    const auto scheme_report = ld::apply_url_scheme_handler(handler, options);
    require(!scheme_report.ok, "URL handler should reject invalid schemes");
    require(has_diagnostic(scheme_report.diagnostics, "url-scheme-invalid"), "URL handler should diagnose invalid scheme");
#endif
}

void xdg_icon_stages_copy_and_reports_cache_activation_plan()
{
#if !defined(_WIN32)
    const auto root = test_root() / "icon";
    const auto source = root / "source.png";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source, std::ios::binary);
        file << "png-ish";
    }

    const auto entry = icon_entry_for_tests(source);
    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.icons_directory_override = root / "icons";

    const auto applied = ld::apply_icon(entry, options);
    require(applied.ok, "icon stage should copy source icon");
    require(applied.path.has_value(), "icon stage should report target path");
    require(applied.path->filename() == "org.linuxdesktop2026.DesktopTests.png", "icon stage should preserve source extension");
    require(read_file(*applied.path) == "png-ish", "icon stage should copy source bytes");
    require(!applied.activation_plan.empty() && applied.activation_plan.front().kind == ld::activation_step_kind::refresh_icon_cache,
        "icon stage should plan icon cache refresh");

    auto relative = entry;
    relative.source_path = "relative.png";
    const auto relative_report = ld::apply_icon(relative, options);
    require(!relative_report.ok, "icon stage should reject relative source paths");
    require(has_diagnostic(relative_report.diagnostics, "icon-source-path-invalid"), "icon stage should diagnose relative source paths");
#endif
}

void xdg_mime_declaration_stages_xml_and_reports_activation_plan()
{
#if !defined(_WIN32)
    const auto root = test_root() / "mime-declaration";
    const auto declaration = mime_declaration_for_tests();
    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.mime_packages_directory_override = root / "mime" / "packages";

    const auto applied = ld::apply_mime_declaration(declaration, options);
    require(applied.ok, "MIME declaration write should succeed");
    require(applied.path.has_value(), "MIME declaration should report path");
    require(read_file(*applied.path).find("<mime-type type=\"text/x-linuxdesktop2026-test\">") != std::string::npos,
        "MIME declaration should write shared-mime-info XML");
    require(!applied.activation_plan.empty() && applied.activation_plan.front().kind == ld::activation_step_kind::refresh_mime_database,
        "MIME declaration should plan MIME database refresh");

    const auto queried = ld::query_mime_declaration(declaration, options);
    require(queried.ok && queried.present, "MIME declaration query should find staged XML");

    const auto removed = ld::remove_mime_declaration(declaration, options);
    require(removed.ok, "MIME declaration remove should succeed");
    require(!std::filesystem::exists(*applied.path), "MIME declaration remove should delete staged XML");
#endif
}

void xdg_mimeapps_updates_preserve_unrelated_entries_and_deduplicate()
{
#if !defined(_WIN32)
    const auto root = test_root() / "mimeapps";
    const auto mimeapps = root / "mimeapps.list";
    std::filesystem::create_directories(root);
    {
        std::ofstream file(mimeapps);
        file << "[Added Associations]\n";
        file << "text/plain=other.desktop;\n";
        file << "[Default Applications]\n";
        file << "image/png=image-viewer.desktop;\n";
    }

    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.mimeapps_file_override = mimeapps;

    ld::mime_association association;
    association.mime_type = "text/x-linuxdesktop2026-test";
    association.desktop_entry_ids = {"org.linuxdesktop2026.DesktopTests", "org.linuxdesktop2026.DesktopTests.desktop"};
    const auto first = ld::apply_mime_association(association, options);
    const auto second = ld::apply_mime_association(association, options);
    require(first.ok && second.ok, "MIME association repeated writes should succeed");

    const auto content = read_file(mimeapps);
    require(content.find("text/plain=other.desktop;") != std::string::npos, "mimeapps update should preserve unrelated association");
    require(content.find("image/png=image-viewer.desktop;") != std::string::npos, "mimeapps update should preserve unrelated default");
    require(content.find("text/x-linuxdesktop2026-test=org.linuxdesktop2026.DesktopTests.desktop;") != std::string::npos,
        "mimeapps update should add and deduplicate desktop ids");

    const auto queried = ld::query_mime_association(association, options);
    require(queried.ok && queried.present, "MIME association query should find staged association");

    const auto removed = ld::remove_mime_association(association, options);
    require(removed.ok, "MIME association removal should succeed");
    require(read_file(mimeapps).find("text/x-linuxdesktop2026-test=") == std::string::npos,
        "MIME association removal should remove only targeted association");
#endif
}

void xdg_default_application_and_url_scheme_stage_mimeapps_entries()
{
#if !defined(_WIN32)
    const auto mimeapps = test_root() / "defaults" / "mimeapps.list";
    ld::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.mimeapps_file_override = mimeapps;

    ld::default_application_intent intent;
    intent.mime_type_or_scheme = "text/x-linuxdesktop2026-test";
    intent.desktop_entry_id = "org.linuxdesktop2026.DesktopTests";
    intent.make_default = true;
    const auto default_report = ld::apply_default_application(intent, options);
    require(default_report.ok, "default application write should succeed");
    require(ld::query_default_application(intent, options).present, "default application query should find staged default");

    ld::url_scheme_handler handler;
    handler.scheme = "ld2026";
    handler.desktop_entry_id = "org.linuxdesktop2026.DesktopTests";
    const auto scheme_report = ld::apply_url_scheme_handler(handler, options);
    require(scheme_report.ok, "URL scheme handler write should succeed");
    require(ld::query_url_scheme_handler(handler, options).present, "URL scheme query should find x-scheme-handler default");

    const auto content = read_file(mimeapps);
    require(content.find("text/x-linuxdesktop2026-test=org.linuxdesktop2026.DesktopTests.desktop;") != std::string::npos,
        "default application should update Default Applications");
    require(content.find("x-scheme-handler/ld2026=org.linuxdesktop2026.DesktopTests.desktop;") != std::string::npos,
        "URL scheme handler should update x-scheme-handler default");

    require(ld::remove_url_scheme_handler(handler, options).ok, "URL scheme removal should succeed");
    require(!ld::query_url_scheme_handler(handler, options).present, "URL scheme removal should clear targeted default");
#endif
}

void desktop_flavors_share_standard_xdg_registration_paths()
{
#if !defined(_WIN32)
    struct flavor_case {
        const char* name;
        const char* xdg_current_desktop;
    };

    const flavor_case flavors[] = {
        {"xdg_full_gnome", "ubuntu:GNOME"},
        {"xdg_full_kde", "KDE"},
        {"xdg_light_xfce", "XFCE"},
        {"xdg_minimal_bare_wm", "i3"},
    };

    for (const auto& flavor : flavors) {
        const auto root = test_root() / "registration-flavors" / flavor.name;
        scoped_env_var config_home_env("XDG_CONFIG_HOME", (root / "config").string());
        scoped_env_var data_home_env("XDG_DATA_HOME", (root / "data").string());
        scoped_env_var desktop_env("XDG_CURRENT_DESKTOP", flavor.xdg_current_desktop);

        const auto entry = desktop_entry_for_tests();
        const auto desktop_report = ld::query_desktop_entry(entry);
        require(desktop_report.ok, "Desktop Flavor registration query should resolve desktop entry path");
        require(*desktop_report.path == root / "data" / "applications" / "org.linuxdesktop2026.DesktopTests.desktop",
            "Desktop Flavors should share the standard XDG desktop entry location");

        const auto mimeapps_report = ld::query_default_application({"text/x-linuxdesktop2026-test", "org.linuxdesktop2026.DesktopTests", true, true});
        require(mimeapps_report.ok, "Desktop Flavor registration query should resolve mimeapps path");
        require(*mimeapps_report.path == root / "config" / "mimeapps.list",
            "Desktop Flavors should share the standard XDG mimeapps.list location");
    }
#endif
}

void policy_write_requires_explicit_permission()
{
    auto entry = policy_entry_for_tests();
    ld::apply_options options;
    options.policy_defaults_directory_override = test_root() / "dconf" / "defaults";

    const auto report = ld::apply_policy(entry, options);
    require(!report.ok, "desktop policy write should be denied without permission");
    require(has_diagnostic(report.diagnostics, "policy-write-denied"), "desktop policy write should require allow_policy_write");
}

void policy_reports_sanitized_ids_and_rejects_malformed_directories()
{
#if !defined(_WIN32)
    auto entry = policy_entry_for_tests();
    entry.id = "desktop/tests:theme";
    entry.enforced = true;

    const auto root = test_root() / "policy-sanitized";
    ld::apply_options sanitized;
    sanitized.dry_run = false;
    sanitized.allow_policy_write = true;
    sanitized.policy_defaults_directory_override = root / "defaults";
    sanitized.policy_locks_directory_override = root / "locks";

    const auto sanitized_report = ld::apply_policy(entry, sanitized);
    require(sanitized_report.ok, "desktop policy should accept sanitized ids");
    require(sanitized_report.path.has_value(), "desktop policy should report sanitized defaults path");
    require(sanitized_report.path->filename() == "desktop-tests-theme.conf",
        "desktop policy should sanitize path separators in ids");
    require(has_diagnostic(sanitized_report.diagnostics, "policy-id-sanitized"),
        "desktop policy should diagnose sanitized ids");

    ld::apply_options relative_defaults;
    relative_defaults.dry_run = false;
    relative_defaults.allow_policy_write = true;
    relative_defaults.policy_defaults_directory_override = "relative-defaults";
    const auto relative_defaults_report = ld::apply_policy(policy_entry_for_tests(), relative_defaults);
    require(!relative_defaults_report.ok, "desktop policy should reject relative defaults directories");
    require(has_diagnostic(relative_defaults_report.diagnostics, "policy-defaults-directory-relative"),
        "desktop policy should diagnose relative defaults directories");

    ld::apply_options relative_locks;
    relative_locks.dry_run = false;
    relative_locks.allow_policy_write = true;
    relative_locks.policy_defaults_directory_override = root / "relative-lock-defaults";
    relative_locks.policy_locks_directory_override = "relative-locks";
    auto enforced = policy_entry_for_tests();
    enforced.enforced = true;

    const auto relative_locks_report = ld::apply_policy(enforced, relative_locks);
    require(!relative_locks_report.ok, "desktop policy should reject relative lock directories");
    require(has_diagnostic(relative_locks_report.diagnostics, "policy-locks-directory-relative"),
        "desktop policy should diagnose relative lock directories");
    require(!std::filesystem::exists(root / "relative-lock-defaults" / "desktop-tests-theme.conf"),
        "desktop policy should not write defaults before rejecting malformed lock directories");
#endif
}

void policy_linux_writes_queries_and_removes_dconf_files()
{
#if !defined(_WIN32)
    auto entry = policy_entry_for_tests();
    entry.enforced = true;

    const auto root = test_root() / "dconf";
    ld::apply_options options;
    options.dry_run = false;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = root / "defaults";
    options.policy_locks_directory_override = root / "locks";

    const auto applied = ld::apply_policy(entry, options);
    require(applied.ok, "desktop Linux policy write should succeed");
    require(applied.path.has_value(), "desktop Linux policy write should report defaults path");
    require(has_diagnostic(applied.diagnostics, "policy-dconf-activation-required"),
        "desktop policy write should diagnose that dconf activation is still manual");
    require(read_file(*applied.path).find("[org/linuxdesktop2026/desktop-tests]") != std::string::npos, "desktop policy file should include dconf group");
    require(read_file(*applied.path).find("theme='dark'") != std::string::npos, "desktop policy file should include value");

    const auto queried = ld::query_policy(entry, options);
    require(queried.ok, "desktop Linux policy query should succeed");
    require(has_diagnostic(queried.diagnostics, "policy-dconf-activation-required"),
        "desktop policy query should diagnose that it reads generated files, not active dconf state");
    require(queried.present, "desktop Linux policy query should report present value");
    require(queried.enforced, "desktop Linux policy query should report lock file");
    require(queried.value.has_value() && *queried.value == "'dark'", "desktop Linux policy query should return value literal");

    const auto removed = ld::remove_policy(entry, options);
    require(removed.ok, "desktop Linux policy removal should succeed");
    require(!std::filesystem::exists(*applied.path), "desktop Linux policy removal should remove defaults file");
    require(!std::filesystem::exists(root / "locks" / "desktop-tests-theme.conf"),
        "desktop Linux policy removal should remove matching lock file");
#endif
}

void policy_atomic_write_cleans_temp_after_defaults_replace_failure()
{
#if !defined(_WIN32)
    auto entry = policy_entry_for_tests();
    const auto root = test_root() / "policy-defaults-atomic-replace-failure";
    const auto target = root / "defaults" / "desktop-tests-theme.conf";
    std::filesystem::create_directories(target);

    ld::apply_options options;
    options.dry_run = false;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = root / "defaults";

    const auto report = ld::apply_policy(entry, options);
    require(!report.ok, "desktop policy should fail when the defaults path is a directory");
    require(has_diagnostic(report.diagnostics, "atomic-replace-failed"),
        "desktop policy defaults should surface atomic replace failures");
    require(has_diagnostic(report.diagnostics, "policy-write-failed"),
        "desktop policy defaults should keep its effect-specific write diagnostic");
    require(std::filesystem::is_directory(target), "failed policy defaults replace should preserve existing target");
    require(!has_temp_sibling(target), "failed policy defaults replace should clean temporary files");
#endif
}

void policy_atomic_write_cleans_temp_after_lock_replace_failure()
{
#if !defined(_WIN32)
    auto entry = policy_entry_for_tests();
    entry.enforced = true;

    const auto root = test_root() / "policy-lock-atomic-replace-failure";
    const auto lock_target = root / "locks" / "desktop-tests-theme.conf";
    std::filesystem::create_directories(lock_target);

    ld::apply_options options;
    options.dry_run = false;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = root / "defaults";
    options.policy_locks_directory_override = root / "locks";

    const auto report = ld::apply_policy(entry, options);
    require(!report.ok, "desktop policy should fail when the lock path is a directory");
    require(has_diagnostic(report.diagnostics, "atomic-replace-failed"),
        "desktop policy locks should surface atomic replace failures");
    require(has_diagnostic(report.diagnostics, "policy-lock-write-failed"),
        "desktop policy locks should keep its effect-specific write diagnostic");
    require(std::filesystem::is_directory(lock_target), "failed policy lock replace should preserve existing target");
    require(!has_temp_sibling(lock_target), "failed policy lock replace should clean temporary files");
#endif
}

int main()
{
    capability_report_covers_extraction_scope();
    desktop_bundle_vocabulary_covers_current_registration_groups();
    managed_policy_capability_reports_dconf_activation_limit();
    windows_registration_capabilities_report_limited_native_mapping();
    windows_registration_dry_runs_report_limits_without_mutation();
    autostart_dry_run_does_not_write();
    autostart_reports_sanitized_ids_and_escaped_arguments();
    autostart_exec_uses_freedesktop_argument_grammar();
    autostart_rejects_relative_or_file_backed_output_directory();
    autostart_linux_writes_queries_and_removes_desktop_file();
    autostart_linux_removal_only_deletes_generated_entry();
    autostart_atomic_write_cleans_temp_after_replace_failure();
    autostart_linux_routes_config_home_through_paths();
    desktop_flavors_share_standard_xdg_autostart_contract();
    desktop_flavor_capabilities_do_not_create_per_desktop_backends();
    desktop_bundle_applies_queries_and_removes_staged_artifacts();
    desktop_bundle_partial_failure_preserves_per_effect_diagnostics();
    desktop_bundle_cleanup_reports_are_idempotent_and_parent_safe();
    xdg_desktop_entry_writes_queries_removes_and_reports_activation_plan();
    xdg_registration_rejects_hostile_ids_names_paths_and_global_writes();
    xdg_icon_stages_copy_and_reports_cache_activation_plan();
    xdg_mime_declaration_stages_xml_and_reports_activation_plan();
    xdg_mimeapps_updates_preserve_unrelated_entries_and_deduplicate();
    xdg_default_application_and_url_scheme_stage_mimeapps_entries();
    desktop_flavors_share_standard_xdg_registration_paths();
    policy_write_requires_explicit_permission();
    policy_reports_sanitized_ids_and_rejects_malformed_directories();
    policy_linux_writes_queries_and_removes_dconf_files();
    policy_atomic_write_cleans_temp_after_defaults_replace_failure();
    policy_atomic_write_cleans_temp_after_lock_replace_failure();
    return EXIT_SUCCESS;
}
