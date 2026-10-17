# Overlay-owned HWND — bounded design review

Date: 2026-10-12. Status: **Design proposal; implementation not approved.**

## Decision and evidence boundary

Use one owned, non-activating top-level Overlay HWND, created on the existing
Overlay owner thread. Its DComp target persists across menu/toast open and close.
The game HWND retains keyboard, native language and game-activation duties.
No Camera Tweaks DComp target is created on the game HWND in this proposed path.

This removes target-layer occupancy on the game HWND and per-toggle target
recreation. It does not prove NVIDIA HUD compatibility, Independent Flip, VRR,
Frame Generation stability, or the mechanism behind the earlier stalls.
Production root detach remains unchanged. All target-release artifacts remain
preserved; neither experiment is reclassified as a successful production fix.

The original G-SYNC-return reference is preserved as
[C-root reference](../evidence/overlay-compatibility/root-detach-gsync-return-2026-10-12/REFERENCE.md).
Its ASI SHA-256 is
`78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E`.
That observation belongs to the root-detach experiment, not this design.

## 1. Ownership and window contract

| Object | Responsibilities | Lifetime owner |
| --- | --- | --- |
| GameHwnd | Delete/keyboard, native language requests, existing raw input, activation, discovery and client geometry | Game; borrowed by Overlay |
| OverlayHwnd | DComp target, display surface placement, visibility and ordinary mouse hit-testing | Existing Overlay owner thread |
| Renderer | Existing graphics resources, ImGui state, serialized rendering and target attachment | Existing Overlay owner; no second renderer/runtime owner |
| Composition runtime | Native Overlay window creation/destruction, event filtering and placement scheduling | Existing Overlay owner thread |

Renderer borrows both handles. The drawing window is an owned `WS_POPUP`, not a
child window; the game HWND is its owner. An owned popup stays above its owner,
without requiring global always-on-top behavior. Owner replacement requires a new
Overlay HWND, not in-place owner transfer. Ordinary menu toggles never replace it.
[Window ownership contracts](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features).

### Selected styles and the first feasibility gate

Use `WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED`. Add
`WS_EX_TRANSPARENT` only for passive/toast-only display; remove that flag for an
interactive menu. Set layered alpha to 255, without a color key. Use the existing
premultiplied DComp surface; no GDI background brush, no UpdateLayeredWindow backend.
The initial proposal does not combine this with `WS_EX_NOREDIRECTIONBITMAP`.

Layered + transparent windows provide documented mouse pass-through. In contrast,
`HTTRANSPARENT` forwarding is documented for windows in the same thread; it is not
the cross-thread passive-input contract used here.
[Layered windows](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features),
[WM_NCHITTEST](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest).

**G0 is mandatory before renderer integration:** a local native fixture must verify
layered HWND + DComp alpha rendering, absence of an opaque full-window rectangle,
passive click-through, and the first interactive click without activation.
Microsoft permits a layered window as a DComp target, but that permission alone
does not establish this particular pixel/input behavior or its performance.
If G0 fails, stop and revise the design; do not silently change styles or backend.
[CreateTargetForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-createtargetforhwnd).

## 2. Keyboard, mouse and cursor

### Input ownership states

| Display state | Native mouse policy | Input bridge |
| --- | --- | --- |
| Closed, no notification | Overlay HWND hidden; root detached; target retained | Inactive |
| Notification only | Visible, transparent/pass-through, no activation | Inactive |
| Menu requested but not successfully shown | No interactive input lease | Inactive |
| Menu shown, game focused, geometry valid | Interactive client area, no activation | Active |
| Focus lost, host invalid or failure | Input lease withdrawn immediately; hide scheduled | Inactive |

Logical menu visibility remains distinct from accepted interactive presentation.
Extend the existing input-ownership snapshot with that acceptance condition;
do not introduce another mutable owner. Bridge activity is permitted only when
menu requested, game focused, geometry/host valid, presentation accepted and no
terminal failure. Input must not disappear into an invisible menu.

