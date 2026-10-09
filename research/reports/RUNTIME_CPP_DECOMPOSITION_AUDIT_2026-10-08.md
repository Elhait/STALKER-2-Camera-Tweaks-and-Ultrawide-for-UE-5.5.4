# RUNTIME_CPP_DECOMPOSITION_AUDIT — 2026-10-08

## Статус і межі

**READ-ONLY аудит поточного working tree.** Єдиний створений файл — цей звіт. Production-код, headers, build scripts, tests, configuration та існуючі research records не змінювались. Build, harnesses і гра в межах аудиту не запускались.

Ціль: встановити фактичні відповідальності та залежності `src/plugin/runtime.cpp`, запропонувати семантично обґрунтоване розділення без зміни поведінки. Це audit/design input для подальшого затвердження, **не затверджений implementation plan**.

- Repository: `E:/Work/Slaker2 mods/01-Projects/STALKER-2-Ultrawide-Fix-for-UE-5.5.4`.
- HEAD: `c844d2e379f922fe72f46e98c3e9ff61dd8eaeb0`.
- `runtime.cpp`: **6800 рядків**, SHA-256 `D4E124A5A1871CE80F736BAF4AD221E2F4AB24931CA0B2DC9BD761E74F00B341`.
- Working tree вже містив значний staged набор змін. HEAD сам по собі не ідентифікує audited implementation; наведений hash і посилання стосуються поточного файлу.
- Evidence: static source inspection + читання registry/source lists тестів. Історичні PASS та користувацький runtime acceptance не переатрибутовуються цьому аудиту.
- Authority: активний `agent-rules/`, поточні `docs/ARCHITECTURE.md`, `docs/SAFETY_INVARIANTS.md`, `docs/SUPPORTED_BUILD_MANIFEST.md`, фактичний production render/runtime/build path.
- `TESTING_AND_RESEARCH.md` містить старі конфігураційні приклади й historical hashes: наприклад, HorPlus default та Dialogue.Zoom/Native. Поточний `FeatureConfig` має **RuntimeCombine default**. Звіт не використовує старі приклади як актуальну source authority.

## Висновок

Файл поєднує integration/lifetime ownership, engine adapters, hook setup, runtime settings, camera publication/readback і persistent diagnostics. Основні policy/state-machine transactions **вже виділені** в reusable headers. Наступний рефакторинг має розділяти integration adapters навколо цих меж, а не створювати заново три незалежні camera runtimes.

Найсильніша наявна межа — **один process-resident integration owner + спільний CameraCorrectionGate + reference-based transaction views**. Recovery зв'язує Gameplay, Cinematics і Dialogue; він не належить виключно одному доменному файлу.

Не встановлено необхідності змінювати algorithms, ownership priority, qualification, source association чи FOV math. Конкретні extraction risks наведені нижче; аудит не оголошує всю архітектуру або відсутність крашів доведеними.

## 1. Фактичні шари й залежності

| Шар | Поточний власник / source | Що робить runtime.cpp | Межа |
|---|---|---|---|
| DLL ABI | `plugin/dll_entry.cpp`, `plugin/runtime.hpp` | SetModuleHandle, InitializeThread, Shutdown, NotifyProcessDetach | Public facade; loader-lock contract |
| Integration state | private RuntimeState, runtime.cpp:156–355 | Process-resident storage + g_* reference aliases:362–558 | Один instance; aliases не є copies |
| Callback serialization | `plugin/camera_correction_gate.hpp` | Acquire у writer/ENTER/EXIT/aspect/dialogue/settings/readback; deferred logs | Спільна serialization boundary |
| Writer transaction | `plugin/camera_writer_transaction.hpp` | Формує inputs/views/engine operations, виконує transaction, застосовує register output, публікує і логує | ProcessCameraWriter вимагає already-held gate |
| Cinematic lifecycle | `plugin/cinematic_lifecycle_transaction.hpp` | ENTER/EXIT/aspect adapters + contextual diagnostics + observations | Generation/selection/association/recovery transactions |
| Dialogue boundary | `plugin/dialogue_boundary_transaction.hpp` | XMM6/read native target → transaction → XMM1/publication | Dialogue lock і lifecycle policy |
| Gameplay policies | `gameplay/{gameplay_state,gameplay_camera,aspect_policy,horplus_gameplay}.*` | Реальні memory operations, pending transitions, dispatch integration | Reusable policy/calculation вже існує |
| Cinematic policies | `cinematics/{cinematic_fov,cinematic_selection,cinematic_aspect,cinematic_initialization}.*`, exit_authority/recovery_retirement headers | Engine boundary й installation orchestration | Не дублювати у нових adapters |
| Camera facts | `camera/{fov_observation,gameplay_baseline,gameplay_aspect_restoration,camera_state_snapshot}.*`, camera_state_publication.hpp | Власність stores, збирання publication input, lock/wake | Store authority відрізняється від shared UI facts |
| Settings/config | runtime_settings.hpp, config repository/template/feature_config | Apply/read/persist callbacks + startup load | Mutate і persist — різні операції |
| Overlay bridge | overlay composition/runtime/renderer/language reader | Semantic snapshot, locale synchronization, notifications, startup readiness | Overlay споживає facts; own presenter lifecycle |
| Native infrastructure | hooks scanner/validator, SafetyHook, platform/win32 memory/sha256/viewport | Resolves sites; installs hooks; reads/writes memory | Signature/register/offset contracts зберігаються |
| Diagnostics | diagnostics domain helpers + inline observers у runtime.cpp | Dedup, provenance captures, ENTER/EXIT probes, formatting, optional profile hooks | Не перетворювати log caches на production authority |

`runtime.cpp` includes config, hooks, gameplay, cinematics, dialogue, camera, diagnostics, Win32 platform, Overlay bridge, SafetyHook, Zydis, spdlog. Залежність на renderer implementation не потрібна для camera calculations; фактичний зв'язок з Overlay — callbacks/readback/wake/notifications та startup.

### Dependency flow

```text
dll_entry → plugin facade → runtime integration owner
                              ├─ config load → effective settings → SettingsApi.Publish
                              ├─ hook resolvers/installers → native adapters
                              ├─ WorkerLifecycle → HotkeyLoop → SettingsApi.Apply / config persist
                              └─ optional StartOverlayDiscovery → core-ready notification

native writer / ENTER / EXIT / aspect / Dialogue
    → shared CameraCorrectionGate
    → reference views + domain transaction helpers
    → existing stores / association / recovery / coordinator
    → existing memory or register output
    → observation / shared publication
    → semantic-change wake → Overlay reads semantic snapshot
                                  → camera_integration / camera_state_view
                                  → renderer

diagnostic observers read snapshots/results → deferred Log → spdlog
```

Diagram означає залежності, не спільний порядок register write для всіх callbacks. Наприклад, writer може publish before final XMM0 assignment; Dialogue publish follows XMM1 assignment; aspect RIP skip встановлюється на початку callback. Ці локальні порядки не уніфікуються під час extraction.

## 2. State inventory і authority

Повний перелік **159 named RuntimeState members**, включно з conditional fields, міститься у Appendix B. Це source inventory, а не твердження, що всі ці fields активні в production profile.

