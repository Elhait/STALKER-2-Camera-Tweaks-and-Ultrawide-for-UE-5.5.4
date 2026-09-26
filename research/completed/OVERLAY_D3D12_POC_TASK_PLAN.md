# Overlay D3D12 POC Task Plan

## Objective

Begin the post-v1 standalone overlay POC with an optional, fail-closed
subsystem. Establish a deterministic lifecycle boundary and prepare bounded
presentation discovery without changing camera behavior.

## Established evidence/current state

- `research/reports/OVERLAY_FEASIBILITY_AUDIT.md` found no current D3D12/DXGI,
  swapchain, Present or command-queue integration.
- The project has transient Win32 HWND/viewport discovery and SafetyHook-based
  game hooks, but no graphics presentation hook.
- Queue association with the game's presentation path is a runtime unknown.

## Approved scope

1. Add a pure overlay lifecycle model for optional initialization, resize,
   failure, disable and shutdown semantics.
2. Add deterministic tests for that model.
3. Add only bounded discovery-state/reporting scaffolding if it does not make
   graphics or queue assumptions.
4. Produce `research/reports/OVERLAY_D3D12_POC.md` with static status and the
   exact runtime gate.

## Explicit non-goals

- No Gameplay, Cinematics or Dialogue behavior changes.
- No settings UI or shared runtime settings API.
- No ImGui/D3D12 dependency until presentation/queue evidence supports it.
- No arbitrary Present/ExecuteCommandLists hook based on an unverified queue.
- No input blocking, cursor capture or WndProc replacement.
- No game launch and no Git commit.

## Expected files/areas

- `src/overlay/overlay_lifecycle.hpp/.cpp`
- `tests/overlay/overlay_lifecycle_harness.cpp`
- `test.cmd` and `build.cmd` only for the new pure module/harness
- `research/reports/OVERLAY_D3D12_POC.md`

## Batches

### Batch 1 — lifecycle boundary

Define the overlay states and safe transitions. The model must be independent
of camera state and must make failed/disabled rendering a no-op for production.

Validation: overlay harness, `test.cmd`, focused diff review.

### Batch 2 — static/runtime gate report

Record that actual swapchain/device/queue discovery and rendering remain a
runtime gate. Do not add speculative graphics hooks.

Validation: `build.cmd`, `git diff --check`, static review.

## Risks and safe failure

- The main risk is accidentally coupling overlay availability to camera
  initialization. Keep the lifecycle model standalone and do not call it from
  camera paths in this batch.
- A graphics discovery failure must resolve to `Failed` or `Disabled`, never to
  a camera feature failure.
- The change is reversible by removing the isolated overlay module and its
  harness entries.

## Stop conditions

Stop before rendering if queue association remains unproven. Report
`PRESENTATION_DISCOVERY_IMPLEMENTED / QUEUE_ASSOCIATION_RUNTIME_UNKNOWN`.
Do not add settings controls or launch the game.

## Final review

Inspect status, relevant diff and recent history. Report completed, remaining,
deferred and not-runtime-validated items. Move this plan to
`research/completed/` only after the approved batch and validation finish.
