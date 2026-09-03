# 81 - Add Coverage Failure-Mode And Watch Performance Evidence

**What to build:** Add evidence that tests exercise the risky states reviewers
keep finding late.

**Blocked by:** 62 - Bound And Coalesce Settled-File Readiness Work; 63 -
Replace Blocking Settle FIFO With Deadline Scheduler.

**Status:** pending

- [ ] Add coverage reporting or an equivalent local coverage artifact for
  active modules.
- [ ] Add deterministic failure-mode tests for disk-full-like writes,
  permission denial, resource-limit paths, and cross-device-like behavior.
- [ ] Add watcher burst/load measurements for event throughput, latency, queue
  depth, and memory growth.
- [ ] Use the data to decide whether hot-path `std::filesystem::path`
  allocation is a real problem.

## Review Anchor

The broad review found no coverage reporting, no benchmarks, and limited
failure-mode evidence. It also raised `std::filesystem::path` hot-path costs as
speculative until profiling exists.
