# Runtime.cpp decomposition — implementation plan

**Created:** 2026-10-08  
**Status:** Batch 0 complete; V0-A representative actual API/persistence matrix implemented; V0-B expanded but incomplete; V0-C read-only orchestration review complete, actual orchestration evidence remains a runtime gate; B1 complete.  
**Current authorized work:** B1 is complete. No later batch is authorized. No ASI installation or game launch.  
**Approved scope:** Batch 0 and Validation Batch V0 are complete to their recorded partial-coverage disposition. B1 is conditionally approved by the user on 2026-10-08. B2–B7 remain unapproved. V0-A/B remain gates for their dependent batches; V0-C remains a B7 gate.

## Objective and evidence

Separate the existing production integration layer into coherent components while preserving observable behavior, one runtime owner and one correction gate. This is structural work, not camera remediation.

Baseline audit: [RUNTIME_CPP_DECOMPOSITION_AUDIT_2026-10-08.md](../../research/reports/RUNTIME_CPP_DECOMPOSITION_AUDIT_2026-10-08.md). Its function inventory, 159-member state inventory, callback contracts and dependencies are the starting move ledger. No repeat broad audit is required.

At plan preparation:

- HEAD: `c844d2e379f922fe72f46e98c3e9ff61dd8eaeb0`.
- `src/plugin/runtime.cpp`: 6800 lines; SHA-256 `D4E124A5A1871CE80F736BAF4AD221E2F4AB24931CA0B2DC9BD761E74F00B341`, unchanged from the audit.
- Existing staged/unstaged/untracked user work is preserved; HEAD alone is not the baseline identity.
- `build.cmd` already compiles and links the real production `runtime.cpp`. The gap is **callback register/context and integration behavior coverage in `test.cmd`**, not absence of production compilation.
- Existing transaction harnesses remain useful but do not call the actual native adapters. `semantic_snapshot_harness` uses a fixture reader, not `ReadOverlaySemanticSnapshot`.

The user initially approved Batch 0 + V0 only on 2026-10-08; that scope was subsequently extended to B1 only as recorded below. [Execution results and remaining coverage gates](../../research/reports/RUNTIME_CPP_DECOMPOSITION_B0_V0_RESULTS_2026-10-08.md) continue to govern dependent later batches.

On 2026-10-08, the user subsequently approved **B1 only**, without requiring all V0-A/B combinations to be closed first. B1 may add a private typed binding to the existing owner, but it must preserve allocation, fields, initialization/destruction behavior, and startup/shutdown order. No callback, algorithm, installer, `Initialize`, or `ResetAllRuntimeResources` moves. Stop if the boundary requires an ownership redesign.

## Invariants and non-goals

1. Exactly one process-resident RuntimeState owner and one CameraCorrectionGate. State views reference the existing fields; no copied state or per-translation-unit singleton.
2. Preserve Gameplay, Cinematic, Dialogue, RuntimeCombine and recovery calculations, predicates, tolerances, memory ordering, source/association validation and ownership priority.
3. Preserve native register writes, memory writes, readback, publication and early-return ordering for each callback. Do not impose a new common ordering across callbacks.
4. Preserve correctionGate → dialogueMutex → cameraStateMutex ordering, hotkey-config ordering, guard lifetime and deferred log emission after gate release.
5. Preserve process residency, failed-worker-join resource retention, hook commit/rollback order and minimal loader-lock detach. Dynamic unload remains unsupported.
6. Keep Overlay presenter/worker lifetime independent. Overlay.Enabled=false remains startup-only, camera/hotkeys remain operational, no new graphics initialization.
7. Keep GameplayBaselineStore, shared canonical FOV facts, Transition Evidence, writer association, restoration and diagnostic caches semantically distinct.
8. Preserve all accepted Overlay/shared-state F1–F9 contracts. Prefix their identities as `Overlay/shared F1–F9`; other camera-module remediation numbering is separate.
9. No remove-unused cleanup, branch simplification, default/mode changes, new polling, guessed session boundary, tooltip/localization/INI changes or diagnostic marker renaming.
10. No generic camera framework, service registry, new coordinator, callback bus or domain-module access to the integration owner.

Main Menu/save-load session-boundary invalidation remains OPEN / authoritative event unknown. Default coordinator Gameplay is not an observed game session state. Existing evidence limitations are retained.

## Batch 0 — baseline and exact move ledger

