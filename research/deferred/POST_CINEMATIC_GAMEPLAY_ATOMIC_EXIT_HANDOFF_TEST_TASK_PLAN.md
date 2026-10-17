# POST_CINEMATIC_GAMEPLAY_ATOMIC_EXIT_HANDOFF_TEST

## Objective

Test whether applying the final gameplay aspect/flags state on the first
confirmed downward FOV sample after the final cinematic EXIT improves the
visible handoff without waiting for native FOV recovery to complete.

## Established evidence and current state

- The validated 2.0.5 production path transforms authored cinematic FOV 90 to
  approximately 126.87 on ENTER and hands native target FOV 90 back on EXIT.
- Native recovery then supplies a per-frame stream approximately 126.87 -> 90.
- The recovery-complete atomic test removed the later replay oscillation but
  did not visibly change the remaining native FOV movement.
- The current test must distinguish that native recovery from the gameplay
  projection replay and must not alter the stable release artifact.

## Approved scope

- Research-only compile-time branch in the existing combined experimental
  source.
- On the first writer sample satisfying all gates below, apply exactly one
  atomic `aspect=1.777778`, `flags=0x4` state to the active source.
- Preserve existing telemetry and add an explicit `phase=RecoveryStart`
  marker.
- Build one separate 2.0.5 research ASI.

Trigger gates:

- coordinator is `CinematicExiting`;
- dialogue is inactive;
- source is the same source armed at final EXIT;
- previous FOV is approximately the cinematic transformed value;
- current FOV is lower than the previous sample by epsilon;
- current FOV remains above the native target FOV.

## Explicit non-goals

- No production release changes.
- No changes to cinematic ENTER transformation.
- No dialogue changes.
- No timer-based trigger, frame counter or repeated clamp.
- No hard-coded gameplay FOV value.
- No new hooks, static analysis or runtime calls.

## Expected files or areas

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `build-artifacts/research/build-post-cinematic-gameplay-atomic-exit-handoff-test.cmd`
- `build-artifacts/research/STALKER2CameraTweaks-PostCinematicGameplayAtomicExitHandoffTest205.asi`

## Batches

### Batch 1 — implementation

Add isolated state and a one-shot first-downward-sample branch. New ENTER,
source changes, dialogue activation, coordinator changes or recovery completion
must disarm/cancel the pending trigger.

### Batch 2 — build and static checks

Build with the established VS 2022 toolchain, verify the output exists, and
verify that the branch is compile-time disabled in the normal source path.

### Batch 3 — user runtime validation

User runs one final cinematic EXIT with only the research ASI active and
provides the module log plus visual result. Runtime behavior is not considered
validated by the build alone.

## Risks and safe failure

- If the source or FOV samples are unreadable or violate any gate, do nothing
  and retain native behavior.
- If the one-shot write fails, log refusal and retain the current state.
- A new ENTER or dialogue activation cancels the pending handoff.
- The production branch remains unchanged when the research macro is absent.

## Stop conditions and phase gates

- Stop if the implementation requires a guessed pointer, new hook or timer.
- Stop if the trigger cannot distinguish the first real downward recovery sample
  from EXIT/ENTER chatter.
- Stop runtime research if the visual jump is unchanged and no new evidence is
  produced.

## Final review requirements

- Read-only `git status`, relevant diff/diff summary and recent commit review.
- Compare changed paths with this plan.
- Report build evidence separately from runtime validation.
- Keep the research plan in `backlog` until runtime disposition is known.
