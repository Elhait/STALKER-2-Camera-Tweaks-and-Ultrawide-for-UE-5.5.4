# Overlay Semantic Deduplication — Task Plan

## Objective

Remove overlay-specific duplication where the production `CameraStateSnapshot` already owns authoritative camera runtime state, while preserving separate RuntimeSettings and capability/pending-state ownership.

## Established evidence and current state

- `CameraStateSnapshot` is production-maintained and runtime-validated for gameplay, zoom/binocular, dialogue, cinematic, FOV, provenance, and generation evidence.
- `OverlaySemanticSnapshot` currently combines settings, capability facts, pending state, and duplicated camera facts.
- The full technical Camera State panel is useful for inspection but should be collapsed by default.

## Approved scope

- Audit every camera-related field in `OverlaySemanticSnapshot` and project it from the authoritative owner.
- Remove duplicated camera fields only where the central snapshot has equivalent validity/provenance semantics.
- Update semantic projectors and camera integration to consume the central snapshot.
- Preserve configured settings, capabilities, pending transitions, and next-lifecycle configuration as separate facts.
- Make the detailed Camera State section collapsed by default.
- Add deterministic semantic-equivalence coverage.

## Explicit non-goals

- No camera hook, correction, lifecycle, or RuntimeSettings behavior changes.
- No new camera state/store.
- No runtime launch unless static/deterministic validation reveals a required contradiction.

## Expected files or areas

- `src/plugin/runtime_settings.hpp`
- `src/plugin/runtime.cpp`
- `src/overlay/feature_presentation.cpp`
- `src/overlay/camera_integration.cpp`
- `src/overlay/renderer_runtime.cpp`
- related overlay harnesses

## Batches

1. Audit field ownership and establish a mapping.
2. Deduplicate semantic projection and projectors without changing user-facing results.
3. Collapse technical Camera State presentation by default.
4. Extend deterministic equivalence coverage.
5. Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.

## Risks and safe failure

- Risk: a runtime fact is removed before its validity semantics are equivalent. Mitigation: retain the field until deterministic coverage proves equivalence.
- Risk: active lifecycle state is confused with configured-next state. Mitigation: keep configured settings separate and use observed snapshot state only for active labels.
- Safe failure: unavailable snapshot fields remain unavailable; existing status projectors retain `Cannot assess` behavior.

## Stop conditions and phase gates

- Stop if deduplication requires moving configuration/capability facts into `CameraStateSnapshot`.
- Stop if user-facing status mapping changes unexpectedly.
- Stop before runtime launch; runtime is deferred while deterministic semantic equivalence passes.

## Final review

- Compare changed paths with this plan.
- Confirm no camera behavior or RuntimeSettings semantics changed.
- Archive this plan under `research/completed/` after validation passes.
