#include "linuxdesktop/root.hpp"

#include <cstdlib>

int main()
{
    const auto logs_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_log_root_request(
            "logs",
            linuxdesktop::root::ownership_kind::user_local,
            "logs"));

    linuxdesktop::root::options options;
    options.create_directories = false;
    options.named_roots = {logs_root.request};

    const auto report = linuxdesktop::root::resolve_app_roots(
        {"LinuxDesktop2026", "root-consumer-smoke"},
        options);

    return report.roots.config.empty() ||
            linuxdesktop::root::find_named_root(report, logs_root) == nullptr
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}
