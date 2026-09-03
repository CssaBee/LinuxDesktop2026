# 70 - Make Dconf Policy Activation Honest

**What to build:** Make managed/enforced policy capability reporting match what
`ld_desktop` actually activates.

**Blocked by:** 69 - Share Durable File Write Primitive With Desktop.

**Status:** pending

- [ ] Decide whether `ld_desktop` invokes `dconf update`, reports manual
  activation as required, or marks managed policy as backend-limited.
- [ ] If activation is implemented, use an explicit process execution path with
  diagnostics for missing tools, permission failure, and nonzero exit.
- [ ] If activation is not implemented, adjust `query_capabilities()` and apply
  diagnostics so callers cannot mistake file generation for active policy.
- [ ] Add tests for capability reporting and apply diagnostics.

## Review Anchor

Both reviews point at dconf policy state: the implementation writes source
files but does not run `dconf update`, while `query_capabilities()` currently
reports managed policy as supported on non-Windows platforms.
