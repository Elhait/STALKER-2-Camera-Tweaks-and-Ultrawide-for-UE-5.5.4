# Runtime.cpp decomposition — implementation plan

**Created:** 2026-10-08  
**Status:** `RUNTIME_CPP_DECOMPOSITION` is complete at B1–B7, with each batch closed according to its individual disposition. Batch 0 and V0 retain their documented partial-coverage status; this architectural closure does not close runtime verification gates.  
**Current status:** B7 was approved closed on 2026-10-10 as **Complete — with documented `Remain` decisions**. No further source splits are authorized by this campaign. N03 remains `Partially mitigated`; N04–N06 remain open; live validation of final remediation ASI `E349…DC81` remains pending. No ASI installation or game launch was performed for this documentary closure.  
**Approved scope:** Batch 0 and Validation Batch V0 retain their recorded partial-coverage disposition. B1 was approved on 2026-10-08 and is complete. B2 was authorized on 2026-10-09 and completed with the persistence extraction plus four approved `Remain` dispositions. B3 was closed on 2026-10-09 with documented `Remain` decisions; no production extraction was performed. B4 was closed on 2026-10-09 as **Complete — with documented Remain decisions**; no production extraction was performed. B5 was closed on 2026-10-10 with documented `Remain` decisions. B6 was approved closed on 2026-10-10 with the dispositions from its boundary review; no production extraction was performed. B7 was approved closed on 2026-10-10 with documented `Remain` decisions; no additional extraction was performed. V0-A/B remain partial evidence gates where applicable; V0-C and N04–N06 remain separate validation gates.

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
| runtime_settings.cpp | `RuntimeHotkeyBinding`, `PersistHotkeyActionValue`, `PersistRuntimeSetting` | Extracted in B2; actual production-adapter settings/readback/persistence coverage and builds recorded below. |
| runtime.cpp | `SelectGameplayMode`, `ApplyGameplayModeSetting`, `ApplyRuntimeSetting`, `ReadRuntimeSettings` | **Remain (approved 2026-10-09):** tightly coupled to resident RuntimeState, owner-local callbacks, and lock ordering; no new owner API. |
| runtime_camera_publication.cpp | B3 candidates: `MarkCameraStateFreshness`, `MarkCameraPresentationTransition`, `MarkCameraStateForSetting`, `PublishCameraStateSnapshot`, `PublishCameraFovTransaction`, `ReadOverlaySemanticSnapshot`, `GetRuntimeViewportAspect` facade | **No file created.** Publication/freshness adapters, semantic reader and viewport facade are approved `Remain` in `runtime.cpp`; pure domain operations/resolver are already extracted. |
| Existing diagnostics boundaries | B3-specific `CameraStatePublish` / `SemanticSnapshotRead` event sites, snapshot/FOV state log emission, deferred log handoff | **Remain:** event sites stay with runtime lifecycle operations; event counter implementation is already in `diagnostics/performance_telemetry.cpp`; logger deferral remains bound to `CameraCorrectionGate`. Other diagnostics are outside B3 and stay assigned to their later reviewed batch. |
| runtime_camera_callbacks.cpp | ReplayManualTransition, GetCameraWriterEngineOperations, native read/write utilities needed by it; writer recovery interaction | Whole writer dispatch ordering; no separate recovery owner |
| runtime_cinematic_callbacks.cpp | ApplyCinematicAspectStore, TraceCinematicEnter, TraceCinematicExit, ResolveAutoAspect/ResolveCinematicAspect and store viewport adapters | Existing accepted transactions; native RIP/XMM0 handling |
| runtime_dialogue_callbacks.cpp | B4 candidates: `TraceDialogueBoundary`, `ResetDialogueRuntimeState`, `GetDialogueBoundaryStateView`, post-Cinematic Dialogue exclusion wrappers | B4 closed 2026-10-09: owner-bound runtime adapters are approved `Remain`; transaction/lifecycle policy helpers are `Already extracted` in `dialogue_boundary_transaction.hpp` and `dialogue/*`. No physical B4 move. |
| runtime_zoom_observation.cpp, only if justified | TraceZoomTransition/IN/OUT and observation TLS | Production qualification remains enabled independently of diagnostics; existing affinity preserved |
| runtime_hook_installation.cpp | VerifyExecutableAndInstruction, Validate*Boundary, ResolveCinematic* sites, Install*/Commit*/Rollback*/RestoreCinematicAspect, InstallDialogueBoundary, InstallZoomTransitionObservation | Same signature contracts, thunk addresses, component commit/rollback |
| runtime_diagnostics.cpp or small domain-specific diagnostic files | Existing Log/LogImmediate/deferred probes; source/cinematic baseline/writer captures; trace/dedup formatting; optional observers | Exact existing storage lifetimes and Diagnostic Mode/profile gates; no correction authority |
| runtime_hotkeys.cpp, optional | HotkeyLoop, notification queue facade, rebind guards | Stop event/settings/persistence/notifications; no Delete handling relocation |
| Remain initially | Initialize, ResetAllRuntimeResources, worker facade, optional experimental replay/trigger code, definition-only legacy helpers | No startup extraction or dead-code cleanup as an incidental dependency |

