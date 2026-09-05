#include "settings_internal.hpp"
#include "durable_file_write.hpp"

#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <cstdlib>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace linuxdesktop::settings {
namespace {

std::filesystem::path normalized_target(const std::filesystem::path& target)
{
    std::error_code ec;
    auto absolute = std::filesystem::absolute(target, ec);
    if (ec) {
        absolute = target;
    }
    return absolute.lexically_normal();
}

std::string target_key(const std::filesystem::path& target)
{
    return normalized_target(target).generic_string();
}

std::shared_ptr<std::mutex> mutex_for_target(const std::filesystem::path& target)
{
    static std::mutex registry_mutex;
    static std::map<std::string, std::weak_ptr<std::mutex>> registry;

    const auto key = target_key(target);
    std::lock_guard<std::mutex> lock(registry_mutex);
    if (auto existing = registry[key].lock()) {
        return existing;
    }
    auto created = std::make_shared<std::mutex>();
    registry[key] = created;
    return created;
}

bool target_matches_token(
    bool expected_existed,
    const std::string& expected_content,
    const std::filesystem::path& target,
    std::vector<diagnostic>& diagnostics)
{
    std::error_code exists_ec;
    const bool exists = std::filesystem::exists(target, exists_ec);
    if (exists_ec) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            exists_ec.message(),
            target));
        return false;
    }

    if (!exists) {
        if (!expected_existed) {
            return true;
        }
        diagnostics.push_back(detail::make_diagnostic(
            severity::warning,
            "settings-version-stale",
            "Settings file changed since the expected version was captured",
            target));
        return false;
    }

    if (!std::filesystem::is_regular_file(target, exists_ec)) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            exists_ec ? exists_ec.message() : "Settings target is not a regular file",
            target));
        return false;
    }

    std::error_code read_ec;
    const auto content = ::linuxdesktop::detail::read_text_file(target, read_ec);
    if (read_ec) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            read_ec.message(),
            target));
        return false;
    }
    if (!expected_existed || content != expected_content) {
        diagnostics.push_back(detail::make_diagnostic(
            severity::warning,
            "settings-version-stale",
            "Settings file changed since the expected version was captured",
            target));
        return false;
    }
    return true;
}

class target_commit_guard {
public:
    explicit target_commit_guard(const std::filesystem::path& target, std::vector<diagnostic>& diagnostics)
        : process_mutex_(mutex_for_target(target))
        , process_lock_(*process_mutex_)
    {
        const auto lock_path = target.string() + ".ld2026.commit.lock";
#if defined(_WIN32)
        handle_ = CreateFileW(
            std::filesystem::path(lock_path).wstring().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "settings-version-guard-failed",
                detail::system_error_code().message(),
                lock_path));
            return;
        }
        OVERLAPPED overlapped = {};
        if (LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &overlapped) == 0) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "settings-version-guard-failed",
                detail::system_error_code().message(),
                lock_path));
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
            return;
        }
#else
        handle_ = ::open(lock_path.c_str(), O_RDWR | O_CREAT, 0600);
        if (handle_ == -1) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "settings-version-guard-failed",
                detail::system_error_code().message(),
                lock_path));
            return;
        }
        if (::flock(handle_, LOCK_EX) == -1) {
            diagnostics.push_back(detail::make_diagnostic(
                severity::error,
                "settings-version-guard-failed",
                detail::system_error_code().message(),
                lock_path));
            ::close(handle_);
            handle_ = -1;
            return;
        }
#endif
        locked_ = true;
    }

    target_commit_guard(const target_commit_guard&) = delete;
    target_commit_guard& operator=(const target_commit_guard&) = delete;

    ~target_commit_guard()
    {
#if defined(_WIN32)
        if (locked_) {
            OVERLAPPED overlapped = {};
            UnlockFileEx(handle_, 0, 1, 0, &overlapped);
        }
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
#else
        if (locked_) {
            ::flock(handle_, LOCK_UN);
        }
        if (handle_ != -1) {
            ::close(handle_);
        }
#endif
    }

    bool locked() const { return locked_; }