| Група / рядки runtime.cpp | Authoritative state | Mutators / consumers | Lifetime та synchronization |
|---|---|---|---|
| Module/resources:158–168,218 | module, executable, workers, hooks, fovWriteAddress, logger, config/configPath, mutexes/gate | Initialize, installers, ResetAllRuntimeResources | Один process-resident RuntimeState; workers externally serialized |
| Effective settings/epochs:169–217 | runtime*Enabled/modes, requested і active settings epochs, capability/gates | Startup; ApplyRuntimeSetting/ApplyCameraSetting; lifecycle capture; settings/semantic readers | Atomics + correction gate; active != requested |
| Gameplay transitions:179–190,219–221,334–340 | apply/restore/mode pending, ReplayState, last source/FOV, trace dedup | Setting commit; ProcessCameraWriter; logging; legacy helpers | Не переносити pending state у settings copy |
| Coordinator:222 | CoordinatorState | Lifecycle/writer recovery transactions; readback | Gameplay/CinematicActive/CinematicExiting; не session classifier |
| Writer observations:223 | CameraFovObservationStore | Writer та accepted ENTER publication; ENTER boundary read | Boundary-scoped observations + sequence; не UI float cache |
| Transition Evidence:224 | CameraTransitionBoundaryObservation | accepted ENTER captures operands; semantic reader copies | Окремо від live state; continued-ownership carry |
| Gameplay baseline:225 | GameplayBaselineStore | Gameplay transaction projection; cinematic baseline selection; settings invalidation | Correction baseline; власний usability/source contract |
| Cinematic identity/recovery:226–227 | CinematicWriterAssociation, CinematicRecoveryOwner | Aspect/writer association; ENTER/EXIT; writer retirement/recovery | Generation-scoped association; validated writer/source ownership |
| Restoration:230 | GameplayAspectRestorationStore | Writer observations; mode/enabled transition consumers | Native aspect restoration evidence, не viewport/target |
| Shared camera state:231–236,308 | CameraStateSnapshot + valid + neutral qualification fields/eventSequence | Publication helpers; freshness boundary; semantic reader | cameraStateMutex; canonical four FOV roles |
| Cinematic numeric/continuation:237–263,277–279 | selected/applied target, authored ENTER, pre-ENTER baseline facts, native EXIT, continuation/handoff fields, exclusion | lifecycle + writer transactions | Correction/recovery authority; не заміна CameraState facts |
| Zoom:295–307 | hooks/counters + observation metadata/active/hold | Zoom callbacks; writer qualification/publication | Zoom safety входить у shared qualification; не diagnostic-only |
| Dialogue:309–311,341–349 | phase, active policy/activation, baseline/previous/exit anchor/candidate/rearm, snapshot target | Dialogue transaction; writer invalidation; settings disable; readback | dialogueMutex для lifecycle; requested atomics відокремлені |
| Diagnostics:228–229,259–261,264–294,312–330 | continuation trace, optional counters, telemetry/dedup | diagnostic capture/emission/reset | Не production source identity; conditional profile fields зберігати |

### State поза RuntimeState

| Storage | Source | Semantic role / extraction constraint |
|---|---|---|
| g_runtimeSettings | runtime.cpp:104 | Process-wide published callback bundle; publish exactly once. Збереження цього owner не вимагає нового global instance |
| g_notificationMutex, g_pendingNotifications, g_nextNotificationId | :105–107 | Bounded queue (8) і IDs; disabled Overlay publication bypass; не camera truth |
| g_cinematicDiagnosticMutex і last cinematic writer/aspect caches | :963–995 | Diagnostic dedup state, окреме від association |
| CinematicBaselineWriterPublicationState, lazy static pointer | :1263–1304 | Diagnostic provenance snapshot; lazy process-lifetime allocation |
| EnterWriterRawObservation, EnterWriterCaptureState, g_enterWriterCapture | :2005–2212 | Diagnostic pre/post ENTER samples, QPC, identity/sequence; не Transition Evidence authority |
| DialogueSourceProvenanceTrace lazy pointer | :592–600 | Diagnostic-only, allocated on demand; not lifecycle authority |
| GameplayFovSourceTraceSnapshot TLS + nextTraceSequence | :3974–4073 | Diagnostic emission dedup/sequence |
| RuntimeCombineDiagnosticSnapshot TLS | :4617–4687 | Diagnostic dedup, separate from production transaction telemetry |
| ZoomTransitionSnapshot TLS | :5186–5263 | Previous per-direction observation used for change detection; updates shared zoom metadata |
| Profile-only namespace/TLS state | :781–820,1636–1655,3512–3517,3553–3568,3821–3841,5460–5472,5565–5588 | Trigger/replay traces, Dialogue counters/recovery traces, HorPlus trace, discovery. Не створювати ці storage заново в кожній TU |

`g_coordinator` і `g_coordinatorState` обидва посилаються на `RuntimeState::coordinator` (:423–424). `g_hook`, `g_cinematic*Hook`, `g_dialogueBoundaryHook` — references до HookSet. Не дублювати state за aliases.

## 3. Hooks, callbacks і native contracts

| Boundary | Installer / callback | Site/register contract | Activation / failure / preserve |
|---|---|---|---|
| Gameplay writer | VerifyExecutableAndInstruction:5327; Initialize:6553; ReplayManualTransition:4499 | Validated MOVSS [RBX+0x30],XMM0; RSI source/RBX output; aspect +0x254/flags +0x259 | StartDisabled → assign → enable → writer gate; local failure resets hook/availability |
| Cinematic aspect | ResolveCinematicAspectStore:2934; InstallCinematicAspect:3011; ApplyCinematicAspectStore:2329 | C7 80 store, RAX target; 10-byte replaced instruction | StartDisabled; callback increments RIP before auxiliary work; inaccessible target preserves current skip behavior |
| Cinematic ENTER | ResolveCinematicFovCallsites:2952; InstallCinematicFovComponent:3468; TraceCinematicEnter:3036 | validated MOVSS + rel32 call + vcall evidence; XMM0 authored FOV | Accepted selection only; wrapper SafeMidHookEntry; generation/target/observation ordering |
| Cinematic EXIT | same installer; TraceCinematicExit:3251 | Legacy RDI+0x38 or indexed RBX+RAX*4+0x38; shared call target | Legacy first, indexed only when legacy absent; rejected EXIT performs no lifecycle transaction |
| Dialogue | InstallDialogueBoundary:3793; TraceDialogueBoundary:3646 | unique match +9, six concrete bytes FF 90 08 06 00 00; XMM6 input/XMM1 output; RSI +0x2C target read | Installed only with required observation capability; current create_mid/enable sequence retained |
| Zoom IN/OUT | InstallZoomTransitionObservation:5282; TraceZoomIn/Out:5279/5280 | Exact supported image hash, two unique matches, +13 post-load byte check; RAX/RSI fields | Installed independently of Diagnostic Mode when identity allows; rollback IN if OUT fails; callbacks currently direct |
| Optional post-EXIT observer | :5546 / ObservePostExitDiagnosticWriter:5533 | Profile-specific writer observer | POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; not ordinary production observer substitute |
| Optional producer/write-owner/consumer probes | installers:1812/1871/1934; callbacks:1785/1839/1898 | Historical exact-image RVAs/byte checks | Compile-time gated research traces; не portability claims |

