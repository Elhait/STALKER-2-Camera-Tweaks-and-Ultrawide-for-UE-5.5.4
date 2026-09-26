# Overlay Focus Debug Cleanup — Task Plan

## Objective

Remove temporary focus/Alt+Tab instrumentation after the same window flicker
was reproduced with the overlay hidden, visible, and with the mod absent.
Record the negative-control result without changing overlay behavior.

## Established evidence and current state

- Runtime logs recorded focus transitions with the overlay hidden and visible.
- The user visually confirmed flicker in both conditions.
- The user then tested with the mod absent and observed the same flicker.
- Therefore the tested flicker does not require this mod or overlay visibility.
- Existing resize/resource release and rebuild lifecycle must remain intact.
- The working tree already contains many unrelated modified and untracked files;
  preserve them and limit edits to the listed paths.
- `docs/assistant/implementation-guidelines.md`,
  `docs/assistant/testing-guidelines.md`, `docs/code-style.md`, and
  `docs/assistant/documentation-guidelines.md` are absent. The available
  `docs/ARCHITECTURE.md` and `docs/SAFETY_INVARIANTS.md` were reviewed.

## Approved scope

- Remove focus-message classification, focus-transition logging, the
  first-`WM_SETCURSOR`-after-focus diagnostic flag and its log records.
- Remove the timestamp formatting helper introduced for correlating focus and
  resize events; retain ordinary resize lifecycle records and existing resize
  timing/recovery diagnostics without focus-specific timestamps.
- Keep window procedure dispatch, cursor ownership, input capture, visibility,
  swapchain lifecycle, and resource release/rebuild behavior unchanged.
- Add a short factual report recording the hidden/visible/no-mod result.
- Run deterministic tests, the overlay settings build, whitespace validation,
  and read-only Git review. Do not launch the game.

## Explicit non-goals

- No cursor, focus, input, rendering, resize, synchronization, camera, or
  configuration behavior changes.
- No removal of functional overlay or resize diagnostics.
- No new hooks, tests, runtime sessions, Git mutations, release, or unrelated
  cleanup.

## Files or areas expected to be touched

- `src/overlay/discovery_runtime.cpp`
- `research/reports/OVERLAY_FOCUS_FLICKER_RESULT.md`
- `backlog/TASKLOG.md`
- This plan, archived under `research/completed/` after validation.

## Batches

1. Remove only focus-specific diagnostic branches and timestamp decoration;
   preserve normal message forwarding and resize/resource lifecycle.
2. Record the bounded runtime conclusion and validate with `test.cmd`,
   `build-overlay-settings.cmd`, and `git diff --check`.
3. Compare final changed paths with this plan, archive this plan, update the
   task log, and stop.

## Risks and rollback / safe-failure behavior

- The focus logs share a source file with production overlay input/window-proc
  behavior. Remove only diagnostic conditionals and string formatting; preserve
  all message handling, cursor calls, and visibility state changes.
- Keep resize release/rebuild and existing timing/association diagnostics so
  the overlay's established resource lifecycle remains observable.
- If a diagnostic symbol is shared with functional behavior, retain the
  functional code and remove only its log use.

## Stop conditions and phase gates

- Stop if removing telemetry requires changing focus/cursor behavior or resize
  synchronization.
- Stop on validation failure; do not launch the game or broaden the cleanup.
- No new runtime test is needed for the already completed negative control.

## Final review requirements

- Read-only status, affected-path review, recent commit inspection, and
  `git diff --check`.
- Confirm unrelated pre-existing dirty paths are untouched.
- Report completed, remaining, deferred, blocked, and not-runtime-validated
  items separately. No commit or other Git state mutation.
