# Lifecycle Authority: карта та межі першої міграції

2026-11-17. **M1 COMPLETE. M2 resumed and incomplete.** Corrective parity
is green for the six supported OFF/ON sequences. See current authorization at
the end of this document. Live cutover remains incomplete; M3 Release/runtime
is outside the current implementation batch. No game installation is authorized.

## 1. Вихідні матеріали й зафіксований oracle

Повний [звіт transition study](../reports/LIFECYCLE_EVIDENCE_AND_TRANSITION_MODEL_2026-11-17.md),
[checker results](../evidence/lifecycle-transition-model-2026-11-17/result.json)
та [T01–T36 coverage](../evidence/lifecycle-transition-model-2026-11-17/transition-coverage.md)
доступні окремо від підсумку в чаті. Під час цієї підготовки hashes звірено з
попереднім manifest: усі сім зафіксованих research artifacts збігаються.
Перевірено записані результати: focused BFS 7644 states /125328 edges depth6;
all-family BFS 407/9559 depth3; 7/7 fault variants detected, 8 streams,
10 scenarios. Checker повторно не запускався; це перевірка збережених результатів.

[Oracle R1 freeze](../evidence/lifecycle-transition-model-2026-11-17/oracle-freeze-R1.json)
фіксує hashes моделі та результатів. R1 — незалежний oracle символічних
ownership/provenance/permission переходів У МЕЖАХ звіту. Він не є оракулом
native extraction, числових FOV, ABI чи всіх C++ guards.

Нове уточнення меж R1: `matches()` і `hKey` у model lines44–47,88 перевіряють
component/thread/generation, але не містять окремого arming-event identity.
Тому repeated arming із тією самою tuple не є перевіреною native activation
identity. R1 не змінено, його PASS не поширено на цей випадок.

При розбіжності зафіксувати мінімальну історію, artifact/profile, identity,
oracle expectation та production result; класифікувати причину: implementation,
native binding, oracle defect або out-of-domain. Не оновлювати expected під
production автоматично. Зміна oracle — окрема обґрунтована ревізія з red
counterexample, review та новими hashes; R1 залишається відтворюваним.

## 2. Межа першої міграції

Пропонується перенести **право camera lifecycle-рішення** звичайного ASI,
а не формули, resolver-и, нові engine detectors чи платформну архітектуру.
RuntimeState лишається верхнім runtime owner. Усередині нього один приватний
camera Authority володіє O/D/R/H/P/B/K/G; старі camera fields не залишаються
паралельним writable owner. Назви подій нижче — запропонований API контракт,
не вимога окремого класу для кожної події чи 36 публічних entry points.

Зовнішня межа: `Accept(EventFacts) -> CommittedDecision` і read-only projection.
Виклик під чинним CameraCorrectionGate; один приватний послідовний W і один
commit на actual accepted event. Кілька T-рядків одного Writer/Setter/ENTER
не стають кількома зовнішніми commits. Ранній вихід також має явний результат.

У scope входять primary writer, Cinematic ENTER/EXIT/aspect-store,
ordinary/provider setter, provider armer, recovery/exclusion, settings camera
changes, startup/reset camera permissions, native result observations та
Camera State projection їхніх рішень. `POST_EXIT_BLEND_OBSERVATION` включено:
нині він активний у звичайній production-збірці.

Не змінювати Gameplay RuntimeCombine/AspectRecalculation, GameplayHorPlus,
Cinematic mapping, Dialogue formulas, доступні режими, defaults/INI, FOV
tolerances та animation math. Не додавати hook, таймер expiry, LoadGame reset,
global candidate veto або ще один reconciliation layer. Відмінності lifecycle
поведінки, потрібні чинному контракту, мають окремі red regressions та review.

Graphics/workers/logger/config persistence/resolver/resource lifetime не
переносяться до camera Authority. Platform admission може негайно закрити
callback gate для safety; це не право приймати Gameplay/Dialogue/baseline.
Camera permission revoke оформлюється у серіалізованій lifecycle межі, без
зміни чинних loader-lock/drain/join правил і без нового блокування teardown.

