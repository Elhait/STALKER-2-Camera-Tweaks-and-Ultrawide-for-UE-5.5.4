# 2.0.1 → 2.1 Production Release Delta

**Date:** 2026-09-29  
**Scope:** read-only release-delta audit. No source, test, build, package, Git, or release metadata was changed for this report.

## Comparison basis and summary

The local source baseline is commit `04fcae2dcc99cef4448be1942ce6077d3d4b8aec` (`prepare: v2.0.1`), whose parent is the `v2.0.0` release point. There is no local `v2.0.1` tag, so this commit is the best available repository anchor for the user-designated last public 2.0.1 release. The target is the current production source/build tree.

The working tree contains the unreleased 2.1 candidate changes; they were preserved. The root production artifact is `STALKER2CameraTweaks.asi`, SHA-256 `DF02F40BDEDA42666D2395F9558FA53F2996AFBAB8BFA54D1BE32995068E57A1`. The supplied runtime camera log identifies the same hash. The current source diff spans 67 paths (about 6,080 insertions and 5,136 deletions, including tests, docs and research records).

The main product change is an Overlay presentation architecture replacement: game DXGI/swapchain interception and game-resource ownership are removed from the production path; the Overlay now uses its own D3D11 device and DirectComposition surfaces. Camera modes and configuration remain unchanged. A separate Gameplay correction maps validated post-cinematic native interpolation into HorPlus output space without changing the native `GameplayBaseline`.

### Evidence labels

- **Runtime-confirmed:** supported by supplied runtime logs or user runtime acceptance. This is limited to the tested artifact/configuration; it is not a universal hardware or wrapper-compatibility claim.
- **Offline-tested only:** source contracts/regression fixtures/build checks support the behavior, but the specific runtime branch or environment was not demonstrated.
- **OPEN:** runtime evidence is insufficient or the behavior is explicitly awaiting an A/B test.

## Full categorized delta

