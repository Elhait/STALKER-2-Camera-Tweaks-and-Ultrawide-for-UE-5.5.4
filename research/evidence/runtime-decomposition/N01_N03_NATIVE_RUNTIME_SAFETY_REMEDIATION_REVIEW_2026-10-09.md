# N01–N03 Native Runtime Safety Remediation Review

Date: 2026-10-09. **Review and remediation design only; implementation requires separate approval.**

## Scope and evidence boundary

This report follows `NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md` and reviews
N01, N02 and N03 against the current dirty workspace. It does not implement
remediation, approve B5, change decomposition decisions, or certify game execution.
Production/vendor code, tests, configuration, implementation plan, move ledger
and release artifacts are unchanged. No tests/builds, game launch or ASI installation
are performed for this review. Earlier PASS counts belong to the preceding audit,
not to fault-injection validation of these findings.

Acceptance: identify actual failure paths and preconditions; distinguish static
reachability from reproduction; define bounded fixes, tests, risks and proof limits;
stop for implementation approval. Repository-relative source references below
refer to this snapshot, not to an inferred release tag.

| Reviewed source | SHA-256 |
| --- | --- |
| `src/plugin/runtime.cpp` | `CDA9E9CEA372965623B8ADD110AD741CBE7EEB28690E808E3797EC963108A9DF` |
| `src/plugin/dll_entry.cpp` | `DCC32DC209DB8A45D90190C43E25CB29EED48BD8927624B74C4C29460AFBBDCF` |
| `external/safetyhook/safetyhook.cpp` | `3C2C7FCF6FB5D0E578FD24A7155B255D01DAB653BF6A2E408D84CEAB8D4AA1F8` |
| `external/safetyhook/safetyhook.hpp` | `708D22B42C6BAC62E21BA86A3FCC5D8E89DBD156B09629EB81D58D6A07EACB3E` |
| `src/diagnostics/performance_telemetry.cpp` | `7ED6CDE08783FC791E681AE406F035A0405D932AFC80DD513859BB0863C7B509` |

## Decision summary

| Finding | Source-level result | Severity / scope | Recommendation |
| --- | --- | --- | --- |
| N01 | Failed patch operation can leave target patched while executable ownership is released. Failed enable is also affected, not just disable. | P1, conditional native memory-safety failure during activation/rollback/reset; no measured production protection failure or reproduced crash. | **Fix**, with complete patch-state and executable-ownership handling; not a one-line destroy change. |
| N02 | Zoom IN/OUT register raw callbacks, bypassing existing exception containment. Mutex acquisition is exception-capable. | P1 boundary-hardening gap, including Diagnostics OFF on the supported executable; no demonstrated ordinary lock failure. | **Fix** via existing `SafeMidHookEntry`; preserve partial telemetry and native continuation. |
| N03 | Path/stream setup precedes first try; initialization thread lacks outer catch. Failure-handler reset can itself throw. | P1 conditional startup/partial-initialization containment gap; not evidence of routine startup crashes. | **Fix** outer containment and guarded cleanup; reliable native teardown depends on N01. |

## N01 — patch failure and executable lifetime

### Actual source path

`external/safetyhook/safetyhook.cpp`:

- `InlineHook::create` (436), move assignment (460), destructor (485), `reset`
  (488), `enable` (689), `disable` (732), `destroy` (748).
- `MidHook::create` (832), move assignment (858), `reset` (871).
- `Allocation::operator=` (24), destructor (39), `free` (43);
  `Allocator::global` (57), `Allocator::free` (87), `Allocator::Memory` destructor (282).
- Windows `trap_threads` (1406): queries/protects pages, invokes patch writer,
  restores protection, then returns a boolean.

`trap_threads=false` conflates failure before the byte write with failure after
the byte write. `enable` updates `m_enabled` only after a successful return.
`destroy` ignores the disable result and frees the trampoline. `reset` and move
assignment reach destruction as well. Default-enabled creation can destroy a
temporary hook after returning an activation error.

