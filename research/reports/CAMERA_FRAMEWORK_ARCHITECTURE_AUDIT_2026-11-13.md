# Camera Framework — архітектура, ownership і Native FOV

**Стан на 2026-11-13.** Read-only аудит наявного working tree, включно з
незакоміченими production-змінами. Production source, формули, Git index і ASI
не змінювалися; нові tests/build/game/debugger не запускалися. Runtime evidence
взято зі збережених captures і не переноситься автоматично на інші білди.

## 1. Фактична архітектура

**CONFIRMED.** Runtime збирає налаштування, hooks і state в `RuntimeState`
([runtime.cpp:185](../../src/plugin/runtime.cpp#L185)). Основний coordinator
має лише `Gameplay`, `CinematicActive`, `CinematicExiting`
([presentation_state.hpp:7](../../src/camera/presentation_state.hpp#L7)).
Dialogue — окремий state machine в Gameplay coordinator, а не четвертий стан
координатора. `CameraCorrectionGate` серіалізує callbacks і commits налаштувань
мода ([camera_correction_gate.hpp:13](../../src/plugin/camera_correction_gate.hpp#L13));
він не серіалізує всі native writes між callbacks.

```text
game ENTER ──> Cinematic selection / R / C ──> native consumer
native blend ──> Dialogue boundary / XMM1 ──> camera+0x230
camera writer / XMM0 ──> coordinator dispatch ──> output+0x30
         ├── Gameplay policy ──> observation / GameplayBaselineStore
         ├── Cinematic association / retention / continuation
         └── CinematicExiting recovery / provider handoff
transaction publication ──> CameraStateSnapshot ──> overlay projection
```

Стрілки показують спостережувані boundaries; вони не доводять, що один setter
без intervening writes є producer кожного writer sample або що output+0x30
використано active view. Різні native camera/view paths можуть перетинатися.

## 2. Компоненти та ownership

| Компонент | Власність і гарантія | Межа / повторне використання |
| --- | --- | --- |
| `CameraCorrectionGate` | Послідовність mod callback/settings transactions | Не спостерігає native writes між callbacks; вже придатний для серіалізації, новий manager не потрібен |
| `CameraFovObservationStore` | Останнє boundary observation із sequence і tags ([fov_observation.hpp:118](../../src/camera/fov_observation.hpp#L118)) | Не незмінний original FOV і не consumer authority; можна розширювати metadata за умови origin evidence |
| `GameplayBaselineStore` | Останній eligible Gameplay writer pair native/result/source/aspect ([gameplay_baseline.cpp:29](../../src/camera/gameplay_baseline.cpp#L29)) | Це окремий production reference, не UI snapshot. Наявного domain gate недостатньо |
| `CameraStateSnapshot` | Публікація фактів із validity/freshness та overlay ([camera_state_publication.hpp:445](../../src/camera/camera_state_publication.hpp#L445)) | Різні writers/setter публікують facts; snapshot — observational, не authority для корекції |
| `CinematicWriterAssociation` | Generation, source/aspect-target, per-writer output admission, R/C ([cinematic_fov.hpp:85](../../src/cinematics/cinematic_fov.hpp#L85)) | Output може змінюватися; identity/sequence не доводять original domain |
| `CinematicRecoveryOwner` | Captured source/generation для EXIT/recovery ([cinematic_fov.hpp:139](../../src/cinematics/cinematic_fov.hpp#L139)) | Не native blend owner; last output — historical evidence |
| Dialogue Candidate/PolicySnapshot/Rearm/Exclusion | Mod lifecycle та вибрана Dialogue policy ([dialogue_state.hpp:11](../../src/dialogue/dialogue_state.hpp#L11)) | Candidate — classifier generic blend, не підтверджений semantic Dialogue ENTER |
| `PostExitProviderHandoff` | Provider activation, receiver, generation, settings, setter/writer sample matching ([post_exit_provider_handoff.hpp:76](../../src/dialogue/post_exit_provider_handoff.hpp#L76)) | Перевіряє pre-setter argument і writer sample; не доводить відсутність native writes між ними |
| `CameraWriterInput`/result | Per-invocation inputs і рішення ([camera_writer_transaction.hpp:303](../../src/plugin/camera_writer_transaction.hpp#L303)) | Не містить окремого upstream-origin proof; природний integration surface для кваліфікованого evidence |
| `GameplayAspectRestorationStore`, zoom observation | Відновлення aspect; zoom transition observations | Не game-origin FOV authority і не повна модель ADS/zoom |

## 3. Native → corrected → applied

Gameplay `ReplayManualTransition` захоплює XMM0 до поточної mutation;
`ProcessGameplayHorPlusWriter` обчислює output з input і settings/aspect
([camera_writer_transaction.hpp:1022](../../src/plugin/camera_writer_transaction.hpp#L1022)).
В observation input і result фізично окремі. RuntimeCombine підтверджує
viewport aspect перед HorPlus. Цей дизайн залишається динамічним; не вимагає
класифікувати ADS, zoom або причину зміни FOV.

**CONFIRMED.** Writer publisher безумовно позначає `result.inputFov` як
`NativeRegisterInput` ([camera_writer_transaction.hpp:1172](../../src/plugin/camera_writer_transaction.hpp#L1172));
baseline перевіряє цей tag і записує observation.inputFov
([gameplay_baseline.cpp:62](../../src/camera/gameplay_baseline.cpp#L62)). Tag
позначає boundary, але не доводить, що upstream input не змінював мод.

Cinematic ENTER захоплює authored operand до mod mutation, зберігає його як
`R` та обчислює `C` ([cinematic_lifecycle_transaction.hpp:160](../../src/plugin/cinematic_lifecycle_transaction.hpp#L160)).
Native consumer отримує C і може породити blend, який поверне mod-influenced
sample на writer input. Dialogue boundary може змінити setter XMM1; native
setter записує camera field. U2 змінює writer output, зберігаючи S/T і output
окремими параметрами. Жоден із цих фактів не робить raw register автоматично
незалежним game-origin reference.

`Native FOV` в overlay зараз є поточним transaction input, який можуть
публікувати Dialogue та writer; `Current Output` окремий. Це не гарантує
оригінального game-origin reference. Український tooltip описує лише Gameplay
writer, хоча production publisher охоплює також Dialogue.

## 4. Lifecycle та transitions

| Межа | Власник і дані | Завершення / ризик |
| --- | --- | --- |
| Gameplay → Cinematic1 | Accepted ENTER створює generation, власний R, C і optional witness; writer association вимагає source/target та output admission | Witness не визначає R. Coordinator ≠ game-owned startup event |
| Cinematic1 → EXIT | `ProcessAcceptedCinematicExit` захоплює exit request і recovery owner, invalidates cinematic generation ([cinematic_lifecycle_transaction.hpp:367](../../src/plugin/cinematic_lifecycle_transaction.hpp#L367)) | Coordinator `CinematicExiting`; pending target не гарантує Gameplay resume |
| EXIT → Dialogue candidate | Exclusion блокує Dialogue під час recovery; U2 може обробити provider activation | Provider target70 і receiver не доводять semantic Dialogue ownership |
| U2 endpoint → Gameplay | Recovery очищає owner/exclusion; completed provider record пригнічує повторний admission конкретної activation | Native blend completion і mod recovery completion різні події; steady Dialogue authority не доведена |
| Dialogue → Cinematic2 | Новий ENTER бере authored R90, не baseline70; per-invocation source/output guards зберігаються | Initial retention завершується на `authored_baseline_departure`, далі sample може пройти raw ([cinematic_fov.cpp:171](../../src/cinematics/cinematic_fov.cpp#L171)) |
| EXIT2 → Gameplay | Recovery класифікує source/flags/endpoint та може інтерполювати | Числовий interval не доводить native domain; cancellation/foreign source потребують fail-closed |

**CONFIRMED runtime:** перед Batch1 `70.0005 → Dialogue Reduced 60.9227 →
writer input60.9227 → baseline native60.9227` записано в
[before-admission-repair.log:583](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L583)
і [рядку597](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L597).
Batch1 усунув конкретний repeated AcceptedStart у наступному capture, але
користувач повідомив візуальний стрибок на обох межах. Другий capture зафіксував
writer `126.87 → 70.1061` на ENTER2. Візуальні дефекти лишаються OPEN.

## 5. Hooks, diagnostics та застосовність повторного використання

| Boundary | Установка / lifecycle | Доступні факти | Обмеження |
| --- | --- | --- | --- |
| Gameplay writer | Validated resolver, hook gate, незалежно від Gameplay correction setting ([runtime.cpp:7950](../../src/plugin/runtime.cpp#L7950)) | RSI/RBX, XMM0, settings, source, aspect | Немає історії всіх native writes до input |
| Cinematic ENTER/EXIT/aspect | Transactional install/rollback; accepted generation ([runtime.cpp:3289](../../src/plugin/runtime.cpp#L3289)) | Original operand, selection, generation, native request | ENTER authority не доводить successful blend initialization |
| Dialogue setter pre-CALL | Validated CALL boundary ([runtime.cpp:4975](../../src/plugin/runtime.cpp#L4975)) | XMM6, XMM1, RSI context, RCX receiver | Перед native setter; не application evidence |
| Successful blend initialization | Existing structurally resolved hook, production provider handoff ([runtime.cpp:3751](../../src/plugin/runtime.cpp#L3751)) | Component/receiver/start/target, caller attribution | ENTER epoch trace diagnostic-scoped; initializer може не дійти success |
| Setter return | Existing structurally resolved hook; presently Diagnostics/layout gated ([runtime.cpp:4985](../../src/plugin/runtime.cpp#L4985)) | Diagnostic TLS pairing thread/RSP/component, receiver field | Не доводить відсутність intervening writes до writer |
| Writer PRE/POST | Existing resolver post-store hook, Diagnostics gated ([runtime.cpp:5191](../../src/plugin/runtime.cpp#L5191)) | Invocation/source/output/thread, pre/post field | POST ≠ projection consumption; single pending frame може бути replaced |
| Zoom IN/OUT | Observer capability окремо від user diagnostics | Transition observations | Не повний zoom ownership; binoculars можуть іти іншим шляхом |

Production U2 `ObserveSetterSample`/`TakeWriterSample` використовує receiver,
generation, thread, endpoints та відповідність sample. Це корисна admission
кореляція, але не доказ останнього native write: між callback і writer може
працювати інший producer. Перенесення diagnostic TLS pairing у production
потребує окремого lifecycle/rollback/invalidation contract; сам gate OFF не
повинен змінювати рішення provenance.

## 6. Підтверджені проблеми та обмеження

1. **CONFIRMED — P0:** `NativeRegisterInput` автоматично стає допустимим
   baseline input. Попередній runtime підтверджує прийняття `60.9227`, створеного
   Dialogue Reduced, як native baseline. Після Batch1 саме цей repeated-start
   шлях не відтворився, але admission predicate не змінений.
2. **CONFIRMED — ENTER2 discontinuity writer output:** initial retention може
   видати126.87, після чого `authored_baseline_departure` пропускає raw70.1061.
   Це завершення retention, не доказ втрати source ownership.
3. **CONFIRMED — U2 endpoint policy mismatch:**
   `TryMapPostExitProviderDialogueSample` обирає `HorPlus(nativeTarget)` для
   дозволених non-Native Dialogue modes ([camera_writer_transaction.hpp:693](../../src/plugin/camera_writer_transaction.hpp#L693));
   він не розраховує Adaptive/Reduced/Disabled endpoint. При цьому endpoint і
   наступна writer policy мають бути узгоджені, інакше новий handoff cliff.
4. **CONFIRMED — provenance presentation:** overlay `nativeFov` є transaction
   input різних boundaries; не підтверджений original game-origin setting.
5. **CONFIRMED — lifecycle model limit:** Dialogue classifier не semantic
   game event; coordinator Gameplay не є незалежним Gameplay ENTER.
6. **INFERRED — duplication risk:** baseline store, observation store і snapshot
   мають схожі числа, але різних owners і призначення. Сліпе об'єднання не
   усуває defect; `CameraWriterInput` та наявну observation schema доцільно
   розширювати лише доказом із boundary, а не створювати ще один last-FOV store.
7. **UNKNOWN:** який active camera/view споживав FOV під час кожного стрибка;
   чи native field після setter було переписано для конкретного writer; чи
   зміни Native FOV при стабільному Gameplay спричиняє саме HorPlus toggle.
   Відсутність таких записів не доводить відсутність подій.

## 7. Interception та конкретна відсутня подія

Збережений [native data-flow](POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md)
для дослідженого EXE описує base/derived setter у virtual slot+0x608; derived
setter також оновлює optical+0xC7C. Derived GetCameraView може викликати
producer, який переобчислює camera+0x230, перед camera writer. Фактичний
receiver/vtable для обох видимих стрибків цим evidence не встановлений.

Точний missing fact для causal P0 admission: **який native write/producer
останнім сформував значення camera+0x230, прочитане конкретним writer, і чи
цей producer використав mod-mutated setter output**. Ні setter-return, ні
CameraCorrectionGate, ні same source/thread/float, ні PRE/POST output+0x30 не
спостерігають усі можливі intervening writes. У поточному native code вже є
можливий проміжний recalculation path; тому проміжні writes не можна виключити
лише порядком двох mod callbacks.

Найменший спосіб отримати потрібне evidence для конкретного receiver — bounded
hardware write-watchpoint на camera+0x230 між setter-return і writer read,
з context/stack/thread та producer inputs; окремо correlation з active-view
consumption. Watchpoint — дослідницький інструмент, не production authority.
Лише після встановлення фактичного producer можна оцінити структурний resolver
і чи одна production boundary покриває всі потрібні writes. Фіксований RVA,
одноразовий numeric match або таймер не забезпечують цей contract.

## 8. Мінімальні варіанти P0 та trade-offs

| Варіант | Перевага | Обмеження / ризик |
| --- | --- | --- |
| Explicit domain evidence в `CameraWriterInput` → `CameraFovObservation` → `GameplayBaselineStore` | Повторне використання наявної transaction/observation/store chain; known mod-derived не оновлює reference | Потрібен producer доказ, а не автоматична перекласифікація NativeRegisterInput; Unknown не можна продавати як GameOrigin |
| Лише заборона baseline під час Dialogue/Cinematic або cooldown | Дешевий локальний guard | Не відрізняє legitimate Gameplay update; суперечить user contract і не покриває reentry |
| Pair setter return із наступним writer за receiver/thread/value | Використовує наявні hooks | Intervening writes і однакові числа створюють хибну attribution; недостатній як доведений fix |
| Ввести незалежний global FOV cache/manager | Може зберігати числа | Дублює наявні owners, не створює missing origin evidence, додає stale-state ризик |
| Повний Unknown fail-closed без positive source | Не називає unknown GameOrigin | Може назавжди припинити baseline updates і зламати Gameplay witness/U2 eligibility; не виконує динамічний Gameplay contract |

## 9. Рекомендований integration path

1. Зберегти Gameplay correction від кожного incoming writer sample; розділити
   його застосування і право оновлення original reference. Наявний gate,
   CameraWriterInput, observation і GameplayBaselineStore — основні integration
   points. Вони не потребують нового coordinator або global numeric cache.
2. Передавати known mod-derived lineage від фактично застосованої mutation до
   конкретного read лише за підтвердженою execution/producer relation. Нову
   independent game-origin зміну допускати за відповідним evidence, а не за
   числовим відхиленням. Без coverage не називати unknown GameOrigin.
3. Визначити failure/retirement для rearm, cut, source/output/generation/settings
   change, nested calls і unavailable capability. Diagnostics ON/OFF не змінює
   production decision. Відомі cinematic R та provider T зберігають свої scope.
4. Перш ніж обрати production propagation seam, встановити конкретну подію з §7.
   Досліджувати кілька можливих producer paths, якщо вони спостерігаються, а не
   підганяти один signature під конкретний FOV.
5. Regression: повний setter mutation→native propagation→writer→baseline із
   `70.0005→60.9227`, незалежна game FOV change з тим самим числом, звичайний
   Gameplay HorPlus, ADS/cut/rearm/foreign source, Diagnostics OFF/ON, stale і
   interrupted invocation. Поточний opt-in baseline test — red characterization
   admission seam, не інтеграційний PASS.

## 10. Вплив на P1/P2/P3

- **P0 OPEN:** confirmed contamination і відсутня causal boundary; найменший
  production patch ще не можна визначити однозначно з наявного evidence.
- **P1 OPEN:** overlay має показувати підтверджений native reference і
  corrected output з окремими validity/freshness. Саме перейменування не
  відновлює походження; application і rendered consumption лишаються різними.
- **P2 OPEN:** lifecycle/U2 adapters мають опиратися на P0 domain та узгодити
  selected Dialogue endpoint з наступним owner. Формули в аудиті не змінювалися.
- **P3 OPEN:** необхідна візуальна перевірка обох переходів та active-view
  FOV/aspect; writer continuity і passing harness не є visual acceptance.

Оцінка ґрунтується на поточному source і названих captures. Невідомі native
paths і розбіжність між writer output та rendered frame залишені UNKNOWN.
