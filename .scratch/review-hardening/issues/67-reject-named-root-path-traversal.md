# 67 - Reject Named-Root Path Traversal

**What to build:** Prevent named-root requests from escaping their selected
base directory through `..` path segments.

**Blocked by:** None.

**Status:** done

- [x] Reject explicit `relative_path` values containing parent-directory
  traversal.
- [x] Ensure default relative paths derived from `name` cannot become `..`.
- [x] After combining base and relative path, verify the result is at or under
  the chosen base.
- [x] Add adversarial tests for `name=".."`, nested `../`, and mixed normal
  segments.

## Review Anchor

The broad review found `resolve_named_root()` rejects absolute relative paths
but not `..`; `sanitize_segment()` also leaves `..` unchanged.