| Operation / failure point | Target bytes | Bookkeeping / destruction consequence |
| --- | --- | --- |
| Enable: query/protect failure before writer | Original bytes remain | No target-to-trampoline reference created by this attempt; protection rollback may still fail. |
| Enable: protection restoration failure after writer | Jump already installed | `m_enabled` remains false; `disable` can return success without restoring bytes; destruction releases referenced storage. |
| Disable: query/protect failure before writer | Installed jump remains | Error returned while enabled; destroy ignores it and releases referenced storage. |
| Disable: protection restoration failure after writer | Original bytes already copied | Not automatically a remaining-target-jump UAF; unresolved protection and in-flight execution still require separate handling. |

The restoration expression uses `&&`; failure restoring the destination can
prevent attempting restoration of the other range. Protection failure is not
equivalent to proof that either range remains safe to execute. Trap setup also
uses allocation/container operations; exceptional exits need cleanup analysis,
not merely handling a returned boolean.

Lifetime extends beyond `m_trampoline`: a mid-hook also owns executable `m_stub`.
Its default destructor (`safetyhook.hpp:986`) destroys members in reverse order;
the declared `m_stub` follows `m_hook` (private member declarations), so stub release precedes
inline-hook destruction. This makes complete bundle lifetime/quiescence essential;
it is not, by itself, proof of a reproduced concurrent destructor crash.
The allocator's global reference is weak. Freed allocations can enter a reusable
free list, and allocator destruction releases backing pages. Merely leaving a raw
address intact does not retain executable memory.

### Preconditions and production reachability

An OS protection/query failure or an exceptional patch operation is required;
normal successful enable/disable is not the reported failure. Relevant callers:

| Production path (`runtime.cpp`) | Reachability |
| --- | --- |
| `InstallZoomTransitionHooks`, 5295–5334 | Supported hash/instruction checks allow default-enabled `create_mid`; error temporaries can be destroyed. |
| Gameplay installation, 6465–6486 | StartDisabled hook is moved to resident owner, then enabled; failed enable leads to reset in local failure handling. |
| `CommitCinematicComponents`, 3498 | Ordered aspect/ENTER/EXIT activation; failed commit attempts disable, with discarded results. |
| `RollbackCinematicFovComponent`, 3514; aspect restoration, 3040 | Rollback/reset reaches vendor teardown; noexcept wrapper is not exception recovery. |
| `ResetAllRuntimeResources`, 745 | Explicit failure/reset cleanup resets hooks after worker-stop handling. |

Therefore this is not limited to unsupported DLL unload. Resident runtime ownership
does not prevent activation-failure resets. Joining plugin workers does not prove
that all native game callback/trampoline frames are quiescent. No current log proves
that these conditional OS failures occurred in the user's game.

### Upstream comparison

