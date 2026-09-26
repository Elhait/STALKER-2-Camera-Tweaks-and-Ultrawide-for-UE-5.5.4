# Camera State Content Layout Task Plan

## Objective

Restructure the expanded Camera State details so all technical content remains readable inside the existing bounded right-hand column without increasing overlay or column width.

## Established evidence and current state

- The viewport-derived main-window width constraint now prevents the overlay from growing on 32:9.
- User runtime screenshot confirms the Camera State header fits, but long technical rows are clipped at the right edge of the fixed runtime column.
- `DrawCameraState` currently renders multiple dense single-line `ImGui::Text` records without wrapping.
- Preserve all existing state fields and their meanings; this is presentation-only.
- The worktree has pre-existing staged/unstaged changes, including the renderer and task log.

## Approved scope

- Organize Camera State into concise labeled sections/fields.
- Use wrapped field rendering so long evidence/provenance/source details fit the current column.
- Keep outer window width, table proportions, data values, and collapsed-by-default behavior unchanged.
- Run `test.cmd`, overlay build, `git diff --check`, and scoped Git review; no game launch.

## Explicit non-goals

- No camera-state semantic, snapshot, sampling, or formatting-source changes.
- No widening the window/column and no conditional Camera State sizing.
- No removal of technical fields or changes to other UI sections.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md`
- This plan, archived to `research/completed/`.

## Implementation batches and validation

1. Add a small wrapped label/value renderer and regroup existing Camera State fields under Presentation, Gameplay, Zoom, Dialogue, FOV, Sources, and Generation.
2. Run deterministic tests, overlay build, and `git diff --check`; review the diff to confirm every prior field remains represented.
3. Archive the plan and record the batch; visual runtime confirmation remains with the user.

## Risks and rollback or safe-failure behavior

- Excessive headings can make the expanded technical block too tall. Keep groups compact and retain the existing collapsed default.
- If a value remains too long, wrapping must preserve the full text rather than clipping or truncating it.
- The change is isolated to rendering and can be rolled back without touching state ownership.

## Stop conditions and phase gates

- Stop if keeping a field readable requires changing snapshot contents or semantics.
- Do not alter the parent width or 45/55 table ratio.
- Do not claim in-game visual PASS.

## Expected final Git review

- Confirm only Camera State presentation, the factual task-log entry, and archived plan changed for this batch; preserve all other staged/unstaged/untracked work.