Cinematic install commit: aspect + ENTER + EXIT created disabled → all enabled → `cinematicHookGate=true` (:3484). Failure disables components; `InitializeTransactional` rolls back FOV then aspect. Цю boundary не розділяти на independently initialized domain singletons.

`SafeMidHookEntry` (:1381) catches C++ exceptions. Це не AV/driver isolation і не rollback вже виконаних context/memory mutations. Під час перенесення потрібно зберегти момент mutation і поточний wrapper, а не стверджувати універсальний transactional pass-through.

Production hook pointers не мають userData параметра: потрібен один deliberate binding до існуючого resident owner. Пропозиція для implementation — мінімальний internal owner accessor/thunk, а не independent per-TU state.

## 4. Initialization, workers, shutdown

### Поточний порядок

1. `DllMain`: SetModuleHandle → DisableThreadLibraryCalls → CreateThread(InitializeThread) → close initialization thread handle. Loader-lock path не виконує hooks/joins.
2. `InitializeThread`: startup journal або opt-in timeline; `Initialize`.
3. `Initialize`: paths/log initialization → worker stop event → stopping=false → ASI/game hashes → optional profile probes.
4. Config load + managed template sync → `TryStartConfiguredOverlay(config.overlayEnabled)` → one disabled-config log.
5. Diagnostic Mode → effective atomics → `RuntimeSettingsApi::Publish` з Apply/Read/Semantic/Persist callbacks.
6. Identity-gated zoom observation hooks.
7. Cinematic transactional aspect+ENTER+EXIT setup → capability publication.
8. Gameplay writer resolution/install → gate/recovery observation capability; failure local.
9. Dialogue dependency calculation → boundary install if capability available; failure local.
10. Hotkey worker starts independently of Hotkeys.Enabled; optional profile workers follow.
11. Logs/flush → initializationComplete=true. Outer failure resets resources; current completion flag still set at function tail. Semantic read separately checks stopping.
12. `InitializeThread` signals core-ready only if Overlay startup succeeded; otherwise enabled-Overlay failure log.

Overlay worker belongs to `overlay/composition_runtime.cpp` (:898), starts its own thread, and is not in plugin WorkerLifecycle. It waits for core-ready before presentation. Do not change launch order or silently make plugin Shutdown own presenter teardown in a file split.

### Workers

| Worker | Current path | Relevant contract |
|---|---|---|
| Initialization | DLL entry → InitializeThread | Handle not retained/joined; residency contract |
| Hotkeys | StartWorker lambda → HotkeyLoop:6256 | WaitForWorkerStop(50); snapshot → rising-edge/capture suppression → mutation → notification → persistence |
| Optional trigger | OneShotTriggerLoop:2472 | GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER; this is not ordinary diagnostics |
| Optional resolution monitor | ResolutionMonitorLoop:2584 | Nested compile-time profile; not production viewport authority |
| Optional Dialogue discovery | DialogueDiscoveryLoop:5734 | DIALOGUE_DISCOVERY_DIAGNOSTIC, bounded discovery |
| Overlay/pacing | composition runtime own workers | Separate presenter ownership, no migration into core worker array |

WorkerLifecycle has capacity 4; StartRequest owns entry/context and ThreadStartThunk contains exceptions. Lifecycle start/join is externally serialized, not a concurrent service API.

### Controlled reset / detach

`ResetAllRuntimeResources` (:734) sets stopping, closes ready/gates/capture → signals/joins workers → **on failed join retains hooks and returns** → clears recovery/exclusion → resets optional probes/zoom/gameplay/dialogue/EXIT/ENTER → RestoreCinematicAspect → clears availability/selection.

`Shutdown` (:6786) calls reset then flushes logger. Source search found no production call site invoking `plugin::Shutdown` beyond its declaration/definition. Controlled teardown is available, not proof of supported dynamic unload.

`NotifyProcessDetach` (:6792): process termination returns without complex cleanup; normal detach closes camera gates/stopping and signals worker event only. Do not add joins, hook resets, owner destruction or Overlay teardown here.

## 5. Shared publication і lifecycle preservation

| Flow | Runtime integration | Facts/contracts that must survive |
|---|---|---|
| Gameplay full writer publication | ReplayManualTransition → ProcessCameraWriter → baseline projection + PublishCameraStateSnapshot:4163 | Canonical Gameplay/Native/Target/Output; neutral/source/zoom qualification; writer/policy/viewport aspects; RuntimeCombine telemetry |
| ENTER | TraceCinematicEnter → Prepare/ProcessAcceptedCinematicEnter → BeginCameraTransitionBoundary → lifecycle freshness → XMM0/ENTER observation | Accepted generation only; baseline numeric value != writer identity; previous boundary carry only on confirmed continuation |
| Cinematic writer | ProcessCameraWriter cinematic dispatch; transaction publication:4255 | Cinematic ownership before Gameplay correction gates; validated association; output/target distinction |
| Dialogue | ProcessDialogueBoundary under dialogueMutex; XMM1/publication adapter | In-cinematic Dialogue does not steal Cinematic ownership; native stream vs transformed output; target safety |
| EXIT/recovery | accepted EXIT transaction → MarkCameraPresentationTransition; recovery transactions in shared writer | Owner capture/retirement; native endpoint validation; independent Dialogue exclusion; no timing/debounce replacement |
| Gameplay after handoff | normal writer transaction and full publication | Baseline Current only after qualification; prior facts retained/historical according to current predicates |
| Settings | ApplyCameraSetting + MarkCameraStateForSetting:4145 | Immediate/deferred enable semantics and epochs; no persistence while holding correction gate |
| Readback | ReadOverlaySemanticSnapshot:5977 | Copy camera snapshot under state lock, current coordinator separately, retained baseline usability, boundary independently, viewport at read |
| Delivery | SnapshotSemanticsChanged → NotifyOverlayCameraStatePublished | Metadata/freshness changes matter even with same FOV; no unconditional per-sample redraw |

Known lifetime remains:
`Gameplay → ENTER → CinematicActive → Dialogue/continuation → EXIT/Recovery → Gameplay`.
Session-boundary invalidation (Main Menu/save-load) remains **OPEN / authoritative event unknown**. Default coordinator Gameplay is not proof of observed Gameplay. Decomposition must not invent a MainMenu owner, timeout or callback-silence invalidation.

### F1–F9 preservation checklist

Ці IDs тут означають **Overlay/shared snapshot audit**; не змішувати їх з окремими camera module remediation findings.

| Finding | Contract to preserve | Current source/test anchor |
|---|---|---|
| F1 | Production baseline usability vs shared qualified Gameplay fact; safe pass-through evidence allowed, source/zoom qualification preserved | GameplayBaselineStore; camera_state_publication.hpp; camera_writer_transaction_harness |
| F2 | Same-source pass-through cannot leave previous target Current | camera_state_publication.hpp canonical target lifetime; writer/projection regressions |
| F3–F4 | ENTER evidence distinct from live roles; accepted boundary operands/generation survive valid continuation and become last validated after EXIT | fov_observation.hpp transition boundary helpers; TraceCinematicEnter; camera_integration_harness |
| F5 | Change-driven wake includes metadata/freshness | runtime.cpp:4106–4313; SnapshotSemanticsChanged |
| F6 | transactionOwner != last full-publication presentation != current coordinator | transaction publication + semantic readback + camera_state_view |
| F7 | Cinematic preview is presentation preview, not native store confirmation | camera_integration.cpp vs ResolveCinematicAspect/ApplyCinematicAspectStore |
| F8 | Summary live-only writer aspects vs explicitly historical details | canonical facts + writer observation freshness in projections |
| F9 | Canonical four roles; production baseline remains separate where required | camera_state_snapshot/publication + stores; no resurrected legacy FOV copies |