Delete remains handled by the game WndProc with the existing key-up semantics.
Escape and rebinding keyboard events also remain there. Keep the ImGui Win32
backend bound to GameHwnd: it supplies the game-client coordinate frame and native
keyboard handling. DComp drawing is independently bound to OverlayHwnd.
All auto-language requests, including startup and manual Auto selection, continue
to target GameHwnd and its native-language thread.

OverlayHwnd returns `MA_NOACTIVATE` for `WM_MOUSEACTIVATE`, not
`MA_NOACTIVATEANDEAT`, so the first click is retained without moving keyboard focus.
Interactive client hit-testing returns `HTCLIENT`. No SetFocus,
SetForegroundWindow, AttachThreadInput or synthetic keyboard forwarding is added.
[WM_MOUSEACTIVATE](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mouseactivate).

Ordinary Overlay mouse positions/buttons/wheel enter the existing bounded bridge,
normalized to physical game-client coordinates. Messages actually delivered to
GameHwnd, including wheel/capture-dependent mouse messages, use the same bridge
once; they are not also forwarded to OverlayHwnd. Existing game WM_INPUT remains
the sole raw-motion source. Preserve relative-motion precedence and default native
cleanup; do not register a second raw-input sink or duplicate raw button events.

Do not add an OS SetCapture/ClipCursor lease. The proposed menu drag domain is the
game client. Add a narrow bridge PointerCancel event for WM_MOUSELEAVE,
WM_CANCELMODE and WM_CAPTURECHANGED to release pending ImGui mouse buttons,
without resetting keyboard focus or the entire bridge. This is an explicit
client-bound drag contract requiring approval, not a claim of equivalent dragging
outside the old window. Owner-thread code alone applies queued events to ImGui.

Closing, losing focus or failing presentation withdraws input ownership before
graphics cleanup. Restore the cursor through the game's original WM_SETCURSOR
continuation on its thread. Owner-originated restoration posts a private game
message; never synchronously SendMessage across threads while holding the renderer
lock. Do not introduce ShowCursor counter manipulation or pointer warping.

## 3. Display and resource lifecycle

Host eligibility requires live Game/Overlay HWNDs, valid usable geometry, a
non-minimized game and game foreground eligibility. Existing focus notifications
and foreground observation update this state; no per-frame foreground polling.
On focus loss, deactivate input and cancel rebinding immediately, then schedule
owner-thread hide. Retain logical menu intent for restoration after a fresh frame.

### First display / reopen

1. Create the hidden owned HWND, establish alpha and physical placement.
2. Initialize Renderer with both handles; create target only for OverlayHwnd.
3. Request language synchronization through GameHwnd where needed.
4. Draw/EndDraw/Commit the surface; attach the root and commit attachment.
5. Show with SetWindowPos using NOACTIVATE/NOOWNERZORDER; check its BOOL result.
6. Publish accepted presentation/input eligibility, then start committed
   notification lifetimes only after accepted host show.

Do not interpret ShowWindow's previous-visibility return as success. These steps
prove local API acceptance, not scanout or DWM completion; no
WaitForCommitCompletion is introduced.
[SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos).

### Close / notification expiry

Withdraw interactive input first. When neither menu nor notification requires
display, perform the existing clear/root-detach transaction and hide OverlayHwnd.
Keep device, target, visual and renderer resources resident. Reopen reuses the same
target. Do not destroy/recreate target at toast expiry or each menu toggle.

Queued startup notifications do not start their lifetime while the host is
ineligible. A notification whose lifetime already started keeps wall-clock expiry
when hidden; remove expired entries before showing again. Do not restart it on
focus regain. Preserve the welcome notification rather than suppressing it.
Scheduler eligibility must prevent pending hints/redraw requests from causing a
background busy loop; eligibility regain schedules one redraw. This affects UI
scheduling only, not camera publication or camera decisions.

### HWND replacement, recovery and cleanup

