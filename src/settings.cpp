#include "linuxdesktop/settings.hpp"

#include "linuxdesktop/desktop.hpp"
#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstdio>
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
#include <ShlObj.h>
#include <objbase.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace linuxdesktop::settings {
namespace {

namespace ld_paths = linuxdesktop::paths;

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

std::string sanitize_segment(std::string value, std::string fallback = {})
{
    for (char& ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
            ch = '-';
        }
    }
    return value.empty() ? fallback : value;
}

unsigned long current_process_id()
{
#if defined(_WIN32)
    return static_cast<unsigned long>(GetCurrentProcessId());
#else
    return static_cast<unsigned long>(getpid());
#endif
}

std::error_code system_error_code()
{
#if defined(_WIN32)
    return std::error_code(static_cast<int>(GetLastError()), std::system_category());
#else
    return std::error_code(errno, std::generic_category());
#endif
}

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
        ec = system_error_code();
        return false;
    }
    const bool flushed = ::fsync(handle) == 0;
    if (!flushed) {
        ec = system_error_code();
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
        ec = system_error_code();
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
        diagnostics.push_back(make_diagnostic(severity::error, "write-failed", "Could not open target content", target));
        return false;
    }
    const bool wrote = write_all_bytes(handle, content);
    if (!wrote) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "write-failed",
            "Could not write target content",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "close-failed",
                system_error_code().message(),
                target));
            return false;
        }
        return false;
    }
    if (durable_write && !flush_file_handle(handle)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "durable-flush-failed",
            "Could not flush target content to disk",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "close-failed",
                system_error_code().message(),
                target));
            return false;
        }
        return false;
    }
    if (!close_file_handle(handle)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "close-failed",
            system_error_code().message(),
            target));
        return false;
    }
    if (durable_write && !target_existed) {
        std::error_code flush_ec;
        if (!flush_parent_directory(target, flush_ec)) {
            diagnostics.push_back(make_diagnostic(
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
        diagnostics.push_back(make_diagnostic(severity::error, "write-failed", "Could not open target content", target));
        return false;
    }
    const bool wrote = write_all_bytes(handle, content);
    if (!wrote) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "write-failed",
            "Could not write target content",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "close-failed",
                system_error_code().message(),
                target));
        }
        return false;
    }
    if (durable_write && !flush_file_handle(handle)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "durable-flush-failed",
            "Could not flush target content to disk",
            target));
        if (!close_file_handle(handle)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "close-failed",
                system_error_code().message(),
                target));
        }
        return false;
    }
    if (!close_file_handle(handle)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "close-failed",
            system_error_code().message(),
            target));
        return false;
    }
    if (durable_write && !target_existed) {
        std::error_code flush_ec;
        if (!flush_parent_directory(target, flush_ec)) {
            diagnostics.push_back(make_diagnostic(
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

std::filesystem::path home_directory()
{
#if defined(_WIN32)
    if (const char* user_profile = std::getenv("USERPROFILE")) {
        return user_profile;
    }
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (drive && path) {
        return std::string(drive) + std::string(path);
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return home;
    }
#endif
    return {};
}

std::optional<std::filesystem::path> absolute_env_path(const char* name, std::vector<diagnostic>& diagnostics)
{
    const char* value = std::getenv(name);
    if (!value || !value[0]) {
        return std::nullopt;
    }

    std::filesystem::path path(value);
    if (!path.is_absolute()) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "relative-env-ignored",
            std::string(name) + " is relative and was ignored",
            path));
        return std::nullopt;
    }
    return path;
}

#if defined(_WIN32)
std::optional<std::filesystem::path> known_folder(REFKNOWNFOLDERID folder_id, std::vector<diagnostic>& diagnostics, const char* code)
{
    PWSTR path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_DEFAULT, nullptr, &path);
    if (FAILED(result) || !path) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            code,
            "Windows known-folder lookup failed"));
        return std::nullopt;
    }

    std::filesystem::path value(path);
    CoTaskMemFree(path);
    return value;
}
#endif

