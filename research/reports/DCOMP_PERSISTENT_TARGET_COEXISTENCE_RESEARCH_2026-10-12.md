# Overlay coexistence without target recreation on the game HWND

Date: 2026-10-12. Scope: read-only source/architecture research and authorized
reference-artifact preservation. No production/vendor edits, build, game launch,
installation, new instrumentation or experimental implementation.

## Outcome

There is a viable architectural candidate: a **persistent target on a separate
Overlay-owned top-level HWND**. This removes our occupation of the game's two
DComp target layers without ordinary menu/toast destroy/recreate. It does not
guarantee NVIDIA visibility or presentation neutrality. Recommend a bounded design
review of that candidate, not implementation or a Renderer migration now.

Pinned G-SYNC-return reference: original C root detach, SHA-256
78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E.
[Reference copy, logs and provenance](../evidence/overlay-compatibility/root-detach-gsync-return-2026-10-12/REFERENCE.md).
Production root BF5B remains unchanged. E-TR 5D2A and E-Timed 011E remain preserved
research baselines, not REJECTED as an entire compatibility direction.

## Current integration that a design must respect

- CreateCompositionTarget prefers TRUE on game HWND, with FALSE fallback.
  Private D3D11/IDCompositionSurface ownership and owner scheduler remain separate
  from native game swapchain/Present. Idle root detach retains target.
- Renderer::window_ is not exclusively a composition destination. OwnsWindow,
  initialization/rebind/recovery, ImGui_ImplWin32_Init, forwarded Win32 input and
  Auto-language synchronization all use it. Relevant anchors:
  src/overlay/presenter/renderer_runtime.cpp:729, 768, 904, 1026, 1362, 1616, 1937.
  Replacing only CreateTargetForHwnd's argument is insufficient.
- Input subclass is on game HWND and Delete must stay available while UI/target
  content is hidden. Moving drawing to another HWND must not silently move native
  language reading or keyboard ownership there.
- FindGameWindow enumerates same-process visible, unowned non-child windows by
  largest client area (:217-241, composition_runtime.cpp). An owned Overlay HWND
  is already excluded by GW_OWNER, but explicit self-exclusion is still a design
  requirement if ownership is absent or changes. WinEvent handling (:246-258)
  presently accepts all same-process HWNDs, including a future Overlay window;
  its geometry/show events must not create discovery/position feedback loops.
- The shared renderer/window-procedure lock and exceptional cleanup constraints
  remain. This research does not authorize shortening critical sections, another
  graphics owner or game presentation synchronization.

## Alternatives

| Candidate | Avoids per-menu target recreation? | Frees game's DComp layers? | Assessment |
| --- | --- | --- | --- |
| Current retained game target + root detach | Yes | No | Preserve production control; established indicator-return behavior, unresolved HUD coexistence. |
| Permanent FALSE game target | Yes | Only the TRUE layer, while occupying FALSE | Narrow hypothetical layer-choice alternative; not a complete coexistence solution. TRUE/FALSE order concerns child windows, not arbitrary NVIDIA components. No evidence justifies a permanent switch. |
| Retained target on own nonactivating top-level HWND | Yes | Does not acquire them at all | Best candidate for isolating target-layer occupancy while reusing private renderer/surfaces. Requires explicit two-HWND input/lifetime design and visibility coordination. |
| Retained target on child HWND of game | Yes | Does not acquire parent's target slots | Still changes the game's child-window/composition hierarchy and clipping/input behavior. Less independent; not preferred. |
| Render directly into game buffers | Removes separate target | Yes | Adds device/queue/backbuffer/Present integration, particularly risky with Streamline/FG and wrappers. Outside current no-game-Present contract; not recommended as a small fix. |
| SetOpacity(0), clear content, or another wrapper around same game target | Yes | No | Changes visual content, not retained HWND target occupancy; does not structurally remove the investigated conflict candidate. |

