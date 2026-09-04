#include "linuxdesktop/desktop.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

void autostart_dry_run_does_not_write()
{
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
    require(content.find("Exec=/usr/bin/ld-desktop-test --profile 'O'\\\\''Brien'") != std::string::npos,
        "desktop autostart should shell-quote arguments with apostrophes");
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
    require(content.find("Exec=/usr/bin/ld-desktop-test --profile 'Default User'") != std::string::npos, "desktop autostart file should quote Exec arguments");

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
    setenv("XDG_CONFIG_HOME", config_home.string().c_str(), 1);

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
    managed_policy_capability_reports_dconf_activation_limit();
    autostart_dry_run_does_not_write();
    autostart_reports_sanitized_ids_and_escaped_arguments();
    autostart_rejects_relative_or_file_backed_output_directory();
    autostart_linux_writes_queries_and_removes_desktop_file();
    autostart_linux_removal_only_deletes_generated_entry();
    autostart_atomic_write_cleans_temp_after_replace_failure();
    autostart_linux_routes_config_home_through_paths();
    policy_write_requires_explicit_permission();
    policy_reports_sanitized_ids_and_rejects_malformed_directories();
    policy_linux_writes_queries_and_removes_dconf_files();
    policy_atomic_write_cleans_temp_after_defaults_replace_failure();
    policy_atomic_write_cleans_temp_after_lock_replace_failure();
    return EXIT_SUCCESS;
}
