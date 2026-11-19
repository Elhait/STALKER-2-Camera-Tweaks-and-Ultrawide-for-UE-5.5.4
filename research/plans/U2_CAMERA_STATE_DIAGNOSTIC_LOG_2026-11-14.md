# U2 Camera State Diagnostic Log

## Goal

Capture an automatically ordered, low-impact history of camera FOV writer
transactions for the current supported writer paths:

`Before -> Decision -> Planned After -> Actual After / Unavailable`

Use existing Camera State, provenance, policy and runtime diagnostics as the
data source. Keep this phase diagnostic-only: do not change any FOV policy,
mapping, ownership or transition behavior.

## Scope

- Inventory every production FOV mutation site and its validated post-store
  observation capability. Assign stable writer-site identifiers and report
  coverage as observed, post-store verified, or unknown.
- Give each captured writer invocation a transaction ID; retain receiver,
  output object/field identity, lifecycle generation, source site, thread,
  writer context, decision/policy/reason, input and planned output.
- Correlate post-store observation only when the same invocation can be
  identified and execution order is established. Otherwise record `Uncertain`
  or `Unavailable`, never infer successful write from planned output.
- Include pass-through, rejected, and unknown decisions for covered entry
  sites. Mark missing entry/post-store records explicitly.
- Reuse `[Diagnostics] Enabled`; keep disabled behavior and FOV algorithms
  unchanged.
- Queue fixed-size records to a bounded buffer; write them away from the
  camera hook. Report queue overflow and dropped-record counts exactly.
- Measure hook-side overhead and frame-time effect with diagnostics disabled
  and enabled under a representative workload before a game capture.

## Non-goals

- No FOV algorithm, baseline, transition, recovery, or policy changes.
- No claim that writer input is pristine native FOV.
- No claim of complete camera history until all relevant write paths are
  accounted for.
- No render-level hook or new camera authority model.

## Acceptance

1. Every captured transaction has one ID and explicit validity/status for each
   stage: input, decision, planned output, and post-store result.
2. A post-store result is associated only with the same writer invocation;
   identity/order mismatch produces `Uncertain` or `Unavailable`.
3. Pass-through, rejected, and unknown outcomes are represented.
4. Buffer overflow and dropped events have exact monotonic counts; summary
   reports coverage by writer site and lifecycle.
5. With diagnostics disabled, FOV output and hook behavior remain unchanged.
6. With diagnostics enabled, measured overhead is documented; no synchronous
   disk writes occur on camera hook threads.
7. One runtime capture covers Gameplay -> Cinematic -> Dialogue -> Cinematic ->
   Gameplay, with transitions identified from confirmed state evidence or
   marked `Unknown`.

## Validation and evidence

- Focused host harnesses for transaction serialization, queue ordering,
  overflow accounting, cancellation/shutdown, and post-store correlation.
- Production Release build and existing required source/build checks.
- Bounded diagnostics-off/on adapter timing comparison; report limitations and
  do not equate a host harness with in-game frame-time evidence.
- User-run game capture with the resulting single ASI; analyze the produced
  diagnostic log before any FOV behavior change.

## Status

Diagnostic-only implementation and Production Release candidate are ready.
Source/build/harness status is recorded in
`../evidence/u2-camera-state-diagnostic-2026-11-14/assessment.md`.
The candidate is not installed. One runtime capture, actual in-game frame-time
comparison, and global writer-site coverage remain pending.
