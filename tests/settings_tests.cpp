#include "linuxdesktop/settings.hpp"
#include "linuxdesktop/settings_c.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace ld = linuxdesktop::settings;

struct test_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw test_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-settings-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

ld::app_identity identity()
{
    ld::app_identity value;
    value.organization = "LinuxDesktop2026";
    value.application = "settings-tests";
    return value;
}

void exposes_cpp_version()
{
    require(ld::version_major == 0, "C++ version major should match project version");
    require(ld::version_minor == 1, "C++ version minor should match project version");
    require(ld::version_patch == 0, "C++ version patch should match project version");
}

void settings_diagnostics_use_shared_core_vocabulary()
{
    ld::diagnostic settings_diagnostic;
    settings_diagnostic.level = ld::severity::warning;
    settings_diagnostic.code = "shared-diagnostic";

    linuxdesktop::diagnostic core_diagnostic = settings_diagnostic;
    require(core_diagnostic.code == "shared-diagnostic", "settings diagnostics should alias shared diagnostics");
    require(linuxdesktop::to_string(core_diagnostic.level) == "warning", "shared severity should stringify");
    require(ld::to_string(settings_diagnostic.level) == "warning", "settings severity alias should stringify");
}

void writes_absolute_settings_override()
{
    const auto root = test_root() / "override";

    ld::root_options options;
    options.settings_override = root;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.settings_override_active, "settings override should be active");
    require(report.roots.config == root, "settings override should own config");
    require(report.roots.data == root, "settings override should own data");
    require(report.roots.state == root, "settings override should own state");
    require(report.roots.session == root / "sessions", "settings override should own session");
}

void rejects_relative_settings_override()
{
    ld::root_options options;
    options.settings_override = "relative-settings";

    const auto report = ld::resolve_app_roots(identity(), options);

    require(!report.settings_override_active, "relative settings override must not activate");
    require(has_diagnostic(report.diagnostics, "settings-override-relative"),
        "relative settings override should report a diagnostic");
}

void denies_portable_under_privileged_install_root()
{
    const auto root = test_root();
    const auto install = root / "Program Files" / "Notepad++";
    const auto marker = install / "doLocalConf.xml";
    std::filesystem::create_directories(install);
    {
        std::ofstream marker_file(marker);
        marker_file << "<localConf />\n";
    }

    ld::root_options options;
    options.resource_root = install;
    options.portable_marker = marker;
    options.privileged_install_roots = {root / "Program Files"};
    options.deny_portable_root_in_privileged_install = true;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.portable_requested, "portable marker should be requested");
    require(!report.portable_active, "portable root should be denied under privileged install root");
    require(has_diagnostic(report.diagnostics, "portable-denied-privileged-install"),
        "privileged portable denial should report a diagnostic");
}

void sync_config_override_keeps_state_local()
{
    const auto root = test_root();
    const auto sync = root / "sync-config";

    ld::root_options options;
    options.sync_config_override = sync;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.sync_config_override_active, "sync config override should be active");
    require(report.roots.config == sync, "sync config override should own config");
    require(report.roots.plugin_config == sync / "plugins" / "Config",
        "plugin config should follow config root");
    require(report.roots.state != sync, "state should stay local when only config is synced");
    require(report.roots.session == report.roots.state / "sessions", "session should follow state");
}

void rejects_relative_sync_config_override()
{
    ld::root_options options;
    options.sync_config_override = "relative-sync";

    const auto report = ld::resolve_app_roots(identity(), options);

    require(!report.sync_config_override_active, "relative sync config override must not activate");
    require(has_diagnostic(report.diagnostics, "sync-config-override-relative"),
        "relative sync config override should report a diagnostic");
}

void settings_override_wins_over_sync_override()
{
    const auto root = test_root();
    const auto settings = root / "settings";
    const auto sync = root / "sync";

    ld::root_options options;
    options.settings_override = settings;
    options.sync_config_override = sync;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.settings_override_active, "settings override should be active");
    require(!report.sync_config_override_active, "sync override should not activate over settings override");
    require(report.roots.config == settings, "settings override should win config");
    require(report.roots.state == settings, "settings override should win state");
    require(has_diagnostic(report.diagnostics, "sync-config-override-ignored"),
        "ignored sync override should report a diagnostic");
}

#if defined(_WIN32)
void windows_default_roots_are_resolved()
{
    ld::root_options options;
    options.create_directories = false;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(!report.roots.config.empty(), "Windows config root should be resolved");
    require(!report.roots.data.empty(), "Windows data root should be resolved");
    require(!report.roots.state.empty(), "Windows state root should be resolved");
    require(!report.roots.cache.empty(), "Windows cache root should be resolved");
    require(!report.roots.session.empty(), "Windows session root should be resolved");
}
#endif

void hydration_copies_missing_models()
{
    const auto root = test_root();
    const auto models = root / "models";
    const auto target = root / "config";
    std::filesystem::create_directories(models);
    {
        std::ofstream model(models / "config.model.xml");
        model << "<Config />\n";
    }

    ld::config_file file;
    file.name = "config.xml";
    file.model_name = "config.model.xml";
    file.required = true;

    ld::hydrate_options options;
    options.model_root = models;
    options.target_root = target;
    options.files = {file};

    const auto report = ld::hydrate_config_bundle(options);

    require(report.copied.size() == 1, "hydration should copy one model");
    require(std::filesystem::exists(target / "config.xml"), "hydration target should exist");
}

