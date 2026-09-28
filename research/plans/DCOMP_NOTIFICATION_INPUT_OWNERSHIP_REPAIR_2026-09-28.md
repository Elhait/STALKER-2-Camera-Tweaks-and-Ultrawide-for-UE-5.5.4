# DComp Notification and Input Ownership Repair — 2026-09-28

## Scope

Repair the production DirectComposition presenter scheduler, notification
visibility, input redraw/ownership, minimize/restore routing, device-recovery
input transaction, and keyboard/focus lifecycle. Preserve the D3D11/DComp
boundary, zero game DXGI/Present/Resize hooks, Camera Runtime independence,
and current UI, settings, localization, and hotkey semantics. Do not alter
theme/colors or reintroduce the game-swapchain renderer.

## Required invariants

### Notification lifecycle

`Created -> PresenterPending -> FrameBuilt -> SurfaceDrawn -> CompositionCommitted
-> VisibleLifetime -> Redraw/Fade -> Expired -> ClearCommitted`.

- Startup hint is presenter-owned state and makes the hidden presenter drawable.
- It is queued before frame construction, not from inside the frame after the
  notification drain.
- Its visible lifetime begins only after a successful commit containing its
  non-zero-opacity draw data.
- Pending notifications cannot expire while waiting for readiness or commit.
- Fade, expiry, and the final transparent clear are scheduled by the presenter
  owner, independent of settings visibility, game frames, and input events.
- A thread timer is owned by its actual SetTimer-returned identifier;
  replacement, WM_TIMER dispatch, cancellation and shutdown use that ID.
- Active visible UI and queued input changes request presenter-owned redraws
  without game presentation callbacks.

### Input ownership

- The game HWND remains the host and message boundary; no independent overlay
  HWND or game-presentation hook is introduced.
- Closed settings UI leaves mouse/keyboard messages and raw input with the game.
- Open settings UI captures the defined interactive input events at the HWND
  boundary, copies them into a bounded bridge, and applies them only on the
  ImGui/DComp owner thread.
- Relative raw mouse deltas move the ImGui virtual cursor; absolute client
  mouse messages remain the fallback when raw movement is not available.
- Closing the UI atomically disables capture, clears pending bridge state, and
  restores game message delivery. Renderer readiness does not own input state.
- Input reset transitions cover keyboard/focus state as well as mouse buttons
  and virtual cursor position; focus reaches ImGui on the owner thread.
- No repeated cursor warping, ClipCursor contention, or game-thread ImGui calls.

### Geometry and recovery lifecycle

- Minimized is a live presenter phase, not equivalent to uninitialized. Restore
  routes through geometry/surface-generation validation and commit.
- Device recovery preserves a coherent panel visibility, InputState and bridge
  ownership tuple, whether the panel is open or closed.

### Presenter frame pacing

Use four semantic scheduling states rather than a fixed visible-panel timer:

- `Idle`: no dirty surface and no active notification animation; no clock/timer
  wakeups, only external owner-thread events.
- `NotificationAnimation`: pace dirty/fade/expiry frames from the system
  DirectComposition clock where available; stop when no notification requires
  animation or clearing.
- `InteractiveUi`: static open UI is event-driven, not continuously redrawn;
  input, hover, scroll, drag and settings changes mark the shared UI frame dirty.
- `EventDrivenRedraw`: coalesce state/input changes, then draw/commit on the
  next available compositor tick. Only the ImGui owner thread touches UI or GPU.

The DirectComposition compositor clock is the preferred pacing source because
the owner surface is an asynchronous DComp batch and the clock follows system
composition cadence. Its blocking wait must not replace the owner thread's
Win32 message pump; a pacing-only worker may emit ticks but must never touch
ImGui/D3D11. Activate that worker only while paced frame work is outstanding.
If the API is unavailable, retain a bounded compatibility fallback; do not
require a Windows version change. No idle busy loop, cursor polling, game
Present dependency, or continuous rendering for a clean static panel.

Validation must cover policy transitions among all four states, coalesced input
updates under high-rate event bursts, notification animation pacing/expiry,
idle quiescence, and clock-unavailable fallback.

## Source evidence

- Startup hint is currently published from `Renderer::RenderImpl` after
  `UpdateNotifications()`, so it requires a later drain/render transition.
- `OVERLAY_STARTUP_HINT_FRAME_COMMITTED` currently checks notification vector
  membership after commit rather than proving that the hint emitted visible
  draw data.
- `OverlayWindowProc` currently calls `ImGui_ImplWin32_WndProcHandler` on the
  game HWND thread while ImGui NewFrame/render work belongs to the composition
  worker thread.
- The visible-overlay `WM_INPUT` path checks only the header and sends the
  message to default processing; it does not expose relative mouse deltas to
  ImGui. Renderer-side `GetCursorPos` is not an authoritative movement source
  when the game controls/recenters the OS cursor.

## Implementation and validation

1. Correct Win32 timer identity and lifecycle; use it for autonomous redraw,
   notification deadlines and visible-panel frames. Input changes request an
   owner-thread redraw through the existing bounded bridge.
2. Track notification draw-list submission through ImGui Render and start
   lifetime only after the corresponding DComp surface commit/publication.
3. Route minimize/restore from published-generation state, never from
   `ready()==false` alone. Keep resize/rebind generation transactions.
4. Make graphics recovery preserve a coherent input ownership tuple; queue
   keyboard/focus reset and focus events to the ImGui owner thread.
5. Add deterministic fixtures that exercise production routing and scheduler
   behavior: autonomous timer progression, sizing-frame to visible-frame toast
   lifetime/expiry, input-triggered redraw, minimize/restore, visible/hidden
   device recovery, and keyboard release/focus loss.
6. Run focused harnesses, full `test.cmd`, production build, and
   `git diff --check`; do not launch the game or perform Git/release/package
   actions.

## Acceptance boundary

Offline checks establish policy transitions and thread-boundary contracts only.
The already-confirmed runtime lifecycle covers startup toast, UI visibility,
mouse/keyboard/focus, stability, and Frame Generation. One runtime retest is
still required specifically to confirm the new compositor-paced cadence feels
smooth on the user's high-refresh display and that the production log reports
the compositor-clock path (or the bounded fallback when the API is absent).
