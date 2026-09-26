# Overlay Two-Column Settings/Runtime Task Plan

## Objective

Split the existing settings overlay into a left configuration column and a right runtime/status column, approximately 45/55, while preserving all existing semantic state, camera behavior, tooltip handlers, and notification behavior.

## Established evidence and current state

- The overlay currently renders settings, Runtime, Camera Integration, and expandable Camera State sequentially in one ImGui window.
- Settings controls and runtime presentation already consume the shared semantic snapshot.
- Tooltip hitboxes, tooltip catalogs, and top-center notifications are already validated and are out of scope for this layout batch.
- The user selected the architecture: left = controls; right = Runtime, Camera Integration, and Camera State.

## Approved scope

- Add a two-column ImGui layout in the existing overlay window.
- Keep Gameplay, Cinematics, and Dialogue controls in the left column.
- Keep Runtime, Camera Integration, and Camera State in the right column.
- Use an approximately 45%/55% stretch split with a subtle internal divider.
- Preserve all current labels, statuses, tooltip invocation, setting mutations, and semantic snapshot reads.

## Explicit non-goals

- Do not implement the Hotkeys settings editor in this batch.
- Do not change camera hooks, camera behavior, lifecycle, semantic projection, FOV ownership, or state composition.
- Do not change tooltip text, tooltip hitboxes, toast placement, timing, or status mapping.
- Do not launch the game or claim runtime validation.

## Expected files or areas

- `src/overlay/renderer_runtime.cpp`: wrap existing settings/runtime blocks in a two-column table.
- `backlog/TASKLOG.md`: record the completed implementation and validation.
- `research/completed/OVERLAY_TWO_COLUMN_SETTINGS_RUNTIME_TASK_PLAN.md`: archive this plan after completion.

## Implementation batches

### Batch 1 — Layout-only source change

Use an ImGui table with stretch weights 0.45 and 0.55. Move only the existing UI blocks across columns; do not duplicate or rewrite semantic logic.

### Batch 2 — Static/build validation

Run the deterministic test suite, production build, overlay build, `git diff --check`, and a read-only Git review. Do not run the game.

## Risks and rollback/safe-failure

- ImGui table sizing may interact with `AlwaysAutoResize`; if compilation or static inspection exposes a problem, keep the existing one-column layout and stop rather than altering behavior.
- The change is localized to presentation layout and is safely reversible by removing the table wrapper and column transitions.
- Existing dirty worktree changes must remain untouched outside the approved renderer and documentation paths.

## Stop conditions and phase gates

- Stop if the change requires edits to camera or semantic state code.
- Stop if validation fails or unexpected files are modified.
- Runtime validation is deferred until a later user-authorized UI review.

## Expected final Git review

Confirm only the planned renderer, task-log, and archived plan paths changed for this batch; distinguish completed static/build validation from deferred runtime validation.
