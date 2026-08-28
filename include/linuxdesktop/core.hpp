#pragma once

#include <filesystem>
#include <string>

namespace linuxdesktop {

enum class severity {
    info,
    warning,
    error
};

struct diagnostic {
    severity level = severity::info;
    std::string code;
    std::string message;
    std::filesystem::path path;
};

inline std::string to_string(severity value)
{
    switch (value) {
    case severity::info:
        return "info";
    case severity::warning:
        return "warning";
    case severity::error:
        return "error";
    }
    return "unknown";
}

} // namespace linuxdesktop