Logger/native exception templates may require a plugin-private shared header. Its functions bind the same state; header static owner copies are forbidden. Prefer the existing transaction views; domain modules never receive the entire runtime owner. Adapter cross-calls use the smallest current interface.

Component count is not a target. Combining tightly coupled adapters/installers is allowed only as a documented plan revision retaining the same tests and ownership. No catch-all new subsystem is approved.

## Controlled extraction batches

Historical approval note: only B1 had been approved when this section was first recorded. Subsequent approvals and closures are recorded in the batch rows and dated appendices below. At the later B6 checkpoint, B5 was closed and B6 review had been performed; the B7 review had not yet been authorized. The later B7 review and pending disposition are recorded below.

| Batch | Scope | Entry gate | Exit gate |
|---|---|---|---|
| B1 | **Complete:** plugin-private binding/layout for one existing owner; no algorithm movement | B0; owner/gate tests | Same fields/init/destruction behavior; ordinary + diagnostic build, one-owner tests — PASS; [B1 results](../../research/reports/RUNTIME_CPP_DECOMPOSITION_B1_RESULTS_2026-10-08.md) |
| B2 | **Complete with documented Remain decisions:** settings/readback/persistence integration | V0-A | Actual production-adapter settings/readback/persistence scenarios, focused regressions, full `test.cmd`, ordinary + diagnostic builds — PASS for the extracted slice; four owner-coupled adapters remain in `runtime.cpp` by design. |
| B3 | **Complete with documented Remain decisions:** shared publication/freshness integration and B3-scoped diagnostics | V0-A and deferred-log tests; complete boundary review | All planned B3 responsibilities classified below; pure domain helpers already extracted; owner adapters remain. Existing harness/build evidence retained; no source change or test rerun for documentary closure. |
| B4 | Dialogue callback and reset glue | Complete — documented `Remain` / `Already extracted` dispositions approved 2026-10-09 | All planned responsibilities classified. V0-B coverage limitations remain explicit and are not treated as completed tests; no production movement. |
| B5 | Cinematic aspect/ENTER/EXIT callbacks | **Complete — with documented `Remain` decisions**, approved 2026-10-10 | [B5 findings and B1–B5 architecture checkpoint](../../research/evidence/runtime-decomposition/B5_CINEMATIC_CALLBACK_BOUNDARY_REVIEW_2026-10-10.md). No source movement. |
| B6 | Whole shared writer adapter and necessary engine operations | **Complete — with documented `Remain` decisions**, approved 2026-10-10 | [B6 Gameplay Writer Boundary Review](../../research/evidence/runtime-decomposition/B6_GAMEPLAY_WRITER_BOUNDARY_REVIEW_2026-10-10.md). Native adapter and owner-bound factories remain; transaction/mode/recovery domain logic is already extracted. No source movement, tests, or builds. |
| B7 | Startup/installers/workers/reset/hotkeys and Overlay readiness orchestration | **Complete — with documented `Remain` decisions**, approved 2026-10-10; no additional source split | [B7 boundary review](../../research/evidence/runtime-decomposition/B7_STARTUP_INSTALL_WORKER_RESET_BOUNDARY_REVIEW_2026-10-10.md). `WorkerLifecycle`, DLL entry and pure resolver/transaction helpers are already separate; remaining composition stays with the runtime owner. N03–N06 and exact-artifact live validation remain separate open gates. |
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

## B2 execution checkpoint — persistence extraction

The user authorized continuation of `RUNTIME_CPP_DECOMPOSITION` after release
preparation on 2026-10-09. Work is limited to B2. Its required V0-A actual
settings API, reader, semantic readback, persistence, owner/gate and wake paths
were exercised by the production-adapter harness before extraction; the
remaining exhaustive OS-failure matrix is not claimed.

