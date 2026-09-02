#include "gearcoleco_flavor.hpp"

#include "gearcoleco/generated/platform_path_defaults.hpp"

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <utility>

namespace flavor_tests::gearcoleco {

namespace {

EmulatorDiagnostic diagnostic(
    DiagnosticCode code,
    std::string message,
    bool fatal,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

void append_root_diagnostics(StartupPlan& plan, const linuxdesktop::settings::root_report& report)
{
    for (const auto& item : report.diagnostics) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
}

void append_hydration_diagnostics(StartupPlan& plan, const linuxdesktop::settings::hydrate_report& report)
{
    for (const auto& item : report.diagnostics) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::ConfigHydrationWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
}

std::vector<std::filesystem::path> symbol_candidates_for(const std::filesystem::path& rom)
{
    auto sym = rom;
    sym.replace_extension(".sym");
    auto noi = rom;
    noi.replace_extension(".noi");
    return {sym, noi};
}

} // namespace

StartupPlan DesktopFrontend::prepare(const RuntimeEnvironment& environment, const LaunchOptions& options) const
{
    const auto portable_marker = environment.executable_directory / "portable.ini";
    linuxdesktop::root::portable_root_request portable_root;
    portable_root.root = environment.executable_directory;
    portable_root.marker = portable_marker;
    portable_root.requested = options.portable;
    portable_root.level = linuxdesktop::root::portable_root_level::profile;

    linuxdesktop::settings::root_builder builder;
    builder.app("Gearcoleco", "Gearcoleco")
        .resource_root(environment.executable_directory)
        .home_directory(environment.home_directory)
        .environment(environment.environment)
        .use_process_environment(false)
        .portable(linuxdesktop::settings::portable_level::profile)
        .portable_root(portable_root);

    if (environment.home_directory) {
        builder.platform_defaults(linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory));
    }

    const auto roots = builder.resolve();

    StartupPlan plan;
    plan.portable_requested = roots.portable_requested;
    plan.portable_active = roots.portable_active;
    plan.config_root = roots.roots.config;
    plan.data_root = roots.roots.data;
    plan.controller_database = environment.executable_directory / "gamecontrollerdb.txt";
    plan.settings_file = plan.config_root / "gearcoleco.ini";
    append_root_diagnostics(plan, roots);

    linuxdesktop::settings::hydrate_options defaults;
    defaults.model_root = roots.roots.resources;
    defaults.target_root = plan.config_root;
    defaults.files = {
        {"gearcoleco.ini", "gearcoleco.ini", true},
    };

    const auto hydration = linuxdesktop::settings::ensure_config_defaults(defaults);
    plan.copied_defaults = hydration.copied;
    append_hydration_diagnostics(plan, hydration);

    if (options.rom_file) {
        plan.symbols.automatic = symbol_candidates_for(*options.rom_file);
    }
    if (options.symbol_file) {
        plan.symbols.requested = *options.symbol_file;
        if (std::filesystem::exists(*options.symbol_file)) {
            plan.symbols.selected = *options.symbol_file;
        } else {
            plan.diagnostics.push_back(diagnostic(
                DiagnosticCode::SymbolFileMissing,
                "requested debug symbol file is missing",
                false,
                *options.symbol_file));
        }
    } else {
        for (const auto& candidate : plan.symbols.automatic) {
            if (std::filesystem::exists(candidate)) {
                plan.symbols.selected = candidate;
                break;
            }
        }
    }

    return plan;
}

} // namespace flavor_tests::gearcoleco
