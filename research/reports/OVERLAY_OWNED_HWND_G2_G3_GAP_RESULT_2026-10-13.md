# Overlay-Owned HWND — G2 Lifecycle / G3 Geometry-Scheduler Gap Result

**Date:** 2026-10-13  
**Scope:** Gap-only implementation after G1-A → G1-E. No broad refactor.  
**Game / ETW / PresentMon:** Not run.

## Summary

G2's owner-thread lifecycle and bounded recovery mechanisms were already in
place from G1-C/D. This slice closes two concrete gaps: partial display-window
creation now unregisters a class owned by this worker during finalization, and
display visibility is published only after the Win32 operation and observed
state agree. Existing DComp target/surface failures already enter the terminal
owner finalizer and do not retry or publish a successful surface generation.

The next G3 slice avoids redundant display-window positioning for unchanged
physical geometry/DPI, explicitly excludes the overlay window class from game
discovery and WinEvent refresh, and tests the placement-cache decision for
negative monitor coordinates and 100/125/150% DPI values.

## G2 — Lifecycle

| Planned failure/contract | Evidence or change | Status |
| --- | --- | --- |
| Stable DComp target across menu/toast visibility changes | Existing Renderer retains the target; toggles change root attachment and window visibility. G1-B/G1-E evidence covers the integrated lifecycle. | Covered; no target destroy/recreate added |
| Startup notification duration starts only after accepted visible submission | `StartPendingNotificationLifetimes` reconciles attachment and requires `overlayWindowVisible_`; existing presenter harness tests submitted/committed visible generations. | Covered by existing evidence |
| `CreateWindowEx` fails after this worker registered its class | Finalizer now unregisters the class even when no HWND was created. Class-registration ownership is tracked; unrelated pre-existing classes are not unregistered. | Fixed |
| `SetWindowPos` show/hide fails or observed visibility disagrees | Visibility state changes only after accepted operation and matching `IsWindowVisible` observation. Failure routes to existing terminal-disable/finalizer path. | Fixed / covered by focused harness |
| DComp device, target, surface or initial commit partially succeeds | Existing initialization/rebind paths fail closed through `Disable`; G1-D finalizer independently drains callbacks and releases owner resources, reporting `Complete`, `Retained` or `Unverified`. No unbounded retry is introduced. | Existing handling retained; real driver fault injection remains unverified |
| Rebind or device recovery fails | Existing bounded recovery state rejects repeated recovery; rebind failure requests terminal owner cleanup. | Covered by existing state/finalizer evidence |
| Owner-thread cleanup and fail-open input | Existing G1-A/C/D admission, callback drain and owner-thread finalizer unchanged. | Covered by prior focused evidence |

## G3 — Geometry / Scheduler gap

- Added a cached physical display placement keyed by x/y, width/height, host
  DPI and topmost policy. Identical geometry events do not call
  `SetWindowPos`; activation, visibility restoration, HWND rebind, or a changed
  key forces placement. Negative coordinates are retained as signed values.
- A DPI transition is applied before placement reconciliation, so the new DPI
  participates in the same update rather than leaving the cache at the old DPI.
- Both candidate discovery and WinEvent refresh now reject the dedicated
  Overlay display class, not only the currently published HWND. This prevents
  create/destroy/location notifications from feeding back into game-window
  discovery during the display window's lifecycle.
- Existing frame scheduling remains event/compositor-clock driven while idle;
  fallback animation timer is armed only for notification animation. No new
  polling loop or retry was added.
- Existing `TryStartConfiguredOverlay(false, ...)` coverage verifies that
  `Overlay.Enabled=false` does not call overlay startup. No display HWND or
  DComp target can therefore be created by that path.

The G3 checks here are offline policy/source checks. Physical movement across
mixed-DPI monitors, negative-origin monitor interaction, minimize/restore,
Alt-Tab and exclusive fullscreen were not re-tested in game and are not claimed
as runtime-verified by this report.

## Validation and artifact

- Focused `composition_presenter_state_harness` build: exit 0.
- Focused harness execution: PASS, exit 0.
- Isolated production `build.cmd`: exit 0; localization/resource preflight
  passed for 18 locales. The first full build emitted existing vendor C4201
  warnings from `external/safetyhook/Zydis.h`; the final incremental rebuild
  compiled the last finalizer-status adjustment and linked successfully with
  no new warning output.
- Candidate ASI, not installed or copied to the game:
  `build-artifacts/g2-g3-candidate/STALKER2CameraTweaks.asi`
- Final SHA-256: `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`
- Size: 3,572,736 bytes.
- Process cleanup check: no `cmake`, `ninja`, `cl` or `link` processes remained.
- Full canonical test suite and game validation were not run.

## Limits

The shared finalizer fixture validates staged/partial resource cleanup and
truthful retained outcomes, but it does not inject failures into the actual
production `CreateWindowEx`, DComp target, or DComp surface APIs. Those source
paths fail closed and use the tested owner finalizer; their driver-specific
failure behavior remains unverified. This work does not close G4 regression or
G5 compatibility gates, and does not alter any prior live result.
