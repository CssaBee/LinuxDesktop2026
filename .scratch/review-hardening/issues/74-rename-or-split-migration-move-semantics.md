# 74 - Rename Or Split Migration Move Semantics

**What to build:** Align migration action names with the now-narrow filesystem
contract.

**Blocked by:** 60 - Narrow And Harden Migration Filesystem Semantics.

**Status:** pending

- [ ] Decide whether pre-1.0 `move_file` should become an explicitly atomic
  rename action name.
- [ ] Decide whether semantic cross-device move belongs as a separate future
  action with copy/verify/remove behavior.
- [ ] Update helper names, examples, FlavorTests, and migration docs if the
  action taxonomy changes.
- [ ] Add compatibility guidance for any pre-1.0 callers using the old names.

## Review Anchor

The newer review says migration is now honest about atomic rename-only file
moves, but the public `move_file` name still suggests broader semantic moving
than the contract provides.
