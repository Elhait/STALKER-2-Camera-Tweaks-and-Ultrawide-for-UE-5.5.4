# Adversarial production Overlay crash-safety audit

Date: 2026-09-27. Scope: current production source after the earlier DXGI startup repair, not the v2.0 ZIP. Authority and bounded batches: [plan](../plans/OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md).

## Evidence boundary

This is static inspection, SDK/API verification and deterministic offline validation. No game, real GPU, OptiScaler or affected-user machine was exercised. Earlier 50/50 harnesses are not proof of native runtime safety. The three field summaries do not establish a common cause. In particular, absence of mod logs in the AMD report does not prove that Overlay ran or did not run; no symbolized crash stack or instrumented GPU execution identifies our submission as the cause.

Classification: **Confirmed defect** means a source/API/lifetime contract contradiction, regardless of field reproduction. **Plausible crash path** has a concrete conditional mechanism but an unconfirmed trigger. **Hardening opportunity** has no established current defect. **Not an issue** applies only to the inspected contract, not arbitrary native/wrapper behavior.

## New confirmed defects and repairs

### AS-01 — Font-upload native failures and hidden unbounded wait

**Confirmed defect; repaired.** `external/imgui/backends/imgui_impl_dx12.cpp::ImGui_ImplDX12_CreateFontsTexture`, reached by `Renderer::BuildImGui` and font rebuild, ignored font texture allocation failure, used assertion-only checks for upload buffer/Map/fence/allocator/list/Close/Signal, ignored SetEventOnCompletion and waited INFINITE. Production defines NDEBUG, so assertions did not prevent null dereference or invalid submission. This path was outside the previously repaired renderer fence waits.

Mechanism: failed allocation/Map can produce null AV; failed Signal/event registration can hang the factory/Present callback indefinitely; invalid resources could poison D3D12. This can match null AV/hang/device failure classes, but is not evidence of the reported crash's exact cause or E_ACCESSDENIED.

Repair: checked bool return propagated to device-object creation; checked initial renderer command-list Close and backend root-signature creation; a local RAII font-upload owner releases pre-submit failures, waits at most 1 second, validates actual completed fence value including UINT64_MAX, retains submitted resources/queue/event if completion is unknown, and prevents retry of that terminal upload. `imgui_impl_dx12_safety.h` is an explicitly project-local vendor adaptation, not an upstream claim. Offline actual-backend tests cover texture/upload allocation failure and Map failure; synthetic submission tests cover Signal failure, device removal, event registration failure, timeout, completed/pre-submit release and pending retention.

### AS-02 — Partial ImGui DX12 init could crash its failure boundary

**Confirmed defect; repaired.** `ImGui_ImplDX12_Init` publishes BackendRendererUserData before frame-resource allocation; allocation failure leaves pFrameResources null. `Renderer::Shutdown -> ImGui_ImplDX12_Shutdown -> InvalidateDeviceObjects` dereferenced that null array using numFramesInFlight. C++ catch could therefore be followed by a native AV during cleanup.

Repair: null-safe frame-array invalidation and value-initialized entries. The actual backend harness exercises the published-backend/null-array state and verifies detached backend data after teardown. It validates the cleanup state, not an OS-level memory exhaustion reproduction. This is a null-AV mechanism; no established E_ACCESSDENIED/DEVICE_HUNG attribution.

### AS-03 — Timeout teardown released still-submitted GPU objects

**Confirmed defect; repaired.** `src/overlay/renderer_runtime.cpp::Shutdown` skipped a second wait after gpuWaitFailed but still released allocators, command list, backbuffers, descriptor heaps, fence/event, device/queue and backend GPU objects. A bounded timeout does not establish GPU completion. D3D12 application-owned resources must outlive their submitted use.