The first B2 slice moved only `RuntimeHotkeyBinding`,
`PersistHotkeyActionValue` and `PersistRuntimeSetting` implementations into
`src/plugin/runtime_settings.cpp`. `runtime.cpp` keeps the production callback
adapters that bind the resident config path, current locale and logger. The
existing `RuntimeSettingsApi` publication and callback registration remain in
the same owner and at the same initialization point. No settings application,
snapshot reader, hotkey loop, state field, camera callback or startup/reset
function moved in this slice.

The B2 slice was validated on 2026-10-09: `test.cmd` completed with all 59
registered checks passing, including 64 actual-adapter scenarios in both
Diagnostic Mode states (128 fresh processes), plus the 18-locale resource
audit. Production and supported diagnostic builds completed into isolated
`build-artifacts/runtime-decomposition/b2a/` outputs. Production ASI is
3,462,144 bytes, SHA-256
`586C8BD50872988ECC2026ED1CEEA157BE72DDFE76AC73C232CF86F65CD4C2E7`;
diagnostic ASI is 3,532,288 bytes, SHA-256
`13FC0859A7B9F74E21657C16FE2A4807EC06F28BDE8BFED2E5FE440061009704`.
No game was launched and no ASI was installed. At this checkpoint, the
persistence extraction was a partial B2 result; the subsequent API design
review and approved `Remain` dispositions below completed B2 without moving
the Apply/readback integration from `runtime.cpp`.

## B2 follow-up checkpoint — settings/readback boundary review

On 2026-10-09, the user authorized a bounded review of `SelectGameplayMode`,
`ApplyGameplayModeSetting`, `ApplyRuntimeSetting`, and `ReadRuntimeSettings`
for possible extraction into `runtime_settings.cpp`. The review found no
existing cross-translation-unit boundary that can host these production
adapters without extending the private RuntimeState owner API:

- `SelectGameplayMode` mutates the resident gameplay mode, coordinator/replay
  state, transition-pending flags, restoration state, last camera source/FOV,
  the resident Gameplay baseline store, and diagnostic telemetry under its
  mutex. These are all aliases or members of the single `RuntimeState` in
  `runtime.cpp`, plus local runtime helpers.
- `ApplyRuntimeSetting` constructs the existing `CameraSettingStateView`,
  supplies runtime-local mode-selection/freshness callbacks, and flushes a
  runtime-local cinematic trace. Its non-camera cases also mutate overlay
  settings and hotkey configuration. The camera transaction currently
  acquires `CameraCorrectionGate`; the remaining cases acquire that same gate
  before, where applicable, taking `hotkeyConfigMutex`.
- `ReadRuntimeSettings` reads RuntimeState-owned atomics while holding the
  correction gate, then reads persisted hotkey configuration under
  `hotkeyConfigMutex`. Moving it would require a new view/facade exposing
  those resident fields and the same lock objects across translation units.
- `ApplyGameplayModeSetting` is only a forwarding callback to
  `SelectGameplayMode`; moving the wrapper alone would not extract a coherent
  responsibility.

Because the user required stopping rather than expanding the owner API for
that stage, none of these four functions was moved and no owner view, facade,
duplicate state, or callback-lifecycle change was introduced. At the time,
this was a boundary-review result awaiting a disposition. The subsequent
approval and B2 closure are recorded in the API design review below. No source
code changed during the review; no tests or builds were rerun, and the prior
B2-slice results/hashes remain the latest applicable evidence.

## B2 API design review — bounded-owner interface

The user requested a narrow API design review without implementation. The
existing `RuntimeSettingsApi` is already the consumer-facing callback facade;
its production handlers are registered from `runtime.cpp` with null contexts
and currently execute against aliases to the one resident owner. The
`CameraSettingStateView` is an existing transaction-specific view for camera
settings, not a complete owner boundary for all settings or readback.

Two possible extraction approaches were assessed:

1. Pass `RuntimeState` or a general mutable view into `runtime_settings.cpp`.
   This would expose substantial mutable state across translation units and
   couple settings code to coordinator, replay, baseline-store and diagnostic
   internals. Rejected: it weakens the private-owner boundary.
2. Add a collection of narrow owner operations/callbacks for each mutation,
   snapshot read, log, freshness update, and trace flush. This duplicates the
   existing handler layer as a second dispatch API, while preserving almost
   all integration code in `runtime.cpp`. Rejected for B2: the extra surface
   is not justified by a coherent extraction.

