# Overlay Focus/Flicker Instrumentation - Task Plan

## Objective

Add bounded, behavior-neutral telemetry to correlate overlay visibility,
window focus/activation, the first cursor-shape message after a focus
transition, and DXGI ResizeBuffers begin/end during one user-run Alt+Tab A/B.

## Established evidence and current state

- The user reports visible game-window flicker when switching away from and
  back to the game while the overlay is open.
- The latest overlay log contains six successful ResizeBuffers cycles at
  5120x1440 and visibility transitions, but no focus records or usable
  timestamps to establish causality.
- The latest runtime confirms clean game exit and immediate OS cursor
  restoration; do not regress that behavior.
- OverlayWindowProc currently forwards activation/focus messages to the game
  WndProc and applies overlay cursor policy only while visible.
- The worktree contains unrelated user changes and untracked overlay
  implementation/assets. Preserve them; edit only this plan and the listed
  files.
- docs/assistant/implementation-guidelines.md,
  docs/assistant/testing-guidelines.md, and docs/code-style.md are absent in
  this checkout. docs/ARCHITECTURE.md and docs/SAFETY_INVARIANTS.md were
  inspected where available.

## Approved scope

- Add local timestamps to focus/activation messages, overlay visibility
  changes, the first WM_SETCURSOR after a focus transition, and ResizeBuffers
  begin/end (plus resize-rebuild timing if needed for correlation).
- Keep cursor/focus/rendering behavior unchanged.
- Keep cursor-event logging bounded: no per-frame or continuous WM_SETCURSOR
  spam; record only the first such event after a focus transition.
- Build and run deterministic tests relevant to the overlay, then inspect the
  final diff.
- Do not launch the game. The user will perform the hidden/visible Alt+Tab A/B.

## Explicit non-goals

- No cursor-policy, focus, input-capture, rendering, swapchain, resize, or
  synchronization behavior changes.
- No new hooks or UI controls.
- No camera/gameplay/settings changes.
- No game launch, Git state mutation, release, or unrelated cleanup.

## Expected files/areas

- src/overlay/discovery_runtime.cpp: timestamp helper and targeted WndProc /
  resize event records.
- src/overlay/renderer_runtime.cpp: timestamp resize rebuild timing if needed;
  preserve rendering behavior.
- OVERLAY_FOCUS_FLICKER_INSTRUMENTATION_TASK_PLAN.md: archive after completion
  under research/completed/.
- backlog/TASKLOG.md: factual implementation/validation entry after the batch
  passes.

## Batches and validation

1. Instrumentation implementation
   - Add local wall-clock timestamp formatting for selected events.
   - Emit activation/focus message names, visible state, and only the first
     WM_SETCURSOR after the latest activation/focus transition.
   - Timestamp ResizeBuffers begin/end and resize rebuild timing.
   - Validate by source review and git diff --check.
2. Deterministic/build validation
   - Run test.cmd.
   - Run build-overlay-settings.cmd to produce the overlay integration ASI.
   - Run git diff --check and review status/diff against this plan.
3. Archive and stop
   - Move this plan to research/completed/ after validation.
   - Record changed paths and validation limits in backlog/TASKLOG.md.
   - Stop before runtime; report the user-run A/B steps.

## Risks and rollback/safe-failure behavior

- Extra logging can perturb timing if high-rate; avoid per-frame or repeated
  cursor logging and emit only low-rate state-transition records.
- Timestamp formatting must not alter event dispatch, cursor calls, lock order,
  renderer state, or DXGI callback decisions.
- If instrumentation adds meaningful hot-path cost or changes behavior, revert
  only this batch's targeted source changes while preserving existing user
  work.

## Stop conditions and phase gates

- Stop if focus/cursor lifecycle requires behavior changes rather than added
  evidence; request a separate bounded repair task.
- Stop if validation fails; report the failing command and do not launch game.
- On successful static/build validation, stop and await user-run A/B logs.

## Final review requirements

- Read-only git status, relevant diff/stat, and recent commit inspection.
- Confirm changed paths match this plan and unrelated dirty files remain
  untouched.
- Report completed, remaining, deferred, blocked, and not-runtime-validated
  items separately.
- No commit or other Git state mutation.
