# Overlay-owned HWND — G1 production integration design

Date: 2026-10-13. Status: **design review complete; implementation not authorized**.

## Decision summary

The existing Overlay owner thread can remain the sole owner of ImGui, D3D11,
DirectComposition, the new display HWND and their cleanup. The Renderer should
borrow two distinct handles: `GameHwnd` for game input/language/geometry and
`OverlayHwnd` for its composition target and visible surface. No second Runtime,
Renderer, graphics thread or camera dependency is needed.

The user-approved failure-path requirement changes the proposed input contract:
the Overlay HWND should remain natively mouse-transparent in *all* display
states. Game input continues to arrive at the existing GameHwnd subclass and is
captured only when an atomic, owner-published interactive-presentation gate is
open. The subclass path must not wait for Renderer or bridge locks. On terminal
failure, lock contention or queue saturation, it passes the event to the saved
game continuation. This permits gameplay input to continue even if the owner
thread cannot hide or destroy a still-visible Overlay HWND.

If the owner is permanently hung, its pixels may remain frozen over the game;
the game remains mouse/keyboard controllable, but visual removal is not
guaranteed. A stronger promise of immediate visual disappearance would require
a separately proven OS-level recovery mechanism or a second owner and is not
part of this design.

The design retains G0 and failure-fixture evidence only for their tested
contracts. It does **not** establish production correctness, NVIDIA HUD/G-SYNC
coexistence, Independent Flip, VRR, DLSS Frame Generation behavior or game
compatibility.

## Current source baseline reviewed

This review used the current working-tree files, including existing local
changes. Hashes identify the exact inspected content; they do not imply a clean
or committed tree.

| File | SHA-256 | Relevant current contract |
| --- | --- | --- |
| `src/overlay/composition/composition_runtime.cpp` | `75071D3C1A08DDD831C7C788EF206A95EE18A4436F3459372B81387D10E037B5` | One game HWND subclass, binding lookup behind a mutex, game WndProc work under `g_rendererMutex`, `GetMessage` owner loop, unchecked/supplemental thread-message wakes, synchronous `DisableAfterOverlayException`, owner epilogue without Renderer finalization. |
| `src/overlay/presenter/renderer_runtime.cpp` | `C18840B951FC88713CAB1277FD2728697896EC96BFA62943AA948FD3F12F9D07` | One `window_` currently serves as host, composition target and `ImGui_ImplWin32_Init` HWND. `Shutdown` tears down ImGui and composition synchronously; `ResetComposition` does not return a cleanup outcome. |
| `src/overlay/presenter/renderer_runtime.hpp` | `71ACB8E7FFA16DC3AFA9F77C943CCAE52B3A8A124A47D5865BBB2F2DB88C9B83` | Renderer API and state are single-window. |
| `src/overlay/input/input_state.cpp` | `FAECEBCC5D0E89344FC893B20A13B350C0B695B88C1D2392C54E9FBE690E3AA4` | Bridge deactivation and enqueue use a mutex; input close/rebind operations can lock. |
| `src/overlay/input/input_state.hpp` | `08E51D66880B161511BD6288EDFC6E554A6BFCB7F9FE85167C6971942C298476` | Effective input ownership is currently `visible && focused`; no presentation-acceptance or terminal condition. |
| `docs/ARCHITECTURE.md` | `A53B10B4613D38BA803B2AFC8BD75963590DB12F77EC97AE686B74C540B5335B` | Current canonical architecture still specifies a DComp target on GameHwnd and composition resources resident while hidden. |
| `docs/SAFETY_INVARIANTS.md` | `F2D54954E19033EDE0776D8DEC830C93A7DAB7B3CFBCB83AF6D5E3A926939D70` | Current input and optional-overlay invariants still describe GameHwnd as the input boundary. |

Evidence: G0 local pixel/input PASS is recorded in
[`OVERLAY_OWNED_HWND_G0_FEASIBILITY_RESULT_2026-10-12.md`](OVERLAY_OWNED_HWND_G0_FEASIBILITY_RESULT_2026-10-12.md).
The ten-scenario failure fixture passed twice and is recorded in
[`OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_RESULT_2026-10-13.md`](OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_RESULT_2026-10-13.md).
These are separate surrogate fixtures, not production integration evidence.

