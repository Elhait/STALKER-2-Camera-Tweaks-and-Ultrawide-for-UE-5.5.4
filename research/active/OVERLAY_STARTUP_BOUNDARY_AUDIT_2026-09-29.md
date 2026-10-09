# Overlay startup boundary audit — 2026-09-29

This is a source-level ordering record for the ordinary S12 production path, not runtime proof of an NVIDIA API conflict. S13 (`OVERLAY_CONTENT_LIFETIME_EXPERIMENT`) conditionally changes content binding and is not part of that path. S13's NVIDIA criterion was inconclusive because its HUD never appeared.

## Ordered startup path

1. `src/plugin/dll_entry.cpp:5-28`: `DllMain` saves the module handle and starts `InitializeThread`. No overlay graphics operation runs under loader lock.
2. `src/plugin/runtime.cpp:5135-5153` and `src/plugin/optional_overlay_startup.hpp:9-40`: the initialization thread starts the optional Overlay worker, initializes Camera Runtime/config/hooks, then signals camera-core readiness. Camera work is retained in every proposed variant.
3. `src/overlay/composition_runtime.cpp:878-924`: the Overlay owner thread establishes PMv2 thread DPI, message queue, log, idle-capable compositor-clock worker, and out-of-context window event hooks. Before camera-core readiness it cannot initialize the presenter.
4. `src/overlay/composition_runtime.cpp:236-260,798-847`: after readiness the owner thread enumerates visible top-level process windows, chooses the game HWND, reads client extent and DPI, and enters the Initialize route. Discovery reads but does not change HWND style/WndProc.
5. `src/overlay/renderer_runtime.cpp:636-660,757-802`: on that same owner thread, the renderer records the HWND, applies DPI state, creates a private hardware D3D11 device and an `IDCompositionDesktopDevice` from its DXGI device. These objects persist in idle and do not directly mutate the game HWND.
6. `src/overlay/renderer_runtime.cpp:804-849`: `CreateTargetForHwnd(game HWND, TRUE)` is the first DComp call explicitly bound to the selected HWND. Only on failure does production try `FALSE`. `CreateVisual` follows; both objects persist. Creation alone is distinct from adding a root or committing content.
7. `src/overlay/renderer_runtime.cpp:966-1055`: the ImGui context, locale/fonts, Win32 backend and D3D11 UI resources initialize after target/visual creation. This does not install Camera Tweaks' WndProc.
8. `src/overlay/renderer_runtime.cpp:661-704,851-873,1159-1324`: production creates the premultiplied BGRA8 surface, queues `SetContent(surface)` and `SetRoot(visual)`, then performs a full transparent `BeginDraw`/clear/`EndDraw` and the first `Commit`. Those bindings remain in idle; a successful Commit is an API result, not proof of physical visibility.
9. `src/overlay/renderer_runtime.cpp:705-754,1853-1915,2137-2149`: generation 1 is published, the first UI frame is attempted, and startup-notification creation may wait for Auto locale. Auto-locale synchronization posts a message to the game HWND (`src/overlay/game_language_reader.cpp:181-184`). Later startup geometry can replace generation 1.
10. Only after `Renderer::Initialize` returns success does `src/overlay/composition_runtime.cpp:847-852` call `InstallInputHook`, which replaces `GWLP_WNDPROC` at `src/overlay/composition_runtime.cpp:688-719`. This is the first direct game-HWND procedure mutation by the Overlay and remains active in idle.
11. `src/overlay/renderer_runtime.cpp:1945-1985,2673-2689` builds and submits the notification; `src/overlay/composition_runtime.cpp:745-788` drives active animation with compositor-clock demand and returns to idle when no UI work remains. Toast appearance therefore occurs after several potentially persistent graphics operations.

## Persistent side effects and isolation

The worker, window-event hooks and dormant clock thread can outlive toast expiry without rendering. The private graphics devices, HWND target, visual, surface/root/content binding, ImGui resources and installed WndProc also persist in the ordinary idle path. `CreateTargetForHwnd(TRUE)`, `SetRoot`, `SetContent`, the initial clear/Commit, and WndProc installation are separate boundaries; no source evidence identifies which changes NVIDIA's physical behavior. S13 only exercised alternate content lifetime without an observable NVIDIA HUD.

An intentional diagnostic terminal state is required for an intermediate boundary. Returning `InitializeImpl=false`, calling `Disable`, or allowing normal teardown would remove the very resources under test. Likewise, a WndProc-only probe would reorder production startup because WndProc installation normally follows initial surface publication.

## Adaptive boundary matrix

Use the unchanged S12 artifact as the full-presentation reference and at most four diagnostic cuts selected from: (1) core only; (2) worker/selection only; (3) private devices plus topmost HWND target and visual, but no root/surface/commit/WndProc; (4) initial surface/root/content/transparent Commit without WndProc. Compare physically observed HUD behavior, with the same HUD-on-before-launch setup. If the HUD is absent even in core-only, the Overlay boundary is not established. Results need not be monotonic, so a simple binary-search conclusion is conditional on actual observations.

The highest-information first cut is (3), **target-only**: it distinguishes persistent HWND-bound target/visual creation from later publication and WndProc/toast work. A HUD change at this cut would call for a device-only split; normal HUD behavior would move the next cut after target creation. Neither outcome by itself attributes the effect to one API or proves the cause of Bug B.
