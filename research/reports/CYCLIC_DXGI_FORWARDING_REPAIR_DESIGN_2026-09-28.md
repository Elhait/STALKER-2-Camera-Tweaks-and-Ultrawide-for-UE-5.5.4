# Cyclic DXGI forwarding: bounded repair design

## Status and decision

Design only, requested on 2026-09-28. No production code, tests, build definitions, configuration or binaries were changed. No game, debugger, regression suite, build, release or packaging operation was run. Existing working-tree changes were preserved.

The confirmed cycle is a forwarding defect, not a duplicate optional-observation defect. Extending `DxgiResizeNesting` to suppress additional Overlay work does not repair it.

**Recommendation:** accept an explicit, owned, acyclic continuation contract as the prerequisite for publishing Resize interception. A call-scoped forwarding guard may enforce that contract; it cannot manufacture the continuation. Preserve the existing FS-04 observation nesting separately. If a compatible continuation cannot be established before publication, leave the affected Resize dispatch unmodified and do not activate the Overlay renderer/input for that coverage domain.

**Implementation gate remains open:** current source and captured runtime evidence do not provide the missing authoritative native/wrapper continuation or an ownership contract for subsequent foreign changes. Neither a raw COM address nor an automatically generated SafetyHook trampoline establishes it. No full-functionality repair is approved by this design merely by naming a `safeOriginal` pointer. A generic repair satisfying all the requested semantics against arbitrary opaque foreign mutations cannot be guaranteed from the available callback ABI and saved pointer alone.

The non-publication fallback is an explicit loss of Overlay availability, not a successful Overlay compatibility repair. It is not being implemented or proposed as a silent substitute for fixing the working Overlay.

## 1. Current-source and runtime boundary

The [causal investigation](FRAME_GENERATION_HOOK_CHAIN_CAUSAL_ANALYSIS_2026-09-28.md) proves this graph in the captured process:

```text
H = Camera Tweaks ResizeBuffers body
E = saved system-DXGI method entry
A/B = active Steam/RTSS forwarding layers

H -> E -> A -> B -> foreign continuation -> H
```

The foreign continuation executes relocated Camera Tweaks prologue bytes and resumes inside H. Checking only whether H's public entry address is hooked would not protect this body re-entry. The captured graph contains no reached terminal native resize operation.

The captured public H entry is also hooked into the foreign chain. Consequently an application can already traverse A/B before its first H body entry, then H's saved-entry forwarding traverses them again. A correct carrier must account for that arrival provenance too; fixing only the second H entry cannot by itself promise exactly-once wrapper side effects for the complete application call.

Verified current source:

- `src/overlay/dxgi_hook_registry.hpp:42`: `Original(slot)` returns an entry copied into the registry vector. Lines 76-84 copy addresses, not downstream execution graphs. Lines 109-114 publish records before selected-slot patching; correct publication does not freeze the function code or foreign continuation state.
- `src/overlay/discovery_runtime.cpp:1130` and `:1159`: the two Resize callback bodies resolve those entries. Lines 1146 and 1176 invoke them outside optional-work handlers. Re-entry repeats the same invocation.
- `src/overlay/discovery_runtime.cpp:1037` and `:1085`: nesting suppresses inner setup and defers completion to the outermost resize. It intentionally does not select a different native continuation.
- `src/overlay/discovery_runtime.cpp:1353` and `:1358`: Resize dispatch is currently introduced by replacing shared-table slots 13 and 39 with Camera Tweaks callbacks.
- `src/overlay/dxgi_resize_nesting.hpp`: the 16-slot limit bounds tracked identities, not recursive forwarding depth. Increasing or limiting depth would not recover the missing operation.
- `tests/overlay/dxgi_callback_harness.cpp:304`-338: FS-04 exercises finite ResizeBuffers1 -> ResizeBuffers and authoritative outer results. It does not exercise a mutable saved entry routing back into the same callback body.
- Vendored `external/safetyhook/safetyhook.cpp:511`-612 copies/relocates the entry prefix and emits a continuation into the remaining target. An existing JMP into Steam is preserved as such; foreign code/data behind it remain mutable. `external/safetyhook/safetyhook.hpp:683` also shows that the convenience `stdcall` call holds its hook mutex across the original invocation: it must not be adopted as a re-entrant forwarding solution.

