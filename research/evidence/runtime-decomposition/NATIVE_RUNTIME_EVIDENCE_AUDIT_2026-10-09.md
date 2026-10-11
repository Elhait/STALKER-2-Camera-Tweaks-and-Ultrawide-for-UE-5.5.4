# Native runtime evidence audit and coverage expansion — B1–B7

Date: 2026-10-09. Scope: user-approved evidence task, **not decomposition implementation**.
Baseline is the current dirty workspace captured before this task, not an inferred
Git release. B1–B4 closure decisions, B5–B7 approvals, implementation plan and move
ledger are unchanged. No production/vendor/localization source, release artifact
or release documentation is changed. No game is launched or ASI installed.

## 1. Evidence rules and execution boundary

| Classification | Meaning in this report |
| --- | --- |
| Verified | The specific stated assertion ran against the identified production adapter/helper or local native hook and passed. Not whole-subsystem certification. |
| Partially verified | Deterministic subsets pass; original game instructions, wiring, concurrency or environment remain outside the fixture. |
| Unverified | No matching current automated or live assertion; static analysis alone does not count as runtime PASS. |
| Blocked | Safe expansion would require prohibited production seams/changes or a game process. That branch stops; other branches continue. |
| Not applicable | Contract does not belong to the particular adapter; e.g. RBX output writes are not Dialogue callback behavior. |

Proof type is kept explicit: `Verified — actual adapter` means the named real
runtime handler; `Verified — isolated/domain harness` means a production helper
with fixture-owned inputs/operations; `Verified — static analysis` means only the
inspected control-flow fact, never execution or a runtime PASS. Native local-hook
execution is a fourth, narrower proof type and is not game-hook execution.

The actual-adapter harness includes `src/plugin/runtime.cpp` once, links the
current production/vendor dependencies (59 inputs), and uses real resident state,
gate, callbacks, settings/readback/persistence handlers. `dll_entry.cpp` is not
linked into the fixture. Memory and viewport facts, Overlay wake and logging are
controlled dependencies. Fixture memory is allocated in the child process; no
game image or installed mod is touched. External native effects are not inferred
from callback-body tests.

Each named adapter case runs in a fresh child, twice (Diagnostic Mode OFF/ON).
The parent rejects child errors and 10-second timeouts; it may terminate only its
own timed-out fixture. Both ordinary and supported diagnostic compilations are
tested. This is per-case assertion coverage, not exhaustive paired equivalence
of every internal state or diagnostic code path.

New local native tests additionally execute the actual bundled SafetyHook stub
and relocation in the fixture process. They do not execute game hook sites.
Concurrent hook patch/removal, VirtualProtect failure and native game object
lifetime are deliberately not injected.

## 2. B1–B7 responsibility coverage

