#include "linuxdesktop/paths.hpp"
#include "linuxdesktop2026_install_tree/path_defaults.hpp"

#include <cstdlib>
#include <filesystem>

namespace {

bool has_selected_platform_default(const linuxdesktop::paths::resolver_report& report)
{
    for (const auto& candidate : report.candidates) {
        if (candidate.selected &&
            candidate.source == linuxdesktop::paths::candidate_source::platform_default) {
            return true;
        }
    }
    return false;
}

bool has_home_missing_diagnostic(const linuxdesktop::paths::resolver_report& report)
{
    for (const auto& diagnostic : report.diagnostics) {
        if (diagnostic.code == linuxdesktop::paths::diagnostic_code::home_missing) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    linuxdesktop::paths::app_identity paths_identity;
    paths_identity.organization = "LinuxDesktop2026";
    paths_identity.application = "install-cmake-defaults";

    linuxdesktop::paths::resolver_options path_options;
    path_options.use_process_environment = false;
    path_options.platform_defaults =
        linuxdesktop2026::generated::platform_path_defaults_for_home(
            std::filesystem::temp_directory_path() / "linuxdesktop2026-install-consumer-home",
            std::filesystem::temp_directory_path() / "linuxdesktop2026-install-consumer-runtime");

    const auto paths_report = linuxdesktop::paths::resolve_app_paths(paths_identity, path_options);
    if (paths_report.selected.find(linuxdesktop::paths::path_family::config) ==
            paths_report.selected.end() ||
        has_home_missing_diagnostic(paths_report)) {
        return EXIT_FAILURE;
    }

#if !defined(_WIN32)
    if (!has_selected_platform_default(paths_report)) {
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
