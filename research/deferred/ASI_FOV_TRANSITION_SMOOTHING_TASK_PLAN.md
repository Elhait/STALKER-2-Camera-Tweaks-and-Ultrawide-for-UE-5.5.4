# Task Plan — Seamless Cinematic FOV Transitions

## Objective

Reduce the visible FOV transition time at cinematic ENTER and EXIT so the
existing cinematic correction feels seamless, while preserving the current
final FOV/aspect policies and gameplay/dialogue behavior.

## Established evidence and current state

- The production ASI has validated cinematic ENTER and EXIT boundaries.
- ENTER transforms authored cinematic FOV at the native boundary.
- EXIT currently waits for native recovery through the coordinator and does
  not change the target FOV policy.
- The requested change concerns only the visible transition between the
  authored gameplay FOV (for example 90°) and the corrected cinematic FOV
  (typically 120°+), and the reverse transition after the cinematic.

## Approved scope

- Inspect and adjust only cinematic FOV transition timing/state handling in
  `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Preserve the authored gameplay FOV, cinematic aspect policy, dialogue zoom,
  guarded signature resolution and fail-closed behavior.
- Build the unified production ASI and perform source/build/static checks.

## Explicit non-goals

- No new signature families or executable-version claims.
- No aspect-ratio or projection changes.
- No gameplay two-pass changes.
- No dialogue logic changes.
- No UE4SS changes.
- No runtime claim until the user tests the rebuilt ASI in-game.
- No installation, deletion, commit, tag or publication.

## Files or areas expected to be touched

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `backlog/TASKLOG.md`
- `research/reports/ASI_FOV_TRANSITION_SMOOTHING.md`
- generated production build output and its release copy only if requested

## Batches

### Batch 1 — Transition-path audit

Confirm the existing ENTER/EXIT state machine and identify the smallest
transition-time change that does not alter final policy values.

### Batch 2 — Bounded implementation

Apply only the selected cinematic transition timing change. Keep native
pass-through and recovery fallback when the target or state is not valid.

### Batch 3 — Build and static review

Build `STALKER2CameraTweaks.asi`, verify the expected output and review the
source diff against this plan. Do not call the build a runtime validation.

## Validation

- Source review confirms no changes outside cinematic FOV transition handling.
- Build completes successfully with the existing production command.
- Existing resolver/signature code remains intact.
- New ASI hash is recorded.
- User runtime test should compare ENTER and EXIT perceived transition time and
  confirm no gameplay/dialogue regression.

## Risks and rollback / safe failure

- Forcing a target too early could produce a visible snap or fight native
  camera interpolation. Keep the change bounded to the confirmed cinematic
  coordinator state and leave native values untouched outside it.
- If the target FOV is unavailable or invalid, retain native pass-through.
- Rollback is the source diff reversal; no existing artifacts are deleted.

## Stop conditions and phase gates

- Stop if the requested behavior cannot be isolated from final FOV/aspect
  policy without changing unrelated lifecycle logic.
- Stop if the build fails or resolver safety contracts are altered.
- Stop before release packaging until the user provides runtime evidence.

## Expected final Git review

Inspect status, relevant diff/statistics, changed paths and recent commits.
Report completed, remaining, deferred, blocked and not-runtime-validated
items. Do not commit or publish.
