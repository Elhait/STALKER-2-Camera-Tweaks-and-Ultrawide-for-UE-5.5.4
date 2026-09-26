# Standalone In-Game Overlay Feasibility Audit

Date: 2026-09-21  
Scope: read-only repository/source/build inspection  
Runtime: not performed  
Production changes: none

## Executive summary

The current ASI has no D3D12, DXGI, D3D11, swapchain, Present, device or
command-queue integration. It does have a small Win32 helper layer that finds
the process window and reads client/display dimensions for aspect calculation.

The existing hook infrastructure is `SafetyHookMid`, used for validated game
camera and lifecycle boundaries. No existing hook is a graphics presentation
hook and no source evidence establishes a swapchain-to-command-queue
relationship.

A standalone overlay remains technically feasible as an optional subsystem,
but the primary POC unknown is the actual STALKER 2 D3D12 presentation path:
the swapchain, its device, and the command queue that is valid for rendering
overlay resources. The common strategy of remembering the last `DIRECT`
queue from `ExecuteCommandLists` is only a heuristic until that association is
observed and validated in this game.

## 1. Current graphics dependencies

### Confirmed source/build facts

- `build.cmd` compiles the production ASI from the listed project sources and
  links only `user32.lib` and `bcrypt.lib`.
- Include paths are limited to `external\safetyhook` and
  `external\spdlog\include` in the supported build command.
- `external\safetyhook` and Zydis provide the current hook/resolver layer.
- `external\spdlog` provides logging.
- No `d3d12.h`, `dxgi.h`, D3D11 headers, DirectX helper library, ImGui,
  Kiero, MinHook or Detours dependency is present in the production source or
  build command.
- Repository search found no use of `Present`, `Present1`, `ResizeBuffers`,
  `ResizeBuffers1`, `ExecuteCommandLists`, `CreateSwapChain*`,
  `IDXGISwapChain`, `ID3D12CommandQueue`, `ID3D12Device` or equivalent
  graphics APIs in `src/`, `tests/` or the build scripts.

Relevant files:

- `build.cmd`
- `build-diagnostic.cmd`
- `src/hooks/hook_set.hpp`
- `external/safetyhook/`
- `external/spdlog/`

### Existing hook infrastructure

`src/hooks/hook_set.hpp` stores `SafetyHookMid` instances for gameplay,
cinematic and dialogue boundaries, plus diagnostic camera hooks when enabled.
The runtime uses validated signature resolution and instruction checks before
installing these hooks. This infrastructure is potentially reusable as a
general code-hook mechanism, but it does not provide a swapchain or graphics
object abstraction.

## 2. Current viewport and window path

`src/platform/win32/window.cpp` implements `FindCurrentProcessWindow()` by
enumerating top-level visible windows and selecting one belonging to the
current process with no owner window.

`src/platform/win32/viewport.cpp` implements `ReadClientViewportAspect()`:

1. try `GetForegroundWindow()`;
2. reject it when it is not owned by the current process;
3. fall back to `FindCurrentProcessWindow()`;
4. read `GetClientRect()` and accept dimensions of at least 16x16;
5. fall back to `EnumDisplaySettings()` when client geometry is unavailable;
6. return an aspect ratio or NaN.

The runtime calls this helper for aspect policy and cinematic aspect decisions.
The returned `HWND` is local to the query. The current code does not retain a
window handle as overlay state and does not expose an HWND accessor to a
graphics subsystem.

Confirmed current access:

- process-owned top-level `HWND`: transiently discoverable;
- client/display width and height: readable;
- runtime aspect ratio: available.

Not established by source:

- game swapchain;
- graphics device;
- command queue;
- back-buffer resource or RTV heap;
- presentation synchronization object.

## 3. Swapchain and presentation feasibility

The current ASI neither creates nor intercepts a swapchain. There is no
existing presentation hook.

Possible bounded investigation points for a future POC include the game's
DXGI/D3D12 object creation path, swapchain vtable methods, or a validated
presentation boundary discovered at runtime. These are investigation options,
not selected implementation points. The repository does not establish which
one STALKER 2 exposes reliably.

Hooking `Present` alone would identify a presentation callback but would not
prove that the overlay has a usable D3D12 command list, descriptor resources,
and command queue for that swapchain. Any future POC must validate the full
resource relationship before rendering.

## 4. D3D12 command queue status

No current source path obtains an `ID3D12CommandQueue` associated with the
game's presentation swapchain.

The commonly suggested approach is:

```text
hook ExecuteCommandLists
→ remember a DIRECT queue
→ use the last observed queue from Present
```

This is not sufficient as a production assumption. A game can use multiple
direct queues, queue wrappers, separate submission paths, or queues that are
valid for rendering but unrelated to the swapchain being presented.

Minimum runtime evidence for a bounded STALKER 2 POC would include:

- swapchain pointer and its `GetDevice()` result;
- queue pointer and queue type;
- queue observations near the same Present sequence;
- command-list/allocator submission relationship;
- back-buffer index and resource state at the render point;
- behavior across resize and swapchain recreation.

Until these observations exist, queue association is `UNKNOWN`, not a reason
to reject the overlay direction.

## 5. Lifecycle and integration risks

A future overlay would need explicit handling for:

