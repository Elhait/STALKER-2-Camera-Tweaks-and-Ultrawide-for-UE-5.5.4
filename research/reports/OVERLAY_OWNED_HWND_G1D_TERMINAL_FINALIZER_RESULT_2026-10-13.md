# Overlay-Owned HWND — G1-D Terminal Finalizer Result

**Date:** 2026-10-13  
**Scope:** Terminal owner finalization and failure-path integration.  
**Game validation:** Not performed. G1-E remains pending.

## Result

The G1-D implementation and focused offline checks pass. Renderer disable and
failure paths now request terminal shutdown instead of synchronously releasing
graphics resources. The existing Overlay owner remains the sole finalizer for
ImGui, D3D11, DirectComposition and the display HWND.

## Changes

- Added a one-shot `OwnerFinalizer` shared by production and the focused
  harness. It executes every independent cleanup step even if a step throws or
  reports failure, verifies the final state, and distinguishes `Complete`,
  `Retained` and `Unverified`. A second invocation cannot repeat teardown.
- Added `CloseInputAdmission`, the shared production/harness terminal helper.
  It closes callback admission before try-only input-bridge deactivation and
  never waits for the owner.
- `Renderer::Disable` now closes UI state and requests terminal owner handoff;
  it no longer calls `Shutdown` synchronously. The owner installs this request
  callback when creating the Renderer.
- Owner finalization closes input, drains admitted callbacks, then removes
  observers/timers, stops and joins the frame-clock worker, removes the input
  hook, shuts down the Renderer, and destroys the display HWND—in that order.
  Each cleanup stage is guarded and its result contributes to the final status.
- `ResetComposition` now requires successful `SetRoot(nullptr)` and
  `Commit()` before releasing DComp/D3D references. On failure, it retains
  those references; `Renderer::Shutdown` then keeps the display HWND alive as
  well. A failure is not logged as successful teardown.
- Frame-clock handle closure is checked. Failed timer cancellation preserves
  its identity for status reporting. Cleanup logs a `PENDING` marker before
  finalization and reports the observed terminal outcome and individual
  resource steps afterward.
- Added focused tests using the shared production gate, bridge deactivation
  helper and finalizer. They cover a stalled owner, fail-open admission,
  admitted-callback drain, an unowned display HWND surviving host destruction,
  owner-thread HWND/HANDLE release, independent cleanup after an injected
  exception, truthful `Unverified`, and one-shot behavior.

## Validation

| Check | Result |
| --- | --- |
| Focused `input_state_harness` build | exit `0` |
| Focused finalizer/input harness | PASS, exit `0` |
| Production-profile candidate build | exit `0`; changed production TU rebuilt and linked |
| Localization/resource audit invoked by build | 18 locales, PASS |
| Changed Overlay source warnings in final incremental build | None |
| Full `test.cmd` / diagnostic build | Deferred to G1-E |
| Game launch / ASI installation | Not performed |

The initial production candidate build also showed two existing C4201 warnings
in vendored `external/safetyhook/Zydis.h`. A transient C4456 shadow warning in
the changed composition source was corrected; the final incremental compile of
that source emitted no warning. One early focused fixture run exposed that its
window class had been unregistered by a preceding test; the fixture was made
self-contained and the final run passed.

## Candidate identity

The isolated production candidate was not installed, copied to the game, or
published:

- `build-artifacts/cm/g1d-candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`
- SHA-256: `3636784F2C6CEBE14AC0479EB5F5438D8F8652ABED72AF45AA4E1040CE6D287F`

The focused harness executable SHA-256 is
`76AE365541992C81E484459AE87A9BECBDA7870589B28EDE1D0493FA3FDF41D2`.

## Limits / next gate

- These checks exercise the shared finalizer/control helper and real Win32
  window/resource ownership, not the production DComp driver failure path.
- No in-game test, NVIDIA HUD/G-SYNC/FG check, full `test.cmd`, or diagnostic
  profile build was performed in G1-D. Those belong to G1-E or the separately
  approved live-runtime gate.
- A permanently blocked owner or graphics driver can leave finalization at
  `PENDING`; the fail-open input contract preserves gameplay input but cannot
  guarantee that a frozen display HWND disappears.
- A successful empty Renderer-owned-reference check is not proof that DWM or
  the graphics driver released all external consumers.

G1-D focused offline criteria are met. G1-E remains the next stage; this report
does not claim full integration validation or live-runtime correctness.
