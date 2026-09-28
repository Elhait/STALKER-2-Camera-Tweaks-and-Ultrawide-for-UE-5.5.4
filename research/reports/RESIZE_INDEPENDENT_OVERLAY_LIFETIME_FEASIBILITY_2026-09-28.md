# Resize-Independent Overlay Renderer Lifetime — Feasibility Report

Date: 2026-09-28

Scope: static source and public-source review only. No production code was changed, no build/tests were run, and the game was not launched.

## Decision

**A generic, performant design that removes Camera Tweaks from both `ResizeBuffers` and `ResizeBuffers1` interception is not established and is not safe under the current constraints.** The blocker is not that every renderer object must be recreated on resize. It is the narrower, unavoidable lifetime of the swapchain backbuffer resources while Overlay GPU work may still reference them:

1. `Renderer::BuildResources` obtains and retains every buffer with `IDXGISwapChain::GetBuffer`.
2. Overlay command lists refer to those resources through barriers and RTV descriptors. D3D12 command-list recording does not AddRef objects passed to its methods, so the application must keep each resource alive until the GPU has finished using it.
3. DXGI requires all direct and indirect references to swapchain backbuffers to be released before `ResizeBuffers` can succeed.
4. With no pre-resize notification/interception, a resize may happen at any point between Presents. Retaining the references is safe for GPU lifetime but can prevent that resize. Releasing them before GPU completion is unsafe. Waiting for completion before dropping them after each rendered Present avoids that race only by synchronously serializing the CPU with the GPU on every Overlay-rendered frame, which this task explicitly rejects.

Thus the requested combination — arbitrary foreign-owned resize timing, active D3D12 Overlay rendering, no resize notification, no per-render completion wait, and guaranteed native resize success — has no supported general solution in the reviewed API/source evidence. This is a capability boundary, not a claim that no provider-specific arrangement can work.

## 1. Current resource-lifetime graph

Source of truth: `src/overlay/renderer_runtime.cpp`, `renderer_runtime.hpp`, `discovery_runtime.cpp`, and the vendored `external/imgui/backends/imgui_impl_dx12.cpp`.

```text
validated swapchain/device/queue association
  └─ Renderer::InitializeImpl
      ├─ AddRef swapchain, device, queue; retain canonical swapchain identity
      ├─ create RTV heap sized to buffer count; create SRV heap
      ├─ create renderer fence/event, one allocator per buffer, command list
      ├─ BuildResources
      │   ├─ GetDesc; require matching buffer count
      │   ├─ GetBuffer(i) → retain each ID3D12Resource in backbuffers[]
      │   └─ create RTV descriptor for each retained resource
      └─ BuildImGui
          ├─ retain ImGui context, Win32 backend and localization/UI state
          ├─ DX12 backend stores device/queue/heap pointers under Renderer ownership
          ├─ create font texture/SRV, PSO/root signature, per-frame upload buffers
          └─ texture upload uses queue/fence completion before publishing font resource

Present (only when visible or a notification needs drawing)
  ├─ GetCurrentBackBufferIndex
  ├─ wait only if this frame allocator's previous fence has not completed
  ├─ record barriers and draw commands referring to backbuffers[index]/RTV
  ├─ execute on retained queue; signal renderer fence
  └─ keep backbuffer COM references and descriptors for later Presents

ResizeBuffers / ResizeBuffers1 interception (outermost call)
  ├─ BeforeResize: mark Resizing; WaitForGpu; invalidate ImGui DX12 device objects
  ├─ ReleaseBackbuffers: Release every retained GetBuffer resource
  ├─ call saved native/wrapper continuation exactly once with original args
  ├─ OnResizeResult: on success inspect desc and (for ResizeBuffers1) queue list
  └─ next validated Present: reacquire buffers/RTVs; recreate ImGui device objects
```

### Resource classes and why their lifetimes differ

