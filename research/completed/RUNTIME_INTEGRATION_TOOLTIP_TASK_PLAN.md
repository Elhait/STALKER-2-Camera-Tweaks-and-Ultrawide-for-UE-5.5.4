# Runtime And Integration Tooltip Task Plan

## Objective

Replace programmer-facing boolean names in checkbox tooltips and provide consistent explanations for each Runtime and Camera Integration value, with the entire row as its tooltip hover target.

## Established evidence and current state

- The shared option-list tooltip renderer already provides standardized two-column wrapping and hover behavior.
- Gameplay.Enabled's tooltip currently exposes `true` / `false`; Hotkeys.Enabled does likewise. These are presentation strings only.
- Runtime currently displays Viewport aspect. Camera Integration displays Aspect, an optional Cinematic aspect comparison, and Gameplay/Native/HorPlus FOV rows.
- The displayed facts come from existing `OverlaySemanticSnapshot` and `ProjectCameraIntegration`; do not change their ownership or semantics.
- Existing unrelated staged/unstaged work is present and must be preserved.

## Approved scope

- Change checkbox tooltip labels to Enabled/Disabled.
- Add tooltip catalogs for every displayed Runtime and Camera Integration value using the existing shared renderer.
- Group label and value and extend the group hitbox across the available row width; no help icons.
- If Cinematic aspect comparison is unavailable, keep the row visible as unavailable and explain that value without inferring a ratio.

## Explicit non-goals

- No changes to semantic snapshot, projector logic, camera behavior, settings, or displayed numeric source.
- No new tooltip implementation or UI icons.
- No automatic game launch or runtime validation.

## Expected files or areas

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md` after validation.

## Implementation batches and validation

1. Update boolean wording and add shared-renderer tooltip catalogs plus full-width row targets for Runtime/Camera Integration.
2. Run `test.cmd`, `build-overlay-settings.cmd`, and `git diff --check`; review Git paths against this plan.

## Risks and safe-failure behavior

- Incorrect tooltip wording could overstate source semantics. Describe only current client viewport, retained neutral Gameplay FOV, current writer input, and the same-observation HorPlus result already represented by source.
- Long tooltips may wrap poorly in the narrower runtime column. Preserve the existing standardized wrap/alignment renderer and keep the requested content concise.
- If a field is invalid, show the existing unavailable value and explain that no valid observation exists; do not synthesize numbers.

## Stop conditions and phase gates

- Stop if a requested explanation would require changing semantic ownership or claiming a value not represented by the current snapshot.
- Stop after deterministic/build validation; visual review remains with the next runtime/UI pass.

## Expected final Git review

- Confirm only the planned renderer/tooltips, plan archive, and factual task-log update changed for this batch; preserve all previous work and do not stage or commit.
