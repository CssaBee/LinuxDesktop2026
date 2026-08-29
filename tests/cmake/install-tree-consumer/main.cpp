#include "linuxdesktop/settings.hpp"
#include "linuxdesktop/desktop.hpp"
#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/watch.hpp"

#include <cstdlib>
#include <string>

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
    if (linuxdesktop::watch::to_string(linuxdesktop::watch::event_kind::created) != "created") {
        return EXIT_FAILURE;
    }
    if (linuxdesktop::paths::to_string(linuxdesktop::paths::path_family::config) != "config") {
        return EXIT_FAILURE;
    }
    if (linuxdesktop::desktop::to_string(linuxdesktop::desktop::effect_kind::autostart) != "autostart") {
        return EXIT_FAILURE;
    }

    const auto report = linuxdesktop::settings::resolve_app_roots(identity, options);
    return report.roots.config.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
}