| Resource/state | Current lifetime and resize relevance |
|---|---|
| `IDXGISwapChain` reference + canonical identity | Retained across Presents to identify the renderer owner. By itself it is not the outstanding backbuffer reference DXGI requires released for buffer resize. It does retain an old wrapper/proxy object across replacement until renderer teardown. |
| `ID3D12Resource* backbuffers_` | Strong COM references from `GetBuffer`, kept until `BeforeResize`/shutdown. These are the direct, confirmed blocker: they must be gone for `ResizeBuffers` to succeed. They are also the resources named in recorded D3D12 command lists. |
| RTV heap/descriptors | Heap is device-owned and sized to buffer count; descriptor slots encode resource views and are used for draw submission. The heap itself is not a swapchain-buffer COM reference, but descriptors cannot be used after their underlying resource has been destroyed/replaced. Current code invalidates the DX12 backend and overwrites views after resize. |
| Frame allocators + command list | Device-owned, one allocator per backbuffer. They may outlive a resize if device/queue/frame-slot assumptions still hold, but an allocator cannot be reset until all work recorded with it completed. Their current count is tied to BufferCount. The command list does not AddRef resources passed to its methods. |
| Fence + event + `gpuWorkSubmitted_` | Device/queue synchronization state, not buffer-generation state. The fence is used both for per-slot reuse and for a full `WaitForGpu` before resize/replacement/font teardown. A signaled fence is the completion evidence needed before releasing/reusing GPU-visible resources. |
| Device + queue | Retained by Renderer; ImGui backend stores the pointers but the project-owned Renderer owns their COM lifetime. They can persist across same-device resize. A proxy/queue change requires revalidation; do not infer an FG provider's internal queue from a public `ResizeBuffers1` argument list. |
| ImGui context, UI/settings/localization/notifications, Win32 backend | Primarily CPU/platform state and not inherently tied to a backbuffer generation. These need not be recreated merely because buffer width/height changes, assuming HWND/device remain compatible. Visibility is not the render-work boundary: queued startup toast/notifications can render while the settings panel is hidden. |
| ImGui DX12 backend objects | Font texture/SRV, pipeline/root signature, and vertex/index upload buffers are not the swapchain's backbuffer references. They still have GPU completion requirements. RTV format, device, queue, frame-resource count and descriptor heap are compatibility inputs; changes may require partial or full backend rebuild. Current `InvalidateDeviceObjects` releases font and upload resources as well as pipeline state, so it is broader than just clearing backbuffer views. |
| Input/window state | Input capture and WndProc state are distinct from D3D12 buffer lifetime. Do not deactivate input solely because settings UI is hidden; preserve current validated activation contract. |

### What `BeforeResize` currently guarantees

`discovery_runtime.cpp` invokes it only for the outermost matching resize call. `Renderer::BeforeResize` validates ownership, enters `Resizing`, waits for previously submitted Overlay GPU work (bounded; checks actual fence completion), invalidates ImGui DX12 device objects, and releases all retained backbuffer resources before forwarding to the captured continuation. Nested `ResizeBuffers1 → ResizeBuffers` is handled by per-thread nesting; only the outer call performs teardown/recovery decisions. On success, rebuild is deferred to a subsequent validated Present.

Removing that callback removes the only current point that is both (a) before the foreign native/proxy resize operation and (b) able to release Camera Tweaks' retained backbuffer references after proving its GPU work is complete. A post-resize callback cannot substitute for this precondition.

## 2. Lifetime model alternatives

### A. Present-scoped backbuffer ownership — safe only with a rejected stall

Acquire `GetBuffer` only for the current draw, create/use its RTV, submit, signal a fence, then release the resource before returning from Present. Because D3D12 command-list APIs hold no references to objects they name and application-owned resources must survive until GPU execution completes, releasing immediately after `ExecuteCommandLists` is not safe. Waiting for the fence before releasing is safe but turns every Overlay-rendered Present into a CPU/GPU synchronization point. Waiting after the draw also defeats normal pipelining and can stall on the frame that draws the panel, startup toast or notification. This violates the explicit performance contract.

