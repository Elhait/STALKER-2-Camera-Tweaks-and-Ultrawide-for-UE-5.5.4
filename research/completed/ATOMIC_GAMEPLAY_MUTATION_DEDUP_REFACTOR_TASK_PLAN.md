# ATOMIC_GAMEPLAY_MUTATION_DEDUP_REFACTOR

## Objective

Refactor the combined research candidate so normal gameplay and post-cinematic
RecoveryStart triggers share one atomic gameplay aspect/flags mutation function
without changing their independent gates or runtime behavior.

## Established evidence and current state

- Combined atomic gameplay/cinematic candidate passed runtime validation.
- Both paths perform the same final mutation: `aspect=1.777778`, `flags=0x4`.
- Gameplay and cinematic trigger conditions are intentionally different and
  must remain separate.
- The old staged state (`AppliedConstrainPass`) must not be removed unless
  reference and reachability checks prove it is dead code.

## Approved scope

- Add shared `ApplyGameplayAspectFixAtomic(source, phase, fov, previousAspect,
  previousFlags)` mutation helper.
- Replace only the duplicated atomic writes in the gameplay and cinematic
  research branches.
- Preserve telemetry meaning with an explicit phase/reason field.
- Build the combined candidate and perform static/logical checks.

## Explicit non-goals

- No changes to trigger/state-machine gates.
- No changes to cinematic ENTER, dialogue or FOV formulas.
- No removal of `AppliedConstrainPass` in this batch.
- No production artifact or release changes.
- No new hooks, timers, writes or runtime experiments beyond the existing
  atomic writes.

## Expected files or areas

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `build-artifacts/research/build-combined-atomic-cinematic-gameplay-handoff-candidate.cmd`
- `backlog/TASKLOG.md`

## Implementation batches

### Batch 1 — mutation deduplication

Introduce the shared helper and route both existing atomic branches through it.
Verify `AppliedConstrainPass` references remain unchanged.

### Batch 2 — build and static review

Build the combined candidate, inspect the artifact/hash, search for remaining
duplicated atomic mutation blocks, and perform read-only Git review.

### Batch 3 — user regression validation

Confirm one gameplay atomic apply, one cinematic RecoveryStart atomic apply,
and no `0x5` replay markers in the combined runtime log.

## Risks and safe failure

- If the helper cannot preserve existing state updates or logging, stop and
  retain the prior candidate.
- Trigger gates remain unchanged, so invalid states continue to pass through
  native behavior.
- The previous combined artifact remains the rollback/control candidate.

## Stop conditions

- Stop if refactoring changes trigger ordering or requires new conditions.
- Do not remove legacy staged state in this batch.
- Do not promote to production based on build success alone.

## Expected final Git review

Compare changed paths with this plan, preserve unrelated dirty work, and report
refactor/build evidence separately from runtime validation.
