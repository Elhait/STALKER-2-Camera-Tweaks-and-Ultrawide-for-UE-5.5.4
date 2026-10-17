# Overlay-Owned HWND — G1-B Offline Implementation Result

**Date:** 2026-10-13  
**Scope:** Unowned display-window feasibility and offline Renderer integration.  
**Runtime/game validation:** Not performed.

## Result

G1-B passed its local unowned-HWND lifecycle fixture, canonical offline test
suite, and isolated production-profile build. The existing G1-A input-admission
and owner-command contracts remain in place. No game was launched; no ASI was
installed or copied to the game directory; no release/reference artifact was
replaced.

## Local lifecycle fixture

Added `research/prototypes/overlay_owned_hwnd/g1b_unowned_lifecycle.cpp` and
`research/tools/run_overlay_owned_hwnd_g1b_lifecycle.ps1`. The fixture creates a
surrogate host on one thread and an unowned top-level display HWND on a separate
owner thread. The DComp target is created only for the display HWND.

Observed PASS results:

- Target HWND differs from host HWND and is created on the display HWND's owner
  thread.
- Transparent, half-alpha and opaque DComp pixels render as expected over the
  host.
- The display HWND is placed above the host without changing foreground or
  focus.
- `WindowFromPoint` passes through to the host HWND.
- Destroying the host does not destroy or acquire ownership of the display HWND.
- The owner thread detaches the visual and destroys its own HWND; `WM_NCDESTROY`
  is observed and the HWND no longer exists.
- Fixture build exit code: `0`; fixture exit code: `0`.

The fixture does not inject physical mouse input. A `SetCursorPos` attempt was
rejected by the current execution environment with Win32 error 5, so the
integrated real-input path is not claimed by this run. The separate G0 fixture
previously established real cross-thread input behavior for its tested window
style; G1-B's permanent-transparent window plus production Game-WndProc bridge
still needs live validation.

## Renderer integration

- The existing Overlay owner thread creates one unowned, non-activating,
  mouse-transparent top-level display HWND and owns its destruction.
- Renderer state separates `gameWindow_` (input, language, host activation and
  geometry) from `overlayWindow_` (DirectComposition target and display).
- `CreateTargetForHwnd` is called for `overlayWindow_`; no game-HWND target
  fallback was added.
- Renderer updates display bounds from the game client rectangle, follows the
  host's topmost state, and hides the display window when the host is inactive,
  minimized, unavailable, or the visual is detached. Showing never activates
  the display window.
- Game-window discovery ignores the display HWND to prevent self-discovery.
- Startup notification lifetime begins only after the display surface becomes
  visible; host rebind retains the owner-thread display HWND and rebuilds its
  composition resources.
- The existing owner-thread terminal/finalization path destroys the display
  HWND after Renderer shutdown. Cleanup outcome remains explicitly
  `UNVERIFIED`; this stage does not upgrade the G1-A cleanup guarantee.

The localization/source-contract audit was updated to recognize the explicit
game/display HWND split and passed without relaxing catalog or resource checks.

## Validation

| Check | Result |
| --- | --- |
| G1-B unowned HWND + DComp fixture | PASS, build `0`, run `0` |
| Canonical registry | 60 targets / 60 repository harnesses |
| Canonical runner | 63/63 invocations, exit `0` |
| Production-adapter matrix | 102 scenarios × Diagnostics OFF/ON = 204 fresh processes, PASS |
| N06-M01 Zoom | 4/4 invocations, PASS |
| Localization/resource audit | 18 locales, PASS |
| Production-profile candidate build | exit `0` |
| Changed production-source compiler diagnostics | None observed |
| Vendor compiler diagnostics | Two C4201 warnings in `external/safetyhook/Zydis.h` |
| Game / ASI installation | Not performed |

Production-profile candidate:

`build-artifacts/cm/candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

SHA-256:

`AA5E239BF52307654E1057B5C935A4D4998D69B8D43251D0D7A08ADF449EFFD4`

This is an isolated offline candidate identity, not a released or live-tested
ASI.

## Remaining gates

- No in-game check of NVIDIA Performance HUD, G-SYNC Indicator, frame
  generation, focus/activation, resize, DPI, minimize/restore or fullscreen
  behavior.
- No live verification of the permanent `WS_EX_TRANSPARENT` display window
  together with the production Game-WndProc input bridge.
- No additional proof of complete Renderer resource cleanup; status remains
  `UNVERIFIED`.
- G1-C and later stages were not started or authorized by this result.