If the Overlay is idle and does not render, it could release resources after prior work has completed. That helps only during idle stretches; it does not guarantee release before an arbitrary resize while the panel or a notification is rendering.

### B. Fence-retired generations — cannot meet DXGI's pre-call condition

Keeping an old generation alive until its fence completes is the correct GPU-lifetime strategy, but the retained `ID3D12Resource` references remain outstanding during that time. DXGI's contract requires all direct and indirect backbuffer references released *before* the resize succeeds. An asynchronous retirement queue therefore creates one of two outcomes:

- keep the generation until its fence signals: safe for D3D12, but resize may fail because references remain;
- release it before completion: no longer blocks resize, but violates D3D12 resource lifetime.

An unbounded/wait-free retirement scheme cannot guarantee both. A pre-resize notification can wait for the relevant generation and release it, which is the existing architecture boundary; absent that signal, the only deterministic substitute is to wait before each final release.

### C. Lazy post-transition rebuild — useful recovery, not a resize precondition

The next valid Present can inspect `GetDesc`, current index, device/queue association where observable, and reacquire buffers/RTVs when the target is still supported. Buffer count/format/device/HWND changes may require different subsets of state to be rebuilt. This is useful after a successful resize or replacement, but cannot make a resize succeed while old backbuffer references remain. Dimensions or buffer identity also do not expose every proxy's internal queue changes. Checking `GetBuffer` identities on every Present would add repeated API/COM work and still not solve the in-flight release race.

The current delayed rebuild after `OnResizeResult` is already a version of this post-transition recovery, with the missing pre-transition ownership released first.

### D. Stateless/minimally persistent renderer — reduces rebuild scope, not the hard blocker

The renderer can be split conceptually into:

- **persistent per device/UI:** ImGui context, localization, settings/notifications, SRV/font resources, pipeline state when format/device are unchanged, queue/device ownership, synchronization objects;
- **frame-slot state:** allocators/upload buffers indexed by a fixed/capacity policy and their fence values;
- **swapchain generation:** the retained backbuffer references and RTV view contents; plus format/count dependent compatibility metadata.

That split could avoid the current broad `InvalidateDeviceObjects`/`CreateDeviceObjects` cycle for a same-device, same-format resize and could reduce resize-time CPU/GPU work. A full per-frame backend recreate is unnecessary. However, retaining the swapchain-generation COM references across Presents still blocks resize. Removing those references without the fence-completion proof still risks GPU use-after-free. This is a worthwhile bounded optimization only if later runtime measurements show resize rebuild cost matters; it does not justify removing Resize interception.

### E. Alternative attachment point — none generic within the current contract

`WM_SIZE` is a useful application-level signal for ordinary window resizing, but it is not a universal pre-call contract for arbitrary `ResizeBuffers`/`ResizeBuffers1`, fullscreen/output transitions, FG on/off replacement, proxy-internal changes, or resizes from other threads. Window subclassing would also introduce another shared hook/ordering/lifetime surface without proving it precedes each authoritative proxy resize.

An official engine render extension, provider-owned overlay API, or cooperative callback from the authoritative proxy could offer a usable lifecycle contract, but none is established for arbitrary installed stacks. Moving into undocumented game internals or targeting one FG provider is explicitly out of scope. Rendering through Present remains the available generic attachment point, but Present alone supplies no pre-resize ownership notification.

## 3. Production precedent

### ReShade — source-observed, not resize-independent

The current ReShade DXGI wrapper source calls its reset path before forwarding `_orig->ResizeBuffers`/`ResizeBuffers1`, then reinitializes on success. Its public add-on event documentation describes `destroy_swapchain(..., resize=true)` as occurring before both resize methods and initialization after the buffer resize. Its D3D12 runtime also owns backbuffer resources/RTVs and waits/resets/releases them during runtime reset. This is a strong precedent for **coordinated pre-resize reset + post-resize rebuild**, not for avoiding resize lifecycle ownership.