| Batch / responsibility | Production source / contract | Existing evidence | New evidence | Classification and remaining boundary |
| --- | --- | --- | --- | --- |
| B1 resident owner / cross-TU access | `runtime_internal.hpp`, `runtime.cpp` resident owner, `GetRuntimeState` | `runtime_owner_binding_probe.cpp`; `UnchangedContext` checks state, gate, hooks, coordinator identity | All new callback cases retain the same identity checks | Verified pointer identity in fixture; process-resident game lifetime is static/live evidence |
| B1 correction gate | `camera_correction_gate.hpp` | `camera_correction_gate_harness.cpp:20`; actual serialization cases | Settings lock ordering; nested logging scopes | Verified stated serialization; gate is **non-recursive**, not a nested-lock API |
| B2 hotkey mapping | `runtime_settings.cpp`, runtime wrapper | Registry/config mapping tests and actual settings API cases | All typed settings and persistence kinds | Verified handler paths; real keyboard polling remains unverified |
| B2 persistence | `PersistRuntimeSetting`, `PersistHotkeyActionValue` | Actual `settings-api`; config staging/replacement failure tests | All 16 setting kinds persisted and parsed through actual config loader | Verified normal actual-handler round trips; arbitrary multi-setting failure sequences not covered |
| B2 apply/select/read | `runtime.cpp:5770,5829,5942` | Actual `settings-api`, `settings-wake`, `serialization-settings/reader` | Typed/no-op/rejection/readback matrix; blocked reader must not hold hotkey mutex | Partially verified; all overlapping mutation schedules not enumerated |
| B3 shared publication | `runtime.cpp:4237,4294`; `camera_writer_transaction.hpp` | Actual writer/settings/publication/wake assertions; helper publication matrix | Writer ADS-like samples, resize, Dialogue publication ordering | Verified fixture effects; real engine final camera consumer unverified |
| B3 freshness | `runtime.cpp:4118` and snapshot helpers | `camera_state_snapshot_harness`, actual historical target / Zoom cases | Invalid sample and settings cases preserve bounded facts | Partially verified; helper diagnostic neutrality is not whole runtime neutrality |
| B3 semantic snapshot | `runtime.cpp:5971` | Actual `ReadFacts`, serialized readers; synthetic semantic snapshot harness | Settings readback and callback publication checks | Verified specified snapshot facts under fixture locks; not every cross-owner interleaving |
| B3 deferred logging | `Log`, gate deferred actions | Actual serialization captures log/wake sequence | Two nested logging scopes queue FIFO and drain only after gate release | Verified local ordering, not logger throughput/disk latency |
| B3 viewport / diagnostics | Viewport facade, `performance_events`, diagnostic wrappers | Presenter/camera integration helpers; actual OFF/ON cases | Four viewport ratios through actual RuntimeCombine | Partially verified; actual HWND, DXGI/DComp, VRR/FG excluded |
| B4 Dialogue callback | `runtime.cpp:3658`, install at `3819` | Actual baseline/candidate/active/exiting/rearm/disabled cases | RSI/RBX identities, invalid FOV/target, source changes, reset, policy snapshot, reenable, exclusion | Verified exact adapter contracts below; native indirect call/engine camera consumption unverified |
| B4 reset / state view | `ResetDialogueRuntimeState`, `GetDialogueBoundaryStateView` | Helper reset/rearm/exclusion assertions | Reset from Inactive/Candidate/Active/Exiting/Rearm, repeated reset | Verified controlled state resets; not asynchronous reset during an executing game callback |
| B5 Cinematic ENTER/EXIT | `runtime.cpp:3047,3263` | Actual initial baseline/continuation/generation/foreign association/recovery cases | Invalid ENTER; rejected EXIT; indexed EXIT adapter; disable/reenable | Partially verified: post-load adapter and lifecycle yes, original loads/CALL relocation at game site no |
| B5 aspect adapter | `runtime.cpp:2340,3032` | Actual aspect valid/read-only/OFF and serialization cases | Null/inaccessible target RIP/context behavior; stopped EXIT also covered in this case | Verified controlled store/skip behavior; executable site binding is static/live |
| B5 transactional installers | `runtime.cpp:3486`; `cinematic_initialization.hpp` | Helper aspect/FOV/commit failures; resolver/capability harnesses | Explicit commit and reverse rollback sequence on false/throw failures | Verified helper only. **Production Initialize/install/reset orchestration blocked** without seams/game |
| B6 Gameplay writer | `runtime.cpp:4511`, install `6468` | Actual native/HorPlus/aspect/RuntimeCombine/OFF/failure matrix | RSI/RBX matrix; invalid fields; ADS-like source/input sequence | Verified callback output; original `[RBX+0x30]` instruction not executed by these adapter cases |
| B6 RuntimeCombine | Actual writer + transaction/memory helpers | Invalid viewport/readback/read-only/telemetry tests | Resize 16:9 → 32:9 → 3:1 → 21:9 | Verified synchronization/output/flags in fixture; live resize timing unverified |
| B6 baseline/ownership/recovery | Coordinator, baseline store, recovery owner | Actual recovery/foreign source/continuation; domain matrices | Disable/reenable does not resurrect old cinematic generation | Partially verified; real camera source churn/ADS/artistic FOV must be checked live |
| B6 Zoom observation | `runtime.cpp:5223,5291,5318` | Actual Zoom read-only/invalid/neutral context cases | Local wrapper exception test does **not** cover Zoom registration | Static exception-containment gap identified; runtime failure injection blocked |
| B7 startup | `Initialize:6323`, `InitializeThread:6688`, `dll_entry.cpp:26` | Optional Overlay startup leaf, capability/helper tests | No full Initialize invocation | Unverified / Blocked; would scan/patch uncontrolled image and initialize unrelated subsystems |
| B7 worker control | `worker_lifecycle.*` | Normal worker, NULL-entry rejection, capacity/stop-before-start | Actual reset self-join refusal, later successful external join | Verified fixture-owned worker subset; OS CreateThread failure not injected |
| B7 reset | `ResetAllRuntimeResources:745` | Static order, worker helpers | Empty/idempotent reset; local active MidHook + worker reset | Verified serialized local reset and byte restoration; partial production hook installation and game callback quiescence unverified |
| B7 hotkey loop / detach | `HotkeyLoop:6173`, `Shutdown:6711`, `NotifyProcessDetach:6717` | Predicate/settings helpers; static source | No synthetic OS-key or loader-lock execution | Unverified live wiring; dynamic unloading is unsupported, not newly declared safe |

## 3. Existing tests: what their assertions actually establish

Paths below are repository-relative. References identify assertion/function
locations, not just suggestive harness names. None of these tests is removed.

