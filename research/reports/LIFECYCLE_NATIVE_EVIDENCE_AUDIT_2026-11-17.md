# Lifecycle Authority: достатність native evidence

2026-11-17. Read-only audit завершено в наведених межах. Концепція Authority
погоджена користувачем; міграція production не дозволена. Дизайн редакції 2
не змінено. Production, тести, налаштування та ASI не змінювалися.

## 1. Висновок для рішення

Наявні hooks дають достатні факти для розбору відомих source-bound recovery,
provider handoff та corrected-input переходів. Виявлені в них дефекти не
доводять потреби в додатковому hook: частина потрібних доказів уже надходила,
але втрачалася або неправильно використовувалася між компонентами.

Водночас НЕ встановлено універсального незалежного Gameplay proof для нового
source без baseline, а також спостережуваного завершення Active Dialogue за
відсутності подальших setter callbacks. Перенесення нинішніх правил до одного
Authority саме по собі не доводить позитивну доступність корекції в цих випадках.

Це не доказ неможливості використати існуючі hooks і не дозвіл додавати нові.
Не вичерпано семантику доступних native об'єктів/їхніх безпосередніх dataflow.
Повноцінну міграцію за результатом цього аудиту не можна назвати обґрунтованою.

## 2. Джерела й межі

Перевірено поточні HookSet, runtime adapters, provider authorization, Gameplay
recovery та Dialogue termination paths. Основні точки:

- `src/hooks/hook_set.hpp`: actual hook surface.
- `src/plugin/runtime.cpp`: `ReplayManualTransition` (6870),
  `TraceDialogueBoundary` (5258), provider producer capture (4195–4271).
- `src/dialogue/post_exit_provider_authorization.hpp`: owner authorization.
- `src/gameplay/gameplay_state.cpp`: `IsNativeHorPlusRecoverySample` (55),
  `ResolveHorPlusRecoveryAction` (105).
- `src/dialogue/dialogue_state.cpp`: `RecoveryRearm::Observe` (289),
  `PostCinematicRecoveryExclusion::ObserveValidated`.
- `src/plugin/dialogue_boundary_transaction.hpp`: resets, handoff and recovery.

Чотири історичні captures зафіксовано SHA-256 у
`../evidence/lifecycle-native-evidence-audit-2026-11-17/corpus-identities.json`.
Вони описують різні попередні збірки, а не runtime останнього ASI.
PID7372 використовував ASI `3D30F28619EFFA64DAAF9256A683BB19C6C2BB8C6760A3113F733B3B368F2262`;
актуальний ASI `67E56485DAE42842331FD034AD1A6B574E63265C69CEF54F0B779513BB714DDE`
у цій роботі не запускався. Початкові меню-зразки 120° не є доказом Gameplay.

## 3. Що насправді бачать hooks

| Межа | Безпосередні native факти | Що ними не доведено |
| --- | --- | --- |
| Primary writer | RSI/receiver, RBX/output, prepared XMM0, aspect, flags; безпечні читання +0x230/+0x234 | Режим Gameplay; оригінальність підготовленого FOV; lifetime об'єкта |
| Cinematic ENTER/EXIT | Виклик конкретного native site, source/аргументи цієї активації | Завершення всієї видимої сцени за кожним EXIT; provenance усіх наступних writer samples |
| Dialogue setter | Вхідний XMM6, component=RSI−0x28, target [RSI+0x2C], receiver RCX, перевірка vtable slot+0x608 | Сам факт справжнього Dialogue замість іншого blend; завершення Dialogue через мовчання callback |
| Provider producer / setter return | Caller site, component, receiver, thread, start/target, повернення setter | Універсальна активність Dialogue; глобальна ідентичність Gameplay |
| Post-store / post-helper | Корельоване значення конкретного output field на названій межі | Остаточний rendered view; відсутність інших writers; engine ownership |
| Settings | Прийнята модом зміна параметрів | Native завершення Dialogue чи початок Gameplay |

Provider eligibility додатково залежить від coordinator, exclusion, recovery
owner/generation/settings. Це модові висновки та історичні зв'язки, а не нові
незалежні native сигнали. `GameplayEligible`, `RecoveryOwned`, Candidate,
baseline.current і `IsOwnerAuthorized` не можна використати як незалежний
оракул правильності тих самих класифікаторів.

`POST_EXIT_BLEND_OBSERVATION` увімкнено звичайною production-збіркою
(`CMakeLists.txt:223`): це не лише логування. Diagnostics OFF не означає
відсутності цього production шляху. Наявність поля в hook не означає, що всі
його виклики/відхилення збережені в логах: є coalescing і обмежене покриття.

## 4. Відповіді на чотири питання

| Питання | Встановлено | Невстановлена частина |
| --- | --- | --- |
| Gameplay нового source без baseline | Writer визначає адресу/shape/числа; наявний recovery може підтверджувати повернення від відомого EXIT для того самого source | Незалежний універсальний Gameplay ownership нового source. Адреса, flags 4/5, +0x234 або збіг FOV цього не встановлюють |
| Завершення без початкового callback | Writer може завершити source-bound Cinematic recovery; новий прийнятий Cinematic lifecycle може відкликати несумісні Dialogue claims | Мовчання setter не є terminal event Active Dialogue. Скасування чужого/старого claim не доводить Gameplay нового source |
| Швидкі reentry/provider/кілька сцен | Є ENTER/EXIT, source-bound recovery, producer→setter→writer chain, corrected witnesses | Повнота callback coverage та наявність сумісного corrected-start witness для кожної активації не доведені |
| Однакові факти, різні потрібні рішення | Одна поточна native проєкція справді недостатня без історії/provenance | Не знайдено двох підтверджених реальних станів із тотожною ПОВНОЮ історією hooks та різними необхідними рішеннями |

