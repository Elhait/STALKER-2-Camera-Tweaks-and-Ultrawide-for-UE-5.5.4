# POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST

## Objective

Create a research-only ASI build that defers the automatic gameplay aspect
replay after a confirmed cinematic FOV recovery until the same gameplay camera
source is stable for three consecutive writer samples. An optional short
post-stability delay is applied only after the state gate, never directly from
cinematic EXIT.

## Established evidence and current state

- Current game log shows native cinematic recovery reaching FOV `90` before the
  gameplay replay begins.
- The existing replay then visibly runs the aspect/flags sequence
  `2.38889/0x4 -> 2.38889/0x5 -> 1.77778/0x5 -> 1.77778/0x4`.
- The production ASI is stable and must remain unchanged by this research
  build.
- Existing telemetry already records source, FOV, aspect, flags, coordinator,
  dialogue phase and transition markers.

## Approved scope

- Modify only the combined cinematic/gameplay source under the compile-time
  flag `POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST`.
- Arm defer after confirmed native recovery, not merely on EXIT.
- Require coordinator `Gameplay`, cinematic inactive, dialogue inactive, the
  same source pointer, applicable ultrawide aspect/flags and three stable FOV
  samples within the existing recovery tolerance.
- Apply an optional compile-time research delay (default 120 ms) only after
  stability confirmation, then execute one existing gameplay replay.
- Log `DeferArmed`, `StableSample n/3`, `StableStateConfirmed`,
  `DeferredReplayApplied` and `DeferCancelled` with relevant state.
- Build one separate research artifact; do not install or replace the stable
  production ASI.

## Explicit non-goals

- No production behavior change, cinematic ENTER/EXIT transform change,
  dialogue change or FOV clamp.
- No timer-driven write or replay directly from EXIT.
- No new hook, signature, guessed offset or runtime invocation outside the
  existing validated writer boundary.
- No claim that the replay itself is atomic or that the visual jump is fixed
  until a clean in-game test confirms it.

## Files or areas expected to be touched

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- One research build script under `build-artifacts/research`.
- One research ASI under `build-artifacts/research`.
- This plan and a task-log entry after build/review.

## Implementation batches

### Batch 1 — research-only state machine

Add compile-time guarded defer state, invalidation on ENTER/source/dialogue/
FOV/coordinator changes, stability counting and optional post-stability delay.
Keep the existing replay function as the single apply path.

### Batch 2 — build and static review

Build through the established VS 2022 environment with the research define and
inspect the resulting artifact/hash. No game injection is performed in this
batch.

### Batch 3 — user runtime validation

User runs one clean cinematic EXIT with only the research ASI active. Compare
defer markers, recovery timing, replay timing and visual behavior. Production
ASIs remain disabled for this test.

## Validation

- Confirm the normal compile path has no research behavior enabled.
- Confirm research build succeeds and is separate from the stable artifact.
- Read-only Git status/diff review must match this plan.
- Runtime behavior is not considered validated until the user supplies a clean
  game log and visual result.

## Risks and rollback / safe failure

- Any invalidated state cancels the pending replay without writing camera state.
- A new cinematic ENTER cancels all pending defer state.
- If stability never completes, the research build leaves native state alone.
- The stable production ASI is not overwritten; rollback is simply to omit the
  research artifact and use the existing release binary.

## Stop conditions and phase gates

- Stop implementation if the existing replay path cannot be reused unchanged.
- Stop runtime work if the research build changes ENTER, dialogue or normal
  gameplay behavior outside the approved defer state.
- If the jump merely moves later, classify timing as insufficient and do not
  promote the delay to production.
- If the replay becomes a no-op or the jump disappears, preserve evidence but
  require a separate production decision.

## Expected final Git review

Review only the planned source/build/plan/task-log paths, preserve all unrelated
dirty work, and report completed, remaining, deferred, blocked and not-runtime-
validated items.
