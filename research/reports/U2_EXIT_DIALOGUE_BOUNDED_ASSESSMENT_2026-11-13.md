# U2: Cinematic EXIT → Dialogue FOV — обмежена оцінка

Дата: 2026-11-13. Висновок **B**: механічний discriminator джерела запиту знайдено; достатнього контракту автоматичної корекції поточного writer sample немає. Production edits, збірки, тести й нові runtime/debugger-сеанси в цій задачі не виконувалися.

## 1. Що саме можна розрізнити

У поточному дослідженому EXE окремий callsite `RVA 0x2E5457D` передає literal 70° у generic blend wrapper `0x30DB9B8`. Його return — `0x2E54582`. Збережений runtime activation із таким return раніше зіставлено з тим самим receiver та setter-шляхом. Cinematic EXIT викликає інший, валідований шлях через `0x35C7D3F`; для нього вже враховано original/relocated CALL returns.

Це позитивна ознака **джерела запиту на активацію**, незалежна від числового порівняння target. Вона не встановлює semantic owner Dialogue і не підтверджує, який activation записав поточний writer input. Сам target=70 не є discriminator.

Повний збережений decode `0x2E5450E..0x2E54596` показує provider resolution, nonnull check, завантаження float-константи та CALL wrapper. У цій ділянці немає підтвердженого Dialogue state/tag. Ширші callers функції не встановлені. Не знайдено підстав вважати адресу стабільною між патчами. Для майбутнього використання потрібен унікальний structural resolver CALL із перевіркою меж, operand/data-flow та target wrapper; адреса й SHA тут лише provenance дослідження.

