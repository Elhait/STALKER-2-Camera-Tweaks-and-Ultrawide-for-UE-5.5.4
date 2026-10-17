# COMBINED_ATOMIC_CINEMATIC_GAMEPLAY_HANDOFF_CANDIDATE

## Objective

Build a combined candidate containing the two independently successful atomic
paths: gameplay atomic replay and cinematic RecoveryStart atomic handoff.

## Established evidence and current state

- Gameplay atomicity research passed runtime validation and removed the staged
  gameplay replay sequence.
- Cinematic RecoveryStart atomic handoff passed runtime validation and removed
  the later post-EXIT projection flicker.
- The stable release artifact must remain untouched until the combined candidate
  is checked as one build.

## Approved scope

- Compile the existing source with
  `GAMEPLAY_FIX_ATOMICITY_TEST`,
  `POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST`, and
  `POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMIC_EXIT_HANDOFF_TEST`.
- Produce one separate candidate ASI.
- Preserve existing telemetry and fail-closed guards.

## Explicit non-goals

- Do not call the full staged gameplay replay after cinematic EXIT.
- Do not change cinematic ENTER, dialogue or FOV formulas.
- Do not modify or install the production ASI.
- Do not publish, package or change release metadata.

## Expected files or areas

- One research build script under `build-artifacts/research`.
- One separate candidate ASI under `build-artifacts/research`.
- This task plan and a factual task-log entry.

## Implementation batches

### Batch 1 — combined candidate build

Build the existing guarded implementation with both atomic paths enabled.

### Batch 2 — static review

Verify artifact existence/hash and perform a read-only Git review. Runtime
behavior remains unvalidated until the user runs the candidate.

### Batch 3 — user regression validation

Check ordinary gameplay, one cinematic EXIT, FOV handoff, framing, dialogue,
reload and repeated camera transitions.

## Risks and safe failure

- The candidate is separate from production and can be removed/disabled without
  changing the stable release.
- Any failed guard leaves native state unchanged.
- If combined behavior regresses, revert to the previously tested individual
  research artifacts.

## Stop conditions

- Do not promote if either atomic path loses its previously validated behavior.
- Do not add timers, extra writes or a second post-cinematic replay.

## Expected final Git review

Compare changed paths with this plan, preserve unrelated dirty work and report
build evidence separately from runtime validation.