std::filesystem::path current_directory(std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    auto path = std::filesystem::current_path(ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "current-directory-failed",
            ec.message()));
        return {};
    }
    return path;
}

std::filesystem::path executable_resource_guess(std::vector<diagnostic>& diagnostics)
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size > 0) {
        buffer.resize(size);
        return std::filesystem::path(buffer).parent_path();
    }
#else
    std::error_code ec;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return exe.parent_path();
    }
#endif
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        "resource-root-guessed",
        "Could not locate executable directory; using current directory as resource root"));
    return current_directory(diagnostics);
}

void create_directory_if_needed(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    if (path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "empty-directory",
            "Cannot create an empty directory path"));
        return;
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        if (!std::filesystem::is_directory(path, ec)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "path-not-directory",
                "Path exists but is not a directory",
                path));
        }
        return;
    }

    if (std::filesystem::create_directories(path, ec)) {
        diagnostics.push_back(make_diagnostic(
            severity::info,
            "directory-created",
            "Created directory",
            path));
        return;
    }

    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "directory-create-failed",
            ec.message(),
            path));
    }
}

bool create_directory_for_root(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    const auto before = diagnostics.size();
    const auto existed = !path.empty() && std::filesystem::is_directory(path);
    create_directory_if_needed(path, diagnostics);
    for (auto index = before; index != diagnostics.size(); ++index) {
        if (diagnostics[index].level == severity::error) {
            return false;
        }
    }
    return !existed && !path.empty() && std::filesystem::is_directory(path);
}

void append_directory_diagnostics(const ld_paths::ensure_directory_report& source, std::vector<diagnostic>& diagnostics)
{
    diagnostics.insert(diagnostics.end(), source.diagnostics.begin(), source.diagnostics.end());
}

void ensure_root_directory(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    ld_paths::ensure_directory_options options;
    options.dry_run = false;
    append_directory_diagnostics(ld_paths::ensure_directory(path, options), diagnostics);
}

std::filesystem::path selected_path_or_empty(
    const ld_paths::resolver_report& report,
    ld_paths::path_family family)
{
    const auto item = report.selected.find(family);
    return item == report.selected.end() ? std::filesystem::path{} : item->second;
}

ld_paths::resolver_options path_options_from_root_options(const root_options& options)
{
    ld_paths::resolver_options result;
    result.resource_root = options.resource_root;
    result.home_directory = options.home_directory;
    result.environment = options.environment;
    result.use_process_environment = options.use_process_environment;
    return result;
}

void apply_default_roots_from_paths(root_report& report, const ld_paths::resolver_report& paths)
{
    report.roots.config = selected_path_or_empty(paths, ld_paths::path_family::config);
    report.roots.data = selected_path_or_empty(paths, ld_paths::path_family::data);
    report.roots.state = selected_path_or_empty(paths, ld_paths::path_family::state);
    report.roots.cache = selected_path_or_empty(paths, ld_paths::path_family::cache);
    report.roots.resources = selected_path_or_empty(paths, ld_paths::path_family::resources);
    report.roots.runtime = selected_path_or_empty(paths, ld_paths::path_family::runtime);
}

bool has_error(const std::vector<diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        if (item.level == severity::error) {
            return true;
        }
    }
    return false;
}

int default_precedence(config_layer_kind kind)
{
    switch (kind) {
    case config_layer_kind::defaults:
        return 10;
    case config_layer_kind::global:
        return 20;
    case config_layer_kind::user:
        return 30;
    case config_layer_kind::local:
        return 40;
    case config_layer_kind::portable:
        return 50;
    case config_layer_kind::managed:
        return 60;
    case config_layer_kind::enforced:
        return 70;
    }
    return 0;
}

