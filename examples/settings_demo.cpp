#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using linuxdesktop::settings::diagnostic;

struct cli_options {
    std::optional<std::filesystem::path> resource_root;
    std::optional<std::filesystem::path> settings_dir;
    std::optional<std::filesystem::path> sync_config_dir;
    std::optional<std::filesystem::path> portable_marker;
    std::vector<std::filesystem::path> privileged_install_roots;
    bool deny_portable_under_privileged_root = false;
    bool allow_sync_with_portable = false;
    std::filesystem::path model_root = LD2026_EXAMPLE_MODEL_ROOT;
};

void print_usage(const char* program)
{
    std::cout
        << "Usage: " << program << " [options]\n"
        << "\n"
        << "Runs the first LinuxDesktop2026 settings/config proof sample.\n"
        << "\n"
        << "Options:\n"
        << "  --resource-root PATH\n"
        << "  --settings-dir PATH\n"
        << "  --sync-config-dir PATH\n"
        << "  --portable-marker PATH\n"
        << "  --deny-portable-under-root PATH\n"
        << "  --allow-sync-with-portable\n"
        << "  --model-root PATH\n";
}

cli_options parse_args(int argc, char** argv)
{
    cli_options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("Missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--settings-dir") {
            options.settings_dir = require_value("--settings-dir");
            continue;
        }
        if (arg == "--sync-config-dir") {
            options.sync_config_dir = require_value("--sync-config-dir");
            continue;
        }
        if (arg == "--resource-root") {
            options.resource_root = require_value("--resource-root");
            continue;
        }
        if (arg == "--portable-marker") {
            options.portable_marker = require_value("--portable-marker");
            continue;
        }
        if (arg == "--deny-portable-under-root") {
            options.privileged_install_roots.push_back(require_value("--deny-portable-under-root"));
            options.deny_portable_under_privileged_root = true;
            continue;
        }
        if (arg == "--allow-sync-with-portable") {
            options.allow_sync_with_portable = true;
            continue;
        }
        if (arg == "--model-root") {
            options.model_root = require_value("--model-root");
            continue;
        }
        throw std::runtime_error("Unknown argument: " + arg);
    }
    return options;
}

void print_diagnostics(const std::vector<diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        std::cout << "diagnostic " << linuxdesktop::settings::to_string(item.level)
                  << " " << item.code << ": " << item.message;
        if (!item.path.empty()) {
            std::cout << " [" << item.path.string() << "]";
        }
        std::cout << "\n";
    }
}

void print_backup(const linuxdesktop::settings::write_report& report)
{
    if (report.backup_path) {
        std::cout << "    backup: " << report.backup_path->string() << "\n";
    }
}

bool validate_session_file(const std::filesystem::path& path, std::string& message)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        message = ec.message();
        return false;
    }
    if (size == 0) {
        message = "session-like file is empty";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const auto cli = parse_args(argc, argv);

        linuxdesktop::settings::root_options root_options;
        root_options.resource_root = cli.resource_root;
        root_options.settings_override = cli.settings_dir;
        root_options.sync_config_override = cli.sync_config_dir;
        root_options.portable_marker = cli.portable_marker;
        root_options.privileged_install_roots = cli.privileged_install_roots;
        root_options.deny_portable_root_in_privileged_install = cli.deny_portable_under_privileged_root;
        root_options.allow_sync_config_for_portable_root = cli.allow_sync_with_portable;

        linuxdesktop::settings::app_identity identity;
        identity.organization = "LinuxDesktop2026";
        identity.application = "settings-demo";

        const auto roots = linuxdesktop::settings::resolve_app_roots(identity, root_options);

        std::cout << "roots\n";
        std::cout << "  resources:     " << roots.roots.resources.string() << "\n";
        std::cout << "  config:        " << roots.roots.config.string() << "\n";
        std::cout << "  data:          " << roots.roots.data.string() << "\n";
        std::cout << "  state:         " << roots.roots.state.string() << "\n";
        std::cout << "  cache:         " << roots.roots.cache.string() << "\n";
        std::cout << "  runtime:       " << roots.roots.runtime.string() << "\n";
        std::cout << "  session:       " << roots.roots.session.string() << "\n";
        std::cout << "  plugin_config: " << roots.roots.plugin_config.string() << "\n";
        std::cout << "  portable:      " << (roots.portable_active ? "active" : "inactive") << "\n";
        std::cout << "  override:      " << (roots.settings_override_active ? "active" : "inactive") << "\n";
        std::cout << "  sync config:   " << (roots.sync_config_override_active ? "active" : "inactive") << "\n";
        print_diagnostics(roots.diagnostics);

        linuxdesktop::settings::config_file shortcuts_file;
        shortcuts_file.name = "shortcuts.xml";
        shortcuts_file.model_name = "shortcuts.model.xml";
        shortcuts_file.required = true;

        linuxdesktop::settings::config_file config_file;
        config_file.name = "config.xml";
        config_file.model_name = "config.model.xml";
        config_file.required = true;

        linuxdesktop::settings::hydrate_options hydrate_options;
        hydrate_options.model_root = cli.model_root;
        hydrate_options.target_root = roots.roots.config;
        hydrate_options.files = {shortcuts_file, config_file};

        const auto hydrated = linuxdesktop::settings::hydrate_config_bundle(hydrate_options);

        std::cout << "\nhydration\n";
        for (const auto& path : hydrated.copied) {
            std::cout << "  copied:  " << path.string() << "\n";
        }
        for (const auto& path : hydrated.skipped_existing) {
            std::cout << "  skipped: " << path.string() << "\n";
        }
        print_diagnostics(hydrated.diagnostics);

        linuxdesktop::settings::write_options shortcuts_options;
        shortcuts_options.target = roots.roots.config / "shortcuts.xml";
        shortcuts_options.content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Shortcuts saved=\"first\" />\n";
        shortcuts_options.keep_backup = true;

        linuxdesktop::settings::write_options config_options;
        config_options.target = roots.roots.config / "config.xml";
        config_options.content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Config shortcutsSaved=\"true\" />\n";
        config_options.keep_backup = true;

        linuxdesktop::settings::write_options session_options;
        session_options.target = roots.roots.session / "session.xml";
        session_options.content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Session files=\"0\" />\n";
        session_options.keep_backup = true;

        const auto shortcuts_write = linuxdesktop::settings::write_with_backup(shortcuts_options);
        const auto config_write = linuxdesktop::settings::write_with_backup(config_options);
        const auto session_write = linuxdesktop::settings::write_with_backup(session_options, validate_session_file);

        std::cout << "\nordered save\n";
        std::cout << "  shortcuts.xml: " << (shortcuts_write.ok ? "ok" : "failed") << "\n";
        print_backup(shortcuts_write);
        print_diagnostics(shortcuts_write.diagnostics);
        std::cout << "  config.xml:    " << (config_write.ok ? "ok" : "failed") << "\n";
        print_backup(config_write);
        print_diagnostics(config_write.diagnostics);
        std::cout << "  session.xml:   " << (session_write.ok ? "ok" : "failed") << "\n";
        print_backup(session_write);
        print_diagnostics(session_write.diagnostics);

        return shortcuts_write.ok && config_write.ok && session_write.ok ? 0 : 1;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 2;
    }
}