Recommended disposition: mark `SelectGameplayMode`,
`ApplyGameplayModeSetting`, `ApplyRuntimeSetting`, and `ReadRuntimeSettings`
as `Remain` in `runtime.cpp`. Keep the existing `RuntimeSettingsApi`, camera
transaction view, lock objects, and their ownership unchanged. This preserves
the current `CameraCorrectionGate → hotkeyConfigMutex` order and avoids
exporting coordinator or mutable owner state. `ApplyGameplayModeSetting`
remains beside its target because it is only the callback adapter for
`SelectGameplayMode`.

The user approved this disposition on 2026-10-09: all four functions are
final `Remain` decisions, the persistence/mapping work is `Extracted`, and B2
is **Complete — with documented Remain decisions**. No owner API was added.
The B2 completion relies on the already-recorded 2026-10-09 verification of
the extracted slice: all 59 registered `test.cmd` checks passed, including
64 actual-adapter scenarios in both Diagnostic Mode states (128 fresh
processes) and the 18-locale audit; production and supported diagnostic
builds passed. Artifact sizes and SHA-256 values are recorded in the
persistence checkpoint above. No production source changed after those
checks, so no repeat build/test run was needed for this documentation-only
closure. This does not authorize B3.

## B3 boundary review and closure — shared publication/freshness

The user authorized a read-only boundary review only. Production source was
not changed. The review followed the semantic reader, camera-state publication
and freshness wrappers, Overlay wake path, and deferred logging behavior.

### Existing separation

- Pure snapshot construction/comparison lives in `camera_state_snapshot.*`;
  locked publication, FOV transaction projection, and freshness transitions
  are already expressed as domain helpers in `camera_state_publication.hpp`.
  Camera-setting freshness projection is already a helper in
  `camera_writer_transaction.hpp`. These helpers receive explicit inputs or
  references and do not own runtime state.
- `runtime.cpp` remains the integration owner. Its publication adapters
  assemble facts from the writer result, coordinator, dialogue lifecycle,
  zoom observations, RuntimeCombine telemetry, Gameplay baseline store, and
  the single resident `CameraStateSnapshot` before calling those helpers.
- `ReadOverlaySemanticSnapshot` is registered through the existing
  `RuntimeSettingsApi` callback bundle. It acquires the correction gate,
  projects settings and lifecycle facts from RuntimeState, reads Dialogue
  state under `dialogueMutex`, reads the baseline store and camera snapshot,
  resolves the process viewport, and returns one semantic snapshot. Moving
  this adapter would require a broad view of owner state/locks or a second
  callback fan-out API, not a bounded publication component.
- The planned viewport facade is `GetRuntimeViewportAspect`: it is a thin
  public runtime adapter over the already-existing
  `platform::win32::ReadClientViewportAspect` resolver. The semantic snapshot
  uses the existing current-process viewport resolver directly. Moving this
  wrapper would not form a meaningful component boundary; both remain as
  currently wired.

### Synchronization, wake and logging constraints

- The documented nested lock order is
  `CameraCorrectionGate → dialogueMutex → cameraStateMutex`. Snapshot readers
  hold the correction gate while obtaining a coherent cross-source view.
  Writers serialize camera callbacks/settings through that same gate; the
  canonical snapshot itself is mutated under `cameraStateMutex`.
- Freshness and presentation wrappers compare before/after semantics while
  holding `cameraStateMutex`, release that mutex, then notify Overlay only for
  a semantic change. Publication follows the same change-detect / unlock /
  wake pattern. Overlay wake is an asynchronous UI-thread message, not part of
  the canonical snapshot store.
- `Log` queues work in `CameraCorrectionGate`'s existing thread-local deferred
  action list whenever the current thread owns the gate, and drains actions
  only after the gate is unlocked. The diagnostic camera-log probe has its own
  deferred flush wrapper. Moving either piece would affect callback ordering
  and potentially perform logger I/O inside a protected camera path; neither
  is a standalone B3 extraction.
- B3-scoped telemetry is fully classified: the `CameraStatePublish` and
  `SemanticSnapshotRead` event calls remain at their production runtime event
  sites; the atomic event-counter implementation is already extracted in
  `diagnostics/performance_telemetry.cpp`. The `CAMERA_STATE_SNAPSHOT` and
  `CAMERA_FOV_STATE` messages remain with their publication callbacks, where
  their values and deferral timing are known. Broader source/cinematic/writer
  diagnostics are not B3 publication responsibilities and remain assigned
  to their existing/later batch scope.
