# Expanded `ld_settings` API Plan

This plan captures the post-grilling shape of `ld_settings`. It supersedes the idea that the module is only a standard-path resolver.

`ld_settings` is a toolkit-neutral configuration placement, lifecycle, Registry compatibility, and policy-effect library.

It still does not parse every application's settings payload. Applications keep ownership of XML, JSON, INI, TOML, database, and domain-specific validation unless they choose to use the Registry value API.

## Shippable Scope

`ld_settings` is shippable only when the following are implemented and verified:

- fixed standard roots,
- named roots,
- component roots,
- portable levels,
- all config layers: `defaults`, `global`, `user`, `local`, `portable`, `managed`, `enforced`,
- default precedence with enforced values non-overridable,
- hydration of default/model config files,
- migration plans with dry-run default and explicit execution,
- atomic file writes with validation and backups,
- Windows Known Folder resolution,
- full practical Windows Registry support,
- JSON canonical Registry import/export,
- `.reg` compatibility import/export,
- autostart effect implementation on Windows and Linux,
- managed/enforced policy implementation on Windows and Linux,
- C ABI coverage for first-public concepts,
- examples from Notepad++, ShareX, WinSCP, KeePassXC, and PortableApps-style workflows,
- Windows CI or equivalent automated Windows verification for Registry, Known Folders, C ABI, and atomic writes,
- manual Windows verification transcript before release.

No half-finished product: documentation-only placeholders are acceptable during development, but not for the first declared shippable release.

## Proposed Namespaces

```cpp
namespace linuxdesktop::settings;
namespace linuxdesktop::settings::registry;
namespace linuxdesktop::settings::effects;
```

Rationale:

- root/config lifecycle remains easy to discover,
- raw Registry operations do not pollute the root API,
- effect APIs can model autostart and policy without pretending every Registry effect belongs to config files.

## Core Types

### Portable Level

```cpp
enum class portable_level {
    off,
    settings_only,
    profile,
    clean
};
```

### Root Purpose

```cpp
enum class root_purpose {
    resources,
    config,
    data,
    state,
    cache,
    runtime,
    session,
    plugin_config,
    logs,
    profiles,
    backup,
    temp,
    component_config,
    component_data,
    component_state,
    managed_config,
    enforced_config,
    custom
};
```

### Persistence Class

```cpp
enum class persistence_class {
    roaming,
    machine_local,
    portable,
    ephemeral,
    managed,
    enforced
};
```

### Named Root

```cpp
struct named_root_request {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path relative_path;
    bool create = true;
};

struct named_root {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path path;
    bool created = false;
    std::vector<diagnostic> diagnostics;
};
```

Why vector, not map:

- C ABI can expose arrays naturally,
- ordering remains deterministic,
- metadata stays attached to each root,
- duplicate names can be diagnosed explicitly.

### Component Roots

```cpp
enum class component_kind {
    plugin,
    embedded_tool,
    profile,
    language_pack,
    extension,
    custom
};

struct component_root_request {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root_request> roots;
};

struct component_root_group {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root> roots;
    std::vector<diagnostic> diagnostics;
};
```

Rule: component roots resolve paths only. Hydration, writes, migration, and Registry import/export operate through separate APIs.

## Config Layers

```cpp
enum class config_layer_kind {
    defaults,
    global,
    user,
    local,
    portable,
    managed,
    enforced
};

enum class storage_backend {
    file,
    registry,
    null_backend,
    override_values,
    app_callback
};

struct config_layer {
    config_layer_kind kind;
    storage_backend backend;
    std::string name;
    std::filesystem::path path;
    bool writable = false;
    bool required = false;
    bool enforced = false;
    int precedence = 0;
};

struct layer_report {
    std::vector<config_layer> candidates;
    std::vector<config_layer> active_read_order;
    std::optional<config_layer> active_write_layer;
    std::vector<diagnostic> diagnostics;
};
```

Default precedence:

```text
defaults < global < user < local < portable < managed < enforced
```

The report should include all candidates because migrations often need to explain why a different store won.

## Merge Boundary