## 3. Карта T01–T36

`W` = приватний робочий стан; `commit` нижче означає зміни, які входять у
єдиний фінальний commit події. Leaf helper може оцінити proposal або змінити
лише переданий приватний W; не публікує самостійний verdict. «Закрити доступ»
— запланована дія, не вже виконана перевірка майбутнього коду.

| ID | Поточна точка | Authority entry / приватна робота | Що входить у commit | Який старий доступ усунути |
| --- | --- | --- | --- | --- |
| T01 | RuntimeState init, runtime.cpp camera defaults/setup | Initialize: effective camera settings/capabilities | Початкові G/K, порожні scoped claims | Прямі init stores camera lifecycle поза Initialize; platform gate лишається platform |
| T02 | Prepare/ProcessAcceptedCinematicEnter | CinematicEnter: validate/admit activation | O, generation, captured K; reject без owner | Mutable CinematicLifecycleStateView у adapter |
| T03 | ENTER selector і BaselineStore read | CinematicEnter: SelectReference з trusted B | Reference origin/reason та target intent | Adapter сам обирає/відновлює B або target |
| T04 | CaptureDialogueCorrectedStartCandidate, EnterActivation::Arm/Capture | CinematicEnter / ProviderArm observation: EvaluateStartWitness | Scoped provenance carry або Unknown | Adapter Arm/Capture, що сам дає activation permissions |
| T05 | ObserveCinematicAspectStore, Association::ObserveAspectStoreTarget | CinematicAspect: EvaluateBinding/conflict | Source relation і scoped permission | Пряме mutating association query у callback |
| T06 | Association ObserveWriter/AdmitWriterOutput | Writer: EvaluateCinematicAdmission | Per-invocation output admission | Writable association зі writer adapter |
| T07 | EnterActivation ObserveWriter/RecordWriterResult | Writer: EvaluatePreparedMapping/progress | P/witness і mapped effect intent | Самостійний activation mutator після writer result |
| T08 | ProcessAcceptedCinematicExit/RecoveryOwner::Capture | CinematicExit: accepted source-bound transfer | O retire, R capture, окремо B/P retain | Adapter capture recovery/coordinator store |
| T09 | Writer-empty EXIT/Clear/RetireForReentry | CinematicExit: EvaluateEmptyActivationCarry | O retire; carry без invented R authority | Reset B/continuity через саму empty generation |
| T10 | Dispatch/IsAcceptedCinematicExit/early return | CinematicExit: reject stale/duplicate facts | NoChange/Rejected | Early return зі прихованим reset або permission |
| T11 | ObserveWriter, RetireRecoveryForWriterRsiChange | Writer: EvaluateSourceRelations | Scoped invalidation/retention; output relation окремо | Pointer change як автоматичний mode/world verdict |
| T12 | ShouldRejectPostRecoveryForeignWriter | Writer: EvaluateForeignScope | PassThrough для sample, незалежний owner retained | Foreign refusal очищає чинний R/H |
| T13 | ProcessHorPlusRecovery, CompleteCameraWriterRecovery | Writer: EvaluateRecoveryProgress/Terminal на W | R terminal; final O/P/B і policy одночасно | Entry RecoveryOwned після completion, окремий post-commit baseline verdict |
| T14 | ClearCameraWriterRecoveryState без owner | Writer: EvaluateOwnerLoss | Scoped R retirement/Unknown; B дія явна | Coordinator=Gameplay як автоматичний admission |
| T15 | AspectRecalculation recovery branch | Writer: спільний terminal reducer, інший effect proposal | Той самий R/O/B terminal contract | Окремий early-return completion owner |
| T16 | Exclusion::Arm/ObserveValidated/Reset | Writer/Exit: EvaluateExclusionDependency | Scoped guard progress/retire | Public mutating Observe/reset, reset із query |
| T17 | ProcessAtomicExitHandoff | Writer: EvaluateHandoffEffect | Pending intent або cancel; effect result окремо | Native aspect/flags write, який сам завершує lifecycle |
| T18 | runtime provider callback/ObserveArmer | ProviderArm: AcceptArmEvidence | Новий scoped arming event, policy latch, supersession | Adapter ObserveArmer/Reset; tuple як episode ID |
| T19 | ObserveSetterSample/reset branches | DialogueSetter: MatchAcceptedArm | H progress або Reject; retirement тільки за evidence | Helper mismatch сам очищає старий H |
| T20 | TakeWriterSample/TryMapPostExitProviderDialogueSample | Writer: ConfirmProviderTrajectory | H confirmation/consumption, P/effect intent | Adapter consuming writer samples до рішення W |
| T21 | CompleteRecovery/ConsumeDialogueAdmission | Writer або DialogueSetter: TransferProviderOwnership | R retire, H consumption, D activation одним event commit | CompleteRecovery після writer commit та окремий повторний admission |
| T22 | BlocksRepeatedAdmission, Record/ClearCorrectedDialogueSetterSample | DialogueSetter/EffectObservation: EvaluateConsumption/Provenance | Epoch retirement або prepared fact на названій boundary | Mutating BlocksRepeatedAdmission query/adapter Record/Clear |
| T23 | ProcessDialogueBoundary/Candidate Begin/BindReceiver | DialogueSetter: EvaluateCandidate | Scoped D hypothesis, не active permission | Відкритий CandidateTracker& у adapter |
| T24 | Candidate Observe/phase/policy stores | DialogueSetter: EvaluateTrajectory | Activate/Cancel/Retain; P/B окремо | Phase/policy writes і implicit native admission від Cancel |
| T25 | RetireStationaryDialogueCandidateOnGameplayWriter | Writer: EvaluateCandidateTerminal | D retire плюс final sample/B permissions | Pre-classification reset adapter, який живе окремо від writer commit |
| T26 | Active/RecoveryRearm Begin/Observe | DialogueSetter або accepted terminal event: EvaluateDialogueTerminal | D/policy release; prepared sample не стає native | Callback-local independent rearm/correction owner |
| T27 | runtime resetDialogueIfActive/context callbacks | Writer: EvaluateContextIncompatibility | Scoped revoke з explicit proof | Function pointer/capture, який викликає ResetDialogueRuntimeState з policy |
| T28 | GameplayBaselineStore::Project/Invalidate; runtime mode callback | Writer/Setting: EvaluateBaselineTrust на final W | Update/Retain/Invalidate + identity/reason | Store повторно інтерпретує lifecycle tag; зовнішні Project/Invalidate |
| T29 | ApplyCameraSetting/SelectGameplayMode/pending helpers | SettingChange та Writer physical boundary | K/pending flags/B trust; physical result окремо | CameraSettingStateView, ApplyGameplayModeSetting як mutable callback |
| T30 | Camera enable/Cinematic/Dialogue policy updates | SettingChange: EvaluateEffectiveSettings | Immediate mutation revoke; activation policy latch | Direct active snapshot/epoch writes, settings callback reset lifecycle |
| T31 | Store/helper callbacks, correlation/publisher | EffectObservation: ValidateCorrelation | Named observation; no owner resurrection | Publisher/trace додає production permissions; planned=applied |
| T32 | Zoom callbacks/neutral publication | ZoomObservation: read-only evidence proposal | Observation; trust лише за окремим accepted rule | Neutral observer сам перемикає owner або є новим baseline owner |
| T33 | ABI invalid/read failure/reentrant/stopping returns | Admission rejection або admitted event abort/finalize | NoChange/ScopedRetirement, no leaked W/effect | Reset state у SafeMidHookEntry/adapter catch; unfinished admitted paths |
| T34 | ResetAllRuntimeResources/init failure/shutdown | Revoke/Reset: camera permissions only | G revoke і clear dependent claims | Platform cleanup напряму вирішує D/R/H/B; Authority керує threads |
| T35 | Historical helper wrappers без production callers | Не entry: видалити або залишити private delegate до одного Accept | Жодного альтернативного commit | Public Arm/Complete/Clear/Apply wrappers, здатні обійти Authority |
| T36 | ReplayManualTransitionOriginal/conditional experiments | Поза першою підтримуваною міграцією | Явний profile exclusion | Заборонити збирати такий mutating profile як migrated release без тієї ж межі |

