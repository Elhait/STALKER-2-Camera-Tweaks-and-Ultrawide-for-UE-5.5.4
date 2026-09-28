# Final bounded Overlay safety/performance audit — 2026-09-27

## Scope and disposition

The user confirmed successful runtime of the productionized early-observation lifecycle. This audit accepts that evidence; it does not repeat the earlier isolation experiments or attribute the original startup crash to newly discovered defects.

Scope: current Overlay/DXGI callback initialization, publication, COM ownership, deferred renderer activation, resize/replacement, optional-failure boundaries and steady-state Present cost. Camera semantics, UI/layout/localization, packaging and release publication are outside the repair scope. No production changes or game launches were performed during this audit.

**Disposition: FS-01–FS-05 are repaired within the audit's bounded scope.** The detailed findings below preserve the original mechanisms and evidence; the final disposition and replacement regression contracts are recorded after them. No material steady-state performance regression was established. No additional audit or redesign is proposed.

## Findings

### FS-01 — P1: native replacement starts before old Overlay ownership is released

Locations: `src/overlay/discovery_runtime.cpp:1192`, `:1220`, `:1109`; `src/overlay/renderer_runtime.cpp:944`.

Both HWND creation callbacks invoke the saved native creation function immediately. Neither calls `BeforeSwapchainReplacement` before native creation. The only discovery caller of that helper is `TryActivateRenderer`, after a replacement chain already exists and has produced two successful Presents. A pending target also retains its chain through a `ComPtr` and is not discarded before replacement creation.

Mechanism: the application releases its references and attempts a new flip chain for the same HWND, but Overlay still owns the previous chain/backbuffers. Native creation can fail because that HWND still has a live flip chain. A rejected creation produces no replacement Presents, so delayed cleanup cannot repair it. This violates the existing architecture/safety documentation; it is not a proposal to change the accepted startup readiness gates.

Evidence: an offline fixture seeds the actual global Renderer with synthetic COM owners, invokes the actual `HookCreateSwapChainForHwnd`, and records the old reference counts inside the saved native callback. Both chain and backbuffer still have one reference. The fake native callback deliberately models rejection; its HRESULT is not a measurement of real DXGI failure. The old lifetime harness passes because it invokes the cleanup helper directly rather than the factory integration path.

The one-flip-chain-per-HWND restriction is documented in [CreateSwapChainForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd). This finding is separate from the intentional retention of an in-flight terminal GPU resource set when completion cannot be proved; that safety retention must not be undone to force native success.

Bounded repair: before native HWND replacement, detach the matching pending target and release matching active Renderer ownership through the existing bounded-wait cleanup. Preserve unrelated HWNDs and native arguments/results. Do not initialize the replacement Renderer there; activation must still require core readiness and successful native Presents. Define failed-creation input disposition using the existing owner.

Regression: exercise actual legacy/HWND creation callbacks with active and pending previous ownership; assert reference release precedes native entry, unrelated HWNDs are untouched, and failure does not leave capture active.

### FS-02 — P1: preactivation resize leaves a stale descriptor and presentation queue

Locations: `src/overlay/discovery_runtime.cpp:778`, `:927`, `:1015`, `:1113`; `src/overlay/renderer_runtime.cpp:992`, `:641`, `:743`.

`PendingRendererTarget` freezes count, format and queue at creation. Resize callbacks notify only `Renderer::OnResizeResult`; that method returns immediately unless the Renderer already owns the chain. Thus a successful resize while activation is delayed does not refresh or invalidate the pending count/format/queue. Successful Presents then make the old creation evidence eligible again, and activation passes stale metadata into `Initialize`.

Consequences are distinct:

- Changed buffer count: `BuildResources` detects the mismatch and initialization fails, unnecessarily disabling Overlay.
- Changed format with unchanged count: `BuildResources` checks count but not format; ImGui PSO initialization receives the old RTV format for new backbuffers.
- `ResizeBuffers1` changes presentation queue: the initialized Renderer can use the old queue without the queue validation applied to an already-active Renderer. This breaks the validated submission/presentation association contract; it is not the previously rejected cross-swapchain-resource hypothesis.

Evidence: the actual ResizeBuffers1 callback is invoked with a pending, not-yet-owned chain and a successful native stub. Requested count/format/queues change, but the pending target retains its original count, format and queue. No actual GPU submission or driver error is simulated. The subsequent stale initialization arguments and missing format check are established by source inspection.

The ability to change count, format and per-buffer presentation queues is documented in [ResizeBuffers1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-resizebuffers1); the render-target format is PSO state as described in [D3D12 pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12).

Bounded repair: apply resize invalidation/revalidation to pending targets too. After outer native success, read the actual descriptor, validate presentation queue identity or leave Overlay unavailable on ambiguity, and restart successful-Present evidence. At activation, verify descriptor freshness; never rely on two Presents to establish an unchanged descriptor/queue by themselves.

Regression: resize before core readiness or before the second qualifying Present; cover count changes, format-only changes, replacement queues, UNKNOWN/zero preservation parameters and rejected/ambiguous queue topology. Native resize must keep its original result.

