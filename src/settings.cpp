#include "linuxdesktop/settings.hpp"

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
        ec = system_error_code();
        return false;
    }
    const bool flushed = FlushFileBuffers(handle) != 0;
    if (!flushed) {
        ec = system_error_code();
    }
    CloseHandle(handle);
    if (!flushed && ec.value() == ERROR_INVALID_FUNCTION) {
        // Some Windows filesystems reject FlushFileBuffers for directory handles.
        // The file handle was already flushed before the atomic replacement, so
        // treat the directory flush as an unavailable extra durability step.
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

void append_action_gate_diagnostics(
    const migration_action& action,
    const migration_options& options,
    std::vector<diagnostic>& diagnostics)
{
    if (action.name.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-action-name-empty",
            "Migration action requires a non-empty name"));
    }
    if (action.dangerous && !options.allow_dangerous) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-dangerous-action-denied",
            "Dangerous migration action requires allow_dangerous"));
    }
    if (action.requires_elevation && !options.allow_elevation) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-elevation-denied",
            "Migration action requires elevation permission"));
    }
}

bool is_file_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_file ||
        kind == migration_action_kind::move_file ||
        kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

bool is_directory_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

bool is_move_action(migration_action_kind kind)
{
    return kind == migration_action_kind::move_file ||
        kind == migration_action_kind::move_directory;
}

void append_file_action_diagnostics(
    const migration_action& action,
    const migration_options& options,
    std::vector<diagnostic>& diagnostics)
{
    if (!is_file_action(action.kind)) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-action-not-executable-yet",
            "This migration action kind is planned but does not have an executor yet"));
        return;
    }

    if (action.source_path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-empty",
            "File migration action requires a source path"));
    }
    if (action.target_path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-target-empty",
            "File migration action requires a target path"));
    }

    std::error_code ec;
    if (!action.source_path.empty() && !std::filesystem::exists(action.source_path, ec)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-missing",
            "Migration source does not exist",
            action.source_path));
    }
    if (!action.source_path.empty() && std::filesystem::exists(action.source_path, ec)) {
        const auto directory = std::filesystem::is_directory(action.source_path, ec);
        if (is_directory_action(action.kind) != directory) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-source-kind-mismatch",
                "Migration source kind does not match the action",
                action.source_path));
        }
    }
    if (!action.target_path.empty() && std::filesystem::exists(action.target_path, ec) && !options.overwrite_existing) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-target-exists",
            "Migration target exists and overwrite_existing is false",
            action.target_path));
    }
}

#if defined(_WIN32)
std::wstring widen_utf8(const std::string& value)
{
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return std::filesystem::path(value).wstring();
    }
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string narrow_utf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

HKEY native_hive(registry::hive value)
{
    switch (value) {
    case registry::hive::current_user:
        return HKEY_CURRENT_USER;
    case registry::hive::local_machine:
        return HKEY_LOCAL_MACHINE;
    case registry::hive::classes_root:
        return HKEY_CLASSES_ROOT;
    case registry::hive::users:
        return HKEY_USERS;
    case registry::hive::current_config:
        return HKEY_CURRENT_CONFIG;
    }
    return HKEY_CURRENT_USER;
}

REGSAM registry_view_flags(registry::view value)
{
    switch (value) {
    case registry::view::registry_32:
        return KEY_WOW64_32KEY;
    case registry::view::registry_64:
        return KEY_WOW64_64KEY;
    case registry::view::native:
        return 0;
    }
    return 0;
}

DWORD registry_type_to_native(registry::value_type value)
{
    switch (value) {
    case registry::value_type::none:
        return REG_NONE;
    case registry::value_type::string:
        return REG_SZ;
    case registry::value_type::expandable_string:
        return REG_EXPAND_SZ;
    case registry::value_type::multi_string:
        return REG_MULTI_SZ;
    case registry::value_type::dword:
        return REG_DWORD;
    case registry::value_type::qword:
        return REG_QWORD;
    case registry::value_type::binary:
        return REG_BINARY;
    case registry::value_type::unknown:
        return REG_NONE;
    }
    return REG_NONE;
}

registry::value_type registry_type_from_native(DWORD value)
{
    switch (value) {
    case REG_NONE:
        return registry::value_type::none;
    case REG_SZ:
        return registry::value_type::string;
    case REG_EXPAND_SZ:
        return registry::value_type::expandable_string;
    case REG_MULTI_SZ:
        return registry::value_type::multi_string;
    case REG_DWORD:
        return registry::value_type::dword;
    case REG_QWORD:
        return registry::value_type::qword;
    case REG_BINARY:
        return registry::value_type::binary;
    default:
        return registry::value_type::unknown;
    }
}

diagnostic win32_diagnostic(std::string code, std::string message, LSTATUS status)
{
    return make_diagnostic(
        severity::error,
        std::move(code),
        std::move(message) + ": Win32 error " + std::to_string(status));
}

bool is_policy_key(const registry::key& key)
{
    return key.subkey.find("Software\\Policies") == 0 || key.subkey.find("Software/Policies") == 0;
}

bool registry_write_allowed(
    const registry::key& key,
    const registry::options& options,
    bool recursive_delete,
    std::vector<diagnostic>& diagnostics)
{
    if (key.root == registry::hive::local_machine && !options.allow_hklm_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-hklm-write-denied",
            "HKLM writes require allow_hklm_write"));
    }
    if (is_policy_key(key) && !options.allow_policy_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-policy-write-denied",
            "Policy key writes require allow_policy_write"));
    }
    if (recursive_delete && !options.allow_recursive_delete) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-recursive-delete-denied",
            "Recursive Registry delete requires allow_recursive_delete"));
    }
    return !has_error(diagnostics);
}
#endif

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

std::string_view to_string(migration_action_kind value)
{
    switch (value) {
    case migration_action_kind::copy_file:
        return "copy_file";
    case migration_action_kind::move_file:
        return "move_file";
    case migration_action_kind::copy_directory:
        return "copy_directory";
    case migration_action_kind::move_directory:
        return "move_directory";
    case migration_action_kind::import_registry:
        return "import_registry";
    case migration_action_kind::export_registry:
        return "export_registry";
    case migration_action_kind::write_registry_value:
        return "write_registry_value";
    case migration_action_kind::delete_registry_key:
        return "delete_registry_key";
    case migration_action_kind::write_autostart:
        return "write_autostart";
    case migration_action_kind::write_policy:
        return "write_policy";
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

migration_plan plan_migration(std::vector<migration_action> actions, const migration_options& options)
{
    migration_plan plan;
    plan.actions = std::move(actions);
    plan.dry_run = true;

    if (!options.dry_run) {
        plan.diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-plan-forced-dry-run",
            "Migration plans are always created as dry-run objects; call execute_migration_plan to apply them"));
    }

    for (const auto& action : plan.actions) {
        append_action_gate_diagnostics(action, options, plan.diagnostics);
        append_file_action_diagnostics(action, options, plan.diagnostics);
    }

    return plan;
}

