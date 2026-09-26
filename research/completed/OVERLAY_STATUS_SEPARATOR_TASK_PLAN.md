# Overlay Status Separator — Task Plan

## Objective and scope

Replace unsupported Unicode em-dash separators in user-facing overlay status labels with ASCII hyphens. This is a presentation-only source change; preserve the existing semantic state mapping and all unrelated dirty work.

## Non-goals

- No status logic, camera behavior, hooks, semantic snapshot, tooltip content, or runtime changes.
- No full game/runtime rerun.

## Expected changes and validation

- `src/overlay/renderer_runtime.cpp` and `src/overlay/feature_presentation.cpp` only.
- Build `build-overlay-settings.cmd` and run `git diff --check`; inspect the exact diff.
- Stop after validation; archive this plan under `research/completed/`.

## Risk and rollback

- Risk is limited to displayed punctuation. Revert only the changed separator literals if validation fails.
- Report runtime as not rerun; prior compact-status runtime PASS remains the evidence for behavior.