The hook writer/order that formed the foreign continuation is still unknown. The captured cycle does not prove that an inline-only installation would necessarily prevent its formation.

## 2. What a local guard can and cannot guarantee

If the only available callable is E and E currently leads back to H, the local choices are: call E again, avoid making the native call, change the dispatch graph, or use a separately established continuation C. The first preserves the cycle. The second cannot provide the result of an operation that was never executed. The third needs a hook-ownership/semantics proof. The fourth needs a real source and lifetime for C.

Disabling the renderer, turning off diagnostics, catching C++ exceptions, or recording a recursion marker does not add C to this graph. Restoring the current COM slot also does not undo a foreign trampoline which already resumes H's body.

There is a second information limit: same thread + same object + same method + identical arguments + active outer call is not an exact cycle predicate. A legitimate wrapper can make one finite nested retry with those same values and then proceed using its internal state. Its first re-entry is observationally identical to the cycle at our ABI boundary. Changed arguments are not proof of progress either: a cyclic wrapper can modify them on every lap.

Therefore a reliable distinction needs **continuation provenance and logical-call/progress ownership**, not only a depth counter, return-address heuristic, argument comparison or module name. A new token assigned to every callback entry would label every cycle lap as a new request and provide no protection.

## 3. Alternatives

### A. Keep raw originals; add TLS recursion detection

- **Authority:** still E, the mutable saved method entry. There is no new continuation.
- **Nested calls:** cross-method and unrelated-object nesting can be counted; finite same-method retry cannot be distinguished exactly from cyclic re-entry using those fields alone.
- **Later re-hook:** still changes what E executes without updating the registry vector.
- **Wrapper semantics:** preserved only while forwarding remains acyclic. Returning E_FAIL/S_OK, skipping resize, or replaying a previous result on re-entry breaks the required call/result contract.
- **Lifetime/thread safety:** TLS avoids a global recursion lock but neither pins foreign trampolines nor preserves their mutable original slots. A recursion counter does not bound RTSS work already entered.
- **New failure:** either the original infinite cycle survives or a fabricated result/omitted resize replaces it.
- **Fixture:** the exact closed graph must reject this as a complete repair, even if optional observation runs only once.
- **Disposition:** useful diagnostic/enforcement component only after continuation authority is solved; not a standalone fix.

### B. Freeze the captured entry prefix in a SafetyHook trampoline

- **Authority:** owned relocated prefix plus the target it continues into, not an immutable snapshot of the whole chain.
- **Nested calls:** must invoke a leased typed continuation without holding the hook mutex across native work. FS-04 must remain separate.
- **Later re-hook:** overwriting the original entry can leave the copied prefix usable, but an already-copied branch into A/B, a mutable foreign downstream slot, or modification beyond the copied bytes can still route back into H.
- **Wrapper semantics:** existing layers are preserved only if their continuations remain valid. Copying pristine native bytes instead silently changes this into the bypass option below.
- **Lifetime/thread safety:** own executable storage and its unwind/ABI requirements; pin all actual executable owners and lease private trampolines. Pinning a DLL does not prevent its hook manager from freeing a relay allocation.
- **New failure:** a stable-looking trampoline still reaches a mutable cycle, or a stale/freed tail. StartDisabled is a publication tool, not a proof that the tail is acyclic.
- **Fixture:** mutate a fake foreign downstream slot after snapshot creation; unchanged snapshot address must not be treated as sufficient proof of safety.
- **Disposition:** possible continuation carrier, not sufficient authority by itself.

### C. Replace only Resize shared-table patches with inline method-entry hooks

- **Authority:** per-method inline-hook continuation, with the same restrictions as B.
- **Nested calls:** distinct method ABIs and binding leases are required; legitimate ResizeBuffers1 -> ResizeBuffers remains possible.
- **Later re-hook:** foreign managers can replace the same entry, reuse mutable handler state, or capture our detour. A prologue trampoline still cannot guarantee the global chain remains acyclic.
- **Wrapper semantics:** can be preserved under a compatible chain manager. It may prevent the specific aliasing pattern in which a foreign manager treats our published callback as another native implementation, but that formation mechanism has not been proved by writer evidence.
- **Lifetime/thread safety:** per-address deduplication, shared native implementation with multiple tables, entry-patch publication and process-resident trampoline ownership would be necessary. Unobserved objects sharing the hooked implementation also enter the detour and must have a valid native continuation even without a table record.
- **New failure:** global coverage changes, foreign overwrite, publication races or the same cycle through a copied wrapper branch. Inline-hooking our callback body itself is not a continuation repair.
- **Fixture:** cover both capture orders and a later shared-handler downstream rewrite, not merely an immutable Native -> H detour.
- **Disposition:** bounded architectural candidate for the two Resize methods only if evidence establishes its ownership contract; not yet a guaranteed repair.