std::filesystem::path base_path_for(const app_roots& roots, persistence_class persistence, root_purpose purpose)
{
    switch (persistence) {
    case persistence_class::machine_local:
        if (purpose == root_purpose::cache || purpose == root_purpose::temp) {
            return roots.cache;
        }
        if (purpose == root_purpose::runtime) {
            return roots.runtime;
        }
        return roots.state;
    case persistence_class::portable:
        return roots.config;
    case persistence_class::ephemeral:
        return roots.cache;
    case persistence_class::managed:
        return roots.config / "managed";
    case persistence_class::enforced:
        return roots.config / "enforced";
    case persistence_class::roaming:
        break;
    }

    switch (purpose) {
    case root_purpose::resources:
        return roots.resources;
    case root_purpose::config:
    case root_purpose::plugin_config:
    case root_purpose::profiles:
    case root_purpose::backup:
    case root_purpose::component_config:
    case root_purpose::managed_config:
    case root_purpose::enforced_config:
    case root_purpose::custom:
        return roots.config;
    case root_purpose::data:
    case root_purpose::component_data:
        return roots.data;
    case root_purpose::state:
    case root_purpose::session:
    case root_purpose::logs:
    case root_purpose::component_state:
        return roots.state;
    case root_purpose::cache:
    case root_purpose::temp:
        return roots.cache;
    case root_purpose::runtime:
        return roots.runtime;
    }
    return roots.config;
}

std::filesystem::path default_relative_path(const named_root_request& request)
{
    if (!request.relative_path.empty()) {
        return request.relative_path;
    }
    if (!request.name.empty()) {
        return sanitize_segment(request.name);
    }

    switch (request.purpose) {
    case root_purpose::logs:
        return "logs";
    case root_purpose::profiles:
        return "profiles";
    case root_purpose::backup:
        return "backups";
    case root_purpose::temp:
        return "temp";
    case root_purpose::plugin_config:
        return std::filesystem::path{"plugins"} / "Config";
    case root_purpose::component_config:
        return "config";
    case root_purpose::component_data:
        return "data";
    case root_purpose::component_state:
        return "state";
    case root_purpose::managed_config:
        return "managed";
    case root_purpose::enforced_config:
        return "enforced";
    default:
        return {};
    }
}

named_root resolve_named_root(const named_root_request& request, const app_roots& roots, bool create_directories)
{
    named_root result;
    result.name = request.name;
    result.purpose = request.purpose;
    result.persistence = request.persistence;

    if (request.name.empty()) {
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-name-empty",
            "Named root requires a non-empty name"));
        return result;
    }

    const auto relative = default_relative_path(request);
    if (relative.is_absolute()) {
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-relative-path-absolute",
            "Named root relative_path must be relative",
            relative));
        return result;
    }

    const auto base = base_path_for(roots, request.persistence, request.purpose);
    if (base.empty()) {
        result.diagnostics.push_back(make_diagnostic(
            severity::error,
            "named-root-base-empty",
            "Named root base path could not be resolved"));
        return result;
    }

    result.path = relative.empty() ? base : base / relative;
    if (create_directories && request.create) {
        result.created = create_directory_for_root(result.path, result.diagnostics);
    }
    return result;
}

void append_unique_name_diagnostics(std::vector<named_root>& roots)
{
    std::vector<std::string> seen;
    for (auto& root : roots) {
        if (std::find(seen.begin(), seen.end(), root.name) != seen.end()) {
            root.diagnostics.push_back(make_diagnostic(
                severity::error,
                "named-root-duplicate",
                "Named root names must be unique within the same scope"));
        } else {
            seen.push_back(root.name);
        }
    }
}

