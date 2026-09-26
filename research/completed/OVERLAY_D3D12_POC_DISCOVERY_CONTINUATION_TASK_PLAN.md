# Overlay D3D12 Discovery Continuation Task Plan

## Objective

Extend the isolated post-v1 overlay POC with bounded runtime-only
presentation discovery telemetry and deterministic evidence handling, without
enabling rendering or changing production camera behavior.

## Established evidence and current state

- Lifecycle-only overlay scaffolding and its harness already pass.
- Static audit found no existing production D3D12/DXGI/ImGui path.
- The presentation swapchain and command-queue relationship remain runtime
  unknowns.
- The experimental discovery ASI is separate from the production build.

## Approved scope

- Observe DXGI factory, swapchain creation, Present, ResizeBuffers,
  `GetDevice`, command-queue candidates and queue type.
- Correlate candidate identity with Present observations through a bounded,
  deterministic evidence store.
- Add deterministic tests for windows, duplicates, ambiguity, invalidation and
  fail-closed behavior.
- Keep discovery in `STALKER2CameraTweaksOverlayDiscovery.asi` and update the
  overlay POC report.

## Explicit non-goals

- No ImGui, command-list creation, queue submission or rendering.
- No production `build.cmd` source-list or camera/settings behavior changes.
- No input interception, WndProc hook, cursor handling or new graphics owner.
- No game launch, Git commit or release packaging.

## Expected files/areas

- `src/overlay/discovery_runtime.cpp`
- `src/overlay/discovery_evidence.hpp/.cpp`
- `tests/overlay/discovery_evidence_harness.cpp`
- `test.cmd`, `build-overlay-discovery.cmd`
- `research/reports/OVERLAY_D3D12_POC.md`

## Validation

- Overlay evidence harness and complete `test.cmd`.
- Production `build.cmd` unchanged and successful.
- Experimental discovery build successful.
- Focused `git diff --check`.
- Read-only Git status/diff review against this plan.

## Risks and safe failure

- Discovery remains observational and bounded; it must never select a queue for
  rendering.
- Null, duplicate or multiple queue candidates remain unsupported/ambiguous.
- Any unknown or failed runtime relationship leaves rendering disabled.

## Stop conditions

Stop after discovery instrumentation and validation. Do not add rendering or
claim runtime queue association until a controlled game session produces the
required evidence.

## Final review

Record completed, remaining, deferred and not-runtime-validated items in the
POC report, then move this plan to `research/completed/`.