Rapid generations with an empty superseded generation must remain supported. Diagnostic `EnterWriterCaptureState` and shared `CameraTransitionBoundaryObservation` have different owners and semantics; moving the former must not clear the latter.

## 6. Concurrency, lock order і exception boundaries

- **Current camera path:** CameraCorrectionGate → dialogueMutex where needed → cameraStateMutex for publication/readback. Dialogue lock is usually released before shared publication. Do not hold cameraStateMutex then enter a helper that acquires dialogueMutex.
- **Settings:** camera-setting transaction owns gate; pending and epochs commit there. Overlay/hotkeys invoke the same typed API. Config persistence is separate.
- **Hotkey config:** ReadRuntimeSettings and mutations take correction gate then hotkeyConfigMutex. Preserve this order; do not call SettingsApi.Snapshot while holding that mutex.
- **Logging:** Log captures arguments under a held gate and queues emission for release; LogImmediate writes directly. Moving formatting/emission into another TU must use the **same gate instance** and preserve tuple lifetime/copy behavior.
- **Gate destructor:** unlock → clear thread-local currentGate → move/drain deferred actions, each caught. Scope exit is behaviorally significant, including on early return.
- **Diagnostics mutexes:** cinematic dedup, ENTER capture, baseline writer publication, telemetry, notifications remain distinct. Do not treat these mutexes as camera transaction serialization.
- **Ordinary non-atomic state:** callback owner-thread assumptions remain documented; references/views are not thread-safety by themselves.
- **Zoom:** TraceZoomTransition currently does not acquire CameraCorrectionGate; it touches shared qualification counters/atomic observations and freshness. This is a pre-existing affinity/synchronization boundary to preserve and characterize, not permission to add a new lock.
- **ABI catches:** DLL init/worker thunk, SafeMidHookEntry, RuntimeSettingsApi forwarding, logging/notifications contain C++ exceptions at their respective boundaries. Keep catches at the same boundary.
- **Lifetime:** RuntimeState intentionally allocated without registered destructor. Some existing auxiliary globals do have static storage. Moving them into a destructor-owned context would change termination behavior; owner relocation requires explicit comparison rather than silent cleanup.

## 7. Findings для decomposition

| ID | Classification | Evidence / cause | Possible manifestation if split carelessly | Direction |
|---|---|---|---|---|
| D1 | Architectural risk | RuntimeState private TU storage + aliases across all integration paths | Per-TU duplicate owner/store/atomics; callbacks and UI disagree | One resident owner; internal-only reference views/accessor |
| D2 | Architectural risk | Shared gate and deferred Log:855–884; ProcessCameraWriter held-gate check | Self-deadlock if extraction adds nested Acquire; skipped transaction if gate lost; logging happens under hot lock | Document which entry acquires vs requires gate; preserve guard extent |
| D3 | Evidence gap | test.cmd source lists omit runtime.cpp; semantic_snapshot_harness uses stub ReadFacts | Green domain tests while real hook adapters/publication/readback are miswired | Link/test moved adapters through bounded engine/context seam; real production/diagnostic build |
| D4 | Architectural risk | Hook component commit:3484 and InitializeTransactional rollback order | Partial active cinematic setup; observable wrong gate timing | Keep resolve/install/commit/rollback one orchestrated flow |
| D5 | Architectural risk | RuntimeState allocation:355; WorkerLifecycle; detach:6792 | Destructors under loader lock or hook reset before workers stop | Preserve process residency, failed-join retention and minimal detach |
| D6 | Intentional distinct state | BaselineStore, live CameraState, Transition Evidence:223–236 | Numeric merging weakens source/qualification or evidence lifetime | Preserve authoritative owner for each semantic entity |
| D7 | Redundant/legacy candidate, not removal approval | Local symbol search finds definition-only ApplyHorPlusGameplay:5343, CinematicAspectOverrideEnabled:1984, ArmPostCinematicDialogueExclusion:711, ArmAtomicExitHandoff:1556; ClearCinematicRecoveryAuxiliaryState:1509 and CompleteCinematicRecovery:1532 likewise not called | Extracting dormant implementations as active APIs falsely expands behavior; deleting dependencies during move changes research profiles | Mark candidate reachability; no deletion in structural batch; establish all-profile call closure separately |
| D8 | Architectural risk | Numerous conditional storage/code blocks and macro-dependent HookSet layout | Profile compiles differently, duplicate TLS/dedup, ODR/layout mismatch if defines differ across TUs | Same defines for all plugin TUs; production + supported diagnostic validation |
| D9 | Intentional behavior / evidence boundary | Zoom hooks always attempted under exact hash:6515; runtime updates zoom facts regardless of diagnostics | Moving entire zoom block to diagnostic-only TU breaks baseline safety when diagnostics OFF | Treat zoom observation as production; gate only diagnostic emission |
| D10 | Architectural risk | EnterWriterCapture diagnostic storage:2005 vs shared boundary:224; continuedOwnership at ENTER | Generation replacement resets accepted Evidence or presents capture as live FOV | Preserve independent stores and event order |
| D11 | Intentional behavior / evidence gap | Startup settings API published before all capability installation; stopping checked by semantic read; core-ready notification on overlayStarted | Refactor accidentally changes readiness/failure semantics or hides configuration/readback | Preserve current ordering; failure-injection characterization before moving startup |
| D12 | Evidence gap / existing limitation | Zoom direct callbacks without SafeMidHookEntry/gate; callback affinity assumed | New TU split changes affinity/locks and appears to fix/alter behavior without evidence | Record boundary; do not repair concurrency/ABI behavior in this refactor |
| D13 | Intentional behavior | Overlay worker outside plugin WorkerLifecycle; Shutdown has no found call site | New teardown ownership wrongly implies dynamic unload support | Keep facade contract; presenter lifetime remains own responsibility |
| D14 | Documentation drift | TESTING_AND_RESEARCH default examples differ from feature_config.hpp | Plan preserves wrong defaults/mode lists | Current source first; update architecture when approved structure actually implemented, not historical records now |

D7 — bounded lexical/local call search, not linker-proven whole-profile dead-code proof. `ReplayManualTransitionOriginal` is **not** equivalent to an unused production helper: it is under POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST and called by TryDeferredGameplayReplay when the atomicity variant is not selected. Keep it with its experimental profile. ApplyHorPlusGameplayDiagnostic executes its own diagnostic path; it does not call the definition-only ApplyHorPlusGameplay.

Жодний finding не є дозволом змінювати current correction semantics. Окремий correctness/crash аудит, якщо потрібен, має власний scope та runtime evidence.

## 8. Обґрунтовані extraction boundaries — proposal

Назви нижче — proposal, не створені файли. Не потрібно досягати довільної кількості рядків; межі можна об'єднати, якщо interfaces стануть непропорційними.

