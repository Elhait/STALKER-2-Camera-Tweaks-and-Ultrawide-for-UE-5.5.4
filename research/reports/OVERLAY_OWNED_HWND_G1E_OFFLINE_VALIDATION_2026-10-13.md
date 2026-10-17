# Overlay-Owned HWND — G1-E Offline Validation

**Date:** 2026-10-13  
**Scope:** Integrated offline validation after G1-A through G1-D.  
**Game launch / ASI installation:** Not performed.

## Result

G1-E passed its offline criteria. The canonical `test.cmd` completed with exit
code 0, the single ordinary ASI candidate built successfully in an isolated
directory, and a focused review of the integrated Game WndProc path found no
synchronous wait for the Overlay owner.

This is not live-runtime validation. NVIDIA Performance HUD, G-SYNC Indicator,
Frame Generation, in-game input behavior and graphics-driver teardown remain
unverified for this candidate.

## Validation results

| Check | Result |
| --- | --- |
| Canonical registry audit | PASS — 60 harness targets, 63 ordered invocations |
| Full `test.cmd` | PASS — exit code 0 |
| Production-adapter matrix | PASS — 102 scenarios × Diagnostics OFF/ON = 204 fresh processes |
| N06-M01 Zoom fixtures | PASS — Zoom IN/OFF, IN/ON, OUT/OFF and OUT/ON |
| Localization/resource audit | PASS — 18 locales; embedded resource and locale checks passed |
| Integrated admission/finalizer coverage | PASS through canonical `input_state_harness` and `composition_presenter_state_harness`; previously passed G1-B lifecycle fixture was not duplicated |
| Single ASI candidate build | PASS — exit code 0 |
| Game WndProc synchronous owner wait review | PASS — no owner join/wait or blocking renderer-mutex acquisition found on the callback path |
| In-game validation / installation | Not performed |

Diagnostics OFF and ON are runtime states controlled by `[Diagnostics] Enabled`
in the INI. They are not separate ASI variants. The adapter matrix launches
the same test executable in isolated child processes with each state.

The WndProc review traced callback admission, input forwarding, keyboard and
mouse handoff, owner-command queuing, terminal requests and redraw wakeups.
Owner-directed commands use bounded queue operations and event signaling;
input-bridge callback operations use try-lock behavior. Settings mutation,
persistence and notification publication occur when the owner processes the
queued command. The auto-language callback performs its existing game-language
read on the GameHwnd callback before queuing the result; this is not a wait for
the Overlay owner. Normal native window-procedure continuation remains intact.

## Candidate identity

The isolated candidate was built at:

`build-artifacts/cm/g1e-candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

SHA-256:

`2ACF313142A25B6263E053F384C2646AB44A135FFAA2A6474C9C1D3182787CE3`

The build returned exit code 0. Its output included two C4201 warnings in the
vendored `external/safetyhook/Zydis.h`; no test or build failure resulted. The
candidate remains an offline artifact and was not copied to the game or release
directories.

## Limits and next gate

- Offline fixtures do not establish behavior of the integrated Overlay HWND
  with the game's real window-message schedule or graphics stack.
- `Complete`, `Retained` and `Unverified` finalizer states are only as strong as
  their observed local cleanup contracts; a stalled driver/owner can still
  leave resources retained and a display surface visible.
- A controlled user-run live test is still required for menu open/close, mouse
  and Delete, startup and shutdown, NVIDIA HUD, G-SYNC Indicator and Frame
  Generation. No additional architecture stage is implied by this report.

**G1-E status: Offline PASS. Live runtime: UNVERIFIED.**
