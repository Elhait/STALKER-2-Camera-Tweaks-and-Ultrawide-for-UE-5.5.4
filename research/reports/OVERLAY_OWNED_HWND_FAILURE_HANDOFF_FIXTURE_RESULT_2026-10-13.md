# Overlay-owned HWND — failure handoff fixture result

Date: 2026-10-13. **Isolated fixture: 10/10 scenarios PASS on two consecutive
full runs.** This validates the fixture protocol, not production integration.

## Implementation and artifact

- Standalone harness: `research/prototypes/overlay_owned_hwnd/failure_handoff_fixture.cpp`.
- Runner: `research/tools/run_overlay_owned_hwnd_failure_fixture.ps1`.
- Plan: `research/plans/OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_2026-10-12.md`.
- MSVC via VS 2022 developer environment 17.14.28; `/std:c++latest /EHsc /W4
  /WX /utf-8 /O1 /MT`. Build exit 0; no compiler warnings.
- OS registry: DisplayVersion 25H2, build 26200.9457 (ProductName field reports
  the legacy string `Windows 10 Pro`).
- Fixture source SHA-256: `3DCECC94EE21281D4DCC372E1F07CB28D26FD0855958F7EEAB1C65F7F7AA0BE4`.
- Tested EXE SHA-256: `B8B210AED70D7B1089CD95629C7CE3030945273EF4B9404069551301C7436422`.
- Source, executable, build responses/log and per-process stdout/stderr/exit results
  are preserved in `research/evidence/overlay-compatibility/owned-hwnd-failure-2026-10-13/`.

The fixture is compiled separately from all mod targets and launches each scenario
in a fresh process. It creates an owner-thread hidden HWND, D3D11 device/context,
DirectComposition device/target bound only to that HWND, and a real kernel event
resource. It asserts resource creator/release thread, release count, owner outcome,
callback count, and bounded completion. G0's ASI and historic experiments are not
inputs to this fixture.

## Results

| Scenario | Verified behavior | Result |
| --- | --- | --- |
| `race` | One callback waits behind a deliberately held renderer mutex; a second injects a WndProc exception. Failure callback returns in under 500 ms without that mutex or joining owner. Owner waits for the admitted callback to leave before releasing resources; no stale mutation occurs. | PASS |
| `startup` | Terminal latches before owner-thread message-route publication and before resource initialization. Owner wakes from the pre-created event and completes with zero resources. | PASS |
| `queue-full` | Fills the real owner thread queue with 10,000 posted messages; next post returns `ERROR_NOT_ENOUGH_QUOTA` (1816). The persistent event still wakes cleanup; six tracked owner resources are released. | PASS |
| `partial-0` | Terminal after initialization with no staged resources. | PASS |
| `partial-1` | Partial initialization with one real resource, then owner cleanup. | PASS |
| `partial-2` | Partial initialization with event and hidden HWND, then owner-thread cleanup. | PASS |
| `cleanup-failure` | Injects a secondary exception while releasing the real event HANDLE. Cleanup continues for the other five resources; that HANDLE remains live and outcome is `Retained` (created 6 / released 5 / retained 1), never `Complete`. | PASS |
| `stopping` | Repeated terminal request while owner finalizer is paused is idempotent; cleanup runs once. | PASS |
| `late` | Callback and repeated terminal request after owner reaches Stopped cannot reopen admission or schedule duplicate cleanup; native continuation runs. | PASS |
| `stale-token` | Old binding generation is rejected and uses the saved-continuation model without mutating Overlay state. | PASS |

Canonical test inventory/builds and game were not run, as authorized. BuildOnly
validation also passed. The scenario matrix ran twice after the final source change:
20 child-process executions, all exit 0. An earlier fixture iteration exposed an
orchestration mistake: its readiness signal was placed after the owner's blocking
wait. That fixture-only defect was corrected before these two reported full runs.

## What this establishes

Within this surrogate protocol, a terminal transition is monotonic, callback
admission closes atomically, in-flight callbacks drain before finalization, a
full/absent message hint does not lose the event wake, and only the owner thread
releases tracked HWND/D3D11/DComp references. The cleanup status reflects a
deliberately retained real HANDLE. These are deterministic local fixture results.

The failure callback performs only atomic admission closure, SetEvent, an optional
non-authoritative PostThreadMessage attempt and native continuation; measured
latency in this local mutex-held scenario was below the fixture's 500 ms ceiling.
That is not a production latency guarantee. The Win32 WndProc still has ordinary
renderer-mutex work outside a failure path, and a hung owner can leave the visible
Overlay HWND intercepting mouse hits even after logical input revocation.

## Limits and next gate

This is not the production `composition_runtime.cpp`/`InputEventBridge` or
`Renderer::Shutdown`; it does not prove that the production callback binding can
adopt the protocol without changing native/game continuation, window subclass
lifetimes or worker shutdown. A real D3D11 device, DComp target and HWND were
released in the fixture, but no ImGui context, production visual tree, hung driver,
forced owner termination, NVIDIA HUD, G-SYNC, FG, resize/DPI, or game was tested.
The simulated secondary exception is a controlled fail point, not a fault-injected
Windows API failure.

Recommendation: accept this failure-path fixture stage as complete and review the
protocol/results before the next authorization. G1 and Renderer integration remain
separate gates. No production source, vendor source, settings, release artifact,
or ASI was changed; no canonical tests/builds or game launch occurred.
