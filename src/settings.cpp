#include "settings_internal.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <shlobj.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace linuxdesktop::settings {
namespace {

#if defined(_WIN32)
unsigned long current_process_id()
{
    return static_cast<unsigned long>(GetCurrentProcessId());
}
#endif

bool close_file_handle(
#if defined(_WIN32)
    HANDLE handle
#else
    int handle
#endif
)
{
#if defined(_WIN32)
    return CloseHandle(handle) != 0;
#else
    return ::close(handle) == 0;
#endif
}

bool flush_file_handle(
#if defined(_WIN32)
    HANDLE handle
#else
    int handle
#endif
)
{
#if defined(_WIN32)
    return FlushFileBuffers(handle) != 0;
#else
    return ::fsync(handle) == 0;
#endif
}

bool flush_parent_directory(const std::filesystem::path& path, std::error_code& ec)
{
    ec.clear();
    const auto parent = path.parent_path();
    if (parent.empty()) {
        return true;
    }
#if defined(_WIN32)
    const auto parent_text = parent.wstring();
    HANDLE handle = CreateFileW(
        parent_text.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        ec.clear();
        return true;
    }
    const bool flushed = FlushFileBuffers(handle) != 0;
    CloseHandle(handle);
    if (!flushed) {
        // Windows does not reliably support directory handle flushing across all
        // runner filesystems. The file handle and MoveFileExW replacement already
        // requested write-through semantics, so this extra step is best effort.
        ec.clear();
        return true;
    }
    return flushed;
#else
    const auto parent_text = parent.c_str();
    const int handle = ::open(parent_text, O_RDONLY | O_DIRECTORY);
    if (handle == -1) {
        ec = detail::system_error_code();
        return false;
    }
    const bool flushed = ::fsync(handle) == 0;
    if (!flushed) {
        ec = detail::system_error_code();
    }
    ::close(handle);
    return flushed;
#endif
}

bool write_all_bytes(
#if defined(_WIN32)
    HANDLE handle,
#else
    int handle,
#endif
    const std::string& content)
{
    std::size_t offset = 0;
    while (offset < content.size()) {
#if defined(_WIN32)
        DWORD written = 0;
        const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(content.size() - offset, 1u << 20));
        if (WriteFile(handle, content.data() + offset, chunk, &written, nullptr) == 0) {
            return false;
        }
        offset += written;
#else
        const auto written = ::write(handle, content.data() + offset, content.size() - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        offset += static_cast<std::size_t>(written);
#endif
    }
    return true;
}

bool create_secure_temp_file(
    const std::filesystem::path& target,
    std::filesystem::path& temp_path,
#if defined(_WIN32)
    HANDLE& handle,
#else
    int& handle,
#endif
    std::error_code& ec)
{
    ec.clear();
    const auto parent = target.parent_path();
    const auto stem = target.filename().string();
#if defined(_WIN32)
    for (int attempt = 0; attempt != 128; ++attempt) {
        const auto candidate = parent / (stem + ".tmp." + std::to_string(current_process_id()) + "." + std::to_string(attempt));
        const auto candidate_text = candidate.wstring();
        handle = CreateFileW(
            candidate_text.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
            nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            temp_path = candidate;
            return true;
        }
        const auto last_error = GetLastError();
        if (last_error != ERROR_FILE_EXISTS && last_error != ERROR_ALREADY_EXISTS) {
            ec = std::error_code(static_cast<int>(last_error), std::system_category());
            return false;
        }
    }
    ec = std::make_error_code(std::errc::file_exists);
    return false;
#else
    const auto pattern_text = (parent / (stem + ".tmp.XXXXXX")).string();
    std::vector<char> pattern(pattern_text.begin(), pattern_text.end());
    pattern.push_back('\0');
    handle = ::mkstemp(pattern.data());
    if (handle == -1) {
        ec = detail::system_error_code();
        return false;
    }
    temp_path = pattern.data();
    return true;
#endif
}

bool write_direct_file(
    const std::filesystem::path& target,
    const std::string& content,
    bool durable_write,
    std::vector<diagnostic>& diagnostics)
{
    std::error_code exists_ec;
    const bool target_existed = std::filesystem::exists(target, exists_ec);
#if defined(_WIN32)
    const auto target_text = target.wstring();
    const DWORD flags = durable_write ? FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH : FILE_ATTRIBUTE_NORMAL;
    HANDLE handle = CreateFileW(
        target_text.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        flags,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        diagnostics.push_back(detail::make_diagnostic(severity::error, "write-failed", "Could not open target content", target));
        return false;
    }
    const bool wrote = write_all_bytes(handle, content);
    if (!wrote) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "write-failed",
            "Could not write target content",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "close-failed",
                detail::system_error_code().message(),
                target));
            return false;
        }
        return false;
    }
    if (durable_write && !flush_file_handle(handle)) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "durable-flush-failed",
            "Could not flush target content to disk",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "close-failed",
                detail::system_error_code().message(),
                target));
            return false;
        }
        return false;
    }
    if (!close_file_handle(handle)) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "close-failed",
            detail::system_error_code().message(),
            target));
        return false;
    }
    if (durable_write && !target_existed) {
        std::error_code flush_ec;
        if (!flush_parent_directory(target, flush_ec)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "parent-flush-failed",
                flush_ec.message(),
                target.parent_path()));
            return false;
        }
    }
    return true;
