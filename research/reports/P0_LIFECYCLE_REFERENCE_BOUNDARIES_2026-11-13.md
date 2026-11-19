# P0 — native reference на ENTER/EXIT boundaries

2026-11-13. Scope: перевірка відомих lifecycle seams до нової production зміни.
Source поточного working tree та збережений runtime-182050.log; native evidence
із попередніх аудитів. Без нових hooks, runtime/debugger, build/tests чи source
edits. P0 negative admission regression лишається FAIL; adapters не реалізуються.

## 1. Позитивний результат перевірки

Для обмежених ролей уже існує production request-origin evidence:

1. Accepted Cinematic ENTER operand до mod mutation: власний R generation.
   TraceCinematicEnter захоплює before=context.xmm0 до запису outputFov;
   ProcessAcceptedCinematicEnter окремо зберігає R/C і selection epoch.
   Runtime підтверджує90→126.87 для ENTER1 та ENTER2. Це reference конкретного
   cinematic request, не довільний Gameplay sample.
2. Validated provider call завантажує constant operand і викликає linked wrapper.
   src/hooks/blend_producer_resolver.hpp, ResolveBlendProducer перевіряє
   instruction sequence, RIP-relative data operand та CALL target; numeric
   literal не використовується як classifier. На дослідженому образі operand70.
   Successful initialization з caller attribution підтверджує виконання цього
   request на конкретному receiver. Це native requested target, не доведений
   Dialogue baseline B або семантичний Dialogue lifecycle.

Отже, неправильно стверджувати, що серед наявних hooks взагалі немає request-origin
authority. Вона є, але scoped до цих запитів. Перенесення її на весь Gameplay
або на всі samples component без lineage не випливає з цього результату.

## 2. Матриця lifecycle

| Boundary | Оригінальний параметр до correction | Mutable state поруч | Що можна підтвердити | Що не можна автоматично прийняти |
| --- | --- | --- | --- | --- |
| Вхід у гру / load save | Окремий production Gameplay-start FOV parameter не знайдений | Перший observed writer XMM0, camera fields | Transaction input та факт callback | Перший sample як GameOrigin лише тому, що мод щойно стартував; save може починатися у camera transition |
| Gameplay writer | XMM0 до поточної writer correction | Upstream camera+0x230, aspect, flags | Локальний read-before-write | Відсутність попереднього mod effect; game setting reference |
| Cinematic ENTER | Accepted own authored operand R | Pre-ENTER Gameplay observation/witness W | R до mutation, нова generation; C окремо | W як R, previous corrected C як новий original R, або R як Gameplay reference |
| Provider request→successful init | Original literal-backed requested target, caller-qualified initialization | S із camera+0x230, component/receiver | Requested target цього caller activation | S як original baseline; target70 як універсальний доказ Dialogue ownership |
| Dialogue AcceptedStart | Окремого semantic native Dialogue-enter parameter немає; classifier captures incoming | XMM6 generic blend, Candidate baseline, target fields | Mod classifier lifecycle, не game-owned Dialogue ENTER | Candidate baseline як обов'язково незмінений game-origin FOV |
| Cinematic EXIT | Incoming native exit request target | Cached C, recovery owner, текущий camera field | Pending exit target, scoped exiting generation | Негайний Gameplay resume; EXIT1 може бути superseded іншим request |
| Dialogue EXIT | Ascending/rearm classifier samples | Попередній B, setter output, native target | Mod recovery state і cancellation | Independent game-owned Gameplay target/Dialogue exit event |
| Gameplay RESUME після Cinematic | Existing recovery completion та cached exit request | Writer sample біля endpoint | Mod handoff, source/generation relation | Сама endpoint proximity як game-origin proof; native completion або активний Gameplay view |

Production source: src/plugin/runtime.cpp (TraceCinematicEnter/TraceCinematicExit,
TracePostExitBlendStartCapture, TraceDialogueBoundary, ReplayManualTransition);
src/plugin/cinematic_lifecycle_transaction.hpp (accepted enter/exit);
src/plugin/dialogue_boundary_transaction.hpp (Candidate/AcceptedStart/RearmPending).

## 3. Послідовність фактичного capture

Evidence: ../evidence/u2-transition-policy-2026-11-13/runtime-182050.log,
ASI F07C4FC1F44783A46379D059F84BD9C4A5B077AA51CD877EE488D56CE5849CFD.

