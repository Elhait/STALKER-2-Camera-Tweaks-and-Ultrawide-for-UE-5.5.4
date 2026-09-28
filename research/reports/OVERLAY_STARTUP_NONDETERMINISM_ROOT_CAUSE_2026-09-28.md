# Overlay startup nondeterminism — causal architecture analysis

Date: 2026-09-28. Scope: read-only analysis of current source, preserved observations and upstream implementations. No production repair, build, game launch, Git mutation or packaging was performed. Only this report and the master investigation plan are changed.

## Decision summary

The strongest explanation is **capture/publication ordering between independent hook managers**, not a semantic ASI filename dependency. Our asynchronous initialization worker captures and patches system DXGI factory entries while ReShade can independently capture their then-current contents and publish its own queued patches. A wrapper's saved trampoline can therefore include our callback or bypass it even when the final system export bytes look identical.

This precise interleaving is **STRONG INFERENCE**, not a captured writer-stack finding. What is **CONFIRMED FROM SOURCE** is the absence of an observation-coverage invariant: successful installation of our export patches is treated as sufficient to await factories, although external dispatch can bypass those patches and there is no independent bootstrap or missed-event recovery.

A second plausible mechanism is missing the useful creation event before observation starts. The current architecture cannot distinguish that case from downstream bypass. It cannot recover an already-created swapchain merely from HWND.

Recommendation: one bounded **factory-table bootstrap / shared COM observation** batch, using the existing registry rather than competing to remain the top system export hook. It is the smallest order-independent route worth implementing and testing for the observed stack. Its shared-table coverage and pre-existing-swapchain limits must be explicit implementation gates, not assumed guarantees. A debugger is not necessary to start that bounded batch; it is necessary if we want to attribute the historical failure to one exact saved target/writer or if the bootstrap coverage gates fail.

## 1. Evidence subjects and limits

