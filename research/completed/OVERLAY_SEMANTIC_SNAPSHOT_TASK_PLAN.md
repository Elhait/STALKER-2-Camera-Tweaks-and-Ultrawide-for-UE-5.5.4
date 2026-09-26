# Overlay Semantic Snapshot and Integration Status — Task Plan

## Objective

Add a production read-only semantic snapshot, a deterministic UI-independent
Gameplay/Cinematics integration projector, and contextual Gameplay,
Cinematics, Dialogue, and Camera Integration statuses in the overlay.

## Established evidence and current state

- `RuntimeSettingsSnapshot` already exposes the five configured settings.
- The overlay reads that snapshot and the current Win32 client viewport aspect.
- Runtime owns separate feature capability gates, transition-pending flags,
  coordinator/lifecycle values, FOV observations, GameplayBaseline, and
  restoration state; these are not currently exported as one UI model.
- `CameraStateSnapshot` is diagnostic-only and must not be connected to the
  production overlay.
- Camera writer observations preserve FOV space/provenance but are latest
  per-boundary evidence, not inherently a guaranteed current-camera value.
- `GameplayHorPlus` can fall back to authored cinematic baseline when the
  retained GameplayBaseline is unavailable. `NativeHorPlus` is semantically
  independent even if numeric FOV values happen to match.
- The working tree already contains extensive unrelated modified/untracked
  work, including the overlay implementation. Preserve it and touch only the
  paths listed below.
- `docs/assistant/implementation-guidelines.md`,
  `docs/assistant/testing-guidelines.md`, `docs/code-style.md`, and
  `docs/assistant/documentation-guidelines.md` are absent. Available
  architecture and safety invariants have been reviewed.

## Approved scope

- Extend the read-only overlay-facing runtime snapshot with separate configured,
  capability/application, and observed data; retain per-field validity and
  provenance and do not claim cross-store atomicity.
- Add a pure Camera Integration projector using resolved numeric aspects,
  explicit assessment states, and semantic FOV-path classification.
- Render only contextual statuses in the Gameplay, Cinematics, and Dialogue
  sections plus a Camera Integration section.
- Add deterministic projector/snapshot tests and register them in `test.cmd`.
- Validate with `test.cmd`, production build, overlay settings build, and
  `git diff --check`; review only scoped paths and preserve unrelated dirty work.

## Explicit non-goals

- No camera/gameplay/cinematic/dialogue behavior changes and no new hooks.
- No new HorPlus, cinematic, or dialogue calculations in the UI.
- No current-FOV or full Camera Runtime panel in this batch.
- Do not consume diagnostic-only `CameraStateSnapshot` in production UI.
- No game launch, runtime test, Git state mutation, release, or unrelated cleanup.

## Expected files or areas

- `src/plugin/runtime_settings.hpp`
- `src/plugin/runtime.hpp`
- `src/plugin/runtime.cpp`
- `src/platform/win32/viewport.hpp` and `.cpp` for strict client-viewport evidence
- `src/overlay/camera_integration.hpp` and `.cpp` (or the smallest equivalent
  production module)
- `src/overlay/renderer_runtime.cpp`
- `tests/overlay/` semantic snapshot and integration projector harnesses
- `test.cmd`
- `backlog/TASKLOG.md`
- `research/reports/OVERLAY_SEMANTIC_SNAPSHOT.md`
- This plan, archived under `research/completed/` after validation.

## Batches and validation

1. Define typed snapshot fields and project existing runtime facts only.
   Review synchronization, field validity, and capability/application
   distinction; do not add camera decisions.
2. Implement and test the pure integration projector for matching/mismatching
   aspects, Native/unavailable aspect, baseline fallback, independent FOV path,
   equal-number NativeHorPlus, and invalid numeric inputs.
3. Connect contextual statuses to the four overlay sections. Do not add the
   runtime FOV monitor or display unproven values as current.
4. Run focused harnesses, `test.cmd`, `build.cmd`,
   `build-overlay-settings.cmd`, and `git diff --check`.
5. Write the source/evidence report, archive this plan, and perform the
   read-only Git review.

## Risks and rollback / safe-failure behavior

- Overlay rendering and camera callbacks may observe state on different
  execution paths. Read only synchronized/atomic facts; keep each field's
  validity/provenance independent and avoid claiming an atomic camera snapshot.
- Missing or invalid evidence must yield `CannotAssess`/`Waiting`, never a
  guessed match or green status.
- Configuration settings are not proof of application. Keep capability,
  pending/application, and observation fields distinct.
- Preserve camera production decisions and the existing settings mutation path.
- If exact application semantics cannot be derived from existing facts without
  new camera logic, expose an explicit unknown/pending state instead.

## Stop conditions and phase gates

- Stop if the task requires a new hook, camera algorithm, or behavioral change.
- Stop and report if coherent/safe access to a required field cannot be
  established without expanding the approved scope.
- Stop on failing tests/build; do not launch the game or broaden the batch.
- After report, validation, and final review, stop before runtime testing.

## Final review requirements

- Read-only Git status, recent commit, scoped diff/path review, and
  `git diff --check`.
- Identify unrelated pre-existing dirty paths as preserved, not as part of this
  batch.
- Report completed, remaining, deferred, blocked, and not-runtime-validated
  items separately. No commit or other Git state mutation.