migration_execution_report execute_migration_plan(const migration_plan& plan, const migration_options& options)
{
    migration_execution_report report;
    report.dry_run = options.dry_run;

    if (has_error(plan.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-plan-invalid",
            "Migration plan has errors and cannot be executed"));
        report.actions.reserve(plan.actions.size());
        for (const auto& action : plan.actions) {
            migration_action_result result;
            result.action = action;
            result.planned = true;
            result.skipped = true;
            append_action_gate_diagnostics(action, options, result.diagnostics);
            append_file_action_diagnostics(action, options, result.diagnostics);
            report.actions.push_back(std::move(result));
        }
        return report;
    }

    report.ok = true;
    report.actions.reserve(plan.actions.size());
    for (const auto& action : plan.actions) {
        migration_action_result result;
        result.action = action;
        result.planned = true;
        append_action_gate_diagnostics(action, options, result.diagnostics);
        append_file_action_diagnostics(action, options, result.diagnostics);

        if (has_error(result.diagnostics)) {
            result.skipped = true;
            report.ok = false;
            report.actions.push_back(std::move(result));
            continue;
        }

        if (options.dry_run) {
            result.skipped = true;
            result.diagnostics.push_back(make_diagnostic(
                severity::info,
                "migration-dry-run",
                "Migration action was planned but not executed because dry_run is true"));
            report.actions.push_back(std::move(result));
            continue;
        }

        if (!is_file_action(action.kind)) {
            result.skipped = true;
            report.ok = false;
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-action-not-executable-yet",
                "This migration action kind does not have an executor yet"));
            report.actions.push_back(std::move(result));
            continue;
        }

        std::error_code ec;
        if (options.create_parent_directories) {
            std::filesystem::create_directories(action.target_path.parent_path(), ec);
            if (ec) {
                result.skipped = true;
                report.ok = false;
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-create-parent-failed",
                    ec.message(),
                    action.target_path.parent_path()));
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        const auto copy_options = options.overwrite_existing
            ? std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
            : std::filesystem::copy_options::recursive;

        if (is_directory_action(action.kind) || action.kind == migration_action_kind::copy_file) {
            if (is_directory_action(action.kind)) {
                std::filesystem::copy(action.source_path, action.target_path, copy_options, ec);
            } else {
                std::filesystem::copy_file(
                    action.source_path,
                    action.target_path,
                    options.overwrite_existing
                        ? std::filesystem::copy_options::overwrite_existing
                        : std::filesystem::copy_options::none,
                    ec);
            }
            if (ec) {
                result.skipped = true;
                report.ok = false;
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-copy-failed",
                    ec.message(),
                    action.target_path));
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        if (is_move_action(action.kind)) {
            if (!is_directory_action(action.kind)) {
                std::filesystem::rename(action.source_path, action.target_path, ec);
            } else {
                std::filesystem::remove_all(action.source_path, ec);
            }
            if (ec) {
                result.skipped = true;
                report.ok = false;
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-move-cleanup-failed",
                    ec.message(),
                    action.source_path));
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        result.executed = true;
        report.actions.push_back(std::move(result));
    }

    return report;
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

namespace registry {
namespace {

std::string json_escape(std::string_view value)
{
    std::string output;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            output += "\\\\";
            break;
        case '"':
            output += "\\\"";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output += ch;
            break;
        }
    }
    return output;
}

std::string json_unescape(std::string_view value)
{
    std::string output;
    for (size_t index = 0; index != value.size(); ++index) {
        const char ch = value[index];
        if (ch != '\\' || index + 1 == value.size()) {
            output += ch;
            continue;
        }
        const char escaped = value[++index];
        switch (escaped) {
        case 'n':
            output += '\n';
            break;
        case 'r':
            output += '\r';
            break;
        case 't':
            output += '\t';
            break;
        default:
            output += escaped;
            break;
        }
    }
    return output;
}

char hex_digit(unsigned value)
{
    return static_cast<char>(value < 10 ? '0' + value : 'a' + (value - 10));
}

std::string bytes_to_hex(const std::vector<std::byte>& bytes)
{
    std::string output;
    output.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        const auto value = static_cast<unsigned>(byte);
        output.push_back(hex_digit((value >> 4) & 0x0f));
        output.push_back(hex_digit(value & 0x0f));
    }
    return output;
}

std::optional<unsigned> parse_hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return static_cast<unsigned>(ch - '0');
    }
    if (ch >= 'a' && ch <= 'f') {
        return static_cast<unsigned>(ch - 'a' + 10);
    }
    if (ch >= 'A' && ch <= 'F') {
        return static_cast<unsigned>(ch - 'A' + 10);
    }
    return std::nullopt;
}

std::optional<std::vector<std::byte>> hex_to_bytes(std::string_view hex)
{
    std::string compact;
    for (const char ch : hex) {
        if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        compact.push_back(ch);
    }
    if (compact.size() % 2 != 0) {
        return std::nullopt;
    }

    std::vector<std::byte> output;
    output.reserve(compact.size() / 2);
    for (size_t index = 0; index != compact.size(); index += 2) {
        const auto high = parse_hex_digit(compact[index]);
        const auto low = parse_hex_digit(compact[index + 1]);
        if (!high || !low) {
            return std::nullopt;
        }
        output.push_back(static_cast<std::byte>((*high << 4) | *low));
    }
    return output;
}

