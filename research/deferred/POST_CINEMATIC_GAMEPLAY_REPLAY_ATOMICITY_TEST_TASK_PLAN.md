# POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMICITY_TEST

## Objective

Build a research-only variant that uses the existing post-cinematic stable
state gate, then applies the final gameplay aspect/flags state in one writer
invocation instead of replaying the visible intermediate constrain sequence.

## Established evidence and current state

- Native FOV recovery completes before the deferred gameplay replay.
- The 120 ms state-based defer separates the two transitions but does not
  remove the visual jump.
- The jump correlates with the replay sequence
  `2.38889/0x4 -> 2.38889/0x5 -> 1.77778/0x5 -> 1.77778/0x4`.
- FOV remains `90` throughout that replay, so this test targets aspect/flags
  atomicity only.

## Approved scope

- Keep the existing research defer gate: same source, Gameplay coordinator,
  inactive dialogue/cinematic, target FOV tolerance and three stable samples.
- Stop logging stable samples after `3/3` and use an explicit delay-pending
  phase represented by existing research state.
- Under `POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMICITY_TEST`, write only the final
  native aspect and flags (`1.77778 / 0x4`) once at the eligible writer
  invocation, mark the replay complete and log the atomic apply.
- Keep the normal two-pass replay for builds without the atomicity flag.
- Produce a separate research ASI; do not replace or install the production ASI.

## Explicit non-goals

- No changes to cinematic ENTER/EXIT FOV transforms, dialogue handling or
  gameplay FOV.
- No timer-based behavior outside the existing post-stability research delay.
- No new hook, signature, guessed offset, renderer change or frame suppression.
- No claim that a single invocation is visually atomic until runtime confirms
  it.

## Files or areas expected to be touched

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- One research build script and one research ASI under `build-artifacts/research`.
- This plan and a task-log entry after build/review.

## Implementation batches

### Batch 1 — atomic research path

Add the compile-time guarded final-state apply and clean stable-sample state
transition while preserving the existing defer path.

### Batch 2 — build and review

Build with both defer and atomicity defines, inspect the artifact/hash, and run
read-only Git review. No game validation occurs in this batch.

### Batch 3 — user runtime validation

Run one clean cinematic EXIT with only the research ASI active. Compare visual
framing and telemetry against the previous deferred build.

## Validation and stop conditions

- Build succeeds with the research flags.
- Production artifact is not overwritten.
- Runtime acceptance requires `AtomicReplayApplied` followed by no intermediate
  replay markers for that handoff.
- If framing is wrong or the jump remains, do not promote this path; classify
  the replay as non-atomic or semantically required and stop this branch.

## Risks and safe failure

- If final-state writes fail, leave native state unchanged and log refusal.
- If any gate invalidates, cancel pending replay without a camera write.
- The stable production ASI remains the rollback path.

## Expected final Git review

Compare changed paths with this plan, preserve unrelated dirty work, and report
completed, remaining, deferred, blocked and not-runtime-validated items.
