# ResizeBuffers interception and wrapper ownership comparison

Date: 2026-09-28

## Scope and disposition

Focused static/source and preserved-runtime-evidence comparison of Camera Tweaks, ReShade, and the relevant OptiScaler frame-generation hook path. No production code, configuration, diagnostic binary, or package was changed; no game was launched; no Git action was performed.

**The closed `ResizeBuffers` cycle is confirmed. The exact T2→T3 hook-formation write/order is not.** The final graph proves that the Camera Tweaks callback's entry was detoured and that Steam's downstream continuation reached a trampoline which resumes inside that callback. It does not identify the writer of each code/relay pointer or the moment that topology first formed. That is the single material runtime evidence gap. Per the task boundary, no interception repair is selected or designed from the incomplete formation chronology.

## Evidence provenance and limits

The runtime evidence is the already-preserved PID 23136 session described in `FRAME_GENERATION_HOOK_CHAIN_CAUSAL_ANALYSIS_2026-09-28.md`, with raw WinDbg transcripts in `build-artifacts/fg-compatibility-20260928/`. It covers a local ReShade 6.8.0.2155 `dxgi.dll`, OptiScaler 0.9.5.4 `d3d12.dll`, Steam Overlay, RTSS, and the game's Streamline stack. Module presence alone is not treated as authorship.

The final graph was read directly from executable bytes, object/vtable memory, the Camera Tweaks saved-original vector, and Steam's continuation pointer. The session did not record writes as they happened. Addresses below are PID-specific and must not be reused in another process.

ReShade claims below are based on upstream `main` source, reviewed 2026-09-28; those source URLs are not a hash-pinned copy of the exact local ReShade binary. OptiScaler source claims are based on upstream `master` `FG_Hooks.cpp`, not a verified source/build match for local OptiScaler 0.9.5.4. The runtime transcript does not place an OptiScaler address in the proven cycle. Keep these distinctions when interpreting the comparison.

## Hook-formation epochs

| Epoch | Evidence-backed state | What is and is not known |
|---|---|---|
| T0 — original/proxy dispatch | Camera Tweaks' registry reads the current interface vtable and saves the method pointers before replacing selected entries. In the failed chain's record, saved slot 13 is the system `dxgi!CDXGISwapChain::ResizeBuffers` implementation. | No pre-Camera snapshot of this exact object's object pointer, vtable aliases, or dispatch bytes is preserved. The exact pre-T2 foreign topology for this object is unknown. |
| T1 — ReShade / OptiScaler state | ReShade and OptiScaler were loaded in the tested process. Upstream ReShade source implements an application-facing swapchain proxy for recognized D3D devices. Upstream OptiScaler source has a separate FG-swapchain detour path. | Neither loaded-module presence nor the source patterns prove which path owned this exact chain at each instant. The actual local OptiScaler 0.9.5.4 method topology is not source-matched here. |
| T2 — Camera Tweaks publication | Current source copies the current method vector, stores an immutable record, then uses `InterlockedExchangePointer` to replace selected entries in the table read from the COM object's vptr. Slots 13 and 39 become `HookResizeBuffers` and `HookResizeBuffers1`. The failed runtime object's slot 13 was observed pointing at `STALKER2CameraTweaks+0x5b990`; its registry original slot 13 was `dxgi!CDXGISwapChain::ResizeBuffers` at `0x7ffaa5a87180`. | This is a write to the existing table, not a per-object shadow table. The registry's immutable saved pointer value does not make the code at that address immutable. Which other objects alias that table was not inventoried during formation. |
| T3 — foreign chain modification | At the final snapshot, the system DXGI method entry jumps through a relay to Steam Overlay; the Steam entry is redirected through RTSS; Steam's downstream pointer leads to a relay containing relocated bytes from the Camera Tweaks callback entry and a jump back into the callback body. The callback entry itself begins with a relative jump into that relay/dispatch path. | The byte pattern proves a detour/trampoline involving the callback. It does not prove which manager wrote the callback patch, system-method patch, relay, or downstream pointer, nor their write order. The relay allocation's module/region owner was not recorded. No T2→T3 writer stack was captured. This is the unresolved formation edge. |
| T4 — FG OFF→ON replacement | The user associated the later replacement with enabling native DLSS FG. Runtime logs show ReShade-mediated replacement creation at about 13:55:32.785; the new chain was accepted with validated device/queue association. | The exact user-toggle timestamp is not present in the logs. Replacement creation succeeding is established; it is not itself identified as the defect. |
| T5 — replacement-chain Resize | About 70 ms after replacement creation, ReShade enters `ResizeBuffers`; Camera Tweaks logs resize sequence 4, but the native call has no matching end record. | This is the first confirmed non-returning boundary. No successful Present/Overlay renderer activation on replacement chain B was established. |
| T6 — closed cycle | The debugger snapshot proves `Camera callback → saved system DXGI address → Steam/RTSS → Steam continuation → Camera callback body`. The callback entry relay includes relocated callback prologue bytes followed by a jump to callback RVA `+0x5b997`. | This proves a closed call graph at the stopped state, not the chronology or the identity of every writer. |

