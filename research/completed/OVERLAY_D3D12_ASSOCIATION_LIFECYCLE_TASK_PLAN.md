# Overlay D3D12 Association Lifecycle Task Plan

## Objective

Separate backbuffer/resource invalidation from observed swapchain/device/queue
association after the second runtime discovery session.

## Established evidence/current state

- `CreateSwapChainForHwnd` is observed in STALKER 2.
- The swapchain, D3D12 device and DIRECT queue candidate are valid before
  resize.
- Present and repeated successful `ResizeBuffers` are observed.
- The current store clears the queue candidate at every resize, so the next
  Present is reported ambiguous even though the same swapchain continues.

## Approved scope

- Retain structural association across successful resize of the same
  swapchain/device and revalidate it on the next Present.
- Invalidate association for a new swapchain, changed device or conflicting
  candidate evidence.
- Keep lifecycle `Ready -> Resizing -> Ready` distinct from recreation
  discovery.
- Add deterministic evidence/lifecycle coverage and telemetry fields.
- Update the overlay POC report.

## Explicit non-goals

- No ImGui, RTVs, command allocators/lists, submission or input.
- No camera/settings/production ASI changes.
- No heuristic queue selection or inference from a first/last DIRECT queue.
- No universal DXGI lifetime model and no game launch in this batch.

## Validation

- `test.cmd`.
- `build.cmd`.
- `build-overlay-discovery.cmd`.
- `git diff --check`.
- Read-only status/diff review.

## Safe failure and stop conditions

Failed resize, incomplete resize or identity conflict must leave rendering
ineligible. Stop before rendering and report the association lifecycle as
runtime-unvalidated until the next controlled session.

## Final review

Record the static/harness result and remaining runtime gate in the overlay
report, then move this plan to `research/completed/`.
