#include "linuxdesktop/settings.hpp"

#include <cstdlib>

int main()
{
    linuxdesktop::settings::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "consumer-smoke";

    linuxdesktop::settings::root_options options;
    options.create_directories = false;

    linuxdesktop::diagnostic diagnostic;
    diagnostic.level = linuxdesktop::severity::warning;
    if (linuxdesktop::to_string(diagnostic.level) != "warning") {
        return EXIT_FAILURE;
    }

    const auto report = linuxdesktop::settings::resolve_settings_roots(identity, options);
    if (report.roots.config.empty()) {
        return EXIT_FAILURE;
    }

    linuxdesktop::settings::write_options write_options;
    write_options.keep_backup = true;
    write_options.atomic_replace = true;

    if (linuxdesktop::settings::to_string(linuxdesktop::settings::storage_backend::file) != "file") {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