| Proposed area | What moves | Explicit dependencies | What remains outside |
|---|---|---|---|
| runtime.cpp facade/owner | Existing resident owner creation, public initialization/shutdown facade, high-level orchestration | Internal references/context, worker/install/settings setup | Algorithms залишаються у domain modules |
| runtime_internal.hpp (plugin-private) | Definition of existing owner if cross-TU access needed; minimal internal declarations/view factories | Existing state/types; no new global copies | Not public plugin::RuntimeState; no overlay/domain consumer access |
| runtime_settings.cpp | Apply/read/persist callbacks, SelectGameplayMode, binding mapping, effective locale sync | Gate, effective settings, existing pending state, config repository, freshness callback | No camera calculations; no state owned by UI |
| runtime_camera_publication.cpp | Mark* freshness, Publish* adapters, ReadOverlaySemanticSnapshot | References to snapshot/qualification/epochs/stores + existing viewport resolver + wake bridge | Publication helpers remain camera authority; no new projections |
| runtime_camera_callbacks.cpp | ReplayManualTransition + engine memory/register adapter + existing recovery interaction | Existing transaction views, gate, memory operations, publication/diagnostic observers | Do not split writer's recovery decision into independent Gameplay/Cinematic owners |
| runtime_cinematic_callbacks.cpp | Aspect/ENTER/EXIT adapters, auto/native aspect resolution | Lifecycle view, stores, publication/boundary, logger/diagnostic sink | Cinematic policies/association types remain existing |
| runtime_dialogue_callbacks.cpp | Dialogue adapter, reset glue and source trace calls | Dialogue view, same gate, source reads, publication/diagnostic observers | Existing dialogue state/transform policies |
| runtime_hook_installation.cpp | Native resolvers/validators, hook creation/commit/rollback; zoom install integration | Executable span, hook storage, stable callback thunks, logger | No correction policy; startup chooses existing component order |
| runtime_diagnostics.cpp | Established capture/dedup/formatting/emission and optional probe adapters | Value snapshots/results, same deferred logger; profile-specific hooks where required | No new source authority; zoom qualification not hidden here |
| runtime_hotkeys.cpp (optional separate extraction) | Hotkey loop and notification bridge, rebind guards | SettingsApi, config persist, worker stop, existing queue | Presenter rendering/input stays overlay-owned |

Prefer existing typed views over passing the whole RuntimeState into domain modules. At the native callback boundary, a private owner lookup can form these views. No generic callback bus, service registry, universal camera context or new coordinator is justified by this audit.

Small internal headers may define logger/native-boundary templates if required by C++ instantiation; keep templates shared without copying static objects. Do not replace reference views with captured copies of settings/epochs.

Moving private state across TUs changes the literal translation-unit privacy statement in ARCHITECTURE.md. An approved implementation must update it to **plugin-internal ownership** while retaining the ownership/lifetime guarantee. That documentation change is not performed in this audit.

## 9. Suggested incremental order and acceptance gates

Each stage below requires an approved durable implementation plan; this report does not begin Batch 1.

| Stage | Bounded action | Main risk | Minimum evidence for that future stage |
|---|---|---|---|
| 0 | Freeze audited baseline + write approved move map, state/lock contracts, source lists | Dirty baseline or stale line map | Current hashes/diff, explicit files/non-goals/validation |
| 1 | Establish internal binding/reference access while retaining current owner construction/lifetime | Duplicate singleton, include cycles, ODR/macro layout | Builds in ordinary + supported diagnostic profiles; same storage used by callbacks/readers |
| 2 | Extract settings/persistence/readback, keeping publication invocation order | API callback lifetime, mutex ordering, partial failure | Settings/config/gate harnesses + actual moved reader/handler tests |
| 3 | Extract publication/freshness glue and observation-only diagnostics separately | Changed qualification/wake/order; log under gate | Canonical roles, same-source pass-through, metadata wake, Evidence/preview regressions; deferred-log guard behavior |
| 4 | Extract Dialogue boundary adapter | XMM6/XMM1 and native target read; reset lock | Production-equivalent adapter tests; Dialogue transaction + disabled/in-cinematic/rearm paths |
| 5 | Extract Cinematic adapters with install/commit coherence | RIP skip, ENTER generation, rejected EXIT, carry | Native-context adapter checks; accepted/rejected boundaries; rapid generation chain; Cinematic transaction harness |
| 6 | Extract full writer adapter, preserving shared recovery flow | Branch ordering, register mutation, same-sample readback/publication | Actual adapter register/engine checks; RuntimeCombine/AspectRecalculation/HorPlus/OFF + recovery/source matrix |
| 7 | Extract installers/startup/hotkey workers only if still worthwhile | Partial activation, ready/gate timing, stop/join/failure | Resolver fixtures, failure-injection installation, workers, optional Overlay OFF, persistence |
| Final | Whole-diff equivalence review, full suite, ordinary + diagnostic build; agreed runtime acceptance later | Offline evidence overstated as runtime equivalence | Named artifacts/hashes, exact scenarios; no camera behavior claim solely from build PASS |

Alternative: installers can move alongside their callbacks if a split otherwise requires overly wide interfaces. Keep orchestration and commit ordering explicit. Do not automatically extract all proposed files.

For every move: no thresholds/defaults/enums/memory ordering changes; no simplification of branches; no remove-unused cleanup; no new polling/events; diff must separate move from any separately approved functional repair. Do not require byte-identical ASI (translation-unit/linker changes legitimately change binary layout); compare contracts/results and retain identity-specific runtime evidence.

## 10. Validation coverage and gaps (static registry review)

| Existing check | Establishes when run | Does not establish |
|---|---|---|
| camera_correction_gate_harness | Gate/deferred work contract | Actual callbacks use same moved owner/gate |
| camera_setting_transaction_harness | Camera setting commits/epochs/lifetime requests | Real ApplyRuntimeSetting binding and full persistence switch |
| camera_writer_transaction_harness | Shared dispatch, source/recovery, RuntimeCombine, canonical publication interactions | SafetyHookContext adapter/register application in runtime.cpp |
| cinematic_lifecycle_transaction_harness | Accepted/rejected transaction behavior, generation/selection/continuation/recovery | Actual ENTER/EXIT/aspect callback ordering/RIP/readers |
| dialogue_boundary_transaction_harness | Production transaction and serialized disable paths | Real native +0x2C reader/XMM6→XMM1 glue |
| camera_state_snapshot/fov_observation/gameplay_baseline harnesses | Fact validity/publication and store contracts | Integration owner placement/actual adapter publication |
| camera_integration_harness | Projection semantics including Evidence and canonical roles | Production ReadOverlaySemanticSnapshot; pixel/render equivalence |
| semantic_snapshot_harness | Published API routes fixture facts | Actual production reader (uses stub ReadFacts) |
| optional_overlay_startup_harness | Config startup gate/helper behavior | Real initialization path/hooks/worker side effects; also tests legacy helper not currently called by InitializeThread |
| worker_lifecycle_harness | Start/stop/join primitive | Loader/core/presenter complete teardown or dynamic unload |
| runtime_evidence_replay_harness | Selected recorded inputs and domain behaviors | Live game memory/hooks/thread affinity |
| Ordinary production build | All production TUs link under current defines | Runtime lifecycle/graphics/no-crash proof |
| Supported diagnostic build | Additional selected compile-time paths link | Dormant research macro combinations not selected by build.cmd |