The byte-identical trace-v2 artifact is `90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019`. The recorded game image is `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.

`build.cmd:34–36,67` adds `OVERLAY_STARTUP_TIMELINE` to production feature definitions when requested. Thus these runs exercise production features **with diagnostic instrumentation**, not proof that the flag-off artifact has deterministic startup. Instrumentation itself can perturb ordering.

Evidence classes used below:

- **CONFIRMED FROM TRACE:** a recorded event/identity within the instrument's scope, or the user's observed activation outcome.
- **CONFIRMED FROM SOURCE:** a current implementation property, not proof that a particular branch ran.
- **STRONG INFERENCE:** a concrete source-supported mechanism consistent with the observed outcomes, without its decisive runtime event captured.
- **POSSIBLE:** a mechanism not excluded but lacking the distinguishing evidence.
- **DISPROVEN / INSUFFICIENT:** a rejected explanation or insufficient evidence for the stated conclusion.

Raw latest FAIL logs were inspected. Earlier PASS raw logs are no longer available: after a previous misunderstanding they were deleted without backup. PASS timings and outcomes below are preserved observations in `research/plans/ASI_FILENAME_STARTUP_TIMELINE.md` and the conversation, not a newly reproducible raw-log comparison. Do not silently upgrade their provenance.

Latest FAIL snapshots inspected:

| File | Snapshot SHA-256 |
| --- | --- |
| Startup log | `1AD9B0725B726B0320AFCAE48D1DA90BDE38C3B8F28EBA98368835FBFE7D7C90` |
| Overlay log | `747FBB68FA12A7590DB9583D92964526C9B0F30067B3A365144CF999EBE80F2B` |
| Core log | `800B8D2CAE65F9943C5A19962EC96AFD2BCDCE8E28D560911E8579B35BA8894D` |
| ReShade log | `B9AA06FA4E3CF0FA5532779F4CA6D67F5A67B9A42C7A36436BC40FAB3B20A53D` |

These identify read snapshots, not immutable archived copies. Active-process logs can continue changing.

### Material correction: ReShade wall-clock records were process-mixed evidence

`startup_timeline.hpp:312–323` obtains actual process creation FILETIME through `GetProcessTimes`; event formatting uses local time. Converting the recorded FILETIMEs with the machine's FLE Standard Time rules gives:

| Run | ASI trace PID | Process born, local time | ReShade log begins | First logged Factory1 redirect |
| --- | --- | --- | --- | --- |
| Alternate PASS | 28604 | 00:17:46.2039504 | 00:17:42.883 | 00:17:43.768 |
| Canonical PASS | 17696 | 00:27:37.9796341 | 00:27:34.353 | 00:27:35.236 |
| Latest canonical FAIL | 3828 | 00:36:05.5092188 | 00:36:01.929 | 00:36:02.854 |

The early ReShade entries precede the traced process's existence by several seconds. **They cannot establish that process's pre-arm factory chronology.** No clock correction was established that would reconcile this. A different/launcher/parent game instance is a plausible explanation, not an identified owner.

[ReShade's logger source](https://github.com/crosire/reshade/blob/v6.8.0/source/dll_log.cpp#L40) opens one fixed path with `CREATE_ALWAYS`, write access, only read sharing and write-through; its bracketed number is a thread ID (`GetCurrentThreadId`, line 78), not PID. Multiple game processes can compete for this file, and a nearby timestamp does not certify ownership. Later entries in that same file also cannot simply be assigned to the ASI PID.

Withdraw the earlier conclusions that ReShade's first redirect preceded our arm **in the same process**, or that matching those wall-clock orders explained both PASS runs. The ASI's own module ownership and callback caller records remain valid same-process evidence.

## 2. Current observation architecture

`src/plugin/dll_entry.cpp:4–30` starts `InitializeThread` with `CreateThread` and returns without a readiness handshake. `src/plugin/runtime.cpp:5018–5040` arms Overlay discovery before camera initialization, **inside that worker**. This is synchronous relative to camera startup, not relative to DllMain return, the loader's caller or other graphics initialization.

`src/overlay/discovery_runtime.cpp`:

- `LoadExactSystemDxgiModule` at 261 selects an absolute system path, not the local proxy by basename.
- `InstallExportHook` at 1673 creates a disabled SafetyHook, publishes its process-resident owner and enables it.
- `InitializeImpl` at 1702 installs system `CreateDXGIFactory`, `CreateDXGIFactory1`, `CreateDXGIFactory2`. Any installed export suffices for its boolean success.
- Factory export callbacks use the saved SafetyHook original, preserve native result, then call `ObserveFactoryResult` at 1529.
- That function queries widest supported factory interfaces first and invokes `InstallFactoryHooks` at 1433. The registry patches selected slots in the original SDK-bounded tables, not per-instance truncated clones.
- Only intercepted real factory `CreateSwapChain*` calls supply creation queue/device association and trigger `InstallSwapchainHooks` at 1269.
- `TryActivateRenderer` at 1136 requires core readiness, validated pending association and successful Present evidence. That accepted gate is not the defect and must remain.

There is no separate seed factory, production module-event bootstrap, existing-object enumeration or late swapchain recovery. No factory callback means no factory-table installation, no swapchain observation and no renderer/input activation. Core `AVAILABLE` is independent of that progress.

**Violated invariant:** `export hook enabled` is local installation state; it does not imply `the application's useful factory/swapchain dispatch is covered`. Our correctness currently needs an externally uncontrolled ordering relation.

## 3. What the hook managers actually capture

### SafetyHook: authoritative vendored implementation

The repository does not establish an exact pinned upstream revision for its amalgamation. The actual files, rather than a guessed release version, are authoritative:

- `external/safetyhook/safetyhook.cpp`: SHA-256 `3C2C7FCF6FB5D0E578FD24A7155B255D01DAB653BF6A2E408D84CEAB8D4AA1F8`.
- `external/safetyhook/safetyhook.hpp`: SHA-256 `708D22B42C6BAC62E21BA86A3FCC5D8E89DBD156B09629EB81D58D6A07EACB3E`.