std::string trim(std::string_view value)
{
    size_t begin = 0;
    while (begin != value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end != begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return std::string(value.substr(begin, end - begin));
}

std::optional<std::string> json_string_field(std::string_view object, std::string_view field)
{
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find(':', pos + needle.size());
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find('"', pos + 1);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    ++pos;

    std::string raw;
    bool escaped = false;
    for (; pos != object.size(); ++pos) {
        const char ch = object[pos];
        if (escaped) {
            raw.push_back('\\');
            raw.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return json_unescape(raw);
        }
        raw.push_back(ch);
    }
    return std::nullopt;
}

std::optional<std::string_view> json_object_field(std::string_view object, std::string_view field)
{
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find('{', pos + needle.size());
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    size_t depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (size_t index = pos; index != object.size(); ++index) {
        const char ch = object[index];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_string && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return object.substr(pos, index - pos + 1);
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string_view> json_array_objects(std::string_view object, std::string_view field)
{
    std::vector<std::string_view> objects;
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return objects;
    }
    pos = object.find('[', pos + needle.size());
    if (pos == std::string_view::npos) {
        return objects;
    }

    size_t depth = 0;
    size_t object_begin = std::string_view::npos;
    bool in_string = false;
    bool escaped = false;
    for (size_t index = pos + 1; index != object.size(); ++index) {
        const char ch = object[index];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_string && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                object_begin = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && object_begin != std::string_view::npos) {
                objects.push_back(object.substr(object_begin, index - object_begin + 1));
                object_begin = std::string_view::npos;
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return objects;
}

std::optional<hive> hive_from_string(std::string_view value)
{
    if (value == "current_user") {
        return hive::current_user;
    }
    if (value == "local_machine") {
        return hive::local_machine;
    }
    if (value == "classes_root") {
        return hive::classes_root;
    }
    if (value == "users") {
        return hive::users;
    }
    if (value == "current_config") {
        return hive::current_config;
    }
    return std::nullopt;
}

std::optional<view> view_from_string(std::string_view value)
{
    if (value == "native") {
        return view::native;
    }
    if (value == "registry_32") {
        return view::registry_32;
    }
    if (value == "registry_64") {
        return view::registry_64;
    }
    return std::nullopt;
}

std::optional<value_type> value_type_from_string(std::string_view value)
{
    if (value == "none") {
        return value_type::none;
    }
    if (value == "string") {
        return value_type::string;
    }
    if (value == "expandable_string") {
        return value_type::expandable_string;
    }
    if (value == "multi_string") {
        return value_type::multi_string;
    }
    if (value == "dword") {
        return value_type::dword;
    }
    if (value == "qword") {
        return value_type::qword;
    }
    if (value == "binary") {
        return value_type::binary;
    }
    if (value == "unknown") {
        return value_type::unknown;
    }
    return std::nullopt;
}

std::string reg_hive_name(hive value)
{
    switch (value) {
    case hive::current_user:
        return "HKEY_CURRENT_USER";
    case hive::local_machine:
        return "HKEY_LOCAL_MACHINE";
    case hive::classes_root:
        return "HKEY_CLASSES_ROOT";
    case hive::users:
        return "HKEY_USERS";
    case hive::current_config:
        return "HKEY_CURRENT_CONFIG";
    }
    return "HKEY_CURRENT_USER";
}

std::optional<hive> reg_hive_from_name(std::string_view value)
{
    if (value == "HKEY_CURRENT_USER" || value == "HKCU") {
        return hive::current_user;
    }
    if (value == "HKEY_LOCAL_MACHINE" || value == "HKLM") {
        return hive::local_machine;
    }
    if (value == "HKEY_CLASSES_ROOT" || value == "HKCR") {
        return hive::classes_root;
    }
    if (value == "HKEY_USERS" || value == "HKU") {
        return hive::users;
    }
    if (value == "HKEY_CURRENT_CONFIG" || value == "HKCC") {
        return hive::current_config;
    }
    return std::nullopt;
}

std::string reg_escape(std::string_view value)
{
    std::string output;
    for (const char ch : value) {
        if (ch == '\\' || ch == '"') {
            output.push_back('\\');
        }
        output.push_back(ch);
    }
    return output;
}

std::string reg_unescape(std::string_view value)
{
    std::string output;
    bool escaped = false;
    for (const char ch : value) {
        if (escaped) {
            output.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output.push_back(ch);
    }
    return output;
}

std::string bytes_to_reg_hex(const std::vector<std::byte>& bytes)
{
    std::string output;
    for (size_t index = 0; index != bytes.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        const auto value = static_cast<unsigned>(bytes[index]);
        output.push_back(hex_digit((value >> 4) & 0x0f));
        output.push_back(hex_digit(value & 0x0f));
    }
    return output;
}

std::string bytes_to_lossy_string(const std::vector<std::byte>& bytes)
{
    std::string output;
    output.reserve(bytes.size());
    for (const auto byte : bytes) {
        const auto ch = static_cast<char>(byte);
        if (ch == '\0') {
            break;
        }
        output.push_back(ch);
    }
    return output;
}

std::vector<std::byte> string_to_bytes(std::string_view value)
{
    std::vector<std::byte> bytes;
    bytes.reserve(value.size());
    for (const char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::string dword_bytes_to_reg_text(const std::vector<std::byte>& bytes)
{
    unsigned value = 0;
    for (size_t index = 0; index != bytes.size() && index != 4; ++index) {
        value |= static_cast<unsigned>(bytes[index]) << (index * 8);
    }

    std::string output(8, '0');
    for (size_t index = 0; index != output.size(); ++index) {
        const auto shift = static_cast<unsigned>((output.size() - index - 1) * 4);
        output[index] = hex_digit((value >> shift) & 0x0f);
    }
    return output;
}

std::optional<std::vector<std::byte>> dword_reg_text_to_bytes(std::string_view text)
{
    if (text.size() != 8) {
        return std::nullopt;
    }

    unsigned value = 0;
    for (const char ch : text) {
        const auto digit = parse_hex_digit(ch);
        if (!digit) {
            return std::nullopt;
        }
        value = (value << 4) | *digit;
    }

    return std::vector<std::byte>{
        static_cast<std::byte>(value & 0xff),
        static_cast<std::byte>((value >> 8) & 0xff),
        static_cast<std::byte>((value >> 16) & 0xff),
        static_cast<std::byte>((value >> 24) & 0xff),
    };
}

std::string combine_key_path(const key& root, const std::string& relative)
{
    if (relative.empty()) {
        return root.subkey;
    }
    if (root.subkey.empty()) {
        return relative;
    }
    return root.subkey + "\\" + relative;
}

} // namespace

std::string_view to_string(hive value)
{
    switch (value) {
    case hive::current_user:
        return "current_user";
    case hive::local_machine:
        return "local_machine";
    case hive::classes_root:
        return "classes_root";
    case hive::users:
        return "users";
    case hive::current_config:
        return "current_config";
    }
    return "unknown";
}

std::string_view to_string(view value)
{
    switch (value) {
    case view::native:
        return "native";
    case view::registry_32:
        return "registry_32";
    case view::registry_64:
        return "registry_64";
    }
    return "unknown";
}

std::string_view to_string(value_type value)
{
    switch (value) {
    case value_type::none:
        return "none";
    case value_type::string:
        return "string";
    case value_type::expandable_string:
        return "expandable_string";
    case value_type::multi_string:
        return "multi_string";
    case value_type::dword:
        return "dword";
    case value_type::qword:
        return "qword";
    case value_type::binary:
        return "binary";
    case value_type::unknown:
        return "unknown";
    }
    return "unknown";
}

format_report serialize_snapshot_json(const snapshot& snapshot)
{
    format_report report;
    std::ostringstream output;
    output << "{\n";
    output << "  \"format\": \"linuxdesktop.settings.registry.snapshot.v1\",\n";
    output << "  \"root\": {\n";
    output << "    \"hive\": \"" << to_string(snapshot.root.root) << "\",\n";
    output << "    \"subkey\": \"" << json_escape(snapshot.root.subkey) << "\",\n";
    output << "    \"view\": \"" << to_string(snapshot.root.registry_view) << "\"\n";
    output << "  },\n";
    output << "  \"values\": [\n";
    for (size_t index = 0; index != snapshot.values.size(); ++index) {
        const auto& item = snapshot.values[index];
        output << "    {\n";
        output << "      \"key_path\": \"" << json_escape(item.key_path) << "\",\n";
        output << "      \"name\": \"" << json_escape(item.item.name) << "\",\n";
        output << "      \"type\": \"" << to_string(item.item.type) << "\",\n";
        output << "      \"data_hex\": \"" << bytes_to_hex(item.item.bytes) << "\"\n";
        output << "    }";
        if (index + 1 != snapshot.values.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "  ]\n";
    output << "}\n";
    report.content = output.str();
    report.ok = true;
    return report;
}

snapshot_report parse_snapshot_json(std::string_view content)
{
    snapshot_report report;
    const auto root_object = json_object_field(content, "root");
    if (!root_object) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-missing",
            "Registry JSON snapshot is missing the root object"));
        return report;
    }

    const auto hive_text = json_string_field(*root_object, "hive");
    const auto subkey = json_string_field(*root_object, "subkey");
    const auto view_text = json_string_field(*root_object, "view");
    if (!hive_text || !subkey || !view_text) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-invalid",
            "Registry JSON snapshot root requires hive, subkey, and view"));
        return report;
    }

    const auto parsed_hive = hive_from_string(*hive_text);
    const auto parsed_view = view_from_string(*view_text);
    if (!parsed_hive || !parsed_view) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-invalid",
            "Registry JSON snapshot root has an unknown hive or view"));
        return report;
    }

    snapshot parsed;
    parsed.root.root = *parsed_hive;
    parsed.root.subkey = *subkey;
    parsed.root.registry_view = *parsed_view;

    for (const auto value_object : json_array_objects(content, "values")) {
        const auto key_path = json_string_field(value_object, "key_path");
        const auto name = json_string_field(value_object, "name");
        const auto type_text = json_string_field(value_object, "type");
        const auto data_hex = json_string_field(value_object, "data_hex");
        if (!key_path || !name || !type_text || !data_hex) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-json-value-invalid",
                "Registry JSON value requires key_path, name, type, and data_hex"));
            return report;
        }

        const auto parsed_type = value_type_from_string(*type_text);
        const auto bytes = hex_to_bytes(*data_hex);
        if (!parsed_type || !bytes) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-json-value-invalid",
                "Registry JSON value has an unknown type or invalid hex data"));
            return report;
        }

        snapshot_value value;
        value.key_path = *key_path;
        value.item.name = *name;
        value.item.type = *parsed_type;
        value.item.bytes = *bytes;
        parsed.values.push_back(std::move(value));
    }

    report.ok = true;
    report.item = std::move(parsed);
    return report;
}

