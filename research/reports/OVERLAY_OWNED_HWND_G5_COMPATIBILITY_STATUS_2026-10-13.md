# Overlay-Owned HWND — G5 Compatibility Status

**Date:** 2026-10-13  
**Gate:** G5 Compatibility  
**Status:** **CLOSED — user-accepted live compatibility result for this candidate.**

## Plan criteria

The master plan requires separately attributable runtime observations for the
NVIDIA HUD/indicator activation order and Frame Generation / VRR. Results from
an older ASI do not transfer to this candidate. No new ETW, PresentMon, or
synthetic graphics scenario is part of this step.

## Exact artifact and rollback

- Installed target: `E:/Steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl/Stalker2/Binaries/Win64/STALKER2CameraTweaks.asi`
- Candidate SHA-256: `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`
- Candidate size: 3,572,736 bytes.
- Rollback copy: `build-artifacts/g5-rollback/STALKER2CameraTweaks-pre-G5-C7287C8C.asi`
- Rollback SHA-256: `C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C`
- The rollback matches the previously installed/live-tested C728 artifact.

Installation was an atomic same-volume replacement after verifying both the
current target and rollback hashes. The target hash was checked again after
installation. No temporary staging files remained. The agent did not launch
the game during installation; the subsequent user-run session is recorded
below.

## Live result and evidence boundary

The user reports that the current run looked good and accepts G5 closure for
this exact installed candidate. The supplied logs identify the running ASI by
the same SHA-256 as the installed target and show successful initialization,
first frame, input activation, startup notification draw/commit, and repeated
menu show/hide and input-ownership transitions. No `ERROR`, `WARN`, `FAIL`,
exception or crash entry appears in the three supplied logs.

Compatibility observations are attributed to the user's visual/runtime report;
the logs do not instrument NVIDIA HUD, G-SYNC, Frame Generation or actual VRR
state. The previously accepted G-SYNC Indicator behavior while the menu is
open remains accepted; it is not evidence that VRR itself turns off.

The logs contain no Renderer shutdown/finalizer record. Therefore cleanup is
**not established by these logs**; this evidence limit does not overturn the
user's G5 acceptance and does not trigger another test cycle. A process
snapshot after log review found no running game process, but that is not proof
of graphics-resource teardown.

G5 is closed by the user's explicit disposition. Do not repeat G1–G5 or start
another graphics investigation unless a concrete new defect appears. The
verified rollback remains available at the path and hash above.