## Ownership and authoritative state

| State/resource | Authoritative owner | G1 rule |
| --- | --- | --- |
| Camera runtime and module configuration | Existing plugin Runtime | Unchanged; Overlay failure does not alter camera features/settings. |
| ImGui context, Renderer, D3D11 and DComp objects | Existing Overlay owner thread | All create/use/release and ImGui calls remain serialized there. |
| `GameHwnd` | Game; borrowed by Overlay | Discovery, original WndProc continuation, Delete/Escape/rebind keyboard, native language requests, client geometry, activation and raw-input source. Never a DComp target. |
| `OverlayHwnd` | Existing Overlay owner thread | Display surface/target, z-order/placement, show/hide and its own window procedure. Renderer borrows it; only its creating thread destroys it. |
| Input intent/effective admission | Small process-resident control word in composition runtime | One packed/CAS-updated authority distinguishes menu intent, game eligibility, accepted presentation, terminal state and effective capture. Do not infer effective capture from `InputState::visible()` alone. |
| Cleanup delivery/outcome | Same existing owner lifecycle | Persistent terminal event is authoritative; messages are hints. State is `Starting → Running → Stopping → Stopped`; cleanup outcome separately reports `Pending`, `Complete` or `Retained/Unverified`. No restart after terminal. |

The control record is not a RuntimeState or new generic manager. It contains only
Overlay admission, owner phase, wake/drain handles and cleanup result. Event
handles are created before subclass admission can occur and remain valid for the
process-resident ASI lifetime. `Stopped` never implies successful cleanup.

## Input and degraded-operation contract

### Always-pass-through display HWND

Keep `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW`
for the Overlay HWND in passive, notification and interactive states. G0
demonstrated layered DComp alpha and mouse pass-through with this style; its
interactive step removed `WS_EX_TRANSPARENT`, so permanent pass-through with
production bridge routing still requires explicit G1 validation.

Ordinary mouse events therefore reach GameHwnd. Its subclass feeds the existing
bounded bridge only while effective input admission is open and the committed
Overlay presentation is accepted. Input is consumed only if that event was
accepted for the bridge; otherwise it goes to the original game procedure once.
Do not register a second raw-input sink. Preserve `WM_INPUT` default-procedure
cleanup when pass-through is selected. Preserve the game thread as the sole
keyboard/Delete/native-language boundary, and keep ImGui/D3D11 access on the
Overlay owner.

### Nonblocking game-window boundary

The current `g_rendererMutex`, mutable binding vector, `InputEventBridge` mutex
and some rebind/settings calls are not usable as wait points on the GameHwnd
callback path. G1 integration must either remove these calls from WndProc or use
strict try-only operations with fail-open behavior. No game callback waits,
joins the owner, performs Renderer/graphics cleanup, logs synchronously, or
performs a synchronous cross-thread window request.

Native continuation records must be stable before the subclass is installed and
must outlive any foreign subclass chain that can call back into Camera Tweaks.
The WndProc obtains a stable binding/generation without taking the current
binding mutex. A stale generation can call only its saved continuation; it
cannot mutate the new host's Overlay state. Records remain process-resident or
are otherwise proven not to be reclaimed while any chain/callback can reference
them. Do not use or overwrite the game's `GWLP_USERDATA`.

If an input try-operation cannot enqueue or safely inspect rebind state, close
effective capture for that input epoch, pass the current message through, and
invalidate queued stale mouse/key state so the owner resets ImGui buttons/keys
before a later re-admission. A dropped event must never be reported as consumed.
The exact queue overflow/reset protocol is a mandatory implementation design
item; the current mutex-backed bridge cannot simply be called from the
nonblocking path and still satisfy this contract.

Delete/Escape close requests update atomic menu intent and revoke input capture
before signaling the owner. Opening sets intent only; effective capture stays
closed until a committed frame and successful host visibility/placement are
published. Focus loss, game HWND replacement, terminal failure and invalid
geometry revoke capture immediately. Toast visibility never opens input capture.
Cursor restoration is a best-effort game-thread continuation; no `ShowCursor`
counter manipulation, pointer warping, `AttachThreadInput` or blocking
`SendMessage` is introduced.

