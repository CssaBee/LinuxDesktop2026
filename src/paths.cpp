#include "linuxdesktop/paths.hpp"

#include <string>
#include <utility>

namespace linuxdesktop::paths {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

} // namespace

std::string_view to_string(path_family value)
{
    switch (value) {
    case path_family::config:
        return "config";
    case path_family::data:
        return "data";
    case path_family::state:
        return "state";
    case path_family::cache:
        return "cache";
    case path_family::temp:
        return "temp";
    case path_family::documents:
        return "documents";
    case path_family::desktop:
        return "desktop";
    case path_family::downloads:
        return "downloads";
    case path_family::music:
        return "music";
    case path_family::pictures:
        return "pictures";
    case path_family::videos:
        return "videos";
    case path_family::public_share:
        return "public_share";
    case path_family::executable:
        return "executable";
    case path_family::executable_directory:
        return "executable_directory";
    case path_family::install_prefix:
        return "install_prefix";
    case path_family::resources:
        return "resources";
    case path_family::plugin_search:
        return "plugin_search";
    }
    return "unknown";
}

std::string_view to_string(candidate_source value)
{
    switch (value) {
    case candidate_source::explicit_option:
        return "explicit_option";
    case candidate_source::environment:
        return "environment";
    case candidate_source::xdg_base_dir:
        return "xdg_base_dir";
    case candidate_source::xdg_user_dir:
        return "xdg_user_dir";
    case candidate_source::known_folder:
        return "known_folder";
    case candidate_source::executable_relative:
        return "executable_relative";
    case candidate_source::legacy:
        return "legacy";
    case candidate_source::site_default:
        return "site_default";
    case candidate_source::fallback:
        return "fallback";
    }
    return "unknown";
}

resolver_report resolve_app_paths(const app_identity& identity, const resolver_options& options)
{
    (void)identity;
    (void)options;

    resolver_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::warning,
        std::string(diagnostic_code::resolver_not_implemented),
        "ld_paths resolver skeleton is present; platform resolution is not implemented yet"));
    return report;
}

} // namespace linuxdesktop::paths