layer_report build_layer_report(const app_identity& identity, const root_report& report)
{
    layer_report layers;
    const auto organization = sanitize_segment(identity.organization);
    const auto application = sanitize_segment(identity.application, "application");
    const auto app_leaf = organization.empty() ? application : organization + "/" + application;

    const auto add_layer = [&](config_layer_kind kind,
                               storage_backend backend,
                               std::string name,
                               std::filesystem::path path,
                               bool writable,
                               bool required = false,
                               bool enforced = false) {
        config_layer layer;
        layer.kind = kind;
        layer.backend = backend;
        layer.name = std::move(name);
        layer.path = std::move(path);
        layer.writable = writable;
        layer.required = required;
        layer.enforced = enforced;
        layer.precedence = default_precedence(kind);
        layers.candidates.push_back(layer);
    };

    add_layer(config_layer_kind::defaults, storage_backend::file, "defaults", report.roots.resources, false, false, false);
#if defined(_WIN32)
    add_layer(config_layer_kind::global, storage_backend::registry, "global", "HKLM/Software/" + app_leaf, false, false, false);
    add_layer(config_layer_kind::user, storage_backend::registry, "user", "HKCU/Software/" + app_leaf, true, false, false);
    add_layer(config_layer_kind::local, storage_backend::file, "local", report.roots.state, true, false, false);
    add_layer(config_layer_kind::managed, storage_backend::registry, "managed", "HKLM/Software/Policies/" + app_leaf, false, false, false);
    add_layer(config_layer_kind::enforced, storage_backend::registry, "enforced", "HKLM/Software/Policies/" + app_leaf, false, false, true);
#else
    add_layer(config_layer_kind::global, storage_backend::file, "global", std::filesystem::path{"/etc/xdg"} / app_leaf, false, false, false);
    add_layer(config_layer_kind::user, storage_backend::file, "user", report.roots.config, true, false, false);
    add_layer(config_layer_kind::local, storage_backend::file, "local", report.roots.state, true, false, false);
    add_layer(config_layer_kind::managed, storage_backend::file, "managed", std::filesystem::path{"/etc/dconf/db"} / application / "defaults", false, false, false);
    add_layer(config_layer_kind::enforced, storage_backend::file, "enforced", std::filesystem::path{"/etc/dconf/db"} / application / "locks", false, false, true);
#endif
    if (report.portable_active) {
        add_layer(config_layer_kind::portable, storage_backend::file, "portable", report.roots.config, true, false, false);
    }
    if (report.settings_override_active) {
        add_layer(config_layer_kind::user, storage_backend::override_values, "settings_override", report.roots.config, true, false, false);
    }
    if (report.sync_config_override_active) {
        add_layer(config_layer_kind::user, storage_backend::override_values, "sync_config_override", report.roots.config, true, false, false);
    }

    layers.active_read_order = layers.candidates;
    std::sort(layers.active_read_order.begin(), layers.active_read_order.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.precedence > rhs.precedence;
    });

    for (const auto& layer : layers.active_read_order) {
        if (layer.writable) {
            layers.active_write_layer = layer;
            break;
        }
    }

    return layers;
}

std::filesystem::path comparable_path(const std::filesystem::path& path)
{
    std::error_code ec;
    auto absolute = path.is_absolute() ? path : std::filesystem::absolute(path, ec);
    if (ec) {
        absolute = path;
    }
    return absolute.lexically_normal();
}

bool path_is_at_or_under(const std::filesystem::path& candidate, const std::filesystem::path& root)
{
    const auto normalized_candidate = comparable_path(candidate);
    const auto normalized_root = comparable_path(root);
    auto candidate_part = normalized_candidate.begin();
    auto root_part = normalized_root.begin();

    for (; root_part != normalized_root.end(); ++root_part, ++candidate_part) {
        if (candidate_part == normalized_candidate.end() || *candidate_part != *root_part) {
            return false;
        }
    }
    return true;
}

std::vector<std::filesystem::path> default_privileged_install_roots()
{
#if defined(_WIN32)
    std::vector<std::filesystem::path> roots;
    if (const char* program_files = std::getenv("ProgramFiles")) {
        roots.emplace_back(program_files);
    }
    if (const char* program_files_x86 = std::getenv("ProgramFiles(x86)")) {
        roots.emplace_back(program_files_x86);
    }
    if (roots.empty()) {
        roots.emplace_back("C:\\Program Files");
        roots.emplace_back("C:\\Program Files (x86)");
    }
    return roots;
#else
    return {"/usr", "/opt", "/app"};
#endif
}