### Dear ImGui D3D12 example/backend — source-observed reference lifecycle

The official Win32/DX12 example's `WM_SIZE` path calls `CleanupRenderTarget` before `ResizeBuffers`, then reacquires buffers and recreates RTVs. Cleanup waits for pending GPU operations. The DX12 backend separately retains font and per-frame vertex/index resources; those are renderer resources, not replacement swapchain buffers. The example supports the resource split in option D, but its resize path still explicitly releases backbuffers before the native call.

### OptiScaler — narrow configuration precedent only

The current OptiScaler configuration exposes `PreserveSwapChain` and `SkipResizeBuffers`, with the latter described as conditional on old/new descriptions matching; it also exposes separate buffer-state/index handling options and warns in its comments that combinations may fix or cause crashes. This is a source-observed, provider-specific way to avoid certain resize calls in an FG swapchain path. It does not establish a general third-party overlay contract, does not show that arbitrary active backbuffer references can be ignored, and is not transferable as a Camera Tweaks bypass.

## 4. Resize-independent feasibility and performance

| Requirement | Finding |
|---|---|
| Keep ResizeBuffers/ResizeBuffers1 completely foreign-owned | Not safely established while Overlay holds backbuffer resources between Presents. |
| Never block a correct resize due to Camera Tweaks refs | Requires releasing all backbuffer references before resize; no generic pre-resize signal exists on Present alone. |
| Never release GPU-visible backbuffer early | Requires fence completion before resource release. |
| Avoid per-Overlay-frame CPU/GPU synchronization | Compatible with persistent resources only while they remain retained; incompatible with guaranteed arbitrary resize success without a pre-resize event. |
| Rebuild lazily after resize | Useful only after the foreign resize has already succeeded; insufficient as the sole lifecycle. |
| Preserve wrapper/proxy authority | No system-DXGI bypass or presumed terminal swapchain solves this; no supported generic proxy callback was found. |

The conflict is therefore structural: keep resource ownership and risk resize rejection, or drop ownership early and risk GPU lifetime corruption, or synchronously wait per rendered frame. The present request forbids all three outcomes as an acceptable repair.

## 5. Minimum production diff

**No production diff is recommended for this request.** Do not remove the `ResizeBuffers`/`ResizeBuffers1` hooks or their native forwarding. The hooks' current pre-call release/fence guarantee is still necessary for correctness under the renderer's persistent backbuffer model. Do not add hook-chain workarounds or provider-specific behavior.

If separately authorized later, a narrower performance-oriented refactor could split same-device/same-format resize recovery so it recreates only swapchain-generation resources and resizes frame-slot capacity only when needed, while retaining pre-resize ownership release and native forwarding. That would need measured resize cost and its own plan; it would not be a Resize-independent design.

## 6. Regression contract for any future proposal

Any future implementation claiming resize independence must have deterministic contracts for:

1. **Arbitrary native resize timing:** invoke resize after Overlay submission but before the next Present. Prove no Camera Tweaks-owned direct/indirect backbuffer references remain and no submitted D3D12 command list references a resource already released. Native arguments and exact HRESULT must pass through unchanged.
2. **Completion ordering:** test fence complete and incomplete cases. An incomplete generation may not be freed; the contract must also show how a foreign resize succeeds without retaining it or waiting synchronously on each rendered frame. If it cannot, reject the design.
3. **Generation transition:** after resize, reacquire and validate current buffer count/format/index/resource identities on the first safe Present; never render against an old generation. Avoid unconditional per-Present `GetBuffer`/resource rebuild.
4. **ResizeBuffers1:** changed/multiple passed queues or unavailable evidence must not be interpreted as proof of the proxy's internal presentation queue. A stale/unknown association must skip/disable Overlay while preserving native call arguments/result.
5. **Proxy lifecycle:** FG OFF→ON and ON→OFF replacement, same-HWND swapchain replacement, auxiliary swapchain, foreign Present re-hook, and another Present thread. The proxy presented to Camera Tweaks remains authoritative; no terminal-native assumption.
6. **UI behavior:** visible settings panel, hidden panel with startup toast, hidden panel with notification, then idle. Required UI/notification rendering must remain functional without using visibility as the resource-activation boundary.
7. **FS-01–FS-05:** retain the existing repaired replacement, pending target, resize nesting/ownership, failure, and exception-boundary contracts as currently registered; this report does not reopen or redefine them.
8. **Performance:** assert no `WaitForSingleObject`/queue-idle per rendered frame, no per-frame renderer/backend reinitialization, and no unconditional `GetBuffer`/descriptor rebuild. A fixture can prove control flow, not actual GPU frametime.

