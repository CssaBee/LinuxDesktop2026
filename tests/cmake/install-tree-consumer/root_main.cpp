#include "linuxdesktop/root.hpp"

#include <cstdlib>

int main()
{
    linuxdesktop::root::options options;
    options.create_directories = false;
    options.named_roots = {
        linuxdesktop::root::make_log_root_request(
            "logs",
            linuxdesktop::root::ownership_kind::user_local,
            "logs"),
    };

    const auto report = linuxdesktop::root::resolve_app_roots(
        {"LinuxDesktop2026", "root-consumer-smoke"},
        options);

    return report.roots.config.empty() ||
            linuxdesktop::root::find_named_root(report, "logs") == nullptr
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}