- The domain publication helpers and their harnesses already exercise the
  pure snapshot semantics. The production adapter harness includes the actual
  `runtime.cpp` semantic reader/publication callbacks and substitutes only
  external wake/log/platform boundaries. This is useful offline evidence, not
  proof of live Overlay scheduling or game lifecycle behavior.

### Boundary recommendation (not implementation approval)

The coherent seam is already at the pure domain helpers. The remaining
functions named for `runtime_camera_publication.cpp` are owner-bound adapters
that bind those helpers to shared camera state, lifecycle, locks and Overlay
wake. Splitting them into another translation unit would require essentially
the same broad owner access rejected in B2, or introduce a second operation
dispatch surface with little reduction in coupling. Recommendation: classify
the listed runtime adapters as `Remain` if the user later chooses to close
B3 without physical extraction. Do not create `runtime_camera_publication.cpp`
for this glue under the current plan.

Deferred logging is also classified `Remain` under the gate/logger owner. No
extra mutex, publication store, wake queue, or mutable-state API is justified
by this review. The user approved the complete B3 disposition on 2026-10-09:
B3 is **Complete — with documented Remain decisions**. All planned B3
responsibilities are classified; no source changes, builds, or test reruns
were needed for this documentary closure. At that checkpoint B4–B7 were
unapproved, and the next authorized step was a separate B4 boundary review;
that review is now recorded below and did not approve B4 implementation.

## B4 boundary review — Dialogue callback and reset glue

The user authorized boundary review only on 2026-10-09. No production source,
tests, configuration, or release artifacts were changed. The individual move
ledger was updated with the resulting dispositions.

### Callback and ownership boundary

`TraceDialogueBoundary` is a native adapter, not the Dialogue policy/state
machine itself. It acquires the single `CameraCorrectionGate`, captures
incoming FOV from `XMM6` and source identity from `RSI`, reads the native
target at `RSI + 0x2C`, then takes `dialogueMutex` and passes the existing
resident references through `GetDialogueBoundaryStateView` to
`ProcessDialogueBoundary`. The helper returns a decision; the adapter then
performs state-freshness publication, lifecycle/provenance logging, optional
`XMM1` mutation, and camera FOV publication/wake. Ordering matters: the
resulting `XMM1` is written before publication, while `Log` defers emission
until the correction gate is released.

The state view binds Dialogue phase, baseline/previous FOV, candidate and
re-arm trackers, active policy/correction activation, settings epochs,
coordinator, post-Cinematic exclusion, and snapshot source/target fields from
the existing RuntimeState. It is not an independent Dialogue owner. The
callback releases `dialogueMutex` before the freshness/snapshot helpers take
`cameraStateMutex`; the correction gate remains held across both stages, so
the current ordering is gate → dialogue critical section → camera-state
critical section without overlapping the two inner mutexes. Moving
the hook adapter would require passing this broad mutable reference view (or
adding another owner callback/context facade) and would still call back into
runtime publication, provenance and logging adapters. That does not create a
meaningful reduction in coupling.

`ResetDialogueRuntimeState` is a lock-preconditioned wrapper: it records the
source-end diagnostic and active-policy release before invoking the already
extracted `ResetDialogueBoundaryState`. It does not acquire the Dialogue
mutex itself. Its callers establish the existing `dialogueMutex` scope, and
camera callback paths also hold the correction gate; reset/publication order
must therefore remain at those call sites. ResetAllRuntimeResources is a
separate B7 orchestration responsibility and is not moved or redefined here.

The post-Cinematic exclusion wrappers (`ResetPostCinematicDialogueExclusion`,
`ArmPostCinematicDialogueExclusion`, `ObservePostCinematicDialogueRecovery`)
bind a resident exclusion object to RuntimeState and to EXIT/recovery call
sites. The underlying exclusion policy is already in `dialogue/dialogue_state`
and remains pure/shared; wrappers are not an independent runtime component.

### Per-responsibility disposition