| Category | 2.0.1 → current 2.1 | Change type | Production evidence and validation status |
|---|---|---|---|
| **Overlay architecture / presentation** | 2.0.1 observed/intercepted DXGI factory and game swapchain methods, with rendering tied to game presentation. 2.1 draws the existing ImGui UI into private D3D11-rendered DirectComposition surfaces attached to an independent composition target/visual. | Architectural; compatibility | [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`build.cmd`](../../build.cmd). Exact-hash runtime log reports `dxgi_factory_hooks=0 swapchain_hooks=0` and `OVERLAY_FIRST_FRAME`: **runtime-confirmed**. |
| **Frame Generation compatibility** | The Overlay no longer participates in the game’s mutable `Present`/`ResizeBuffers` forwarding graph, removing its contribution to the recursive forwarding cycle observed during tested swapchain transitions. This is not a blanket guarantee for every driver, FG mod, injector, or wrapper combination. | Architectural; compatibility | Production source/build path contains no game factory/Present/Resize hook backend; runtime marker confirms zero factory/swapchain hooks. User confirmed native FG toggling works with the integrated DComp Overlay; the prototype also passed FG OFF→ON→OFF→ON. **Runtime-confirmed in tested setups; not universal compatibility certification.** |
| **Window discovery, surfaces, minimize/restore, device recovery** | Target discovery is independent of swapchain discovery. The presenter selects a process-owned game HWND and responds to window events. It owns surface generations, geometry transitions, minimize/restore and bounded recovery of its own composition/D3D11 resources. It does not own the game’s backbuffers, device, queue, or swapchain. | Architectural; compatibility | [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`composition_presenter_state_harness.cpp`](../../tests/overlay/composition_presenter_state_harness.cpp). State-machine contracts are **offline-tested**. The DComp prototype survived resolution/window-mode changes at 3440×1440, 2560×1440 and 5120×1440; that proves prototype feasibility, not all integrated production recovery paths. Current production log shows surface generation 1, not a full production resize/device-loss cycle. |
| **Overlay lifecycle / startup toast and notifications** | Notifications are independent of settings-panel visibility and game Present. Auto-language startup hints wait for accepted game-language synchronization; their visible lifetime starts only after the notification is actually submitted in a committed DComp frame. A first ImGui sizing frame alone does not start the lifetime. | User-visible fix; architectural | [`renderer_state.cpp`](../../src/overlay/renderer_state.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`composition_presenter_state_harness.cpp`](../../tests/overlay/composition_presenter_state_harness.cpp). Current logs record `game=uk`, hint creation, lifetime start and `DRAWN_AND_COMMITTED`: **runtime-confirmed** for the supplied run. |
| **Input / cursor / focus** | Input no longer depends on game swapchain callbacks. The game HWND is the event boundary; a bounded event bridge forwards mouse, keyboard and focus events to one ImGui/render owner thread. Delete/Esc behavior is retained. Closing or losing ownership resets input and returns it to the game. | User-visible fix; architectural | [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`input_state.cpp`](../../src/overlay/input_state.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`input_state_harness.cpp`](../../tests/overlay/input_state_harness.cpp). User confirmed mouse/keyboard/focus functionality; current log records mouse position application: **runtime-confirmed** for functionality. High-refresh smoothness has no measured benchmark or separate quantitative acceptance result. |
| **Rendering / pacing / efficiency** | The prior 33 ms interactive timer cadence is replaced by semantic presenter states: idle, notification animation, interactive UI and event-driven redraw. Notification animation and coalesced dirty work use the compositor clock when available; idle has no repeating render source. Damage tracking uses conservative previous/current visible content bounds with full redraw as correctness fallback. Redundant commits that did not publish composition-tree state were removed. | Performance/efficiency; responsiveness | [`presenter_runtime_policy.hpp`](../../src/overlay/presenter_runtime_policy.hpp), [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`composition_presenter_state_harness.cpp`](../../tests/overlay/composition_presenter_state_harness.cpp). **Offline-tested** for policy/damage behavior. No FPS, latency, GPU-time or CPU-time benchmark is claimed. |
| **DPI / scaling / localization / fonts** | The presenter uses physical client pixels for its DComp surface and ImGui display size, with `DisplayFramebufferScale=1`. Its owner thread temporarily uses per-monitor-v2 DPI context; font/style metrics scale from a 96-DPI baseline and refresh on DPI changes. Host mouse coordinates are converted into the physical coordinate space. Auto-language startup presentation waits for game-language detection. | User-visible fix; compatibility | [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`game_language_reader.cpp`](../../src/overlay/game_language_reader.cpp), [`localization_harness.cpp`](../../tests/overlay/localization_harness.cpp). Current runtime log records DPI 120 / scale 1.25 and Auto locale `uk`: **partially runtime-confirmed**. Additional monitor-DPI transitions are offline-tested, not all runtime-validated. |
| **Color / HDR** | Surface contract is BGRA8 UNORM with premultiplied alpha, transparent-black clear and source-alpha blending. Theme colors, surface format and gamma were not changed as a speculative compensation. The renderer applies no explicit HDR/scRGB transform or SDR-white-level mapping. | Visual correctness / internal | [`renderer_runtime.cpp`](../../src/overlay/renderer_runtime.cpp), [`imgui_d3d11_renderer.cpp`](../../src/overlay/imgui_d3d11_renderer.cpp), [DComp visual/DPI audit](DCOMP_VISUAL_DPI_COLOR_AUDIT_2026-09-28.md). **HDR/SDR visual parity remains OPEN.** User feedback was that appearance seemed improved but was not conclusive. |
| **Gameplay / HorPlus** | HorPlus formula, modes and `GameplayBaseline` semantics remain. New behavior: while post-cinematic recovery is still active, a sample proven to be native interpolation from the validated gameplay source is mapped continuously between the cached transformed cinematic endpoint and the HorPlus form of the native EXIT target. It cannot publish a transitional value into `GameplayBaseline`. Ambiguous or invalid samples remain pass-through. This supplements—not replaces—the 2.0.1 immediate-resume-on-already-native-first-sample fix. | User-visible fix; camera lifecycle correctness | [`gameplay_state.cpp`](../../src/gameplay/gameplay_state.cpp), [`horplus_gameplay.cpp`](../../src/gameplay/horplus_gameplay.cpp), [`runtime.cpp`](../../src/plugin/runtime.cpp), [`horplus_post_exit_recovery_harness.cpp`](../../tests/cinematics/horplus_post_exit_recovery_harness.cpp). New long interpolation branch is **offline-tested only**. Exact-hash runtime evidence confirms 3:1 / FOV 140, validated native recovery and subsequent ADS without a visible jerk, but recovery in that run was nearly immediate and did not exercise the long transitional sequence. |
| **Cinematics** | No new cinematic mode/API or baseline policy. The existing cinematic camera behavior remains; the new recovery mapping is Gameplay-coordinator-specific. | No separate product behavior change | Existing production camera path and regression coverage remain. The current exact-hash runtime test exercised cinematic exit at 3:1 / FOV 140; the long transitional mapping itself remains offline-only. |
| **Dialogue** | Adaptive/Reduced/Disabled behavior and Dialogue depart-then-return recovery semantics are not replaced by the Gameplay interpolation rule. | Preserved behavior; regression guard | Dialogue production state code has no functional change in this delta; the new interpolation policy is in Gameplay recovery. No new Dialogue-specific runtime claim is made. |
| **Diagnostics / logging** | The dedicated production Overlay log now records DComp attachment, zero game DXGI hook state, compositor clock, DPI, startup hint, input ownership and related presenter events. The general startup journal predates this delta; it is not a new 2.1 feature. | Diagnostic/internal | [`composition_runtime.cpp`](../../src/overlay/composition_runtime.cpp), [`startup_journal.hpp`](../../src/diagnostics/startup_journal.hpp). Current supplied runtime logs include these events: **runtime-confirmed**. |
| **Configuration / compatibility** | No delta found in config source, runtime settings API, INI schema/defaults, camera modes or locale catalogs. No filename/load-order requirement was introduced. | No user-facing configuration change | Read-only source diff of config/runtime settings/locales: no changes. Current source retains 18 locale catalogs and 182 canonical keys; audits cover the catalogs but do not prove every glyph/RTL scenario at runtime. |
| **Tests / safety / architecture contracts** | DComp-specific regression coverage replaces the old DXGI/D3D12 interception and renderer-lifetime fixtures. Contracts now cover composition generations, publication/retirement, minimize/restore, recovery, input ownership, notification visibility/lifetime, event/redraw policy and damage. Gameplay adds the post-EXIT interpolation regression. | Internal safety / regression change | [`test.cmd`](../../test.cmd), [`composition_presenter_state_harness.cpp`](../../tests/overlay/composition_presenter_state_harness.cpp), [`horplus_post_exit_recovery_harness.cpp`](../../tests/cinematics/horplus_post_exit_recovery_harness.cpp). Last recorded validation: 47/47 harnesses, localization/resource/glyph audits, production build and `git diff --check` passed. These are **offline results**, not runtime proof. |
| **Removed / retired legacy behavior** | Production no longer builds Camera Tweaks DXGI factory observation/interception, game `Present`/`Present1`, `ResizeBuffers`/`ResizeBuffers1` hooks, game backbuffer/device/queue ownership, or the continuation/resize-nesting registry. There is no old DXGI/D3D12 Overlay fallback. `dxgi.lib` remains for D3D11/DirectComposition interop interfaces, not for game swapchain interception. | Architectural removal | Deleted production sources include `discovery_*`, `dxgi_*`, old `overlay_lifecycle.*` and `renderer_wait_policy.hpp`; current production source list is in [`build.cmd`](../../build.cmd). Runtime marker reports zero factory/swapchain hooks: **source/build and runtime confirmed**. |

