# U2 — здійсненність інтеграції наявних boundaries

Read-only аудит production source та наявних native evidence, 2026-11-13.
Scope: три прогалини transition adapters і локальна setter dependency check.
Не виконувалися production edits, build/tests, game/debugger, новий disassembly
або capture. Висновки native data-flow успадковано з явно названих evidence;
цей аудит не додає нового runtime proof.

Design: [U2_TRANSITION_ADAPTER_DESIGN_2026-11-13.md](../plans/U2_TRANSITION_ADAPTER_DESIGN_2026-11-13.md).
Endpoints: [U2_POLICY_ENDPOINTS_2026-11-13.md](U2_POLICY_ENDPOINTS_2026-11-13.md).
Обидві visual acceptance лишаються OPEN.

## 1. Результат за трьома прогалинами

| Факт | Що доступно зараз | Мінімальна зміна контракту на існуючих boundaries | Що це НЕ встановлює |
| --- | --- | --- | --- |
| Qualified corrected writer start | ProcessCameraWriter result; фактична XMM0 mutation; source/output identity; generation; PRE/POST invocation trace | Виділити production application record на commit XMM0 та валідованому POST-store, незалежний від Diagnostics/budget; scoped identity/epoch/sequence, без global last-FOV authority | Active view consumption та відсутність intervening native writes |
| Current ENTER activation | Successful-init hook, original/relocated caller attribution, component/receiver/start/target; Dialogue pre-setter; diagnostic post-setter pairing | Production activation epoch для ENTER, передача qualified setter invocation/result у writer; чітке retirement на rearm, foreign sample, source/settings/generation change | Setter→writer lineage не доведена одним float/thread match; camera cut і native branch/domain не випливають лише з epoch |
| Steady Dialogue policy | Native target70 producer, paired transition samples, mod Dialogue Candidate/Active/Exiting/RearmPending | На цих seams немає підтвердженої незалежної Dialogue lifecycle-події; продовження policy потребує додатково встановленої authority або явного обмеження scope до activation | Native terminal sample/completion не доводить, що наступний writer усе ще Dialogue |

Висновок: перші два пункти мають наявні точки інтеграції без нової адреси hook,
але не закриті самим фактом їх існування. Третій не закривається поточним
classifier чи продовженням lifespan record. Повні adapters у цьому contract
ще не готові до production implementation.

## 2. Corrected application: конкретні межі

src/plugin/runtime.cpp:

- BeginCameraWriterPostStoreTrace зберігає source(RSI), output(RBX), thread,
  generation та invocation. Воно обмежене активною cinematic generation,
  Diagnostics і sample budget. Запис не є загальним Gameplay/Dialogue journal.
- CompleteCameraWriterPostStoreTrace записує розрахований corrected argument,
  а не доказ store. Фактична XMM0 mutation відбувається далі у відповідних
  branches ReplayManualTransition.
- ObserveCameraWriterPostStore зіставляє TLS frame з thread/source/output,
  SafeRead output+0x30, source aspect/flags. Це application evidence тільки
  для конкретного invocation; PRE/POST mismatch не створює валідний record.
- NEXT log прямо позначає intervening_writer=not_excluded. Наявний один TLS
  pending frame може бути replaced; його не можна представляти як довільно
  reentrant production journal без explicit rejection/stack contract.
- POST-store hook встановлюється в initialization під diagnostics gate.
  Відокремити production capability від logging — зміна lifecycle contract
  існуючого hook, хоча нова native interception address не потрібна.

Production record може довести «значення X застосоване цим writer invocation
до output Y». Передавання його як C0 потребує встановленої execution relation
до початку нової activation, freshness і того самого output domain/aspect.
Порівняння source pointer або найближчий timestamp недостатні. Зміна output
destination не автоматично конфлікт, але old RBX не автоматично active view.

Projection consumer, manager override і post-writer helper лишаються downstream
за цією boundary. Новий application record не розв'язує перший visual defect
сам по собі й не перетворюється на rendered-start evidence.

## 3. ENTER activation: доступні seams та ліміт

src/plugin/runtime.cpp:

- InstallCinematicFovComponent/CommitCinematicComponents вже створюють і
  вмикають validated successful-init hook; capability availability зараз потрібна
  production provider handoff. TracePostExitBlendStartCapture виконує його
  production частину під cameraCorrectionGate до diagnostic-capture early return.
- Caller evidence читається через ReadBlendCallerEvidence з original/relocated
  ENTER/EXIT та provider returns. Воно встановлює caller activation, не семантику
  всіх наступних setter/writer samples.
- Наявний production ObserveArmer допускає лише ProviderProducer після EXIT.
  ENTER successful-init зараз не створює production ENTER transition record.
- Далі йдуть diagnosticCaptureActive/trace.Active gates; ObserveStartCapture
  і matched activation epochs у postExitBlendActivationObservation є діагностикою.
  Пряме використання цього cache зробило б поведінку залежною від capture budget.
- TraceDialogueBoundary бачить component=RSI-0x28, receiver=RCX, sample=XMM6,
  setter argument=XMM1 і endpoints із component. Ці факти існують до native CALL.
