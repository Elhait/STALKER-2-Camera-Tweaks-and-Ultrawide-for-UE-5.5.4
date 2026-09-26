# Overlay No-Collapse Task Plan

## Objective

Prevent the main settings overlay window from collapsing to a title bar while preserving Insert visibility toggling and the nested Camera State disclosure.

## Established evidence and current state

- The main ImGui window is opened with `NoSavedSettings | AlwaysAutoResize` and no `NoCollapse` flag.
- User screenshots show the window title-bar collapse triangle; collapsing hides content while the overlay remains visible/active, which can mislead users about mouse input being released.
- Insert controls whole-overlay visibility; Camera State is an independent nested tree disclosure.
- Existing staged/unstaged/untracked changes must be preserved.

## Approved scope

- Add ImGui's standard `NoCollapse` flag to the main settings window only.
- Leave its title bar, window visibility/input behavior, Insert handling, and Camera State tree control unchanged.
- Run deterministic tests, overlay build, `git diff --check`, and scoped Git review. Do not launch the game.

## Explicit non-goals

- No input-capture policy changes or new minimized state.
- No changes to the Camera State disclosure or toast windows.
- No camera, settings, or runtime semantics changes.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md`
- This plan, archived under `research/completed/`.

## Implementation batches and validation

1. Add `ImGuiWindowFlags_NoCollapse` to the main window flags.
2. Run `test.cmd`, `build-overlay-settings.cmd`, and `git diff --check`; review the exact flag location.
3. Archive the plan and record the validation and runtime limitation.

## Risks and rollback or safe-failure behavior

- The title-bar collapse control will no longer be available. Whole-overlay hiding remains through the established Insert path; reverting the single flag restores old behavior.

## Stop conditions and phase gates

- Stop if removing collapse requires changing input capture, Insert handling, or nested tree controls.
- Do not claim visual/runtime PASS without a user-run check.

## Expected final Git review

- Confirm only the main-window flag, task log, and archived plan changed for this batch; preserve prior changes and do not stage or commit.