## Important changes not to misattribute to 2.1

- The core HorPlus transform, camera modes, configuration API/defaults, locale catalog content, Dialogue policies and production startup journal are not new 2.1 features.
- The 2.0.1 baseline already included Overlay failure isolation, DXGI table/bootstrap safety, delayed Overlay activation, startup journaling, and the first Gameplay recovery branch that resumed immediately when the first validated post-EXIT sample was already native. The new 2.1 Gameplay delta is the continuous mapping of validated transitional interpolation samples.
- DComp prototype source, WinDbg capture plans and causal research records supported the design; they are not runtime product features and should not be listed as shipped features.
- “No game DXGI hooks” means Camera Tweaks has left that presentation-hook graph. It does not certify every third-party graphics stack or all FG implementations.

## Documentation discrepancies noticed

These were not edited in this read-only task:

1. [`docs/SAFETY_INVARIANTS.md`](../../docs/SAFETY_INVARIANTS.md) still says transitional samples pass through unchanged. That is now true for ambiguous/invalid samples, but not for validated same-source native interpolation, which is deliberately mapped into HorPlus output space.
2. The current validation snapshot near the top of [`TESTING_AND_RESEARCH.md`](../../TESTING_AND_RESEARCH.md) still identifies the earlier `E804…` artifact and says integrated runtime acceptance is pending. The current root ASI and supplied exact-hash runtime log identify `DF02…` instead. The report’s delta is based on production source and the supplied current-hash evidence, not that stale snapshot.