Game destruction can automatically destroy its owned Overlay HWND. Immediately
invalidate presentation/input acceptance, then serialize resource cleanup on the
owner. A genuinely new GameHwnd permits one existing bounded rebind/recovery path
and a new owned window. Ordinary geometry/focus changes do not recreate targets.
Retain existing bounded device recovery and terminal-disable semantics; no retry
loop or fallback target on GameHwnd.

Cleanup order: withdraw readiness/input, hide, Renderer::Shutdown on owner thread,
then DestroyWindow on the creating thread, followed by observer teardown.
Account for partially created resources and an already-destroyed owned window.
No cleanup under loader lock or joining the worker from a game WndProc.

**Required source-boundary change if implementation is later approved:** current
`DisableAfterOverlayException` can call Renderer::Shutdown on the game WndProc
thread. The new HWND makes that unsuitable. Split immediate nonthrowing fail-closed
readiness/input publication from one queued owner-thread cleanup action. This is
an explicit handoff change to the existing owner, not a new owner, mutex or manager.
If the handoff cannot preserve exception containment, stop implementation.

## 4. Geometry, DPI, activation and self-event filtering

The existing PMv2 owner maps the game client rectangle to physical screen pixels.
Overlay client size matches that rectangle; saved menu coordinates remain
game-client coordinates. Negative monitor coordinates are valid. Compare cached
rectangle/DPI before repositioning; position-only changes do not recreate the
surface or target. Resize follows the existing surface-generation transaction.
Game geometry is authoritative: Overlay WM_DPICHANGED does not resize the game.
Failed placement invalidates input mapping until recovery; do not publish stale
interactive coordinates.

Explicitly exclude current/retiring Overlay HWNDs and the dedicated Overlay class
from FindGameWindow before scoring candidates, even though the existing GW_OWNER
filter already rejects owned popups. Apply the same exclusion before WinEvent
refresh posting. Own-window position events only acknowledge cached placement;
they do not schedule game rediscovery/reposition. Reentrant Overlay WndProc work
queues control/input events and never performs rendering or recursive refresh.

On minimize, Alt-Tab or loss of eligible game foreground, hide without activation.
Restore on owner thread after valid geometry and a fresh frame. Do not rely only
on automatic owner/minimize behavior or force global topmost positioning.

Windowed and borderless are the initial candidate scope. Exclusive fullscreen,
HDR, FG, VRR, multi-overlay z-order and Independent Flip remain verification
items, not guaranteed support. Do not force display modes or add a Present hook
to make the separate window visible. A visible external window can still change
desktop composition behavior.
[Flip-model guidance](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model).

## 5. Minimal proposed API/source delta

| Existing boundary | Proposed change |
| --- | --- |
| Renderer::Initialize(HWND, ...) | Explicit borrowed GameHwnd and OverlayHwnd arguments |
| Renderer::RebindWindow(HWND, ...) | Rebind the pair only for genuine host replacement |
| Renderer::OwnsWindow | Rename/clarify OwnsGameWindow for game message dispatch |
| Renderer::OnWindowGeometry | Keep game geometry authoritative; drawing placement stays owner-managed |
| Renderer visibility | Preserve logical SetVisible; add owner-only host eligibility and derived accepted-interactive presentation |
| Composition attachment | Target the drawing HWND; retain target while hidden; show acceptance precedes notification timing |
| Input bridge | Add narrow PointerCancel and extend ownership acceptance invariant |
| Exception cleanup | Immediate fail-closed publication plus queued existing-owner cleanup |

Keep Render, RequestRedraw and existing settings entry points. No public runtime
state API, second graphics owner, additional hook, INI key or new renderer backend.
Expected future files are composition_runtime.cpp, renderer_runtime.cpp/.hpp,
input_state.cpp/.hpp, targeted fixtures and architecture/safety documentation.
Camera algorithms, native camera ABI, vendor code and native language-reader
implementation are outside the proposed slice.

## 6. Validation and rollback gates — not executed

