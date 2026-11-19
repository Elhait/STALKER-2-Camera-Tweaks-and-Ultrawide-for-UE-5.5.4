# CINEMATIC_HARDCODED_NATIVE_STATE_FEASIBILITY

Status: BATCH 1 COMPLETE / RUNTIME TEST PENDING

## Objective

Build a separate experimental ASI to test whether a minimal, hardcoded
cinematic camera-state transition can preserve authored FOV while removing
ultrawide side bars. This is a feasibility probe, not a production change.

## Established evidence and current state

- Native gameplay A/B/C control preserves authored FOV 90 and reaches a
  correct 32:9 gameplay frame after a settings-driven reevaluation.
- The stable production cinematic fix currently uses a proven Full Hor+
  transformation and remains the fallback/control.
- Direct storage writes to camera fields do not prove or reproduce the native
  reevaluation path.
- Current-build cinematic enter/exit boundaries and the camera-writer
  signature have prior runtime/static evidence for the supported executable.

## Approved scope

- Create one separate experimental source/binary for a hardcoded cinematic
  state feasibility test.
- Keep gameplay behavior pass-through or use only the already validated
  gameplay fix contract; do not alter its implementation.
- Observe cinematic enter/exit boundaries and preserve the authored FOV lane.
- Apply only the smallest experimentally justified cinematic state change at
  the validated boundary, with explicit logging and safe rollback.
- Build the experimental binary for user-controlled runtime testing.

## Explicit non-goals

- No edits to the stable production source or release ASI.
- No broad renderer, vtable, projection or settings-origin search.
- No arbitrary virtual-slot invocation, UE4SS dependency or property writes
  from outside the validated cinematic boundary.
- No new FOV multiplier, no durable authored-FOV rewrite and no hardcoded
  display-aspect assumption as the final answer.
- No dialogue, weapon/viewmodel, overlay or input/UI changes.
- No claim of success from compilation or hook installation alone.

## Expected files/areas

- New experimental source under `src/` with an unmistakable feasibility name.
- A bounded build command/output under `build-artifacts/test-asi/`.
- Optional evidence note under `research/evidence/` after runtime testing.
- Stable source, release assets and production ASI intentionally untouched.

## Batches

### Batch 1 — isolated implementation

- Reuse only current-build validated resolver/boundary contracts.
- Add a fail-safe cinematic state probe with logging of before/after values,
  object identity where available, and one-shot/recovery state.
- Preserve the original FOV input and avoid changing gameplay logic.

Validation: source review, compile the experimental ASI, verify expected output
name and that no stable release file is overwritten.

### Batch 2 — user runtime feasibility test

- User runs the binary in a controlled cinematic at the target aspect.
- Compare authored FOV, CameraComponent/PCM state, side bars and exit state.

Validation: user-supplied runtime log and visual result. Build/load evidence
alone is insufficient.

## Risks and safe failure

- Resolver ambiguity, decode mismatch or unsupported executable identity must
  leave the game untouched and disable the experimental path.
- If the boundary is not reached or required state cannot be validated, keep
  native pass-through and log the reason.
- On exit or failed initialization, remove experimental hooks and restore any
  temporary state owned by the probe.
- The experimental binary must never replace the production ASI.

## Stop conditions and phase gates

- Stop Batch 1 if the current executable identity or instruction contract does
  not validate uniquely.
- Stop if the minimal state change requires multiple guessed writes, timers,
  repeated refreshes or broad additional hooks.
- Stop and classify as bounded negative if authored FOV is not preserved or
  exit leaves camera state polluted.
- Only consider a second implementation after a concrete runtime result;
  do not iterate from visual speculation alone.

## Expected final Git review

- Read-only status/diff/path review after each source/build batch.
- Confirm actual changed paths match this plan and stable release assets remain
  untouched.
- Classify results as completed, remaining, deferred, blocked and not
  runtime-validated before finalizing or archiving this plan.