format_report serialize_snapshot_reg(const snapshot& snapshot)
{
    format_report report;
    std::ostringstream output;
    output << "Windows Registry Editor Version 5.00\n\n";

    std::string current_key;
    for (const auto& item : snapshot.values) {
        const auto full_key = reg_hive_name(snapshot.root.root) + "\\" + combine_key_path(snapshot.root, item.key_path);
        if (full_key != current_key) {
            if (!current_key.empty()) {
                output << "\n";
            }
            current_key = full_key;
            output << "[" << current_key << "]\n";
        }

        if (item.item.name.empty()) {
            output << "@=";
        } else {
            output << "\"" << reg_escape(item.item.name) << "\"=";
        }

        switch (item.item.type) {
        case value_type::string:
            output << "\"" << reg_escape(bytes_to_lossy_string(item.item.bytes)) << "\"";
            break;
        case value_type::expandable_string:
            output << "hex(2):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::multi_string:
            output << "hex(7):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::dword:
            output << "dword:" << dword_bytes_to_reg_text(item.item.bytes);
            break;
        case value_type::qword:
            output << "hex(b):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::binary:
        case value_type::none:
        case value_type::unknown:
            output << "hex:" << bytes_to_reg_hex(item.item.bytes);
            break;
        }
        output << "\n";
    }

    report.content = output.str();
    report.ok = true;
    return report;
}

snapshot_report parse_snapshot_reg(std::string_view content)
{
    snapshot_report report;
    snapshot parsed;
    bool have_root = false;
    std::string current_relative_key;

    std::istringstream input{std::string(content)};
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';') {
            continue;
        }
        if (line == "Windows Registry Editor Version 5.00" || line == "REGEDIT4") {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            const auto full_key = line.substr(1, line.size() - 2);
            const auto slash = full_key.find('\\');
            const auto hive_text = slash == std::string::npos ? full_key : full_key.substr(0, slash);
            const auto hive = reg_hive_from_name(hive_text);
            if (!hive) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-hive-invalid",
                    "Registry file contains an unknown hive"));
                return report;
            }

            const auto subkey = slash == std::string::npos ? std::string{} : full_key.substr(slash + 1);
            if (!have_root) {
                parsed.root.root = *hive;
                parsed.root.subkey = subkey;
                parsed.root.registry_view = view::native;
                have_root = true;
                current_relative_key.clear();
            } else if (*hive != parsed.root.root) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-multiple-hives",
                    "Registry file snapshots must contain one hive"));
                return report;
            } else if (subkey == parsed.root.subkey) {
                current_relative_key.clear();
            } else if (subkey.find(parsed.root.subkey + "\\") == 0) {
                current_relative_key = subkey.substr(parsed.root.subkey.size() + 1);
            } else {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-outside-root",
                    "Registry file contains a key outside the snapshot root"));
                return report;
            }
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos || !have_root) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-invalid",
                "Registry value line is malformed or appears before a key"));
            return report;
        }

        const auto name_text = trim(std::string_view(line).substr(0, equals));
        const auto value_text = trim(std::string_view(line).substr(equals + 1));

        snapshot_value value;
        value.key_path = current_relative_key;
        if (name_text == "@") {
            value.item.name.clear();
        } else if (name_text.size() >= 2 && name_text.front() == '"' && name_text.back() == '"') {
            value.item.name = reg_unescape(std::string_view(name_text).substr(1, name_text.size() - 2));
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-name-invalid",
                "Registry value name is malformed"));
            return report;
        }

        if (value_text.size() >= 2 && value_text.front() == '"' && value_text.back() == '"') {
            value.item.type = value_type::string;
            value.item.bytes = string_to_bytes(reg_unescape(std::string_view(value_text).substr(1, value_text.size() - 2)));
        } else if (value_text.find("dword:") == 0) {
            value.item.type = value_type::dword;
            const auto bytes = dword_reg_text_to_bytes(std::string_view(value_text).substr(6));
            if (!bytes || bytes->empty()) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry DWORD value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(b):") == 0) {
            value.item.type = value_type::qword;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry QWORD value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(2):") == 0) {
            value.item.type = value_type::expandable_string;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry expandable string contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(7):") == 0) {
            value.item.type = value_type::multi_string;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry multi-string contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex:") == 0) {
            value.item.type = value_type::binary;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(4));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry binary value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-data-invalid",
                "Registry value data is not a supported .reg form"));
            return report;
        }

        parsed.values.push_back(std::move(value));
    }

    if (!have_root) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-reg-root-missing",
            "Registry file does not contain a key section"));
        return report;
    }

    report.ok = true;
    report.item = std::move(parsed);
    return report;
}