| Function / responsibility | Decision | Reason |
|---|---|---|
| `TraceDialogueBoundary` | `Remain` | Native `XMM6`/`RSI` adapter; lock, target-read, XMM1 write, publication and diagnostics ordering spans several owner-bound components. |
| `GetDialogueBoundaryStateView` | `Remain` | Constructs references to the single RuntimeState; exporting it would expose the same mutable owner surface rejected in B2. |
| `ResetDialogueRuntimeState` | `Remain` | Caller-owned Dialogue mutex/gate contract and before/after diagnostic ordering must remain with all existing call sites. |
| `ResetPostCinematicDialogueExclusion` | `Remain` | Runtime-state binding and lifecycle call-site wrapper around existing domain operation. |
| `ArmPostCinematicDialogueExclusion` | `Remain` | Combines the resident EXIT target with lifecycle arming and ordered logging. |
| `ObservePostCinematicDialogueRecovery` | `Remain` | Native callback input and recovery/lifecycle logging remain at the existing callback boundary. |
| `InstallDialogueBoundary` | `Remain` (B7) | Hook install/commit/capability lifetime is explicitly outside B4 and remains unapproved. |
| `DialogueBoundaryStateView` | `Already extracted` | Shared transaction view is defined in `dialogue_boundary_transaction.hpp`. |
| `ProcessDialogueBoundary` | `Already extracted` | Shared lifecycle/transform decision helper in `dialogue_boundary_transaction.hpp`. |
| `ResetDialogueBoundaryState` | `Already extracted` | Shared reset operation in `dialogue_boundary_transaction.hpp`; runtime wrapper retains lock/log responsibilities. |
| `DialogueBoundaryIsActive` | `Already extracted` | Shared state predicate in `dialogue_boundary_transaction.hpp`. |
| Dialogue phase/candidate/rearm/exclusion policies | `Already extracted` | Domain state and transforms are in `src/dialogue/dialogue_state.*` and `dialogue_fov.*`. |
| `GetDialogueSourceProvenanceTrace` | `Remain` | Lazily binds process-lifetime diagnostic storage to Diagnostic Mode and owner. |
| `ReadDialogueWriterSnapshot` | `Remain` | Joins Dialogue callback evidence to the shared Cinematic writer-association store. |
| `DialogueSourceStageName` | `Remain` | Runtime diagnostic string bridge; no independent policy responsibility. |
| `EmitDialogueSourceEvent` | `Remain` | Uses the runtime gate-aware logger and deferred emission contract. |
| `RecordDialogueSourceCallback` | `Remain` | Binds callback phase/coordinator/writer identity to the already-extracted trace model. |
| `RecordDialogueRecoverySource` | `Remain` | Binds recovery source/coordinator/writer identity to the trace model. |
| `RecordDialogueSourceEnd` | `Remain` | Emits lifecycle-end evidence before the runtime reset operation. |
| `DialogueSourceProvenanceTrace` | `Already extracted` | Event/state policy is in `src/diagnostics/dialogue_source_provenance.*`. |
| `DialogueDiscoveryNowNs` | `Remain` | Optional observer's clock source is coupled to its RuntimeState-backed timer and diagnostic lifetime. |
| `LogDialogueDiscoveryEnd` | `Remain` | Optional worker lifecycle and runtime logger adapter; worker path is outside a callback-only extraction. |
| `LogDialogueDiscoveryChanges` | `Remain` | Optional observer formats current/prior runtime samples and emits through the gate-aware logger. |
| `TraceDialogueDiscovery` | `Remain` | Optional compile-time callback observer consumes the same native callback context and Dialogue lock. |
| `DialogueDiscoveryLoop` | `Remain` (B7) | Worker start/stop ownership is outside B4; optional profile remains unapproved. |
| `DialogueLifecycleCapabilityAvailable` | `Already extracted` | Existing feature-capability predicate is in `src/plugin/feature_status.*`. |
| `LogDialogueDiagnosticReturn` | `Remain` | Conditional diagnostic counters/logger are runtime-owned; Diagnostic Mode does not imply this compile-time branch is tested. |
| `LogDialogueDiagnosticSummary` | `Remain` | Reset-time summary remains with B7 reset orchestration; no B4 move. |
| `ArmDialogueRecoveryEndpointTrace` | `Remain` | Compile-time diagnostic trace shares the callback's native sample and owner lifecycle. |
| `ObserveDialogueRecoveryEndpointTrace` | `Remain` | Compile-time diagnostic observation executes inside the Dialogue critical section. |

### V0-B evidence and gaps

The existing actual-adapter harness includes production `runtime.cpp` once,
uses the real `SafetyHookContext`, resident RuntimeState/gate and production
Dialogue callback, and substitutes only bounded external leaves. Recorded
Dialogue cases include Active promotion, feature-off pass-through,
non-Gameplay refusal, unreadable native target, Exiting/RearmPending and
completion, foreign-source re-arm cancellation, disable-in-flight pass-through,
XMM1/publication-wake ordering, and callback serialization. They ran in the
recorded ordinary/diagnostic profiles and Diagnostic Mode settings; consult
the B0/V0 report for exact aggregate run counts and evidence limits. These
results are useful characterization, not full V0-B completion or live
trampoline/game evidence.