Карта охоплює normal production cutover цілком, а не лише writer.
T31/T32 observation records не зобов'язані породжувати lifecycle mutation.
Pure queries не стають додатковими accepted events лише заради логування.

## 4. Конкретні точки закриття mutable-доступів

Поточні інтеграційні вузли перевірено в `src/plugin/runtime.cpp`:
RuntimeState:187; GetDialogueBoundaryStateView:1107;
recovery/cinematic/gameplay views:2020/2036/2050/2069;
provider armer:4219; setter match/admission:5333/5401;
writer pre-processing:6966/7031; ProcessCameraWriter і reset callbacks:7070;
provider CompleteRecovery:7140; baseline projection:8078;
mode Invalidate:8427; ApplyCameraSetting:8465; startup camera settings:9114–9127.
Це навігаційні anchors поточного source, не стабільні ABI offsets.

Після cutover adapters отримують immutable event inputs, committed decisions
та read-only projections. Вони не отримують RuntimeState&, writable store,
StateView з references, lambda reset capability чи g_* aliases до camera
lifecycle. Helpers Candidate/RecoveryRearm/Association/Handoff/Activation
стають приватними implementation details Authority. Якщо їхній public method
здатен сам приймати рішення, він не експортується adapter-у навіть через friend.

GameplayBaselineStore може зберегти структурний validation, але Project більше
не видає власний lifecycle verdict. Policy formulas отримують immutable inputs
і повертають числа/proposals; engine effects executor отримує scoped committed
permission, не writable lifecycle. Старі reset/invalidation callbacks з
ProcessCameraWriter та settings view вилучаються з call boundary.

