# Cinematic FOV authority audit

Дата: 2026-11-13. Статус після додаткового аудиту: **BLOCKED — ADDITIONAL EVIDENCE REQUIRED** для повної реалізації. Production implementation не виконано. Доповнення нижче уточнює попередні рекомендації.

Актуальні незалежні статуси (мають пріоритет над узагальненим BLOCKED):

| Напрям | Статус | Межа висновку |
| --- | --- | --- |
| ENTER selection | **ROOT CAUSE CONFIRMED** | Код і runtime довели підміну own cinematic reference попереднім Gameplay baseline. Семантична роль producer70 не потрібна для цього висновку |
| U1-A | **BOUNDED CONTRACT CANDIDATE** | Own ENTER retention відокремлено від UPDATE у §10.5; потрібне прийняття обмеженого scope та його реалізація/перевірка |
| U1-B | **UPDATE PROVENANCE UNKNOWN** | Same-source raw sample не розрізняє authored UPDATE та blend; універсальна підтримка не доведена |
| U2 | **RECOVERY UNRESOLVED** | Capture 11:18:55 охопив EXIT2, native blend 70→90,3115 та відновлення HorPlus. Coverage gap закрито для цього сценарію; безпечна recovery policy ще не визначена |

Capture 11:18:55 підтвердив `CreationGuardRejected` для ENTER2; деталі наведено в §10.4. Це результат guard, а не встановлена внутрішня причина його відмови. U1 лишається відкритим: authored UPDATE цим проходженням не встановлено.

## Предмет і джерела

Аудит стосується ефективного working tree, включно зі staged ownership changes та unstaged diagnostics. CONFIRMED означає підтвердження названою ділянкою коду або конкретним capture; INFERRED — висновок; UNVERIFIED — відсутній доказ. Зміни production, збірки, тести та нові runtime-сеанси в цьому завданні не виконувалися.

Перевірено `git status --short`, unstaged/cached diff statistics, `git diff HEAD` для cinematic ownership і writer integration, HEAD lifecycle transaction та історію цих файлів. Working tree вже містив численні сторонні зміни; вони збережені. Основний baseline selection і continuation уже присутні в HEAD; staged зміни розділяють RSI source та RBX output admission. Unstaged діагностика додає observation boundaries, а не новий cinematic transform. Це не аудит безпеки всього стороннього diff.

Розміщення цього звіту відповідає [research README](../README.md); план знаходиться в `research/plans/` за чинною практикою робочого дерева.

Ключові посилання на поточні вихідні файли:

| Позначка | Файл / функція / початкова ділянка |
| --- | --- |
| S1 | [runtime.cpp:3052](../../src/plugin/runtime.cpp#L3052), `ValidateEnterBoundary`; `ResolveCinematicFovCallsites`:3131; installation:3983 |
| S2 | [runtime.cpp:3519](../../src/plugin/runtime.cpp#L3519), `TraceCinematicEnter`, до публікації observation:3650 |
| S3 | [cinematic_lifecycle_transaction.hpp:160](../../src/plugin/cinematic_lifecycle_transaction.hpp#L160), `ProcessAcceptedCinematicEnter` |
| S4 | [cinematic_fov.cpp:345](../../src/cinematics/cinematic_fov.cpp#L345), selection та transformation:355–418 |
| S5 | [cinematic_fov.cpp:26](../../src/cinematics/cinematic_fov.cpp#L26), generation, aspect target, observation, admission, initial retention:190 |
| S6 | [cinematic_fov.cpp:286](../../src/cinematics/cinematic_fov.cpp#L286), `ResolveCinematicWriterSample` |
| S7 | [camera_writer_transaction.hpp:1623](../../src/plugin/camera_writer_transaction.hpp#L1623), CinematicActive dispatch і per-invocation admission |
| S8 | [cinematic_lifecycle_transaction.hpp:319](../../src/plugin/cinematic_lifecycle_transaction.hpp#L319), `ObserveCinematicAspectStore`; EXIT:377 |
| S9 | [runtime.cpp:2495](../../src/plugin/runtime.cpp#L2495), aspect-store callback; Gameplay observation/baseline publication:6131–6180 |
| S10 | [gameplay_state.cpp:18](../../src/gameplay/gameplay_state.cpp#L18), reentry predicate і recovery classification; [writer transaction:1525](../../src/plugin/camera_writer_transaction.hpp#L1525) recovery dispatch |
| S11 | [config_template.cpp:222](../../src/config/config_template.cpp#L222), задекларована різниця `GameplayHorPlus` / `NativeHorPlus` |
| S12 | [ARCHITECTURE.md:17](../../docs/ARCHITECTURE.md#L17), ownership; [SAFETY_INVARIANTS.md:122](../../docs/SAFETY_INVARIANTS.md#L122), recovery |

Номери рядків описують стан під час аудиту; функції залишаються основними навігаційними anchors.

## Фактичний pipeline і межі authority

CONFIRMED (S1–S3): resolver знаходить унікальний ENTER pattern, перевіряє RIP-relative `MOVSS XMM0,[...]`, rel32 CALL і native vcall evidence. MidHook стоїть на callsite після завантаження XMM0. Отже, ENTER має власний native operand до модифікації модом. Це boundary evidence, а не доказ фінальної rendered camera. ENTER регістри RCX/RSI не можна автоматично ототожнювати з writer source.

`TraceCinematicEnter` під correction gate готує immutable effective selection, отримує наступну generation, читає authored input, актуальний aspect і GameplayBaseline, викликає transaction, змінює XMM0 та публікує input/result окремо (S2–S3). Повторний ENTER за вже valid selection відхиляється; generation не визначається порівнянням FOV. Прийнята generation очищує попередній recovery owner, handoff і exclusion.

CONFIRMED (S3–S4): `GameplayHorPlus` вибирає `gameplayBaseline.nativeFov.value`, якщо baseline придатний. `NativeHorPlus` використовує authored ENTER. Формула спочатку масштабує tangent projection від `cinematicReferenceFov` до `targetBaselineFov`, потім виконує HorPlus. На ENTER `cinematicFov == cinematicReferenceFov`, тому target фактично дорівнює HorPlus(selected baseline). Наявність власного ENTER operand не забезпечує його пріоритету в GameplayHorPlus.

CONFIRMED (S5, S8–S9): ENTER ще не має підтвердженого writer source. Aspect-store boundary реєструє target object. Перший writer із `RSI == aspectStoreTargetObject` встановлює source association. Суперечливий aspect target інвалідує association. Звичайна зміна RBX за тим самим RSI зберігає її; destination допускається для конкретних generation/source/output/sequence. Source conflict і output transition мають різну семантику.

CONFIRMED (S5–S7): native writer input не стає автоматично власним cinematic FOV. Initial retention допускає samples, схожі на authored ENTER або source-bound Gameplay baseline. Коли input залишає обидві категорії, establishment завершується. Dispatch або утримує cached transformed target, або використовує обмежену continuation policy, або пропускає sample. Це захист від повторного transform і native overwrites, але не універсальна модель authored cinematic UPDATE.

## Джерела та сховища

| Дані | Власник і запис | Використання / обмеження |
| --- | --- | --- |
| Authored ENTER input | S2; `enterNativeFov` і association.authoredEnterFov у S3 | Власна native boundary evidence; зберігається окремо від corrected output; source пов'язується пізніше |
| GameplayBaseline native / HorPlus / aspect / source | Gameplay observation projection, S9 | Numeric target для GameplayHorPlus; retention qualification; continuation source. Придатність baseline не доводить відсутність Dialogue/blend |
| `transformedFov`, association.targetFov | S3 | Retained cinematic output; не native input. Дубльовані representations потребують узгодженого оновлення |
| preEnterNativeFov / preEnterHorPlusFov / preEnterAspect | S3 | Контекст matching/diagnostics; не поточний cinematic UPDATE |
| latestWriter.rawFov / sequence / identityEpoch | S5 | Boundary sample; identityEpoch змінюється і при RBX transition, тому це не cinematic generation |
| lastAdmittedOutput | S5 | Історичний destination із generation/source/sequence; не pointer lifetime guarantee |
| EXIT nativeTarget і exitTransformedFov | S8 | Різні endpoints для recovery; transformed endpoint копіюється до очищення active target |
| FovObservation / Camera State | S2, S9 | Observational publication; не додаткова correction authority |

## Повна карта залежностей від Gameplay

| Залежність | Фактична мета | Наслідок вилучення |
| --- | --- | --- |
| S4 `SelectCinematicBaseline` | Matching катсцени до Gameplay FOV | Потрібна явна зміна семантики GameplayHorPlus |
| S3 preEnter fields / gameplayContinuationSource | Зберегти reference і source для continuation | Не можна видалити source binding разом із numeric target |
| S5 `BeginGeneration` baseline/source | Дозволити initial replacement native baseline після ENTER | На реальному writer input 70 при authored 90 проста заміна selection може залишити writer passthrough |
| S5 `AuthorizeInitialEstablishmentRetention` | Authored/baseline classification + generation/source/output admission | Треба зберегти authorization окремо від numeric provenance |
| S6 gameplayBaseline comparison | Retention classification | Не переносити будь-який writer sample в authoritative input |
| S3 baseline-reentry fallback:255–268 | Зберегти continuation, коли reference відрізняється від authored ENTER | Зміна reference змінює можливість перенесення старого target |
| S7 gameplayBaselineContinuation | Додаткові per-call guards для такого continuation | Зберегти generation і output checks; переглянути numeric baseline assumptions |
| S8 / S10 EXIT endpoints і recovery | Передача керування Gameplay, exclusion, native interpolation | Новий cinematic target змінить recovery geometry, навіть без змін recovery classifier |
| S11 config / UI; S2 diagnostic predictions | Задекларувати matching, пояснити reference | Потрібно оновити опис і тести разом із семантикою, не приховувати під старою назвою |

Історично це свідоме рішення: [F1–F8 plan:37](../completed/CAMERA_MODULE_REMEDIATION_F1_F8_TASK_PLAN.md#L37) задає GameplayHorPlus через raw Gameplay baseline, lifecycle-scoped selection та source-bound recovery. `git log` для lifecycle/FOV показує release commits `db67e4d` (2026-10-09) і `9e8ebdc` (2026-09-21). HEAD уже містить selection і baseline reentry; поточне виправлення RBX ownership не ввело цю policy. Це конфлікт старої matching-семантики з новим погодженим контрактом, а не доказ випадково доданої останнім diff формули.

## Runtime evidence та root cause

Поточний game log: [STALKER2CameraTweaks.log](<E:/Steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl/Stalker2/Binaries/Win64/STALKER2CameraTweaks.log>), рядки 644–693 на момент аудиту. Це live файл, який може бути перезаписаний наступним запуском. Сталий CDB transcript: [cdb-live.log](<E:/Work/Slaker2 mods/02-Research/evidence/downstream-runtime-20261112/cdb-live.log>).

CONFIRMED: 2026-11-13 00:20:36.208 generation 2 має authored ENTER 90, selected numeric baseline 70, target 108.941. Перший writer о 00:20:36.220 має source `0x2c5bf2c8080`, output `0x2c6b3a93690`, raw 70, flags 5, admitted association, результат 108.941. EXIT summary о 00:20:54.206: 1169 samples, усі applied, input min/max 70, result min/max 108.941. Це прямо підтверджує підміну numeric reference і успішне застосування отриманого target.

CONFIRMED: CDB scalar selection для 127.119 і 108.941 показує однаковий manager, override 0, cache fallback; captures матриць містять відповідні різні коефіцієнти. Для 108.941 є пряме спрацювання main math boundary. Назви `CINEMATIC1_SCALAR_SELECTION` (рядок 500) і `DIALOGUE_SCALAR_SELECTION` (589) — ручні мітки. Вони не доводять lifecycle. Не можна стверджувати, що capture 108.941 — перший projection кадр generation 2; його зроблено до підтвердженого ENTER #2. У ранньому capture 127.119 main/alternate marker не спрацював; окремий post-jump capture підтвердив main branch для 127.119.

CONFIRMED: після EXIT gen2 native blend 70→90.3115 проходить recovery; о 00:20:55.198–199 recovery завершується біля 90.3081 і Gameplay публікує HorPlus 127.116. Пізніший одноразовий projection capture має scalar 127.1187 і матрицю. Це окремий EXIT discontinuity; він не є доказом причини звуження на ENTER.

INFERRED: спад retained target із 127.119 до 108.941 узгоджується з описаним користувачем звуженням. Root cause відносно погодженого контракту CONFIRMED у S3–S4: gameplay numeric sample має пріоритет над власним authored input нової катсцени. Повний причинний ланцюжок до першого дефектного rendered frame UNVERIFIED. Твердження «діалог завжди має 70» є контекстом від автора; production-рішення не повинні залежати від цього числа.

## UPDATE, aspect та source switching

CONFIRMED (S3, S5–S9): окремого authoritative cinematic UPDATE API тут немає. Retention не означає update capture. Raw writer 70 співіснує з authored ENTER 90, тому lifecycle+source самі по собі не доводять, що новий writer input повинен замінити cinematic reference. Input, рівний cached transformed output, також не доводить native origin.

Aspect-store повторно resolves aspect, але не перераховує `transformedFov` у цій transaction (S8). Requested settings і active selection розділені; автоматичне використання поточних requested settings під час UPDATE порушило б lifecycle latching. Потрібно перераховувати за чинною effective policy і validated aspect, а не приймати новий mode без переходу.

CONFIRMED (S5): source switching не підтримується як автоматичний rebind. Інший aspect target призводить до conflict; foreign source не отримує initial admission. Повторний writer одного source/output не створює нову generation. Invalid ENTER math проходить native, target не встановлено; немає доведеного delayed-reference promotion mechanism. Будь-яке майбутнє promotion потребує незалежного native boundary evidence.

UNVERIFIED: яка наявна native boundary повідомляє authored FOV change всередині generation та відрізняє її від blend/copy/replay. Погоджений повний UPDATE-контракт не можна реалізувати шляхом `HorPlus(rawWriterInput)` на кожному CinematicActive callback.

## Transition matrix і ризики

| Перехід | Чинний контракт | Ризик зміни |
| --- | --- | --- |
| Gameplay→Cinematic | ENTER selection, потім aspect-source admission | Втратити retention writer input, відмінного від authored input |
| Dialogue→Cinematic | ENTER очищує exclusion і бере останній Gameplay baseline | Виправити reference; зберегти незалежний Dialogue reset/lifecycle |
| Cinematic→Gameplay | EXIT snapshots native/corrected endpoints і current generation owner | Новий corrected endpoint не обов'язково описує native blend start |
| Cinematic→Dialogue | Exclusion до native recovery, потім Dialogue rearm | Не дозволити early Dialogue correction або stuck exclusion |
| Cinematic→Cinematic | Accepted EXIT/reentry predicate, source та cached targets | Старий target не має перевизначати новий authored intent |
| Duplicate ENTER/EXIT | selection-valid guards; accepted EXIT очищує active state | Не створювати generation numeric comparison-ом і не застосовувати stale event |
| Source/target conflict | Fail closed; output A→B→A дозволений за current admission | Не перетворити source change на звичайний output switch |
| Interrupted/empty generation | Carry inherited continuation за обмежених умов S8 | Stale carry може поширити стару policy на нову generation |

## Альтернативи

| Критерій | A: тільки selection | B: окремий authority store | C: чинний framework state |
| --- | --- | --- | --- |
| ENTER own input | Так | Так | Так, authored field уже існує |
| UPDATE provenance | Не вирішує | Може представити, але не здобути доказ | Потребує явного qualified update API |
| Додатковий state | Немає | Дублі поколінь/source/targets | Мінімальні validity/provenance/revision поля за потреби |
| Double correction | Старі guards лишаються | Залежить від ingestion | Зберегти native/corrected roles і per-call admission |
| Source switch | Старий conflict | Ризик розходження двох owners | Чинний association owner; explicit invalidation |
| Policies | Непомітно зливає два modes | Можна розділити | Потрібна явна migration semantics |
| Recovery | Нова endpoint geometry без оцінки | Новий handoff integration | Існуючий EXIT snapshot з актуального confirmed target |
| Тестованість | Прості positive тести приховають прогалини | Висока, але більше concurrency states | Actual lifecycle/writer adapters уже доступні |
| Оцінка | Недостатній повний fix | Надлишковий як новий manager | Рекомендовано з evidence gate для UPDATE |

Рекомендація: C. Використати наявні generation/association/authored input/target та correction gate, додавши до цього owner лише мінімально потрібний provenance/revision для qualified updates. Не створювати глобальне сховище «останнього правильного FOV». A може бути частиною реалізації ENTER, але не доводить виконання всього контракту. B допустимий лише як локальний typed record всередині чинного owner, якщо без нього неможливо виразити native validity; новий незалежний authority manager не обґрунтований.

Новий user contract supersedes numeric matching із останнім Gameplay sample. Проте S11 обіцяє саме matching: Luna має явно відобразити зміну семантики в config/UI/contracts та перевірити custom Gameplay FOV. Неприпустимо мовчки оголосити, що обидва modes зберегли стару поведінку. Підтверджений незалежний Gameplay matching, якщо його зберігати як окрему можливість, потребує власного qualified reference; це не дозвіл повернути latest baseline як fallback.

## Висновок і незакриті докази

ENTER root cause встановлено. Повна реалізація має також змінити retention/continuation assumptions та перевірити EXIT endpoints. Нових projection hooks не обґрунтовано. Відкриті питання: provenance authored UPDATE у межах generation; native підтвердження безпечного source rebind (якщо він потрібен); перший render view дефектного ENTER; прийнятна migration semantics колишнього Gameplay matching. Останнє є продуктовим рішенням у межах погодженого own-reference контракту, а не невідомою механікою гри.

План для Luna визначає незалежно виконувані етапи й точну зупинку перед непідтвердженим UPDATE. Жоден PASS старого regression набору не є доказом майбутньої policy.

## Додатковий аудит перед передачею Luna

### Розмежування налаштувань

CONFIRMED: `Gameplay.Mode=RuntimeCombine` і `Cinematics.FovMode=GameplayHorPlus` — різні selectors. Перший керує Gameplay aspect/FOV transaction; другий обирає reference катсцени. У цьому capture вони активні одночасно. Пропонована migration стосується cinematic FovMode; змінювати рекомендований Gameplay RuntimeCombine через схожість назв не потрібно. Джерела: `src/config/feature_config.hpp:33`, `src/config/config_template.cpp:222`, S3, S7 та runtime EXIT gen2 `gameplay_mode=RuntimeCombine`.

### Перевірений native producer graph

Використано збережений disassembly образу SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`; нові decoder/debugger процеси не запускалися. Це обмежений graph, не exhaustive virtual-call analysis усього EXE.

| Evidence | Native data-flow | Висновок і статус |
| --- | --- | --- |
| [native-boundaries.txt:4](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/native-boundaries.txt:4>) | `0x35C7CB2`: state byte +AC порівнюється з DL; при однаковому значенні ранній return. ENTER `0x35C7D99` завантажує RIP-relative scalar; CALL `0x35C7DA1` | CONFIRMED: цей ENTER producer state-transition driven; його повторне виконання з тим самим state не утворює authored UPDATE. Constant-load operand не є live camera field |
| [target-forwarding.txt:3](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/target-forwarding.txt:3>) | `0x6AFFF7C` зберігає XMM0, resolves global provider +100, tail-calls `0x30DB9B8` із XMM1 | CONFIRMED: shared ENTER/EXIT consumer передає target у generic blend; він не є безпосереднім store camera+230 |
| [target-state.txt:3](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/target-state.txt:3>) | Provider+2CF8: existing component; однаковий target повертає early; інший target re-arms `0x30DBCD2` | CONFIRMED: native target може змінитися/бути rearmed без нової allocation. Але це generic target change, не доказ authored Cinematic UPDATE |
| [target-arming.txt:3](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/target-arming.txt:3>) | `0x30DBCDB`: S+54=target; `0x30DBD12`: S+50=duration; `0x30DBD60..68`: S+58=receiver+230; S+40=weak camera | CONFIRMED: target/start/receiver різні ролі; failed paths можуть змінити target до capture start |
| [blend-tick.txt](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/blend-tick.txt>) | `0xCCE4FA`: easing; `0xCCE670..673`: raw XMM6→XMM1→vslot+608 | CONFIRMED: чинний Dialogue observer бачить generic interpolated setter argument. Він не відрізняє cinematic authored producer від Dialogue/EXIT на підставі самої boundary |
| [camera-base-setter.txt:3](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/camera-base-setter.txt:3>) | `0x1F88EDC`: camera+230=XMM1 | CONFIRMED: base setter виконує store без provenance tag |
| [fov-evaluation.txt:3](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/fov-evaluation.txt:3>) і [writer-caller-state-type.txt:6](<E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/writer-caller-state-type.txt:6>) | Derived view calls `0x3236604`; evaluator `0x3236740` обчислює FOV з optical fields; view потім викликає writer | CONFIRMED: camera FOV також має optical producer. Не можна всі зміни +230 приписати одному blend |

У saved graph не знайдено producer, що водночас доводить authored cinematic UPDATE, active generation і receiver/source binding. Знайдені generic rearm/tick і optical evaluation доводять можливість FOV changes без ENTER, але не їхнє cinematic authorship. Повний перелік непрямих virtual callers не відновлено в наявних artifacts; це конкретна межа статичного покриття, а не твердження, що окремого UPDATE немає. `refs-0x30dbcd2.txt` містить header без встановлених callers і не є доказом їх відсутності.

### Чому ENTER 90 співіснує з writer 70

CONFIRMED runtime: після gen1 EXIT записано activation epoch2 component `0x2c73f627960`, receiver `0x2c5bf2c8080`, start90.3115→target70. О 00:05:49.152 post-setter поле цього receiver дорівнює70. Перед gen2 о 00:20:36 writer того самого RSI читає70; aspect store gen2 і writer прив'язані до того ж source. Gen2 EXIT summary показує raw70 протягом усіх1169 samples. Отже, це не інший RSI source і не прихований пізній raw ramp у покритих writer calls.

CONFIRMED: модове утримання target змінює writer XMM0/output, а native source +230 лишається70. Source ownership не означає, що authored ENTER90 повинен негайно з'явитися в source field. Static consumer ENTER — blend target arming з early/failure paths, а не synchronous camera field assignment.

INFERRED: stable70 успадкований від зафіксованого попереднього native blend того самого receiver. UNVERIFIED: відсутність intervening writes за15 хвилин між phases; хто скасував/перекрив blend, armed actual ENTER, чи native consumer повернув early, або інший producer утримував70. Caller epoch2=`unmatched_return`; це не підтверджує семантичне ім'я Dialogue. Новий висновок сильніший за numerical coincidence, але не закриває конкретну native cause збереження70 після ENTER.

Для own-reference retention однієї заміни selection недостатньо: старий код авторизує raw70 саме завдяки historical gameplayBaseline/source. Видалення цієї qualification може припинити initial establishment. Безумовне утримання126.87 для кожного admitted sample, навпаки, приховає справжній authored UPDATE, якщо він приходить через той самий writer. Поточні captures не розрізняють ці випадки. Критичний blocker — не право записати в RBX, а semantic authority змінити/утримати cinematic reference при raw≠ENTER.

### Source/generation contract — що вже визначено

| Ідентичність | Підтверджений зв'язок | Заборонений висновок |
| --- | --- | --- |
| ENTER RSI/RCX | Native lifecycle owner у callback; gen2 RSI `0x2c6579091c0` | Це не writer RSI і не pointer до active render camera |
| Aspect-store RAX | Прийнятий target у current generation | Наявність target не дає native FOV UPDATE provenance |
| Writer RSI | Exact equality з aspect target establishes association | Не доводить автора кожного sample |
| Writer RBX | Current native destination, admitted з sequence | Не persistent camera identity; cached RBX не розіменовується recovery |
| selection generation | Accepted ENTER/EXIT scope | Не blend activation epoch |
| writer identityEpoch | Змінюється при RSI або RBX transition | Не означає нову cinematic activation |
| writer sequence | Конкретний observed invocation, current admission | Не глобальний frame ID і не порядок подій інших native потоків |

S5/S7 достатньо для чинного output ownership. Нового source framework не потрібно. Однак прив'язка нового authored UPDATE до цього source залишається UNVERIFIED; при source/target conflict потрібна invalidation, автоматичний rebind не планується.

### Aspect recalculation: визначений механізм, ще не реалізований

CONFIRMED S8: aspect callback resolves effective aspect і writes aspect, але не оновлює target. `ResolveAspect` (`src/cinematics/cinematic_aspect.cpp:74`) підтримує Auto і forced policies; `OverrideEnabled` вимикає correction для Native policy. Target replicas — Runtime transformedFov, association.targetFov, ENTER/target publication; вони не мають автоматичного shared revision.

Рішення C: на admitted cinematic writer перед policy dispatch resolve effective aspect з active latched policy; для Auto використати чинний validated viewport resolver, для forced — відповідний effective aspect. Якщо validated aspect змінився, під correction gate compute target від власного qualified native reference та commit усі target replicas з єдиним revision. Sequence/source/generation checks виконати до commit. Aspect-store callback має використовувати цей самий helper, коли reference і binding уже valid; якщо callback прийшов раніше, writer виконає потрібний commit. Diagnostics не постачає aspect. Invalid resolver не створює новий target; mutation у непідтвердженому aspect domain fail closed. Requested settings не замінюють latched active selection. Конкретне правило numeric aspect tolerance взяти з чинного viewport/aspect validation contract, не з FOV tolerance.

Це закриває вибір механізму aspect update на рівні плану. Actual callback ordering, replica coherence і gate serialization ще мають бути перевірені adapter tests. Target revision на EXIT snapshot має відповідати останньому committed revision.

### Continuation/recovery з новим endpoint

CONFIRMED S3/S5–S10: initial retention, baseline reentry fallback і gameplayBaselineContinuation використовують baseline для різних задач. Їх не можна вилучати одним видаленням структури GameplayBaseline. Own target selection має бути відокремлений від per-call admission; inherited continuation не може переносити previous target поверх new ENTER intent. Empty generation carry нині дозволений тільки якщо generation не спостерігала writer; це окремий guard, який треба зберегти й перевірити після reference revision.

CONFIRMED `gameplay_state.cpp:85–117`: interpolation classification використовує interval між cached corrected endpoint та native EXIT target. Старий interval приблизно[90.3115,108.941]; після own reference буде[90.3115,126.87]. Recorded native70→90.3081 залишається поза ОБОМА interval. Endpoint completion біля90.3115 повертає Gameplay HorPlus. Отже, selection change сама не усуває EXIT cliff. INFERRED: перший перехід corrected cinematic→native70 може стати більшим (126.87→70 замість108.941→70), якщо native-start path і downstream consumption збережуться. Це ризик regression через зміну input recovery без зміни classifier.

RuntimeCombine також викликає `ProcessHorPlusRecovery` (writer transaction:1482–1555) після aspect synchronization; той самий interval issue актуальний і при рекомендованому Gameplay mode. Старий transformed-start fixture106.688→90 має іншу геометрію і не підтверджує native-start path. Без proven sample-space lineage не можна розширити interval до70 або механічно HorPlus-transform кожен sample.

### Конкретна migration semantics

Рекомендація за погодженим own-reference контрактом: canonical cinematic mode=`NativeHorPlus`; legacy `GameplayHorPlus` і наявний parser alias `MatchGameplay` приймати при читанні INI як compatibility aliases до own-reference mode. Parsing version-independent; при наступному штатному save записувати canonical `NativeHorPlus`. Не переписувати INI під час самого завантаження; один bounded migration log. Два UI choices із однаковою математикою не залишати: показувати одну cinematic FOV policy; legacy FOV-cycle binding читати без помилки, але як deprecated no-op з bounded notice, без зміни aspect-cycle key. Старі locale keys можна залишити для compatibility, нові тексти мають пояснювати own cinematic FOV. Native aspect policy/OFF, aspect selectors і Gameplay.Mode=RuntimeCombine зберігають незалежні значення.

Це навмисно припиняє старий Gameplay numeric matching; збереження його як окремого mode потребувало б іншого контракту confirmed Gameplay reference і не входить у цей own-reference fix. Migration має бути задокументована як зміна поведінки, особливо для custom Gameplay FOV; не заявляти повне збереження старої семантики GameplayHorPlus. Це конкретна пропозиція для review плану, ще не внесена в config/UI.

### Переглянуте рішення і blockers

C залишається доцільним для owner/generation/source/aspect; B у вигляді нового manager не вирішує відсутність native provenance. Проте попередній план не можна передавати як готовий full implementation: код не має достатнього сигналу, щоб одночасно гарантувати own target retention при raw70 і коректний authored UPDATE через той самий source.

| Умова передачі | Статус |
| --- | --- |
| ENTER authority | CONFIRMED boundary; source binding відкладений до existing aspect/writer association |
| Retention raw≠ENTER та authored UPDATE | BLOCKER U1: native producer attribution/dispatch distinction не підтверджено |
| Source/generation ownership | Визначено для current output admission; не підміняє UPDATE provenance |
| Aspect recalculation | Механізм визначено вище; implementation/tests pending |
| Continuation / EXIT | Paths проаналізовано; BLOCKER U2: native-start recovery sample space/lineage для нового endpoint не доведено |
| Config migration | Конкретне backwards parsing/canonicalization рішення наведено |
| Перший rendered view | UNVERIFIED; validation gap, а не привід для нового production correction hook |

Наявні captures навмисно обмежені (post-EXIT армер/tick, writer PRE/POST, окремі projection samples); вони не мають caller attribution для authored update всередині активної generation. Жодна додаткова перестановка цих самих чисел не відновить відсутній native call identity. Найкоротший discriminating capture та умови його дозволу описані в доповненні плану нижче. Статус повної реалізації: **BLOCKED — ADDITIONAL EVIDENCE REQUIRED**.

### D1 — caller attribution не враховує relocated CALL

CONFIRMED code defect у діагностичній моделі: `runtime.cpp:3187–3188` записує тільки native ENTER/EXIT address+5; `ReadBlendCallerEvidence` (`src/diagnostics/post_exit_blend_activation_observation.hpp:31`) порівнює return slot лише з ними. SafetyHook (`external/safetyhook/safetyhook.cpp:565–570`) копіює rel32 CALL у trampoline і виправляє displacement. CPU CALL повертає до relocated instruction+length, а не до original callsite+5. За нормальної цієї hook-схеми checker не зможе впізнати такий ENTER/EXIT caller. Це діагностичний дефект; camera math від нього не залежить.

INFERRED: logged `caller_return=0x7ff65a980005` при game base `0x7ff65a9d0000` узгоджується з trampoline return і лежить поза EXE. Визначити, чий це конкретно trampoline, ретроспективно неможливо без його allocation/mapping evidence; не позначати його EXIT лише через nearby log. `tests/plugin/post_exit_blend_activation_cases.hpp:128–150` перевіряє синтетичний stack slot з original адресами, а не виконаний relocated CALL, тому не закриває цей сценарій.

Найкоротше усунення attribution gap у майбутньому діагностичному етапі: установити exact mapping original↔relocated call-return для вже installed ENTER/EXIT hooks, перевірити його decoded instruction/actual adapter та записати mapping при startup. Не допускати всю trampoline region як відомого caller. Зберігати unknown, якщо mapping не підтверджено. Нових native observation points для виправлення саме D1 не потрібно. Це ще не дає універсального authored UPDATE producer, але прибирає конкретну втрату evidence у наявній схемі.

## D1 diagnostic implementation — 2026-11-13

Після окремого дозволу користувача реалізовано діагностичний етап; попередні розділи описують стан до нього. Cinematic selection, formula, retention, continuation та recovery policy не змінено. Статус full FOV fix залишається BLOCKED до runtime evidence U1/U2.

CONFIRMED code: MidHook має read-only доступ до allocation trampoline, аналогічний existing InlineHook API. `ResolveRelocatedBlendCallReturn` перевіряє original/relocated E8, довжину, арифметику адрес і тотожність call target; повертає лише exact relocated return. Runtime створює mapping для existing ENTER/EXIT hooks за Diagnostics ON та записує `BLEND_CALL_RETURN_MAPPING`; при rollback mapping очищується. Checker приймає exact original або validated relocated return; ambiguous/foreign/сусідні адреси залишаються unknown. Metadata не надає camera correction authority.

CONFIRMED actual test: `ExecutedBlendCallRelocationCases` у `tests/plugin/post_exit_blend_activation_cases.hpp` генерує callable Win64 fixture, встановлює справжні MidHooks на два rel32 CALL та callee після prologue `push RSI; push RDI; sub RSP,28h`. Під час виконання читається справжній saved return через hook context+38h. ENTER/EXIT класифіковані саме за relocated address, відмінним від original+5. Перевірено foreign executed CALL, сусідню trampoline адресу, truncated CALL, wrong call target і retirement. Це перевіряє механізм, але ще не native game caller attribution у майбутньому capture.

Capture озброюється перед native ENTER CALL, а не лише на EXIT. Existing observation class обмежує window 60 секундами monotonic clock із першого ENTER; нові generations/EXIT не поновлюють deadline. Blend structured records мають спільний process-session cap192, pair cap4 і existing callback cap4096. PRE-enter writer snapshot та writer PRE/POST залишаються на чинних boundaries; нові native hooks не додано. Intended runtime scope — одна Dialogue→Cinematic→EXIT sequence до60 секунд. Existing unrelated diagnostics можуть мати власні записи; cap192 стосується blend capture family, а не всього game log.

Regression виявив додаткову boundedness помилку existing PRE/POST diagnostics: після8 sampled PRE і без POST delivery pending frame залишався valid, тому CompleteCameraWriterPostStoreTrace друкував той самий PRE на кожному наступному writer. Додано per-frame `preRecorded`: PRE друкується лише раз, pending evidence залишається для POST. Це необхідно для bounded capture, не змінює writer output. Actual aggregation test із10000 output transitions після виправлення:115 file lines при ON,12 при OFF; раніше отримано10015 store-trace records і FAIL. Початковий FAIL збережено в evidence, не видано за PASS.

Validation: focused diagnostic-work ON/OFF PASS; фінальний повний `runtime_native_adapter_harness --non-graphics` PASS —224 fresh processes, Diagnostics OFF/ON, supervisor timeout120s, cleanup verified. Це adapter regression, не canonical test.cmd усіх harnesses. Гру/debugger/WARP не запускали. Build/test evidence: `research/evidence/d1-caller-attribution-2026-11-13/`. Native runtime attribution, U1 і U2 очікують нового capture; попередні unmatched_return ретроспективно не перекласифіковано.

Фінальний production-profile Release build PASS; cleanup verified. Candidate: `build-artifacts/d1-diagnostics/STALKER2CameraTweaks.asi`, 3,729,920 bytes, SHA-256 `818C76E05A899E98A9F5D28FAF4767A82730A3106235A563E9B728043AE5EB29`. Звична ASI назва, diagnostics через INI; окремий diagnostic-profile binary не потрібен. Попередній root/installed ASI не перезаписувався, installation не виконувалася. [Validation summary](../evidence/d1-caller-attribution-2026-11-13/validation-summary.json) і raw build/adapter records збережені. Поточний artifact ще не має game-runtime PASS.

## Runtime D1 capture 01:23:33 — 01:26:54

Новий evidence supersedes попередній статус «artifact не запускався»: користувач виконав game runtime. Аналізатор не запускав гру/debugger, не змінював код або INI. Збережено [immutable log copy](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/STALKER2CameraTweaks.log), SHA-256 `8A4F6605574B96F19C84C23966713DD90ABE26EEA6F1C1C0FB6E919AB9CFA93D`. Log identity підтверджує ASI `818C…EB29`, EXE `2BD3…5479`, Gameplay RuntimeCombine, cinematic GameplayHorPlus, Dialogue Reduced, Diagnostics ON, aspect3.55556.

CONFIRMED D1 native runtime: mapping має ENTER original `0x7ff7bdf47da6` → relocated `0x7ff7ba940005`, EXIT original `0x7ff7bdf47d44` → relocated `0x7ff7ba930005`. О 01:25:54.652 successful initialization epoch1 має exact EXIT relocated return і `caller_kind=cinematic_exit_return`. Receiver `0x26a29109a00`, start=target90.3115. Це перший позитивний game-runtime доказ, що corrected caller attribution працює. Відсутній ENTER event не перетворюється на negative PASS усієї mapping-схеми.

| Подія | Час / generation | Факт |
| --- | --- | --- |
| ENTER1 | 01:25:34.953 /1 | native90, forwarded127.119; capture озброєний до native CALL |
| EXIT1 | 01:25:54.652 /1 |1327 writer calls, raw90.3115→127.119; summary activation_captures=0 для active scene |
| Mapped EXIT activation | 01:25:54.652 /1 | epoch1, receiver той самий, start=target90.3115 |
| Independent activation | 01:25:54.758 /1 | epoch2, інший component `0x26a413238e0`, той самий receiver; start90.3115→target70, return `0x7ff7bd7d4582` |
| Setter completion | 01:25:55.751 /1 | epoch2 actual post-setter receiver+230=70 |
| ENTER2 | 01:26:28.267 /2 | native90, baseline70→108.941; successful-init records відсутні в покритій частині |
| Capture deadline | 01:26:34.953 |60 секунд із ENTER1; для gen2 покрито лише перші6.686 секунди |
| EXIT2 | 01:26:46.259 /2 |1395 writer calls: raw70 і corrected108.941 весь captured writer lifecycle; EXIT поза blend window |
| Recovery completion | 01:26:47.256 | існуючі writer/recovery logs доступні, але gen2 activation/setter lineage цим capture не покрито |

Важлива межа: record budget не вичерпано на ENTER2 (session_records42); cutoff спричинив wall-clock deadline. Вікно спрацювало згідно з лімітом, але почалося на попередній катсцені. Тому не можна оголосити U2 перевіреним на новій instrumentation або приписати відсутність EXIT2 activation native поведінці. Capture не поновлювати автоматично після timeout.

### Ідентифікація окремого producer70

CONFIRMED: з native original EXIT `0x35C7D3F` і logged absolute address отримано game base `0x7ff7ba980000`. Unmatched return `0x7ff7bd7d4582` відповідає RVA`0x2E54582`, а не relocated trampoline цього ENTER/EXIT. Бounded offline decode фактичного EXE (SHA незалежно перевірено) показав:

```
function 0x2E5450E..0x2E54596
0x2E54568 CALL 0xF176BA          ; resolves provider into stack slot
0x2E5456D MOV RCX,[RSI]
0x2E54570 TEST RCX,RCX
0x2E54575 MOVSS XMM1,[RIP+...]   ; RVA0x9EDE384 = float70.0
0x2E5457D CALL 0x30DB9B8         ; generic blend wrapper
0x2E54582 ...                   ; exact captured return
```

Джерела: [native-caller-and-enter-guard.txt](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-and-enter-guard.txt), [constant read](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-constant.txt), supervised run records у тому самому каталозі. Обидва read-only decoder subprocesses завершились без timeout із verified cleanup; target EXE не виконувався.

Це зв'язує конкретний runtime activation70 із точним native CALL і його literal target. «Unmatched» тут означає third caller поза ENTER/EXIT classification, а не помилку D1. UNVERIFIED: reflected/semantic роль цієї function — Dialogue, weapon transition чи інший controller; її ширші callers ще не ідентифіковані. Не додавати classifier «якщо70, тоDialogue». Це також не authored UPDATE поточної катсцени: event відбувся після EXIT1.

### ENTER без successful initialization і уточнення gates

CONFIRMED: протягом active gen1 activation_captures=0, але observer зареєстрований і пізніше бачить EXIT1. У перших6.686 секундах gen2 теж немає start captures. INFERRED: ці конкретні ENTER не проходять той successful camera-start initialization, який observer покриває. Це не доводить, що shared consumer не викликано.

Додатково перевірено guard `0x30DBB08..0x30DBB71`: positive target→provider resolver0xCCE84C→virtual getter+818→float comparison; equality повертаєfalse для creation path. Wrapper0x30DB9B8 має також invalid-provider та existing-component/same-target early returns; initializer має власні failed-camera paths. Отже, відсутність successful-init може бути легальним раннім виходом у кількох конкретних місцях. Logged arguments90/forwarded108.941 не визначають результат getter+818 або saved component target. Це точна відсутня branch evidence для ENTER, а не причина для ще одного загального camera log.

D1: runtime positive PASS для EXIT; exact ENTER mapping startup validated, executed game ENTER attribution не спостерігався. Producer70 та його receiver/setter path тепер CONFIRMED. U1 залишається BLOCKED: authored UPDATE усередині active generation не спостерігався і його producer не доведено. U2 залишається BLOCKED: другий EXIT поза deadline, а provenance/новий endpoint policy не встановлені. Наступний evidence крок має перевірити конкретний wrapper/guard branch і почати capture без попередньої довгої катсцени; нових production FOV змін цей лог не авторизує.

## 10.3 Consumer outcome та one-shot capture — реалізація 2026-11-13

Діагностичний етап 10.2 реалізовано в звичайному **production-profile ASI** з незмінною назвою `STALKER2CameraTweaks.asi`. `POST_EXIT_BLEND_OBSERVATION` присутній у цьому профілі, але consumer hooks та event створюються лише за `[Diagnostics] Enabled=true`. Іншого FOV authority, Gameplay/Cinematic/Dialogue policy, retention чи recovery не додано й не змінено.

Новий structurally resolved набір містить чотири точки: ENTER return/fallthrough, resolver result, creation-guard result і спільний wrapper epilogue. Остання точка розміщена на початку epilogue `0x30DBADC`; hook на попередньому compare/branch span відхилено, бо він перекривав native branch targets. Resolver перевіряє інструкції, branch destinations, межі `.text`, унікальність і відсутність overlap з активними hook spans. У поточному read-only image mapping точки знайдено за RVA `0x35C7DA6`, `0x30DB9CF`, `0x30DBA00`, `0x30DBADC`. Це підтвердження саме поточного EXE, не hash gate.

Кожен armed ENTER має bounded TLS invocation frame з ID, generation, thread, stack, native input та forwarded target. Wrapper/resolver/guard/epilogue observations зводяться лише за тим самим frame. Неповний, stale, overflowed або mismatched frame дає `OutcomeUnobserved`. Перевірка окремо обмежує `ProviderNull`, `ResolverObjectFlagRejected`, `ExistingTargetMatched` і `CreationGuardRejected`; випадок resolver-null не може стати `ExistingTargetMatched` лише через випадковий збіг pointer/flags. Досягнення наявної blend-start observation позначається `InitializerObservationReached`, але **не** означає успішний native commit.

Одноразове озброєння використовує named auto-reset event `Local\STALKER2CameraTweaks.FovCapture.<PID>`. Немає polling worker. Existing ENTER callback перевіряє event zero-timeout; 60-секундний capture починається від прийнятого ENTER. Загальний budget лишився консервативним: **192 structured capture records** (менше ліміту 256), 4 blend pairs і 4096 callbacks; consumer, blend та post-store capture записи використовують той самий лічильник. Подальші покоління не поновлюють deadline або record budget. Event creation failure вимикає лише цю diagnostic capability.

Додано [команду одноразового arming](../tools/ArmCinematicFovCapture.ps1). Вона перевіряє Shipping process, відкриває вже створений event, сигналить його й закриває handle; вона не створює event і не запускає гру. Синтаксис скрипта та фактичний `OpenExisting`/`Set` перевірено на тимчасовому event. Команда з кореня репозиторію:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "research\tools\ArmCinematicFovCapture.ps1"
```

Якщо запущено більше одного процесу гри, вкажи точний PID: `-GameProcessId <PID>`.

### Регресії та кандидат

- Actual adapter suite: **224/224 fresh-process cases PASS**, Diagnostics OFF/ON; графічні сценарії пропущено. Цільові `diagnostic-work` ON/OFF проходять.
- CPU fixture виконує native-like CALL-и через реальні SafetyHook MidHooks і перевіряє provider-null (включно з adversarial self-pointer), invalid resolver flags, guard reject, existing target match, initializer без start evidence, mismatch/overflow, nested LIFO, stale generation, ізоляцію TLS між потоками, незмінність тестової native memory та одноразовість event.
- Чинний EXE image resolver PASS; чотири адреси наведено вище. Це статична перевірка resolver, не runtime гри.
- Release build PASS; version resource `3.2.2.0`, product `3.2.2`, розмір `3,764,224` bytes. Новий окремий кандидат: `build-artifacts/consumer-outcome/STALKER2CameraTweaks.asi`, SHA-256 `39114C22D4A3D036F0BBAFC6471A88B9C6DC27CCBACD40FC2D130A3532FC353C`. Попередній D1 ASI збережено без зміни: SHA-256 `818C76E05A899E98A9F5D28FAF4767A82730A3106235A563E9B728043AE5EB29`.
- Усі контрольовані процеси завершені; supervisor підтвердив cleanup. Гру, debugger і graphics runtime не запускали; ASI автоматично не встановлювали.
- Збірка показала довжинні CMake warnings для object directories кількох test targets та наявні third-party Zydis C4201 warnings, але production target зібрався й злинкувався успішно.

Під час валідації застаріле очікування «рівно 4 records» відхилило новий спільний бюджет; тест виправлено на спільний cap із окремими перевірками наявності типів записів. Чиста перекомпіляція також виявила, що dependency cache цього harness не відстежував includes (`deps=0`); target очищено й перебудовано, після чого наведений фінальний PASS отримано з актуальних файлів. У першому варіанті тестового заголовка стандартний `<barrier>` опинився всередині anonymous namespace; include перенесено на рівень translation unit, чиста компіляція та фінальні тести пройшли.

**Межі результату:** це діагностичний кандидат, не FOV fix. Нового runtime capture немає. ENTER consumer outcome ще потрібно зібрати в грі; U1 authored UPDATE/retention та U2 EXIT2/recovery лишаються невирішеними. Маркер initializer observation не доводить commit або те, що renderer використав це значення. Evidence bundle: [validation summary](../evidence/cinematic-consumer-outcome-2026-11-13/validation-summary.json) та прикладні process results у тій самій теці.


## 10.4 Ручний runtime capture ENTER2 / EXIT2 — 2026-11-13

Користувач вручну виконав arming script для PID12036; спроби агента завершилися до сигналізації event, бо його process enumeration не бачив PID. Скриншот користувача підтверджує Shipping process12036. Причина недоступності процесу агенту не встановлена; інший сеанс або права не доведені.

Архів: [runtime log](../evidence/cinematic-consumer-outcome-2026-11-13/runtime-111855/STALKER2CameraTweaks.log). SHA-256 архівного snapshot: `B8F35C2290D5E7D5AD937F3D80CC58A821E946869D9EB899171B0121D31B14D3`. Runtime identity: ASI `39114C22D4A3D036F0BBAFC6471A88B9C6DC27CCBACD40FC2D130A3532FC353C`, game `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`; aspect3,55556, Gameplay RuntimeCombine, Cinematic GameplayHorPlus. Архів містить уже записані події, не новий запуск.

### Підтверджений outcome та межа guard evidence

11:18:55.730: generation2 ENTER native90, baseline70, forwarded108,941. 11:18:55.731: invocation1/thread8668 має resolver_seen=1, nonnull resolver_result, guard_seen=1, guard_accepted=0, shared_epilogue_seen=1, actual_return=1, correlation_lost=0, start_seen=0; outcome=`CreationGuardRejected`. Це позитивне спостереження повернення false від creation guard і повернення consumer, а не висновок лише з відсутності initializer marker.

`component=0` і `receiver=0` описують доступні поля diagnostic frame; самі по собі не доводять відсутності всіх native camera objects. `native_start_commit=unverified` не перетворюємо на загальне твердження про всі native writes.

Перевірено classifier у `src/diagnostics/cinematic_consumer_observation.hpp::ConsumerInvocation::Outcome` та `src/plugin/runtime.cpp::TraceCinematicConsumer`: guard result читається з AL після CALL; equality/unordered беруться з flags у спільному epilogue. Тут `comparison_seen=0`, тому `native_compare_equal=1/native_compare_unordered=1` **не є evidence existing-target comparison**. `saved_target=0` також не є підтвердженим target компонента без comparison evidence.

Статичне джерело: [guard decode](../evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-and-enter-guard.txt), RVA0x30DBB08..0x30DBB71. Guard перевіряє positive target, викликає ВЛАСНИЙ resolver0xCCE84C, перевіряє null/bit30, викликає virtual getter+0x818 та повертає CMPNEQSS(getter,target)&1; null/invalid повертаєfalse. Для додатного forwarded108,941 основні невирішені альтернативи — inner resolver null/invalid або getter equality. Nonnull resolver_result wrapper не доводить валідність окремого inner resolver. Точну внутрішню причину поточний capture не виміряв.

11:18:55.743 writer читає raw70 та передає108,941. Чинний lifecycle/selection authorizes writer незалежно від diagnostic outcome: `TraceCinematicEnter` публікує selection до native CALL; `guardAccepted` використовується тільки observer. Diagnostic guard result не має production correction authority. Успішне native blend initialization не є чинною передумовою cinematic writer correction. Чи має новий контракт зберігати це розділення — окреме рішення; відмова guard сама по собі не спростовує чинний cinematic lifecycle.

### EXIT2 та continuity

11:19:13.729 EXIT2 отримує target90,3115, recovery_owner_captured=1. Того ж моменту start capture має caller_kind=cinematic_exit_return, receiver0x19c827d6700, start70, target90,3115. Наступні matched_tick, POST_SETTER та writer records пов'язують sample із тим самим receiver. 11:19:14.709 native90,2877 ще проходить passthrough; 11:19:14.722 input90,3074 дає output127,115 та recovery completion; 11:19:14.737 input90,3115 дає127,119. Це writer policy discontinuity, не вимір фінальної проєкції.

Рядок completion має generation=0 після скидання стану, тоді як activation має generation2. Не використовувати generation0 як доказ нової activation. `postExitTrace=disabled` у загальному EXIT status не скасовує конкретні records observer: фактичне покриття підтверджено ними.

### Оцінка та наступне рішення

ENTER selection ROOT CAUSE CONFIRMED. Consumer outcome CONFIRMED; внутрішня причина guard UNKNOWN. U1 IMPLEMENTATION BLOCKED: authored UPDATE не встановлено. U2 RECOVERY UNRESOLVED, але EXIT2 coverage тепер CONFIRMED. Записані native endpoints не дають автоматичного права коригувати кожен проміжний sample або вважати всі його значення Native-space.

Далі потрібне обмежене рішення щодо lifecycle authority при відхиленому native blend, retention/UPDATE та mapping native-start recovery. Current selection authority і native commit слід оцінювати як різні факти. Не прив'язувати production до diagnostics, не застосовувати global numeric equality correction. Наявний guard decode звужує альтернативи; новий runtime дозволений лише окремо, якщо вибір policy справді залежатиме від inner resolver/getter outcome. Широкий збір FOV, нові hooks та зміна camera behavior цим аналізом не виконані. Передача повного FOV fix на реалізацію поки не готова.

## 10.5 Розділення U1-A/U1-B та U2 ownership — bounded analysis

Цей розділ уточнює gates10.4. Production edits, нові hooks, builds, tests або runtime не виконувалися. Джерела: поточні `src/cinematics/cinematic_fov.cpp` (`ObserveWriter`, `AdmitWriterOutput`, `AuthorizeInitialEstablishmentRetention`, `ResolveCinematicWriterSample`, `CinematicRecoveryOwner::Capture`); `src/plugin/cinematic_lifecycle_transaction.hpp::ProcessAcceptedCinematicEnter`; cinematic/recovery branches `src/plugin/camera_writer_transaction.hpp`; `src/gameplay/gameplay_state.cpp::ResolveHorPlusRecoveryAction`; `src/gameplay/horplus_gameplay.cpp::ResolveHorPlusRecoveryInterpolationFov`; `src/diagnostics/post_exit_blend_activation_observation.hpp` та runtime111855.

### U1-A: own ENTER retention можна відокремити

Підтверджено кодом: native authored input уже зберігається окремо в `enterNativeFov` і association; accepted generation/settings та source/output admission існують незалежно від guard result. `AuthorizeInitialEstablishmentRetention` перевіряє чинну generation, source/aspect-store binding, exact current sequence/output, lastAdmittedOutput та raw відповідність authored input або Gameplay baseline witness. При іншому raw припиняє initial establishment. `ResolveCinematicWriterSample` повторює numeric admission, далі має незалежний continuation path. Отже, заміна лише selection не гарантує узгодженого retention.

Можливий **обмежений контракт U1-A** для плану реалізації:

1. Accepted ENTER створює власний native reference R=authored operand та corrected target C=чинна cinematic policy(R,validated aspect). Невалідний R не заміщується Gameplay baseline. ENTER не надає права foreign writer до чинного binding/admission.
2. У valid generation cinematic lifecycle авторизує лише admitted invocation. Native guard false не скасовує lifecycle; guard true не авторизує чужий invocation.
3. Pre-ENTER native witness W може підтверджувати незавершене встановлення source FOV (у capture W=70), але **не визначає R або C**. Зберегти його provenance/source/generation relation та нинішні sequence/output checks. Розділити роль witness від target selection навіть якщо storage повторно використовується.
4. Під час initial establishment raw, що відповідає R або допустимому W, отримує C. Так raw70 може дати HorPlus(90), не HorPlus(70). Збіг із W не доводить authored UPDATE і не є загальним доказом native FOV-space.
5. Departure від дозволених raw evidence завершує initial retention за чинною fail-closed семантикою; unknown raw не стає новим R. При invalid generation/source/aspect/settings domain invocation проходить без успадкування чужого C. RBX A→B→A дозволено лише з новим per-call admission.
6. Continuation/reentry не можуть повернути previous generation target замість нового R/C; дозволена continuity повинна бути окремо кваліфікована. EXIT бере endpoint саме цієї generation; duplicate/rejected events не публікують новий reference. New ENTER, source conflict, stop/reset інвалідують залежний retention; active selection/aspect revision обробляються чинним serialized owner.

Це доказ структурної можливості обмеженої реалізації, **не runtime PASS майбутнього коду**. U1-B не потрібен для збереження R=90 на вже спостереженому raw70. Точне зміщення коду має охопити selection + retention + continuation guards, не лише formula. Пряме копіювання останнього corrected output, guard-driven authority та blanket overwrite всіх admitted raw не входять у цей контракт.

### U1-B: межа гарантії та контрприклад

Справжній authored UPDATE не встановлений. Два native сценарії можуть мати однакові доступні спостереження: (A) після ENTER90 source ще має dialogue/blend70; (B) під час тієї самої generation авторська зміна до70 проходить невідомим producer без нового accepted ENTER. Source/output, flags і raw70 збігаються. Retention90 правильний для A, але може приховувати зміну в B. За поточними evidence їх неможливо розрізнити.

Тому U1-A не доводить універсальної підтримки динамічних катсцен чи camera cuts у межах тієї самої source/generation. Для обмеженого scope UPDATE — явна непідтверджена можливість; unknown raw не просувається в reference. Якщо вимога включає такі UPDATE, U1-B залишається окремим gate. Якщо scope обмежений accepted ENTER reference та існуючою fail-closed departure policy, U1-B не блокує його реалізаційну підготовку. Прийняття цієї межі потрібне до implementation; це не нова вимога знімати capture.

### U2: recovery ownership contract candidate

Чинний `CinematicRecoveryOwner::Capture` фіксує generation/source і last admitted output evidence. Це право розглядати writer після EXIT; воно не встановлює producer або простір його FOV. Classifier та mapping використовують interval `[cachedCinematicFov,exitNativeTarget]`, тоді як native-start70 поза ним. Однієї заміни interval на receiver match недостатньо.

Кандидат контракту має дві послідовні перевірки:

- **Lifecycle eligibility:** accepted EXIT чинної generation, snapshot corrected endpoint, valid owner/source, актуальні camera fields/settings. Кешований RBX не розіменовується. Новий ENTER, source conflict, відключення/невалідність policy завершують або скасовують тільки відповідне право; stale generation не може відновити його.
- **Invocation provenance:** activation належить цьому EXIT через валідований call lineage, receiver відповідає owner, актуальний epoch пережив rearm/retirement; поточний tick/setter і writer пов'язані перевіреним execution relation. Потрібен explicit FOV-space/progress contract. Самі target equality, same thread, близький час та receiver не замінюють його. Кілька activation на receiver дозволені; вибір last activation не авторизує writer автоматично.

Native володіє часом/easing; mod не вводить таймер. При підтверджених native start S,target T та sample f progress можна брати з валідованої branch/math лише при придатному T-S. Visual start endpoint C походить із EXIT snapshot; visual end G — із Gameplay policy(T), а не з raw sample як нового baseline. Формула між C і G ще потребує прийнятого provenance/mapping contract; вона не випливає з самого owner. S=T/invalid progress не обробляються діленням або довільним fallback. Раніше transformed-start106,688→90 має залишитися окремо класифікованим, щоб не застосувати HorPlus вдруге.

Completion lifecycle та передача корекції Gameplay — окремі факти: при незмінній policy зміна owner не повинна змінювати mapping. На обґрунтованій handoff boundary outputs мають відповідати одній policy; це не вимога згладжувати native cuts. Missing evidence означає відсутність authority для нового mapping, а не утримання старого FOV безстроково. Cancellation не гарантує continuity і не має маскувати camera cut.

### Чому наявний observer ще не є production U2 authority

`PostExitBlendActivationObservation` має diagnostic lifecycle, bounded capacity і start/tick matching; same component start capture замінює epoch. Це краще за pointer identity, але не повний native cleanup/allocation lifecycle. Його стан активний тільки в diagnostic capture. Його не можна прямо використовувати для рішень production або просто зробити always-on без окремого контракту.

Точний контрприклад: EXIT activation A та unrelated activation B мають один receiver. Після setter A відбувається setter B або інший native write; потім writer читає field. Same receiver/thread та cached A не доводять, що writer input походить із A, навіть за numeric equality. Поточні записи прямо залишають `intervening_writes=unknown` і `writer_same_invocation=not_established`. Інший контрприклад — rearm того самого component з тими самими endpoints; pointer/field match без повноти start observations не доводить попередню activation. Cut на тому самому source не має окремого встановленого discriminator. Отже, **activation+receiver+lifecycle — необхідні для цього candidate, але за наявними даними недостатні** для загального production mapping.

### Regression obligations (специфікація, не виконані тести)

| Сценарій | Що має довести майбутня перевірка |
| --- | --- |
| ENTER90/W70, guard false/true | Однаковий own-reference target для однаково admitted invocation, незалежність diagnostics/commit |
| Foreign source, stale generation, stale output sequence | Жодного overwrite через чужий retained target |
| RBX A→B→A | Admission кожного output, без зміни R |
| Unknown raw departure | Немає вигаданого UPDATE або blanket retention; документована fail-closed поведінка |
| Reentry/new ENTER90 при старому target | Немає підміни нової reference старим continuation |
| EXIT native-start70→90,3115 | Provenance та mapping перевіряються окремо від owner; немає policy cliff за однакової authorization |
| EXIT transformed-start106,688→90 | Немає подвійного HorPlus |
| Interleaved A/B, same receiver | B не успадковує mapping A лише за адресою/числом |
| Rearm same endpoints, interrupted blend, new source/cut | Stale activation не authorizes output; cut не згладжується без доказів |
| Diagnostics OFF, overflow/unknown evidence | Production не залежить від записаного diagnostic cache |

Рекомендація: підготувати bounded implementation scope U1-A за вказаними guards та явною межею U1-B; не називати його повним FOV fix чи release-ready. U2 залишити конкретним contract gap invocation provenance/FOV-space та cancellation, а не coverage gap чи вимогою ще одного загального capture. Внутрішню причину creation guard далі не досліджувати без рішення, яке від неї залежить.