**Status:** Complete. Baseline builds/full tests, recoverable copies, identities and exact ledger recorded; no production movement.

Expected record: `research/evidence/runtime-decomposition/BASELINE_2026-10-08.md` and a companion `MOVE_LEDGER_2026-10-08.md`. Existing reports are not rewritten.

Record:

- Current source/build/test/contracts hashes, HEAD, and index/worktree state, with private paths/data kept out of reusable reports.
- A recoverable copy or patch of current relevant staged/unstaged source outside release assets. Never reset, stage/unstage or overwrite user changes to create a baseline.
- Existing root ASI identity, if present, labelled built artifact only; do not assume it matches current source or a historical runtime session.
- Fresh ordinary and supported diagnostic build identities if the approval includes the validations below. Separate output names/object directories prevent one profile overwriting the other's evidence or the retained ASI.
- Current full test result and any exact pre-existing failure. Do not carry forward a historical CJK/config failure as a present failure without running the approved baseline check.
- Every audited function/template, local data shape, namespace/static/TLS storage, RuntimeState member, alias and profile gate gets a ledger entry: current location, destination or `Remain`, dependencies, owner, guard contract and batch.
- All 159 fields remain under the same owner; list conditional fields and macro-dependent HookSet layout. Any inventory mismatch is resolved by bounded current-source reading.
- Compiler defines/source lists for ordinary and supported diagnostic builds, and the exact changed test registry entries.

No function can disappear into an unassigned “misc” destination. Definition-only/dormant helpers are `Remain` unless a later approved batch explicitly moves them intact; they are not deletion candidates in this task.

**Gate B0:** Identity, preservation record, exact ledger and baseline results recorded. Known failed baseline checks have explicit impact/disposition. An unrelated failure is not a reason to change camera behavior or silently waive affected checks.

## Validation Batch V0 — actual adapter characterization before extraction

**Status:** Partial. Real adapter harness and test-only leaf recording implemented without production source edits. V0-A representative published API/persistence paths and named V0-B lifecycle/source cases are exercised. V0-C static review classifies existing leaf evidence and remaining runtime gates. Full V0-A/B matrix and actual V0-C orchestration remain incomplete; see execution report. No callback movement or policy changes.

### How real production code will be exercised

Start with a harness translation unit that includes the current `runtime.cpp` **exactly once**. This provides access to its existing private adapters/state in that test translation unit without publishing RuntimeState or duplicating callback logic. Do not separately compile/link runtime.cpp into the same executable. Do not include dll_entry.cpp, invoke DllMain, load the ASI or attach hooks to the game.

Use the real SafetyHookContext type, existing production state, gate, transactions, publication helpers and platform memory checks. Supply allocated test buffers with the real field offsets, valid/invalid protection, and controlled native context values. Fresh process cases avoid reconstructing or assigning mutex-bearing resident state. Domain transforms and callback bodies are never mocked or copied.

Link the actual required project/vendor dependencies. For deterministic external inputs, use existing typed engine-operation seams where they reach the tested adapter. A test operation passed only to ProcessCameraWriter is not proof of ReplayManualTransition integration.

Where the actual adapter hard-codes an external operation (viewport query, Overlay wake, hook installation or startup), use a bounded test-only leaf seam if required. Its production path must keep the same operation and call position. Any compile-time seam must replace only that external leaf; it must not substitute the callback, settings handler, publication, lifecycle transaction or broad initialization flow. A stubbed external effect is labelled simulated in results.

A no-seam test can use real readable/writable memory to prove final values. Order assertions that cannot be observed this way need leaf-event capture; final values alone must not be reported as proof of write/readback/publication order.

Before any seam edit, record exact changed files and prove production-disabled instrumentation adds no owner, branch decision, callback policy or new runtime storage. If this cannot be achieved narrowly, stop only that test mechanism and revise it; do not begin extracting untested callbacks.

After extraction, retarget the same harness to the actual linked adapter implementation using plugin-private entry points. Remove the monolith inclusion when it would cause duplicate definitions. Test-only accessibility must not turn the integration owner into a public API.

### Required scenarios and assertions

