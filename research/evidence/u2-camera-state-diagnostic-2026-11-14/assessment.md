# U2 Camera State Diagnostic Log

## Implementation

The existing `[Diagnostics] Enabled` setting controls a bounded asynchronous
writer log. The primary camera FOV writer records separate `BEFORE`, `DECISION`,
and `POST_STORE` events linked by `writerTransactionId`. Each record carries
receiver/output/field addresses, site ID, thread, lifecycle/settings epochs,
coordinator and writer context, policy inputs, decision reason, planned output,
and readback status.

The post-store value is labeled `MatchedAtCorrelatedSite` or
`DifferentAtCorrelatedSite`. This is an observation at the correlated site; it
does not claim exclusive authorship if another writer could interleave. Missing
or mismatched observations are reported as unavailable/uncertain.

Records enter a fixed 8192-event queue using a nonblocking producer lock. A
WorkerLifecycle-managed worker drains and serializes them. Periodic and stop
summaries report accepted events, queue-full drops, producer-contention drops,
transactions, post-store observations, mismatches, and events that could not be
queued because the worker was unavailable. No FOV policy or transform was
changed.

## Coverage

The log currently instruments writer site ID 1: the validated primary camera
FOV store and its validated post-store observation site. It records that site
as available/unavailable at startup and explicitly reports global coverage as
unknown. Other writes to the shared camera field have not been proven absent.

## Validation

- Production Release CMake/Ninja build and PE inspection: PASS; AMD64 PE32+.
- Focused camera writer identity/log queue harness: PASS, including concurrent
  producers, monotonic event ordering, exact accepted/full/contention accounting,
  and drain count.
- Worker lifecycle harness: PASS, including five-worker capacity.
- Camera writer transaction harness: PASS.
- Canonical non-graphics harness build: PASS. Canonical runner stopped at
  `config_persistence_harness` (10/62) because the existing hardlink fixture
  setup returned Windows error 5 (`occupied=1`); later harnesses were not run.
- Runtime adapter non-graphics case was not accepted by the direct targeted
  invocation (`--non-graphics` was parsed as an unknown case); no runtime adapter
  PASS is claimed for this candidate.
- No game run, runtime capture, or in-game frame-time comparison has occurred.

## Candidate

See `candidate-manifest.json`. The candidate is staged under the build artifacts
and has not replaced the game ASI. A complete user-run capture is the next gate.