CMake is **not** the production build system: no repository CMake file was found. `build.cmd:61–65` explicitly lists compilation sources/libs/resources. `tools/build/build-diagnostic.cmd` delegates with CAMERA_TWEAKS_BUILD_PROFILE=diagnostic; `tools/build/build-overlay-settings.cmd` is an ordinary build alias. The production source list must be updated for every extracted .cpp.

Source gates include OVERLAY_COMBINED, OVERLAY_STARTUP_JOURNAL/TIMELINE, HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC, HORPLUS_FOV_STATE_DIAGNOSTIC, DIALOGUE_BOUNDARY_DIAGNOSTIC, DIALOGUE_DISCOVERY_DIAGNOSTIC, DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC, MATCHGAMEPLAY_DIAGNOSTIC, POST_EXIT_* probes, POST_CINEMATIC_GAMEPLAY_REPLAY_* tests, GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER, COMBINED_GAMEPLAY_DIAGNOSTIC, FOV_SETTINGS_TRACE_DIAGNOSTIC and derived POST_EXIT_TRACE_STATE. Not all are current supported profiles. Existing build.cmd must be inspected before selecting any research macro validation; do not resurrect retired experiments as a production gate.

## 11. Source/consumer map

- `src/plugin/runtime.hpp`: public DLL/runtime/readback/locale/viewport facade.
- `src/plugin/runtime_settings.hpp`: typed settings mutation/read/persist callback bundle, semantic snapshot, notification/rebind API.
- `src/plugin/{camera_correction_gate,camera_writer_transaction,cinematic_lifecycle_transaction,dialogue_boundary_transaction,optional_overlay_startup}.hpp`: reusable integration transactions/gates.
- `src/plugin/{worker_lifecycle,feature_status}.*`, `src/hooks/hook_set.*`: existing lifetime/status/hook containers.
- `src/plugin/dll_entry.cpp`: SetModuleHandle/InitializeThread/NotifyProcessDetach consumer.
- `src/overlay/renderer_runtime.cpp`: settings Apply/Snapshot/Persist, semantic readback, notifications/rebind.
- `src/overlay/composition_runtime.cpp`: settings readback, overlay startup/core-ready/wake, own presenter/worker resources.
- `src/overlay/game_language_reader.cpp`: GetRuntimeSettingsSnapshot/SetDetectedOverlayLocale.
- `src/overlay/{camera_integration,camera_state_view,feature_presentation}.*`: semantic projections consumed by UI.
- `src/config/{feature_config,config_repository,config_template}.*`: defaults/parsing/compatibility/persistence/template; no formatter cleanup in decomposition.
- `src/gameplay/*`, `src/cinematics/*`, `src/dialogue/*`: existing domain policies and stores, retained.
- `src/camera/{fov_observation,gameplay_baseline,gameplay_aspect_restoration,camera_state_snapshot}.*`, `camera_state_publication.hpp`, `presentation_state.*`, `fov_semantics.hpp`: distinct observations/baseline/publication/coordinator/calculation contracts.
- `src/diagnostics/{diagnostic_runtime,performance_telemetry,cinematic_continuation_trace,dialogue_source_provenance}.*`, `camera_writer_identity.hpp`, `camera_log_probe.hpp`, `cinematic_baseline_provenance.hpp`, `cinematic_lifecycle_context.hpp`, startup headers: observational support.
- `src/hooks/signature_scanner.*`, `instruction_validator.*`, `signatures/*`; `src/platform/win32/{memory,viewport,sha256,window}.*`: native resolver/read/write/window inputs.
- `build.cmd`, `test.cmd`, `tools/build/build-diagnostic.cmd`, `tools/build/build-overlay-settings.cmd`: current build/test wiring.

## 12. Завершення audit scope

Read-only dependency/state/hooks/lifetime/build/test карта підготовлена. Production source changes — **none**. Build/test/runtime — **not exercised in this audit**. Proposed decomposition — **pending user approval of a durable implementation plan**. Closed functional findings не перевідкриті; known session event gap залишений як evidence boundary.


## Appendix A. Function/template index

Definitions (including compile-time-gated ones); line references are for the audited source hash. Overloaded ResolveCinematicAspect is listed twice. Forward declarations at :100, :102, :561, :684, :731, :1565, :1633, :3547 and :3641 do not create additional owners/functions. This is a source inventory, not a proof of runtime reachability.

