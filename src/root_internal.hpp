#pragma once

#include "linuxdesktop/root.hpp"

#include <system_error>

namespace linuxdesktop::root::detail {

diagnostic make_diagnostic(
    severity level,
    std::string code,
    std::string message,
    std::filesystem::path path = {});

std::string sanitize_segment(std::string value, std::string fallback = {});
std::error_code system_error_code();

void create_directory_if_needed(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics);

bool create_directory_for_root(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics);

void ensure_root_directory(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics);

bool has_error(const std::vector<diagnostic>& diagnostics);

} // namespace linuxdesktop::root::detail
