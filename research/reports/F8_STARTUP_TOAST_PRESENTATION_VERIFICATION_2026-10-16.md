# F8 — Startup Toast Presentation Verification

**Date:** 2026-10-16  
**Scope:** Read-only source and existing-evidence investigation. No game launch,
graphics fixture, build, or source change was performed for F8.

## Executive result

F7's readiness gate and 10-second TTL are not implicated by the available
evidence. The current source has no distinct startup-toast rendering backend:
startup toast and menu notifications use the same `DrawNotifications` ImGui
path, the same `DrawImGuiSurface` DirectComposition surface update, the same
surface `Commit`, and the same attachment/display reconciliation. The menu
adds its settings UI and changes requested visibility/input state; it does not
select a different toast renderer.

The current live log shows the display HWND already visible and above the game
before the toast is created. It then records toast vertices in ImGui draw data,
a successful surface update/commit, and the 10-second TTL. The user did not see
the toast. Therefore the unresolved failure seam is not readiness or queue
creation; it is the real desktop presentation of a toast-only frame during the
game's startup/loading presentation state. Available logs do not identify
whether the toast pixels were suppressed/occluded by the startup presentation
transition or whether the toast's own pixels/content were not perceptible.

No production fix is justified yet. One bounded same-run observation is
recommended below to discriminate those remaining explanations.

## Evidence identity and live sequence

The supplied live logs identify the F7 candidate SHA-256
`72F373AB015D93D1662E2D62DCAB67B4FF681C3202D7C08F343748790BFD02E5`.
The overlay log records:

| Event | Monotonic ms | Interpretation |
| --- | ---: | --- |
| Initial root attached | 11,592,359 | Initial cleared surface attached while startup hint remained pending. |
| Display shown, host active, above game | 11,592,359–11,598,015 | Internal HWND/z-order observations precede toast creation. |
| Overlay/input readiness | 11,598,015 | F7 presenter readiness reached. |
| Validated game-language read | 11,599,734 | Actual game-owned language data observed; no fallback. |
| Toast created and draw-list submitted | 11,599,734–11,599,750 | Startup notification entered the common renderer. |
| TTL start | 11,599,750 | Draw-list evidence and attached/display commit path accepted. |
| Expiry, hide and detach | 11,609,750 | Exactly 10,000 ms after TTL start. |
| First manual menu open | 11,629,312 | About 19.56 s after TTL start and 9.56 s after toast expiry. |

At toast creation and TTL start the log reports `root_attached=1`,
`display_visible=1`, `hwnd_visible=1`, `host_active=1`, and
`z_above_host=1`. These are state observations, not proof of visible pixels.
The later manual menu opening does not overlap the toast lifetime in this run;
therefore the run cannot compare toast-only and toast-plus-menu display in the
same notification lifetime.

## Source trace: startup-only versus menu-open

### Shared rendering path

- `Renderer::RenderImpl` calls `PrepareStartupNotification`, drains queued
  notifications, draws the settings panel only when the requested menu state
  is open, and then calls `DrawNotifications` in either state
  (`src/overlay/presenter/renderer_runtime.cpp`, around lines 2040–2055 and
  2720–2725).
- `Renderer::DrawNotifications` creates the notification ImGui window at the
  horizontal center and a fixed top inset, draws the title/body, and records
  that notification's draw-list token (around lines 1843–1925).
- `RenderImpl` recognizes the toast draw list only if its vertices are in
  `ImDrawData`, computes damage, and sends the resulting draw data to
  `DrawImGuiSurface` (around lines 2730–2790).
- `DrawImGuiSurface` performs `BeginDraw`, renderer submission, `EndDraw`, and
  DirectComposition `Commit` (around lines 1261–1324).
- `Renderer::Render` then calls `StartPendingNotificationLifetimes`; this
  reconciles root attachment and display HWND before starting TTL (around lines
  1929–1937 and 1809–1838).

Thus a successful toast-only frame is drawn and committed through the same
surface as menu frames. The startup path does not depend on a user-generated
menu visibility transition. At initialization, the pending startup state can
keep the root attached; after the language probe succeeds, notification
creation dirties the surface and its draw-list submission starts the bounded
animation/redraw cadence.

### Z-order and visibility