#else
    const auto flags = O_WRONLY | O_CREAT | O_TRUNC;
    const auto mode = static_cast<mode_t>(0600);
    const int handle = ::open(target.c_str(), flags, mode);
    if (handle == -1) {
        diagnostics.push_back(detail::make_diagnostic(severity::error, "write-failed", "Could not open target content", target));
        return false;
    }
    const bool wrote = write_all_bytes(handle, content);
    if (!wrote) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "write-failed",
            "Could not write target content",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "close-failed",
                detail::system_error_code().message(),
                target));
        }
        return false;
    }
    if (durable_write && !flush_file_handle(handle)) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "durable-flush-failed",
            "Could not flush target content to disk",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "close-failed",
                detail::system_error_code().message(),
                target));
        }
        return false;
    }
    if (!close_file_handle(handle)) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "close-failed",
            detail::system_error_code().message(),
            target));
        return false;
    }
    if (durable_write && !target_existed) {
        std::error_code flush_ec;
        if (!flush_parent_directory(target, flush_ec)) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "parent-flush-failed",
                flush_ec.message(),
                target.parent_path()));
            return false;
        }
    }
    return true;
#endif
}

bool replace_file(const std::filesystem::path& from, const std::filesystem::path& to, std::error_code& ec)
{
    ec.clear();
#if defined(_WIN32)
    if (MoveFileExW(from.wstring().c_str(), to.wstring().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return false;
    }
    return true;
#else
    std::filesystem::rename(from, to, ec);
    return !ec;
#endif
}

bool write_file_content(const std::filesystem::path& target, const std::string& content)
{
    std::ofstream output(target, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(output);
}

std::string read_text(const std::filesystem::path& path, std::error_code& ec)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        ec = std::make_error_code(std::errc::io_error);
        return {};
    }

    ec.clear();
    return buffer.str();
}
} // namespace

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
    write_report report;
    report.durable_write = options.durable_write;
    detail::create_directory_if_needed(options.target.parent_path(), report.diagnostics);
    if (detail::has_error(report.diagnostics)) {
        return report;
    }

    std::error_code ec;
    std::filesystem::path write_target = options.target;

    const auto backup = options.target.string() + ".bak";
    if (!options.atomic_replace && options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "backup-copy-failed",
                ec.message(),
                backup));
            return report;
        }
        report.backup_path = backup;
    }

    if (options.atomic_replace) {
        report.temp_path = {};
#if defined(_WIN32)
        HANDLE temp_handle = INVALID_HANDLE_VALUE;
#else
        int temp_handle = -1;
#endif
        if (!create_secure_temp_file(options.target, write_target, temp_handle, ec)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "temp-open-failed",
                ec.message(),
                options.target));
            return report;
        }
        report.temp_path = write_target;
        if (!write_all_bytes(temp_handle, options.content)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "temp-write-failed",
                "Could not write temporary target content",
                write_target));
            if (!close_file_handle(temp_handle)) {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::error,
                    "temp-close-failed",
                    detail::system_error_code().message(),
                    write_target));
            }
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
        if (options.durable_write && !flush_file_handle(temp_handle)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "durable-flush-failed",
                "Could not flush temporary target content to disk",
                write_target));
            if (!close_file_handle(temp_handle)) {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::error,
                    "temp-close-failed",
                    detail::system_error_code().message(),
                    write_target));
            }
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
        if (!close_file_handle(temp_handle)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "temp-close-failed",
                detail::system_error_code().message(),
                write_target));
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
    } else if (!options.durable_write) {
        if (!write_file_content(write_target, options.content)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "write-failed",
                "Could not write target content",
                write_target));
            return report;
        }
    } else if (!write_direct_file(write_target, options.content, options.durable_write, report.diagnostics)) {
        return report;
    }

    if (validate) {
        std::string validation_message;
        if (!validate(write_target, validation_message)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "validation-failed",
                validation_message.empty() ? "Written file failed validation" : validation_message,
                write_target));

            if (options.atomic_replace) {
                std::filesystem::remove(write_target, ec);
                if (ec) {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::warning,
                        "temp-cleanup-failed",
                        ec.message(),
                        write_target));
                } else {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::info,
                        "temp-cleaned",
                        "Removed invalid temporary file",
                        write_target));
                }
            } else if (report.backup_path) {
                std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::error,
                        "backup-restore-failed",
                        ec.message(),
                        options.target));
                } else {
                    report.diagnostics.push_back(detail::make_diagnostic(
                        severity::warning,
                        "backup-restored",
                        "Restored previous target from backup",
                        options.target));
                }
            }
            return report;
        }
    }

    if (options.atomic_replace && options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "backup-copy-failed",
                ec.message(),
                backup));
            if (options.atomic_replace) {
                std::filesystem::remove(write_target, ec);
            }
            return report;
        }
        report.backup_path = backup;
    }

    if (options.atomic_replace) {
        if (!replace_file(write_target, options.target, ec)) {
            report.diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "atomic-replace-failed",
                ec.message(),
                options.target));
            std::filesystem::remove(write_target, ec);
            return report;
        }
        if (options.durable_write) {
            std::error_code flush_ec;
            if (!flush_parent_directory(options.target, flush_ec)) {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::error,
                    "parent-flush-failed",
                    flush_ec.message(),
                    options.target.parent_path()));
                return report;
            }
        }
    }

    std::error_code read_ec;
    const auto persisted_content = read_text(options.target, read_ec);
    if (read_ec || persisted_content != options.content) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            read_ec ? "write-readback-failed" : "write-readback-mismatch",
            read_ec ? read_ec.message() : "Written file content did not match readback content",
            options.target));
        if (report.backup_path) {
            std::error_code restore_ec;
            std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, restore_ec);
            if (restore_ec) {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::error,
                    "backup-restore-failed",
                    restore_ec.message(),
                    options.target));
            } else {
                report.diagnostics.push_back(detail::make_diagnostic(
                    severity::warning,
                    "backup-restored-after-readback-failure",
                    "Restored previous target after readback failure",
                    options.target));
            }
        }
        return report;
    }

    report.ok = true;
    report.diagnostics.push_back(detail::make_diagnostic(
        severity::info,
        options.durable_write ? "write-ok-durable" : "write-ok",
        options.durable_write ? "Wrote target file with durability enabled" : "Wrote target file",
        options.target));
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