`ld_settings` should not become a universal parser.

```cpp
using merge_callback = std::function<bool(
    const std::vector<config_layer>& read_order,
    const std::filesystem::path& output_path,
    std::string& error)>;
```

Rules:

- app-owned XML remains app-owned,
- app-owned JSON/INI/TOML remains app-owned unless the app opts into helper adapters later,
- Registry value import/export is owned by `ld_settings::registry`,
- merge callbacks let the app decide content semantics.

## Migration Plans

Hydration copies missing defaults. Migration moves or transforms existing user state. Keep them separate.

```cpp
enum class migration_action_kind {
    copy_file,
    move_file,
    copy_directory,
    move_directory,
    import_registry,
    export_registry,
    write_registry_value,
    delete_registry_key,
    write_autostart,
    write_policy
};

struct migration_action {
    migration_action_kind kind;
    std::string name;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
    bool dangerous = false;
    bool requires_elevation = false;
};

struct migration_plan {
    std::vector<migration_action> actions;
    bool dry_run = true;
    std::vector<diagnostic> diagnostics;
};
```

Execution rules:

- dry-run first by default,
- destructive and global operations require explicit flags,
- every executed action reports before/after state where practical,
- rollback support is required for portable run-scoped registry/file moves before release.

## Registry API

### Registry Types

```cpp
namespace linuxdesktop::settings::registry {

enum class hive {
    current_user,
    local_machine,
    classes_root,
    users,
    current_config
};

enum class view {
    native,
    registry_32,
    registry_64
};

enum class value_type {
    none,
    string,
    expandable_string,
    multi_string,
    dword,
    qword,
    binary,
    unknown
};

struct key {
    hive root;
    std::string subkey;
    view registry_view = view::native;
};

struct value {
    std::string name;
    value_type type;
    std::vector<std::byte> bytes;
};

struct options {
    bool allow_hklm_write = false;
    bool allow_policy_write = false;
    bool allow_recursive_delete = false;
    bool allow_import = false;
    bool dry_run = true;
};

}
```

### Registry Operations

First shippable API should include:

- `read_value`,
- `write_value`,
- `delete_value`,
- `delete_key`,
- `enumerate_values`,
- `enumerate_subkeys`,
- `export_tree_json`,
- `import_tree_json`,
- `export_tree_reg`,
- `import_tree_reg`.

Safety rules:

- default access is read-only,
- writes to HKLM require explicit permission and real platform capability,
- recursive deletion requires explicit permission,
- imports require explicit permission,
- policy writes require explicit permission,
- Linux builds return unsupported diagnostics for raw Registry operations unless reading/writing a portable Registry snapshot file.

## Effects API

Only these effects are first-scope:

- autostart,
- managed preference,
- enforced preference.

These effects are explicitly unsupported in first `ld_settings` and should become GitHub issue requests:

- file associations,
- default applications,
- protocol handlers,
- shell context menus,
- recent documents,
- jump lists,
- services,
- COM/shell extension registration.

### Autostart

```cpp
namespace linuxdesktop::settings::effects {

struct autostart_entry {
    std::string id;
    std::string display_name;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    bool enabled = true;
    bool user_scope = true;
};

struct apply_options {
    bool dry_run = true;
    bool allow_global_write = false;
    bool allow_desktop_integration_write = false;
    bool allow_policy_write = false;
};

}
```

Windows backend:

- write/remove/check `CurrentVersion\Run` entries,
- support per-user first,
- require explicit global flag for machine-wide startup.

Linux backend:

- write/remove/check XDG Autostart `.desktop` files,
- support `Hidden=true` for disable,
- validate desktop entry fields before writing.

### Managed And Enforced Preferences

First C++ API shape:

```cpp
namespace linuxdesktop::settings::effects {

struct policy_entry {
    std::string id;
    std::string schema_id;
    std::string group;
    std::string key;
    std::string value;
    bool enforced = false;
    bool user_scope = false;
};

policy_report apply_policy(const policy_entry&, const apply_options& = {});
policy_report remove_policy(const policy_entry&, const apply_options& = {});
policy_report query_policy(const policy_entry&, const apply_options& = {});

}
```