- TracePostExitBlendSetterReturn та PostExitBlendSetterPairing мають bounded
  TLS LIFO pairing за thread, RSP, component. Return hook встановлюється лише
  за diagnostic validated-layout capability; його pending state і epoch checks
  зараз diagnostic-scoped.

Інтеграційно можливий окремий production epoch та paired application record
на цих самих адресах. Немає доказу, що цього вже досить для causal setter→writer:
між ними можливі інші writes, зокрема documented GetCameraView recalculation.
Збереження «останнього setter sample» не є вирішенням цієї прогалини.

Не всі ENTER виклики доходять successful-init. Для CreationGuardRejected
не виникає activation evidence, хоча accepted cinematic lifecycle й own ENTER
reference лишаються чинними. Мінімальна зміна contract повинна явно зберегти
U1-A fallback для цього випадку й не обіцяти interpolation, якої не спостерігали.
Збіг activation endpoints не замінює validated native branch і FOV-space.

## 4. Steady Dialogue: terminal не lifecycle

ProcessDialogueBoundary у src/plugin/dialogue_boundary_transaction.hpp та
CandidateTracker::Observe у src/dialogue/dialogue_state.cpp приймають lifecycle
за source/target-compatible descent, потім використовують Active/Exiting та
RearmPending. Це mod heuristic. Hook стоїть у generic FOV blend, не на
незалежному Dialogue enter/exit. Repeated-admission suppression виправляє один
false start, але не надає Dialogue authority після завершення native tick.

Native tick може dispatch completion delegate та cleanup provider+0x2CF8;
це описано у POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md. Production hook
на terminal delegate/cleanup відсутній. Навіть його спостереження підтвердило б
завершення blend, а не Dialogue start/continued ownership. Додавати completion
hook лише для steady Dialogue evidence не обґрунтовано.

Наявний historical UE4SS IsInStaticDialog oracle у
../active/DIALOGUE_GROUND_TRUTH_CORRELATION_TASK_PLAN.md — окреме evidence source,
не standalone production dependency мода. Його не можна оголосити доступним
native state на підставі існування того дослідження.

На теперішніх seams можна підтверджувати окремі activation-scoped samples.
Не встановлено, як без незалежної події авторизувати наступні writer samples
після останнього tick та відрізнити stationary Dialogue від іншого camera state
на тому самому receiver. Policy context не повинен жити необмежено «до іншого FOV».

## 5. Setter → native blend → writer: локальна перевірка

Наявний native evidence POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md встановлює:

1. Initializer читає camera+0x230 у S+0x58, target у S+0x54, duration у S+0x50.
2. Tick обчислює eased sample зі збережених endpoints/progress, передає XMM6
   через XMM1 у virtual setter+0x608, а потім порівнює XMM6 із target.
3. Base setter записує XMM1 у camera+0x230. Derived setter також обчислює
   optical field+0xC7C. Derived GetCameraView може переобчислити camera+0x230
   через інший helper перед validated writer.

Із цього випливає обмежений висновок: mutation XMM1 на поточному setter call
не є автоматично mutation збережених S/T/remaining та порівнюваного XMM6.
Проте setter value впливає на camera state, derived optical state і start
наступної initialization. Не доведено, що всі subclasses/споживачі незалежні
від цих полів. Runtime receiver/vtable та фактичний downstream path для обох
візуальних стрибків не встановлені цим аудитом.

Отже, writer-only correction — допустимий design candidate, але НЕ доведена
семантична заміна setter correction. Навіть рівні writer endpoints не гарантують
рівних optical параметрів або однакового rendered view. Literal копіювання
чинної Dialogue формули на writer boundary поки не обґрунтоване.

## 6. Мінімальні контрактні рішення після аудиту

| Рішення | Межа роботи | Умова приймання |
| --- | --- | --- |
| Production application record | Наявні writer commit/POST; scope, validity, failure, lifecycle незалежні від Diagnostics | Не стверджує consumption; rejects replaced/mismatched/stale invocations |
| ENTER activation integration | Наявні initializer/pre-setter/return/writer; epoch/rearm/current invocation | Не успадковує diagnostic authority; causal gap та CreationGuardRejected explicit |
| Destination policy lifetime | Обрати evidence-backed steady lifecycle або явно обмежений activation-only scope | Не обіцяти persistent Reduced/Adaptive/Disabled endpoint при поверненні до plain H(raw70) |
| Mutation boundary | Визначити setter-vs-writer side effects для фактичного receiver | Не оголошувати writer-only equivalent за одними FOV числами |

Не потрібно нового HorPlus audit, підбору tolerance чи переписування recovery.
Поточних фактів вистачає для названих integration decisions, але не для повної
реалізації двох adapters з обіцяною visual correctness.

Якщо знадобиться додатковий runtime evidence, він має усунути конкретне
невідоме: фактичний receiver setter/vtable та downstream active-view FOV/aspect
на початку стрибка. Ще один endpoint capture або terminal blend marker цього
не встановлює. Жодного нового runtime-сеансу цей звіт не запускає/не вимагає.
