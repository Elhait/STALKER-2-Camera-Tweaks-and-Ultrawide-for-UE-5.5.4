# Overlay-owned HWND — G1-A follow-up result

Date: 2026-10-13  
Disposition: **G1-A offline acceptance gates PASS; no live-game claim. G1-B not started.**

This report records the approved follow-up to the earlier partial status in
[`OVERLAY_OWNED_HWND_G1A_IMPLEMENTATION_STATUS_2026-10-13.md`](OVERLAY_OWNED_HWND_G1A_IMPLEMENTATION_STATUS_2026-10-13.md).
The earlier report remains a historical record of the first slice; this
follow-up resolves its listed G1-A blockers within the authorized scope.

## Follow-up delivered

- Game WndProc now performs bounded admission and command/event forwarding;
  settings Apply/Persist/Snapshot, setting-rebind publication, renderer state
  changes and file logging are handled by the existing Overlay owner thread.
- Accepted owner commands enter a bounded multi-producer queue in reservation
  order. Queue-full/contention is explicit: the current native event fails
  open to the game, admission is closed, and accepted-but-unconfirmed commands
  are reported during owner finalization rather than silently overwritten.
- The owner uses an event-aware message pump for terminal shutdown and owner
  work. The terminal event is authoritative and independent of posted-message
  queue capacity; D3D11, DirectComposition, ImGui and cleanup retain one owner.
- Callback admission remains in flight through native WndProc continuation.
  Owner finalization drains admitted callbacks before removing the subclass and
  releasing owner-held renderer resources.
- A focused Win32 fixture exercises real `SetWindowLongPtr` subclassing and
  continuation, foreign subclass replacement, stale generation pass-through,
  `WM_NCDESTROY`, and terminal drain while a native continuation is blocked.
- Native-language handling retains its established contract: the language is
  read on the game WndProc path and applied on the Overlay owner.

## Validation

| Check | Result |
| --- | --- |
| Focused input/Win32 lifecycle harness | PASS, exit 0; queue ordering/full rejection, concurrent producers, subclass chain and callback drain covered |
| Canonical `test.cmd` | PASS, process exit 0; 60 registered targets, 63 ordered invocations |
| Production actual-adapter matrix | PASS, 102 cases × Diagnostics OFF/ON = 204 isolated processes |
| N06-M01 Zoom fixtures | PASS, 4/4 invocations: IN/OUT × Diagnostics OFF/ON |
| Production-profile CMake target | PASS, exit 0; isolated output under `build-artifacts/cm/test-suite` |
| Production candidate SHA-256 | `AAF9EFB2717835B04A14775A91B9CEF1138EA64961D9FC4C984E9560024F7AA4` |
| Game launch / ASI installation | Not performed |

The production build emitted no warnings from the changed project-owned
translation units. The localization/resource preflight passed for all 18
locales. No diagnostic-profile build was part of this follow-up gate.

## Remaining limits

- This establishes the named offline callback/queue/finalization contracts; it
  does not establish behavior under the game's real window-message schedule or
  graphics-driver/device teardown.
- The owner finalizer records cleanup as `UNVERIFIED` rather than claiming
  `Complete` when the fixture/build cannot establish that every native resource
  was released. A retained resource is not reported as successful teardown.
- G1-A does not include a new Overlay-owned HWND or production live test. Do not
  infer NVIDIA/FG/VRR compatibility from these harnesses.
- Do not start G1-B or replace/install an ASI until separately authorized.