| Test / assertion source | Established facts | Evidence limits |
| --- | --- | --- |
| `tests/plugin/runtime_native_adapter_harness.cpp` (`UnchangedContext`, writer/aspect/lifecycle/initial/continuation/serialization/recovery/settings/Dialogue/Zoom branches) | Actual adapters mutate only permitted context bytes, preserve owner/gate identities, and produce asserted memory/publication/wake facts | Controlled dependency boundary; original displaced game instructions not generally executed |
| `tests/plugin/runtime_owner_binding_probe.cpp` | Separate TU gets identical owner/gate/hooks | Not lifetime/destruction or ABI proof |
| `tests/config/runtime_settings_api_harness.cpp:122,132,138,182,259,270,281,294` | Publication-once, type validation, dispatch/rejection and callback behavior | API uses fixture callbacks, not runtime handlers; actual adapter matrix supplies complementary evidence |
| `tests/config/config_persistence_harness.cpp:320,333,407,420,875` | Template/update staging/replacement failure preserves original; hotkey round trip | Not full native runtime Apply→Persist orchestration on every filesystem failure |
| `tests/plugin/camera_setting_transaction_harness.cpp:71,73,80,98,106,132,151` | Enabled epochs, pending state and setting transactions | Fixture state views / mode callback |
| `tests/plugin/camera_correction_gate_harness.cpp:20` | Mutual exclusion and deferred reentry | Numeric mode loop does not execute five actual runtime algorithms |
| `tests/plugin/camera_writer_transaction_harness.cpp:254,315,367,390,420,427,436` | RuntimeCombine transaction, rollback/failure/flags | Controlled memory engine, not game writer |
| Same file: `449,561,585,742,858,949,1014,1096,1199,1211,1332,1458,1545,1610` | OFF side-effect order, publication facts, restoration/recovery | Helper engine wires publication manually; not actual `ReplayManualTransition` by itself |
| `tests/camera/camera_state_snapshot_harness.cpp:18` | Snapshot projection/stable publication/historical target | Same-input diagnostic comparison is not actual Diagnostic toggle coverage |
| `tests/overlay/semantic_snapshot_harness.cpp:36,41–55` | Projection from supplied facts | Fabricated facts; no runtime lock/source acquisition |
| `tests/plugin/dialogue_boundary_transaction_harness.cpp:104,121,128,147,159,166,179,231,244` | Candidate/active/exiting/rearm policy and transaction commits | Helper XMM commit is a fixture float; active A→B source transition is deliberately accepted |
| `tests/dialogue/*candidate_hardening*`, `*rearm*`, `*policy_snapshot*`, `*post_cinematic_exclusion*` | Domain candidate/rearm/snapshot/exclusion decisions | Pure/helper evidence, not native call ABI |
| `tests/plugin/cinematic_lifecycle_transaction_harness.cpp:376,385,398,423,449,483,504,519,556,582,648,707` | Generation, aspect selection, establishment, exit/recovery decisions | Controlled operations, no native game CALL/loads |
| Same file: diagnostic scenario at `339`, events/digest near `1455–1570` | Selected helper events/state identical for controlled diagnostics | Not complete instrumentation equivalence |
| `tests/cinematics/horplus_post_exit_recovery_harness.cpp:53,112,170,651` | Recovery interpolation/gates and manually composed writer flow | Composition is not production writer callback |
| `tests/overlay/camera_integration_harness.cpp:77,137,168,202,232,269` | Potential/Actual, FOV roles, transition projection | Supplied semantic facts; not engine final camera |
| `tests/overlay/composition_presenter_state_harness.cpp:63,151,177,359` | Generation/minimize/recovery/publish-refresh scheduling | No GPU presentation / actual wake delivery |
| `tests/lifecycle/worker_lifecycle_harness.cpp:29,40,50,57` | Start/join, NULL-entry refusal, capacity, stop-before-start | “Partial startup” is not injected CreateThread failure |
| `tests/cinematics/cinematic_initialization_harness.cpp:74,85,96,109,122` | Native bypass and helper installer rollback counts | Success fixture may omit commit callback; not actual installed hook set |
| `tests/plugin/optional_overlay_startup_harness.cpp:6` | Actual `TryStartConfiguredOverlay` leaf disabled/enabled/throw handling | `InitializeOptionalOverlayThenCore` helper is not the production `InitializeThread` call path |
| `tests/feature_status/feature_status_harness.cpp:19,49` | Capability derivation/matrix | Local peer-status assertion is not startup failure isolation |
| Memory/game-camera/resolver harnesses | Real allocated buffers, permission checks, instruction decoding and resolver predicates | No current game source lifetime or all supported game-image hook execution |
| `tests/camera/gameplay_baseline_harness.cpp:87` | Store acceptance/qualification rules | Neutral runtime qualification has separate predicates; store PASS alone insufficient |
| `tests/gameplay/zoom_transition_harness.cpp:25,29` | Helper output | Expected numerical helper repeated as oracle, not independent math evidence |
| `tests/regression/runtime_evidence_replay_harness.cpp:49` and four replay fixtures | Recorded primitive endpoint/math checks | All four fixture game/mod/build identities are `UNKNOWN`; no current-version native runtime certification |

Historical `GLOBAL_HORPLUS_RUNTIME_PASS.md` observations belong to their recorded
game/mod binaries only. Historical Dialogue candidate conclusions must not
override current source: observation and correction enablement are distinct.

## 4. New deterministic coverage

Implementation: `tests/plugin/runtime_native_evidence_cases.hpp`, included inside
the existing harness. No alternate production callback implementation is added.
Names are dispatched by the unchanged fresh-process runner mechanism.

