# DComp Visual, Damage, Color, and DPI Contract

Date: 2026-09-28

Scope: production-source inspection and deterministic offline validation. The game was not launched. SDR/HDR appearance remains a runtime question.

## 1. DComp commits

`DrawImGuiSurface` performs `BeginDraw → D3D11 draw/clear → EndDraw → Commit`. Therefore its caller must not immediately issue a second commit unless it first changed composition-tree publication state.

Removed the redundant caller commits after the initial draw in initialize, window rebind, and device recovery. Kept commits after `SetContent`/`SetRoot`, resize replacement publication, rollback, and root detachment: those publish distinct visual-tree changes and are not redundant with the preceding surface commit.

## 2. Damage-region model

The DComp surface retains pixels outside an update. Each frame computes a conservative union of clipped ImGui vertex bounds. The update damage is:

```text
(previous submitted bounds ∪ current submitted bounds) ∩ surface extent
```

That clears disappearing pixels (including the software cursor's previous bounds) and redraws current content inside the same region. All current ImGui commands are replayed under the update scissor. Notifications, popups/tooltips, scrolling, dragging, and cursor changes are naturally included through current/previous draw data rather than separate widget-specific bookkeeping.

Bounds are published only after `EndDraw` and `Commit` succeed. New/replaced/resized generations, locale/font/DPI changes, unsupported callbacks/data, unavailable D3D11.1 regional clear, invalid bounds, and draw failure force a full-surface redraw. Full redraw remains the correctness fallback. If a partial update cannot be rendered, it is retried as a full update before the frame is rejected.

The tracker and fallback policy have deterministic coverage for first generation, moved cursor, disappearing content, hidden panel, notifications, failed commit, and generation replacement. `ComputeImGuiDrawBounds` validates indices/vertices and treats custom draw callbacks as requiring full redraw.

## 3. Color and HDR contract — runtime A/B OPEN

Observed production path:

1. ImGui vertex colors and RGBA font texture are sampled by a shader that multiplies them.
2. The D3D11 target is BGRA8 UNORM. The blend equation is RGB `SRC_ALPHA / INV_SRC_ALPHA`, alpha `ONE / INV_SRC_ALPHA`; each update starts with transparent black.
3. Thus the target stores premultiplied source-over results, matching the DComp surface declaration `DXGI_ALPHA_MODE_PREMULTIPLIED`.
4. No explicit HDR/scRGB color-space transform or SDR-white-level mapping is applied in this renderer. The surface is SDR BGRA8; final appearance is produced by DComp/Windows composition on the active output path.

No theme, surface format, gamma, or blend changes were made for the reported gray appearance. Static inspection cannot distinguish an SDR transfer/blend interpretation issue from HDR SDR-white/compositor mapping or capture-path effects.

### Minimal runtime A/B

Use the same canonical ASI, game scene, resolution, Overlay state, and graphics settings. Capture the Overlay over the same scene once with Windows HDR/Auto-HDR off (SDR), then once with HDR on (do not change in-game theme/settings). Record output mode, Windows HDR state, game window mode, screenshots/capture method, and whether the difference is in solid fills, text edges, or all colors. Do not compare screenshots captured through different capture pipelines. If available, also compare the same screenshot on the physical display. This A/B is required before changing format or color transforms; **color/HDR finding remains OPEN pending user runtime evidence**.

## 4. DPI contract

- The game's process and HWND DPI-awareness are not changed.
- Only the independent presenter worker temporarily uses `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2`, restoring its prior thread context at exit.
- Presenter client geometry and DComp surface dimensions are physical client pixels. ImGui `DisplaySize` matches those pixels and `DisplayFramebufferScale` remains `1,1`; there is no second framebuffer scaling pass.
- Configured font size and style metrics are 96-DPI logical baselines scaled to the HWND's effective DPI. Font atlas and style are rebuilt/rescaled on DPI change; the renderer forces full damage.
- `WM_DPICHANGED` and window-location notifications route through the existing owner-thread geometry refresh. The overlay never applies the game's suggested top-level window rectangle.
- Host `WM_MOUSEMOVE` coordinates are converted with `LogicalToPhysicalPointForPerMonitorDPI(hwnd, ...)`, rather than inferring an unaware-window scale from `GetDpiForWindow` (which reports 96 for an unaware HWND). Initial pointer seeding runs on the PMv2 owner thread; raw relative deltas are unchanged.

Offline tests cover the 96-DPI font baseline at 96/144/192 DPI. Runtime validation should include monitor-DPI transition if the user has a non-100% display; no process-level awareness mutation is permitted.

## 5. Validation boundary

Deterministic tests/build validate damage planning and DPI sizing policy, not the Windows compositor's SDR/HDR appearance. No game launch or runtime A/B was performed in this batch. No theme/color compensation was introduced.