Джерела: [original decode](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-and-enter-guard.txt), [constant read](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-constant.txt), [попереднє зіставлення activation](CINEMATIC_FOV_AUTHORITY_AUDIT.md#ідентифікація-окремого-producer70).

## 2. Поточний runtime: встановлені факти

Збережено [незмінний snapshot журналу](../evidence/u2-exit-dialogue-2026-11-13/STALKER2CameraTweaks-141503.log), 616981 bytes, SHA-256 `D25E77060F2594CC8D369A0AFD8C554ED2D1C94F024DB4C74D786D6086A68E7C`.

- Game SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- ASI SHA-256: `DB25DC0D0CF62A11AB720788B9E31E24000BD9C91FF9E6464307A57D11D1FC01`.
- Gameplay RuntimeCombine; Cinematic NativeHorPlus/Auto; Dialogue Reduced; Diagnostics ON. Під час writer samples підтверджено aspect 3.55556; це не незалежний вимір rendered projection.

| Подія | Спостереження | Межа висновку |
| --- | --- | --- |
| ENTER1 14:17:38.947 | authored 90 → selected 126.87 | U1-A selection працює в цьому capture |
| EXIT1 14:17:57.464 | native operand 90.3115, accepted generation1/recovery owner | Native EXIT intent, не гарантія наступних camera values |
| Callback 14:17:57.486 | XMM6 126.857; target90.3115 | Shared blend tick почав спостерігатися |
| Callback 14:17:57.752 | XMM6 122.212; target70 | Native target у callback context уже інший |
| Callback 14:17:58.442 | XMM6 71.7549; latest writer72.1634 | Snapshot writer попередній, не invocation pairing |
| Writer 14:17:58.453 | input/output71.7549, UNCLASSIFIED | Числова й порядкова кореляція з callback |
| Writer 14:17:58.580 | input/output70; fov_applied=0 | 70 доходить до writer без модифікації FOV у цьому invocation |
| ENTER2 14:18:04.723 | authored90 → selected126.87 | Own-reference не підмінено значенням70 |
| EXIT2 14:18:22.725 | native target90.3115 | Такий самий saved EXIT target |
| EXIT2 callbacks | native target90.3115; XMM6 знижується від126.858 | Той самий callback також обслуговує recovery |
| 14:18:23.723 | recovery completed, exclusion cleared | Completion підтверджений журналом; візуальна коректність — окреме повідомлення користувача |

Під час обох post-EXIT callback-серій точний результат — `ExclusionReset`, `guard_reason=post_cinematic_exclusion_active`, `result_mutated=0`. Перша причина раннього виходу — exclusion; перевірка coordinator також існує нижче, але в цих invocation до неї не доходять. `selected_policy=Native` у ранньому result не доводить, що користувач обрав Native: configuration — Reduced, а result ще не дійшов до selection.

У першій серії 54 provenance records, у другій 38 records під час Exiting та 59 після повернення Gameplay. Hard cap128 на generation не вичерпано. У capture немає `START_CAPTURE`, blend sample/caller records; startup capability і event availability не доводять активацію цього окремого one-shot capture.

## 3. Структурний зв'язок об'єктів і межа спостереження

Поточний native tick `0xCCE4FA` працює з `T = component S + 0x28`. У Dialogue boundary `RSI=T`, target `[T+0x2C]=[S+0x54]`, start `[S+0x58]`; RCX уже містить receiver наступного virtual setter CALL `+0x608`. Отже, різні RSI callback і writer очікувані: це blend context і camera source відповідно. Їхня нерівність не доводить різних камер. Перевірений RCX receiver можна спостерігати через наявний hook без нового interception point.

Це підтверджують [native data-flow](POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md), `PostExitBlendCallbackCapture` у [runtime.cpp](../../src/plugin/runtime.cpp) та [activation observation](../../src/diagnostics/post_exit_blend_activation_observation.hpp). Нинішній lightweight provenance запис не зберігає RCX receiver/activation caller; старий повний observer уміє їх фіксувати, але в цьому capture його не озброєно.

Збережений попередній runtime доводить окремий producer70 в іншому запуску того самого EXE. Його не можна автоматично приписати кожній події70 нового запуску. Новий журнал доводить зміну спостережуваного target, а не те, що той самий component був rearmed: до/після не зафіксовано повний activation lifecycle.

Навіть підтверджений receiver не потребує повного semantic camera ownership для вузького експерименту. Однак залишається конкретна математична неоднозначність: native initializer захоплює camera+0x230 як start, і цей start може бути вже transformed. Blend не містить FOV-space tag. Прямий HorPlus(sample) може виконати подвійну корекцію. Інший activation або native writer на тому самому receiver може змінити поле до shared camera writer.

## 4. Чинне рішення та альтернативи

Production `ProcessGameplayWriter` / `ProcessHorPlusRecovery` у [camera_writer_transaction.hpp](../../src/plugin/camera_writer_transaction.hpp) використовують підтверджений source, native EXIT target та cached corrected cinematic endpoint. [gameplay_state.cpp](../../src/gameplay/gameplay_state.cpp) визначає допустимий recovery segment; samples поза ним не отримують нової authority. [ProcessDialogueBoundary](../../src/plugin/dialogue_boundary_transaction.hpp) спершу перевіряє exclusion, потім coordinator.

| Варіант | Доказова оцінка / ризик | Рішення зараз |
| --- | --- | --- |
| Дозволити Dialogue під час Exiting | Callback є generic blend tick, також присутній при EXIT2; можна змінити штатний recovery | Не приймати |
| HorPlus усіх UNCLASSIFIED або всіх70 | Немає FOV-space/producer доказу; подвійна корекція і чужі camera modes | Не приймати |
| Завершувати recovery при70 | Число не означає Gameplay endpoint; втрата exclusion/owner і ризик ENTER2 | Не приймати |
| Розрізняти successful activation за валідованим CALL lineage | Реальний discriminator запиту, не camera semantics; перевикористовує наявний start hook | Придатний для bounded observation / shadow model |
| Коригувати конкретний tick на setter boundary | Поточний receiver та invocation доступні без вгадування майбутнього writer; але writer потім може повторно застосувати recovery mapping | Кандидат дослідження, зараз не product fix |

**B:** ознаки достатньо для відмінності request paths у дослідженому образі, недостатньо для безпечного автоматичного intervention. Це конкретна межа доступних доказів, не вимога відновити весь закритий рушій.

## 5. Найбезпечніший обмежений експеримент — shadow, один capture

Це пропозиція наступного scope; реалізація та запуск не виконувалися. Поточну product behavior до окремого рішення зберегти.

1. Мінімальна діагностична підготовка: структурно знайти third producer CALL і класифікувати його точний return у наявному start observer як окремий механічний caller kind. Нуль/неунікальність/invalid decode → Unknown. Не називати його Dialogue за70 і не додавати SHA/RVA gate.
2. Через наявні start/tick/post-setter boundaries записати epoch, component, RCX receiver, validated receiver shape/vtable, source relation, start/target, branch/eased sample і actual setter argument. Це вимога до підготовки capture, не твердження про повноту нинішнього adapter: самі XMM7/XMM8 не доводять branch/progress. Якщо чинна точка не дозволяє безпечно прочитати та перевірити актуальні native branch/settings, shadow mapping лишається unavailable; не викликати довільний native resolver із hook. Replay/rearm того самого component замінює epoch; неповна кореляція має статус Unknown. Нових native hooks не потрібно.
3. Для спостережуваного producer-specific activation обчислювати лише hypothetical mapping. Для підтверджених affine branches: `w=(f-S)/(T-S)`, `H(w)=C+(HorPlus(T,aspect)-C)*w`. Це дослідницька модель: C — підтверджений corrected endpoint попередньої катсцени, S/T — native activation endpoints, f — фактичний raw tick output. Не застосовувати HorPlus до S повторно. Перевіряти finite/range, придатність T-S і незмінність branch/endpoints; невідома branch, S≈T або зміна policy → shadow unavailable. Це не доводить, що C є фактично rendered start.
4. Окремо оцінити downstream writer: якщо він переобчислює вже hypothetical corrected setter sample через чинний recovery, помітити double-mapping hazard. Не використовувати last setter cache як дозвіл для наступного writer. Не змінювати ні XMM1, ні XMM0, ні native fields, ні guards.
5. Один ручний one-shot capture послідовності Cinematic1 → Dialogue → Cinematic2 → EXIT2. Event треба подати **до ENTER1**: нинішній `ManualFovCapture::Consume` споживає його тільки на accepted ENTER. Сигнал у діалозі почне capture на ENTER2 і пропустить потрібне EXIT1. У наявному capture від ENTER1 до completion EXIT2 приблизно44.8с, що вкладається у чинні60с. Якщо сценарій не вкладається — непокрите позначити Unknown, не продовжувати таймер автоматично.
6. Загальний бюджет старого blend capture192 records/60с зберегти, caller/epoch transitions та terminal outcomes мають пріоритет перед повторними samples. Diagnostics OFF: instrumentation не працює; один звичайний `STALKER2CameraTweaks.asi`, налаштування через INI. Capture припиняється при budget/deadline, новому непов'язаному source/порушенні correlation, shutdown; втрата запису не перетворюється на absence evidence.

Мета цього єдиного capture: перевірити, чи producer-specific активація справді є джерелом transition70 цього сценарію, чи доступний придатний branch/progress/FOV-space mapping та чи чинний writer зруйнував би такий mapping. Це не ще один загальний FOV log.

**Критерій зупинки:** якщо request→activation→current setter або придатний mapping не спостерігаються наявними точками, не додавати наступні hooks заради абсолютного ownership. Залишити поточну поведінку й записати цей zoom як відому межу сумісності. Навіть позитивний shadow не є дозволом на product mutation: після нього потрібне окреме вузьке рішення щодо writer/setter routing, без обіцянки підтримки всіх camera transitions.

## 6. Regression і runtime validation для майбутнього scope

Нижче — acceptance matrix, не виконані тести.

| Сценарій | Необхідна перевірка |
| --- | --- |
| Producer-specific / EXIT / Unknown caller з однаковим target | Класифікація спирається на validated CALL relation; зміна самого числа не змінює caller kind |
| Зміна SHA при незмінному native contract; relocated CALL | Рішення resolver не залежить від SHA; exact validated return mapping |
| Transformed-start126.87→70 і native-start70→90.3115 | Різні provenance; shadow не застосовує HorPlus двічі й не використовує current policy output як native reference |
| Старий106.688→90; EXIT2→90.3115 | Existing recovery output/complete/exclusion не змінені |
| Два component одного receiver; rearm із тими самими endpoints | Epoch замінюється; ambiguity не успадковує право mutation |
| Intervening writer / same-source cut | Receiver equality не знімає uncertainty; не утримувати старий FOV для приховування cut |
| U1-A ENTER90 із witness70, continuation, repeated ENTER | Target походить від own authored reference; жодних змін selection/retention |
| Settings/aspect/source змінено; disabled correction | Shadow інвалідується; product рішення залишаються чинними |
| Diagnostics OFF, cap/deadline, incomplete pair | Немає shadow work/mutation; Unknown і terminal reason видимі; bounded output |
| Ручний сценарій | Окремо оцінити діалог, ENTER2, EXIT2 і Gameplay; writer output не видавати за rendered FOV |

Ризик майбутньої live-корекції найбільший на межі setter→writer: два різні mapping можуть дати подвійну корекцію або новий policy cliff. Ризик послаблення guards — false Dialogue admission на звичайному EXIT2. Ризик для U1-A — успадкувати target70 як cinematic reference або перенести shadow state через новий ENTER. Жоден із цих ризиків не усувається лише точним визначенням producer CALL.

## 7. Диспозиція

Завдання дослідження завершено з висновком B. Рекомендований наступний scope — один bounded shadow experiment на наявних observation points, якщо потрібна наступна спроба виправлення; зараз автоматична корекція не обґрунтована. Повне semantic ownership не є acceptance gate. Необхідні лише ознаки, що прямо впливають на вибір математичної політики та її application boundary. Новий широкий архітектурний аудит, повторення старих watchpoint sessions та додаткові hooks не рекомендовані.
