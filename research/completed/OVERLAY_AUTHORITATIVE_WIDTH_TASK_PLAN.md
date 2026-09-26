# Overlay Authoritative Width Task Plan

## Objective

Stop the settings overlay from growing toward the full ultrawide viewport when its contents/table columns expand, while retaining responsive sizing and the 45/55 two-column layout.

## Established evidence and current state

- The main overlay window is opened with `ImGuiWindowFlags_AlwaysAutoResize` and has no width constraint.
- The two table column widths are derived from `GetContentRegionAvail()` each frame.
- User runtime screenshots show the window growing dramatically across the 32:9 viewport after the prior table-width change; this contradicts the assumption that a fixed per-frame table width alone prevents parent feedback.
- Long technical Camera State rows are content, not an authority for the outer window width.
- Existing staged/unstaged source work must be preserved.

## Approved scope

- Derive a bounded preferred overlay width from the active ImGui viewport width, with safe horizontal margins and practical min/max limits.
- Apply that width as an explicit window size constraint before `Begin`, while retaining automatic height sizing.
- Keep the two-column table contained within the resulting content area; long Camera State content must not enlarge the parent beyond its constraint.
- Run deterministic tests, overlay build, `git diff --check`, and a scoped Git review. Do not launch the game.

## Explicit non-goals

- No special-case sizing based on Camera State open/closed.
- No camera/settings semantics, tooltip, content, or input behavior changes.
- No changing overlay visibility/position behavior beyond constraining its width.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md`
- This plan, archived to `research/completed/`.

## Implementation batches and validation

1. Apply a viewport-derived width constraint to the auto-resizing main window and preserve table proportions within that bounded width.
2. Run `test.cmd`, `build-overlay-settings.cmd`, and `git diff --check`; verify window constraints and table widths against this plan.
3. Archive the plan and write a factual task-log entry after read-only Git review.

## Risks and rollback or safe-failure behavior

- An unsuitable width range could make controls cramped on small viewports or waste space on wide ones. Compute the preferred width from viewport width, cap it, and leave horizontal margin; retain a minimum only when it fits.
- ImGui constraints may clip height if configured incorrectly. Constrain only the X dimension and leave Y unconstrained for existing auto-height behavior.
- If static evidence cannot show that the X constraint is applied before auto-size resolution, stop and inspect the vendored ImGui implementation before changing unrelated layout behavior.

## Stop conditions and phase gates

- Stop if the proposed constraint changes window position/visibility policy or requires new persistent UI state.
- Do not claim runtime visual PASS; user must repeat the expand/collapse observation after installing the build.

## Expected final Git review

- Confirm the batch touches only main-window width constraint, table sizing if needed, task log, and archived plan. Preserve all pre-existing unrelated changes and do not stage or commit.
