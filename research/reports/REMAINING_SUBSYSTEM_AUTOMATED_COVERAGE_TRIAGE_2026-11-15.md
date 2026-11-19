# Remaining Subsystem Automated Coverage Triage — 2026-11-15

## Scope

Triage of native hook installation/feature availability, runtime settings
persistence, Overlay owner/presenter lifecycle, localization/resource
contracts, and release packaging after the camera writer coverage batch. This
is not a full repository or security review. No production behavior changed.
A release-package test was added after the read-only triage identified that
concrete gap; the focused test and canonical non-graphics suite were then run
under process-tree supervision.

## Findings

### Native hook installation and feature availability — adequately covered offline

- `cinematics::InitializeTransactional` is production code exercised by both
  `cinematic_initialization_harness` and the compiled actual-runtime adapter
  scenario `evidence-install-transaction-order`.
- The adapter scenario injects returned failure and exceptions at aspect
  install, FOV install, and commit, then checks reverse rollback ordering. The
  focused harness also covers disabled initialization, success, component
  failure, and commit failure.
- `feature_status_harness` covers Dialogue capability dependencies and
  isolation of a failed feature from peers. Runtime adapter scenarios cover
  selected early/partial startup failure and native reset ownership.
- Current source review found no additional deterministic install/rollback
  branch without a corresponding production-helper or adapter test. These
  tests still do not prove resolution or activation against a live game image.

### Runtime settings and persistence — interaction is covered

- `RuntimeSettingsApi::ApplyAndPersist` serializes apply plus persistence and
  returns both outcomes separately. The Overlay settings and hotkey paths
  surface a persistence error after an accepted runtime mutation.
- `semantic_snapshot_harness::TestMutationPersistenceOrdering` exercises
  concurrent commits and persistence failure. It verifies that the effective
  value remains applied while the previous persisted value remains unchanged,
  and that the result reports `accepted=true, persisted=false`.
- Configuration persistence tests separately cover atomic multi-value writes,
  staging/replacement failures, preservation of original data, and the named
  host-specific hardlink `SKIP`.
- No untested deterministic Apply/Persist interaction was established in the
  inspected production path. This does not establish every individual setting
  type's live visual response.

### Overlay/input lifecycle — broad deterministic coverage; graphics limits remain

- `composition_presenter_state_harness` covers minimize/restore, host rebind,
  display activity/focus, DPI and geometry placement invalidation, surface
  generation commit ordering, attachment failures, input capture readiness,
  and stale inactive-host events.
- `runtime_native_adapter_harness` has a real WARP-backed partial ImGui backend
  cleanup scenario. The DComp lifecycle investigation also records an isolated
  actual Renderer/hidden-window fixture covering selected open/close, resize,
  minimize/restore, rebind, and shutdown sequences.
- No concrete omitted deterministic branch was identified in this bounded
  source-to-test comparison. Physical presentation, driver scheduling and
  live-window focus remain environment/runtime evidence; deterministic tests
  cannot claim those results.

### Production startup, hooks, configuration, diagnostics, and release pipeline

- Runtime start, partial initialization, fatal cleanup, DLL/module residency,
  worker stop/join and callback draining are covered through startup
  timeline/journal harnesses, `worker_lifecycle_harness`, module ownership,
  and selected actual-runtime adapter scenarios.
- Native memory readability/writability, viewport-window selection, signature
  token parsing/scanning, and instruction-span/relative-call validation have
  focused platform harnesses. Runtime adapter cases exercise production hook
  transactions and reset paths; these checks do not prove target signatures
  still resolve in a live game build.
- Config reading, migration, duplicate managed keys, template repair, atomic
  multi-setting persistence and failure preservation are exercised by
  `config_persistence_harness`; runtime apply/persist ordering and concurrent
  publication are covered by runtime-settings and semantic-snapshot harnesses.
- Diagnostics gates, aggregation, writer identity, evidence replay,
  performance telemetry and startup journaling have dedicated harnesses or
  actual-adapter cases. No additional deterministic invariant was found
  missing in the bounded source-to-registry comparison.
- `publish_asi.ps1` had no registered contract test. The new
  `publish_asi_contract.ps1` verifies replacement/hash, same-content no-op,
  missing-source preservation, and destination mutex exclusion. It is
  registered in canonical `test.cmd` beside the release ZIP contract test.

## Disposition

The reviewed FOV/camera batch, hooks, settings/persistence, overlay/input,
localization/resources, platform helpers, diagnostics, startup/lifecycle, and
release pipeline now have mapped deterministic coverage in this bounded
inventory. Two release pipeline gaps were closed with registered contract
tests. The focused package contract passed five assertions; the publisher
contract passed four. Canonical `test.cmd -NonGraphics` completed with 62/62
harness invocations passing, zero failures, one explicitly skipped hardlink
subcase, and WARP omitted. Process-tree cleanup was verified.

The input-path safety check intentionally targets the required `.asi` path and
verifies rejection without source damage. The packager's later exact-path
collision guard is not reached by this case because the earlier `.zip`
extension requirement rejects that path first; the test does not count this as
coverage of the collision guard.

This remains a source-to-test inventory rather than proof of exhaustive
runtime correctness. Live game signature resolution, rendered FOV, and
physical graphics presentation still need runtime evidence. Future subsystem
work should follow
`research/plans/REMAINING_SUBSYSTEM_AUTOMATED_COVERAGE_AUDIT_2026-11-15.md` and
add tests only when a concrete uncovered production invariant is established.
