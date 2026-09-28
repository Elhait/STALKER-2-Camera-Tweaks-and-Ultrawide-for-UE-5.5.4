# FG-safe DXGI interoperability: bounded repair analysis

## Disposition

This report answers the expanded design/implementation request dated 2026-09-28. It incorporates the confirmed per-process `ResizeBuffers` cycle documented in [the causal report](FRAME_GENERATION_HOOK_CHAIN_CAUSAL_ANALYSIS_2026-09-28.md).

**No production repair was implemented.** The investigation found no bounded, generic continuation or cooperative hook-chain contract available to this ASI that can satisfy all of the requested semantics across arbitrary proxy swapchains and later foreign re-hooks. Implementing a recursion guard, choosing system DXGI as a fallback, or removing only the Resize callbacks would claim guarantees not supported by the source or runtime evidence.

This is a capability gap, not one missing runtime value. A runtime capture of another FG stack could classify that stack, but cannot create a common coordination API for arbitrary foreign hook managers. The safe general boundary remains: do not admit Overlay rendering on a presentation domain unless the required call and resource lifetimes are established; camera core stays independent. With the current architecture, late unannounced mutation of an admitted foreign chain cannot be made fail-open after re-entry, because callback code still needs a real native/proxy operation and its actual result.

No production source, tests, build scripts, project documentation or binary was changed. No game/debugger launch, test suite, build, Git mutation, release or packaging operation was performed. The earlier `CYCLIC_DXGI_FORWARDING_REPAIR_DESIGN_2026-09-28.md` remains historical; this report supersedes its abstract owned-continuation recommendation as an implementation-ready proposal.

## 1. Evidence from current code and vendor contracts

### Confirmed local cycle

The captured runtime graph is:

```text
Camera Tweaks ResizeBuffers body
  -> saved system-DXGI entry
  -> Steam / RTSS forwarding
  -> foreign continuation into Camera Tweaks callback body
  -> saved system-DXGI entry
  -> ...
```

The terminal native resize is not reached. The recorded callback body can be re-entered through a relocated prologue, so a guard only at our public function entry would not establish safety. See the linked causal report for the PID-specific pointer/disassembly evidence and its limits.

### Current renderer lifetime dependency

- `src/overlay/renderer_runtime.cpp:550`-654 retains the swapchain, device, queue, and every `GetBuffer` resource.
- `src/overlay/renderer_runtime.cpp:958`-987 waits for Overlay GPU work and releases backbuffers in `BeforeResize`.
- `src/overlay/discovery_runtime.cpp:1130`-1176 calls that preparation before forwarding ResizeBuffers/ResizeBuffers1, then uses the outer result to recover/invalidate state.
- `src/overlay/discovery_runtime.cpp:1353` and `:1358` install the two Resize callbacks in the shared method tables.

Therefore, simply omitting Resize callbacks while leaving the current renderer active can leave Camera Tweaks backbuffer COM references live across a native/proxy resize. Conversely, disabling rendering only after re-entry does not solve the call already in progress. Microsoft documents that `ResizeBuffers` requires outstanding direct and indirect backbuffer references to be released; this is a real lifetime requirement, not an Overlay preference ([ResizeBuffers](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)).

The existing `DxgiResizeNesting` and FS-04 correctly govern optional observation and outermost resize disposition. They do not own or repair the mutable native forwarding graph. FS-01 through FS-05 remain valid contracts and are not evidence of general foreign-chain safety.

### Official FG contracts reject a native-DXGI escape as universal policy