### FS-03 — P2: first base factory observation prevents modern slots from being hooked

Locations: `src/overlay/discovery_runtime.cpp:1402`, `:1440`, `:1330`; `src/overlay/dxgi_hook_registry.hpp:70`.

Factory observation queries only the requested IID. A first `IDXGIFactory1` result installs a 14-slot record, even if the object supports a newer factory interface with the same table. A later modern observation receives `DuplicateVtable` and is considered reused; slots 15, 16 and 24 were never patched. The descending `FactoryInterfaces` list does not prevent this because this path uses it to look up one requested extent, not to query/install the richest supported interface first.

Mechanism: an application/wrapper using CreateDXGIFactory1 followed by QueryInterface to Factory2+ can create HWND chains through unobserved methods. Overlay never captures the target and cannot activate, while core remains functional. This is a deterministic missing-coverage condition, not proof of the historical startup crash.

Evidence: actual `ObserveFactoryResult` receives a base IID and then a modern IID on a synthetic shared table. The registry stays at 14 originals and the HWND creation slot stays untouched. Swapchain installation already queries interfaces in descending order; this finding is specific to factory observation.

Bounded repair: query supported factory interfaces in descending SDK order before first table publication, preserving distinct queried tables and captured originals. Do not blindly reinstall over existing hooks or capture our own hook as the original.

Regression: base-first/shared-table and modern-first/shared-table observations; distinct tables; unsupported IIDs; private vtable tails; saved-native pass-through and single publication.

### FS-04 — P2: recovered nested resize irreversibly erases discovery evidence

Locations: `src/overlay/discovery_runtime.cpp:979`, `:1048`; `src/overlay/discovery_evidence.cpp:99`; compare `src/overlay/renderer_runtime.cpp:996`.

Renderer teardown/rebuild correctly defers to the outermost matching resize. Discovery evidence does not: every inner failure immediately erases all queue candidates. If a ResizeBuffers1 wrapper recovers an inner ResizeBuffers failure and returns S_OK, outer success cannot restore those candidates. Later successful Presents increment nothing, so association remains Unknown and Overlay rendering stays off. Discovery lifecycle also receives the inner failure before outer disposition.

Evidence: actual nested callbacks model inner E_ACCESSDENIED followed by outer S_OK; two later actual HookPresent calls return S_OK but candidate count remains zero and association Unknown. This is the same recovery scenario the existing Renderer comment explicitly supports, rather than an invented concurrent native call sequence.

Bounded repair: use the outermost matching resize disposition consistently for Renderer, pending target, discovery lifecycle and queue evidence. Do not invalidate evidence permanently on a recoverable inner attempt.

Regression: inner failure/outer success, inner success/outer failure, repeated nesting and unrelated chain identities. Only outer success establishes revalidation; only outer terminal failure disables the affected Overlay path.

### FS-05 — P2: optional readiness exceptions can escape the Win32 startup boundary

Locations: `src/plugin/optional_overlay_startup.hpp:28`, `src/plugin/runtime.cpp:5020`, `src/overlay/discovery_runtime.cpp:1622`.

Startup isolation protects `startOverlay` and the startup-failure notification, but calls `markCoreReady` without containment. The real readiness callback publishes the atomic gate and then calls potentially-throwing logging. A string allocation or logging/mutex exception can therefore escape `InitializeThread` after core initialization completed.

Evidence: a throwing readiness callback in the actual startup template escapes to the probe caller after the core callback returns. This deterministically demonstrates the missing exception boundary; a real memory-pressure/logging failure was not forced in the game.

Related static boundary gaps of the same class: diagnostic Present formatting at `discovery_runtime.cpp:873` runs outside `RunOptionalOverlayWork`; registry lookup takes a potentially-throwing mutex before the callback boundary; `RestoreInputAfterTerminalFailure` also runs outside it. Diagnostic formatting is bounded and disabled by default, but enabling diagnostics must not weaken native callback containment.

Bounded repair: contain Overlay readiness/logging and other Overlay-owned throwing work at their actual foreign boundaries with a defined local failure transition. Keep original/native invocation outside optional-work catches; preserve its arguments and result. Do not declare throwing code `noexcept` without containment, and do not mask native access violations with SEH.

Regression: throwing readiness and failure logging, pre-native optional work failure with native called exactly once, and post-native diagnostic failure with native HRESULT preserved.

## Performance review

Normal successful Presents do not format/flush diagnostic frame logs when Diagnostics is disabled. After pending target consumption, activation returns before its owning COM copies and activation queries. Hidden Overlay with no notifications returns before ImGui frame creation, allocator waits and GPU submission. The confirmed lifecycle is not implemented as a sleep, polling loop or recurring hook installation from Present.

Steady-state overhead still includes one registry lookup with a mutex, ReadProcessMemory and shared lease; evidence scans under the evidence mutex; renderer ownership queries/serialization; and notification draining. Native Present and native Resize calls run outside the Overlay renderer/evidence critical sections. Bounded GPU waits exist only where submitted GPU ownership requires them; the normal hidden idle path does not wait on a fence.