Not covered exhaustively: all native RSI/RBX identity pairings and source
replacement points; every invalid XMM6/target/readability permutation across
Candidate/Active/Exiting/RearmPending; all reset call-site interleavings and
failure paths; exhaustive publication/wake ordering for each early return;
and live hook/trampoline behavior. Thus existing V0-B evidence does not
authorize a physical move by itself.

### B4 boundary decision — approved disposition (2026-10-09)

The policy/state-machine work is already extracted. The remaining callback,
state-view and reset functions form the owner-side integration boundary;
moving them would relocate coupling rather than reduce it. No independently
cohesive extraction slice currently meets the requested complexity criterion.
The user approved this disposition on 2026-10-09. B4 is **Complete — with
documented Remain decisions**: retain the B4 runtime adapters as `Remain` and
record the Dialogue domain helpers as `Already extracted`. The incomplete
RSI/RBX and invalid-input/reset portions of V0-B remain evidence limitations,
not completed checks; they do not overturn the approved `Remain` decision and
must not be represented as full Dialogue runtime validation. No new owner API
or synchronization mechanism was introduced. No tests/builds were rerun
because this closure changed documentation only.

### Historical stop condition — before B5 approval (2026-10-10)

At that earlier checkpoint B5 boundary review was the only authorized next
step. This historical gate was superseded by the dated B5 approval and B6
review authorization below; it is retained to preserve decision history.

## Historical approval checkpoint — B1–B6

B1–B6 are complete with their approved extraction/Remain dispositions. B2 includes hotkey mapping/persistence; B3 publication/freshness and semantic-readback adapters remain; B4 Dialogue adapters remain; B5 Cinematic adapters remain; B6 Gameplay native adapter remains. V0-B remains partial and its evidence gaps are not closed. V0-C source review is complete, but Initialize/install/reset wiring remains a runtime validation gate for any B7 closure/implementation claim.

## B5 boundary review and B1–B5 architecture checkpoint — 2026-10-10

The authorized read-only B5 review is recorded in
`research/evidence/runtime-decomposition/B5_CINEMATIC_CALLBACK_BOUNDARY_REVIEW_2026-10-10.md`.
Current source confirms that the remaining cinematic callbacks bind native
context and instruction continuation to the single runtime owner, correction
gate, lifecycle generation, writer/recovery association, shared publication,
Dialogue reset, deferred logging and Gameplay recovery. The corresponding
selection, aspect, lifecycle transaction, recovery decision and continuation
trace logic is already in domain/helper components.

The review recommended B5 **Complete — with documented `Remain` decisions**.
That recommendation was subsequently approved; B5 is closed. It found no physical move that presently
reduces coupling without widening the owner interface or splitting the
cross-callback lifecycle. Existing evidence is sufficient to support this
bounded `Remain` recommendation, but not a claim of complete native/live
correctness. In particular, original EXIT instruction loads/call relocation,
live target binding, whole Initialize/install/reset orchestration, callback
quiescence and replay provenance remain limited as described in the audit.

The checkpoint concludes that B1–B5 have separated the owner boundary,
settings persistence/mapping, pure publication/freshness operations, Dialogue
domain state/transactions, and Cinematic selection/aspect/lifecycle/recovery
helpers. Within this scope, `runtime.cpp` is predominantly the composition
root and native integration owner. At the time this checkpoint was written,
B6/B7 had not been reviewed. The user subsequently approved B5 closure and
authorized the B6 boundary review; B6 findings and its pending recommendation
are recorded in the dated addendum below. B7 remains unreviewed and
unapproved.

The checkpoint incorporates the current N01–N03 remediation report: N01 and
N02 are `Fixed` within their offline/tested bounds; N03 is **Partially
mitigated**. The preceding live smoke logs identify a different ASI hash from
the remediation report's final production ASI, so they do not provide exact-
artifact runtime validation. N04–N06 remain independent evidence gates. No
production/vendor code, test, build, runtime, or release artifact was changed
or exercised by this documentation-only review.

## B5 closure and B6 boundary-review checkpoint — 2026-10-10