### Exact final graph

```text
Camera Tweaks HookResizeBuffers body
  -> saved system dxgi!CDXGISwapChain::ResizeBuffers address
  -> relay -> Steam Overlay entry
  -> RTSS detour -> saved Steam continuation
  -> Steam downstream pointer
  -> trampoline with relocated Camera Tweaks callback prologue
  -> Camera Tweaks HookResizeBuffers body
  -> ...
```

The callback is exposed in COM dispatch by the Camera Tweaks shared-table patch. The final graph demonstrates the resulting foreign callback capture/re-entry. **It does not show whether that exposure was the sufficient trigger, which component consumed it, or whether another independent inline interception decision produced the same graph.** Do not attribute T3 to Steam, RTSS, ReShade, or OptiScaler solely from module presence or trampoline proximity.

## Camera Tweaks current interception and lifecycle

`src/overlay/dxgi_hook_registry.hpp` reads the object's current vtable pointer, copies `methodCount` pointers, publishes a process-resident record, then patches the selected entries in that same table. It does not clone the vtable or retain a COM reference to the object. Because it mutates the table rather than the object's vptr, all objects sharing that table can observe the slot change; the exact alias set is not recorded for the failed run.

`src/overlay/discovery_runtime.cpp` replaces slot 13 with `HookResizeBuffers` and slot 39 with `HookResizeBuffers1`. The callbacks retrieve the stored function address and forward the original `self` and arguments once, then return the actual HRESULT. Optional pre/post-resize work is separately guarded. `DxgiResizeNesting` controls optional observation/setup for nested calls; it does not make the native continuation acyclic. In the observed graph, calling the saved address eventually re-enters the callback.

## ReShade: owned proxy boundary, not an immutable native continuation

Upstream ReShade's `init_swapchain_proxy` creates a `DXGISwapChain` object and replaces the factory's returned swapchain pointer with that proxy when the D3D device is recognized; the D3D12 branch requires `IDXGISwapChain3` and passes the recognized command queue. The proxy implements `IDXGISwapChain4`; it does not implement its resize behavior by patching the underlying swapchain's shared vtable. See [ReShade DXGI factory/proxy creation](https://github.com/crosire/reshade/blob/main/source/dxgi/dxgi.cpp#L2363-L2448) and [the proxy class declaration](https://github.com/crosire/reshade/blob/main/source/dxgi/dxgi_swapchain.hpp#L27-L117).

The proxy stores `_orig` as an underlying COM interface pointer. Its constructors assume ownership of a live interface reference, and teardown releases that reference after resetting/destroying the ReShade runtime. When QI requests a supported swapchain interface, ReShade returns `this`; an interface upgrade QIs the underlying object for the newer interface, releases the previous `_orig`, and stores the upgraded reference. QI for `IUnknown` therefore preserves the outer proxy identity. ReShade's private `IID_UnwrappedObject` explicitly returns the inner object; other unrecognized IIDs are forwarded to `_orig`. AddRef/Release mirror the inner reference while maintaining the proxy object's own count. These rules preserve an outer wrapper identity for the supported interfaces while intentionally permitting an explicit unwrapped path.