An optimized x64 `/O2 /MT /EHsc` synthetic probe measured 200,000 iterations per sample:

| Operation | Measured CPU cost |
| --- | --- |
| Registry lookup, three samples | 0.333–0.356 microseconds |
| Evidence State + GetSingleCandidate + ObservePresent, 2 candidates | 0.0048 microseconds |
| Same evidence operations, 64 candidates | 0.0925 microseconds |
| Same evidence operations, 1024 candidates | 1.83 microseconds |

These are uncontended synthetic lookup/evidence timings, not a full Present benchmark, frame-time/FPS measurement, real wrapper cost or contention measurement. A separate nearly-empty direct-read loop is an optimized baseline, not a meaningful replacement for the validated registry. Do not derive an optimization mandate from its ratio.

Evidence storage scales linearly and does not prune different historical chain identities. That is a bounded-audit observation, not a demonstrated material performance defect for the observed two-chain topology. No caching, weaker COM identity checks, removal of safety locks or generic storage redesign is justified by these measurements.

## Contracts retained and limitations

- Synchronous early factory observation remains before core initialization; renderer/input activation remains behind release/acquire core readiness and two native S_OK Presents. OCCLUDED and TEST/DO_NOT_WAIT do not count as activation frames.
- Callback function types derive from Windows SDK methods; tested SDK extents and private tails are preserved. Registry records are published before slot dispatch and callback leases synchronize through the registry mutex.
- Original native calls remain outside optional GPU/UI catches and renderer/evidence locks; TEST/DO_NOT_WAIT pass through without Overlay GPU work.
- Core and Overlay resources remain independent; the newly found boundary omissions are exceptions to, not a reason to weaken, optional-UI failure isolation.
- Active renderer canonical identity checks and serialization remain intact; no renewed cross-swapchain-resource-use claim is made.
- GPU waits remain bounded at 1000 ms, with completion checks and terminal retention of unproven submitted references. Font upload has the same checked/bounded ownership disposition. Retention may block subsequent native resource recreation; unsafe release is not an acceptable workaround.
- Successful startup and offline tests do not prove driver/device-loss recovery across every external wrapper. No live game, GPU-debug-layer experiment or real driver failure injection was performed here.
- Present1 remains native-only by current design. An exclusively Present1-based path is not demonstrated to provide Overlay rendering and is not expanded in this audit.

## Repair disposition and regression contracts

- **FS-01 — closed:** the actual `HookCreateSwapChainForHwnd` fixture verifies matching old renderer/backbuffer references are released before native replacement creation, unrelated HWND ownership remains intact, and failed replacement closes input capture without applying terminal-disable semantics to unrelated targets. Registered in `renderer_lifetime_harness`.
- **FS-02 — closed:** the actual `HookResizeBuffers1` fixture verifies `count=0` refreshes the post-resize descriptor without changing the queue, valid same-device direct-queue rebinding refreshes the target, and ambiguous per-buffer queue identity keeps native resize successful while discarding the Overlay target. Registered in `renderer_lifetime_harness`.
- **FS-03 — closed:** actual factory observations verify base-first/shared-table and modern-first ordering both publish the supported modern creation slots once, preserve private table tails, and forward the native call/result. Registered in `dxgi_callback_harness`.
- **FS-04 — closed:** nested resize fixtures verify inner failure/outer success preserves candidate evidence until outer revalidation, while outer failure after inner success invalidates evidence; original native callbacks remain exactly-once. Registered in `dxgi_callback_harness` and `discovery_evidence_harness`.
- **FS-05 — closed:** startup readiness exceptions are contained after core readiness without making camera initialization fail; optional logging/failure transitions cannot escape callback boundaries, and native calls/results remain outside and preserved across optional-work failure. Registered in `optional_overlay_startup_harness` and `dxgi_callback_harness`.

The former standalone `CONFIRMED_*` reproduction probe and runner were retired after their five negative cases were replaced by the registered positive contracts above. The replacement fixtures establish callback ordering, state, COM reference disposition and native pass-through using synthetic COM objects/stubs; they do **not** simulate native GPU/device failure.

Validation completed: focused `renderer_lifetime_harness`, `dxgi_callback_harness`, `discovery_evidence_harness` and `optional_overlay_startup_harness` all pass directly. `test.cmd` exits 0 with 51/51 registered harnesses compiled and executed. The catalog/resource/font/glyph audit passes for 18 catalogs and 182 keys, including embedded resource and glyph coverage for sizes 12–24; Arabic shaping/bidi/RTL and runtime visual rendering remain outside that offline claim. The production `build.cmd` exits 0 and emits the separate candidate `build-artifacts/candidate-FS01-FS05/STALKER2CameraTweaks-FS01-FS05-candidate.asi`. `git diff --check` and `git diff --cached --check` report no whitespace errors; Git emits only LF-to-CRLF normalization warnings for existing edited text files. The build retains vendor/compiler warnings (Zydis C4201 and ImGui backend C4189). No game launch or Git/release/package action was performed.