| Час | Подія | Reference до mod correction | Mod result / native feedback | Роль після передачі |
| --- | --- | --- | --- | --- |
| Перед18:22:38.588 | Gameplay witness | Writer90.3115 | H≈127.119 | Останній eligible writer; не configured setting proof |
| 18:22:38.588 | ENTER1 generation1 | R90 | C126.87; native consumer отримує C | R лишається own Cinematic reference |
| 18:23:37.030 | EXIT1 | Requested target90.3115 | Recovery, coordinator CinematicExiting | Pending target, не Gameplay-resume certificate |
| 18:23:37.121 | Новий provider activation | Requested T70 | S126.211 із mutable camera field | Новий request може supersede очікуваний return-to90.3115 |
| 18:23:38.090…38.137 | U2 endpoint/handoff | Native sample70.1558…70 | Output108.976…108.941 | Native engine target досягнуто; mod Gameplay coordinator не доводить steady Dialogue semantics |
| Перед18:23:46.904 | Gameplay-tagged writer |70 |108.941, baseline witness70 | Не own Cinematic reference |
| 18:23:46.904 | ENTER2 generation2 | R90 | C126.87; raw70→…→126.87 | Новий R незалежний від old writer70/C |
| 18:24:04.894 | EXIT2 | Requested target90.3115 | Native recovery до exit target | Pending Gameplay reference candidate зі scope generation2 |
| 18:24:05.892 | Recovery completion | Writer90.3141 близько target90.3115 | Output127.121; coordinator Gameplay | Mod resume підтверджений; completion/native origin не виводяться лише з близькості |

У таблиці не приписується semantic Dialogue ownership producer70. Точний native
intervening-write та active-view gap лишаються такими, як у попередньому audit.

## 4. Як lifecycle зменшує обсяг P0

Не потрібно оголошувати окремий global producer для кожного кадру. Потрібно
зберігати authority встановленого reference у своїй ролі протягом його lifecycle
і не переприсвоювати її за повторним читанням через native engine:

- Accepted ENTER зберігає R як immutable reference generation; output C не
  оновлює R. Current U1-A уже зберігає це розділення локально.
- Provider request зберігає original T та mutation/request lineage. Native
  initializer S — observed current state; він не успадковує GameOrigin від T.
- EXIT запам'ятовує original requested target, але Gameplay reference не
  оновлюється, якщо з'явився новий unrelated request або lifecycle handoff
  не підтверджено. EXIT target і фактичний наступний mode — різні факти.
- Gameplay permits dynamic changes (ADS, zoom, settings, cuts). Lifecycle scope
  дозволяє переносити підтверджений lineage, але саме source/generation не
  визначає походження нового числа. Unknown не оновлює reference.
- Native engine може знову видавати C або setter output. Це не новий original
  input незалежно від coordinator Gameplay і незалежно від stabilization.

Це логічне уточнення contract, не нові stores чи реалізована propagation.
CameraCorrectionGate серіалізує рішення мода, але не встановлює історію native
writes між callbacks і не замінює evidence source ownership.

## 5. Межі наявного Gameplay startup/resume

Код не має окремого validated new-game/save-load Gameplay initialization event,
який передає FOV до всіх можливих upstream effects. Initialization hooks мода
і перший Gameplay-tagged writer не тотожні native Gameplay ENTER.
Результат пошуку в camera/gameplay/runtime source не встановив такого handler.

Historical STABLE_GAMEPLAY_BASELINE_CAPTURE_CONTRACT.md уже документує, що
stable writer endpoint і zoom observer не є configured/game-origin authority.
GAMEPLAY_BASELINE_BATCH2A1_NATIVE_COVERAGE.md свідомо зберігав останній eligible
native/pass-through observation без semantic ownership. Це пояснює чинний
scope baseline store, але не виконує нову сильнішу original-reference вимогу.

Сам факт закінчення катсцени/діалогу може дати місце для handoff. Він не доводить,
що камера вже Gameplay, що pending target не superseded, або що наступний writer
не читає mod-derived field. Capture EXIT1→provider70 це конкретно демонструє.

## 6. Рішення щодо P0 scope після цього етапу

Доведені request-origin seams уже достатні для обмеженої реалізації збереження
власного Cinematic R та provider T/mutation lineage. Вони НЕ закривають повний
GameplayBaseline admission invariant автоматично. Треба відокремити task outputs:

1. Existing original request/reference — не змінювати при повторному input C.
2. Mod-derived/unknown transitions — переносити domain до admission без повторної
   класифікації NativeRegisterInput→GameOrigin.
3. Нове independently originated Gameplay значення — потрібен конкретний
   підтверджений producer/handoff rule для нового reference; інакше retain або
   explicit unavailable відповідно до lifecycle, не вигадане GameOrigin.

За scoped target evidence не потрібно шукати нові Cinematic ENTER/provider hooks.
Для Gameplay startup/independent changes та semantic Dialogue ENTER наявні hooks
не дають такого ж позитивного доказу. Якщо для повного P0 буде потрібна додаткова
observation seam, вона має відповідати саме одному з цих невідомих, а не дублювати
generic writer або completion marker. У цьому етапі нові seams не шукалися.

Стан: lifecycle matrix та scoped request-origin paths перевірені. P0 production
origin propagation/admission НЕ реалізовано; negative contract test лишається
FAIL; native-reference overlay і adapters залишаються залежними. Це завершує
саме перевірку відомих ENTER/EXIT boundaries, без нового циклу формул або capture.