| New named case | Actual boundary and assertion | Classification / limit |
| --- | --- | --- |
| `evidence-dialogue-identities` | Actual safe-wrapped Dialogue; RBX distinct/same/null/inaccessible; RSI valid; XMM6 input → low XMM1; whole other context and output memory unchanged; publication/wake result | Verified callback. RBX is unused; does not certify original indirect call |
| `evidence-dialogue-invalid-fov` | 5 phases × NaN/+Inf/−1/0/1/179 input; unchanged XMM1, no new FOV, phase/reset rules | Verified invalid callback body |
| `evidence-dialogue-invalid-target` | 5 phases × NaN/+Inf/1/179 target and inaccessible source target read | Verified phase-specific refusal/cancellation, not pointer lifetime races |
| `evidence-dialogue-source-change` | Candidate/Active/Exiting/Rearm × foreign/null RSI | Verified characterization: Candidate/Rearm cancel; Active/Exiting preserve captured policy and can transform valid input |
| `evidence-dialogue-reset` | All 5 phases; actual reset clears tracked lifecycle/baseline/context/policy fields; idempotence | Verified serialized reset |
| `evidence-dialogue-policy-snapshot` | Actual selected policy Disabled while active Adaptive retained; unchanged enabled epochs; new activation captures Disabled | Verified selection versus active policy distinction |
| `evidence-dialogue-reenable` | OFF→ON disarms in-flight Dialogue; no resurrection before new activation | Verified in-flight settings behavior |
| `evidence-dialogue-exclusion` | Real post-cinematic exclusion and CinematicActive owner suppress/reset Dialogue | Verified controlled exclusion |
| `evidence-writer-identities` | RSI valid and RBX distinct/same/null/inaccessible; actual HorPlus output; null/inaccessible RSI pass-through | Verified callback only; original RBX store not applicable to callback body |
| `evidence-writer-invalid-fields` | Bad stored aspect and native input; full context unchanged | Verified refusal subset |
| `evidence-writer-ads` | Qualified baseline, FOV 60/70/90 and replacement RSI; actual native/output publication | Verified synthetic ADS-like numerical/source sequence; **not physical ADS proof** |
| `evidence-runtime-resize` | Actual RuntimeCombine at four viewport ratios; camera aspect memory/output/flags | Verified controlled viewport changes |
| `evidence-cinematic-invalid-enter` | NaN ENTER operand retained; observed generation/owner advances; target remains NaN and no numeric boundary evidence fabricated | Verified. `fovApplied` tracks enabled selection, not successful transformation; it may remain true |
| `evidence-cinematic-rejected-exit` | EXIT without accepted selection leaves context/owner/evidence/exclusion unchanged | Verified rejection |
| `evidence-cinematic-indexed-exit` | Indexed mode with unusable RBX/RAX; already-loaded XMM0 consumed without adapter dereference | Verified adapter; **original indexed MOVSS not executed** |
| `evidence-cinematic-cancel` | Actual OFF/ON retains observation owner, drops current correction, only new ENTER rearms | Verified fixture generation/cancellation |
| `evidence-cinematic-aspect-targets` | Null/no-access aspect target follows current skip/store rules; separate stopped EXIT keeps context unchanged | Verified aspect RIP/store and EXIT stop behavior |
| `evidence-settings-kinds` | All 16 typed setting kinds, accepted no-ops, wrong types/invalid locale string, hotkey conflicts and readback | Verified actual handlers. Internal typed Native compatibility acceptance is not user-visible mode availability |
| `evidence-settings-lock-order` | Actual reader blocks on resident gate without acquiring hotkey mutex; after release reads committed values | Verified scheduled contention; 30-ms wait not exhaustive scheduler proof |
| `evidence-persistence-kinds` | All 16 actual persistence mutations, actual `LoadFeatureConfig` readback, Native persistence rejection | Verified normal file round trips in unique fixture temp file |
| `evidence-reset-empty` | Actual reset closes gates/readiness and clears state; second reset safe | Verified empty-resource branch |
| `evidence-exception-boundary` | Actual `SafeMidHookEntry` catches throwing fixture callback, context unchanged | Verified wrapper; not Zoom registration or injected real camera exception |
| `evidence-nested-deferral` | Nested logging scopes under **one** gate defer two logs FIFO until release | Verified ordering. No recursive gate acquisition |
| `evidence-native-trampoline` | Actual local MidHook StartDisabled→enable→disable→reenable/throw→disable/reset; native float result, ABI frame relations, relocated bytes restored | Verified local stub/relocation and exception continuation; not game-site ABI |
| `evidence-reset-self-join` | Actual reset on owned worker refuses self-join and retains capability/selection; external join then reset clears state | Verified serialized-control fixture; not concurrent controller calls or retained real game hooks |
| `evidence-reset-native-hook` | Actual reset joins owned worker and removes active local native hook; original bytes/function result restored | Verified normal local teardown, not failed protection restoration |
| `evidence-install-transaction-order` | Actual helper false/throw at aspect/FOV/commit; commit order and rollback FOV→aspect | Verified production helper with controlled installers; not `Initialize` wiring |

There are **27 new names**, added to 64 existing names: **91 cases**, 182 fresh
processes per compilation profile, 364 across the two profiles. Parameter loops
inside a case are not counted as additional fixture processes or harnesses.

### Assertion location index

New-case entry/assertion ranges in `tests/plugin/runtime_native_evidence_cases.hpp`
(the case table above specifies the relevant assertions; dispatch follows at 646):

