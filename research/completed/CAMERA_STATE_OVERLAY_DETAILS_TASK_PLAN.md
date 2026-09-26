# Camera State Overlay Details — Task Plan

## Objective

Add a separate bottom `Camera State` section to the existing overlay that presents the complete shared `CameraStateSnapshot` through a read-only semantic projection.

## Established evidence and current state

- `CameraStateSnapshot` is already the shared production runtime model.
- The overlay already receives Gameplay, Native, and HorPlus FOV values from that model.
- No parallel camera state/store is allowed.

## Approved scope

- Export the remaining snapshot fields through the existing overlay semantic snapshot path.
- Render them in a separate bottom `Camera State` section.
- Show unavailable values honestly.
- Add deterministic projection coverage.

## Non-goals

- No camera hooks, camera correction, lifecycle, or state ownership changes.
- No diagnostic-only alternate state path.
- No runtime game launch in this batch.

## Expected areas

- `src/plugin/runtime_settings.hpp`
- `src/plugin/runtime.cpp`
- `src/overlay/renderer_runtime.cpp`
- relevant overlay/camera-state tests

## Batches and validation

1. Extend the existing semantic read-only projection with complete snapshot fields.
2. Render the bottom `Camera State` section.
3. Add deterministic valid/invalid projection coverage.
4. Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.

## Risks and safe failure

- Risk: UI accidentally invents or recomputes state. Mitigation: copy fields directly from the shared snapshot and display unavailable for invalid data.
- Risk: snapshot layout changes. Mitigation: use existing typed fields and central name functions.
- Failure behavior: the details section reports `Unavailable`; existing settings UI remains operational.

## Stop conditions and phase gates

- Stop if a new state/store or camera behavior change appears necessary.
- Stop before runtime launch; runtime validation is a separate follow-up.

## Final review

- Confirm only the approved projection/UI/test areas changed.
- Confirm no camera behavior or diagnostic ownership changes.
- Archive this plan under `research/completed/` after static validation passes.