| Surface | Required cases | Actual integration assertions |
|---|---|---|
| Owner/gate | Writer, ENTER/EXIT/aspect, Dialogue, settings, semantic read | All reference same owner/gate; acquire vs already-held contract; no nested Acquire; deferred actions execute after release |
| Gameplay writer | Gameplay ON/OFF; all 3 modes; 16:9 pass-through; valid ultrawide; invalid/unreadable source; foreign RSI/RBX | Exact XMM0 output and unchanged register sentinels; permitted aspect/flags writes only; real callback publication; pending-state consumption |
| RuntimeCombine | Aspect match; mismatch; write/readback success/failure; invalid viewport; source mismatch | Synchronization → readback confirmation → transform contract; no flag synchronization; no HorPlus on unconfirmed aspect |
| Cinematic aspect | Accepted target; disabled correction; failed/unwritable target | RIP advances exactly 10 bytes as currently implemented; RAX target aspect behavior; register/flags sentinels; association ordering |
| ENTER | Accepted/duplicate; ON/OFF; native/Gameplay baseline fallback; rapid continuation | XMM0 mutation; selection/generation/epoch; observations and Evidence operands from actual callback; duplicate not accepted |
| EXIT/recovery | Accepted/rejected; source-owned endpoint; interpolation; foreign source; no owner; empty superseded generation | Native EXIT preserved; owner capture/retirement and recovery routing; no baseline contamination; actual writer handles subsequent recovery |
| Dialogue | Candidate/Active/Exiting/Rearm; enabled/disabled; invalid native target; cinematic ownership | XMM6 read/XMM1 write; untouched other registers; valid native target safety; in-cinematic reset/pass-through; disable serialization |
| Shared publication/readback | Gameplay → ENTER → Dialogue/continuation → EXIT/Recovery → Gameplay | Actual ReadOverlaySemanticSnapshot; four role values/validity/provenance/freshness; last full-publication vs transaction owner vs coordinator |
| Evidence | gen1 ENTER→EXIT→gen2 ENTER→gen3 no-writer superseded→gen4 ENTER | Independent valid operands, generation, Match; preserved continued chain; last validated after exit, never live FOV |
| Notification delivery | Same number + changed freshness/provenance/visible metadata; unchanged semantics | Real publication decision invokes change-driven wake; no unconditional per-sample redraw |
| Settings/persistence | ON/OFF and mode mutations; rejected input/capability; active/requested distinction; failed persistence | Actual callback bundle routing; same epochs/pending fields; persistence outside correction gate; last-known-good preservation |
| Startup/install/reset | Overlay ON/OFF/failure; cinematic partial failure; gameplay local failure; missing Dialogue dependency; failed join | Same actual orchestration with simulated leaf operations; activation/rollback/capability/ready order; no dependent correction on missing observation |
| Zoom | IN/OUT, failed reads, diagnostic OFF/ON | Existing shared qualification/hold semantics; no new gate acquisition; diagnostic emission not production authority |

Context sentinels include RIP, RFLAGS, general registers and non-target XMM lanes/registers where the callback must preserve them. Compare semantically relevant state fields, not padding bytes or process-specific pointer addresses. Normalize fixture identities for comparison while keeping RSI/RBX distinctions. Numerical assertions use the existing purpose-specific tolerances; no new blanket epsilon.

Use expected observations established from the monolith plus independent invariant assertions. Check in deterministic fixtures/results only when stable and reviewable. Do not regenerate expected outputs automatically after each move. Same-input baseline/new comparison is characterization, not proof that every pre-existing behavior is correct.

### Coverage approval gate

`V0-A` is mandatory before settings/publication extraction: actual settings/readback/publication, one owner/gate, event delivery.

`V0-B` is mandatory before each native callback extraction: the matching actual adapter matrix above has passed against the unchanged algorithm in the monolith. Full native matrix and rapid-chain checks precede writer extraction.

`V0-C` is mandatory before moving installers/startup/reset: actual orchestration order/failure tests, with simulated leaf hook/thread effects explicitly distinguished from real hook installation.

V0 is not complete merely because the test compiles or a domain transaction harness passes. The test registry must compile/run actual adapter tests with ordinary-profile callback bodies, and supported diagnostic variants where relevant. Compile-time test access must not enable experimental correction profiles. V0-C's read-only review is complete, but actual Initialize/partial-install/reset orchestration was deliberately not mocked; named runtime gates remain before installer/startup/reset movement.

## Exact component move map

This map fixes named responsibilities. Batch 0 expands it into one entry per audited symbol, including profile guards; ranges refer to the audited hash, not future line numbers.

