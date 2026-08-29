#pragma once

#include "linuxdesktop/core.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop::desktop {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

using ::linuxdesktop::diagnostic;
using ::linuxdesktop::severity;
using ::linuxdesktop::to_string;

enum class effect_kind {
    autostart,
    desktop_entry,
    icon,
    mime_association,
    default_application,
    url_protocol_handler,
    shell_integration,
    desktop_database,
    managed_policy
};

enum class capability_state {
    supported,
    unsupported,
    backend_missing,
    sandbox_limited,
    permission_denied
};

struct capability {
    effect_kind kind = effect_kind::autostart;
    capability_state state = capability_state::unsupported;
    bool can_query = false;
    bool can_dry_run = false;
    bool can_write_user = false;
    bool can_write_global = false;
    std::vector<diagnostic> diagnostics;
};

struct capability_report {
    std::vector<capability> effects;
    std::vector<diagnostic> diagnostics;
};

struct autostart_entry {
    std::string id;
    std::string display_name;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    bool enabled = true;
    bool user_scope = true;
};

struct apply_options {
    bool dry_run = true;
    bool allow_global_write = false;
    bool allow_desktop_integration_write = false;
    bool allow_policy_write = false;
    std::optional<std::filesystem::path> autostart_directory_override;
    std::optional<std::filesystem::path> policy_defaults_directory_override;
    std::optional<std::filesystem::path> policy_locks_directory_override;
};

struct effect_report {
    bool ok = false;
    bool dry_run = false;
    bool enabled = false;
    std::optional<std::filesystem::path> path;
    std::vector<diagnostic> diagnostics;
};

struct policy_entry {
    std::string id;
    std::string schema_id;
    std::string group;
    std::string key;
    std::string value;
    bool enforced = false;
    bool user_scope = false;
};

struct policy_report {
    bool ok = false;
    bool dry_run = false;
    bool present = false;
    bool enforced = false;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> value;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(effect_kind value);
std::string_view to_string(capability_state value);

capability_report query_capabilities(const apply_options& options = {});

effect_report apply_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report remove_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report query_autostart(const autostart_entry& entry, const apply_options& options = {});

policy_report apply_policy(const policy_entry& entry, const apply_options& options = {});
policy_report remove_policy(const policy_entry& entry, const apply_options& options = {});
policy_report query_policy(const policy_entry& entry, const apply_options& options = {});

} // namespace linuxdesktop::desktop