#if defined(_WIN32)
value_report read_value(const key& key, const std::string& name)
{
    value_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_QUERY_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for read",
            open_status));
        return report;
    }

    DWORD type = REG_NONE;
    DWORD size = 0;
    const auto value_name = widen_utf8(name);
    LSTATUS query_status = RegQueryValueExW(
        handle,
        value_name.c_str(),
        nullptr,
        &type,
        nullptr,
        &size);
    if (query_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-failed",
            "Failed to query Registry value size",
            query_status));
        return report;
    }

    value item;
    item.name = name;
    item.type = registry_type_from_native(type);
    item.bytes.resize(size);
    query_status = RegQueryValueExW(
        handle,
        value_name.c_str(),
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(item.bytes.data()),
        &size);
    RegCloseKey(handle);
    if (query_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-failed",
            "Failed to query Registry value",
            query_status));
        return report;
    }
    item.bytes.resize(size);
    report.ok = true;
    report.item = std::move(item);
    return report;
}

operation_report write_value(const key& key, const value& value, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry value write was planned but not executed"));
        return report;
    }

    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS create_status = RegCreateKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | registry_view_flags(key.registry_view),
        nullptr,
        &handle,
        nullptr);
    if (create_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-create-failed",
            "Failed to create/open Registry key for write",
            create_status));
        return report;
    }

    const auto name = widen_utf8(value.name);
    const auto type = registry_type_to_native(value.type);
    const LSTATUS set_status = RegSetValueExW(
        handle,
        name.c_str(),
        0,
        type,
        reinterpret_cast<const BYTE*>(value.bytes.data()),
        static_cast<DWORD>(value.bytes.size()));
    RegCloseKey(handle);
    if (set_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-write-failed",
            "Failed to write Registry value",
            set_status));
        return report;
    }
    report.ok = true;
    return report;
}

operation_report delete_value(const key& key, const std::string& name, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry value delete was planned but not executed"));
        return report;
    }

    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_SET_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for value delete",
            open_status));
        return report;
    }

    const auto value_name = widen_utf8(name);
    const LSTATUS delete_status = RegDeleteValueW(handle, value_name.c_str());
    RegCloseKey(handle);
    if (delete_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-delete-value-failed",
            "Failed to delete Registry value",
            delete_status));
        return report;
    }
    report.ok = true;
    return report;
}

operation_report delete_key(const key& key, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry key delete was planned but not executed"));
        return report;
    }

    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS delete_status = RegDeleteKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        registry_view_flags(key.registry_view),
        0);
    if (delete_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-delete-key-failed",
            "Failed to delete Registry key",
            delete_status));
        return report;
    }
    report.ok = true;
    return report;
}

values_report enumerate_values(const key& key)
{
    values_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_QUERY_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for value enumeration",
            open_status));
        return report;
    }

    DWORD value_count = 0;
    DWORD max_name_length = 0;
    DWORD max_value_size = 0;
    LSTATUS info_status = RegQueryInfoKeyW(
        handle,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &value_count,
        &max_name_length,
        &max_value_size,
        nullptr,
        nullptr);
    if (info_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-info-failed",
            "Failed to query Registry key metadata",
            info_status));
        return report;
    }

    for (DWORD index = 0; index != value_count; ++index) {
        std::wstring name(max_name_length + 1, L'\0');
        std::vector<std::byte> data(max_value_size);
        DWORD name_size = static_cast<DWORD>(name.size());
        DWORD data_size = static_cast<DWORD>(data.size());
        DWORD type = REG_NONE;
        const LSTATUS enum_status = RegEnumValueW(
            handle,
            index,
            name.data(),
            &name_size,
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(data.data()),
            &data_size);
        if (enum_status != ERROR_SUCCESS) {
            RegCloseKey(handle);
            report.diagnostics.push_back(win32_diagnostic(
                "registry-enum-value-failed",
                "Failed to enumerate Registry value",
                enum_status));
            return report;
        }

        name.resize(name_size);
        data.resize(data_size);
        report.values.push_back({narrow_utf8(name), registry_type_from_native(type), std::move(data)});
    }

    RegCloseKey(handle);
    report.ok = true;
    return report;
}