| Component / proposed file | Named functions / state binding | Boundary |
|---|---|---|
| runtime.cpp + runtime_internal.hpp | Existing resident allocation; RuntimeState definition if needed; view factories; public InitializeThread/Shutdown/NotifyProcessDetach/SetModuleHandle facade | Sole owner; private plugin context, not public plugin::RuntimeState |
| runtime_settings.cpp | SelectGameplayMode, RuntimeHotkeyBinding, ApplyGameplayModeSetting, ApplyRuntimeSetting, ReadRuntimeSettings, PersistHotkeyActionValue, PersistRuntimeSetting; settings/locale facade functions | Existing effective settings and pending fields; one published RuntimeSettingsApi |
| runtime_camera_publication.cpp | MarkCameraStateFreshness, MarkCameraPresentationTransition, MarkCameraStateForSetting, PublishCameraStateSnapshot, PublishCameraFovTransaction, ReadOverlaySemanticSnapshot, viewport readback facade | Same snapshot/qualification references; store authority unchanged |
| runtime_camera_callbacks.cpp | ReplayManualTransition, GetCameraWriterEngineOperations, native read/write utilities needed by it; writer recovery interaction | Whole writer dispatch ordering; no separate recovery owner |
| runtime_cinematic_callbacks.cpp | ApplyCinematicAspectStore, TraceCinematicEnter, TraceCinematicExit, ResolveAutoAspect/ResolveCinematicAspect and store viewport adapters | Existing accepted transactions; native RIP/XMM0 handling |
| runtime_dialogue_callbacks.cpp | TraceDialogueBoundary, ResetDialogueRuntimeState, GetDialogueBoundaryStateView; reset glue used by other adapters | Existing Dialogue state/gate; shared reset entry explicit |
| runtime_zoom_observation.cpp, only if justified | TraceZoomTransition/IN/OUT and observation TLS | Production qualification remains enabled independently of diagnostics; existing affinity preserved |
| runtime_hook_installation.cpp | VerifyExecutableAndInstruction, Validate*Boundary, ResolveCinematic* sites, Install*/Commit*/Rollback*/RestoreCinematicAspect, InstallDialogueBoundary, InstallZoomTransitionObservation | Same signature contracts, thunk addresses, component commit/rollback |
| runtime_diagnostics.cpp or small domain-specific diagnostic files | Existing Log/LogImmediate/deferred probes; source/cinematic baseline/writer captures; trace/dedup formatting; optional observers | Exact existing storage lifetimes and Diagnostic Mode/profile gates; no correction authority |
| runtime_hotkeys.cpp, optional | HotkeyLoop, notification queue facade, rebind guards | Stop event/settings/persistence/notifications; no Delete handling relocation |
| Remain initially | Initialize, ResetAllRuntimeResources, worker facade, optional experimental replay/trigger code, definition-only legacy helpers | No startup extraction or dead-code cleanup as an incidental dependency |

Logger/native exception templates may require a plugin-private shared header. Its functions bind the same state; header static owner copies are forbidden. Prefer the existing transaction views; domain modules never receive the entire runtime owner. Adapter cross-calls use the smallest current interface.

Component count is not a target. Combining tightly coupled adapters/installers is allowed only as a documented plan revision retaining the same tests and ownership. No catch-all new subsystem is approved.

## Controlled extraction batches

Only B1 was approved, and it is now complete. B2–B7 remain **Not started / unapproved**; approval of B1 does not approve these moves.

| Batch | Scope | Entry gate | Exit gate |
|---|---|---|---|
| B1 | **Complete:** plugin-private binding/layout for one existing owner; no algorithm movement | B0; owner/gate tests | Same fields/init/destruction behavior; ordinary + diagnostic build, one-owner tests — PASS; [B1 results](../../research/reports/RUNTIME_CPP_DECOMPOSITION_B1_RESULTS_2026-10-08.md) |
| B2 | Settings/readback/persistence integration | V0-A | Actual handler/reader/persistence tests + focused config/API/gate tests; production/diagnostic builds |
| B3 | Shared publication/freshness glue; diagnostics as a separate reviewed sub-batch | V0-A and deferred-log tests | Canonical roles, qualification, Evidence lifetime, wake and projection tests; real adapters still call same path |
| B4 | Dialogue callback and reset glue | Dialogue V0-B | Actual XMM6→XMM1/read/reset/serialization matrix; domain Dialogue regression; builds |
| B5 | Cinematic aspect/ENTER/EXIT callbacks | Cinematic V0-B; rapid-chain characterization | RIP/native register/order + generation/recovery/Evidence matrix; domain Cinematic regression; builds |
| B6 | Whole shared writer adapter and necessary engine operations | Complete writer V0-B | All modes/OFF/source/recovery/RuntimeCombine matrix; actual publication/readback; builds |
| B7 | Installers/startup/worker/hotkey extraction only if a remaining coherent responsibility warrants it | V0-C; separate approval of exact subset | Real orchestration with leaf simulation, worker/reset/config tests; builds |
| Final offline | Complete relevant diff/ledger review; full test.cmd and ordinary/diagnostic builds | All selected extraction gates passed | No unassigned symbols, no semantic changes, artifacts/hashes and unresolved coverage explicitly recorded |

