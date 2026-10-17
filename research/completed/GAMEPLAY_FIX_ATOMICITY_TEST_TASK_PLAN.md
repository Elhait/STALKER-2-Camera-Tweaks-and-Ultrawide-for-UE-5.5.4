# GAMEPLAY_FIX_ATOMICITY_TEST

## Objective

Build a research-only gameplay variant that replaces the existing staged
aspect/flags replay with one final-state write, and compare the visual and
telemetry result without changing cinematic or dialogue behavior.

## Established evidence and current state

- The current gameplay replay uses the staged sequence
  `2.38889/0x4 -> 2.38889/0x5 -> 1.77778/0x5 -> 1.77778/0x4`.
- The successful cinematic RecoveryStart experiment showed that a single
  atomic `1.77778/0x4` apply can remove subsequent replay flicker.
- It is not yet proven that the same simplification is valid for ordinary
  gameplay camera transitions.

## Approved scope

- Add a compile-time research branch around `ReplayManualTransitionOriginal`.
- On the normal eligible gameplay trigger, write only
  `aspect=1.777778`, `flags=0x4` once.
- Preserve gameplay FOV and the existing source/safety checks.
- Log a dedicated atomic gameplay marker.
- Build one separate research ASI.

## Explicit non-goals

- No cinematic ENTER/EXIT or RecoveryStart changes.
- No dialogue changes.
- No production artifact or release packaging changes.
- No new hooks, guessed offsets, timers or frame suppression.
- No claim of visual success before runtime validation.

## Expected files or areas

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `build-artifacts/research/build-gameplay-fix-atomicity-test.cmd`
- `build-artifacts/research/STALKER2CameraTweaks-GameplayFixAtomicityTest205.asi`

## Implementation batches

### Batch 1 — guarded gameplay atomic path

Add the compile-time branch at the normal gameplay replay entry. Keep the
existing staged implementation as the default when the flag is absent.

### Batch 2 — build and review

Build with the research flag, verify the output and hash, and perform a
read-only Git review. Do not alter or install the production ASI.

### Batch 3 — user runtime validation

Test normal gameplay camera activation and transitions, then check FOV,
framing, repeat triggers, ADS/settings behavior and telemetry.

## Validation and stop conditions

- Build must succeed with the atomic gameplay flag.
- Production artifact must remain untouched.
- Runtime success requires correct gameplay framing with preserved user FOV,
  no required intermediate `0x5` state and no regression in ordinary camera
  transitions.
- If atomic gameplay fails visually or semantically, do not promote it.

## Risks and safe failure

- If the final fields are unreadable or unwritable, leave the native state
  unchanged and log refusal.
- The non-atomic implementation remains available by removing the flag.
- The stable release ASI remains the rollback path.

## Expected final Git review

Compare changed paths with this plan, preserve unrelated dirty work, and report
build evidence separately from runtime validation.
