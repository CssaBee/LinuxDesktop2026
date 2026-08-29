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
    require(read_file(*applied.path).find("[org/linuxdesktop2026/desktop-tests]") != std::string::npos, "desktop policy file should include dconf group");
    require(read_file(*applied.path).find("theme='dark'") != std::string::npos, "desktop policy file should include value");

    const auto queried = ld::query_policy(entry, options);
    require(queried.ok, "desktop Linux policy query should succeed");
    require(queried.present, "desktop Linux policy query should report present value");
    require(queried.enforced, "desktop Linux policy query should report lock file");
    require(queried.value.has_value() && *queried.value == "'dark'", "desktop Linux policy query should return value literal");

    const auto removed = ld::remove_policy(entry, options);
    require(removed.ok, "desktop Linux policy removal should succeed");
    require(!std::filesystem::exists(*applied.path), "desktop Linux policy removal should remove defaults file");
#endif
}

int main()
{
    capability_report_covers_extraction_scope();
    autostart_dry_run_does_not_write();
    autostart_linux_writes_queries_and_removes_desktop_file();
    autostart_linux_routes_config_home_through_paths();
    policy_write_requires_explicit_permission();
    policy_linux_writes_queries_and_removes_dconf_files();
    return EXIT_SUCCESS;
}