Repair: on unproven completion, terminal-disable and detach CPU UI state while deliberately retaining one terminal set of COM references/event until process exit; backend Shutdown receives that disposition. No heap allocation is needed on this failure path. Repeat Shutdown cannot release the orphaned refs or restart a frame; reinitialization cannot reset terminal state. WaitForGpu/WaitForFrame now verify the fence after an event wakeup rather than treating the event alone as completion. Actual renderer/backend fixtures check retained reference counts and idempotent teardown; normal completed/unsubmitted teardown remains covered.

This removes a concrete GPU use-after-free/device-removal mechanism. **Tradeoff:** retained backbuffers can make subsequent native ResizeBuffers/replacement fail; freeing them without completion proof merely to force native success is unsafe. Core camera state remains unchanged, but a hung/removed device or a game treating native resize failure as fatal cannot be made survivable by this UI boundary. This limitation can intersect E_ACCESSDENIED/native recreation failure, but is not field causality proof.

### AS-04 — Two independent frame-slot clocks

**Confirmed defect; repaired.** Renderer waits allocator/backbuffer slot indexed by GetCurrentBackBufferIndex; `ImGui_ImplDX12_RenderDrawData` previously chose vertex/index upload buffers using its own draw-call counter. Hidden/skipped UI frames or repeated/nonsequenced backbuffers can decouple those rings. CPU writes or growth/release of a backend buffer could then occur while a different slot's GPU work still reads it.

Repair: pass the actual waited slot to backend RenderDrawData, with bounds checks. Allocation/Map failure returns false so Renderer disables before submitting its partially recorded list. No global per-frame wait or presentation change was introduced. Tests use slot sequence 2,2,0,2 and actual backend Map dispatch to verify only the waited slot is touched. GPU execution/order itself remains runtime-only. This is a resource corruption/device-hung mechanism, not an established E_ACCESSDENIED explanation.

### AS-05 — Terminal UI could reopen invisible input capture

**Confirmed defect; repaired.** `discovery_runtime.cpp::ProcessOverlayWindowMessage` still accepted Toggle and invoked input capture after Renderer Disable destroyed its context. The game could lose input to an Overlay that could never render again.

Repair: renderer terminal state gates all Overlay input decisions before rebind/toggle/ImGui processing; native messages remain pass-through. Actual discovery policy/WndProc tests verify Delete/Esc/mouse remain unhandled, visibility stays closed and native message result is retained. This is a failure-isolation/input-loss defect, not proof of GPU/startup crashes.

### AS-06 — Unsynchronized WndProc chain and foreign-thread cursor call

**Confirmed defect; repaired.** `RestoreInputAfterTerminalFailure` read mutable g_inputWindow/g_originalWindowProc outside the renderer mutex while WM_NCDESTROY could clear them; OverlayWindowProc also read the original chain outside synchronization. The restore path directly invoked the game's window procedure from a DXGI callback thread rather than dispatching through the window owner.

Repair: snapshot original inside the existing renderer lock, retain that per-call native pass-through target, and post one terminal cursor message to the window thread. Installation already occurs under the same lock, so another-thread callback cannot observe half-published subclass state. Fixture checks native forwarding and WM_NCDESTROY clearing after forwarding. Real cross-thread message scheduling/foreign subclass behavior is not emulated. Data-race/incorrect-thread reentrancy can explain generic corruption or native AV, but no field trigger is established.

### AS-07 — Present probes/nonblocking attempts ran GPU work

**Confirmed defect; repaired.** `HookPresent` treated DXGI_PRESENT_TEST as an actual valid frame, advanced association evidence and could draw UI. DO_NOT_WAIT could also enter Overlay's GPU wait before reaching the native nonblocking call.

Repair: resolve original then immediately pass both flags through without identity probing, association mutation, UI wait or submission; original arguments/HRESULT remain unchanged. Actual callback fixture verifies native result and one call, zero QueryInterface probes and no readiness advancement. This avoids probe side effects/extra waits; no reported flag sequence is available to link it to DEVICE_HUNG. The source audit was updated to protect native-outside-optional-work semantics rather than the former location of the original declaration; negative mutation fixture remains enforced.