| Cases | Test function / starting line |
| --- | --- |
| Dialogue identities / invalid FOV / invalid target | `EvidenceDialogueIdentities:49`, `EvidenceDialogueInvalidFov:70`, `EvidenceDialogueInvalidTarget:96` |
| Dialogue source / reset | `EvidenceDialogueSourceChange:125`, `EvidenceDialogueReset:149` |
| Dialogue policy snapshot / reenable / exclusion | `EvidenceDialoguePolicySnapshot:168` (false/true branches), `EvidenceDialogueExclusion:199` |
| Writer identities / invalid fields / ADS-like | `EvidenceWriterIdentities:215`, `EvidenceWriterInvalidFields:234`, `EvidenceWriterAds:249` |
| Resize / invalid ENTER / rejected EXIT / indexed EXIT | `EvidenceRuntimeResize:268`, `EvidenceCinematicInvalidEnter:282`, `EvidenceCinematicRejectedExit:293`, `EvidenceCinematicIndexedExit:306` |
| Cinematic cancellation / aspect targets | `EvidenceCinematicCancel:321`, `EvidenceCinematicAspectTargets:347` |
| Settings kinds / lock order / persistence | `EvidenceSettingsKinds:365`, `EvidenceSettingsLockOrder:400`, `EvidencePersistenceKinds:427` |
| Empty reset / exceptions / deferred logs | `EvidenceResetEmpty:459`, `EvidenceExceptionBoundary:481`, `EvidenceNestedDeferral:490` |
| Local trampoline / self-join reset / native-hook reset / transaction | `EvidenceNativeTrampoline:529`, `EvidenceResetFailedSelfJoin:572`, `EvidenceResetNativeHook:595`, `EvidenceInstallTransactionOrder:624` |

Existing actual-adapter assertions start at `UnchangedContext:93`, `Gameplay:144`,
`Aspect:212`, `Lifecycle:225`, `RestorationLogging:298`,
`CinematicGameplayBaselineContinuation:331`, and
`CinematicInitialGameplayBaseline:483`, `Dialogue:576`, `Zoom:599`,
`SettingsAndWake:610`, `Serialization:633`, `HistoricalTarget:664`, `Recovery:678`,
`SettingsApi:741` and `DialogueLifecycle:802` in the existing harness. The unchanged
case names are preserved in the case registry beginning at line 882; raw matrix
logs print each name and Diagnostic mode next to its PASS. The final full-test
log contains both compilation profiles; standalone matrix logs identify their
respective profile separately.

### Test-construction failures retained honestly

Initial focused runs did not all pass. Recorded failures included command-line
quoting, generated dependency response-file placement, nonexistent fixture symbol
assumptions, and three incorrect test expectations. They were corrected in
test-only code after checking source:

- Policy changes do not increment Enabled epochs.
- Invalid ENTER can set `fovApplied` for enabled selection while transformed FOV
  remains invalid; this flag is not a transform-success predicate.
- The correction gate uses `std::mutex`, not recursive acquisition. Test logging
  may nest scopes, but the fixture must not reacquire a held gate or call a handler
  which acquires it on the same thread.

Internal typed Native policy acceptance and user-visible/persistable mode support
also differ. These are current contracts, not production changes introduced here.
The early failed attempts remain in `commands.jsonl`; the first two compiler
attempts shared an old log filename (later attempts use timestamped filenames).
No early failure is counted as a final PASS.

## 5. Native callback / trampoline contract matrix

| Boundary | Native inputs / displaced instruction | Permitted adapter changes | Verified evidence | Unverified / blocked |
| --- | --- | --- | --- | --- |
| Gameplay writer | RSI camera source; RBX destination; low XMM0 already computed; original `MOVSS [RBX+0x30],XMM0` | Low XMM0 and validated camera aspect/flags according to mode; publication/wake | Whole-context sentinels except low XMM0, memory-effects checks, RSI/RBX matrix | Original game store not executed by adapter fixture; null RBX adapter PASS does not make native store safe |
| Cinematic aspect | RAX target; replaced 10-byte store instruction | Actual validated store and `RIP += 10` skip (including refused store contract) | Valid/read-only/OFF/null/no-access cases; other context bytes | Actual game target lifetime/instruction binding |
| Cinematic ENTER | Post-native-load low XMM0; call site relocation | Low XMM0; selection/generation/ownership/evidence | Initial/continuation matrix plus invalid ENTER | Engine call side effects / authored camera consumer and game-site call stack |
| Cinematic EXIT | Post-load low XMM0; legacy `[RDI+0x38]` or indexed `[RBX+RAX*4+0x38]` before callback | Context preserved; state handoff/recovery | Rejected/accepted/indexed adapter paths | Both actual original load variants in game; adapter does not validate original source operand accessibility |
| Dialogue | Low XMM6 input; RSI target `+0x2C`; displaced `CALL [RAX+0x608]` at signature+9 | Low XMM1 only, lifecycle/publication; XMM6 unchanged | All other context bytes unchanged; 5 phases and identity/input/reset matrices | Original vtable call, caller ABI at live site, engine consuming XMM1 |
| Zoom IN/OUT | Validated post-load `UCOMISS XMM1,XMM0`; source offsets in RAX/RSI for diagnostics | No context mutation; telemetry/freshness | Actual adapter sentinel checks and source static analysis | Original comparison flags, generated game trampoline, exception boundary gap |
| Generic local MidHook | Compiler-defined Win64 `float(float)` leaf; displaced ADDSS instructions | Fixture observer changes low XMM0 to 5 or throws before change | Native execution produces 11 unhooked / 13 hooked / 11 on contained throw; enable/disable/restoration | Does not establish all game signatures, extended register state, trap races or failure rollback |