Per-batch builds validate linkage/source lists in addition to tests; skip no affected profile merely because ordinary build passed. Focused tests run for changed boundaries; the complete suite is required at baseline and final offline gate, and when an intermediate change has broad dependencies. Do not repeatedly run unrelated expensive checks after sufficient evidence without a changed reason.

Future expected files: named plugin components/private headers, focused `tests/plugin/runtime_*_harness.cpp`, possible test fixture support/runner, `test.cmd`, `build.cmd` source lists and profile wiring, current architecture documentation when the owner becomes plugin-private across TUs. Existing camera/domain policy files are not algorithm-change targets. Any seam requires its own exact-file entry in V0.

## Acceptance and evidence levels

| Criterion | Required evidence | Current status |
|---|---|---|
| Baseline preserved and identified | B0 hashes/state/ledger/results | Complete |
| Real production code compiles | Ordinary build and supported diagnostic build for each affected move | Both baseline builds PASS; no moves |
| Real callbacks retain register/context integration | V0 baseline and unchanged expectations against moved adapters | Actual monolith adapters tested; complete matrix incomplete; no moved adapters |
| One owner/gate and no duplicate stores | Link/source review + actual adapters/readers using same state | Source unchanged; tested owner/gate bindings preserved |
| Native side-effect order and failure behavior retained | Leaf event capture/context/memory assertions with actual adapters | Tested named read/write/wake/context paths; remaining matrix incomplete |
| Shared lifetime/provenance/Evidence unchanged | Actual publication/readback + existing projection/domain regressions | Tested named actual paths; production source unchanged |
| Startup/reset/residency unchanged | Actual orchestration/failure characterization + source review | Static baseline recorded; V0-C not exercised; dynamic unload unsupported |
| Complete offline regression | Final full test.cmd, profile builds and scoped diff review | See execution report; this does not close missing V0 gates |
| Live hook installation/game lifecycle behavior | Separately approved matching-artifact runtime scenario | Follow-up; not proved offline |

Offline closure and runtime acceptance are separate milestones. Any later runtime sequence is run by the user or under separate explicit authorization: menu→Gameplay→rapid Cinematic/Dialogue generations→Recovery→Gameplay, zoom/ADS, settings/hotkeys and Overlay OFF/ON startup as appropriate. Do not launch the game, install ASI, package, publish or claim release safety through this plan.

Do not require byte-identical ASI after a TU split; binary layout legitimately changes. Required equivalence is at the stated behavior/state/context/order boundaries, with the new artifact identity recorded.

## Failure and scope-change policy

- If a baseline test exposes a defect, record it separately; decomposition cannot silently fix it. Characterization does not promote a known wrong result into desired behavior.
- If adapter access requires a broader production redesign, revise only that proposed seam and obtain approval for the material expansion before continuing dependent movement.
- New concurrency/ABI issues, session event discovery, camera remediation or diagnostics policy changes are separate tasks.
- Review the complete relevant diff at each boundary. No destructive Git operations, automatic rollback over user edits or blanket replacement of runtime.cpp.
- Record exact checks/artifacts/coverage limits; no borrowed PASS from an earlier ASI. Write task log only after verified validation and user-confirmed closure per active rules.

## Approval checkpoint

B1 is complete under the user's 2026-10-08 conditional approval. V0-A and V0-B remain partial; their full matrices gate the dependent later batches. V0-C source review is complete, but Initialize/install/reset wiring remains a runtime validation gate for B7. The recorded [B1 results](../../research/reports/RUNTIME_CPP_DECOMPOSITION_B1_RESULTS_2026-10-08.md) do not authorize B2–B7.

No callback extraction, game launch, ASI installation or task-log closure occurred. The B0/V0 results report remains historical and unchanged.