Фізичне сховище можна перемістити, не дублюючи дані. Не залишати «старий»
RuntimeState state плюс новий authoritative mirror. Тести можуть створювати
окремі fixtures; у live runtime одночасно лише один authoritative store.

## 5. Arming event: окрема обов'язкова умова

`component + thread + cinematic generation` — compatibility evidence, не native
episode identity. На кожен accepted arming потрібен новий Authority event
identity; supersession retire-ить залежні permissions попереднього arming навіть
за однакової tuple. Внутрішній event ID означає прийняту нами подію, а не engine
lifetime, world generation чи доказ того, що всі armings перехоплено.

Setter/writer/result мають підтвердити causal relation із цим arming, а не
лише знайти «останній сумісний» запис. Який конкретний native call/observation
зв'язок підтверджує це, потрібно назвати в adapter binding sheet до cutover.
Якщо зв'язок не спостережено — Unresolved для залежної операції; не автоматичне
знищення незалежного owner і не вигаданий Gameplay proof.

Обов'язкові майбутні binding regressions: same tuple + новий arming;
late setter/result старого arming; повторний target/start; supersession між
setter і writer; interleaved source B; repeated admission після consumption.
Це НЕ вже покриті R1 випадки. R1 порівнюється лише після окремо перевіреної
проєкції конкретних events у символічні facts. Розширення oracle domain потребує
окремої ревізії, а не непомітного редагування R1.

## 6. Міграційні batches після окремого погодження

**M0 — review цього плану.** Зафіксувати підтримувані events/profiles,
поведінкові non-goals та disposition двох capability gaps. План не дозволяє
підміняти missing proof поточним coordinator/default або просто звужувати
робочі Gameplay/Dialogue функції, оголошуючи це safety improvement.

**M1 — приватне ядро + offline binding verification.** Реалізувати relation,
event/result types, finalization та доступи в ізольованих tests; live callbacks
ще не переключати й проміжний ASI не випускати. Використати існуючі harness
entry points, не створювати новий test framework. Зіставити supported events з R1,
а native fields з guards окремими tests/review. Записати accepted proof boundary,
identity/dependencies/expiry для кожного конкретного adapter input. Цей batch
може завершитися корисним ядром без дозволу підключення до гри.