### D. Re-read the live COM slot, temporarily restore it, or re-arm our hooks

- **Authority:** current table entry; it may be H, a foreign detour, or the same cyclic E.
- **Nested calls:** restoration does not change captured continuations into H's body. Re-reading can also select a new epoch midway through an outer operation.
- **Later re-hook:** repeated competing writes make ownership timing-dependent again.
- **Wrapper semantics:** temporary restoration can bypass a foreign wrapper or erase its newer publication; re-hooking can capture H as its own original.
- **Lifetime/thread safety:** shared tables affect unrelated instances/threads. A compare-and-swap only verifies a slot value, not downstream code/data or in-flight callbacks.
- **New failure:** hook war, wrong originals, stale epochs, native bypass and the original cycle.
- **Fixture:** table restoration with an already-captured continuation into H must still reproduce re-entry; concurrent foreign publication must not be overwritten.
- **Disposition:** reject as a repair. No blind re-hooking or unhook-around-call.

### E. Clone an object's table or add a COM proxy

- **Authority:** saved methods of the old table/underlying interface, still subject to the same downstream problem unless continuation ownership is independently solved.
- **Nested calls:** queried interfaces and canonical identity can traverse both tables/layers; method-specific forwarding still needs valid continuations.
- **Later re-hook:** can capture a clone/proxy callback too, or replace a different interface table.
- **Wrapper semantics:** a full proxy must preserve QueryInterface, IUnknown identity, private methods, native interface pointers and wrapper behavior. An SDK-sized clone can truncate foreign private tails.
- **Lifetime/thread safety:** object vptr mutation, table prefix/extent, destruction, multiple QI tables and proxy reference ownership become new responsibilities. The vendored VmtHook infers table length by executable-pointer scanning; that is not a bounded foreign-table extent contract.
- **New failure:** private-tail/identity/lifetime corruption without a guarantee against the original cycle.
- **Fixture:** distinct QI tables, private tails, replacement and foreign capture of proxy callbacks would all be required.
- **Disposition:** out of scope and not a minimal fix.

### F. Use a verified native or wrapper-suffix escape on proven cycle closure

- **Authority:** an independently obtained continuation C belonging to the correct implementation/interface layer. It must resume after the already-executed wrapper prefix, preserving any remaining suffix. The system DLL basename or a method address is not enough.
- **Nested calls:** on a proved return along the same consumed edge, call C rather than E. Do not apply this rule to all same-method nesting. Pass the current incoming arguments, including any legitimate changes already made by a wrapper, not cached outer arguments.
- **Later re-hook:** C must remain valid for a leased call epoch. New foreign suffix layers must not silently be omitted; opaque changes without lifetime/progress ownership invalidate this guarantee.
- **Wrapper semantics:** A/B already entered once need not be entered twice. Their real return processing must still execute. But going straight to N is correct only if there are no required unexecuted suffix layers, no wrapper-specific self conversion, and no other wrapper protocol that C skips. That proof is missing here.
- **Lifetime/thread safety:** an internal native body is not a public export. Module pinning alone does not prove callable entry shape, private relay lifetime, compatible self or immutable tail code/data. Scanning vendor memory or copying pristine DLL code is not approved.
- **New failure:** wrong implementation/self, skipped suffix, freed trampoline or forged success if C cannot be obtained.
- **Fixture:** include a suffix wrapper Cwr after the re-entry point. A shortcut to N that skips Cwr must fail, even when N returns S_OK. Also test wrapper pre/post order and wrapper-adjusted final HRESULT.
- **Disposition:** potentially the smallest call-site change, but conditional on explicit continuation and cycle-classification proof. No unconditional direct-system bypass.

### G. Explicit leased continuation + publication gate

