# Camera State Layout Task Plan

## Objective

Keep the two settings/runtime columns at their intended proportions when Camera State expands, and make the Camera State disclosure visibly appear interactive.

## Established evidence and current state

- The renderer uses a proportional auto-sizing ImGui table with `SizingStretchProp` and no persisted settings.
- `DrawCameraState` emits long technical rows in the runtime column; expanding the node changes the observed column proportions and they remain changed after collapse.
- The disclosure currently uses `TreeNodeEx` with `SpanAvailWidth` only, so it receives a hover fill but no persistent visual button/frame affordance.
- The issue and expected behavior are demonstrated in the user's screenshots.
- The renderer and other project files contain pre-existing staged/unstaged changes; preserve all unrelated edits.

## Approved scope

- Use fixed per-frame table widths derived from the current available content width, preserving the intended 45/55 ratio independent of expanded contents.
- Give Camera State a visible framed disclosure row while preserving its existing tree behavior and default collapsed state.
- Add deterministic validation only if an existing renderer harness can cover the sizing configuration without introducing a UI framework dependency; otherwise perform existing test suite and overlay build.

## Explicit non-goals

- No changes to camera state data, runtime semantics, or expansion content.
- No changes to other controls, tooltips, or persisted window sizing.
- No game launch unless separately requested.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md`
- This plan, archived under `research/completed/`.

## Implementation batches and validation

1. Inspect current table and disclosure rendering; switch to fixed widths recomputed from available space and add a framed tree node.
2. Run `test.cmd`, `build-overlay-settings.cmd`, and `git diff --check`; visually validate only if the user later runs the game.
3. Review Git status and scoped diff against this plan; record factual results.

## Risks and rollback or safe-failure behavior

- Fixed per-frame widths could be slightly clipped at unusually narrow window sizes. Derive widths from available content space; revert to the prior table flags if the layout fails at minimum supported width.
- The frame may be visually stronger than desired. It changes only the disclosure row and can be tuned independently without affecting interaction.

## Stop conditions and phase gates

- Stop if stable sizing requires changing the overall column ratio, window size policy, or persistence behavior beyond this disclosure/table.
- Do not launch the game; source/build checks do not verify visual appearance.

## Expected final Git review

- Confirm changes are limited to the table sizing/disclosure presentation, task log, and archived plan; preserve staged, unstaged, and untracked pre-existing work.
