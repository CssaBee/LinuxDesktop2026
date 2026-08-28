# 02 — Make Pre-RC Compatibility Policy Honest

**What to build:** The compatibility policy should set accurate expectations before release-candidate status: C++ APIs are source-compatible only, existing C ABI entry points are maintained where practical, and no new C ABI or binary-stability design is pursued yet.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] The stability policy explicitly says C++ binary ABI is not promised while public values expose standard-library types.
- [x] Existing C ABI behavior is kept compatible where practical, but C ABI expansion is postponed until release-candidate status.
- [x] Release notes or migration guidance are required for any pre-1.0 source break or existing C ABI break.