bool is_under_privileged_install_root(const std::filesystem::path& path, const root_options& options)
{
    const auto roots = options.privileged_install_roots.empty()
        ? default_privileged_install_roots()
        : options.privileged_install_roots;

    for (const auto& root : roots) {
        if (!root.empty() && path_is_at_or_under(path, root)) {
            return true;
        }
    }
    return false;
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

std::string_view to_string(root_purpose value)
{
    switch (value) {
    case root_purpose::resources:
        return "resources";
    case root_purpose::config:
        return "config";
    case root_purpose::data:
        return "data";
    case root_purpose::state:
        return "state";
    case root_purpose::cache:
        return "cache";
    case root_purpose::runtime:
        return "runtime";
    case root_purpose::session:
        return "session";
    case root_purpose::plugin_config:
        return "plugin_config";
    case root_purpose::logs:
        return "logs";
    case root_purpose::profiles:
        return "profiles";
    case root_purpose::backup:
        return "backup";
    case root_purpose::temp:
        return "temp";
    case root_purpose::component_config:
        return "component_config";
    case root_purpose::component_data:
        return "component_data";
    case root_purpose::component_state:
        return "component_state";
    case root_purpose::managed_config:
        return "managed_config";
    case root_purpose::enforced_config:
        return "enforced_config";
    case root_purpose::custom:
        return "custom";
    }
    return "unknown";
}

std::string_view to_string(persistence_class value)
{
    switch (value) {
    case persistence_class::roaming:
        return "roaming";
    case persistence_class::machine_local:
        return "machine_local";
    case persistence_class::portable:
        return "portable";
    case persistence_class::ephemeral:
        return "ephemeral";
    case persistence_class::managed:
        return "managed";
    case persistence_class::enforced:
        return "enforced";
    }
    return "unknown";
}

std::string_view to_string(component_kind value)
{
    switch (value) {
    case component_kind::plugin:
        return "plugin";
    case component_kind::embedded_tool:
        return "embedded_tool";
    case component_kind::profile:
        return "profile";
    case component_kind::language_pack:
        return "language_pack";
    case component_kind::extension:
        return "extension";
    case component_kind::custom:
        return "custom";
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
const named_root* find_named_root(const root_report& report, const std::string& name)
{
    const auto it = std::find_if(report.named_roots.begin(), report.named_roots.end(), [&](const auto& root) {
        return root.name == name;
    });
    return it == report.named_roots.end() ? nullptr : &*it;
}

const component_root_group* find_component_roots(const root_report& report, const std::string& name)
{
    const auto it = std::find_if(report.component_roots.begin(), report.component_roots.end(), [&](const auto& component) {
        return component.name == name;
    });
    return it == report.component_roots.end() ? nullptr : &*it;
}

const named_root* find_component_named_root(const component_root_group& component, const std::string& name)
{
    const auto it = std::find_if(component.roots.begin(), component.roots.end(), [&](const auto& root) {
        return root.name == name;
    });
    return it == component.roots.end() ? nullptr : &*it;
}

const config_layer* find_config_layer(const layer_report& report, config_layer_kind kind, const std::string& name)
{
    const auto it = std::find_if(report.candidates.begin(), report.candidates.end(), [&](const auto& layer) {
        return layer.kind == kind && (name.empty() || layer.name == name);
    });
    return it == report.candidates.end() ? nullptr : &*it;
}
root_report resolve_app_roots(const app_identity& identity, const root_options& options)
{
    root_report report;

    ld_paths::app_identity path_identity;
    path_identity.organization = identity.organization;
    path_identity.application = identity.application;
    const auto path_report = ld_paths::resolve_app_paths(path_identity, path_options_from_root_options(options));
    report.diagnostics.insert(report.diagnostics.end(), path_report.diagnostics.begin(), path_report.diagnostics.end());
    report.roots.resources = selected_path_or_empty(path_report, ld_paths::path_family::resources);

    report.portable = options.portable;

    if (options.settings_override) {
        if (options.settings_override->is_absolute()) {
            report.settings_override_active = true;
            report.roots.config = *options.settings_override;
            report.roots.data = *options.settings_override;
            report.roots.state = *options.settings_override;
            report.roots.cache = *options.settings_override / "cache";
            report.roots.runtime = std::filesystem::path{};
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "settings-override-relative",
                "Settings override must be absolute",
                *options.settings_override));
        }
    }

    report.portable_requested = options.portable_marker.has_value();
    if (!report.settings_override_active && options.portable_marker) {
        const auto marker = *options.portable_marker;
        std::error_code ec;
        if (std::filesystem::exists(marker, ec)) {
            if (options.allow_portable_root && options.portable != portable_level::off) {
                const auto portable_root = marker.parent_path();
                const auto privileged_path = !report.roots.resources.empty() ? report.roots.resources : portable_root;
                if (options.deny_portable_root_in_privileged_install &&
                    is_under_privileged_install_root(privileged_path, options)) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        "portable-denied-privileged-install",
                        "Portable marker exists, but install root is privileged",
                        privileged_path));
                } else {
                    report.portable_active = true;
                    report.roots.config = portable_root;
                    report.roots.data = portable_root;
                    report.roots.state = portable_root;
                    report.roots.cache = portable_root / "cache";
                    report.roots.runtime = std::filesystem::path{};
                }
            } else {
                report.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "portable-denied",
                    "Portable marker exists, but portable roots are disabled",
                    marker));
            }
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "portable-marker-missing",
                "Portable marker was requested but does not exist",
                marker));
        }
    }

    if (report.roots.config.empty() && !has_error(report.diagnostics)) {
        apply_default_roots_from_paths(report, path_report);
    }

    if (options.sync_config_override) {
        if (report.settings_override_active) {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "sync-config-override-ignored",
                "Sync config override was ignored because settings override is active",
                *options.sync_config_override));
        } else if (report.portable_active && !options.allow_sync_config_for_portable_root) {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "sync-config-override-ignored-portable",
                "Sync config override was ignored because portable root is active",
                *options.sync_config_override));
        } else if (options.sync_config_override->is_absolute()) {
            report.sync_config_override_active = true;
            report.roots.config = *options.sync_config_override;
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "sync-config-override-relative",
                "Sync config override must be absolute",
                *options.sync_config_override));
        }
    }

    report.roots.session = report.roots.state / "sessions";
    report.roots.plugin_config = report.roots.config / "plugins" / "Config";

    for (const auto& request : options.named_roots) {
        report.named_roots.push_back(resolve_named_root(request, report.roots, options.create_directories));
    }
    append_unique_name_diagnostics(report.named_roots);

    for (const auto& request : options.component_roots) {
        component_root_group component;
        component.name = request.name;
        component.kind = request.kind;

        if (request.name.empty()) {
            component.diagnostics.push_back(make_diagnostic(
                severity::error,
                "component-root-name-empty",
                "Component root requires a non-empty name"));
        }

        const auto component_leaf = sanitize_segment(request.name, "component");
        for (const auto& root_request : request.roots) {
            auto scoped_request = root_request;
            const auto relative = default_relative_path(scoped_request);
            scoped_request.relative_path = std::filesystem::path{"components"} / component_leaf / relative;
            component.roots.push_back(resolve_named_root(scoped_request, report.roots, options.create_directories));
        }
        append_unique_name_diagnostics(component.roots);
        report.component_roots.push_back(std::move(component));
    }

    report.layers = build_layer_report(identity, report);

    if (options.create_directories && !has_error(report.diagnostics)) {
        ensure_root_directory(report.roots.config, report.diagnostics);
        ensure_root_directory(report.roots.data, report.diagnostics);
        ensure_root_directory(report.roots.state, report.diagnostics);
        ensure_root_directory(report.roots.cache, report.diagnostics);
        create_directory_if_needed(report.roots.session, report.diagnostics);
        create_directory_if_needed(report.roots.plugin_config, report.diagnostics);
        if (!report.roots.runtime.empty()) {
            ensure_root_directory(report.roots.runtime, report.diagnostics);
        }
    }

    return report;
}

