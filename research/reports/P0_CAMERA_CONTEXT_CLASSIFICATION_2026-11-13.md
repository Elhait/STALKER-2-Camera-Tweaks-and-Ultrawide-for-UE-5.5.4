# P0 — класифікація контексту camera writer

2026-11-13. Обмежене проєктування на поточному source та recorded evidence.
Нових production edits, builds/tests, hooks, game/debugger або runtime capture
немає. Це integration path для перегляду, не твердження, що context classifier
уже реалізований або P0 закритий.

## 1. Уточнений контракт

Gameplay writer коригує актуальний input незалежно від того, чому гра змінила
FOV. Значення10°, 20° чи60.9227° не є ознакою режиму. Застосовуються наявні
межі валідності FOV і aspect, а не «типовий Gameplay range».

Розділяємо три різні факти: **camera context invocation**, **FOV domain** і
числовий sample. Контекст Dialogue не доводить, що input уже transformed;
контекст Gameplay не доводить, що upstream native field ніколи не містив mod
output. Останній випадок після реального handoff — окрема transition/domain
задача, не привід відстежувати кожен native write в Gameplay.

## 2. Recorded60.9227°: що саме можна довести

Збережений [capture](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log):

1. 18:03:39.745 Dialogue classifier перейшов Candidate→Active і mutation
   setter argument `70.0005→60.9227` ([рядок583](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L583)).
2. На наступному writer о18:03:39.760 mod Dialogue state ще існував: writer
   викликав reset через camera-context change ([рядок589](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L589)).
3. Після reset той самий writer опублікував baseline native60.9227
   ([рядок597](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L597)).

**CONFIRMED:** у моделі мода на початку writer був Dialogue Active при coordinator
Gameplay; до baseline admission відбувся reset цього state. Отже coordinator
сам по собі приховав конфлікт context evidence. `dialogueContextInvalidated`
у `ProcessGameplayHorPlusWriter` обчислюється до `baseline.Project`, але
результат не бере участі в eligibility (`camera_writer_transaction.hpp:1122,1199`).

**UNKNOWN:** recorded Dialogue classifier є евристикою generic blend; лог не
містить invocation-paired receiver цього setter для writer. Тому з наявного
capture не можна довести, що writer належав *семантичному Dialogue* або що
користувач бачив саме цю камеру. Можна позначити mod-context conflict/
transition ambiguity; не слід називати його доведеним semantic Dialogue.
Значення60.9227 і часовий збіг не є context proof.

Останній Batch1 пригнітив цей конкретний повторний AcceptedStart в новому run;
це не доводить коректність загального context classifier.

## 3. Що наявний framework уже може класифікувати

| Context evidence | Допустимий статус invocation | Межі |
| --- | --- | --- |
| Accepted cinematic generation + matching source/aspect target + per-invocation output admission | `CinematicOwned` | Authority для cinematic policy, не доказ rendered view або domain довільного raw sample |
| `CinematicExiting` + valid recovery owner matching writer source | `RecoveryOwned` | Перехідний context, не підтверджений Gameplay або Dialogue |
| Validated provider activation + receiver/generation/thread і current setter/writer admission | `ProviderTransitionObserved` | Відомий механічний transition; semantic Dialogue/відсутність intervening writes окремо не доведені |
| Dialogue `Candidate/Active/Exiting/RearmPending` при coordinator Gameplay | `DialogueLifecycleObserved` | Це mod classifier; без receiver binding не можна приписати стан кожному writer source |
| Source/generation conflict, reset у тому ж invocation, missing binding, overlapping owners | `TransitionAmbiguous` | Не публікувати як підтверджений Gameplay baseline; не вважати semantic Dialogue автоматично |
| Validated gameplay writer, coordinator Gameplay, немає релевантного unresolved transition для цього source | `GameplayEligible` | Практичний контракт штатного Gameplay writer; не доказ походження кожного upstream числа або active rendered view |

`GameplayEligible` є **операційним admission** за наявним модовим lifecycle,
а не claim про відомий game-owned Gameplay ENTER. Він зберігає наявну динамічну
Gameplay поведінку, не класифікує ADS/zoom/cuts і не порівнює FOV із попереднім
Dialogue/Cinematic числом. Якщо evidence конкретного source конфліктне або
втрачено binding, context=`TransitionAmbiguous`, а не `GameplayEligible`.

Після справжнього повернення в Gameplay чинний writer може знову отримати
mod-influenced native field. Самого `GameplayEligible` недостатньо для доведення
FOV domain; це межа transition handoff, яку не можна приховати в classifier.

## 4. Конкретний integration path у наявних компонентах

1. **Dialogue transaction:** розширити існуючий `DialogueBoundaryStateView`/
   Candidate або active policy state не numeric baseline, а scoped context
   evidence: validated receiver із RCX, callback component/context, lifecycle
   epoch, settings epoch, phase, validity. Capture на accepted boundary до CALL;
   на source mismatch/rearm/settings change/invalidation очистити або поставити
   ambiguous. Відсутній validated receiver не перетворювати на global Gameplay
   denial. Це нове поле існуючого owner, не новий global camera manager.
