# Overlay D3D12 Factory Coverage Task Plan

## Objective

Repair the bounded experimental DXGI factory interception coverage after the
first runtime log observed factory creation but no swapchain creation, while
keeping rendering disabled and production camera behavior untouched.

## Established evidence/current state

- The discovery ASI loaded and intercepted all three DXGI factory exports.
- The first runtime log recorded 82 factory observations and 55 hook records.
- It recorded zero `SWAPCHAIN_DISCOVERED`, `Present` or `ResizeBuffers` events.
- Queue association was therefore not reached and remains unclassified.
- Current source hooks `IDXGIFactory2` slots 14 and 23, which require static
  verification against the inherited COM slot layout.

## Approved scope

- Verify and correct relevant `IDXGIFactory`/`IDXGIFactory2` method slots.
- Add `CreateSwapChainForCoreWindow` coverage and method-specific telemetry.
- Add identity/method bookkeeping to suppress duplicate factory observations.
- Add deterministic tests for factory identity, method coverage and null/failure
  bookkeeping.
- Update the overlay POC report with the first runtime evidence and validation.

## Explicit non-goals

- No ImGui, rendering, command-list creation or queue selection.
- No camera, gameplay, cinematic, dialogue or production build behavior changes.
- No new graphics ownership model or heuristic first/last queue selection.
- No game launch in this batch and no Git commit.

## Expected files/areas

- `src/overlay/discovery_runtime.cpp`
- `src/overlay/discovery_evidence.hpp/.cpp`
- `tests/overlay/discovery_evidence_harness.cpp`
- `research/reports/OVERLAY_D3D12_POC.md`

## Validation

- `test.cmd` with complete runner.
- `build.cmd`.
- `build-overlay-discovery.cmd`.
- `git diff --check`.
- Read-only status/diff review against this plan.

## Safe failure and stop conditions

Unknown or ambiguous factory/swapchain paths remain observational only. Stop
before rendering and leave queue association unclassified until a later runtime
session records an actual swapchain and Present path.

## Final review

Record completed, remaining, deferred and not-runtime-validated work in the
overlay report, then move this plan to `research/completed/`.