| Gate | Required evidence |
| --- | --- |
| G0: HWND feasibility | Layered DComp alpha, no opaque rectangle, passive pass-through, first click without activation, no game-HWND target |
| G1: Input | Keyboard/raw/legacy delivery exactly once, mouse wheel/buttons, focus/cursor restoration, PointerCancel, toast never owns input |
| G2: Lifecycle | Stable target identity across toggles, accepted show before toast timer, partial creation/show/placement failures, bounded rebind/device recovery and owner-only cleanup |
| G3: Geometry/scheduler | 100/125/150% DPI, negative coordinates, resize/minimize/restore/Alt-Tab, no self-event feedback or background busy loop, Overlay.Enabled=false creates no HWND/target |
| G4: Regression | Canonical tests/builds, native camera output parity, no extra game hooks/warnings, isolated experimental artifacts |
| G5: Compatibility | Separately approved runtime observations for NVIDIA HUD/indicator activation order and FG/VRR; no inherited PASS from old ASIs |

Stop on activation theft, invisible-menu input capture, nontransparent background,
unbounded target churn, feedback loops, cleanup thread violations or camera
regressions. Never mask failure with waits, retries or a GameHwnd target fallback.
Rollback restores the unchanged production root-detach path only when separately
authorized; retain the pinned C-root and existing experiments/evidence. Do not
overwrite a release archive or silently install an experimental ASI.

## Recommendation and stopping point

The design is ready for review, not production. If approved, the smallest next
step is G0 in a local native fixture, followed by integration only if that gate
passes. Layered/DComp feasibility and the cleanup handoff are the principal design
risks. GPU presentation compatibility remains a later evidence question.

This review changed documentation only. No source changes, tests, builds, game
launches or ASI installation were performed. No new runtime PASS is claimed.

## Subsequent isolated G0 execution

The separately approved G0-only fixture completed with local pixel/input PASS:
[G0 feasibility result](OVERLAY_OWNED_HWND_G0_FEASIBILITY_RESULT_2026-10-12.md).
This resolves the tested layered/DComp feasibility question, not the cleanup
handoff or graphics-stack compatibility. G1–G5 and Renderer integration remain
unapproved; the design-review-only historical statement above is unchanged.

## Subsequent cleanup handoff review

[Cleanup handoff review](OVERLAY_OWNED_HWND_CLEANUP_HANDOFF_REVIEW_2026-10-12.md)
defines the proposed nonlocking terminal revocation, durable owner wake,
in-flight callback drain and guarded owner finalizer. It identifies stable native
continuation lifetime and the distinction between logical input revocation and
physical HWND hit-testing as additional requirements. The earlier short queued-
cleanup sketch is not sufficient by itself. Implementation, isolated handoff
tests, G1–G5 and Renderer integration still require separate approval.

## Final disposition — 2026-10-13

The following later implementation and validation records supersede the
prospective gate statuses above; the historical review text is retained:

- **G0:** PASS in the isolated feasibility fixture.
- **G1-A through G1-E:** complete under their approved offline criteria and
  integrated into the Overlay-owned HWND architecture. G1 finalization removed
  temporary activation tracing; ordinary INI-controlled Diagnostics remain.
- **G2 Lifecycle / G3 Geometry-Scheduler:** complete for the documented gap
  scope. Driver-specific partial DComp API failures and physical mixed-DPI /
  fullscreen edge cases retain the limits recorded in the G2/G3 report.
- **G4 Regression:** PASS for 59 registered harness targets / 62 invocations,
  including 204 adapter child processes and four Zoom scenarios.
- **G5 Compatibility:** CLOSED by the user's acceptance of the live run using
  installed ASI SHA-256
  `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`.
  The exact-artifact observations and evidence limits are recorded in the
  [G5 compatibility report](OVERLAY_OWNED_HWND_G5_COMPATIBILITY_STATUS_2026-10-13.md).

The migration to Overlay-owned HWND is complete. G-SYNC Indicator behavior
while the menu is open remains an accepted observation, not proof of actual
VRR state. The supplied logs do not establish Renderer shutdown cleanup; this
limitation is recorded without reopening the gate or requiring another run.
Do not resume G1–G5 absent a concrete new defect.
