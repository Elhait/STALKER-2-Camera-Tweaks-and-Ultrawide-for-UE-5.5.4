# Native FOV в оверлеї: джерело та межі provenance

Read-only перевірка поточного working tree, 2026-11-13. Нових tests, builds,
hooks, runtime/debugger sessions немає. UI, Gameplay HorPlus і U2 не змінювалися.
User observation: Native FOV в оверлеї змінюється разом із HorPlus; чи це
відбувалося при toggle у стабільному Gameplay без transitions, не уточнено.
Не трактувати зміну показника як доказ double transform або baseline corruption.

## 1. Від UI до input boundary

| Шар | Конкретне місце | Фактичне джерело |
| --- | --- | --- |
| Overlay row | src/overlay/presenter/renderer_runtime.cpp, DrawRuntimeTelemetryRow("Native FOV",...) | integration.nativeFov з validity/freshness |
| Детальний camera state view | src/overlay/presentation/camera_state_view.cpp, DrawField("Native FOV",...) | Та сама integration.nativeFov, поруч provenance/freshness |
| Projection | src/overlay/presentation/camera_integration.cpp, ProjectCameraIntegration | snapshot.cameraState.evidence.nativeFov.value |
| Full Gameplay snapshot | src/camera/camera_state_publication.hpp, PublishCameraStateSnapshotLocked | observation.inputFov.value, не resultFov |
| Transaction publication | там само, PublishCameraFovTransactionLocked | transaction.nativeInput; output записується в інший fact |
| Runtime transaction publisher | src/plugin/runtime.cpp, PublishCameraFovTransaction | Caller передає nativeInput та output окремо; provenance nativeInput=NativeNumericObservation |
| Writer caller | ReplayManualTransition, ProcessCameraWriter | writerInputXmm0 захоплений на вході до поточної correction; result.inputFov=input.incomingFov |
| Dialogue caller | TraceDialogueBoundary | incoming=XMM6 до поточної setter mutation; publication може змінити той самий nativeFov fact |

Це поточний input sample, НЕ persisted native setting і НЕ immutable Gameplay
reference. Overlay не читає camera+0x230 напряму і не обчислює HorPlus повторно.
Нормальна native interpolation, ADS або інший producer можуть змінювати sample.
Raw-before-this-invocation не означає globally untouched-by-mod.

## 2. До/після mutation та неоднорідність UI source

Writer: ReplayManualTransition захоплює writerInputXmm0 перед ProcessCameraWriter.
ProcessGameplayHorPlusWriter публікує inputFov і transformed resultFov окремо.
PublishCameraFovTransaction використовує writerTransaction.inputFov, а не його
outputFov. Snapshot публікується як обчислена transaction; сам XMM0 commit у
відповідній downstream branch і POST-store — окремі факти.

Dialogue: TraceDialogueBoundary бере incoming із XMM6, може змінювати XMM1,
потім публікує incoming як nativeInput та setter argument як output. Наступний
writer може прочитати вже змінене native camera field. Таким чином, до зміни
поточним writer input уже може бути результатом іншої mod boundary.

Один рядок оверлею може послідовно відображати:

`setter raw XMM6 → writer raw XMM0 → setter raw XMM6 ...`

Ці boundaries мають різну семантику й source token: Dialogue RSI є blend
context, writer RSI — camera source. Поточний snapshot зберігає source/sequence
для fact, але Native FOV row не є незмінним camera-setting display. Видима зміна
числа може бути зміною boundary, навіть без нового camera FOV reference.

Підтверджена localization неузгодженість: locales/uk.json tooltip.native_fov
говорить лише про Gameplay writer; locales/en.json описує applicable camera
transaction input. Український tooltip вужчий за реальне джерело. Це не причина
native camera mutation; UI в цьому аудиті не виправляли.

## 3. Чи corrected output повертається в native input

### Звичайний Gameplay HorPlus

ProcessGameplayHorPlusWriter у src/plugin/camera_writer_transaction.hpp бере
input.incomingFov і result.outputFov окремо; CameraFovObservation позначає
перший NativeRegisterInput, другий ModTransformResult. GameplayBaselineStore
копіює observation.inputFov у nativeFov, resultFov у horPlusFov.
Прямого присвоєння result.outputFov у observation.inputFov або baseline.nativeFov
на цьому шляху не знайдено. Writer mutation готує XMM0 для output+0x30.
RuntimeCombine окремо синхронізує camera aspect/flags; його побічні наслідки
для native recomputation не встановлюються простим порівнянням цих двох чисел.

Отже, «HorPlus toggle у стабільному Gameplay змінює той самий pre-correction
input при незмінній камері/settings» залишається не перевіреним цим аудитом.
Наявні logs не є ізольованим ON/OFF експериментом такого типу.

### Cinematic ENTER та Dialogue setter

ProcessAcceptedCinematicEnter обчислює transformed ENTER operand; runtime передає
його native consumer. Successful initializer захоплює поточне camera+0x230 як S,
а отриманий target як T. Native blend може інтерполювати до transformed target.
Тому writer raw input згодом закономірно може дорівнювати corrected cinematic
target. Це підтверджений механічний шлях, не арифметична помилка оверлею.

