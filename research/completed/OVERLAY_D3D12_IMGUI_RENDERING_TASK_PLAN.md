# Overlay D3D12 ImGui Rendering Task Plan

## Objective

Render one non-interactive Dear ImGui test window on the runtime-validated
STALKER 2 `CreateSwapChainForHwnd` presentation path, with explicit resource,
resize and fail-closed ownership in an experimental ASI only.

## Established evidence/current state

- One persistent swapchain/device/DIRECT queue path is runtime-validated.
- Repeated `ResizeBuffers` and Present revalidation are runtime-validated.
- Production camera/FOV behavior is independent of the overlay.
- No ImGui snapshot currently exists in the repository.

## Approved scope

- Vendor a pinned official Dear ImGui snapshot locally.
- Build a separate `STALKER2CameraTweaksOverlayPOC.asi`.
- Add minimal D3D12 descriptor, frame-context, allocator/list and fence
  ownership needed for one static ImGui window.
- Render only the POC status window; no settings or input frontend.
- Release backbuffer resources before resize and rebuild after revalidation.
- Add deterministic renderer eligibility/ownership state tests and bounded
  telemetry.
- Correct the two known resize telemetry formatting issues.

## Explicit non-goals

- No production `build.cmd` or release artifact changes.
- No Gameplay/Cinematics/Dialogue/config/camera changes.
- No Insert toggle, WndProc interception, input blocking or cursor capture.
- No queue heuristic, new presentation path or universal DX12 lifecycle model.
- No game launch in this batch.

## Expected files/areas

- `external/imgui/` pinned sources and license notice.
- `src/overlay/renderer_state.*` and experimental renderer source.
- `tests/overlay/renderer_state_harness.cpp`.
- `build-overlay-poc.cmd`.
- `research/reports/OVERLAY_D3D12_POC.md`.

## Validation

- `test.cmd` including pure renderer-state tests.
- `build.cmd` unchanged and successful.
- experimental rendering build successful.
- `git diff --check` and static production-dependency review.
- No game launch; provide a separate bounded runtime plan.

## Safe failure and stop conditions

Any dependency, resource, synchronization, identity or rebuild failure must
disable only the overlay. Rendering must remain ineligible for unknown,
ambiguous or resize-revalidation states. Stop before runtime promotion if the
production build receives ImGui/D3D12 rendering sources.

## Final review

Compare changed paths with this plan, update the POC report, record the
runtime gate and move this plan to `research/completed/` after validation.