`THIRD_PARTY_NOTICES.md` identifies SafetyHook but does not pin the exact local
vendor revision. The local checked boolean patch API differs from upstream.
The [release listing](https://github.com/cursey/safetyhook/releases) identifies
v0.7.0 as the latest release observed during this review.
Its [inline-hook implementation](https://raw.githubusercontent.com/cursey/safetyhook/v0.7.0/src/inline_hook.cpp)
ignores the disable result before freeing; its [Windows implementation](https://raw.githubusercontent.com/cursey/safetyhook/v0.7.0/src/os.windows.cpp)
does not check the relevant protection results. The inspected
[main inline-hook source](https://raw.githubusercontent.com/cursey/safetyhook/main/src/inline_hook.cpp)
and [main Windows source](https://raw.githubusercontent.com/cursey/safetyhook/main/src/os.windows.cpp)
do not supply the required checked-failure ownership solution either.
Main is a mutable observation, not a pinned commit. This comparison does not
establish the local version or exhaust all upstream proposals. A blind upgrade is
not a demonstrated fix for the local failure path.

### Minimal safe remediation design

1. Distinguish byte-write commitment, protection-restoration result and uncertain
   state. A single enabled boolean cannot encode the actual patch outcome. Mark
   possible target reference before a write; clear it only after verified removal.
2. Attempt both protection restorations independently; maintain exception-safe
   trap/protection cleanup. Do not treat failed cleanup as a successful detach.
3. Never free/reuse the whole executable bundle (stub, trampoline and backing
   allocator ownership) while a target or supported in-flight execution can use it.
   On uncertainty, retain it process-resident and report incomplete teardown.
4. Retention must survive temporary-create failure, move assignment, reset and
   member destruction. Provision retention ownership before activation so OOM
   handling does not depend on allocating a quarantine entry after failure.
5. Prevent uncertain objects from being overwritten/re-enabled or retried without
   a proven recovery/quiescence contract. Keep the runtime owner and correction
   gate single; do not introduce a second runtime manager.

This is a bounded vendor-lifetime correction, not a hooking architecture rewrite.
The concrete no-throw retention representation must be settled in the approved
implementation. Skipping `m_trampoline.free()` alone is insufficient: member
destructors still release it, and the mid-hook stub remains a separate dependency.
Checking only plugin reset sites misses default-enabled temporary destruction.
Containment cannot retroactively undo freed executable storage.

### Deterministic fault-injection design

Use a fresh fixture-owned Windows subprocess, private RX target/trampoline pages,
and actual local SafetyHook. Inject query/protection outcomes at test-build-only
dependency call sites, not global Windows hooks or a live game patch.

| Case | Required assertions |
| --- | --- |
| Enable failures before each write | Original target; no false success; protection attempts recorded; safe ownership disposition. |
| Enable failure after jump write | Target jump recognized despite failed return; complete bundle retained through create error/reset/destructor. |
| Disable failure before restore write | Jump remains; no free-list reuse or backing-page release; teardown reports incomplete. |
| Disable failure after original-byte restore | Distinguish restored bytes from restored permissions; both restoration attempts made; no premature release. |
| Move/reset/default-create failure | Old/temporary uncertain ownership survives all destructors; no replacement loses retained resources. |
| Exception in trap setup/cleanup | No exception escapes noexcept destruction; traps/protection/ownership have explicit failure disposition. |
| Normal lifecycle / safe recovery | Original instruction continuation works; ordinary successful cleanup does not leak. |

Record bytes, patch state, protection-call ordering, allocator frees/reuse and
bundle retention. Restore fixture permissions safely for inspection; do not call a
potentially dangling entry to prove a crash. Separate a shim reporting failure
after a real successful restore from true permission failure on nonexecuted fixture
pages. Parent timeout may terminate only its own child. A concurrent fixture must
prove its own quiescence; it is not proof of game-thread quiescence.

Fix risks: bounded retained memory, accidental unbounded retries/resource growth,
changed error semantics, and overclaiming that closed correction flags stop every
callback. Native gate/ownership behavior must remain unchanged. Local automation
can prove injected outcomes and ownership; actual game-address wiring and live
concurrency remain runtime gates. Recommendation: **Fix**, not Not reproducible;
static reachability is sufficient to prioritize it, but not to claim a crash occurred.

## N02 — Zoom callback exception containment

### Registration, exceptions and context

`runtime.cpp:5291–5292` defines `TraceZoomIn/Out`; `InstallZoomTransitionHooks`
registers them directly at 5321/5328. Supported hash and decoded-instruction checks
restrict installation, but Diagnostics OFF does not disable this callback path.
`SafeMidHookEntry` at 1392 is the existing noexcept try/catch wrapper used elsewhere.

| Callback operation | Exception / side-effect assessment |
| --- | --- |
| `TraceZoomTransition` (5223) calls `MarkCameraStateFreshness` (4118) | `ScopedRuntimeMutex` constructor (`performance_telemetry.cpp:111`) locks in both diagnostic modes; mutex lock may throw `std::system_error`. This is a concrete exception-capable operation, not proof of a normal failure. |
| Freshness snapshot update / Overlay wake | Freshness can already be published before a later failure. `NotifyOverlayCameraStatePublished` is noexcept. |
| Zoom counters/validity/activity/sequences and sample fields | Telemetry can be partly updated before completion; not a transaction with automatic rollback. |
| `SafeRead` (2011), platform memory reads | Query + copy is not an allocating C++ exception source shown here. A pointer lifetime race/access violation is not contained by ordinary C++ catch under the current exception model. |
| Diagnostic `Log` (880) | Existing noexcept containment; do not invent an uncontained formatting failure on this path. |

Zoom reads context/register values and memory; it does not write XMM, GPR or RIP.
The relocated original UCOMISS must still execute after callback return. A mutex
destructor termination, SEH fault or stack overflow is not repaired by catch.
No demonstrated normal lock-failure/reentrancy scenario is asserted here.

### Minimal design and tests

Register both existing callbacks through `SafeMidHookEntry<&TraceZoomIn>` and
`SafeMidHookEntry<&TraceZoomOut>`. Do not mark the throwing callback body noexcept,
skip the original instruction, reset hooks inside catch, or add telemetry rollback.
For this read-only-context callback, containment preserves input context and native
continuation; it does not promise that partially published telemetry is rolled back.

Tests must exercise actual Zoom registration and production callback, not merely a
generic throwing function passed to the wrapper. Use controlled dependency throws
before/after freshness mutation; do not induce undefined mutex ownership or a real
deadlock. Compare complete context sentinels, verify no escape and expected partial
telemetry, and verify native continuation in a fixture-owned hook. Run IN/OUT with
Diagnostics OFF/ON and normal/exception paths. ABI/register preservation in a local
fixture does not validate the game's camera association or all CPU flag consumers.

Risk: hiding callback failure without diagnosing partial telemetry; avoid unbounded
error logging in hot paths. No rollback is warranted without a separate publication
contract. Recommendation: **Fix**, using the existing boundary rather than a new API.

## N03 — startup boundary and partial cleanup

### Exception-boundary map

| Boundary | Current behavior / gap |
| --- | --- |
| `dll_entry.cpp:5–34`, `DllMain` attach | Fixed-buffer/noexcept attach diagnostics, module assignment and Win32 thread creation; no demonstrated allocating C++ operation in the inspected body. CreateThread failure is a separate returned-error path. Do not move initialization or cleanup under loader lock. |
| `runtime.cpp:6688`, `InitializeThread` | Calls `Initialize` directly, then success/readiness handling; lacks an outer catch. This is not `WorkerLifecycle::ThreadStartThunk`, whose separate catch does not protect startup. |
| `Initialize`, 6323–6332, before first try | Filesystem path construction/concatenation/copy, default config path and stream construction can allocate/throw. Ordinary failed Win32/file return values are not automatically C++ exceptions. |
| First logger try, 6333 onward | Logger creation/configuration caught locally; partial logger setup can remain, but hooks/workers are not yet installed here. |
| Main startup try, 6350 onward | Stop event, config/hash, settings publication, optional Overlay, hook transactions and workers can be partly initialized before failure. Local/leaf catches cover subsets, not the entire entry. |
| Main catch handlers, 6572 onward | Call `ResetAllRuntimeResources` outside a nested guard. Vendor/reset locking or allocation can throw; a sibling catch does not catch an exception from a catch handler. |
| `ResetAllRuntimeResources`, 745 | Closes stopping/readiness/correction flags, stops workers, resets hooks. Worker-stop failure retains resources and returns; return is not proof of complete cleanup. N01 still applies to hook reset. |
| Process detach, 6717 onward | Process-terminating path does not reset hooks; other detach signals stop. Preserve process-resident lifetime and no loader-lock joins. |

`Log` is already noexcept. In the checked spdlog implementation,
`logger-inl.h:148` catches sink flush exceptions through `SPDLOG_LOGGER_CATCH`;
ordinary sink errors are not established as uncontained startup exceptions.
Exception handling itself can still fail under allocation failure, but N03 does not
depend on assuming routine flush errors escape. Reset failure is the concrete
secondary-exception path requiring containment.

`g_runtimeInitializationComplete` currently records completion of the attempted
initialization path, including handled main failures. This review does not redefine
it as proof that all modules are installed. Optional Overlay startup containment is
not proof that the entire production initialization thread is contained.

### Minimal remediation design

- Add an outer try/catch around the actual initialization thread body, including
  `Initialize` and subsequent readiness handling, with a noexcept thread boundary.
  Leave normal ordering and returned DWORD compatibility unchanged.
- On a fatal outer exception, close existing stopping/readiness/correction flags
  using nonthrowing operations; return from the failure path without readiness
  notification. Do not publish success because cleanup happened to return.
- Use fixed/bounded no-throw reporting that does not require filesystem setup,
  logger existence or dynamic formatting. A second exception must not escape.
- Guard best-effort cleanup separately, on the initialization worker only. Track
  whether inner failure handling already attempted cleanup; do not blindly repeat
  destructive reset or introduce unbounded retries. Retain uncertain resources and
  report incomplete cleanup. N01 must be resolved before claiming safe hook teardown.
- Do not make throwing reset code noexcept as a substitute for handling it. Do not
  destroy the resident RuntimeState, create another owner/gate, unpublish through
  a new API, or move joins/reset/disk logging into DllMain.

Partial resources must be handled by phase: pre-path failure has no installed hooks;
logger-only failure differs from published API/Overlay/partly installed hook failure.
Overlay has an independent resident owner; this bounded fix must not invent a new
shutdown API or claim complete Overlay teardown from core reset. Preserve existing
independence; document resources retained after an incomplete core failure.

### Regression / fault-injection design and limits

Exercise the real initialization thread entry in an owned fixture, with one-shot
controlled failures at path/config-path/stream/logger setup, partial hook activation,
worker start and reset. Capture readiness calls, flags, cleanup attempts, exception
escape and retained ownership. Explicitly cover a secondary cleanup exception and
a cleanup path returning with worker resources retained. Avoid real game scans/hooks;
use controlled external leaf dependencies only in the test build. If reaching a
phase requires a new production seam, stop and request approval rather than
substituting a rewritten Initialize test body.

Verify normal successful startup ordering and optional Overlay OFF/ON behavior.
An isolated optional-start helper or callback test does not prove full startup.
Pre-DllMain CRT/static initialization, SEH faults, stack overflow, terminate and
driver failures are outside the proposed thread catch. Loader-lock safety is a
source/integration constraint; full game partial-init behavior remains a live gate.

Fix risks: double reset, false ready notification, cleanup exceptions under OOM,
and treating resident retention as successful teardown. Recommendation: **Fix**;
outer containment can mitigate escape independently, but complete cleanup cannot
be declared fixed while N01 remains unresolved.

## Approval boundary and recommended sequence

Risk priority remains N01 → N02 → N03. Implementation can start with the small,
independent N02 wrapper change, then N01 fault injection plus bounded ownership fix,
then N03 outer containment/cleanup using N01's checked disposition. Alternatively,
N03 outer containment alone can be an explicitly limited interim mitigation.

Separate approval must cover production/vendor changes and new test-only fault
dependencies. No broad SafetyHook update, runtime decomposition, logger rewrite,
new synchronization system or camera semantic change is proposed. Each slice needs
focused actual/native regression and relevant full tests/builds after implementation;
passing this review is not a fault-injection PASS.

Preservation check: all 313 files in the preceding audit's
`preserved-before.csv` match their current SHA-256 (zero differences). Git status
retains the pre-existing edits; this review adds only this report. No test or build
result is inferred from the preservation check.

**Review stops here. B5 implementation and N01–N03 implementation are not approved
by this report.**