- **Authority:** a chain owner/compatible adapter supplies an executable continuation and its lifetime, semantics and call-epoch contract. It identifies which forwarding prefix has already been consumed and the remaining suffix. This is not an assumed Steam/RTSS API; no such usable provider has been established in the current source/evidence.
- **Nested calls:** distinct method/operation edges remain legal; a repeated edge for the same active logical request resumes its proved successor, never restarts the consumed prefix. Legitimate same-method requests require independently established new-request/progress evidence.
- **Later re-hook:** a supported owner publishes a new immutable epoch for future calls while retaining the old callable state for in-flight leases. An opaque foreign mutation of leased state is outside this contract; pointer/entry-byte checks cannot make it supported.
- **Wrapper semantics:** prefix order and all remaining suffix work are preserved. The callback returns the result received from the actual outer chain, including its legitimate recovery/error translation.
- **Lifetime/thread safety:** publish binding before callback exposure, retain code owners and private allocations for the epoch, and avoid locks across native calls. No persistent swapchain/backbuffer ownership is added.
- **New failure:** falsely claiming an opaque chain has this contract. That is a failed admission gate, not a reason to create a default original or retry.
- **Fixture:** controlled fake chain epochs, explicit progress/new-request events and suffix ownership can prove the forwarding rule. A production acquisition fixture must independently prove that its real mechanism supplies those properties.
- **Disposition:** recommended correctness contract; acquisition gate unresolved. Non-publication is the safe pre-commit result if the gate cannot be met.

## 4. Exact invariants for the conditional minimal repair

These describe the required behavior, not already-available production capabilities.

1. **Continuation authority:** before a Resize callback is reachable, a binding owns a typed callable continuation for that method and interface layer. Its evidence covers executable/private-allocation lifetime and the remaining wrapper suffix, not only table/code address readability.
2. **No repeated edge:** within one uncompleted logical resize request and leased chain epoch, an owned forwarding edge may be consumed once. Returning through that same edge must advance to its proved successor, never invoke the consumed entry again. Successors must progress toward a terminal operation under the chain contract.
3. **Finite nesting is not cycle closure:** ResizeBuffers1 -> ResizeBuffers is a distinct method edge; another object/binding is a distinct dispatch edge. A same-object/same-method finite request is legitimate only when the chain authority establishes a new operation or actual continuation progress. Equal/different arguments, thread depth, caller module or a new locally fabricated token do not establish either fact. Cross-thread delegation similarly needs explicit operation ownership; TLS alone does not solve it.
4. **Forwarding authority survives UI failure:** forwarding selection must not depend on diagnostic logging, optional identity-query success or renderer readiness. H's body performs selection, since foreign code can bypass its entry prefix. Native calls remain outside optional-work catches.
5. **Exact ABI and result:** forward self, every scalar and both ResizeBuffers1 pointer arguments unchanged from that invocation. Do not normalize zero/UNKNOWN, replace node/queue arrays, convert ResizeBuffers1 into ResizeBuffers, or return a leaf/cached/synthetic result in place of the outer chain's actual result. A wrapper's legitimate argument/result transformation remains its own responsibility.
6. **Observation nesting is separate:** retain `DxgiResizeNesting`/FS-04 by canonical identity. Only the outermost matching result commits discovery, pending descriptor/queue refresh and renderer disposition. Inner failures recovered by the wrapper must not erase evidence. A terminal forwarding-integrity failure cannot be reversed by an outer S_OK making Overlay ready again.
7. **No native-call lock:** registry, renderer, evidence, logging and hook-owner mutation locks are not held across forwarding. An immutable leased binding with nonallocating call-scope bookkeeping is preferable to holding a SafetyHook call/reset mutex through a re-entrant method.
8. **No extra COM ownership:** a continuation binding owns method/chain lifetime, not a persistent swapchain or backbuffer reference. FS-01 replacement cleanup and safe retention of unproved in-flight GPU resources stay unchanged. Canonical identity used by optional observation is not substituted for the ABI's self pointer.
9. **Pre-commit fail-open:** if authority or coverage is unavailable, do not publish Resize callbacks for that shared-table/implementation coverage domain. Do not retain/activate an Overlay renderer or input capture that needs unobserved resize cleanup. Leave native dispatch as it was; camera core remains available.
10. **Post-commit limitation is explicit:** `DisableOverlay()` cannot repair an already-cyclic saved continuation. Safe retirement requires a still-valid leased continuation and a quiescent ownership protocol; no restoring foreign slots or freeing in-flight hooks. If an opaque owner can rewrite/free that continuation without cooperation, unconditional runtime fail-open is not established and that ownership model must not be admitted as safe.

