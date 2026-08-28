#include "linuxdesktop/paths.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace ld = linuxdesktop::paths;

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

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, std::string_view code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

void exposes_cpp_version()
{
    require(ld::version_major == 0, "C++ version major should match project version");
    require(ld::version_minor == 1, "C++ version minor should match project version");
    require(ld::version_patch == 0, "C++ version patch should match project version");
}

void paths_diagnostics_use_shared_core_vocabulary()
{
    ld::diagnostic paths_diagnostic;
    paths_diagnostic.level = ld::severity::warning;
    paths_diagnostic.code = "shared-diagnostic";

    linuxdesktop::diagnostic core_diagnostic = paths_diagnostic;
    require(core_diagnostic.code == "shared-diagnostic", "paths diagnostics should alias shared diagnostics");
    require(linuxdesktop::to_string(core_diagnostic.level) == "warning", "shared severity should stringify");
    require(ld::to_string(paths_diagnostic.level) == "warning", "paths severity alias should stringify");
}

void stringifies_public_enums()
{
    require(ld::to_string(ld::path_family::config) == "config", "path family should stringify");
    require(ld::to_string(ld::path_family::public_share) == "public_share", "public share should stringify");
    require(ld::to_string(ld::candidate_source::known_folder) == "known_folder", "candidate source should stringify");
    require(ld::to_string(ld::candidate_source::xdg_base_dir) == "xdg_base_dir", "XDG base dir should stringify");
}

void resolver_skeleton_reports_unimplemented_behavior()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";

    const auto report = ld::resolve_app_paths(identity);

    require(report.selected.empty(), "resolver skeleton should not claim selected paths yet");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::resolver_not_implemented),
        "resolver skeleton should report that resolution is not implemented yet");
}

} // namespace

int main()
{
    try {
        exposes_cpp_version();
        paths_diagnostics_use_shared_core_vocabulary();
        stringifies_public_enums();
        resolver_skeleton_reports_unimplemented_behavior();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