**M2 — один control-path cutover.** Перенести всі normal-production mutation
paths із карти в один owner, включно з callback reset, settings/startup та
provider code під macro. Вилучити старі mutable views/aliases. Підготовчі commits
можуть бути окремими, але не випускати partially migrated runtime, де старий
Dialogue/setter або settings ще приймає незалежні рішення. Не запускати дві
active authorities як shadow compare.

**M3 — access/failure verification та release validation.** Перевірити
production entry sequences, bypass controls, effect failures, Diagnostics OFF/ON
і регресії FOV формул/режимів; повний чинний suite лише в узгодженому ресурсному
вікні з process-tree cleanup. Після всіх gates — ordinary Release ASI та посилання
користувачу, без копіювання в гру. Цільовий runtime перевіряє native binding та
візуальні переходи; offline PASS цього не замінює.

## 7. Що означає «старі незалежні рішення усунені»

1. Для кожного T01–T36 є verified destination: event/internal helper/observation/
   inactive/excluded. Немає fallback «старий компонент вирішує як раніше».
2. Compile-time negative access checks: adapter/policy/publisher не може
   отримати mutable lifecycle reference, reset callback або викликати store
   mutation напряму. Test-only backdoor не доступний production adapter-у.
3. Source/call-site audit охоплює `.store`, Reset/Arm/Begin/Observe/Project/
   Invalidate/Consume та передачу writable refs/callbacks, не лише назви старих
   views. Кожен дозволений mutator досяжний лише всередині accepted W/commit.
   Текстовий rg або перейменування самі по собі цього не доводять.
4. Перевірено, що прийнятий Writer/Setter/ENTER/EXIT/settings event має один
   фінальний coherent decision, включно з усіма конкретними ранніми виходами.
   Неприйнятий callback не змінює claims; partial native effect не відкочує
   committed state вигаданим snapshot і не стає Applied.
5. Послідовності completion→immediate setter, completion→provider→writer,
   Cine1→Dialogue→Cine2, empty generation, component rearm, source replacement
   та ordinary Dialogue завершуються без repair event і чужого baseline.
6. Read-only Camera State/diagnostics беруть committed facts; не відтворюють
   рішення самі. Diagnostics OFF не міняє permissions/output; readback залишається
   observation, а не обов'язковим prerequisite корекції.
7. Усі наявні режими, формули, policy latch і settings compatibility збережено
   або конкретну поведінкову зміну окремо погоджено. Немає мовчазного fallback,
   який прибрав GameplayHorPlus чи зробив відомий сценарій native pass-through.

## 8. Capabilities та go/no-go

| Невстановлена capability | Дозволене планування | Заборонена підміна / cutover gate |
| --- | --- | --- |
| Незалежний Gameplay proof нового source без B | M1 може приймати його як явно типізований input і тестувати Unknown | Нова адреса, flags/FOV або default Gameplay не створюють proof. До live cutover потрібен конкретний підтримуваний bootstrap path або окремо погоджена межа поведінки |
| Dialogue END після зникнення setter | Source-scoped coexistence, accepted Cinematic transfer, відомий native recovery completion | Blend completion, callback silence чи timer не є Dialogue END. Позитивні supported terminal paths і невідомі випадки називаються окремо |
| Episode correlation при component reuse | Accepted arming-event identity та перевірений native causal join | Сам component/thread/generation або останній сумісний запис недостатні; R1 цього binding не доводить |

Go для M1 не означає Go для M2/Release. Не потрібен доказ усіх Unreal
переходів, але неможна вмикати символічний `nativeProof=true` у підтримуваному
adapter без названого реального evidence. Якщо native binding змінює очікувану
доступність функції, це окрема поведінкова дискусія, не автоматичне рішення
під час міграції. Нові hooks цим планом не запропоновані й не дозволені.

## 9. M1 progress and acceptance

Збережені oracle/result hashes та вихідний source identity manifest звірено;
R1 не змінюється. До M1 acceptance належать: приватний C++ reducer без
production callback imports; event/result contracts і атомарне finalize;
focused existing-harness integration; regression coverage для arming reuse,
stale evidence, source scoping, settings revocation, effect failure та
completion/handoff sequences; R1 projection record; native binding sheet з
доказами і явними unresolved межами. Символічний доказ у тесті не є native
binding. Усі зміни залишаються поза live runtime. Після цих критеріїв окремо
перевірити повний diff і зафіксувати M1 результат; це не дає дозволу на M2.

