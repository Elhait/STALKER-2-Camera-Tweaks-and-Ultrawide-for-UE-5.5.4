# Overlay-Owned HWND — G1 Finalization

**Date:** 2026-10-13  
**Scope:** Remove G1-only activation tracing, review the integrated ownership
boundaries, update current documentation, and build one standalone ASI.  
**Game launch / ASI installation / ETW / PresentMon:** Not performed.

## Result

G1-A through G1-E remain complete for their approved offline criteria. The
current integrated architecture was reviewed at the `GameHwnd`/`OverlayHwnd`,
input-bridge and owner-finalizer boundaries. No additional source split or
lifecycle change was indicated by this review.

The standalone candidate built successfully. The preceding live-tested C728
artifact was copied unchanged to a rollback path before the build.

## Final ASI and rollback

| Role | Path | SHA-256 | Size |
| --- | --- | --- | ---: |
| G1 finalization candidate | `build-artifacts/g1-final/STALKER2CameraTweaks.asi` | `887FCE3FBC61C85586A874BDF5D9C4C0ADF89EE353B224239A682E8AB39409CD` | 3,570,688 bytes |
| Previous live-tested G1 artifact | `build-artifacts/g1-final/rollback/STALKER2CameraTweaks-C7287C8C-live.asi` | `C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C` | 3,572,224 bytes |

The candidate was produced by `build.cmd` with the standard production profile
and an isolated output directory. The build completed with exit code 0 in
35.3 seconds. It emitted two existing vendor warnings, MSVC C4201 in
`external/safetyhook/Zydis.h`; no project-owned warning was observed. The
build-owned compiler/linker process check found no remaining processes.

This is one standalone release candidate ASI. No archive, `release-assets`
file, game installation or versioned package was changed.

## Cleanup and architecture review

- Removed the G1 activation-stage QPC trace from the WndProc and Renderer,
  together with its header, dedicated harness and registry entries. The
  canonical registry now contains **59 targets / 62 ordered invocations**.
- The ordinary ASI still contains the approved passive runtime diagnostics.
  `[Diagnostics] Enabled` in the INI remains the runtime switch. The bounded,
  change-deduplicated display-state and composition-attachment lifecycle
  records remain available; camera, input and finalizer semantics were not
  changed by this cleanup.
- The `GameHwnd` remains the game-input, language and geometry boundary. The
  separate unowned `OverlayHwnd` owns its DirectComposition target and is
  created/destroyed on the existing Overlay owner thread. Its window remains
  non-activating and mouse-transparent.
- Game-window callbacks use admission plus bounded command/input forwarding;
  settings work and Renderer access remain on the owner. Owner finalization
  drains admitted callbacks and performs graphics/window teardown on that
  same owner thread, with `COMPLETE`, `RETAINED` and `UNVERIFIED` outcomes.
- G0/G1 fixture sources and capture/research records remain under research
  paths as historical evidence; they are not production source inputs.

The activation trace had previously been retained in the diagnostics gate
matrix. This finalization supersedes that task-specific choice: its per-toggle
QPC stage sequence served the G1 first-frame investigation and is removed at
the user's request after integration. General Diagnostics and useful bounded
lifecycle observations remain intact.

## Validation and limits

- Registry audit: **PASS**, 59 registered harnesses matched 59 repository
  harnesses, with 62 ordered invocations, 102 adapter scenarios × two
  Diagnostics states (204 child processes), four N06-M01 Zoom invocations, and
  zero missing inputs.
- `build.cmd`: **PASS**, exit code 0; exact candidate hash and size are above.
- The full harness suite was not rerun after removing the trace-only harness.
  Earlier G1-E full-suite PASS results apply to the prior revision, not this
  final candidate.
- No live game test was repeated. The positive NVIDIA Performance HUD result
  and the user's acceptance of the G-SYNC Indicator behavior remain the live
  evidence for the previous C728 artifact; they do not automatically validate
  this newly built hash.
- The reported post-toast freeze and ETW `UserRequest` waits remain documented
  observations with unknown waited object and cause. They do not block this
  integration step under the user's current decision; no causal fix is
  claimed.

`TESTING_AND_RESEARCH.md` and `docs/ARCHITECTURE.md` now describe the integrated
Overlay-owned HWND and the single ordinary ASI / INI Diagnostics model.