For `ResizeBuffers`, ReShade runs `on_reset(true)` before forwarding to `_orig->ResizeBuffers(...)`, then reinitializes on success (and handles `DXGI_ERROR_INVALID_CALL` as a recoverable path) and returns the underlying HRESULT. `ResizeBuffers1` likewise resets before forwarding through the underlying `IDXGISwapChain3`; its implementation also extracts the underlying command queues from provider/proxy inputs before the call. See [ReShade ResizeBuffers/ResizeBuffers1](https://github.com/crosire/reshade/blob/main/source/dxgi/dxgi_swapchain.cpp#L2811-L2932) and [ResizeBuffers1](https://github.com/crosire/reshade/blob/main/source/dxgi/dxgi_swapchain.cpp#L3394-L3544).

This supports part of the hypothesis: ReShade owns an explicit outer wrapper/vtable and owns the inner COM reference, so its callback boundary is not created by writing its callback into the inner object's shared table. Its `_orig` is **not** a saved function address, however. A call through `_orig->ResizeBuffers` resolves the inner object's current vtable entry at call time; a later mutation/detour of that inner dispatch can still affect it. ReShade is therefore not protected by an immutable continuation, and its source alone cannot prove immunity from this cycle. Rather, the design has an explicit outer-proxy/inner-object boundary and a coordinated pre-resize/post-resize lifecycle.

On replacement, a newly returned supported swapchain can receive a new ReShade proxy around its own inner object. The old proxy continues to own its old inner reference until released. This is a source-level model, not proof that the failed runtime's chain B was an application-facing ReShade proxy at the instant Camera Tweaks installed its hooks: the captured saved-original pointer and vtable identify the underlying system DXGI implementation. Runtime QI/proxy identity for that exact object was not captured.

## OptiScaler: FG-swapchain method detours; local attribution unproven

The available upstream OptiScaler `FG_Hooks.cpp` uses a distinct model. `SetFGSwapchain` records the FG swapchain; `HookFGSwapchain` reads the method addresses from the FG object's vtable (including slots 13 and 39), stores them as `o_FGSCResizeBuffers`/`o_FGSCResizeBuffers1`, then uses Microsoft Detours to attach `hkResizeBuffers`/`hkResizeBuffers1` to those function addresses. This is an inline function-entry detour/trampoline approach, not a write of OptiScaler callbacks into the COM vtable. Its handlers forward through the Detours-managed original pointer and apply FG-specific lifecycle/parameter rules. See [OptiScaler FG swapchain creation and hook installation](https://github.com/optiscaler/OptiScaler/blob/master/OptiScaler/hooks/FG_Hooks.cpp#L116-L215) and [FG swapchain dispatch hooks](https://github.com/optiscaler/OptiScaler/blob/master/OptiScaler/hooks/FG_Hooks.cpp#L319-L435).

That topology can preserve a provider's FG swapchain implementation when the provider returns that object and OptiScaler hooks its methods. It is not a generic immutable continuation: the saved function pointer is updated by Detours, the executable entry is modified, and a later hook can still wrap or capture the detour target. If the current vtable slot already names another interceptor callback, inline detouring that callback can capture it. That is a plausible interaction to test, not a conclusion about local OptiScaler 0.9.5.4. The preserved cycle's writer stack and final graph do not establish an OptiScaler address as a causal edge.

## Interception-model comparison

| Model | COM object owner | Vtable owner / mutation | Downstream continuation owner | Can later foreign hook mutate/capture? | Proxy and QI semantics | Replacement / scope risks |
|---|---|---|---|---|---|---|
| Current Camera Tweaks shared-table patch | Foreign DXGI/proxy provider | Provider's existing table is modified in place; table aliases receive the callback | Camera Tweaks record stores a method address copied from the table | Yes. The address's code/chain can change; a foreign detour can capture the published callback. This capture/re-entry is present in the final graph. | Preserved only insofar as hooking the exact returned interface/table does not bypass another interface path; QI aliases with different tables require separate coverage. | A new replacement interface/table must be admitted. Old records are process-resident and do not own the COM object. Shared-table mutation can affect sibling objects beyond the validated target. |
| ReShade wrapper/proxy | ReShade owns outer proxy; owns a reference to inner object | ReShade owns outer proxy vtable; underlying provider owns inner table | `_orig` is an inner COM interface pointer; dispatch is resolved through its current vtable | Foreign code can hook outer proxy methods; modification of inner dispatch affects `_orig` calls. Source does not prove immunity. | Explicit proxy identity for supported QI; private unwrapped IID escapes to inner object; other unknown QI forwarded. | New proxy may wrap replacement; old proxy/inner pair retires by COM refs. Proxy and raw-inner paths can coexist; exact local object identity must be observed. |
| OptiScaler FG method path (upstream source) | Provider/current FG path supplies swapchain; exact object ownership depends on provider | It reads method entries, then Detours patches function entry, not COM table slot | Detours trampoline/original pointer is managed by OptiScaler for its hook | Yes. Inline entry/trampoline can be wrapped or captured later; pre-existing callback entries may be captured. | Intended to operate on the selected FG chain without replacing its COM identity in this hook code; complete QI behavior belongs to provider and is not established here. | FG replacement retargeting and object admission are provider-specific; local 0.9.5.4 implementation was not source-matched. |
| Per-object shadow vtable | Foreign object remains owner | Camera Tweaks would own a private table and replace the object's vptr | Still must retain the previous slot target/address | A foreign vtable hook can see/capture the callback; inline re-hook can detour it. Does not by itself prevent this self-cycle. | Every QI interface/alias must be wrapped consistently; unwrapped or alternate interfaces can bypass the shadow table. | Requires object/vptr lifetime management and safe replacement retirement; narrower blast radius than a shared table, but not sufficient proof against the observed callback capture. |
| Inline method interception | Foreign object/provider remains owner | Interceptor modifies shared executable method entry and owns a trampoline | Hook library owns its trampoline chain; native bytes can be changed by later managers | Yes; later hook can wrap the detour/trampoline or recapture a callback target. Method-wide scope may affect unrelated objects. | Hooking a native implementation can bypass an authoritative proxy; hooking a proxy method requires identifying all proxies/interfaces. | Global shared implementation scope, ABI/trampoline lifetime, and replacement routing. No proof that this preserves all FG proxy semantics here. |
| Full COM proxy | Camera Tweaks would own an outer proxy and reference the provider's inner object | Camera Tweaks owns proxy interface tables | Proxy delegates through owned inner COM interface pointers at each call | Foreign hooks may wrap the outer method or mutate inner dispatch; it creates a clearer boundary but not an absolute hook-order guarantee. | Can preserve semantics only with complete interface coverage, correct IUnknown identity, QI/refcounts, private unwrap policy, and exact argument/HRESULT forwarding. | Replacement must produce/retire proxy chains correctly; proxies layered before/after ReShade/FG can expose alternate raw interfaces. Broad redesign and not justified by present formation evidence. |

### Root architectural difference — bounded conclusion

One concrete difference is proven: **Camera Tweaks publishes its callback by mutating the current COM method table and then calls a copied method address; ReShade's resize callback is an owned proxy method that forwards through an inner COM object pointer.** OptiScaler's reviewed upstream FG path uses inline detours on FG-swapchain method addresses. These are different ownership boundaries.

That difference is a credible explanation for why a foreign hook manager can discover/capture the Camera Tweaks callback when it is present in a shared dispatch table. The final trampoline proves capture happened, but no write-time evidence proves that shared-table publication was the reason the relevant foreign manager selected the callback, nor which manager created the final downstream edge. ReShade's proxy does not establish a generally safer immutable chain, and OptiScaler's source does not attribute the local failure. Thus the **architectural contrast is established; the exact root write/ordering mechanism is not**.

## Bounded-repair feasibility

**No interception-only repair is currently proven to meet the complete candidate contract.** A per-object shadow table would stop changing sibling objects' shared table, but still publishes a callback in the object's dispatch and does not rule out foreign capture of that callback. Inline detouring transfers ownership to another mutable code/trampoline chain and may target an authoritative proxy implementation. A full COM proxy could create an owned method boundary but would require complete QI/identity/unwrap/replacement semantics across ReShade and FG layers; the observed formation path has not established that this larger model is necessary or that it meets no-load-order constraints.

The crucial missing evidence is the exact T2→T3 write sequence. Until captured, selecting among those models would be speculative. Therefore there is no proposed production design in this report. All already accepted requirements remain gates for any future design: pre-resize lifecycle before the authoritative resize; one semantically correct provider/wrapper operation with unchanged ABI/arguments and actual HRESULT; no bypass/synthetic result or around-call rehook; explicit callback/trampoline lifetime; safe replacement admission/retirement; no load-order requirement; and FS-01–FS-05 preservation.

## One diagnostic-only runtime capture

The operator-ready WinDbg steps for this single capture are preserved in the
[T2→T3 WinDbg capture plan](../plans/FG_RESIZE_CHAINING_T2_T3_WINDBG_CAPTURE_PLAN_2026-09-28.md).

Use **the existing canonical `STALKER2CameraTweaks.asi` unchanged**, with the already-confirmed local stack and the same single FG transition/resize reproduction. Do not add a renamed ASI or an in-process polling/logging path. Attach a user-mode debugger before the transition and use write watchpoints only after resolving the live addresses for the newly accepted swapchain. This preserves the canonical module basename and avoids adding a second hook implementation whose own timing could perturb formation. The watchpoints do not modify forwarding or target memory, but a debugger stop pauses the process and can perturb timing; record that limitation with the capture.

At the first real Camera Tweaks hook publication for the chain to be replaced, record the live object pointer, vtable pointer, slot 13/39 values, and saved-original addresses. Arm write watchpoints for:

1. the live COM table entries at `vtable + 13*sizeof(void*)` and `vtable + 39*sizeof(void*)` (record and disregard Camera Tweaks' own initial publication write);
2. the first byte of the live `HookResizeBuffers` and `HookResizeBuffers1` entries, to catch later inline entry detours;
3. the first byte of each corresponding saved-original implementation entry, to catch code-entry replacement;
4. if the exact loaded Steam Overlay build matches the preserved module identity, its observed downstream-continuation pointer slot (previous run's relative offset is only a lead; verify the new module identity and instruction reference before arming, do not reuse the old absolute address).

At every watchpoint hit, preserve the exception context before continuing: writer RIP and owning image/private region, thread ID and stack, old/new bytes or pointer, `!address`/module mapping, the surrounding relay bytes and destination, plus current object/table/record and FG-chain identity. Snapshot the watched locations once more at replacement creation, first Resize entry, and return/failure. These are bounded lifecycle snapshots, not a Present hot-path poll. Do not patch forwarding or submit GPU work. If the debugger cannot arm the watchpoint before a candidate write, mark that edge not captured rather than inferring it from the later snapshot.

Hardware data breakpoints are limited resources and may be thread-context-specific; the debugger operator must ensure the watched thread contexts include the actual writer. If one run cannot cover all relevant mutation threads/locations with passive watchpoints, stop and report the missed edge rather than replacing this with invasive page guards or blind API hooks.

**Single runtime question:** after Camera Tweaks publishes slot 13/39, which exact write first causes the final cycle—does a foreign component detour the Camera Tweaks callback, overwrite the object/shared table, redirect the saved system method, or update a downstream relay—and what exact `old target → new target` plus writer stack connects that event to the final Steam/RTSS route?

## Decision

- Closed runtime forwarding cycle: **confirmed**.
- Camera Tweaks shared COM table mutation and copied method-address forwarding: **confirmed by current source and the captured runtime object/record**.
- Callback entry detour/trampoline re-entry in the final graph: **confirmed**.
- ReShade-owned proxy and inner-object forwarding strategy: **confirmed in upstream source; exact local chain identity not established**.
- OptiScaler FG method-entry Detours strategy: **confirmed in upstream source; not mapped to local 0.9.5.4 cycle**.
- Exact writer/order forming T2→T3 and whether table publication was the sufficient trigger: **unknown; one runtime evidence gap**.
- Bounded production interception repair: **deferred pending that one capture; no production change recommended or made**.