| Presentation path | Public integration contract relevant to this Overlay | Consequence |
| --- | --- | --- |
| NVIDIA Streamline DLSS-G | DLSS-G intercepts Present through a proxy swapchain; presentation can be asynchronous; more than one queue/present path can exist; DLSS-G must be turned off around window/resize transitions and its swapchain is recreated when toggled. Streamline cautions overlays not to assume a single swapchain/queue. ([DLSS-G guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_G.md), [general guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md)) | A saved system-DXGI method is not automatically the right object/method to call for a proxy, and application Present count is not displayed-frame count. |
| AMD FidelityFX FSR Frame Interpolation | The documented DX12 integration can replace the swapchain with a frame-interpolation proxy; the application is expected to use that proxy. ([FSR swapchain integration](https://gpuopen.com/manuals/fidelityfx_sdk2/techniques/frame-interpolation-swap-chain/)) | Calling an underlying native chain directly can bypass the intended presentation layer. |
| Intel XeSS-FG | XeSS exposes an `IDXGISwapChain4` proxy and requires subsequent swapchain interaction through it. It retains the queue supplied at setup and can use a background presentation thread/internal direct queue on non-Intel GPUs. ([Intel XeSS-FG guide](https://github.com/intel/xess/blob/main/doc/xess_fg_developer_guide_english.md)) | `ResizeBuffers1` input queues are not a universal statement of effective presentation ownership; neither object identity nor calling thread is stable authority. |
| OptiScaler/mixed FG | Its configuration exposes separate choices around preserving the FG swapchain and skipping resize calls. ([OptiScaler configuration](https://github.com/optiscaler/OptiScaler/blob/master/OptiScaler.ini)) | This is useful adversarial evidence that lifetimes vary; it is not a public chain-coordination contract to copy. |

These references describe their own integrations. They do not prove that every implementation used by S.T.A.L.K.E.R. 2 follows every path above, or that Camera Tweaks currently supports them.

## 2. Required architecture comparison

### A. Observe without owning Resize forwarding

Factory callbacks can observe factory-mediated swapchain creation/replacement. They do not observe every same-object Resize, and they do not necessarily see a proxy created later by an FG API from an existing chain. A Present callback can observe a particular COM path, but its own forwarding continuation is mutable too; proxy Present may return asynchronously and may not correspond one-to-one with display presents.

Window messages are not an authoritative substitute: they do not cover every ResizeBuffers call, do not report its actual HRESULT, and do not establish when a proxy performs internal replacement. Device/queue queries describe interfaces available to the caller; they cannot reveal an implementation's private presentation queue or synchronize its internal work.

The only obvious way to remove Resize forwarding while keeping the current renderer is to release every backbuffer before any Resize can happen. Current resources persist across Presents. Releasing them after every overlay submission would require proving GPU completion first, resetting every command-list/allocator reference, and reacquiring/rebuilding resources on later Presents. That imposes a synchronous wait on the Overlay path and still does not provide a generic pre-resize signal or solve mutable Present forwarding. It is a renderer redesign with a potentially per-frame stall, not a bounded, low-impact observation change. Asynchronous release cannot promise the references are gone before an arbitrary resize arrives.

**Finding:** observation-only factory/present/window evidence is insufficient for the current persistent renderer lifetime and exact native result. No hook-free standard callback was identified for all Resize/Present operations. A broad render-path redesign might change this tradeoff, but it has no demonstrated bounded solution for all listed paths and is outside this repair batch.

### B. Cooperative chaining

COM defines interface dispatch and lifetime rules, but does not define a common API by which unrelated injected hook managers publish/lease an immutable “next” handler, announce later re-hooks, or carry logical operation/progress identity through callbacks. Steam/RTSS/ReShade/OptiScaler cannot be treated as a single shared hook framework.

Streamline offers a vendor integration surface, including upgraded presentation interfaces and proxy identification, but using it as the universal answer would bind this mod to Streamline's setup/timing and does not coordinate FSR, XeSS, RTSS or later arbitrary hooks. The Streamline docs also say overlay integration/order matters in that path. That is not the requested no-load-order dependency. The cited FSR/XeSS interfaces similarly describe their own proxy lifetimes, not a universal third-party overlay callback bus.

Per-object shadow vtables or a full COM proxy are not a cooperative chain API. They can bypass a later hook that patches the old table, be patched themselves, miss QueryInterface aliases, or violate proxy identity/private table requirements. The current registry intentionally patches original tables and rejects truncated private tails; it has no safe general extent or late-re-hook ownership notification. Introducing a chain manager or competing writes would enlarge the architecture without establishing authority.

**Finding:** no common cooperative mechanism is present in the repository or established by the examined public interfaces. Per-provider adapters would impose provider-specific dependencies and still leave unadapted/late foreign hooks outside the contract.

### C. Owned continuation

The previous design's model is useful only when a real owner can provide all of these: callable continuation at the correct proxy/native layer, the suffix not yet executed, code/data lifetime, a stable in-flight epoch across replacement, and a way to announce or exclude later mutation. The ASI currently captures raw method addresses from COM tables; those addresses do not contain this information.

A SafetyHook trampoline owns relocated entry bytes but continues to whatever is currently after those bytes. It does not freeze private wrapper state or future downstream pointers. Resolving a branch to system DXGI does not prove that system DXGI is the correct target for an FG proxy. Preserving an old pointer does not prove its private relay allocation remains alive.

An exact re-entry test also cannot infer cycle versus legitimate finite nested retry from method, object, thread, depth, return address, or identical/changed arguments alone. TLS cannot represent work migrating to another Present/resize thread. Returning a stored/native HRESULT, skipping the call, or passing directly to a presumed lower layer would fail wrapper/result semantics.

**Finding:** the continuation contract remains a correctness specification, but there is no proven producer/owner for such a continuation in the current supported process environment. Do not implement a generic chain manager around the abstraction.

## 3. Chosen safety boundary and its unresolved limit

The only justified product rule is:

```text
core camera remains independent
Overlay may render only while its presentation/resource contract is known safe
unknown or changed topology -> Overlay unavailable, game/proxy call remains authoritative
```

That rule is not yet implementable as a guarantee against **silent late re-hook** with current callbacks. Before publication, the Overlay can refuse a domain it cannot establish. After publication, a foreign manager can mutate a shared dispatch target without a common notification. Once the foreign chain re-enters our body, refusing optional Overlay work still leaves the current ABI call needing a real continuation and HRESULT. Current code has neither a proven escape nor a way to make the foreign manager return one.

Accordingly, the current production topology does **not** satisfy the new universal compatibility contract. This does not show that vanilla DXGI or any particular FG product is universally incompatible; it shows the code has not proved the admission/lifetime boundary needed to claim safe interoperability for mutable foreign chains.

The minimal choice required before implementation is one of:

1. **Narrow the supported interception contract** to chains whose hook owner/lifetime/re-hook protocol is explicitly supported and tested; unknown domains fail admission. This necessarily makes compatibility conditional and requires a sound way to recognize/maintain that domain.
2. **Obtain a real cooperative continuation facility** from the relevant runtime/host or a shared hook framework that owns call ordering, epochs and lifetimes. No such universal facility is currently established.
3. **Replace the Overlay presentation architecture** with a separately justified observation/render lifetime model, accepting its performance and behavior consequences. This exceeds a bounded Resize forwarding fix and has not been proven for Present/proxy transitions.

None satisfies “arbitrary wrappers and silent re-hooks, any load order, no provider-specific dependency” using the code/data currently available. Therefore the conditional implementation authorization has not been triggered.

## 4. Assumptions removed; assumptions actually supported

### Removed from the accepted design

- Captured COM method address is an immutable downstream/native continuation.
- System DXGI is the correct terminal implementation for every object exposing an `IDXGISwapChain` interface.
- Application swapchain identity is terminal presentation identity.
- An application Present maps to one displayed frame or always occurs on one stable thread.
- Factory observation alone covers later proxy construction and every Resize.
- ResizeBuffers1 input queues identify every effective/internal presentation queue.
- Two successful Presents establish safe renderer/resource ownership across all FG lifecycles.
- TLS/depth or matching arguments can distinguish a forwarding cycle from all finite nesting.
- A module being loaded proves that module caused a chain mutation.

### Still supported by evidence

- The specific captured Steam/RTSS path returned through the Camera Tweaks ResizeBuffers body and did not reach its terminal native resize. This is one process topology, not a universal FG diagnosis.
- Current callbacks forward the original method arguments and return the original call's result on ordinary acyclic paths. Resize1 uses its own ABI and does not convert to ResizeBuffers.
- Current renderer holds backbuffer references until `BeforeResize`, and uses the outermost Resize result for recovery. Removing the Resize hooks without changing this lifetime is unsafe.
- FS-01–FS-05 remain regression contracts for their repaired cases. They do not prove proxy semantics or arbitrary foreign re-hook support.
- Official vendor documentation establishes materially different supported proxy/queue/presentation behaviors. It does not establish the exact runtime topology of every report or game configuration.

## 5. Regression suite required for a future authorized implementation

No fixtures were added or run in this analysis-only disposition. A future test must model semantics, not merely show recursion stopped.

### Forwarding graph and results

Exercise both direct callback entry and a foreign-wrapped application entry:

```text
native chain: Game -> Camera Tweaks -> terminal DXGI
foreign before: Game -> Foreign -> Camera Tweaks -> terminal wrapper suffix
foreign after: Game -> Camera Tweaks -> Foreign -> terminal wrapper suffix
confirmed cycle: Camera Tweaks -> saved entry -> A -> B -> Camera Tweaks body
proxy: Game -> FG proxy -> underlying implementation
late re-hook: admitted chain -> foreign rewrites dispatch -> next operation
```

Assert all of the following together: bounded completion, intended native operation count, exact arguments per invocation, exact outer/wrapper-adjusted HRESULT, correct pre/post side effects, no required suffix skipped, no repeated prefix, no renderer-owned backbuffer ref at resize, and no forged result. Include a cycle on body re-entry that bypasses the callback entry prologue.

Same-method finite nesting with identical arguments and changed arguments must both execute as intended when the wrapper creates distinct logical operations. Cover cross-method Resize1 -> Resize -> Resize1 recurrence. The harness may use a test-only watchdog to fail a reproduced hang; it must not use a production recursion cutoff or synthetic result.

### Lifecycle and domain matrix

Use deterministic fake COM objects with separate proxy/underlying identities and tables; queue and presentation-thread identity must vary independently. Cover:

- FG OFF -> ON replacement; ON -> OFF replacement; repeated ON -> OFF -> ON.
- Proxy-only, underlying-only, and proxy plus underlying observations; QueryInterface aliases and canonical identity.
- Foreign re-hook before admission, after admission, and while an old call epoch is active.
- Present and Resize on different threads, with explicit cross-thread operation ownership where the proposed contract requires it; a TLS-only proof fails.
- Application queue distinct from effective/internal presentation queue; ResizeBuffers1 input arrays changed, substituted, NULL/preserve, and multi-queue. Forward the received ABI arguments exactly; do not infer hidden proxy queues from them.
- Auxiliary/temporary swapchain activity that must not replace the validated game target; replacement and old-chain retirement with an in-flight GPU resource set.
- Optional Overlay failure at transition while game/FG dispatch and camera core remain operational.
- Startup toast and notifications when settings UI is hidden; hidden UI is not permission to skip renderer lifetime requirements.
- ReShade and OptiScaler foreign hooks before/after Camera Tweaks and later re-hook, without requiring either to be disabled or imposing a load-order rule.

Retain the existing focused FS-01–FS-05 harness contracts, including FS-04 outermost result. A passing forwarding fixture cannot substitute for lifetime/proxy/transition tests.

### What offline tests cannot establish

Offline fixtures can prove the code's state transitions against their modeled chain contract. They cannot prove a real vendor's private relay stays allocated, discover undocumented re-hook behavior, measure actual FG queue/thread ordering, establish user-visible pacing, or demonstrate coexistence on GPU drivers. These require supported public APIs or runtime evidence from that exact stack.

## 6. Runtime validation matrix if a bounded candidate is found

The smallest useful user-run matrix should group by distinct topology, not branding. A candidate would first need a single no-FG baseline, a native proxy FG OFF -> ON -> OFF transition, and a wrapper-composed FG transition with Resize plus replacement. ReShade + native FG and OptiScaler + FG belong in that wrapper-composed group when their loaded dispatch path is distinct. Add MFG/FSR/XeFG/SM86 only when offline implementation inspection shows an uncovered proxy/queue/ownership topology.

Each scenario should record executable/mod identity, proxy and underlying canonical identities where available, resize/present method and result, queue and thread observations without labeling input queues as effective presentation authority, old-resource release/retirement, overlay availability, and core feature status. These are a future plan only: no runtime matrix was executed here, and currently no candidate exists to validate.

## 7. Production diff, validation and next concrete gate

- **Production diff:** none.
- **Regression fixtures:** none added; none run.
- **Focused harnesses / `test.cmd`:** not run; there was no implementation to validate.
- **Production build / ASI:** not run or produced.
- **Game runtime:** not launched.
- **Git/release/package actions:** none.
- **Offline-established:** source call paths, retained renderer buffer ownership, actual recorded cyclic graph, and vendor-published proxy/queue contracts.
- **Not established offline:** support of any exact current end-user FG stack, lifetime of third-party private hook relays, wrapper-adjusted result behavior in the game, and a generic safe continuation API.

There is no single runtime experiment that can prove a universal interoperability guarantee for arbitrary hook managers. For the already reported local failure, one focused, user-run capture can still answer a narrower question: on a single FG OFF -> ON -> Resize/replacement transition, record the actual application-visible proxy identity and the callback entry/return chain for ResizeBuffers and ResizeBuffers1, including caller thread and outer HRESULT. A diagnostic-only capture must not alter dispatch or submit GPU work. This can determine whether that specific stack offers a stable, supported proxy-level route worth admitting; it cannot authorize a generic native bypass or prove compatibility with other providers.

## References

- [NVIDIA Streamline DLSS-G programming guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_G.md)
- [NVIDIA Streamline general integration guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md)
- [AMD FidelityFX frame-interpolation swapchain guide](https://gpuopen.com/manuals/fidelityfx_sdk2/techniques/frame-interpolation-swap-chain/)
- [Intel XeSS-FG developer guide](https://github.com/intel/xess/blob/main/doc/xess_fg_developer_guide_english.md)
- [OptiScaler configuration](https://github.com/optiscaler/OptiScaler/blob/master/OptiScaler.ini)
- [Microsoft ResizeBuffers contract](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)
- [Microsoft ResizeBuffers1 contract](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-resizebuffers1)