`ReconcileCompositionAttachment` reconciles the same display HWND for either
menu demand or active notification demand. `UpdateOverlayWindowBounds` checks
the relative game/display order and promotes the visible overlay display into
the topmost band when necessary (`renderer_runtime.cpp`, around lines
1400–1515 and 1762–1807). This makes a missing startup-specific reconciliation
call unsupported by the current source and the supplied log: the toast had
already attached root/display and recorded above-host state before TTL began.

The z-order observation is relative to the game HWND and sampled only when the
code reconciles/logs it. It does not prove uninterrupted pixel visibility
against every desktop/compositor state throughout the 10 seconds.

## Existing deterministic tests

The existing `composition_presenter_state_harness` already has focused
coverage relevant to this question:

- `TestInputLifecycleAndNotificationRedraw` asserts that a hidden settings UI
  with a notification still requests notification redraw and that TTL begins
  only after a committed frame.
- `TestStartupToastAttachmentLifecycleWithoutMenu` asserts that a pending or
  queued startup toast alone keeps composition attached, remains one-shot, and
  survives menu open/close transitions without losing its attachment demand.
- `TestToastLifetimeRequiresSubmittedVisibleGeneration` rejects an unrelated
  draw list or blank commit as toast submission and checks the attachment-commit
  condition for starting lifetime.
- Frame-state assertions verify notification animation uses the compositor
  clock path where available, with the bounded timer fallback.

These tests establish the tested policy/state-machine decisions only. They do
not invoke a real ImGui/D3D11/DComp frame, inspect displayed pixels, emulate
game startup mode transitions, or prove the composition surface was perceived
onscreen. F7's previously recorded full canonical suite passed, but it is not
new F8 presentation evidence. No F8 test was rerun because the existing
deterministic cases already exercise the relevant pure contracts and a test
cannot establish the missing desktop-pixel observation.

## Hypotheses and disposition

| Hypothesis | Status | Evidence |
| --- | --- | --- |
| Readiness gate failed or waited for fallback incorrectly | Rejected for this run | Validated language read, eligibility, and toast creation are logged. |
| TTL elapsed before first submitted toast frame | Rejected for this run | TTL begins at draw submission/attached commit and expires exactly 10 s later. |
| Startup toast uses a separate renderer/backend | Rejected by source | Toast and menu share the same renderer, surface and commit path. |
| Startup toast skips root/display reconciliation | Rejected by source and run | Notification demand invokes the common reconciler; root/display were already attached/visible. |
| Relative game z-order was below the host at the recorded checkpoints | Rejected at those checkpoints only | `z_above_host=1`; this does not prove continuous unobscured desktop pixels. |
| Startup/loading presentation state prevented or displaced toast-only pixels | Open hypothesis | Toast-only interval coincides with startup; menu was opened only after toast expiry. No presentation-path telemetry or image evidence distinguishes this. |
| Toast-specific layout/content/alpha made it imperceptible | Open hypothesis | ImGui draw-list vertices exist, but logs do not capture rendered pixel/color/bounds or screenshot. |

## Single discriminating follow-up experiment

If another live observation is approved, use the same F7 candidate and one
normal game launch; do not change code, readiness, TTL, or graphics settings.

1. Keep the startup toast lifetime intact. Observe/capture the screen from
   before the expected toast through the first few seconds after the log's
   `ttl_start` equivalent. Do not open the menu immediately.
2. While the 10-second toast is still pending, press Delete once and record
   whether the toast becomes visible over the menu. Capture the screen/video
   across the transition and save all three mod logs for the same run.
3. Interpret only this contrast:
   - Toast absent before Delete but visible immediately after the menu
     transition: evidence favors a startup-only composition/presentation or
     visibility-transition issue.
   - Menu visible but toast still absent while its logged TTL remains active:
     evidence favors toast-specific geometry, alpha, localization/content, or
     draw-data clipping rather than a global startup-only presentation failure.
   - Toast visible before Delete: the prior non-observation was not reproduced;
     do not infer a fix or causal mechanism.

This is one ordinary integrated observation, not a graphics fixture. It must
not be automated by the agent. A capture should include the screen; logs alone
cannot resolve perceived pixel visibility. Do not repeat if it fails to
separate these outcomes; first inspect the captured screen and exact-run logs.

## Disposition

- Keep F7 readiness gate and 10-second TTL unchanged.
- No production change: no source-proven defect has been found in the shared
  toast/menu render or attachment path.
- F8 remains unresolved at the desktop presentation boundary. The next useful
  evidence is the one same-lifetime before/after-Delete visual comparison above,
  not another assertion that `Commit` is insufficient.
