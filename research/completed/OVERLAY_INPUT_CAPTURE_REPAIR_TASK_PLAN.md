# Overlay Input Capture Repair Task Plan

## Objective

Stop the experimental overlay from blocking all game keyboard and mouse input
while it is visible. Capture messages only when Dear ImGui reports that the UI
needs keyboard or mouse capture; keep Insert reserved for the visibility toggle.

## Established evidence/current state

- Insert visibility toggle works at runtime.
- The current WndProc shell unconditionally consumes common keyboard and mouse
  messages whenever the overlay is visible.
- The static POC window has no settings controls and should not block unrelated
  game input.

## Approved scope

- Add separate keyboard/mouse capture predicates to the pure input state.
- Pass Dear ImGui `WantCaptureKeyboard`/`WantCaptureMouse` to that boundary.
- Preserve Insert toggle handling and original WndProc forwarding.
- Update deterministic input tests and overlay report/task log.

## Explicit non-goals

- No raw-input hook, cursor clipping, focus policy or settings UI.
- No camera/gameplay/cinematic/dialogue changes.
- No production input changes.
- No game launch in this repair batch.

## Validation

- Input harness and full `test.cmd`.
- Experimental POC build and `git diff --check`.
- Read-only Git review; runtime retest remains the next user action.

## Safe failure and stop conditions

If ImGui capture state is unavailable, forward messages to the original game
WndProc. Stop if fixing the issue requires raw-input or cursor ownership.

## Final review

Compare changed paths with this plan, update the overlay report/task log, and
move this plan to `research/completed/` after validation.