hydrate_report hydrate_config_bundle(const hydrate_options& options)
{
    hydrate_report report;
    if (options.create_target_root) {
        create_directory_if_needed(options.target_root, report.diagnostics);
    }

    if (has_error(report.diagnostics)) {
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
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-model-missing" : "optional-model-missing",
                "Model file does not exist",
                model));
            continue;
        }

        std::filesystem::copy_file(model, target, std::filesystem::copy_options::none, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-copy-failed" : "optional-copy-failed",
                ec.message(),
                target));
            continue;
        }

        report.copied.push_back(target);
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "config-hydrated",
            "Copied model file into config bundle",
            target));
    }

    return report;
}

write_report write_with_backup(const write_options& options, validation_callback validate)
{
    write_report report;
    report.durable_write = options.durable_write;
    create_directory_if_needed(options.target.parent_path(), report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

    std::error_code ec;
    std::filesystem::path write_target = options.target;

    const auto backup = options.target.string() + ".bak";
    if (!options.atomic_replace && options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
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
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "temp-open-failed",
                ec.message(),
                options.target));
            return report;
        }
        report.temp_path = write_target;
        if (!write_all_bytes(temp_handle, options.content)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "temp-write-failed",
                "Could not write temporary target content",
                write_target));
            if (!close_file_handle(temp_handle)) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "temp-close-failed",
                    system_error_code().message(),
                    write_target));
            }
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
        if (options.durable_write && !flush_file_handle(temp_handle)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "durable-flush-failed",
                "Could not flush temporary target content to disk",
                write_target));
            if (!close_file_handle(temp_handle)) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "temp-close-failed",
                    system_error_code().message(),
                    write_target));
            }
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
        if (!close_file_handle(temp_handle)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "temp-close-failed",
                system_error_code().message(),
                write_target));
            std::error_code cleanup_ec;
            std::filesystem::remove(write_target, cleanup_ec);
            return report;
        }
    } else if (!options.durable_write) {
        if (!write_file_content(write_target, options.content)) {
            report.diagnostics.push_back(make_diagnostic(
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
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "validation-failed",
                validation_message.empty() ? "Written file failed validation" : validation_message,
                write_target));

            if (options.atomic_replace) {
                std::filesystem::remove(write_target, ec);
                if (ec) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        "temp-cleanup-failed",
                        ec.message(),
                        write_target));
                } else {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::info,
                        "temp-cleaned",
                        "Removed invalid temporary file",
                        write_target));
                }
            } else if (report.backup_path) {
                std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::error,
                        "backup-restore-failed",
                        ec.message(),
                        options.target));
                } else {
                    report.diagnostics.push_back(make_diagnostic(
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
            report.diagnostics.push_back(make_diagnostic(
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
            report.diagnostics.push_back(make_diagnostic(
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
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "parent-flush-failed",
                    flush_ec.message(),
                    options.target.parent_path()));
                return report;
            }
        }
    }

    std::error_code read_ec;
    static_cast<void>(read_text(options.target, read_ec));
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "write-readback-failed",
            read_ec.message(),
            options.target));
        if (report.backup_path) {
            std::error_code restore_ec;
            std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, restore_ec);
            if (restore_ec) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "backup-restore-failed",
                    restore_ec.message(),
                    options.target));
            } else {
                report.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "backup-restored-after-readback-failure",
                    "Restored previous target after readback failure",
                    options.target));
            }
        }
        return report;
    }

    report.ok = true;
    report.diagnostics.push_back(make_diagnostic(
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