The user approved the B5 recommendation. B5 is **Complete — with documented
Remain decisions**: native Cinematic ENTER/EXIT/aspect adapters, source/
generation/recovery wiring, B3 publication/wake integration, and production
Cinematic installer commit/rollback remain with the runtime owner; domain
selection/aspect/lifecycle/recovery/continuation helpers are already
extracted. The detailed dispositions and evidence limits are in the B5 review
report and its move-ledger addendum. No B5 code change was requested or made.

The user authorized B6 boundary review only. That read-only review is recorded
in `research/evidence/runtime-decomposition/B6_GAMEPLAY_WRITER_BOUNDARY_REVIEW_2026-10-10.md`.
It analyzes the native Gameplay writer adapter, RuntimeCombine/HorPlus/
AspectRecalculation routing, Cinematic recovery handoff, source identity,
publication ordering, and existing actual-adapter evidence. B6 implementation
is not approved; the report's recommendation was subsequently approved, and
B6 is now closed as **Complete — with documented Remain decisions**.

At the B6 closure checkpoint, B7 review and implementation remained
unapproved; the subsequent B7 review and recommendation are recorded below.
N04–N06 stay independent verification gates and are not addressed by source movement. The prior live
smoke logs' loaded ASI SHA-256 `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`
still differs from the N01–N03 report's final production artifact
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`; this
remains an open exact-artifact runtime validation item. No tests, builds, or
game runtime were run for these documentation-only updates.

No callback extraction, game launch, ASI installation or task-log closure occurred. The B0/V0 results report remains historical and unchanged.

### B6 closure — 2026-10-10

The authorized read-only review is recorded in
`research/evidence/runtime-decomposition/B6_GAMEPLAY_WRITER_BOUNDARY_REVIEW_2026-10-10.md`.
It classifies the native writer adapter, RuntimeState-bound state/engine
operation factories, publication and diagnostic call sites as recommended
`Remain`; the transaction routing, mode calculations, and recovery policies
already in domain/transaction helpers are `Already extracted`. No independent
move was found that reduces coupling without widening the owner interface or
splitting B2 settings, B3 publication, B4 Dialogue, and B5 Cinematic recovery
dependencies.

The user approved these classifications and B6 is **Complete — with
documented Remain decisions**. B6 had no production movement. B7 was not
reviewed and remains unapproved. N01/N02
are `Fixed` within the remediation report's offline/tested bounds; N03 remains
`Partially mitigated`; N04–N06 remain independent evidence gates. Exact
artifact live validation remains open because the previous live-smoke ASI
hash `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`
differs from final remediation artifact
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
No tests, builds, or game runtime were run for this documentation-only review.

## Historical architecture checkpoint — B1–B6 — 2026-10-10

See [final architecture checkpoint](../../research/evidence/runtime-decomposition/RUNTIME_CPP_DECOMPOSITION_FINAL_ARCHITECTURE_CHECKPOINT_2026-10-10.md)
for the concise summary of extracted responsibilities, owner boundaries,
approved `Remain` decisions, and independent verification gates. B1–B6 are
complete individually; at that checkpoint B7 had not yet been reviewed. This
historical checkpoint is superseded as the campaign status by the final B1–B7
checkpoint below.

## B7 final boundary review and B1–B7 checkpoint — 2026-10-10

The B7 review was read-only and covered the full B7 plan/move-ledger inventory:
startup entry/orchestration, installers and rollback, WorkerLifecycle and
runtime worker loops, reset/shutdown, partial failure cleanup, readiness/gates,
and Overlay startup handoff. Findings and responsibility-level dispositions
are in
`research/evidence/runtime-decomposition/B7_STARTUP_INSTALL_WORKER_RESET_BOUNDARY_REVIEW_2026-10-10.md`.

The user approved B7 closure on 2026-10-10 as **Complete — with documented
`Remain` decisions**, with no new source split. `WorkerLifecycle`, DLL entry,
resolver/transaction policies, and optional Overlay-start policy are already
separate; remaining B7 work composes the single RuntimeState/HookSet and
crosses B1–B6 owner boundaries. B7 and the full B1–B7 decomposition campaign
are closed. No further source splits are part of this campaign.

The final synthesis is
`research/evidence/runtime-decomposition/RUNTIME_CPP_DECOMPOSITION_FINAL_ARCHITECTURE_CHECKPOINT_B1_B7_2026-10-10.md`.
It preserves B1–B6 decisions and leaves N03 `Partially mitigated`, N04–N06
open, and live validation using final ASI
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` pending.
No production/vendor files, tests, builds, release artifacts or configuration
were changed; no tests, builds or game were run.
