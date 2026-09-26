# Overlay Tooltip Presentation — Task Plan

## Objective and evidence

Fix the visible unsupported-curly-apostrophe glyphs and overly wide single-line control tooltips observed in the user screenshots. All five control tooltips have now been captured, including Cinematic FOV Mode; the same presentation issue is confirmed there.

## Approved scope

- Update user-facing overlay tooltip/status punctuation to ASCII-safe apostrophes.
- Replace single-line tooltip rendering with a bounded-width wrapped tooltip using existing ImGui input/hover behavior, including the Camera Integration explanation tooltip found during source review.
- Keep all tooltip wording and settings semantics unchanged.

## Non-goals

- No camera, hook, runtime settings, semantic status, or lifecycle changes.
- No game launch by Codex; hand off a brief post-build runtime presentation review.
- No changes to overlay layout outside tooltip rendering.

## Expected files and validation

- `src/overlay/renderer_runtime.cpp`, `src/overlay/feature_presentation.cpp`; append the factual completion entry to `backlog/TASKLOG.md` as required by repository rules.
- Build `build-overlay-settings.cmd`; run `git diff --check`; inspect the scoped diff.
- Archive this plan to `research/completed/` after validation.

## Risk, rollback, stop

- Risk: tooltip hover semantics or placement could change. Preserve the same last-item hover behavior and use ImGui tooltip APIs only.
- If build or diff validation fails, revert only this batch. Do not launch the game.
- Stop after build and hand off a brief runtime check of bounded tooltip wrapping; all six tooltips share one helper.
