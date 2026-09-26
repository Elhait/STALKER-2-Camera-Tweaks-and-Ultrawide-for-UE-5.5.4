# Camera Transition Overlay Status Task Plan

## Objective

Promote the overall camera-path compatibility verdict into a clear top-level `Camera Transition` section, explain its user-facing meaning, and keep `Camera Integration` focused on technical component facts.

## Established evidence and current state

- The overlay currently places `Configuration aligned` / `Not aligned` / `Cannot assess` at the bottom of `Camera Integration`; its tooltip is attached to that verdict.
- `ProjectCameraIntegration` resolves `Auto` to the current valid viewport, forced policies to numeric ratios, `Native` to CannotAssess, and compares numeric aspects with the existing tolerance.
- Existing deterministic coverage confirms Auto matches a 32:9 viewport and arbitrary 3:1 viewport, forced 32:9 matches 32:9, forced 21:9 mismatches 32:9, Native is CannotAssess, and invalid viewport evidence fails closed.
- The supplied screenshot shows forced 16:9 against a 32:9 viewport, so its `Mismatch` is expected; it does not demonstrate an always-mismatch defect.
- The workspace has extensive prior staged/unstaged/untracked work. Preserve it.

## Approved scope

- Add a top-level `Camera Transition` summary before the Gameplay controls.
- Display one status and a concise explanation for Aligned, Not aligned, or Cannot assess.
- Add a visible help affordance whose tooltip explains the evaluated criteria and each status.
- Remove the duplicate overall verdict from `Camera Integration`; keep its aspect/FOV details.
- Preserve the existing projector and classification semantics unless user runtime evidence after switching forced 16:9 to Auto contradicts the deterministic source behavior.

## Explicit non-goals

- No camera runtime logic, settings mutation, FOV/aspect projector changes, or overlay state ownership changes.
- No claim that the status proves every visual aspect of transitions.
- No game launch or runtime visual test by the agent.

## Expected files or areas

- `src/overlay/renderer_runtime.cpp`
- Existing projector/harness read-only review: `src/overlay/camera_integration.cpp`, `tests/overlay/camera_integration_harness.cpp`
- `backlog/TASKLOG.md` after validation.

## Implementation batches and validation

1. Move and rewrite the user-facing overall verdict as the top-level Camera Transition summary; retain technical integration facts on the right.
2. Run `test.cmd`, `build-overlay-settings.cmd`, `git diff --check`, and a read-only Git review.
3. Ask the user to switch 16:9 to Auto in-game for runtime confirmation. If it remains Mismatch, reopen the projector batch with that concrete evidence and add the requested matrix cases before changing logic.

## Risks and safe-failure behavior

- Avoid overclaiming that matching known criteria guarantees a fully transition-free visual result; describe only the aspects/FOV paths the projector actually evaluates.
- If presentation cannot represent Cannot assess distinctly, retain the current safe status rather than inferring alignment.
- Do not change numeric projector behavior based on the screenshot because its forced 16:9 setting already explains the mismatch.

## Stop conditions and phase gates

- Stop before projector edits unless the user's Auto runtime check contradicts current deterministic/source evidence.
- Stop after deterministic/build validation; do not launch the game.

## Expected final Git review

- Confirm changes are limited to the status presentation, task record, and archived plan; preserve all pre-existing work and do not stage or commit.
