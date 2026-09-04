#include "settings_internal.hpp"
#include "durable_file_write.hpp"

#include <cstdlib>
#include <utility>

namespace linuxdesktop::settings {

std::string_view to_string(portable_level value)
{
    switch (value) {
    case portable_level::off:
        return "off";
    case portable_level::settings_only:
        return "settings_only";
    case portable_level::profile:
        return "profile";
    case portable_level::clean:
        return "clean";
    }
    return "unknown";
}

std::string_view to_string(config_layer_kind value)
{
    switch (value) {
    case config_layer_kind::defaults:
        return "defaults";
    case config_layer_kind::global:
        return "global";
    case config_layer_kind::user:
        return "user";
    case config_layer_kind::local:
        return "local";
    case config_layer_kind::portable:
        return "portable";
    case config_layer_kind::managed:
        return "managed";
    case config_layer_kind::enforced:
        return "enforced";
    }
    return "unknown";
}

std::string_view to_string(storage_backend value)
{
    switch (value) {
    case storage_backend::file:
        return "file";
    case storage_backend::registry:
        return "registry";
    case storage_backend::null_backend:
        return "null";
    case storage_backend::override_values:
        return "override_values";
    case storage_backend::app_callback:
        return "app_callback";
    }
    return "unknown";
}
hydrate_report ensure_config_defaults(const hydrate_options& options)
{
    hydrate_report report;
    if (options.create_target_root) {
        detail::create_directory_if_needed(options.target_root, report.diagnostics);
    }

    if (detail::has_error(report.diagnostics)) {
        return report;
    }

    for (const auto& file : options.files) {
        const auto target = options.target_root / file.name;
        const auto model = options.model_root / file.model_name;

        std::error_code ec;
        if (std::filesystem::exists(target, ec)) {
            report.skipped_existing.push_back(target);
            continue;
        }

        if (!std::filesystem::exists(model, ec)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-model-missing" : "optional-model-missing",
                "Model file does not exist",
                model));
            continue;
        }

        std::filesystem::copy_file(model, target, std::filesystem::copy_options::none, ec);
        if (ec) {
            report.diagnostics.push_back(detail::make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-copy-failed" : "optional-copy-failed",
                ec.message(),
                target));
            continue;
        }

        report.copied.push_back(target);
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::info,
            "config-hydrated",
            "Copied model file into config bundle",
            target));
    }

    return report;
}

hydrate_report hydrate_config_bundle(const hydrate_options& options)
{
    return ensure_config_defaults(options);
}

write_report write_with_backup(const write_options& options, validation_callback validate)
{
    ::linuxdesktop::detail::durable_file_write_options internal_options;
    internal_options.target = options.target;
    internal_options.content = options.content;
    internal_options.keep_backup = options.keep_backup;
    internal_options.atomic_replace = options.atomic_replace;
    internal_options.durable_write = options.durable_write;

    auto internal_report = ::linuxdesktop::detail::write_durable_file(std::move(internal_options), std::move(validate));

    write_report report;
    report.ok = internal_report.ok;
    report.backup_path = std::move(internal_report.backup_path);
    report.temp_path = std::move(internal_report.temp_path);
    report.durable_write = internal_report.durable_write;
    report.diagnostics = std::move(internal_report.diagnostics);
    return report;
}

write_report write_common_config(common_config_write_request request, validation_callback validate)
{
    write_options options;
    options.target = std::move(request.target);
    options.content = std::move(request.content);
    options.keep_backup = true;
    options.atomic_replace = true;
    options.durable_write = request.durable_write;
    return write_with_backup(options, std::move(validate));
}

} // namespace linuxdesktop::settings