Windows documents at most one target per layer for each HWND. A different HWND
has its own target binding, whereas TRUE/FALSE refers to child-window ordering.
[CreateTargetForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-createtargetforhwnd).
NVIDIA sharing those slots remains a hypothesis, so removing that dependency is
not proof of removing its actual internal conflict mechanism.

## Separate-window candidate: bounded design requirements

One existing Overlay thread creates an owned, nonactivating top-level drawing
window and its target once; a normal toast/menu cycle draws/shows and clears/hides
without target destruction. Root detach can remain on that window. Real HWND
destruction, game HWND replacement, device loss and terminal cleanup are separate
boundaries where recreation can be justified. No periodic target recreate/retry.

The ownership contract must name game/input/language HWND separately from drawing
HWND without creating another Runtime owner. Exact public/private API changes and
failure propagation need review before code. Required design points:

1. No focus theft, taskbar/Alt-Tab clutter or globally persistent topmost window.
   Track game activation, minimize/restore, client-to-screen geometry, DPI and
   multi-monitor coordinates; hide alongside unavailable/inactive game as defined.
2. Resolve mouse hit-testing and pass-through without changing existing raw-mouse,
   cursor restoration, keyboard rebind or Delete semantics. Do not assume
   WS_EX_NOACTIVATE implies click-through, or HTTRANSPARENT crosses window threads.
3. Retain owner-thread D3D11/ImGui/target lifetime and existing serialized error
   boundaries. A failed drawing-window setup disables only optional Overlay.
4. Preserve startup toast's draw/attachment-before-lifetime contract. Showing an
   actual window is still not proof that the user saw it; foreground policy must
   be explicit, not coupled accidentally to language or readiness.
5. Filter self-generated discovery/geometry events and separate logical game
   HWND rebind from drawing-window placement updates. No hide/show feedback loop.
6. Establish supported borderless/windowed/HDR/FG behavior separately. A second
   HWND can interact differently with exclusive fullscreen, capture and z-order.

WS_EX_NOACTIVATE and WS_EX_NOREDIRECTIONBITMAP are relevant design ingredients,
not a complete tested recipe. The former prevents ordinary foreground activation;
the latter avoids a redirection bitmap for a window using other visual mechanisms.
[Extended window styles](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles).
HTTRANSPARENT's documented forwarding is within the same thread, while our game
and Overlay owner windows would normally be on different threads.
[WM_NCHITTEST](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest).

## What this can and cannot establish

It structurally avoids a Camera Tweaks target on the game HWND and avoids ordinary
target destroy/recreate churn. It **cannot guarantee Independent Flip, MPO, VRR,
FG stability or active NVIDIA HUD/indicator visibility**: a visible window above
the game is still desktop composition content. Windows may compose, reverse-compose
or use MPO depending on conditions.
[DXGI flip model guidance](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model).

The retained-target game-HWND approach already has a known HUD symptom; the
target-release approach has mixed live results. No source-only candidate proves
coexistence independent of activation ordering. A graphics trace does not supply
missing NVIDIA internal contracts either.

## Recommendation and stopping point

Keep the pinned C-root reference and unchanged production root detach. Preserve
target-release artifacts and all findings. If the user approves a next phase,
prepare only a separate-window boundary design with input/focus, self-event,
language and cleanup contracts; implementation would require separate approval.
Do not create a new ASI, add INI switches, restart game experiments, disable the
startup toast or force FALSE/opacity as a workaround in this research phase.

Reference preservation validated by copy hash and original live identity. Research
is source/platform-contract analysis only; no new runtime PASS is claimed.

## Subsequent bounded design review

The separately approved design-only phase is recorded in
[Overlay-owned HWND bounded design review](OVERLAY_OWNED_HWND_BOUNDED_DESIGN_REVIEW_2026-10-12.md).
It defines the two-HWND responsibilities, passive/interactive input policy,
owner-thread cleanup handoff, geometry/self-event filtering and mandatory G0
layered/DComp feasibility gate. Implementation and a new experimental ASI remain
unapproved. The earlier research and preserved artifacts are unchanged.