**Результат M1 (2026-11-17):** виконано ізольоване ядро та offline bindings;
45 C++ assertions PASS, focused CTest 1/1 PASS, canonical registry audit PASS
(60 targets / 63 invocations / 0 missing inputs). R1 hashes незмінні. Звіт:
[`LIFECYCLE_AUTHORITY_M1_ISOLATED_CORE_2026-11-17.md`](../reports/LIFECYCLE_AUTHORITY_M1_ISOLATED_CORE_2026-11-17.md).
Повний suite ігра/ASI не запускалися. M2 залишається NO-GO до окремого дозволу
та розв'язання native-binding gates у §8.

Перед фіксацією результату перевірено повний task-relevant diff: Authority
header лишається ізольованим від `runtime.cpp` і production source registry;
додані лише його harness, існуюча test registry/CTest інтеграція, offline
evidence та M1 report. Інші попередньо змінені файли робочої копії не включалися
до цього етапу.

## 10. M2 readiness: observer-only replay (2026-11-17)

Після окремого дозволу виконано isolated shadow replay без live adapter:
`lifecycle_authority_m1_harness --shadow-replay` зіставляє факти generation 5,
bounded provider helper та repair contract PID7372. Результати й межі:
[`LIFECYCLE_AUTHORITY_SHADOW_REPLAY_2026-11-17.md`](../reports/LIFECYCLE_AUTHORITY_SHADOW_REPLAY_2026-11-17.md).

Provider handoff для generation 5 і same-source recovery completion проходять
у тіньовій моделі; це не потребує вигаданого native episode ID для саме цих
зафіксованих порядків. Інший receiver без окремого `gameplayRoleProven` лишається
Unknown у Authority, тоді як чинна класифікація повертає `GameplayEligible`.
Цю різницю не приховувати й не заповнювати coordinator-derived доказом.
Підтвердженого універсального Dialogue END немає; terminal rule лишається
source-scoped та event-bound.

Default M1 invocation лишився на 45 assertions PASS; R1 oracle незмінний.
Shadow replay виявив і виправив лише в ізольованому reducer відсутній перехід
`ProviderArmed → scoped Dialogue Candidate`; окремий replay має 29 assertions
PASS. Test registry лишився 60 targets / 63 invocations.
Production callbacks не підключалися; `runtime.cpp`, ASI і game folder не
змінювалися. **M2 cutover залишається NO-GO** до вирішення new-receiver
Gameplay binding або окремого погодження його обмеженої поведінки.

## 11. Isolated M1 hardening after independent review

The user authorized the bounded hardening plan
`LIFECYCLE_AUTHORITY_M1_HARDENING_2026-11-17.md`. Its result is recorded in
`../reports/LIFECYCLE_AUTHORITY_M1_HARDENING_2026-11-17.md`.
For this isolated stage, the earlier proof-only admission gate is superseded by
explicit Unknown/PolicyAdmitted/NativeProven semantics. Policy admission is an
Authority decision over validated source-bound inputs and accepted lifecycle,
not a fabricated nativeProof or an external old-coordinator verdict.
The production binding contract and live availability remain to be reviewed;
this change does not authorize M2/M3 or any behavioral fallback in the game.

## 13. M1.6 composite Writer preparation

The user's attached instruction authorized isolated composite Writer and
execution/observation separation, without runtime wiring. Implementation and
focused results are recorded in
`../reports/LIFECYCLE_AUTHORITY_M16_COMPOSITE_WRITER_2026-11-17.md`.
The new Writer adapter target is Accept(WriterEvent), not the legacy per-step
GameplaySample/PrepareEffect sequence. Its single visible lifecycle commit and
local single-use PRE capability are independent of optional readback capacity.
95 default assertions (including 29 M1.6) and 31 shadow assertions pass.
Full concrete production transaction differential parity remains the next gate;
this does not authorize M2/M3 or imply native callback equivalence.