subkeys_report enumerate_subkeys(const key& key)
{
    subkeys_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_ENUMERATE_SUB_KEYS | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for subkey enumeration",
            open_status));
        return report;
    }

    DWORD subkey_count = 0;
    DWORD max_name_length = 0;
    LSTATUS info_status = RegQueryInfoKeyW(
        handle,
        nullptr,
        nullptr,
        nullptr,
        &subkey_count,
        &max_name_length,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (info_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-info-failed",
            "Failed to query Registry key metadata",
            info_status));
        return report;
    }

    for (DWORD index = 0; index != subkey_count; ++index) {
        std::wstring name(max_name_length + 1, L'\0');
        DWORD name_size = static_cast<DWORD>(name.size());
        const LSTATUS enum_status = RegEnumKeyExW(
            handle,
            index,
            name.data(),
            &name_size,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (enum_status != ERROR_SUCCESS) {
            RegCloseKey(handle);
            report.diagnostics.push_back(win32_diagnostic(
                "registry-enum-subkey-failed",
                "Failed to enumerate Registry subkey",
                enum_status));
            return report;
        }
        name.resize(name_size);
        report.names.push_back(narrow_utf8(name));
    }

    RegCloseKey(handle);
    report.ok = true;
    return report;
}
#else
value_report read_value(const key&, const std::string&)
{
    value_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report write_value(const key&, const value&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report delete_value(const key&, const std::string&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report delete_key(const key&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

values_report enumerate_values(const key&)
{
    values_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

subkeys_report enumerate_subkeys(const key&)
{
    subkeys_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}
#endif

namespace {

bool collect_snapshot_values(const key& root, const std::string& relative, snapshot& output, std::vector<diagnostic>& diagnostics)
{
    key current = root;
    current.subkey = combine_key_path(root, relative);

    const auto values = enumerate_values(current);
    diagnostics.insert(diagnostics.end(), values.diagnostics.begin(), values.diagnostics.end());
    if (!values.ok) {
        return false;
    }
    for (const auto& item : values.values) {
        output.values.push_back({relative, item});
    }

    const auto subkeys = enumerate_subkeys(current);
    diagnostics.insert(diagnostics.end(), subkeys.diagnostics.begin(), subkeys.diagnostics.end());
    if (!subkeys.ok) {
        return false;
    }
    for (const auto& subkey : subkeys.names) {
        const auto child = relative.empty() ? subkey : relative + "\\" + subkey;
        if (!collect_snapshot_values(root, child, output, diagnostics)) {
            return false;
        }
    }

    return true;
}

operation_report import_snapshot(const key& destination, const snapshot& snapshot, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!options.allow_import) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-import-denied",
            "Registry import requires allow_import"));
        return report;
    }

    report.ok = true;
    for (const auto& item : snapshot.values) {
        key target = destination;
        target.subkey = combine_key_path(destination, item.key_path);
        auto write_report = write_value(target, item.item, options);
        report.diagnostics.insert(
            report.diagnostics.end(),
            write_report.diagnostics.begin(),
            write_report.diagnostics.end());
        if (!write_report.ok) {
            report.ok = false;
        }
    }

    return report;
}

} // namespace

format_report export_tree_json(const key& key)
{
    format_report report;
    snapshot snapshot;
    snapshot.root = key;
    if (!collect_snapshot_values(key, {}, snapshot, report.diagnostics)) {
        return report;
    }
    return serialize_snapshot_json(snapshot);
}

operation_report import_tree_json(const key& key, std::string_view content, const options& options)
{
    auto parsed = parse_snapshot_json(content);
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.insert(report.diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    if (!parsed.ok || !parsed.item) {
        return report;
    }
    auto imported = import_snapshot(key, *parsed.item, options);
    imported.diagnostics.insert(imported.diagnostics.begin(), report.diagnostics.begin(), report.diagnostics.end());
    return imported;
}

format_report export_tree_reg(const key& key)
{
    format_report report;
    snapshot snapshot;
    snapshot.root = key;
    if (!collect_snapshot_values(key, {}, snapshot, report.diagnostics)) {
        return report;
    }
    return serialize_snapshot_reg(snapshot);
}

operation_report import_tree_reg(const key& key, std::string_view content, const options& options)
{
    auto parsed = parse_snapshot_reg(content);
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.insert(report.diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    if (!parsed.ok || !parsed.item) {
        return report;
    }
    auto imported = import_snapshot(key, *parsed.item, options);
    imported.diagnostics.insert(imported.diagnostics.begin(), report.diagnostics.begin(), report.diagnostics.end());
    return imported;
}

} // namespace registry

namespace effects {
namespace {

std::string sanitize_autostart_id(std::string value)
{
    return sanitize_segment(std::move(value), "application") + ".desktop";
}

std::string desktop_escape(const std::string& value)
{
    std::string escaped;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string shell_quote(const std::string& value)
{
    if (value.empty()) {
        return "''";
    }
    bool simple = true;
    for (const char ch : value) {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '/' && ch != '.' && ch != '_' && ch != '-' && ch != ':') {
            simple = false;
            break;
        }
    }
    if (simple) {
        return value;
    }
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted += "'";
    return quoted;
}

std::string autostart_command(const autostart_entry& entry)
{
    std::string command = shell_quote(entry.executable.string());
    for (const auto& argument : entry.arguments) {
        command += " ";
        command += shell_quote(argument);
    }
    return command;
}

void append_autostart_validation(const autostart_entry& entry, std::vector<diagnostic>& diagnostics)
{
    if (entry.id.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-id-empty",
            "Autostart entry requires a stable id"));
    }
    if (entry.display_name.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-display-name-empty",
            "Autostart entry requires a display name"));
    }
    if (entry.executable.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-executable-empty",
            "Autostart entry requires an executable path"));
    }
}

std::filesystem::path user_autostart_directory(std::vector<diagnostic>& diagnostics)
{
    if (auto config_home = absolute_env_path("XDG_CONFIG_HOME", diagnostics)) {
        return *config_home / "autostart";
    }
    const auto home = home_directory();
    if (home.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-home-unavailable",
            "Cannot resolve the user autostart directory without HOME or XDG_CONFIG_HOME"));
        return {};
    }
    return home / ".config" / "autostart";
}

std::filesystem::path autostart_directory(const autostart_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.autostart_directory_override.has_value()) {
        return *options.autostart_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/xdg/autostart");
    }
    return user_autostart_directory(diagnostics);
}

std::filesystem::path autostart_path(const autostart_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = autostart_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_autostart_id(entry.id);
}

std::string desktop_file_content(const autostart_entry& entry)
{
    std::ostringstream output;
    output << "[Desktop Entry]\n";
    output << "Type=Application\n";
    output << "Name=" << desktop_escape(entry.display_name) << "\n";
    output << "Exec=" << desktop_escape(autostart_command(entry)) << "\n";
    if (!entry.working_directory.empty()) {
        output << "Path=" << desktop_escape(entry.working_directory.string()) << "\n";
    }
    output << "Terminal=false\n";
    if (!entry.enabled) {
        output << "Hidden=true\n";
    }
    output << "X-LinuxDesktop2026-Autostart=true\n";
    return output.str();
}

bool desktop_file_hidden(const std::string& content)
{
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "Hidden=true" || line == "Hidden=True" || line == "Hidden=1") {
            return true;
        }
    }
    return false;
}