### If the owner hangs

Because the Overlay HWND is click-through in every state and GameHwnd processing
does not wait on owner locks, game input continues to reach the game. After a
terminal latch it is passed through; before terminal, queue/lock saturation also
fails open. The UI may remain visibly frozen and cannot promise immediate
owner-thread hide. This is the explicit degraded mode, not a hidden cleanup
success. If user acceptance requires immediate visual disappearance as well as
continued gameplay control, G1 must stop for a separate OS-level recovery design;
it must not solve this by calling `DestroyWindow`, changing DComp resources or
waiting on the game thread.

## Owner wake, finalization and outcome

1. Create and validate terminal-wake and callback-drain events before installing
   the GameHwnd subclass. Publish the immutable continuation binding before the
   subclass can dispatch to it. If control resources cannot be created, do not
   install input interception; the optional Overlay fails closed.
2. Owner creates its message queue and the Overlay HWND, performs graphics/ImGui
   setup, commits and displays the first usable frame, then publishes
   presentation acceptance. No input lease is published earlier.
3. The GameHwnd failure boundary performs only terminal CAS, nonblocking input
   revocation/publication, event signal and optional queue hint. It then releases
   its admission count and passes the current message to the saved procedure.
   No renderer lock, bridge mutex, `Disable`, `Shutdown`, hook removal or owner
   wait is permitted there.
4. Replace blocking `GetMessage`-only cleanup delivery with an event-aware owner
   pump. It checks terminal state before initialization, before rendering and
   between bounded dispatch batches. A queue post is a hint, never authority.
5. On terminal, owner stops admitting render/presentation work, hides the
   Overlay HWND on its creator thread, releases the Renderer lock, drains
   already-admitted game callbacks, then independently performs guarded cleanup.
   No renderer lock needed by those callbacks is held while draining.
6. Owner removes observers/timers, stops and joins the pacing worker, tears
   down ImGui/DComp, then destroys its HWND on the creator thread. Every step is
   attempted independently where safe; a secondary exception cannot skip later
   independent cleanup. A live worker/resource/window that may still be used is
   retained and reported, not raced or falsely marked complete.
7. Publish cleanup outcome from observed resource/window/worker state. `Complete`
   requires zero owned live resources, no worker that can touch them and a
   completed/known window disposition. Failed `DestroyWindow`, unjoined worker,
   failed required detach/commit, or uncertain resource lifetime means
   `Retained/Unverified`. The process-resident control events and inert stable
   WndProc records are documented control-lifetime resources, not leaked
   graphics resources and not dynamic-unload support.

`Renderer::Disable`/`Shutdown` cannot keep synchronously finalizing on arbitrary
callers. Renderer failures request terminal state; only the owner epilogue calls
the finalizer after the current operation and lock scope have unwound. Normal
nonterminal visibility changes remain owner commands. C++ exception containment
does not cover access violations, a permanently blocked driver call or forced
thread termination; those end in `Pending`/`Unverified`, not a fabricated PASS.

## HWND relationship decision still required

The earlier proposal makes OverlayHwnd a Win32 owned popup of GameHwnd to inherit
z-order and minimize behavior. Microsoft documents that an owned window is
automatically destroyed when its owner is destroyed, while `DestroyWindow`
cannot be used by a thread to destroy a window created by another thread.
Those facts expose a cross-thread host-destruction case that G0 did not exercise.
Before implementation, decide and prove one of:

1. Keep the native owner relationship and show that GameHwnd destruction cannot
   block its thread or bypass Overlay owner-thread resource finalization; or
2. Use an unowned top-level Overlay HWND and explicitly manage z-order/minimize,
   accepting that this needs its own compatibility and lifecycle evidence.

Do not assume the OS owner relationship is a cleanup handoff. Host destruction,
Overlay owner stall, HWND reuse and late `WM_NCDESTROY` must be tested together.
See Microsoft [window ownership](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features)
and [`DestroyWindow`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow).

## Proposed implementation sequence — requires separate approval