### AS-08 — Discovery temporary COM refs leaked on C++ failure

**Confirmed defect; repaired.** `TraceSwapchainIdentity` manually released device/queue/Swapchain3 only at the bottom, after allocating evidence/log strings and initializing input. An exception before that point retained temporary references, including a swapchain reference that could later prevent same-HWND replacement.

Repair: scoped WRL ComPtr owners for all three queried interfaces; raw local aliases preserve existing call semantics. Exception unwinding now releases them even if optional logging/evidence fails. Existing callback exception fixture checks native HRESULT preservation; the exact three-ref allocation-failure sequence is statically verified, not separately injected. Possible future recreation/E_ACCESSDENIED mechanism; no field proof.

### AS-09 — Export hook destructors performed detach-time unpatching

**Confirmed defect; repaired.** Global InlineHook owners had CRT static destructors. SafetyHook destructor calls destroy/disable, which traps/suspends threads and frees executable trampolines. CRT DLL teardown can therefore perform complex hook removal under loader lock, contrary to the loaded-until-exit contract.

Repair: process-resident export hook owners, matching the existing registry/runtime ownership model. No new dynamic-unload support or active callback uninstall was introduced. Publication tests continue to exercise the actual hook handles. Inspection of owner/destructor path establishes the repair; loader-lock/process-exit hangs are not reproduced offline. This is detach/exit safety, not an established startup-crash explanation.

### AS-10 — noexcept forwarding bypassed the C++ failure boundary

**Confirmed defect; repaired.** `src/plugin/runtime_settings.hpp::RuntimeSettingsApi::{Apply,Snapshot,SemanticSnapshot,Persist}` are noexcept but invoked non-noexcept callbacks directly. Production handlers include mutex acquisition and persistence/readback work. A thrown C++ exception therefore caused std::terminate before the renderer/WndProc optional boundary could respond. `plugin::GetRuntimeSettingsSnapshot` had the same direct readback forwarding issue. `DisableAfterOverlayException` itself was noexcept around mutex acquisition, defeating the outer boundary's catch for failure-transition errors.

Repair: settings API contains callback exceptions and reports rejected/unavailable/failed persistence through its existing result types; direct readback returns default rather than a partly filled snapshot after exception. Cleanup callback no longer declares noexcept around locks; RunOptionalOverlayWork already catches failure-transition exceptions. This is bounded C++ containment, not AV/driver recovery, generic rollback or a claim that an arbitrary partially mutated callback is transactional. Successful production camera/config mutations and persistence remain unchanged. API harness injects throws for all four callbacks; actual discovery fixture asserts the cleanup exception specification and tests a throwing failure transition. This is a terminate mechanism, not evidence that the field null AV or DEVICE_HUNG arose this way.

## Inspected non-issues and bounded limitations

