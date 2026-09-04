#pragma once

#include "linuxdesktop/core.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace linuxdesktop::detail {

using file_validation_callback = std::function<bool(const std::filesystem::path&, std::string&)>;

struct durable_file_write_options {
    std::filesystem::path target;
    std::string content;
    bool keep_backup = true;
    bool atomic_replace = true;
    bool durable_write = false;
};

struct durable_file_write_report {
    bool ok = false;
    std::optional<std::filesystem::path> backup_path;
    std::optional<std::filesystem::path> temp_path;
    bool durable_write = false;
    std::vector<diagnostic> diagnostics;
};

diagnostic make_diagnostic(
    severity level,
    std::string code,
    std::string message,
    std::filesystem::path path = {});

std::error_code system_error_code();

void create_directory_if_needed(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics);

bool has_error(const std::vector<diagnostic>& diagnostics);

std::string read_text_file(const std::filesystem::path& path, std::error_code& ec);

durable_file_write_report write_durable_file(
    const durable_file_write_options& options,
    file_validation_callback validate = {});

} // namespace linuxdesktop::detail