Windows backend:

- Registry policy keys and app-owned HKLM/HKCU layers.
- `Software\Policies\<schema/group>` values are written through the raw Registry backend.
- global writes require both `allow_policy_write` and `allow_global_write`.

Linux backend:

- dconf/GSettings-compatible defaults and lock files where schemas exist, without linking GLib.
- defaults are emitted as keyfiles; enforced settings also emit lock files.
- the initial implementation supports override directories for safe tests and staged package generation.
- Return diagnostics when no schema/backend exists.

## C ABI Requirements

The C ABI must expose named roots and config-layer reports in the first public API.

Shape:

- arrays with explicit counts,
- UTF-8 strings owned by report objects,
- one matching free function per report family,
- no STL types or exceptions,
- enum values fixed and documented,
- runtime version functions must match header macros.

Registry C ABI can lag behind C++ during development, but not for the first shippable release.

## Example Targets

Required examples before ship:

- Notepad++ settings root resolution with named plugin/session/log roots.
- Notepad++ model XML hydration and ordered writes.
- ShareX personal path resolution with portable marker, registry source, migration from old path, and named roots.
- WinSCP-style Registry/INI/null/override storage selection.
- KeePassXC-style roaming/local split and Linux state migration.
- PortableApps-style registry snapshot/restore dry-run.
- Autostart enable/disable on Windows and Linux.
- Managed/enforced policy plan on Windows and Linux.

## Implementation Order

1. Update docs and examples to reflect the expanded API.
2. Add named roots and component roots to C++ and C ABI.
3. Add config layers and precedence reports.
4. Add portable levels.
5. Add migration plans as dry-run objects. `(initial C++ API implemented)`
6. Implement Windows Registry raw operations. `(initial C++ API/backend implemented; Windows verification pending)`
7. Add JSON Registry import/export. `(initial C++ snapshot format and tree wrappers implemented)`
8. Add `.reg` compatibility import/export. `(initial C++ snapshot format and tree wrappers implemented)`
9. Implement autostart Windows and Linux backends. `(initial C++ API/backend implemented; Windows verification pending)`
10. Implement managed/enforced policy Windows and Linux backends. `(initial C++ API/backend implemented; Windows verification pending)`
11. Add Windows CI and manual verification transcript. `(CI exists; transcript remains pending after a real Windows run)`
12. Expand the C ABI past roots/layers. `(autostart and policy effects implemented; migration and Registry remain pending)`

## Current Status

`ld_settings` is a working first sample, not a shippable final module.

Implemented in the current C++ sample:

- named roots,
- component roots,
- config layer candidates and active read/write ordering,
- portable levels,
- string names for public root/layer enums,
- C++ lookup helpers for named roots, component roots, component-local roots, and config layers,
- C ABI exposure for named roots, component roots, config layers, and portable levels,
- dry-run-first migration plans with file/directory copy and move execution,
- raw `ld_settings::registry` C++ API for read, write, delete, and enumeration,
- Windows Registry backend seed using Win32 Registry APIs,
- structured unsupported diagnostics for raw Registry calls on non-Windows platforms,
- canonical JSON Registry snapshot serialization/parsing,
- `.reg` snapshot serialization/parsing for string, expandable string, multi-string, DWORD, QWORD, and binary-shaped values,
- Registry tree JSON/`.reg` import/export wrappers over the raw Registry API,
- autostart effect API with dry-run-first writes,
- Linux XDG Autostart `.desktop` write/query/remove support,
- Windows `CurrentVersion\Run` autostart backend shape over the raw Registry API,
- managed/enforced policy effect API with dry-run-first writes,
- Linux dconf/GSettings-compatible defaults and lock-file generation without GLib,
- Windows `Software\Policies` backend shape over the raw Registry API.

The next code work should start with C ABI coverage for migration and Registry concepts. Windows CI or a Windows container/manual run must verify the Registry backend, autostart backend, policy backend, and tree import/export before the module can be considered shippable.