## Scenario naming and process identity

The active regression is named SameSourceRecoveryCompletionBeforeNextCinematic.
Historical PID7372 references identify archived evidence only. No production
branch may select this behavior using a fixed PID, receiver address or generation.
Fixture identities describe inputs; matching identity relations are the contract.
Current PID use for logs, process-local controls and window ownership is separate
from camera lifecycle decisions.

## Current authorization and M2 entry after corrective parity

The user authorized repairing the four concrete differential classes and
proceeding directly to M2 when the same six OFF/ON sequences turn green. This
supersedes historical awaiting-approval/NO-GO wording for starting M2; do not
request the same permission again. M3/ASI/game installation is not included.

The corrective pass is green: zero divergences (139 OFF / 215 ON comparisons),
111 isolated assertions, 31 shadow assertions, production default PASS, CTest
2/2 PASS. See the current differential report and its preserved red/green
evidence. R1 and runtime.cpp remain unchanged.

M2 implementation started with the bound WriterAdapterPort, single local PRE
permission lifetime, canonical policy binding and structural negative access
checks. Live callbacks are not yet switched. §§2–8 still define complete-cutover
acceptance: migrate physical aspect/flags/pending transitions and all lifecycle
mutators, projections/settings/reset and named observations before wiring the
new owner. Green scalar FOV/ownership parity is a completed gate, not proof of
the unfinished complete native effect migration. Preserve all existing modes;
do not leave a partially migrated live runtime with two independent owners.

### M2 implementation work order

1. Move Gameplay physical state (restoration, replay, pending enable/mode) into
   Authority's private working state. Run the existing production physical
   helpers as private delegates; engine operations only read/write named native
   fields and report explicit results. Preserve RuntimeCombine readback before
   HorPlus. Cover failure/defer/retry and all three existing Gameplay modes.
2. Move complete Cinematic activation/recovery and provider/ordinary Dialogue
   transitions under the same composite event boundary. Retain single-use FOV
   argument permissions and optional observation separation.
3. Replace runtime mutable views/aliases with captured facts and read-only
   committed results; transfer all normal-profile mutations together. Keep
   platform hook admission, worker/graphics lifetime and logger ownership out.
4. Validate focused production/native adapter and parity checks before the
   wider non-graphics suite. No Release/installation or graphics run in this
   batch. Review all migrated mutators and early exits before cutover acceptance.

Expected integration files are Authority core/policy/ports, private physical
delegates, runtime.cpp, the existing transaction/native-adapter harnesses and
their registry/dependencies. No formulas, user-facing modes, R1 or native hooks
are added/changed. Evidence is recorded under lifecycle-authority-m2-2026-11-17.

M2 progress: ordinary Gameplay physical helpers/pending state, Cinematic
activation progress, typed receiver-unbound native selection/binding/EXIT and
argument finalization are implemented and exercised offline. Current results
and precise remaining acceptance items are in
../reports/LIFECYCLE_AUTHORITY_M2_PROGRESS_2026-11-17.md. All three new fixture
groups run in the existing production transaction harness default invocation.
Live callbacks, old mutable-access removal and full native adapter migration
are still incomplete. This is progress in M2, not M2 completion or a new gate
requiring the user to repeat approval.

The user explicitly resumed M2 after the saved checkpoint. The resumed batch
adds cross-feature Writer parity and same-source physical recovery, including
the AspectRecalculation single-write handoff. The live migration and remaining
provider/recovery binding checks are open. See the resumed section of the M2
progress report for current evidence and scope.

### M2 production cutover complete — 2026-11-18

The ordinary ASI uses Authority for Writer, Dialogue and provider, Cinematic,
settings, shutdown and publication. The previous mutable paths are confined
to characterization fixtures. Shipping build, native ABI checks, focused
CTest and non-graphics canonical regressions passed. The evidence and
superseded mutation paths are recorded in
../reports/LIFECYCLE_AUTHORITY_M2_PROGRESS_2026-11-17.md.
M3 remains the separate in-game and release stage.