| Responsibility / source interval | Function definitions |
|---|---|
| Owner access, workers, Dialogue reset, restoration (357–854) | `GetCinematicContinuationTrace`:357; `WaitForWorkerStop`:563; `StartWorker`:568; `StopWorkers`:578; `CreateWorkerStopEvent`:587; `GetDialogueSourceProvenanceTrace`:592; `ReadDialogueWriterSnapshot`:602; `DialogueSourceStageName`:609; `EmitDialogueSourceEvent`:621; `RecordDialogueSourceCallback`:644; `RecordDialogueRecoverySource`:662; `RecordDialogueSourceEnd`:673; `GetDialogueBoundaryStateView`:686; `ResetDialogueRuntimeState`:697; `ResetPostCinematicDialogueExclusion`:705; `ArmPostCinematicDialogueExclusion`:711; `ObservePostCinematicDialogueRecovery`:718; `ResetAllRuntimeResources`:734; `IsValidAspect`:822; `TraceGameplayAspectRestorationState`:827; `UpdateGameplayAspectRestorationState`:839; `InvalidateGameplayAspectRestorationState`:847 |
| Deferred logging і persistent cinematic/Dialogue diagnostics (855–1380) | `LogImmediate`:855; `Log`:869; `OutputCameraLogProbeDebug`:889; `FlushCameraLogProbe`:910; `DeferCameraLogProbeFlush`:938; `CinematicDiagnosticActionName`:997; `LogCompletedCinematicContinuationTrace`:1013; `FlushCinematicContinuationTrace`:1054; `InitialEstablishmentEndReasonName`:1063; `DiagnosticFovChanged`:1078; `LogCinematicDiagnosticEnter`:1085; `LogCinematicDiagnosticAspectStore`:1106; `LogCinematicDiagnosticAssociationTransition`:1139; `LogCinematicDiagnosticWriterState`:1161; `LogCinematicDiagnosticExit`:1243; `GetCinematicBaselineWriterPublicationState`:1269; `ReadCinematicBaselineWriterPublication`:1277; `CaptureCinematicBaselineWriterPublication`:1285; `CinematicBaselineOriginName`:1306; `LogCinematicBaselineProvenance`:1318; `LogCinematicBaselineSelection`:1365 |
| Native wrapper, recovery adapters, optional post-EXIT probes (1381–1978) | `SafeMidHookEntry`:1381; `NowSteadyNs`:1392; `CancelDeferredGameplayReplay`:1398; `ArmDeferredGameplayReplay`:1416; `ResetAtomicExitHandoff`:1429; `CameraWriterRecoveryStateView`:1439; `GetCameraWriterCinematicStateView`:1455; `GetCameraWriterGameplayStateView`:1467; `GetCinematicLifecycleStateView`:1486; `ClearCinematicRecoveryAuxiliaryState`:1509; `CompleteCinematicRecovery`:1532; `LogForeignRecoverySource`:1545; `ArmAtomicExitHandoff`:1556; `ApplyDelayedAspectDiagnostic`:1568; `LogGameplayFovSourceChange`:1597; `LogPostExitWriterObservation`:1657; `ObservePostExitRawWriterEntry`:1741; `ObservePostExitProducerStore`:1785; `InstallPostExitProducerStoreTrace`:1812; `ObservePostExitFovWriteOwner`:1839; `InstallPostExitFovWriteOwnerTrace`:1871; `ObservePostExitFovConsumer`:1898; `InstallPostExitFovConsumerTrace`:1934; `LogGameplayFovChange`:1962 |
| Memory/value utilities і ENTER writer diagnostic capture (1979–2214) | `ComputeSha256`:1979; `CinematicAspectOverrideEnabled`:1984; `IsWritable`:1989; `IsValidFovValue`:1994; `SafeRead`:2000; `LogEnterWriterObservation`:2046; `BeginEnterWriterCapture`:2064; `CompleteEnterWriterCapture`:2106; `RecordEnterWriterObservation`:2165 |
| Viewport/engine adapter, cinematic aspect callback, mode diagnostics (2215–2463) | `ReadClientViewportAspect`:2215; `ResolveAutoAspect`:2220; `ReadAutoAspectForCinematicStoreLog`:2231; `ResolveAutoAspectForCinematicStore`:2238; `ResolveCinematicAspect`:2245; `ResolveCinematicAspect`:2251; `WriteAspectAndFlags`:2257; `WriteAspectOnly`:2263; `GetCameraWriterEngineOperations`:2268; `ApplyPendingGameplayModeTransition`:2288; `ApplyGameplayAspectFixAtomic`:2302; `ApplyCinematicAspectStore`:2329; `LogCameraModeChange`:2391 |
| Conditional experimental workers/replay (2464–2872) | `WriteFlagsOnly`:2465; `OneShotTriggerLoop`:2472; `LogTransitionSnapshot`:2535; `LogResolutionChange`:2552; `ResolutionMonitorLoop`:2584; `LogCombinedOutputPost`:2591; `ReplayManualTransitionOriginal`:2616; `TryDeferredGameplayReplay`:2771 |
| Cinematic native resolver/install/restore (2873–3035) | `ValidateEnterBoundary`:2873; `ValidateExitBoundary`:2891; `ValidateIndexedExitBoundary`:2910; `ResolveCinematicAspectStore`:2934; `ResolveCinematicFovCallsites`:2952; `InstallCinematicAspect`:3011; `RestoreCinematicAspect`:3029 |
| ENTER/EXIT callbacks і component transaction (3036–3510) | `TraceCinematicEnter`:3036; `TraceCinematicExit`:3251; `InstallCinematicAspectComponent`:3458; `InstallCinematicFovComponent`:3468; `CommitCinematicComponents`:3484; `RollbackCinematicAspectComponent`:3497; `RollbackCinematicFovComponent`:3502 |
| Dialogue diagnostics, callback, installation (3511–3819) | `LogDialogueDiagnosticReturn`:3519; `LogDialogueDiagnosticSummary`:3535; `ArmDialogueRecoveryEndpointTrace`:3570; `ObserveDialogueRecoveryEndpointTrace`:3579; `TraceDialogueBoundary`:3646; `InstallDialogueBoundary`:3793 |
| Writer diagnostics (3820–4105) | `ReadHorPlusCinematicWriterState`:3843; `HorPlusCinematicWriterSnapshotChanged`:3871; `LogHorPlusCinematicWriterTrace`:3893; `TraceHorPlusCinematicWriterBefore`:3944; `TraceHorPlusCinematicWriterAfter`:3958; `FovTraceValueChanged`:3995; `TraceGameplayFovSourceSample`:4003; `TraceHorPlusRecoverySample`:4075 |
| Shared publication/freshness (4106–4314) | `MarkCameraStateFreshness`:4106; `MarkCameraPresentationTransition`:4125; `MarkCameraStateForSetting`:4145; `PublishCameraStateSnapshot`:4163; `PublishCameraFovTransaction`:4255 |
| Conditional HorPlus telemetry (4315–4498) | `HorPlusTelemetryOwnerName`:4325; `ResolveHorPlusTelemetryOwner`:4337; `TraceHorPlusFovState`:4355 |
| Production camera writer adapter (4499–5181) | `ReplayManualTransition`:4499 |
| Zoom observation/resolver і legacy Gameplay adapter (5182–5457) | `ZoomTransitionFloatChanged`:5204; `TraceZoomTransition`:5211; `TraceZoomIn`:5279; `TraceZoomOut`:5280; `InstallZoomTransitionObservation`:5282; `VerifyExecutableAndInstruction`:5327; `ApplyHorPlusGameplay`:5343 |
| Conditional observer/discovery (5458–5757) | `ApplyHorPlusGameplayDiagnostic`:5474; `ObservePostExitDiagnosticWriter`:5533; `InstallPostExitGameplayObserverDiagnostic`:5546; `DialogueDiscoveryNowNs`:5590; `LogDialogueDiscoveryEnd`:5596; `LogDialogueDiscoveryChanges`:5608; `TraceDialogueDiscovery`:5664; `DialogueDiscoveryLoop`:5734 |
| Settings/readback/persistence (5758–6255) | `SelectGameplayMode`:5758; `RuntimeHotkeyBinding`:5817; `ApplyGameplayModeSetting`:5835; `ApplyRuntimeSetting`:5840; `ReadRuntimeSettings`:5948; `ReadOverlaySemanticSnapshot`:5977; `PersistHotkeyActionValue`:6159; `PersistRuntimeSetting`:6166 |
| Hotkey worker і core initialization (6256–6668) | `HotkeyLoop`:6256; `Initialize`:6406 |
| Public facade/notification/locale/lifetime (6669–6800) | `PublishOverlayNotification`:6671; `DrainOverlayNotifications`:6690; `SetHotkeyRebindCaptureActive`:6703; `MarkHotkeyRebindKeyConsumed`:6708; `GetRuntimeSettingsApi`:6713; `SetDetectedOverlayLocale`:6718; `GetRuntimeSettingsSnapshot`:6740; `GetOverlaySemanticSnapshot`:6748; `GetRuntimeViewportAspect`:6753; `SetModuleHandle`:6758; `InitializeThread`:6763; `Shutdown`:6786; `NotifyProcessDetach`:6792 |

### Local data shapes

| Shape | Line | Role |
|---|---:|---|
| RuntimeState | 156 | Resident integration owner |
| CombinedOutputSnapshot / ResolutionSnapshot | 791 / 805 | Conditional combined diagnostic snapshots |
| CinematicDiagnosticWriterState | 963 | Persistent association/decision diagnostic dedup |
| CinematicBaselineWriterPublicationState | 1263 | Lazy diagnostic baseline publication provenance |
| PostExitTraceSnapshot | 1636 | Conditional TLS trace |
| EnterWriterRawObservation / EnterWriterCaptureState | 2005 / 2020 | Pre/post ENTER diagnostic capture |
| DialogueRecoveryEndpointTrace | 3553 | Conditional diagnostic recovery endpoint trace |
| HorPlusCinematicWriterTraceSnapshot | 3821 | Conditional cinematic writer trace |
| GameplayFovSourceTraceSnapshot | 3974 | Gameplay source diagnostic dedup |
| HorPlusTelemetryOwner enum | 4316 | Diagnostic owner classification |
| RuntimeCombineDiagnosticSnapshot (function-local) | 4617 | RuntimeCombine diagnostic dedup |
| ZoomTransitionSnapshot | 5186 | Direction-specific zoom edge comparison |
| HorPlusDiagnosticSnapshot | 5460 | Conditional observer result |
| DialogueDiscoverySnapshot | 5565 | Conditional bounded discovery |

