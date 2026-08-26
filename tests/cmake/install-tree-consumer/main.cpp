#include "linuxdesktop/settings.hpp"

#include <cstdlib>

int main()
{
    linuxdesktop::settings::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "consumer-smoke";

    linuxdesktop::settings::root_options options;
    options.create_directories = false;

    const auto report = linuxdesktop::settings::resolve_app_roots(identity, options);
    return report.roots.config.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
}