effect_report permission_denied_report(
    const autostart_entry& entry,
    const apply_options& options,
    std::string code,
    std::string message)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.enabled = entry.enabled;
    report.diagnostics.push_back(make_diagnostic(severity::error, std::move(code), std::move(message)));
    return report;
}

#if defined(_WIN32)
registry::key run_key_for(const autostart_entry& entry)
{
    registry::key key;
    key.root = entry.user_scope ? registry::hive::current_user : registry::hive::local_machine;
    key.subkey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    return key;
}

registry::options registry_options_for(const autostart_entry& entry, const apply_options& options)
{
    registry::options registry_options;
    registry_options.dry_run = options.dry_run;
    registry_options.allow_hklm_write = !entry.user_scope && options.allow_global_write;
    return registry_options;
}

std::filesystem::path registry_target_path(const registry::key& key, const std::string& value_name)
{
    return std::filesystem::path(std::string(registry::to_string(key.root)) + "\\" + key.subkey + "\\" + value_name);
}
#endif

std::string sanitize_policy_file_id(std::string value)
{
    return sanitize_segment(std::move(value), "policy") + ".conf";
}

std::string policy_group_name(const policy_entry& entry)
{
    if (!entry.group.empty()) {
        return entry.group;
    }
    return entry.schema_id;
}

void append_policy_validation(const policy_entry& entry, bool require_value, std::vector<diagnostic>& diagnostics)
{
    if (entry.id.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-id-empty",
            "Policy entry requires a stable id"));
    }
    if (entry.schema_id.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-schema-empty",
            "Policy entry requires a schema id"));
    }
    if (policy_group_name(entry).empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-group-empty",
            "Policy entry requires a dconf/GSettings group"));
    }
    if (entry.key.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-key-empty",
            "Policy entry requires a key"));
    }
    if (require_value && entry.value.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-value-empty",
            "Policy entry requires a backend-ready value literal"));
    }
}

std::string dconf_group_from_policy(const policy_entry& entry)
{
    auto group = policy_group_name(entry);
    std::replace(group.begin(), group.end(), '.', '/');
    while (!group.empty() && group.front() == '/') {
        group.erase(group.begin());
    }
    while (!group.empty() && group.back() == '/') {
        group.pop_back();
    }
    return group;
}

std::string dconf_policy_content(const policy_entry& entry)
{
    std::ostringstream output;
    output << "[" << dconf_group_from_policy(entry) << "]\n";
    output << entry.key << "=" << entry.value << "\n";
    output << "# Generated by LinuxDesktop2026 ld_settings.\n";
    return output.str();
}

std::string dconf_lock_content(const policy_entry& entry)
{
    return "/" + dconf_group_from_policy(entry) + "/" + entry.key + "\n";
}

std::filesystem::path user_policy_base_directory(std::vector<diagnostic>& diagnostics)
{
    if (auto config_home = absolute_env_path("XDG_CONFIG_HOME", diagnostics)) {
        return *config_home / "linuxdesktop2026" / "dconf";
    }
    const auto home = home_directory();
    if (home.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-home-unavailable",
            "Cannot resolve the user policy directory without HOME or XDG_CONFIG_HOME"));
        return {};
    }
    return home / ".config" / "linuxdesktop2026" / "dconf";
}

std::filesystem::path policy_defaults_directory(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.policy_defaults_directory_override.has_value()) {
        return *options.policy_defaults_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/dconf/db/local.d");
    }
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        "policy-user-scope-dconf-not-system",
        "User-scope dconf-compatible policy files are generated for inspection; desktop enforcement may require system dconf installation"));
    return user_policy_base_directory(diagnostics) / "defaults";
}

std::filesystem::path policy_locks_directory(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.policy_locks_directory_override.has_value()) {
        return *options.policy_locks_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/dconf/db/local.d/locks");
    }
    return user_policy_base_directory(diagnostics) / "locks";
}

std::filesystem::path policy_defaults_path(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = policy_defaults_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_policy_file_id(entry.id);
}

std::filesystem::path policy_lock_path(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = policy_locks_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_policy_file_id(entry.id);
}

bool has_policy_lock(const std::string& content, const policy_entry& entry)
{
    std::istringstream input(content);
    const auto expected = dconf_lock_content(entry);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line + "\n" == expected) {
            return true;
        }
    }
    return false;
}

std::optional<std::string> read_policy_value_from_keyfile(const std::string& content, const policy_entry& entry)
{
    std::istringstream input(content);
    const auto expected_group = dconf_group_from_policy(entry);
    bool in_group = false;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            in_group = line.substr(1, line.size() - 2) == expected_group;
            continue;
        }
        const auto separator = line.find('=');
        if (in_group && separator != std::string::npos && line.substr(0, separator) == entry.key) {
            return line.substr(separator + 1);
        }
    }
    return std::nullopt;
}

#if defined(_WIN32)
std::string registry_policy_leaf(const policy_entry& entry)
{
    auto leaf = entry.schema_id.empty() ? entry.id : entry.schema_id;
    std::replace(leaf.begin(), leaf.end(), '.', '\\');
    std::replace(leaf.begin(), leaf.end(), '/', '\\');
    std::replace(leaf.begin(), leaf.end(), ':', '-');
    if (!entry.group.empty()) {
        auto group = entry.group;
        std::replace(group.begin(), group.end(), '.', '\\');
        std::replace(group.begin(), group.end(), '/', '\\');
        std::replace(group.begin(), group.end(), ':', '-');
        leaf += "\\" + group;
    }
    return leaf;
}

registry::key policy_key_for(const policy_entry& entry)
{
    registry::key key;
    key.root = entry.user_scope ? registry::hive::current_user : registry::hive::local_machine;
    key.subkey = "Software\\Policies\\" + registry_policy_leaf(entry);
    return key;
}

registry::options registry_options_for(const policy_entry& entry, const apply_options& options)
{
    registry::options registry_options;
    registry_options.dry_run = options.dry_run;
    registry_options.allow_policy_write = options.allow_policy_write;
    registry_options.allow_hklm_write = !entry.user_scope && options.allow_global_write;
    return registry_options;
}
#endif

} // namespace