## Draft: Nexus changelog

**2.1**

- Rebuilt the Overlay on an independent DirectComposition renderer for improved compatibility with Frame Generation and graphics wrappers.
- Fixed Overlay startup notifications, mouse/keyboard input and focus handling.
- Improved Overlay pacing, DPI scaling and automatic game-language startup behavior.
- Smoothed the post-cinematic Gameplay HorPlus transition for validated native FOV recovery.
- No camera settings or INI configuration changes.

## Draft: GitHub release notes

### STALKER 2 Camera Tweaks 2.1

#### Overlay rebuilt for independent presentation

The Overlay now uses a private D3D11 renderer and DirectComposition surfaces instead of intercepting the game’s DXGI factory and swapchain presentation methods. It no longer owns game backbuffers or participates in the game’s `Present`/`ResizeBuffers` forwarding chain. This removes Camera Tweaks Overlay from the mutable swapchain hook path involved in recursive resize forwarding during tested Frame Generation transitions.

The Overlay discovers the game window independently and manages its own surface generations, redraws and recovery lifecycle. The existing ImGui interface, settings, notifications, localization and camera runtime remain. The old DXGI/D3D12 Overlay backend is not retained as a fallback.

#### Lifecycle, input and rendering

- Startup notifications wait for automatic game-language detection and start their visible lifetime only after a committed frame containing the notification.
- Mouse, keyboard and focus events use a bounded game-window event bridge and a single ImGui/render owner thread.
- Minimize/restore, resize, surface replacement and presenter recovery are handled by the independent composition lifecycle.
- Idle rendering is demand-driven; notification animation uses the compositor clock where available. Damage tracking and removal of redundant composition commits reduce unnecessary work while keeping full-redraw fallbacks for correctness.
- DPI handling uses physical client pixels for the composition surface and scales font/style metrics from the effective window DPI.

#### Gameplay camera

Post-cinematic recovery now maps validated native gameplay interpolation into HorPlus output space while preserving the native `GameplayBaseline`. Ambiguous samples remain untransformed, and Dialogue recovery semantics remain separate.

#### Validation notes

Native Frame Generation toggling was runtime-tested with the DComp Overlay in the tested setup. Deterministic regression and localization/resource audits passed. The new long transitional-FOV mapping has offline regression coverage; the supplied exact-hash runtime session exercised 3:1 / FOV 140 with rapid native recovery, not that long interpolation branch.

HDR/SDR visual parity remains open and is not claimed as fixed. No camera configuration or INI schema change is included.

## Draft: “What changed in 2.1”

- Replaced game-swapchain Overlay hooks with an independent D3D11 + DirectComposition presenter.
- Removed Overlay ownership of game backbuffers and its participation in `Present`/`ResizeBuffers`.
- Improved startup toast, mouse/keyboard/focus lifecycle, DPI scaling and demand-driven rendering.
- Added continuous HorPlus-space handling for validated post-cinematic FOV interpolation.
- Kept camera modes and configuration unchanged; HDR/SDR visual parity remains under evaluation.
