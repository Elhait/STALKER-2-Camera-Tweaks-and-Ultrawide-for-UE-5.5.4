# Overlay-Owned HWND — G1-C Offline Result

**Date:** 2026-10-13  
**Scope:** Input/display readiness, host lifecycle and observable owner cleanup.  
**Game validation:** Not performed.

## Implemented contract

- The Game WndProc now leaves toggle/opening and other input pass-through until
  the requested UI has a successfully drawn and committed composition surface,
  the visual is attached, and the display HWND is visible for the active,
  non-minimized host. The owner activates the bounded input bridge only after
  those conditions hold.
- Closing the UI, focus loss, host move/resize/display/DPI changes, minimization,
  host rebind/destruction and terminal state revoke capture before owner-side
  reconciliation. The owner restores capture only after the display becomes
  eligible again. Delete continues through the native Game WndProc while the
  Overlay is not yet ready or capture has been revoked.
- While input capture is admitted, the owner publishes progress at 250 ms
  intervals. If the last progress becomes older than 750 ms, a Game WndProc
  callback closes terminal admission, deactivates the bridge try-only, and
  passes the current message to the saved native continuation. The owner loop
  uses a timed wait only while an interactive display is admitted.
- Host rebind hides the display before rebuilding the renderer binding. The
  existing owner thread remains the sole owner of the Overlay HWND, ImGui,
  D3D11 and DirectComposition objects.
- `Renderer::Shutdown` closes the Overlay input state and clears requested
  visibility. Finalization records whether Renderer-owned ImGui/D3D/DComp
  references were observed empty, separately from the overall cleanup result.
  Overall cleanup continues to report `UNVERIFIED`; local reference emptiness
  does not prove DWM, driver or external consumer teardown.
- The unowned-HWND fixture explicitly commits root detachment, releases its
  local D3D11/DComp COM references and verifies they are empty before the owner
  thread destroys the display HWND. It verifies HWND destruction and
  `WM_NCDESTROY` on the creator thread.

## Offline validation

| Check | Result |
| --- | --- |
| Unowned HWND + DComp lifecycle fixture | PASS; build exit `0`, run exit `0` |
| Alpha output / z-order / no activation theft | PASS |
| Host destruction / owner-thread HWND cleanup | PASS |
| Local D3D11/DComp references released after detach+commit | PASS |
| Owner heartbeat lease boundaries | PASS in `input_state_harness` |
| Canonical registry | 60/60 harness targets |
| Canonical runner | 63/63 invocations, exit `0` |
| Production-adapter matrix | 102 scenarios × Diagnostics OFF/ON = 204 processes, PASS |
| N06-M01 Zoom | 4/4 invocations, PASS |
| Localization/resource audit | 18 locales, PASS |
| Production-profile build | exit `0` |
| Diagnostic-profile build | exit `0` |

The builds emitted two C4201 warnings in vendored
`external/safetyhook/Zydis.h`; no warnings were emitted from the modified
Overlay/runtime sources.

## Candidate identities

Isolated build outputs only; neither was published, installed or copied to the
game directory.

- Production: `build-artifacts/cm/g1c-candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`  
  SHA-256: `C86019870C8CBC5AE0203901D59B12033949F4A437A941F318C59C5FA462B3CE`
- Diagnostic: `build-artifacts/cm/g1c-candidate/diagnostic-tOFF-dOFF/bin/STALKER2CameraTweaksDiagnostic.asi`  
  SHA-256: `548A68E408482D777A3CDAF2F6D4F54407FFEBD123AE2910D41689B7FD647CEE`

## Evidence limits and next gate

- The G1-B integrated fixture's physical `SetCursorPos` attempt was denied by
  this execution environment (Win32 error 5). The fixture verifies hit-test
  pass-through; the earlier G0 fixture separately verified real input for its
  tested style. The combined production permanent-transparent HWND plus
  Game-WndProc bridge has not been exercised with physical input in the game.
- No live validation of NVIDIA Performance HUD, G-SYNC Indicator, frame
  generation, focus, DPI, minimize/restore, fullscreen or game HWND replacement
  was performed.
- Renderer-owned reference emptiness is now observable at finalization, but
  complete Renderer/driver cleanup remains `UNVERIFIED` pending the dedicated
  terminal-finalizer evidence. This result does not close G1-D or G1-E.