| Stage | Scope | Exit gate |
| --- | --- | --- |
| G1-A — admission and continuation | Stable binding record; monotonic terminal/in-flight control; no-wait GameHwnd path; fail-open bridge admission and stale-event reset. No OverlayHwnd yet. | Fixture proves an owner held forever in renderer/bridge critical sections cannot stall GameHwnd input; stale/orphan bindings call only saved native continuation. |
| G1-B — separate HWND/Renderer handles | Create/destroy OverlayHwnd on owner; split Renderer host/draw handles; DComp target only on OverlayHwnd; permanent native click-through. Preserve GameHwnd language, Delete, geometry and raw input. | Fixture proves target HWND/thread ownership, alpha, input delivered once via GameHwnd bridge, and no activation theft. Resolve native owner-vs-unowned decision first. |
| G1-C — accepted presentation and lifecycle | Separate logical visibility, host eligibility, accepted visible frame and effective capture. Owner-only event-aware pump, first-frame/show ordering, toast lifecycle, rebind and bounded device recovery. | Partial failures, message queue full, minimize/focus loss, host replacement and repeated open/close preserve the state contract with no self-event loop. |
| G1-D — terminal owner finalizer | Route all Renderer failure/disable cleanup through owner epilogue; callback drain; guarded independent teardown and truthful outcome. | Failure fixture covers actual shared control/bridge helpers, owner-thread resource release, host destruction race, cleanup failure and hung-owner fail-open behavior. |
| G1-E — offline integration validation | Focused tests, full canonical suite, production and diagnostic builds; no unrelated camera output changes. | Exact artifact/hash, zero new warnings, existing camera/native tests pass. Only then prepare an experimental ASI for separately approved live runtime. |

Do not combine this with target-release, camera/runtime decomposition, NVIDIA
workarounds, new settings, or visual redesign. The user accepts reasonable
visual differences: function, readability and correct placement matter; pixel
parity with the old overlay does not. Priority is: game stability/control,
coexistence evidence, complete input/lifecycle functionality, readability, then
old visual matching.

## Mandatory open questions and stopping conditions

- Win32 owned-popup destruction/host-thread behavior described above is not
  established by current evidence.
- Permanent `WS_EX_TRANSPARENT` plus GameHwnd bridge routing is a proposed
  change from G0's tested style-toggle interaction; validate the actual
  interactive path before Renderer integration.
- A nonblocking input bridge overflow/reset protocol and stable WndProc binding
  lookup need concrete implementation designs and targeted tests.
- Owner-thread finalization can remain pending forever if owner or driver is
  permanently blocked. The design preserves game input but cannot promise visual
  hide/resource release in that condition.
- External subclass replacement, host HWND reuse, foreground/focus changes,
  DPI/negative-coordinate placement, minimize/restore, exclusive fullscreen,
  HDR, NVIDIA HUD/G-SYNC, FG/VRR and other graphics overlays remain separate
  runtime gates. G0 and the failure fixture do not close them.
- Existing production behavior intentionally retains DComp resources while
  hidden. This design changes only which HWND owns the target; it makes no
  measured claim that the new window fixes the observed graphics-stack symptoms.

Stop implementation if game callbacks still need to wait for the owner, if a
stale binding can call mutable Renderer state, if the host HWND can implicitly
destroy an Overlay HWND from an unsafe thread, if input is consumed before
presentation acceptance, or if cleanup reports `Complete` with uncertain live
resources. Do not add another worker, global hook, cross-thread window destroy,
synchronous DComp completion wait, retry loop or GameHwnd target fallback to
escape a failed gate.

## Scope and recommendation

**Recommendation:** proceed toward an experimental implementation only after
resolving the native HWND ownership decision and approving G1-A through G1-E as
their own implementation scope. The strongest architecture is a single existing
Overlay owner, a permanently mouse-transparent display HWND, and a nonblocking
GameHwnd bridge whose admission is revoked atomically. It preserves gameplay
control under owner failure without unsafe cross-thread HWND destruction; a
frozen overlay image is the explicitly accepted degraded visual state.

This review changed documentation only. No source, tests, settings, build
artifacts or ASI were modified; no tests, builds or game were run.