void atomic_write_replaces_target_with_backup()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    ld::write_options options;
    options.target = target;
    options.content = "<Config saved=\"new\" />\n";
    options.keep_backup = true;

    const auto report = ld::write_with_backup(options, [](const std::filesystem::path& path, std::string&) {
        return read_file(path).find("new") != std::string::npos;
    });

    require(report.ok, "valid atomic write should succeed");
    require(report.backup_path.has_value(), "valid atomic write should keep old target as backup");
    require(report.temp_path.has_value(), "atomic write should report temp path");
    require(!std::filesystem::exists(*report.temp_path), "atomic temp file should be replaced away");
    require(read_file(target).find("new") != std::string::npos, "target should contain new content");
    require(read_file(*report.backup_path).find("old") != std::string::npos, "backup should contain old content");
}

void atomic_validation_keeps_original_target()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    ld::write_options options;
    options.target = target;
    options.content = "";
    options.keep_backup = true;

    const auto report = ld::write_with_backup(options, [](const std::filesystem::path&, std::string& message) {
        message = "empty writes are invalid in this test";
        return false;
    });

    require(!report.ok, "invalid atomic write should fail");
    require(!report.backup_path.has_value(), "invalid atomic write should not need a backup");
    require(report.temp_path.has_value(), "invalid atomic write should report temp path");
    require(!std::filesystem::exists(*report.temp_path), "invalid atomic temp file should be cleaned");
    require(has_diagnostic(report.diagnostics, "temp-cleaned"), "invalid atomic write should report temp cleanup");
    require(read_file(target).find("old") != std::string::npos, "original target should stay untouched");
}

void direct_write_validation_restores_backup()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    ld::write_options options;
    options.target = target;
    options.content = "";
    options.keep_backup = true;
    options.atomic_replace = false;

    const auto report = ld::write_with_backup(options, [](const std::filesystem::path&, std::string& message) {
        message = "empty writes are invalid in this test";
        return false;
    });

    require(!report.ok, "invalid direct write should fail");
    require(report.backup_path.has_value(), "invalid direct write should have backup");
    require(has_diagnostic(report.diagnostics, "backup-restored"), "invalid direct write should restore backup");
    require(std::filesystem::file_size(target) > 0, "restored target should not be empty");
}

void c_abi_resolves_settings_override()
{
    const auto root = test_root() / "c-override";

    ld_settings_root_options options = {};
    ld_settings_root_options_init(&options);
    options.organization = "LinuxDesktop2026";
    options.application = "c-settings-tests";
    const auto root_text = root.u8string();
    options.settings_override = root_text.c_str();

    ld_settings_root_report report = {};
    const int ok = ld_settings_resolve_app_roots(&options, &report);

    require(ok == 1, "C ABI root resolution should succeed");
    require(report.settings_override_active == 1, "C ABI settings override should be active");
    require(report.config != nullptr, "C ABI config path should be allocated");
    require(std::filesystem::path(report.config) == root, "C ABI config path should match override");
    require(std::filesystem::path(report.session) == root / "sessions", "C ABI session path should match override");
    require(std::string(ld_settings_severity_name(LD_SETTINGS_SEVERITY_WARNING)) == "warning",
        "C ABI severity names should be stable");
    require(ld_settings_version_major() == LD_SETTINGS_VERSION_MAJOR,
        "C ABI runtime major should match header major");
    require(ld_settings_version_minor() == LD_SETTINGS_VERSION_MINOR,
        "C ABI runtime minor should match header minor");
    require(ld_settings_version_patch() == LD_SETTINGS_VERSION_PATCH,
        "C ABI runtime patch should match header patch");
    require(std::string(ld_settings_version_string()) == "0.1.0",
        "C ABI version string should match project version");

    ld_settings_free_root_report(&report);

    require(report.config == nullptr, "C ABI free should clear report");
    require(report.diagnostic_count == 0, "C ABI free should clear diagnostics");
}

} // namespace

int main()
{
    const std::vector<std::pair<const char*, void (*)()>> tests = {
        {"exposes_cpp_version", exposes_cpp_version},
        {"settings_diagnostics_use_shared_core_vocabulary", settings_diagnostics_use_shared_core_vocabulary},
        {"writes_absolute_settings_override", writes_absolute_settings_override},
        {"rejects_relative_settings_override", rejects_relative_settings_override},
        {"denies_portable_under_privileged_install_root", denies_portable_under_privileged_install_root},
        {"sync_config_override_keeps_state_local", sync_config_override_keeps_state_local},
        {"rejects_relative_sync_config_override", rejects_relative_sync_config_override},
        {"settings_override_wins_over_sync_override", settings_override_wins_over_sync_override},
#if defined(_WIN32)
        {"windows_default_roots_are_resolved", windows_default_roots_are_resolved},
#endif
        {"hydration_copies_missing_models", hydration_copies_missing_models},
        {"atomic_write_replaces_target_with_backup", atomic_write_replaces_target_with_backup},
        {"atomic_validation_keeps_original_target", atomic_validation_keeps_original_target},
        {"direct_write_validation_restores_backup", direct_write_validation_restores_backup},
        {"c_abi_resolves_settings_override", c_abi_resolves_settings_override},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "ok " << test.first << "\n";
        } catch (const test_failure& failure) {
            ++failures;
            std::cout << "not ok " << test.first << ": " << failure.message << "\n";
        } catch (const std::exception& ex) {
            ++failures;
            std::cout << "not ok " << test.first << ": unexpected exception: " << ex.what() << "\n";
        }
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