`InlineHook::e9_hook` (cpp:510 onward) decodes/copies the existing entry and relocates its instructions. An existing relative branch retains its captured destination; the trampoline is not a live lookup of whichever chain later owns the entry. `original<T>()` (hpp:638) returns that trampoline address.

`enable` (cpp:689) writes our entry branch and sets internal enabled state. It does not share a transaction lock with another manager or revalidate a foreign manager's saved original. Another manager may subsequently replace the entry while our object remains enabled. `disable` (cpp:732) restores captured bytes; blind disable/recreate can overwrite a newer owner's entry and invalidate retained chain references.

[Current upstream implementation](https://github.com/cursey/safetyhook/blob/f44cc070a8340f2f26649553c49533475417304d/src/inline_hook.cpp) was inspected as a structural comparison, not claimed to be the exact vendored revision. Neither an allocator relay nor thread suspension establishes cross-manager chain ownership.

### ReShade 6.8.0 and its pinned MinHook

[ReShade hook implementation](https://github.com/crosire/reshade/blob/v6.8.0/source/hook.cpp) separates creation of a saved trampoline from queued activation. Its [delayed hook manager](https://github.com/crosire/reshade/blob/v6.8.0/source/hook_manager.cpp#L313) installs the matching exports and then applies the queued actions. `HookLoadLibraryExW` at 429 calls native loading first, then checks delayed hooks at 435. This permits the ASI's newly runnable worker to overlap the loader caller's post-load work. DLL notifications provide another trigger; this is not an assertion that a particular trigger performed our observed write.

ReShade's MinHook submodule is `8fda4f5481fed5797dc2651cd91e238e9b3928c6`. In [hook.c](https://github.com/TsudaKageyu/minhook/blob/8fda4f5481fed5797dc2651cd91e238e9b3928c6/src/hook.c#L535), `MH_CreateHook` constructs and returns the trampoline before activation. `EnableHookLL` at 351 later writes the branch without comparing current entry contents to the creation snapshot; `MH_ApplyQueued` at 786 freezes threads and performs queued writes. Its own lock does not serialize SafetyHook's separate capture/enable transaction.

[MinHook trampoline decoding](https://github.com/TsudaKageyu/minhook/blob/8fda4f5481fed5797dc2651cd91e238e9b3928c6/src/trampoline.c) turns an already-present external relative jump into a branch to its captured destination. Consequently an old Steam route and our newer route produce different saved originals despite the same subsequent ReShade entry patch.

ReShade's `find_internal` at 283 selects the first matching saved record. Its `call` at 635 returns that record's target/trampoline, loading an export module on demand only when no record exists. `ensure_export_module_loaded` at 600 can register an export-forwarding record, distinct from an inline function-hook record. Factory wrappers in [dxgi.cpp](https://github.com/crosire/reshade/blob/v6.8.0/source/dxgi/dxgi.cpp) use this lookup; they do not universally re-enter the current system export on every call. Record-type/registration ordering can therefore matter too, but its actual state in FAIL has not been captured.

### Ultimate ASI Loader 9.7.4

[UAL's source](https://github.com/ThirteenAG/Ultimate-ASI-Loader/blob/v9.7.4/source/dllmain.cpp#L852) uses `FindFirstFileW/FindNextFileW`, builds a path and loads each ASI without sorting or a CameraTweaks-specific branch. `LoadLib` at 1381 calls `LoadLibraryW`. If an ASI exports `InitializeASI`, UAL calls it after successful loading (927–931); our current entry does not use that protocol.

This provides no completion ordering between our detached initialization worker and later graphics hook work. Filesystem iteration and loading costs may perturb chronology; neither proves semantic filename treatment. Adding `InitializeASI` would narrow one interval, but would not itself serialize foreign capture/enable or recover past creation events. The observed attach stack includes DSOUND/local DXGI/Steam; it does not prove which loader's every branch ran.

### OptiScaler 0.9.5-pre4, matched commit

Matched source: `8dac650cbf90c85ca1d46747a66fc98118be75b8`.

[Dxgi_Proxy.h](https://github.com/optiscaler/OptiScaler/blob/8dac650cbf90c85ca1d46747a66fc98118be75b8/OptiScaler/proxies/Dxgi_Proxy.h) selects an existing DXGI module or loads system DXGI, obtains export targets, and uses Detours transactions to return saved factory originals. [Dxgi_Hooks.cpp](https://github.com/optiscaler/OptiScaler/blob/8dac650cbf90c85ca1d46747a66fc98118be75b8/OptiScaler/hooks/Dxgi_Hooks.cpp) calls those originals and can wrap returned factories. Factory-method interception is separately implemented in [DxgiFactory_Hooks.cpp](https://github.com/optiscaler/OptiScaler/blob/8dac650cbf90c85ca1d46747a66fc98118be75b8/OptiScaler/hooks/DxgiFactory_Hooks.cpp).

The actual local `d3d12.dll` load notification is later than core-ready in the recorded ASI process. It cannot explain an earlier rewrite already visible at core-ready; it can affect subsequent topology. No shared cross-manager transaction guarantee was found. Do not conflate local ReShade DXGI with the later OptiScaler route.

[Its ASI loader](https://github.com/optiscaler/OptiScaler/blob/8dac650cbf90c85ca1d46747a66fc98118be75b8/OptiScaler/dllmain.cpp#L210) iterates a directory without sorting and treats `-loadlate` specially. Neither tested name has that suffix. `LoadAsiPlugins=auto` does not establish that this path actually loaded our ASI.

## 4. PASS/FAIL timeline and earliest possible divergence

| Same-process observation | Alternate PASS | Canonical PASS | Latest canonical FAIL |
| --- | --- | --- | --- |
| Early arm complete | 00:17:46.559 | 00:27:38.328 | 00:36:05.861 |
| Core ready | 00:17:47.719 | 00:27:39.875 | 00:36:07.378 |
| System entry at core-ready | Relay → local DXGI | Relay → local DXGI | Relay → local DXGI |
| Our Factory1 callback | 00:17:51.348 | 00:27:40.449 | Not recorded through toggle |
| Callback caller | Local DXGI | Local DXGI | Unknown |
| Factory1 return | S_OK | S_OK | Not recorded |
| Actual activation | Present evidence → renderer/input → visibility | Same | No activation |

Latest FAIL has no buffer overflow: capacity 128, recorded event count 113. Its toggle snapshot at 00:39:47.566 still has no factory callback, and Overlay ends at `OVERLAY_CAMERA_CORE_READY`. That supports a real discovery miss, not trace-capacity exhaustion. It does **not** prove no factory was created elsewhere or before our observation.

Concrete source-permitted interleavings below concern the same factory export E. These are a causal model, not reconstructed debugger events:

| Phase | PASS-compatible ordering | FAIL-compatible ordering |
| --- | --- | --- |
| Previous owner | E → Steam route | E → Steam route |
| First relevant capture | Our trampoline captures Steam; our hook enabled | ReShade/MinHook captures Steam, queues its hook |
| Overlapping work | ReShade/MinHook captures our installed branch | Our trampoline captures Steam; our hook enabled |
| Later publication | ReShade publishes its entry patch | ReShade publishes its queued entry patch |
| Wrapper saved dispatch | Wrapper → saved trampoline → our callback → Steam/system | Wrapper → saved trampoline → Steam/system |
| Visible E afterward | E → ReShade | E → ReShade |

**Earliest divergence is what the foreign manager snapshots into its original, relative to our entry publication — not the final entry rewrite.** A wrapper/export-forwarding record selecting a different stored original is another source-permitted route to the same visible outcome. Same PID source instrumentation did not capture those private records.

A second FAIL-compatible ordering is useful factory/swapchain creation before worker observation. Later export patches then remain irrelevant without another creation event. The PASS callback several seconds after arm does not disprove earlier useful creation in a different FAIL run.

## 5. Ranked root-cause hypotheses

### H1 — independent capture/publication transactions create different downstream chains

**Confidence:** high for the failure class; medium for the exact ReShade/MinHook interleaving in the observed FAIL.

**Mechanism:** a manager captures the old route before our enable and publishes afterward, so its saved original bypasses us; the opposite capture order preserves us. Different registration of ReShade forwarding/function records is a related private dispatch variation, not a confirmed separate defect.

**Supports:** current SafetyHook originals are snapshots; ReShade/MinHook queues publication separately; asynchronous worker overlaps external load work; same final entry route appears in both outcomes; PASS demonstrably reaches our callback from local DXGI. Relevant code locations are in section 3.

**Counterevidence/limits:** no FAIL writer stack or saved ReShade original was captured; PASS raw evidence is no longer available; mixed-process ReShade timestamps cannot prove the exact overlap. Steam or another layer's stored dispatch could also contribute. No assertion that a particular manager is faulty: our observation assumption is fragile regardless.

**Explains:** same bytes/name/stack, variable outcomes, apparent successful arm followed by zero callbacks, and filename/log I/O as timing perturbations. **Does not establish:** exact historical writer, exact first capture timestamp or absence of an earlier useful creation.

### H2 — only useful creation occurred outside our observation interval

**Confidence:** medium-low as the observed primary cause; high that the design lacks recovery for it.

**Mechanism:** DllMain returns, the worker has not armed observation, and graphics startup creates useful objects. No later covered creation occurs. The current chain has no way to recover them.

**Supports:** detached worker with no handshake; only export-return discovery; no independent bootstrap/recovery. **Counterevidence:** arm occurs early after process birth; PASS useful callbacks occur later; current traces do not record an early FAIL creation. Pre-birth ReShade records are specifically not evidence for H2.

**Explains:** no discovery despite successful arm/core readiness. **Does not specifically explain:** a foreign saved target bypass after arm; H1 and H2 can coexist.

No third hypothesis is promoted merely to fill a list. Deterministic basename behavior is **DISPROVEN** by canonical PASS and FAIL. Export overwrite alone is **INSUFFICIENT** because PASS has it too. Cross-swapchain resource use is not reopened; existing ownership/serialization evidence stands.

## 6. Log-file hypothesis

Current startup trace uses a fixed file, `CREATE_ALWAYS`, read sharing and bounded flushes. Overlay opens its fixed log with truncation (`InitializeImpl:1702` onward) and flushes lifecycle messages. Core truncates and opens a spdlog basic file sink (`runtime.cpp:4692–4697`). No inspected path reads old log prose/content to choose discovery targets or readiness.

Therefore **old log contents enabling Overlay is unsupported**. Opening, truncating, flushing, filesystem metadata and contention can perturb scheduling; that remains POSSIBLE, not a measured cause. ReShade log competition additionally changes evidence availability.

There is a real source distinction: core logger initialization failure returns before core setup. A locked/unwritable core log could therefore affect another run, but it does not explain the inspected FAIL with all three camera components `AVAILABLE`. Overlay log failure is not used as its hook-installation success condition. This analysis records the distinction without expanding into a logger repair task.

Deleting logs before one PASS was a preceding event, not a causal intervention established by evidence. Do not propose deleting logs as a fix, nor delete any further evidence.

## 7. Repair alternatives

| Approach | Does it remove the race? | Ownership/compatibility and recursion | Cost/complexity/failure isolation |
| --- | --- | --- | --- |
| Earlier synchronous observation | Only narrows the worker interval; does not control foreign capture/publication or historical creation | Heavy hooking/LoadLibrary/thread trapping inside DllMain is unsafe. A loader `InitializeASI` handshake is narrower but not universally guaranteed | Low–medium code cost outside loader lock; optional failure can remain isolated; insufficient alone |
| Observe local proxy exports | Avoids one saved system-original bypass, but another injector can capture/replace those entries too | Requires known proxy identity and lifetime. Hooking both directions can create cycles; no automatic ownership guarantee | Medium cost; no polling needed; fail closed on ambiguity; not a fundamental general fix |
| Chaining-aware reattachment | Not with current hook API; patch chasing replaces one race with another | Blind disable restores stale bytes; recreating can invalidate originals retained by other managers or build recursive chains | High safety/coordination complexity; reject periodic repatching and hot-path ownership polling |
| Recover existing objects | A live factory can be instrumented for future creation; existing swapchain discovery is a different requirement | There is no current supported enumeration from HWND. Private memory scanning/unwrapping is not an acceptable fallback | Small if limited to validated shared factory tables; large/unknown for past swapchains; preserve fail-open core |
| Stable application-owned boundary | Can remove global export-chain dependence if a validated game-owned creation/presentation boundary exposes objects and queue | Needs new resolver/ABI evidence and a queue association contract; hooking arbitrary global ExecuteCommandLists is not equivalent | Highest new scope; possible later fallback, not smallest batch established here |
| Factory-table bootstrap hybrid | Removes reliance on factory export callback for implementations whose shared tables cover the actual dispatch | Reuses SDK-bounded registry; must prove implementation/table coverage, dedupe nested observations and retain callback/module lifetime | One bounded CPU bootstrap, no GPU/device/swapchain creation; moderate scope, optional failure remains isolated |

### Selected next batch: independent factory-table bootstrap

The current registry already patches selected slots in the **original shared table** and publishes originals before replacements. A bounded owned seed factory can provide a valid COM interface/table without waiting for an intercepted application factory return. This is not a dummy window/device/swapchain/GPU bootstrap.

[ReShade's factory proxy](https://github.com/crosire/reshade/blob/v6.8.0/source/dxgi/dxgi_factory.cpp#L206) normally delegates real swapchain creation through virtual methods on `_orig` (206–210,247–253). Thus a covered underlying table can observe that creation independently of the system export trampoline chain. This is source support for the proposed boundary, **not runtime proof that a seed returns every table used in the user's process**.

Proposed contract, not implementation:

1. Outside loader lock, acquire a bounded seed factory through a validated DXGI route; query widest public factory interfaces and install existing selected-slot observation. Preserve native arguments, HRESULTs and original-method ownership. Do not bypass wrappers by manually jumping into decoded private trampolines.
2. Make readiness mean covered creation dispatch, not `SafetyHook::enable succeeded`. Export-return discovery may supplement additional table coverage but must not be the only prerequisite for the known stack. Do not add a second inline patch on the same export merely to win ownership.
3. Seed objects are not presentation candidates. Release their owned COM references after setup; keep callback records and their implementation modules resident for the supported process lifetime. Never retain dangling private/per-instance table storage as if it were shared.
4. Real successful `CreateSwapChain*` observations alone supply the queue/device/actual swapchain association. Deduplicate wrapper/native nested observations. Preserve early observation → validated pending target → core readiness → successful Presents → delayed renderer/input activation, plus all FS-01–FS-05 protections.
5. Factory bootstrap failure/unsupported table topology leaves Overlay unavailable with a bounded reason, while camera core and native dispatch continue. No waits for wrapper stabilization, recurring retries, GPU work or synthetic input activation.

**Mandatory coverage gates:** public DXGI does not promise that all factories share one vtable. A seed that returns a proxy table may not cover another private implementation; an already-created useful swapchain cannot be recovered by creating another factory. Verify shared-table lifetime and actual wrapper delegation, and test a factory existing before bootstrap but creating its swapchain afterward. If the useful swapchain itself predates bootstrap, this proposal alone is insufficient. Stop that batch's reliability claim and require a validated live-object/application boundary; do not hide it with sleeps, guessed queues or memory scans.

Accordingly, this is the **smallest architecturally correct bounded implementation candidate**, not a claim that a universal deterministic design has already been proved. Full order independence for arbitrary injectors and arbitrary pre-ASI swapchain creation is not established by the available APIs/evidence. That distinction is material to the requested contract.

## 8. Regression and runtime acceptance

Deterministic offline fixtures for the proposed batch should establish:

- Both capture orderings in section 4, including foreign capture-before-our-enable and publication-after-our-enable. Verify native forwarding once and independent COM discovery even when the export callback is bypassed.
- Zero intercepted factory exports, but a seed covering a shared table used by a distinct pre-existing factory; its later actual swapchain creation must be observed.
- Different base/derived factory tables, widest-first queries, duplicate tables and repeated objects; modern creation slots remain covered.
- Nested proxy/native creation cannot publish the same target twice or recurse through our saved original. Failed creation preserves its exact HRESULT and does not activate anything.
- Bootstrap objects never become renderer targets. Queue/device association and two successful Present requirements remain unchanged.
- Unsupported per-instance/different table implementation, bootstrap failure and unknown prior swapchain: honest unavailable disposition, core initialization and native pass-through intact.
- SDK extent, publication order, module/table lifetime and no partial input/GPU activation on exception. Existing FS fixtures remain regression contracts, not rewritten characterization expectations.

Runtime matrix after authorization, using the canonical filename and one exact repair artifact:

| Scenario | Acceptance evidence |
| --- | --- |
| Ordinary Steam game | Actual target/Present coverage, startup hint, Delete opens/closes Overlay; healthy camera features |
| Current full Steam/ReShade/OptiScaler stack | Same canonical file works across repeated launches without deleting logs or changing stack; factory export callback bypass must not prevent actual COM observation |
| Same stack, log files retained vs fresh | Both conditions work; record PID/creation time so log ownership is not inferred from path |
| Factory already exists when bootstrap occurs | Later real creation is covered despite no factory export event |
| Multiple swapchains/devices/queues, resize/replacement | Correct actual target association, existing FS protections, no early input/GPU activation |
| Unsupported topology/bootstrap failure | Core remains AVAILABLE and INI usable; native rendering unaffected; Overlay reports unavailable rather than falsely ready |
| Useful swapchain exists before bootstrap | Explicitly demonstrate a supported recovery boundary, or report not covered; do not count this as PASS from a seed-only design |

A practical smoke sample is ten consecutive canonical launches of the previously nondeterministic full stack, with existing logs retained, plus the listed focused cases. That sample is a regression confidence check, **not statistical proof of all scheduler interleavings**; offline controlled ordering contracts are essential. Diagnostic evidence and later flag-off production evidence must identify their own artifacts. No such tests were run during this analysis.

## 9. Final answers

1. **Most likely:** the external factory wrapper's saved original depends on capture/publication ordering relative to our asynchronous SafetyHook arm. Missing historical creation remains a live secondary possibility.
2. **Broken invariant:** local hook installation success is mistaken for durable coverage of useful factory/swapchain dispatch.
3. **Why identical runs differ:** independent worker/loader/hook-manager scheduling can change the saved chain without changing the final visible export route. Filename and I/O can perturb that scheduling without semantic branches.
4. **Debugger before repair?** Not required to implement the bounded shared-table coverage candidate with deterministic fixtures. Required before claiming the exact historical writer/capture is proved, or if existing-object/table coverage cannot be established. A breakpoint should inspect saved original capture/selection as well as entry writes; another entry-byte snapshot alone cannot decide H1.
5. **One next approach:** independent factory-table bootstrap using the existing registry, with export callbacks supplemental and the accepted delayed activation gates preserved. Its coverage limitations are hard gates, not implicit fallbacks.
6. **Runtime proof:** the canonical full-stack repeated-launch matrix plus controlled ordering, actual creation/Present association, unsupported-path fail-open and explicit pre-existing-object coverage. A lone PASS or alternate filename is insufficient.

## Validation of this report

Current local source, build definitions, logger contracts, available FAIL traces and exact upstream implementations were inspected. Source/trace/inference boundaries and the earlier process-mixed ReShade chronology were reviewed. Documentation whitespace and changed-path scope were checked separately. No regression suite, production build or runtime validation was performed because this is an analysis-only task.