SafetyHook `Context64` (`external/safetyhook/safetyhook.hpp`, near 850) saves
XMM0–15 lower 128 bits, RFLAGS and GPRs. `rsp` at 0x180 is informational;
`trampoline_rsp` at 0x188 participates in return-stack restoration; `rip` at 0x190
selects continuation; structure size 0x198. These offsets are asserted locally.
Do not infer preservation of upper YMM/ZMM, x87 state or all MXCSR state.

The Windows x64 stub (`safetyhook.cpp:774` vicinity) establishes an aligned callback
stack and shadow space, passes context in RCX, restores saved registers/flags, and
returns through the relocated continuation. The local test checks aligned context,
saved RSP alignment at a compiler function entry, and `trampoline_rsp + 8 == rsp`.
Those function-entry stack relations are not imposed on arbitrary mid-function
game sites. The local emitted 12-byte relocation contains `ADDSS XMM0,XMM0` and
RIP-relative `ADDSS XMM0,[constant]`; its exact bytes are printed in raw logs.
It does not exercise a relocated game indirect CALL or indexed MOVSS.

A relocated CALL can preserve its callee/flow while changing the return address
to trampoline storage. No bit-identical call-stack provenance claim is made.

Publication ordering is hook-specific. RuntimeCombine verifies its aspect store
and readback before publishing transformed writer facts. Gameplay publishes the
selected low-XMM0 result **inside the callback**; the game's displaced destination
store happens later through the trampoline. Thus publication is not proof that
the engine's final output memory already contains that value. Dialogue checks
its final output publication/wake after low-XMM1 assignment; phase freshness can
also schedule earlier wakes. The new assertion does not claim every intermediate
wake necessarily carries the final transformed register value.

Exception containment is also bounded: `SafeMidHookEntry` catches exceptions but
does not roll back context/memory writes already performed by its callback. The
new throwing fixture intentionally throws before its register write; it proves
continuation/containment, not atomic rollback after arbitrary partial effects.
Injecting throws at every production callback side-effect point would require
new prohibited seams and remains a blocked branch.

## 6. Synchronization, lifecycle and findings

The resident correction gate is non-recursive. Camera readers/setting commits
acquire it before nested subsystem locks. `ReadRuntimeSettings:5942` specifically
uses gate → hotkeyConfigMutex. Deferred Log emission occurs after gate unlock.
Tests establish selected orderings, not a model-check of all mutex interactions.
Worker Start/StopAndJoin are externally serialized runtime control operations;
the new self-join fixture transfers that control, rather than inventing supported
parallel controller calls.

| ID / priority | Finding and evidence level | Source / consequence | Minimal next action; current disposition |
| --- | --- | --- | --- |
| N01 / P1 | **Static conditional hook-teardown defect.** Not an observed game crash | `SafetyHook InlineHook::enable:699–725`, `disable:731`, `destroy:748`; `trap_threads:1450–1473`. Protection restoration can fail after patching, before enabled-state commit. Failed disable is ignored by destroy, which frees trampoline. A still-patched target can refer to released storage | Separate production/vendor remediation with controlled protection-failure injection; preserve hook storage on uncertain detach. **Blocked here**: no vendor edits or unsafe failure injection |
| N02 / P1 | **Static exception-containment gap in Zoom native entries** | `runtime.cpp:5291–5292,5318–5330` registers TraceZoom directly, unlike `SafeMidHookEntry:1392`. `TraceZoomTransition:5223` calls `MarkCameraStateFreshness:4118` with a locking path; `std::mutex::lock` can throw | Separate narrow production boundary fix and actual registration regression upon approval. **Blocked here**: wrapper fixture cannot prove direct Zoom entry containment |
| N03 / P1 | **Static startup exception gap before first try** | `Initialize:6323–6333` constructs paths/config path and stream before try; `InitializeThread:6688` has no outer catch; direct CreateThread entry in `dll_entry.cpp:26` | Separate containment review/fix with actual startup fault tests. **Blocked here**: no production injection seam or game initialization |
| N04 / P2 | Whole production startup/install/partial rollback remains unverified | Leaf installer and optional Overlay helper PASS do not execute actual startup wiring/real image scanners | Live runtime gates and a separately authorized minimal failure-test strategy; do not certify Initialize from helper tests |
| N05 / P2 | Game callback quiescence / patch-removal races not proven | Reset joins owned workers, not arbitrary engine callback threads; local teardown is serialized and nonconcurrent | Validate supported process-resident shutdown; do not imply dynamic unload support or test removal while unknown callbacks run |
| N06 / P2 | Current native replay provenance missing | Four replay fixtures have UNKNOWN game/mod/build identities | Capture new explicit game+ASI hash/profile logs before treating replays as current runtime evidence |
| N07 / P3 | Existing test names can overstate scope | Partial-startup means NULL entry; some neutrality/Zoom math oracles repeat same helper/input; feature-status fixture is not actual module isolation | Preserve accurate evidence labels; improve these oracles in a separate bounded task if useful |
| N08 / informational | Active/Exiting Dialogue accepts source changes under captured policy | Current domain helper and existing source-change assertion; new actual-adapter cases agree | Characterization, not new defect or proof of foreign-source ownership safety; verify engine identity behavior live |