The exact requested guarantee is therefore **no cycle introduced/repeated by our forwarding under an established continuation contract**, not bounded execution of an arbitrarily faulty foreign/native operation. RTSS can still spend its own time in a valid wrapper invocation; no local guard can guarantee the duration of opaque code it calls.

## 5. Smallest implementation boundary, if gates pass later

Only the two Resize bindings/forwarding call sites and the necessary installation/activation coverage checks belong in this repair. Do not redesign factory bootstrap, exports, Present, camera features, localization, UI or the validated startup lifecycle.

- Prepare continuation authority before replacing either Resize slot. A binding conceptually contains method identity, implementation/coverage identity, chain epoch, entry continuation, proved re-entry successor and lifetime lease. Merely wrapping E in this record is insufficient.
- Commit the binding before exposing the callback, using the existing publication principles. Shared-table coverage must be decided table-wide; an unsupported second object cannot undo a patch used by existing objects. Partial initialization must not publish an unprotected Resize body.
- Keep forwarding state separate from the UI's nesting state. At a callback body, acquire a binding lease, establish the owned edge scope and select entry or proved successor. Run optional preparation, call the selected continuation once with the current arguments, then run optional outer-disposition work and return the actual result. No native retry is added.
- On proved cycle closure, mark the affected Overlay path unavailable through bounded existing cleanup and finish the native request through the owned successor. Do not redo pre-resize cleanup on the same logical outer scope. Do not skip an unexecuted suffix.
- If the first H body entry already arrives as a foreign downstream continuation, select the successor after that consumed prefix immediately. Do not require one additional cyclic lap to discover what an authoritative carrier already knows. A TLS flag set only by our own outbound call cannot establish this first-arrival provenance.
- If this needs a new generic hook-chain manager, vendor-memory archaeology, a COM proxy or cooperative APIs that the actual stack does not provide, stop this batch and report the failed acquisition gate. Do not build a larger architecture to make the design's abstraction appear available.

## 6. Deterministic regression design

### Model the captured topology, not just nested calls

Construct SDK-typed fake Resize methods and a mutable entry dispatcher E. Initially E resolves to a finite implementation. Publish H with E as its saved address. Then mutate E to foreign A, A to foreign B, and B's continuation to a **body bridge into H**. The body bridge bypasses any entry-only checks, matching the captured relocated-prologue continuation.

```text
entry -> H -> saved E -> A -> B -> body bridge -> H
```

The repaired authorized branch must become:

```text
entry -> H -> saved E -> A -> B -> body bridge -> H
                                                 -> proved suffix -> N
                              <- actual returns <-
       <- outer chain result <-
```

Do not silently alter B to call N directly in the fixture: that would delete the defect being tested. A watchdog or bounded *test-only* dispatch budget may turn an unrepaired infinite cycle into a failing child-process test; it must never be the production mechanism or manufacture a passing HRESULT.

### Required assertions