## Appendix B. RuntimeState member index

All 159 named fields found in RuntimeState (:158–349), including compile-time-gated fields. Initializers, declared types, atomics/mutexes and exact gates remain authoritative in source. Groups below follow declaration order; they do not propose new storage owners. Aliases :362–558 bind these same fields and existing HookSet members.

| Source interval | Members (declaration line) |
|---|---|
| 158–168 | `module`:158; `executable`:159; `workers`:160; `fovWriteAddress`:161; `hooks`:162; `logger`:163; `config`:164; `hotkeyConfigMutex`:165; `cameraCorrectionGate`:168 |
| 169–182 | `gameplaySettingsEpoch`:169; `cinematicsSettingsEpoch`:170; `dialogueSettingsEpoch`:171; `activeCinematicSettingsEpoch`:172; `activeDialogueSettingsEpoch`:173; `hotkeyRebindCaptureActive`:174; `hotkeyRebindConsumedKey`:175; `gameplayRecoveryObservationAvailable`:176; `runtimeInitializationComplete`:177; `overlayStartupSucceeded`:178; `runtimeGameplayEnabled`:179; `gameplayEnableApplyPending`:180; `gameplayDisableRestorePending`:181; `gameplayEnabledTransitionDeferralLogged`:182 |
| 183–205 | `runtimeGameplayMode`:183; `runtimeCinematicsEnabled`:185; `activeCinematicsEnabled`:186; `gameplayModeTransitionPending`:187; `gameplayModeTransitionDeferralLogged`:188; `gameplayModeTransitionTarget`:189; `runtimeCinematicPolicy`:191; `runtimeCinematicFovMode`:193; `activeCinematicAspectPolicy`:195; `activeCinematicFovMode`:197; `cinematicSelectionGeneration`:199; `cinematicSelectionValid`:200; `cinematicExitIndexed`:201; `cinematicExitLogProbeGeneration`:202; `cameraWriterHookGate`:203; `cinematicHookGate`:204; `stopping`:205 |
| 206–222 | `runtimeDialoguePolicy`:206; `runtimeDialogueEnabled`:208; `runtimeOverlayLocale`:209; `runtimeOverlayLocaleAuto`:211; `runtimeOverlayAutoLocaleSynchronized`:212; `runtimeOverlayFontSize`:213; `cinematicLifecycleObservationAvailable`:214; `cinematicAspectComponentAvailable`:215; `dialogueBoundaryHookAvailable`:216; `dialogueNonNativeCapabilityAvailable`:217; `configPath`:218; `state`:219; `lastCameraMode`:220; `transitionTraceSequence`:221; `coordinator`:222 |
| 223–236 | `fovObservationStore`:223; `cameraTransitionBoundary`:224; `gameplayBaselineStore`:225; `cinematicWriterAssociation`:226; `cinematicRecoveryOwner`:227; `cinematicContinuationTrace`:228; `gameplayAspectRestorationStore`:230; `cameraStateMutex`:231; `cameraStateSnapshot`:232; `cameraStateSnapshotValid`:233; `neutralGameplayFovCandidate`:234; `neutralGameplayFovSamples`:235; `neutralGameplayFovCandidateSource`:236 |
| 237–263 | `cinematicFovApplied`:237; `cinematicTransformedFov`:238; `exitCinematicTransformedFov`:239; `matchGameplayEnterNativeFov`:240; `matchGameplayPreEnterNativeFov`:241; `matchGameplayPreEnterHorPlusFov`:242; `matchGameplayPreEnterAspect`:243; `gameplayBaselineContinuationSource`:247; `cinematicContinuationPending`:248; `cinematicContinuationCarryPending`:249; `cinematicContinuationSource`:250; `cinematicContinuationExitNativeTarget`:251; `cinematicContinuationPreviousTarget`:253; `cinematicContinuationInterpolationActive`:255; `cinematicContinuationStartFov`:256; `matchGameplayPreEnterPairValid`:258; `cinematicEnterAspect`:260; `exitTargetFov`:262; `postCinematicDialogueExclusion`:263 |
| 264–294 | `postExitTraceArmed`:265; `postExitTraceWriterCount`:266; `postExitTraceStartNs`:267; `postExitTraceSequence`:268; `deferredGameplayReplayPending`:271; `deferredGameplaySource`:272; `deferredGameplayStableSamples`:273; `deferredGameplayLastFov`:274; `deferredGameplayStableSinceNs`:275; `atomicExitHandoffPending`:277; `atomicExitHandoffSource`:278; `atomicExitHandoffPreviousFov`:279; `delayedAspectDiagnosticApplied`:281; `horPlusDiagnosticWriterCallbacks`:284; `horPlusDiagnosticApplications`:285; `horPlusDiagnosticChanges`:286; `dialogueDiscoveryActive`:289; `dialogueDiscoveryStartNs`:290; `dialogueDiscoverySamples`:291; `dialogueDiscoveryChanges`:292; `dialogueDiscoveryResetGeneration`:293 |
| 295–311 | `zoomInHook`:295; `zoomOutHook`:296; `zoomInHits`:297; `zoomOutHits`:298; `zoomLoggedEdges`:299; `snapshotZoomDirection`:300; `snapshotZoomSequence`:301; `snapshotZoomSource`:302; `snapshotZoomPrimary`:303; `snapshotZoomSecondary`:304; `snapshotZoomActive`:305; `snapshotZoomObservationValid`:306; `snapshotZoomHold`:307; `snapshotEventSequence`:308; `snapshotDialogueSource`:309; `snapshotDialogueTarget`:310; `snapshotDialogueTargetValid`:311 |
| 312–330 | `horPlusFovTelemetryMutex`:312; `horPlusFovTelemetryValid`:313; `horPlusGameplayCacheValid`:314; `stableObservationSource`:315; `stableObservationFov`:316; `stableObservationSamples`:317; `stableObservationReported`:318; `horPlusTelemetryNativeFov`:319; `horPlusTelemetryAspect`:320; `horPlusTelemetryResult`:321; `horPlusTelemetryFlags`:322; `horPlusTelemetryMode`:323; `horPlusTelemetryOwner`:324; `postExitWriteOwnerHitCount`:326; `postExitProducerStoreHitCount`:329 |
| 331–340 | `cinematicAspectStore`:331; `cinematicOriginalImmediate`:332; `cinematicAspectPatched`:333; `lastAutoRestoreSource`:334; `lastGameplayCameraSource`:335; `lastGameplayCameraFov`:336; `lastLoggedFovSource`:337; `lastLoggedWriterInputFov`:338; `lastLoggedCameraWorldFov`:339; `lastLoggedCameraFirstPersonFov`:340 |
| 341–349 | `dialogueMutex`:341; `dialoguePhase`:342; `activeDialoguePolicy`:343; `dialogueCorrectionActivation`:344; `dialogueBaseline`:345; `dialoguePrevious`:346; `dialogueExitIncomingStart`:347; `dialogueCandidate`:348; `dialogueRecoveryRearm`:349 |