An offline mock cannot prove driver/device-specific resize behavior, actual proxy internals, command execution completion timing, or provider interactions. Runtime validation would still be needed for any future alternative, but it cannot override documented D3D12/DXGI lifetime requirements.

## 7. Evidence and limits

### Confirmed from current project source

- `Renderer::InitializeImpl` AddRefs swapchain/device/queue and initializes heaps, fence/event, frame allocators/list, backbuffers and ImGui.
- `BuildResources` retains every successful `GetBuffer` result and creates RTV descriptors.
- `RenderImpl` records barriers/RTV use against `backbuffers_[index]`, submits and stores fence state; the frame allocator wait is conditional on that slot's incomplete fence.
- `BeforeResize` waits for submitted Overlay work, invalidates ImGui device objects and releases backbuffers before forwarding; nested calls are reduced to outermost recovery.
- `Shutdown` deliberately retains GPU-visible COM refs if completion is unknown; the project contract already notes that this can prevent native resize/replacement.
- ImGui DX12 backend uses raw device/queue/heap pointers under caller lifetime, owns font/per-frame upload resources, and its current project-modified shutdown can preserve GPU resources when completion is unknown.

### Confirmed from public API/source references

- DXGI `ResizeBuffers` requires all direct and indirect references to old backbuffers to be released before success.
- D3D12 command-list calls do not AddRef resources passed to them; applications must ensure referenced resources are not destroyed before GPU use completes.
- ReShade and the official ImGui example both explicitly reset/release before resize and reacquire afterward.
- OptiScaler has a narrowly worded skip-resize configuration tied to unchanged swapchain descriptions; it is not a generic overlay resource-lifetime guarantee.

### Unknown / not exercised

- Whether every FG/proxy transition in the target game uses an observable native resize, replaces the public proxy identity, changes the backbuffer identity, or changes a queue while leaving those observations stable is not established offline.
- No arbitrary provider supplies a documented pre-resize callback to this ASI.
- This review did not run a native DXGI/D3D12 harness, game session, FG toggle/resize scenario, or performance measurement.
- Therefore this is a static feasibility decision, not a claim of runtime proof that every resize event is captured by the current hooks.

## References

- Microsoft, [`IDXGISwapChain::ResizeBuffers` remarks](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers): release all direct/indirect backbuffer references before resize.
- Microsoft, [Creating and recording D3D12 command lists — reference counting](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles): command-list methods do not retain passed object references.
- Microsoft, [D3D12 resource binding specification — object lifetime](https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html): resources must remain alive until GPU execution completes.
- ReShade, [`dxgi_swapchain.cpp`](https://github.com/crosire/reshade/blob/main/source/dxgi/dxgi_swapchain.cpp) and [`reshade_events.hpp`](https://github.com/crosire/reshade/blob/main/include/reshade_events.hpp): pre-resize reset and post-resize initialization.
- Dear ImGui, [official Win32/DX12 example](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx12/main.cpp): release backbuffer references before resize, then reacquire.
- OptiScaler, [`OptiScaler.ini`](https://github.com/optiscaler/OptiScaler/blob/master/OptiScaler.ini): conditional `PreserveSwapChain` / `SkipResizeBuffers` settings and separate state/index options.