Так само Dialogue mutation XMM1 через native setter записує camera+0x230;
derived setter має optical side effects. Наступний writer/initializer може
зчитати вже змінене значення. Native engine storage не має FOV-space tag.
Наявні native data-flow докази: POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md.

У runtime-182050.log writer у Cinematic2 спочатку має input70/output126.87,
після release retention пропускає blend samples70.1061…126.87. Raw input досягає
126.87 без повторного застосування writer HorPlus у цих samples. Називати це
«native reference змінився з90 на126.87» неправильно: це sample native pipeline,
до якого вже передали transformed ENTER target.

## 4. Baseline: розділення сховищ є, domain proof обмежений

GameplayBaselineStore::IsEligibleObservation вимагає CameraWriter boundary,
NativeRegisterInput/input space Native, result semantics, source/aspect/sequence
та gameplayEligible. Однак NativeRegisterInput тут задається місцем захоплення.
Він не є незалежним доказом, що upstream setter/initializer ніколи не змінював
числовий domain. Gameplay coordinator eligibility також не доводить neutral
Gameplay без Dialogue/blend.

Отже, corrected upstream sample механічно МОЖЕ бути допущений як native baseline
при виконанні Gameplay gates. Це конкретне обмеження provenance, не доказ того,
що воно спричинило обидва visual defects у наданому запуску. Camera-state stable
Gameplay reference має окремі snapshot gates; це не та сама authority, що
GameplayBaselineStore. Не зливати ці два механізми у висновку.

## 5. Зв'язок із U2 та overlay authority

U2 TryMapPostExitProviderDialogueSample не читає overlay nativeFov/snapshot.
Він бере activation S/T/continuityStart з native component/receiver fields,
sample з paired setter evidence та input.incomingFov поточного writer.
GameplayBaselineStore використовується для eligibility/source qualification;
у чинній формулі U2 endpoint обчислюється від activation.nativeTarget, не від
числа overlay Native FOV і не від baseline.nativeFov.

Спільний upstream camera pipeline може впливати і на overlay, і на U2.
Тому correlation можлива, але зміна UI fact сама не змінює production FOV.
continuityStart із camera+0x230 залишається native field observation, не доказом
corrected/rendered start. Виявлений domain ambiguity підтримує необхідність
розділення цих фактів у transition design; він не обґрунтовує новий mapping сам.

## 6. Coverage наступної consumption-перевірки

| Потрібний факт | Existing boundary / доступність | Межа доказу |
| --- | --- | --- |
| Setter argument до CALL | TraceDialogueBoundary, XMM6/XMM1, RCX receiver, component | Не POST-store і не rendered view |
| Setter result camera+0x230, vtable | Existing diagnostic TracePostExitBlendSetterReturn із paired TLS invocation; safe receiver reads | Конкретний return, не майбутній writer/view |
| Derived optical+0xC7C | Для validated receiver shape можна доповнити snapshot на тому самому return point | Без shape/vtable validation читання/інтерпретація неприйнятні; новий hook не потрібен для самого поля |
| Writer input/output і фактичний output+0x30 | Existing PRE/POST trace | Capture budget/Diagnostics gates; application, не consumption |
| Active view FOV/aspect/projection branch | Не спостерігається цими hooks | Правильні setter/POST не закривають rendered-path gap |
| Stable Gameplay HorPlus ON/OFF causality | У наданих captures немає ізольованого контрольного епізоду | Не приписувати будь-яку зміну Native FOV HorPlus лише за часом |

Consumption capture має зіставляти саме active receiver/view і invocation order
на початку двох стрибків. Timestamp або latest writer недостатні; ще один лог
endpoint108.941/126.87 не усуне невідоме. Цей аудит не запускає capture й не
дозволяє автоматично додавати interception points до renderer.

## 7. Висновок та статус

Підтверджено: UI показує поточний transaction input; Native FOV може законно
змінюватися. Він не гарантує globally unmodified native reference. У системі є
конкретні upstream mutation→native camera→later input шляхи. Direct Gameplay
HorPlus input/output assignment mix-up не знайдено. Окрема неточність uk tooltip
підтверджена; UI залишений без змін.

Причинність user-observed зміни при stable Gameplay toggle, фактичне забруднення
baseline у момент двох дефектів та active-view consumption залишаються UNKNOWN.
Повторне створення нового FOV store або зміна label не встановить ці факти.

## Уточнення після повного invariant audit

[NATIVE_FOV_IMMUTABILITY_INVARIANT_AUDIT_2026-11-13.md](NATIVE_FOV_IMMUTABILITY_INVARIANT_AUDIT_2026-11-13.md)
знайшов прямий saved-runtime counterexample baseline admission: до останнього
Batch1 Dialogue output60.9227 був записаний як Gameplay baseline native60.9227
(before-admission-repair.log, рядки583,596–597). Загальне формулювання цього
попереднього звіту про відсутність runtime proof уточнюється для того конкретного
запуску. Причина двох поточних visual defects і stable Gameplay toggle behavior
від цього не стають доведеними.
