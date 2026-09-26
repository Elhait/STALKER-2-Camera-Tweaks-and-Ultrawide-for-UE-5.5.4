# Hotkey Rebind Cancel Recovery Task Plan

## Objective

Prevent the Escape cancellation path from blocking the overlay window thread and explicitly retain the last accepted binding so Escape, Insert-close, focus loss, or teardown abandons only the in-progress candidate.

## Established evidence and current state

- User reports: after attempting a key already assigned elsewhere, pressing Escape froze and crashed the game.
- The supplied overlay discovery log confirms overlay startup, input-hook installation, and visibility toggles, but contains no key/conflict/cancel events and no crash stack; it cannot independently prove the crash cause.
- `InputState::HandleRebindMessage` acquires `rebindMutex_` and, on Escape, calls `CancelRebind`, which attempts to acquire the same non-recursive mutex again. This is a concrete deadlock path consistent with the report.
- A conflicting binding is rejected before the runtime settings are changed; accepted settings remain the authoritative displayed binding. `InputState` already stores the initial `previousBinding_`, but cancellation does not explicitly restore that session value.
- Existing Git state contains extensive staged, unstaged, and untracked user work. Preserve it.

## Approved scope

- Fix mutex-safe cancellation for Escape and shared cancellation paths.
- Make the input capture state explicitly remember the last accepted key for the active action; only an accepted runtime mutation updates it.
- On cancellation, restore the session's previous-binding value from the last accepted key; rejected/conflicting candidates must not update it.
- Add deterministic tests for duplicate candidate → Escape, accepted rebind → later cancellation, Insert-close, and teardown/focus cancellation.

## Explicit non-goals

- Do not change hotkey action semantics, supported key set, persistence format, worker lifecycle, toast copy, camera logic, or overlay layout.
- Do not launch the game or claim the crash is runtime-reproduced.
- Do not add diagnostic logging or a new settings/state store.

## Expected files or areas

- `src/overlay/input_state.hpp` and `.cpp`
- `src/overlay/discovery_runtime.cpp` (commit only accepted capture into session memory)
- `tests/overlay/input_state_harness.cpp`
- `test.cmd` only if an existing harness needs registration (expected unchanged)
- `backlog/TASKLOG.md` and this plan archive after successful validation.

## Implementation batches and validation

1. Add lock-held cancellation helper / accepted-key update API; use it for Escape and existing external cancellation. Add regression coverage. Run the input-state harness and full `test.cmd`.
2. Build the overlay, run `git diff --check`, and review final Git paths against this plan.

## Risks and safe-failure behavior

- Recursive locking can hang the window procedure. Never call a public mutex-locking method from another method while holding the same mutex; use a private lock-held helper.
- A rejected candidate must not replace the accepted key. Update remembered state only after `RuntimeSettingsApi::Apply` accepts the mutation.
- If validation fails, do not launch the game; report the remaining issue without broader changes.

## Stop conditions and phase gates

- Stop if the reported symptom points to a different code path than overlay rebind cancellation, or if honoring rollback would require changing authoritative config ownership.
- Stop after deterministic/build validation. Runtime confirmation requires a later user-run test.

## Expected final Git review

- Confirm the modified paths are limited to the approved rebind state/cancel flow, its tests, task plan archive, and factual task log; preserve all other work and do not stage or commit.