| Area | Classification and evidence |
| --- | --- |
| Factory/Swapchain ABI and indices | Not an issue in inspected signatures: SDK-derived callback types/static_asserts; factory slots 10/15/16/24, swapchain 8/13/39. SDK extent harness and native callback argument/result tests. No QI/AddRef/Release interception; local successful QI references are balanced. |
| Existing publication/rollback/registry locks | Not reopened: record published before atomic slot patch; readers acquire synchronized leases; disabled export hook assigned before enable. Read-only table and concurrent publication fixtures cover selected paths. No concurrent uninstall is supported/attempted in production. |
| Core-before-Overlay/thread creation | Not reopened: successful core initialization precedes optional discovery thread; thread-creation failure returns false and does not reset core. Offline optional startup harness, not real injected loader lifecycle. Dynamic FreeLibrary remains unsupported. |
| Lifecycle startup fields | Not a demonstrated race: BeginDiscovery occurs before any export hook activation; no-hooks failure has no installed callbacks. No speculative lock repair retained here. Ordinary callback evidence/lifecycle changes are under g_mutex. |
| Renderer/input/notification/settings synchronization | Renderer/ImGui and input chain use renderer mutex; evidence/log use separate mutex; no native Present/Create/Resize call is inside those locks. Notification queue is bounded and mutex-protected; rebind flags/settings publication use existing owners. No lock-order cycle established in the inspected production paths. This is not a universal foreign-callback deadlock proof. |
| Single command list with per-frame allocators | Not an issue solely because Reset overlaps execution: D3D12 permits command-list Reset while prior recorded work executes; allocators/upload buffers still require their own fence completion. Same-slot wait and explicit backend indexing protect those resources. |
| ResizeBuffers/ResizeBuffers1 | Existing pre-release, nesting and queue-revalidation repairs preserved. Count/format changes retain device/queue during rebuild; failures disable UI without rewriting native result. Actual replacement/nested failure tests plus source inspection, not live fullscreen/device transitions. |
| Backbuffer barriers/heaps/format | PRESENT->RENDER_TARGET->PRESENT on the associated direct queue and actual current buffer; heaps/RTVs sized from validated bounded count; GetBuffer/PSO/HRESULT failures fail UI closed. Foreign resource-state ownership still requires runtime validation. |
| Present1 | Not hooked or rewritten. Direct Present1-only paths may not show Overlay unless the implementation reaches hooked Present. This is an observation/compatibility limit, not evidence of a native ABI corruption; no speculative new Present1 hook added. |
| Fullscreen/windowed/HWND/multiple swapchains | One renderer/input owner, not a multiwindow UI. Unrelated HWND replacement is not treated as same-window release. New discovery can select a different swapchain; an old input HWND can remain the subclass owner until destruction. This can impair UI ownership on unusual multiwindow flows, but no native crash mechanism was established from standard valid HWND use. No redesign added. |
| Localization/fonts/placement/Camera State | Catalog failure blocks normal UI before backend readiness; embedded font ownership and bounded sizes retained; native Auto-language reads stay on window-message path with image/prologue/buffer validation; Camera State remains read-only. Atomic config/placement persistence and no per-frame dirty retry remain unchanged. Native game-language ABI/image assumptions are not proven for unknown game builds. |
| Logging | Overlay log allocation/open is under optional startup boundary; callback allocation exceptions fail UI closed and preserve native calls. Synchronous flush can block on storage/foreign locks: bounded filesystem latency is not established. Core logging startup behavior predates this Overlay audit; missing field logs do not identify the failure stage. |

## Conditional risks — no speculative production repair

- **Plausible crash path P-01: wrapper-owned table reuse or cloning.** `DxgiHookRegistry::Find/Install` keys process-resident originals by table address. Standard module-resident DXGI tables fit this model. A third-party wrapper could free/reuse a heap vtable address, causing a stale native original, or clone an already patched table containing our callback into a new table. Capturing that callback as its own native original can recurse; a clone not observed by factory hooks can lack a callable original. These are concrete conditional mechanisms, but no affected wrapper table/lifetime evidence establishes their trigger. No wrapper-unwrapping heuristic or unproven QI/Release hook was added. Revisit with a captured wrapper table/original stack showing this condition; offline cloning/address-reuse fixtures can then protect a chosen ownership contract.
- **Plausible crash path P-02: foreign/native blocking or reentrancy.** Wrappers may change queue/backbuffer state ordering or call across threads while waiting on UI/native locks. C++ catches cannot contain invalid COM ABI, native AV, driver hangs or mutex cycles in external code. Standard native call pass-through and own locks were audited; real wrappers remain unexercised. Revisit with native stack/queue/state evidence, not an assumption that OptiScaler is required.
- **Hardening opportunity H-01: asynchronous diagnostics or stricter multiwindow/context isolation.** Slow synchronous log I/O and unusual secondary-window ownership could justify bounded infrastructure later, but no current field crash is proven from either. No logging framework, multiwindow UI owner, generic exception system or new UX was introduced.

## API sources