- initial device and swapchain creation;
- first usable Present;
- RTV/frame-resource creation;
- `ResizeBuffers` and any swapchain recreation;
- resolution changes;
- windowed, borderless and fullscreen transitions;
- HWND replacement or recreation;
- device/swapchain destruction;
- process shutdown;
- optional subsystem failure and reinitialization.

The existing plugin lifecycle is different:

- `DllMain` starts `InitializeThread` asynchronously;
- `RuntimeState` is process-resident and allocated without a static
  destructor;
- workers are owned by `WorkerLifecycle`;
- controlled shutdown signals and joins workers before hook/resource reset;
- process-terminating detach performs no complex teardown;
- normal dynamic ASI unload is not a supported contract.

This allows an overlay to be added as an optional runtime-owned subsystem, but
it must not add graphics teardown to the loader-lock detach path. Its failure
must only disable overlay rendering and leave the camera/config/hook systems
running.

## 6. Camera-system isolation

The requested isolation is compatible with the current ownership model:

```text
RuntimeState
├── existing camera/config/runtime systems
└── optional overlay subsystem
    ├── presentation integration
    ├── renderer/resources
    ├── input state
    └── UI state
```

The existing camera systems are integrated through the translation-unit-private
`RuntimeState` in `src/plugin/runtime.cpp`. An overlay can be attached to that
owner only as an optional component with an independent availability/failure
state.

For the first rendering POC, no camera API changes are required because the
POC must display a test window only. A later settings UI would need a safe
runtime command/settings boundary; it should not read or mutate camera globals
directly.

Recommended failure semantics:

- graphics discovery failure: mark overlay unavailable;
- resource creation failure: release overlay-owned resources and disable it;
- resize/recreation failure: disable overlay rendering until a fresh valid
  presentation path is observed;
- overlay failure must not reset camera hooks, config, or camera state.

## 7. Settings integration boundary

The current configuration layer already has typed settings for Gameplay,
Cinematics, Dialogue, Diagnostics and Hotkeys in
`src/config/feature_config.hpp`.

`config::LoadFeatureConfig()` and `config::PersistConfigValue()` provide INI
load/persistence. Runtime hotkeys currently update the private runtime atomics
directly and persist values through `PersistConfigValue()` in
`src/plugin/runtime.cpp`.

This means future frontend reuse is conceptually feasible, but the current
source does not expose a public runtime settings command API. An overlay could
not safely reuse the hotkey implementation as-is without either:

- a shared serialized settings-operation boundary; or
- a deliberately bounded overlay-to-runtime adapter.

This is a future settings-integration gap, not part of the rendering POC.

## 8. Minimum next POC boundary

The smallest useful POC should have exactly one functional goal:

> Render a simple test ImGui window over STALKER 2 through a standalone
> optional subsystem.

It should explicitly exclude Gameplay, Cinematics, Dialogue, INI controls and
production camera decisions.

### Potential dependencies to investigate

- D3D12 and DXGI headers/libraries available from the supported Visual Studio
  toolchain;
- a pinned ImGui source/backend integration, if chosen;
- a bounded hook mechanism for the selected presentation boundary.

No dependency should be added until the presentation/queue evidence is known.

### Interception points to investigate

- swapchain creation or first valid Present;
- resize/recreation notification;
- the command submission path needed to associate the presentation swapchain
  with a usable command queue.

No one of these is established as the implementation seam by this audit.

### POC telemetry

The diagnostic POC should record only bounded lifecycle facts, such as:

- discovery status and failure reason;
- swapchain/device/queue identity tokens for one session;
- queue type and presentation sequence;
- back-buffer index/resource state;
- resize/recreation events;
- overlay resource creation/release status;
- fail-closed disable reason.

Avoid per-frame high-rate logging by default.

### Deterministic/offline tests

Before runtime, pure tests can cover:

- state transitions for unavailable, initializing, ready, resizing, failed and
  disabled overlay states;
- idempotent disable/failure behavior;
- no camera-state mutation from overlay failure;
- settings command validation once a shared boundary exists.

They cannot prove the actual STALKER 2 swapchain/queue relationship.

### Required runtime validation

At least one runtime batch is required for:

- first Present and test-window rendering;
- resize and window-mode transitions;
- camera behavior while overlay is unavailable or disabled;
- clean process shutdown with no camera regression.

## Classification

```yaml
overlay_existing_graphics_dependency: NONE
existing_swapchain_access: NONE
existing_command_queue_access: NONE
existing_hwnd_access: PRESENT_TRANSIENT_WIN32_DISCOVERY
presentation_hook_existing: NO
camera_overlay_isolation_feasible: YES_AS_OPTIONAL_SUBSYSTEM
settings_frontend_reuse_feasible: PARTIAL_RUNTIME_BOUNDARY_MISSING
primary_poc_unknown: D3D12_PRESENTATION_PATH_AND_SWAPCHAIN_QUEUE_ASSOCIATION
poc_scope: TEST_ONLY_IMGUI_WINDOW_WITH_FAIL_CLOSED_OPTIONAL_LIFECYCLE
production_changes_made: NO
```

## Final audit status

The repository is ready for a separate bounded presentation/queue discovery
POC plan, but not for direct overlay implementation. No conclusion is made
here about whether the overlay should ultimately be shipped.
