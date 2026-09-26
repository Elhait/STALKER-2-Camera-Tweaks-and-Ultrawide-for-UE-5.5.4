# Overlay Gameplay FOV Values — Task Plan

## Objective

Show three user-facing values in Camera Integration: the Gameplay FOV selected in game settings, the current camera-writer FOV before HorPlus, and the current FOV after HorPlus, by passing authoritative existing values through the read-only overlay snapshot without recomputing them.

## Established evidence and current state

- `camera::CameraFovObservation` carries `inputFov` and `resultFov` for the CameraWriter boundary, with space/provenance/validity.
- `camera::GameplayBaseline` retains `nativeFov` and `horPlusFov`, aspect, source, and observation sequence.
- `plugin::OverlaySemanticSnapshot` currently exports only a boolean `gameplayBaselineUsable`; it does not export numeric FOV samples.
- `CameraEvidenceSnapshot::configuredGameplayFovKnown` is explicitly set to `false`; HorPlus diagnostics also report `configured=UNKNOWN`.
- Source/history review identified a candidate selected-setting field at the live Gameplay writer source object `+0x234`, historically logged as `secondaryFOV` and currently sampled by diagnostics as `cameraFirstPersonFov`.
- The candidate is not retained as a separate production selected-FOV value or exported through `OverlaySemanticSnapshot`; checked-in evidence does not yet prove its ADS/zoom invariance. See `research/reports/OVERLAY_GAMEPLAY_FOV_SOURCE_AUDIT.md`.

## Approved scope

- Locate and confirm the exact authoritative producer for each of the three values.
- Once mapped, extend the read-only semantic snapshot to pass values and validity/provenance/freshness from existing owners, then present them in Camera Integration.
- Keep the three concepts distinct: selected game setting, pre-HorPlus writer input, post-HorPlus writer result.
- Add deterministic coverage for valid, unavailable, and pass-through observations.

## Explicit non-goals

- No FOV transformation/math changes, camera hooks, engine memory reads, lifecycle/persistence changes, or new sampling mechanisms.
- Do not label writer input as the selected game setting without an explicit source contract.
- No game launch by Codex.

## Expected files if source contract is confirmed

- `src/plugin/runtime_settings.hpp`
- `src/plugin/runtime.cpp`
- `src/overlay/renderer_runtime.cpp`
- overlay snapshot/status harness and `test.cmd` registration as needed
- `research/reports/OVERLAY_SEMANTIC_SNAPSHOT.md` only if its accepted contract is extended

## Batches and validation

1. Resolve the existing producer/field mapping for all three values. Stop for user direction if the selected setting field cannot be found.
2. Expose authoritative samples through the semantic snapshot and render labeled values without recalculation; retain validity and lifecycle semantics.
3. Add deterministic tests; run `test.cmd`, `build-overlay-settings.cmd`, `git diff --check`, and scoped source review.

## Risks and safe failure

- Risk: confusing selected setting with dynamic camera-writer input (e.g. ADS/optics). Preserve separate fields and provenance.
- Missing or stale values must render unavailable, not be inferred or recomputed.
- If the source contract is not found, do not change production state or invent an engine read; ask the user to identify the mechanism.

## Stop conditions and phase gates

- Stop before source edits until the selected Gameplay FOV setting producer is verified.
- Runtime visual review occurs only after deterministic/build validation and with user-controlled game launch.

## Final Git review

- Review exact changed paths against this plan, preserve pre-existing dirty/untracked work, inspect recent commits, and report completed/remaining/deferred/blocked/not-runtime-validated separately.