N01–N03 are code-supported conditional risks, **not claims that the user's prior
crash/hang was caused by them**. They are outside test-only repair authority.
`NotifyOverlayCameraCoreReady` in the supported profile uses atomic/noexcept
startup-journal/wake operations; this audit does not assert a demonstrated throw
there merely because its declaration lacks `noexcept`.

## 7. Remaining gates and concrete live procedures

Run only after separate user authorization, against the intended exact game
executable and ASI. For every capture record executable SHA-256, ASI SHA-256,
ordinary/diagnostic profile, actual INI, resolution/aspect/FOV, timestamp, observed
scenario and logs. A screenshot of Overlay target alone is not final camera
consumer evidence. No game verification was performed by this task.

| Gate | Procedure / acceptance | Current state |
| --- | --- | --- |
| Native hook-site ABI | Inspect manifest-approved unique sites/instruction bytes. Exercise writer, aspect, ENTER, both EXIT patterns when actually present, Dialogue indirect call and Zoom. Confirm continuation/no crash and expected native output | Unverified; unsupported site variants are Not applicable for that executable, not PASS |
| First ENTER / continuation | Fresh load → first cinematic (qualified Gameplay baseline differing from authored) → internal Dialogue → rapid EXIT/ENTER → final recovery. Confirm target held and generation/source association stays valid; repeat Auto and forced aspect policies | Live-required; automated adapter subset verified |
| Dialogue identities / resets | Candidate/active/exiting/rearm, Dialogue OFF→ON and policy changes during active interval; different conversation/camera sources and missing/cancelled interaction. Confirm no stale target or surprise rearming | Live-required; controlled state matrix verified |
| Gameplay / ADS / resize | RuntimeCombine, HorPlus, AspectRecalculation and OFF; aim/release/switch camera; resize/change resolution during Gameplay and between cinematic generations; inspect final picture plus native/target/output facts | Live-required; synthetic numerical/source/viewport subset only |
| Settings and hotkeys | Actual Overlay settings, ordinary hotkeys ON/OFF, rebind/capture/conflicts, persistence/restart; compare applied selection vs active lifecycle snapshot | Live-required; handler/helper tests do not synthesize OS keyboard loop |
| Overlay independence | Restart with Overlay Enabled=false and true; repeat core camera paths, hotkeys and Diagnostic OFF/ON. Confirm resources/startup logging and camera functionality | Actual enabled/disabled startup leaf tested; whole runtime/GPU independence live-required |
| Install/failure isolation | On an explicitly approved test binary/environment, verify resolver refusal and actual module capability/status, no partial live hook set or uncontrolled correction | Blocked until safe strategy approved; do not corrupt game memory or protection rights to force failure |
| Normal process exit | Finish gameplay/cinematic/dialogue then close game normally; inspect worker stop and startup/shutdown logs; do not dynamically unload ASI | Unverified live; local join/reset subset verified |
| Diagnostics neutrality | Same ASI/profile/settings/scenario with Diagnostic OFF/ON; compare final native/output observations and visible behavior, not only log counts | Per-case tests PASS does not replace live matched captures |

## 8. Validation record

All requested automated validation finished. Temporary failed fixture-construction
attempts remain in the same command ledger and are not hidden. Final PASS applies
only to the bounded checks listed below.

Evidence directory: `build-artifacts/native-runtime-evidence/2026-10-09/`.
`commands.jsonl` contains exact command lines, UTC completion time, exit code and
raw log path. Build products are isolated by `CAMERA_TWEAKS_OUTPUT_NAME` and
`CAMERA_TWEAKS_OBJECT_DIR`; the installed and root release ASIs are untouched.

Raw log filenames below resolve under that evidence directory. Toolchain: MSVC
17.14.28 x64, `/std:c++latest /O1 /MT /EHsc /W4 /utf-8`. Supported diagnostic defines
are ZOOM_TRANSITION, HORPLUS_FOV_STATE, CAMERA_STATE_SNAPSHOT,
HORPLUS_CINEMATIC_WRITER_TRACE and MATCHGAMEPLAY diagnostic instrumentation;
optional discovery/forensic profiles were not enabled.

| Check | Exit | Result | Raw log |
| --- | --- | --- | --- |
| Final ordinary focused compile | 0 | PASS; retained unchanged dependency objects, rebuilt adapter/probe | `ordinary-compile-20261009-191519-125.log` |
| Ordinary focused matrix | 0 | 91 × OFF/ON = 182 fresh processes PASS | `ordinary-matrix-20261009-191541-213.log` |
| Supported diagnostic focused compile | 0 | PASS; full source compile | `diagnostic-compile-20261009-191613-098.log` |
| Diagnostic focused matrix | 0 | 91 × OFF/ON = 182 fresh processes PASS | `diagnostic-matrix-20261009-191718-237.log` |
| `test.cmd` | 0 | 59/59 registered harnesses PASS; adapter compiled/executed in both profiles, 364 fresh adapter processes | `test-full-20261009-192407-265.log` |
| Locale/resource audit | 0 | 18/18 catalogs; 212 keys; parity/placeholders/UTF-8/resources/font metadata/tooltip checks PASS | `locale-audit-20261009-192614-958.log` |
| Full glyph/atlas harness | 0 (inside test.cmd) | All supported font sizes 12–24 completed; no missing-glyph failure | Same full log, atlas section near lines 487–499 |
| Production `build.cmd` | 0 | PASS; isolated audit ASI | `production-build-20261009-193009-383.log` |
| Supported diagnostic build | 0 | PASS; isolated audit ASI | `diagnostic-build-20261009-193205-290.log` |
| Final preservation comparison | 0 | 313 files unchanged; before/after manifests byte-identical | `preserved-before.csv`, `preserved-after.csv` |