Recovery має різні шляхи. `RecoveryRearm::Observe` завершується на першому
валідному sample біля target; stable counter не є нинішнім completion guard.
Якщо setter більше не викликається, саме цей шлях не виконається.
`PostCinematicRecoveryExclusion` має writer-observation шлях depart/return;
Gameplay native recovery може прийняти endpoint одразу після EXIT, без
обов'язкового depart. Для останнього потрібні validatedSource, source match,
читабельність, flags=4 та близькість до прийнятого exitNativeTarget. Одна лише
рівність чисел такого права не дає. Source mismatch може скасувати стару
залежність, але не ідентифікує режим replacement source.

Підтверджений producer/setter/writer handoff дає bounded positive шлях до
Dialogue між катсценами. Він не вимагає глобально відкривати baseline admission.
Однак cinematic ownership і правильна ціль ще не доводять original provenance
кожного кадру: prepared input та corrected-start witness залишаються окремими фактами.

## 5. Відомі проблеми: інформація вже була

У PID7372 о 02:11:48.965 writer tx2259 мав source-owned native completion
з input 130.313°. Coordinator став Gameplay, але downstream використовував
старий `RecoveryOwned`, після чого baseline перестав бути current. Наступний
neutral setter о .968 створив Candidate. Це конфлікт стадій одного рішення,
а не відсутність native completion event.

О 02:12:01.931 інший setter скасував старий Candidate; наступний writer був
допущений до Gameplay перед новим Dialogue descent. Цей запис сам по собі
НЕ доводить правильність admission: CandidateCancelled не створює provenance.

Після реального Dialogue о 02:12:34.234 свіжий Gameplay sample і далі
source-bound current reference вже дозволили writer-retirement neutral Candidates.
Саме поточний native witness дає позитивний доказ, а не відсутність setter.

В інших captures встановлено повторне перетворення prepared Cinematic input,
втрату baseline на empty generation і вплив Candidate на replacement source.
Це класи неправильного використання identity/provenance/termination. Новий
hook не є необхідним поясненням цих конкретних дефектів. Історичні reconstruction
та останні regression PASS не є перевіркою майбутнього Authority.

## 6. Незалежна обмежена перевірка

Research-only `check_evidence_model.js` виконано у V8 isolate без імпортів
production, native callbacks, потоків або I/O моделі. Перебрано 2048 комбінацій
семи facts і чотирьох permissions: 330 узгоджені із заданими обмеженнями,
1718 суперечать їм. Це результати constraint enumeration, НЕ 2048 ігрових
сценаріїв і НЕ PASS повної state machine T01–T36.

Виділено п'ять порушень коротких правил: cancel→Gameplay, нова адреса→Gameplay,
prepared→native, planned→applied, foreign mismatch→retire старого handoff.
Окремо показано відмінність використання entry owner Recovery та фінального
working state Gameplay за умови валідного completion proof.

Десять abstract observation cases відокремлюють визначене рішення за наявності
доказу від альтернатив, які самі по собі не розрізняються. Їхня native
досяжність не доведена; ці приклади не доводять потребу нового hook.
Безпечний PassThrough не означає досягнення необхідного FOV.
Результат: `../evidence/lifecycle-native-evidence-audit-2026-11-17/model-result.json`.

У 2238 збережених POST_PROCESS записах чотирьох captures виявлено 18 груп
однакового receiver/input/nativeField/aspect/flags із різними derived labels.
Наприклад, PID26108 tx404 і tx4181: input/nativeField 130.311°, aspect3.55556,
flags4; мітки GameplayEligible та PostCinematicRecoveryExcluded. Tx9080/9440
мають 121.882° і мітки DialogueLifecycleObserved/TransitionAmbiguous.
Різниця може пояснюватися історією; ці групи НЕ є доказом однакової повної
історії або незалежно підтверджених різних engine modes. Дані та представник
кожної мітки: `native-projection-collisions.json` у папці evidence цього аудиту.

## 7. Точні відкриті вимоги до evidence

1. Для нового source потрібен незалежний native зв'язок із актуальною
   Gameplay camera/context та його lifetime. Camera-shaped pointer і числовий
   sample недостатні. Перевірка найближчого dataflow наявного hook може знайти
   цей зв'язок; відсутність у нинішньому API не доводить потребу нового hook.
2. Для callback-silent Active Dialogue потрібна native termination/replacement
   подія або незалежне спостереження втрати ownership того самого context.
   Timer, silence, FOV equality та чужий receiver цього не замінюють.
3. Для rapid reentry потрібен перевірений source/activation зв'язок і походження
   prepared/original input. Нові generation модового лічильника не є engine
   lifetime proof; EXIT не означає кінець видимої сцени.

Наступне можливе дослідження обмежується цими capabilities на безпосередніх
native paths. Не потрібно автоматично обирати LoadGame hook, новий менеджер,
global executable scan або новий runtime-прохід. До міграції також залишається
окрема незалежна transition model перевірка T01–T36; виконана relation check
її не підміняє. Рішення про наступний етап залишається за користувачем.

## 8. Контроль меж роботи

Десять source-файлів звірено з manifest попередньої design-stage: SHA-256
незмінні (`source-identities.json`). ASI hash звірено окремо. Не запускалися
build/test.cmd/гра/graphics/capture або довгоживучі дочірні процеси. Легкі
читання та extraction завершилися; checker працював у завершеному V8 isolate.
Не змінювалися production, тести чи встановлений мод. Створено тільки research
plan/report, витяги evidence та analysis-only checker. Загальний брудний diff
репозиторію з попередніх робіт збережено.
