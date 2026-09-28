# DirectComposition Overlay backend migration — 2026-09-28

Authority: user's production migration request following runtime validation of
the DComp feasibility prototype. The prototype visibly rendered over STALKER 2,
survived native FG OFF→ON→OFF→ON and repeated resolution/window-mode changes;
its recorded surface generations completed with S_OK. This validates the
composition attachment in that runtime, not the production ImGui integration.

## Contract and scope

Replace the production game-swapchain D3D12/DXGI Overlay backend with an
independent DirectComposition presenter. Camera Runtime, the existing ImGui
views, settings/config API, localization/catalog/font behavior, notifications
and startup toast, Delete/Esc and cursor semantics, Camera State, optional-UI
failure isolation, and applicable FS-01–FS-05 behavior remain intact. No
production DXGI factory, game Present/Present1, ResizeBuffers/ResizeBuffers1
hooks, game backbuffer ownership, or old-backend production fallback remain
after parity is reached.

The composition presenter owns a D3D11 device, Direct2D/DirectComposition
objects, its current HWND target and surface generation. A dedicated
message-driven owner thread discovers the current game HWND, handles
composition-surface generations and redraw scheduling, and serializes all
ImGui/backend calls. It does not intercept or call into the game's swapchain.
Input messages on the game HWND enqueue/update the same owned input state under
the presenter's synchronization boundary; camera settings remain owned by
Runtime and notification state remains delivered through the existing API.
Resize uses a prepare/draw/attach/commit/retire transaction: publish the new
surface only after successful full draw and composition commit; keep the prior
generation until that commit succeeds. Minimize suppresses drawing without
discarding UI/settings state; restore re-queries the current client rectangle.
Device loss retires the affected graphics generation and attempts one bounded
reinitialization on the presenter owner thread; any terminal Overlay failure
does not affect Camera Runtime.

No per-frame synchronous GPU wait is permitted. Redraws are event/dirty-driven
when hidden and idle; visible UI and animated notifications use a bounded
presenter-owned cadence. Production diagnostics stay lifecycle-oriented and do
not add per-frame file I/O.

## Batches

1. Add deterministic presenter lifecycle/generation policy and tests for
   resize/minimize/restore, atomic generation publication/retirement, device
   loss, notification scheduling, input lifecycle and camera-core isolation.
   Establish the D3D11 ImGui backend path while keeping existing view-building
   code unchanged. Validate the DirectComposition update surface can be used as
   a D3D11 render target before integrating it.
2. Implement the DComp presenter owner and HWND discovery; move frame scheduling
   and window/input lifetime to that owner; connect the existing UI frame builder
   and D3D11 ImGui renderer to each committed surface generation. Preserve
   startup toast, settings snapshots, Auto locale synchronization, font rebuild,
   position persistence, notifications, toggle/rebind and cursor restoration.
3. Replace the production DXGI discovery startup with the presenter lifecycle;
   remove old factory/swapchain callback registration and old renderer lifetime
   paths from the production source list. Keep camera initialization and Overlay
   optional-failure boundaries independent.
4. Convert/retire obsolete DXGI-renderer fixtures and register the required
   deterministic presenter contracts. Run focused fixtures, complete `test.cmd`,
   localization/resource/glyph validation, production build and `git diff --check`.
   Review the complete changed-path diff and deliver one canonical
   `STALKER2CameraTweaks.asi` for the user's runtime acceptance.

## Acceptance

- Production build contains no Camera Tweaks DXGI factory/swapchain hooks or
  game-backbuffer renderer path; there is no old backend fallback.
- The settings panel and hidden-panel startup toast/notifications render via the
  independent DComp surface; existing localization/fonts and config mutations
  remain unchanged.
- Delete/Esc, hotkey rebinding, mouse capture/cursor restoration and window
  focus/destruction preserve their established behavior.
- Deterministic coverage exercises multiple surface generations, failed resize
  publication preserving the current generation, minimize/restore, device-loss
  recovery/terminal isolation, notification expiry/redraw, input setup/teardown,
  and proof that presenter failure does not gate camera core.
- No per-frame synchronous GPU wait, native callback, or game-swapchain dependency.
- Complete regression suite and production build pass for the exact delivered
  artifact. No game launch, Git, release or archive operation is performed by
  this task.

## Risks and evidence limits

The DComp prototype used a Direct2D drawing context. Production will use ImGui
draw data with a project-owned D3D11 backend targeting the acquired DComp update
surface; that native interop must be verified before UI integration. Offline
state-machine fixtures cannot prove GPU/driver behavior. The user will perform
the final integrated runtime acceptance with the delivered canonical ASI.

## Implementation status — 2026-09-28

Implemented in the working tree. The production renderer now uses a private
D3D11 device and an independent DirectComposition HWND target/surface; the
existing ImGui view construction, localization/font selection, settings,
notifications, startup hint, hotkey rebinding and input/cursor behavior remain
connected to that presenter. Window discovery and resize/minimize/restore
handling are owned by the composition runtime. Production DXGI factory and
game-swapchain interception, D3D12 renderer integration, and their
continuation-only source paths were removed rather than retained as fallback.

Validation for the exact canonical artifact: `test.cmd` PASS (47/47 harnesses),
18-catalog/182-key localization-resource-font-glyph audit PASS, production
`build.cmd` PASS, and `git diff --check` PASS. Artifact:
`STALKER2CameraTweaks.asi`, 2,706,432 bytes, SHA-256
`E804ECEA901C697E361484818373F7A28403E8B3DF8CBE114365AA5969199C53`.
The production migration has not been launched in-game by the agent. The
prototype's runtime success is feasibility evidence, not integrated production
acceptance; the next step is one user-controlled runtime acceptance run with
the canonical filename.