The 364 focused processes and 364 full-run processes are two separate validation
passes of the same 91 scenarios, not 728 unique scenarios. The runner has 59
unique harness sources; the adapter's second profile is an extra compile/run,
not a 60th unique harness. No SKIP/XFAIL or live-required gate is counted as PASS.
Full `test.cmd` rebuilds the real adapter dependencies from source, so final
validation does not rely solely on the focused ordinary object's reuse.
Offline font coverage does not establish Arabic shaping/bidi/RTL or live visuals.

### SHA-256

| Artifact / source | SHA-256 |
| --- | --- |
| Isolated `production.asi` | `36A73C3E9CE942C28FDD970C045F7A1ADE0AE884008BCBA0900332B09AD5CBF0` |
| Isolated `diagnostic.asi` | `FC995FC2B3D6AD00701EDCBA0E163A8FDC0048EC8FF983A6A1D1357E39A29FBE` |
| Focused ordinary adapter executable | `B16158B39857791B9CB318AEB7FC2809302B5F44D471DC02C80241A4F60A6116` |
| Focused diagnostic adapter executable | `5FEE73B0F843D87116E7CE2CAAAF87E1ABA4434195CA42F23FBB44C1E756C798` |
| `src/plugin/runtime.cpp` (unchanged) | `CDA9E9CEA372965623B8ADD110AD741CBE7EEB28690E808E3797EC963108A9DF` |
| `src/plugin/runtime_internal.hpp` (unchanged) | `CD1B1394DC972DDE1346A5B370AECEA15C7086642D24A065EC3956E9E7C6247F` |
| `src/plugin/runtime_settings.cpp` (unchanged) | `CF1904040D5D8A5A1162807410F0F5FA53D2F8B286457A1EB6EFEA9D465BE66B` |
| `external/safetyhook/safetyhook.cpp` (unchanged) | `3C2C7FCF6FB5D0E578FD24A7155B255D01DAB653BF6A2E408D84CEAB8D4AA1F8` |
| `external/safetyhook/safetyhook.hpp` (unchanged) | `708D22B42C6BAC62E21BA86A3FCC5D8E89DBD156B09629EB81D58D6A07EACB3E` |
| Extended adapter harness source | `479E3959E579202431B6CF0FC386A0C693A3E6949FB16D821D26E94A2EF44948` |
| New test fragment | `5DEBAD8A049AE98213AAAB1A27793297521EBDF61C353586CEFEF1D6284895D5` |
| Evidence driver | `FDBB1D67081C7A0EB7641DAF90C1EB2A2C6CB7140C9A403615241641CE8961E8` |
| Before and after preservation manifests (both) | `8FAC0EC25C6C66C24AF5EF153CB4496354246D4F2B8293C31A9CCB940FF8C54B` |

Additional machine-readable artifact records: `artifact-hashes.csv`. Reported ASI
hashes identify audit builds only, not an installed or newly published release.

Reproduction from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Focused -Profile both
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Full
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Locale
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Production
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Diagnostic
powershell -NoProfile -ExecutionPolicy Bypass -File tests/runner/native_runtime_evidence.ps1 -Action Preservation
```

`Capture` was executed once before changes and refuses to overwrite an existing
baseline. Future preservation against that baseline is meaningful only while its
production inputs stay unchanged; an intentional future production revision
requires a new evidence identity/baseline, not overwriting this one.

## 9. Changed files and preservation

- New test fragment: `tests/plugin/runtime_native_evidence_cases.hpp`.
- Existing actual adapter harness: passive log capture, fragment include and 27
  named-case dispatch additions only; existing 64 cases retained.
- New evidence driver: `tests/runner/native_runtime_evidence.ps1`.
- Dedicated approved task plan and this report.

The driver captures/computes a before/after SHA-256 manifest for production,
vendor, catalogs, docs, root build/release/config files, decomposition plan and
move ledger. Preservation result and important source hashes appear in the
final validation addendum. Existing unrelated dirty changes are preserved.
`git diff --check` is unavailable in this workspace (work-tree error); direct
file review and the preservation manifest are used instead, not a claimed Git
diff PASS.

## 10. Recommendation

Use this as a bounded evidence baseline, not permission to close decomposition
batches or claim complete native runtime coverage. Prioritize separate narrow
reviews/remediation for N01–N03; retain N04–N06 as explicit verification gates.
Do not expand mutable owner access merely to obtain a green test. Continue B5–B7
only under their separate approval process.