- [D3D12 resource binding/lifetime specification](https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html): submitted resource lifetime belongs to the application until GPU execution completes.
- [Fence completion/device removal](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-getcompletedvalue): UINT64_MAX is not successful completion.
- [Command-list Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-reset) and [allocator Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset): distinct reuse contracts.
- [ExecuteCommandLists validation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists) and [DXGI Present flags](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-present): submission validation and native probe/nonblocking semantics.
- [WndProc subclass pass-through](https://learn.microsoft.com/en-us/windows/win32/winmsg/using-window-procedures): retain original and forward through CallWindowProc.

## Validation and candidate disposition

Validation on the final production source:

- Full `test.cmd`: **51/51 harnesses, exit 0**; registry compile/run/error checks PASS.
- Focused actual-backend/renderer/native-callback fixtures: **PASS**, including the final throwing cleanup transition test. Runtime settings API injected callback exceptions: **PASS** through the full suite.
- Localization audit: **18 catalogs / 182 keys**, placeholders/UTF-8/embedded resources/source contracts and negative fixtures **PASS**. Initial audit gate rejected the moved Present original declaration; the updated boundary contract and negative mutation passed. The rejected initial invocation was not a full-suite PASS.
- Resource/glyph/font coverage: embedded profiles/selector faces and integer sizes **12–24 PASS offline**. This does not establish Arabic shaping/bidi/RTL or runtime visuals.
- Production `build.cmd`: **exit 0**, output `build-artifacts/adversarial-audit/STALKER2CameraTweaks.asi`, 2,728,960 bytes, built 2026-09-27 14:38:03 +03:00. Existing vendor warnings C4201 (Zydis) and C4189 (legacy backend init) remain; test-only included discovery fixture also emits C4459 name shadowing, not a production warning.
- `git diff --check`: **PASS**; LF/CRLF informational warnings, no whitespace errors. Changed paths reviewed against the plan. User-staged screenshots preserved; no README, locale/catalog, camera algorithm, public INI, release archive or root release ASI edits in this audit. Earlier uncommitted repair files remain present, not attributed to this audit alone.

Outputs stay under ignored build-artifacts/adversarial-audit, not release assets. No game launched, Git mutation or ZIP repackaging.

| Area | Audited | New defect | Fixed | Remaining runtime risk |
| --- | --- | --- | --- | --- |
| SDK COM ABI / existing registry/publication | Yes | No newly confirmed ABI defect | Prior repairs preserved | Foreign wrapper table clone/reuse not established |
| Font upload / partial backend init | Yes | AS-01, AS-02 | Yes | Real allocation/driver/wrapper behavior unexercised |
| GPU completion / upload frame slots | Yes | AS-03, AS-04 | Yes | Device failure; retained refs may block native recreation |
| WndProc / terminal capture | Yes | AS-05, AS-06 | Yes | Foreign subclass ordering / unusual multiple HWNDs |
| Present probes / nonblocking attempts | Yes | AS-07 | Yes | Direct Present1-only path may lack Overlay |
| Temporary COM exception lifetime | Yes | AS-08 | Yes | Exact field allocation failure not established |
| Hook detach / thread lifetime | Yes | AS-09 | Yes | Dynamic unload intentionally unsupported |
| C++ settings/cleanup boundaries | Yes | AS-10 | Yes | Native AV/device hang outside C++ containment |
| Resize / queue / fullscreen / formats | Yes | No further confirmed defect | Prior repairs preserved | Real GPU transitions/wrappers unexercised |
| Core isolation / logging / localization / config | Yes | No additional confirmed defect | Preserved; readback containment in AS-10 | Storage latency / native game-build assumptions |

The earlier repaired ASI is superseded by this candidate because concrete new defects were found. **No unfixed confirmed blocker found in this audit prevents giving this final audit candidate to affected users for runtime A/B.** It is not a declaration that the Nexus crashes are fixed. Pending-GPU retention and conditional foreign-wrapper risks remain explicit; offline PASS cannot certify device survival or justify a public compatibility claim. User closure/release disposition remains separate.