| Case | Required result |
| --- | --- |
| Exact closed graph, owned successor available | H body entered twice; saved E/A/B consumed once for that logical request; intended suffix and N each executed once; callback returns within the deterministic dispatch budget without polling/spin |
| Native S_OK and multiple native failures | Identical current arguments at N; exact actual outer result returned, including E_ACCESSDENIED, DXGI errors and legitimate wrapper result translation |
| Wrapper side effects and suffix | A/B pre/post order preserved; remaining suffix executes exactly once; a direct-N shortcut that skips it fails |
| Public H entry already wrapped | Start at A/B -> H body, not only direct H; arrival provenance prevents re-running that consumed prefix; intended native count and complete wrapper side-effect order remain correct |
| Same cycle after later re-hook | Supported new epoch handles future calls; active old lease stays valid; opaque unleased mutation is rejected by the authority gate, not claimed supported |
| Prefix snapshot still points to a mutable foreign slot | Snapshot alone fails the authority contract; unchanged trampoline address is not accepted as acyclicity evidence |
| ResizeBuffers1 -> ResizeBuffers | Both native method invocations occur according to wrapper semantics; node/queue pointer identity and contents are untouched; no false same-method classification |
| Legitimate finite same-method nesting | Include identical arguments and changed arguments, with a proved distinct operation/progress edge; execute every intended native request, no depth-based skip or result reuse |
| Cross-method cycle | ResizeBuffers1 -> ResizeBuffers -> return to the first ResizeBuffers1 edge cannot evade the logical-request guard by alternating methods |
| FS-04 inner failure / outer success | Native inner failure is visible to the real outer wrapper; outer recovery retains candidate evidence; two later successful Presents revalidate normally |
| FS-04 inner success / outer failure | Exact outer failure returned; only outer completion invalidates affected evidence |
| Different chains, QI aliases and concurrent threads | No shared global re-entry flag; ABI self is preserved; identity-level UI nesting remains correct; independent requests do not suppress one another |
| Missing continuation before publication | Resize slots unchanged; no renderer/backbuffer ownership or input activation for uncovered domain; camera core availability unaffected |
| Callback publication and lifetime | Immediate callback can acquire the fully prepared binding; in-flight epoch survives supported replacement; no DLL/private-relay lease released while executing |
| Optional setup/completion/logging throws | Real forwarding and result unchanged; no diagnostic-dependent registration, exceptions escaping optional boundary or second native invocation |
| Already-disabled Overlay | Native forwarding guard/continuation still valid; no UI recapture, repeated teardown or outer-result resurrection |

Native counts are defined **per intended operation**, not as a universal one-call count for the whole outer resize family. The simple cyclic case has one terminal operation. A legitimate nested retry/conversion can intentionally invoke two methods or multiple operations; preserving that is part of the test, not a regression failure.

### Integration and evidence levels

1. A graph-only fixture validates the forwarding rule and the indistinguishability counterexample. It cannot prove a real continuation-acquisition method exists.
2. An actual callback fixture uses the real repaired H bodies and registry/binding publication to exercise the mutable E/A/B/body-bridge graph, exact arguments/results and FS-04. Register it through the current runner only in the later implementation batch.
3. A platform carrier fixture must validate the selected actual trampoline/continuation mechanism, supported mutation ordering, lifetime and ABI. Do not call a synthetic provider a production proof.
4. Re-run existing `dxgi_callback_harness`, `renderer_lifetime_harness`, `discovery_evidence_harness`, `optional_overlay_startup_harness`, full `test.cmd`, production build and whitespace checks only after implementation is separately authorized. Existing FS-01/02/03/05 contracts must remain passing.
5. Runtime confirmation remains separate: the exact previously failing native-FG resize/replacement scenario must return, preserve foreign overlays and keep camera core available. No new runtime experiment was performed in this design task.

## 7. Evidence needed to choose an implementable carrier

Do not repeat the broad FG/DXGI investigation. The unresolved question is specifically: **which callable represented the real remaining resize operation before the foreign downstream was redirected into H, who owns it, and does that owner provide a valid in-flight/later-re-hook contract?**

Existing evidence proves the final cycle but not that acquisition contract. A focused inspection of the implicated continuation's formation and owner can decide between a proved inline-chain repair, a proved suffix continuation, or an unavailable-Overlay admission result. Reading an arbitrary old executable relay and finding it readable is not enough to adopt it in production.

No Sleep, retries, filename rules, Steam/RTSS disabling, native HRESULT fabrication, entry-only recursion guards, hook fighting or general Frame Generation redesign are authorized by this design.

## Primary references and limits

- Current vendored SafetyHook and current repository callbacks are the implementation authority; upstream [InlineHook source](https://github.com/cursey/safetyhook/blob/main/src/inline_hook.cpp) is supplementary, not a version-equivalence claim.
- Microsoft [ResizeBuffers](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers) documents preservation parameters and the requirement to release outstanding backbuffer references. This is why disabling only Resize observation while continuing to own/render buffers is not a safe admission fallback.
- Microsoft [ResizeBuffers1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-resizebuffers1) documents per-buffer node/presentation-queue inputs. A fallback must not silently collapse this ABI into ResizeBuffers.

No offline PASS, runtime fix or universal wrapper compatibility is claimed. The deliverable is the bounded design, admission/continuation invariants, alternatives and regression contract above.