effect_report apply_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.enabled = entry.enabled;
    append_autostart_validation(entry, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-global-write-denied",
            "Machine-wide autostart changes require allow_global_write"));
    }
    if (!options.allow_desktop_integration_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-write-denied",
            "Autostart changes require allow_desktop_integration_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.path = registry_target_path(run_key_for(entry), entry.id);
    if (options.dry_run) {
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Autostart Registry value was planned but not written",
            *report.path));
    }
    registry::value value;
    value.name = entry.id;
    value.type = registry::value_type::string;
    const auto command = autostart_command(entry);
    value.bytes.assign(
        reinterpret_cast<const std::byte*>(command.data()),
        reinterpret_cast<const std::byte*>(command.data() + command.size()));
    const auto written = registry::write_value(run_key_for(entry), value, registry_options_for(entry, options));
    report.ok = written.ok;
    report.diagnostics.insert(report.diagnostics.end(), written.diagnostics.begin(), written.diagnostics.end());
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Autostart desktop entry was planned but not written",
            *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-create-directory-failed",
            ec.message(),
            report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, desktop_file_content(entry))) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-write-failed",
            "Failed to write XDG autostart desktop entry",
            *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report remove_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    append_autostart_validation(entry, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-global-write-denied",
            "Machine-wide autostart changes require allow_global_write"));
    }
    if (!options.allow_desktop_integration_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-write-denied",
            "Autostart changes require allow_desktop_integration_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.path = registry_target_path(run_key_for(entry), entry.id);
    const auto deleted = registry::delete_value(run_key_for(entry), entry.id, registry_options_for(entry, options));
    report.ok = deleted.ok;
    report.diagnostics.insert(report.diagnostics.end(), deleted.diagnostics.begin(), deleted.diagnostics.end());
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Autostart desktop entry removal was planned but not applied",
            *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-remove-failed",
            ec.message(),
            *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report query_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    append_autostart_validation(entry, report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.path = registry_target_path(run_key_for(entry), entry.id);
    const auto value = registry::read_value(run_key_for(entry), entry.id);
    report.ok = value.ok;
    report.enabled = value.ok && value.item.has_value();
    report.diagnostics.insert(report.diagnostics.end(), value.diagnostics.begin(), value.diagnostics.end());
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    if (!std::filesystem::exists(*report.path, ec)) {
        report.ok = true;
        report.enabled = false;
        return report;
    }
    std::error_code read_ec;
    const auto content = read_text(*report.path, read_ec);
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-read-failed",
            read_ec.message(),
            *report.path));
        return report;
    }
    report.ok = true;
    report.enabled = !desktop_file_hidden(content);
    return report;
#endif
}

policy_report apply_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    report.dry_run = options.dry_run;
    report.enforced = entry.enforced;
    append_policy_validation(entry, true, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-global-write-denied",
            "Machine-wide policy changes require allow_global_write"));
    }
    if (!options.allow_policy_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-write-denied",
            "Managed/enforced policy changes require allow_policy_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.present = true;

#if defined(_WIN32)
    report.path = registry_target_path(policy_key_for(entry), entry.key);
    if (options.dry_run) {
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "policy-dry-run",
            "Managed/enforced policy Registry value was planned but not written",
            *report.path));
    }
    registry::value value;
    value.name = entry.key;
    value.type = registry::value_type::string;
    value.bytes.assign(
        reinterpret_cast<const std::byte*>(entry.value.data()),
        reinterpret_cast<const std::byte*>(entry.value.data() + entry.value.size()));
    const auto written = registry::write_value(policy_key_for(entry), value, registry_options_for(entry, options));
    report.ok = written.ok;
    report.value = entry.value;
    report.diagnostics.insert(report.diagnostics.end(), written.diagnostics.begin(), written.diagnostics.end());
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.value = entry.value;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "policy-dry-run",
            "Managed/enforced policy file was planned but not written",
            *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-create-directory-failed",
            ec.message(),
            report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, dconf_policy_content(entry))) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-write-failed",
            "Failed to write dconf-compatible policy defaults file",
            *report.path));
        return report;
    }
    if (entry.enforced) {
        const auto lock_path = policy_lock_path(entry, options, report.diagnostics);
        if (lock_path.empty() || has_error(report.diagnostics)) {
            return report;
        }
        std::filesystem::create_directories(lock_path.parent_path(), ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-lock-create-directory-failed",
                ec.message(),
                lock_path.parent_path()));
            return report;
        }
        if (!write_file_content(lock_path, dconf_lock_content(entry))) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-lock-write-failed",
                "Failed to write dconf-compatible policy lock file",
                lock_path));
            return report;
        }
    }
    report.ok = true;
    report.value = entry.value;
    return report;
#endif
}

policy_report remove_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    report.dry_run = options.dry_run;
    append_policy_validation(entry, false, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-global-write-denied",
            "Machine-wide policy changes require allow_global_write"));
    }
    if (!options.allow_policy_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-write-denied",
            "Managed/enforced policy changes require allow_policy_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.path = registry_target_path(policy_key_for(entry), entry.key);
    const auto deleted = registry::delete_value(policy_key_for(entry), entry.key, registry_options_for(entry, options));
    report.ok = deleted.ok;
    report.diagnostics.insert(report.diagnostics.end(), deleted.diagnostics.begin(), deleted.diagnostics.end());
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "policy-dry-run",
            "Managed/enforced policy file removal was planned but not applied",
            *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-remove-failed",
            ec.message(),
            *report.path));
        return report;
    }
    const auto lock_path = policy_lock_path(entry, options, report.diagnostics);
    if (!lock_path.empty()) {
        std::filesystem::remove(lock_path, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-lock-remove-failed",
                ec.message(),
                lock_path));
            return report;
        }
    }
    report.ok = true;
    return report;
#endif
}

policy_report query_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    append_policy_validation(entry, false, report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    const auto value = registry::read_value(policy_key_for(entry), entry.key);
    report.ok = value.ok;
    report.present = value.ok && value.item.has_value();
    if (value.item) {
        report.value = std::string(
            reinterpret_cast<const char*>(value.item->bytes.data()),
            value.item->bytes.size());
    }
    report.diagnostics.insert(report.diagnostics.end(), value.diagnostics.begin(), value.diagnostics.end());
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    if (!std::filesystem::exists(*report.path, ec)) {
        report.ok = true;
        report.present = false;
        return report;
    }
    std::error_code read_ec;
    const auto content = read_text(*report.path, read_ec);
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "policy-read-failed",
            read_ec.message(),
            *report.path));
        return report;
    }
    report.ok = true;
    report.value = read_policy_value_from_keyfile(content, entry);
    report.present = report.value.has_value();

    const auto lock_path = policy_lock_path(entry, options, report.diagnostics);
    if (!lock_path.empty() && std::filesystem::exists(lock_path, ec)) {
        const auto lock_content = read_text(lock_path, read_ec);
        if (read_ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-lock-read-failed",
                read_ec.message(),
                lock_path));
            report.ok = false;
            return report;
        }
        report.enforced = has_policy_lock(lock_content, entry);
    }
    return report;
#endif
}

} // namespace effects

} // namespace linuxdesktop::settings