2. **Writer entry:** під `CameraCorrectionGate` і наявним lock order зняти
   context evidence **до** `EvaluateGameplayContextChange` і будь-якого Dialogue
   reset. Не використовувати `result.dialogueContextInvalidated` як єдиний
   classifier: цей reset залежить від числового jump threshold і відбувається
   лише для частини послідовностей. Snapshot має відобразити стан навіть тоді,
   коли sample не змінився чисельно.
3. **Per-invocation context:** розширити `CameraWriterInput` typed evidence і
   resolve його один раз у `ProcessCameraWriter`: пріоритет cinematic generation/
   per-output admission, recovery owner, source-bound Dialogue lifecycle,
   explicit ambiguity, потім штатний Gameplay admission. Source mismatch не
   дорівнює автоматично чужому Gameplay: за невстановленої topology він Unknown.
   Різні одночасні receivers не слід блокувати лише через глобальний Dialogue
   flag. On nested/reentrant callback не переносити snapshot іншого invocation.
4. **Baseline admission:** використати існуючі `CameraFovObservation` та
   `GameplayBaselineStore::Project`; додати typed context/validity і дозволяти
   baseline update для `GameplayEligible`, зберігаючи R/C та input/output
   окремими. Для підтверджених cinematic/recovery і пов'язаних Dialogue/unknown
   transition — `Retained` або invalidation за перевіреною source/lifecycle
   зміною. Не робити весь Gameplay OFF і не переписувати формулу HorPlus.
   Старий baseline не має подаватися як current за відсутності актуальності.
5. **Correction окремо від classification:** на unknown invocation зберегти
   чинний dispatcher/fail-closed меж конкретного component; не прирівнювати
   утриманий baseline до output і не пропускати чужий sample через чужу
   Cinematic/Dialogue policy. Gameplay input→HorPlus залишається для штатного
   Gameplay writer. Рішення baseline eligibility не повинно тихо відключати
   gameplay correction.
6. **Publication:** snapshot/overlay можуть показати context + freshness,
   але не стають production authority. Diagnostics ON/OFF і trace caps не
   впливають на classifier. Цей етап не змінює UI і не впроваджує adapters.

Це пропонований path, не доказ того, що існуючий callback receiver завжди
ідентичний writer source. Для реального admission потрібен валідаційний контракт
зв'язку receiver↔writer, а не порівняння двох RSI. RCX receiver на setter
boundary уже доступний; використати його можна без нового hook для capture.
Intervening writers/rearm/other views переводять source relation в ambiguous.
Не потрібно з'ясовувати останній native producer, щоб відмовити *baseline update*
під час підтвердженого context conflict; для твердження про domain самого числа
це може залишитися необхідним.

## 5. Неоднозначності та acceptance

| Випадок | Очікуване рішення |
| --- | --- |
| Gameplay inputs10,20,60.9227 з різними значеннями, без релевантного context conflict | Штатний HorPlus і baseline updates; значення саме по собі не є guard |
| Dialogue Active для підтвердженого receiver перед writer, writer скидає Dialogue у цьому ж invocation | Context captured до reset; transition ambiguous/Dialogue observed; цей input не стає Gameplay baseline |
| Dialogue Candidate або Active, receiver не встановлений | Не заявляти Dialogue або Gameplay authority для потенційно пов'язаного invocation; не блокувати unrelated sources глобально |
| Незалежна Gameplay camera під час Dialogue іншого receiver | Не успадковує Dialogue context, якщо незалежна source identity підтверджена; інакше Unknown, не global cooldown |
| Новий legitimate Gameplay sample чисельно дорівнює минулому Dialogue output | За нової допустимої Gameplay context admission оновити baseline; ніякого FOV blacklist |
| Cinematic R90/C126.87, нова generation / output RBX змінився | Нова generation R; output admission для кожного invocation; не перевизначати R input sample |
| Provider rearm тим самим pointer/target, foreign receiver, epoch change, callback interleaving | Старе context evidence invalid; не реюзати cached association/число |
| Diagnostics OFF/ON; normal Gameplay без Dialogue; settings changes | Однакове production context рішення; динамічний Gameplay не блокується через відсутність окремого game-origin hook |
| Cut або handoff без evidence | Unknown, стару authority не продовжувати; не видавати unknown за підтверджений Gameplay |

Потрібні інтеграційні tests реальних `ProcessDialogueBoundary→ProcessCameraWriter→
GameplayBaselineStore` transactions, включно з snapshot-before-reset і same-value
independent Gameplay. Існуючий `--native-origin-contract` моделює лише store seam
і для другого fixture має зовнішню origin annotation: це не end-to-end тест.
Production correction, overlay та результат rendered view треба перевіряти
окремо; numeric baseline PASS не доводить відсутність обох visual jumps.

## 6. Межі рішення

**CONFIRMED:** для recorded invocation mod state був у Dialogue Active до його
скидання writer і baseline update. **INFERRED:** за receiver-bound evidence та
snapshot-before-reset такий конфлікт може відхиляти baseline admission без
доведення останнього native producer. **UNKNOWN:** семантичний camera owner
записаного writer і повнота receiver binding для інших paths.

Отже контекстний path може вирішувати класифікацію доведених конфліктів без
універсального write-watchpoint. Він не доводить domain вже зміненого native
field після справжнього Gameplay handoff і не забезпечує автоматично rendered
view continuity. Якщо ці факти стануть потрібні конкретному виправленню,
спостереження producer/consumer обґрунтовується для цього випадку окремо.