private:
    std::shared_ptr<std::mutex> process_mutex_;
    std::unique_lock<std::mutex> process_lock_;
    bool locked_ = false;
#if defined(_WIN32)
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int handle_ = -1;
#endif
};

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
config_defaults_report ensure_config_defaults(const config_defaults_options& options)
{
    config_defaults_report report;
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
            "config-default-copied",
            "Copied model file into config defaults target",
            target));
    }

    return report;
}

file_version_read_report read_file_version(const std::filesystem::path& target)
{
    file_version_read_report report;
    report.target = target;
    const auto normalized = normalized_target(target);

    std::error_code exists_ec;
    if (!std::filesystem::exists(target, exists_ec)) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            exists_ec ? exists_ec.message() : "Settings file does not exist",
            target));
        return report;
    }
    if (!std::filesystem::is_regular_file(target, exists_ec)) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            exists_ec ? exists_ec.message() : "Settings target is not a regular file",
            target));
        return report;
    }

    std::error_code read_ec;
    report.content = ::linuxdesktop::detail::read_text_file(target, read_ec);
    if (read_ec) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-read-failed",
            read_ec.message(),
            target));
        return report;
    }

    report.ok = true;
    report.version.target_ = normalized;
    report.version.valid_ = true;
    report.version.existed_ = true;
    report.version.content_ = report.content;
    return report;
}

file_version_token missing_file_version(std::filesystem::path target)
{
    file_version_token token;
    token.target_ = normalized_target(target);
    token.valid_ = true;
    token.existed_ = false;
    return token;
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
    report.diagnostics.push_back(detail::make_diagnostic(
        severity::warning,
        "settings-interprocess-lost-update-not-protected",
        "Settings writes use replacement and backup for corruption safety but do not protect read-modify-write flows from lost updates across processes",
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

write_report write_versioned(versioned_write_request request, validation_callback validate)
{
    write_report report;
    report.durable_write = request.durable_write;

    const auto target = normalized_target(request.target);
    if (!request.expected_version.valid_) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-token-invalid",
            "Versioned settings commits require a token captured from a settings file read or explicit missing-file state",
            request.target));
        return report;
    }
    if (request.expected_version.target_ != target) {
        report.diagnostics.push_back(detail::make_diagnostic(
            severity::error,
            "settings-version-target-mismatch",
            "Versioned settings commit token was captured for a different target",
            request.target));
        return report;
    }

    ::linuxdesktop::detail::create_directory_if_needed(request.target.parent_path(), report.diagnostics);
    if (::linuxdesktop::detail::has_error(report.diagnostics)) {
        return report;
    }

    target_commit_guard guard(request.target, report.diagnostics);
    if (!guard.locked()) {
        return report;
    }
    if (!target_matches_token(
            request.expected_version.existed_,
            request.expected_version.content_,
            request.target,
            report.diagnostics)) {
        return report;
    }

    ::linuxdesktop::detail::durable_file_write_options internal_options;
    internal_options.target = std::move(request.target);
    internal_options.content = std::move(request.content);
    internal_options.keep_backup = request.keep_backup;
    internal_options.atomic_replace = request.atomic_replace;
    internal_options.durable_write = request.durable_write;

    auto internal_report = ::linuxdesktop::detail::write_durable_file(std::move(internal_options), std::move(validate));
    report.ok = internal_report.ok;
    report.backup_path = std::move(internal_report.backup_path);
    report.temp_path = std::move(internal_report.temp_path);
    report.durable_write = internal_report.durable_write;
    report.diagnostics.insert(
        report.diagnostics.end(),
        std::make_move_iterator(internal_report.diagnostics.begin()),
        std::make_move_iterator(internal_report.diagnostics.end()));
    return report;
}

} // namespace linuxdesktop::settings
