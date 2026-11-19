# Lifecycle FOV: реконструкція та межа ухвалення рішень

Дата: 2026-11-17. Статус: технічний висновок для обговорення, рішення щодо
архітектури не погоджене. У цьому етапі production, тести та ASI не змінювалися;
нових перевірок, збірок або runtime probes не запускали.

## 1. Предмет і докази

Основне питання: чому завершений lifecycle може залишити дані, які наступна
транзакція законно за своїми локальними перевірками використає неправильно?
Завантаження збереження — окремий сценарій, не передумова цього аналізу.

Основний запис — PID 7372, 02:09:47–02:13:38, збережений у
`research/evidence/same-save-recovery-completion-2026-11-17-020947/`.
Його ASI: `3D30F28619EFFA64DAAF9256A683BB19C6C2BB8C6760A3113F733B3B368F2262`.
Лог містить 7 969 writer/post-store/post-helper спостережень, три contended-event
втрати, нуль зареєстрованих readback/correlation mismatches. Тому це не буквально
повна покадрова історія. Наведені нижче причинні граничні події присутні.

Додаткове порівняння — PID 13472 у
`research/evidence/multisave-cinematic-baseline-2026-11-17-012536/` та попередній
interactive reentry у
`research/evidence/interactive-cinematic-reentry-2026-11-16-230941/`.

Поточний код уже містить останню локальну правку recovery-класифікації. Зібраний
ASI `67E56485DAE42842331FD034AD1A6B574E63265C69CEF54F0B779513BB714DDE`
має офлайн-перевірку, але його нового runtime тут немає. Не переносимо на нього
ні помилки, ні успіхи старого 3D30F286 без перевірки відповідної зміни.

Межа LoadGame у hooks не спостерігається. Відомість, що наступну сцену користувач
запустив з іншого save, походить від користувача; зміна адрес — окремий факт логу.
Початковий FOV 120 у запуску не використовується як доказ loaded Gameplay FOV.

## 2. Повний відомий шлях останнього запуску

| Час | Подія та ініціатор | Writer / reference / результат |
| --- | --- | --- |
| 02:09:47 | Ініціалізація ASI, налаштування RuntimeCombine / GameplayHorPlus / Reduced | Ще не доказ завантаженого Gameplay |
| 02:11:02 | Перший зареєстрований writer, source A | 120 → 147.796; контекст ігрового save на цій точці незалежно не встановлений |
| 02:11:16.325 | Native writer A подає 130.311 | Gameplay admission, 153.931 output; далі source/identity epoch 1 |
| 02:11:28.830 | Cinematic ENTER g1 | GameplayNativeNumericBaseline 130.311; target 153.931 |
| 02:11:41.457–.463 | EXIT g1, ENTER g2 | Recovery owner з source A; потім нова активація з тією самою ціллю |
| 02:11:42.245–.264 | EXIT g2 → g3 → EXIT g3 → g4 | g3 без writer і без власного recovery owner; reference збережено, g4 target 153.931 |
| 02:11:47.976 | EXIT g4 | Recovery owner A та exclusion; наступні native inputs інтерполюються до 130.311 |
| 02:11:48.965 | Writer tx2259 завершує native recovery | Coordinator уже Gameplay; writerContext ще RecoveryOwned; native input 130.313, output 153.932 |
| 02:11:48.968 | Neutral setter callback | Новий Candidate, start/target 130.311; це не продовження прийнятого Active Dialogue |
| 02:11:48.981 і далі | Writer A | TransitionAmbiguous; reference вже не current; Candidate retirement відмовляє |
| 02:11:53.797 | ENTER g5 — наступна видима катсцена в тому самому save | Старе число 130.311 збережене, але reference unusable; authored 90 → 126.87 |
| 02:11:53.798–58.103 | Native blend / writer g5 | Corrected start 153.931, target 126.87; writer summary підтверджує фінальні 126.87 |
| 02:11:59.076–.079 | Recovery g5, потім neutral setter | Повторюється завершення та створення Candidate |
| 02:12:01.931 | Інший setter/target, старий Candidate скасовується | CandidateCancelled; наступний writer може знову проходити Gameplay admission |
| 02:12:01.944–.973 | Gameplay admission, потім новий Candidate і реальний setter descent до 70 | Reference 130.271; новий Candidate baseline 130.184 стає Active Dialogue; Reduced застосовується; source A |
| 02:12:33.996–34.224 | Dialogue ascent/rearm callbacks | RearmPending → Inactive, active policy звільнено |
| 02:12:34.234 | Перший придатний Gameplay writer після Dialogue | Fresh reference 130.303, writerContext GameplayEligible, output 153.926 |
| 02:12:34.238–.260 | Два neutral Candidate та наступні writer | Retirement спрацьовує двічі: тепер наявний current reference |
| 02:12:51.276 | Receiver/output A → B, epoch 1 → 2 | Старий source не підміняє новий; надалі B отримує власний reference |
| 02:13:06.478 | ENTER g6 після завантаження іншого save за повідомленням користувача | B, fresh 130.311, target 153.931 |
| 02:13:17.618–21.910 | EXIT/ENTER g6 → g7 → writer-empty g8 → g9 | Reference і target 153.931 збережені |
| 02:13:28.961–.964 | Recovery g9 → neutral Candidate | Та сама стара completion-класифікація повторюється; наступної катсцени до кінця логу немає |

Останній рядок показує повторення граничної моделі, але не є доказом ще однієї
видимої поломки: наступну сцену в цьому записі не спостерігали.

## 3. Чому успішний і невдалий шляхи відрізняються

| Властивість | Cinematic recovery → Candidate: невдача | Звичайний Dialogue recovery → Candidate: успіх |
| --- | --- | --- |
| Чи є повернення в Gameplay | Так | Так |
| Чи створюється neutral Candidate | Так | Так, двічі |
| Чи сам FOV Candidate неправильний | Ні: близько 130.311 | Ні: близько 130.31 |
| Перший придатний reference до Candidate | Немає: completion sample подано як RecoveryOwned | Є: writer 02:12:34.234 опублікував GameplayEligible |
| Чи може retirement використати current reference | Ні | Так |
| Наслідок | Блокування admission, authored fallback наступної сцени | Candidate завершується, наступні references придатні |

У PID 13472 після заміни receiver збережений Candidate теж блокував новий
admission. PID 7372 підтвердив, що source replacement не є необхідною умовою:
та сама відмова виникла на source A в одному save. LoadGame не пояснює цю різницю.

Цикл також зміг вийти з блокування о 02:12:01.931 завдяки новому setter, який
скасував старий Candidate. Це залежність від випадкової наступної події, а не
доказ коректного завершення старого контексту. У проміжку до нового accepted
Dialogue writer допускає native 130.271 як Gameplay. Тут значення близьке до
початкового й видимої помилки не доведено; для моделі окремо треба визначити
допуск після CandidateCancelled, якщо наступний sample уже належить іншому blend.

### Простори FOV у цьому шляху

| Факт | Що відомо | Чого не можна припускати |
| --- | --- | --- |
| Gameplay reference 130.311 | Прийнятий source-bound native-register sample; HorPlus 153.931 | Що будь-який наступний camera+0x230 з тією самою адресою має ту саму provenance |
| Cinematic authored 90 | Оригінальний capture на ENTER; обрана policy може використати Gameplay reference замість цього authored reference | Що 90 є актуальним Gameplay FOV користувача |
| g2/g3/g4 writer/native blend starts | Лог бачить 130.311/151.783 та підготовлену target 153.931; corrected-start witness у наведених captures Unavailable | Що кожен такий sample можна знову HorPlus-трансформувати як fresh native |
| g5 start/target | Native-start capture 130.311, corrected start 153.931; target 126.87 уже підготовлена модом із authored 90 | Що правильний start гарантує правильність кінцевої policy/reference |
| Dialogue target 70 і setter result | Callback capture до рішення та результат Reduced належать різним стадіям; writer може отримати prepared sample | Що NativeRegisterInput означає «гра не читала жодного попереднього результату мода» |
| Actual After | Корельований post-store/helper readback на відомій точці | Що це глобальний rendered FOV або доказ повного writer coverage |

## 4. Перша доведена точка порушення

Для невдалого другого Cinematic це tx2259, 02:11:48.965, ДО нового Candidate:

1. Adapter уже захопив вхідну класифікацію RecoveryOwned.
2. Recovery-частина того самого виклику підтвердила native endpoint і прийняла Gameplay.
3. Gameplay dispatch отримав новий coordinator та стару input-класифікацію.
4. BaselineStore сприйняв sample як непридатний для Gameplay admission і зробив
   той самий reference неактуальним.
5. Після повернення callback наступний setter побачив уже стійкий наслідок.

Це не підтверджена міжпотокова гонка. ENTER, EXIT, writer і Dialogue callback
виконуються під спільним CameraCorrectionGate (`runtime.cpp:4384,4685,5262,6894`).
Він захищає порядок callback-ів, але не виправляє логічно застарілу локальну копію
всередині одного callback. Потрібна узгодженість прийнятого рішення, а не ще один mutex.

Також неправильний універсальний інваріант «coordinator == writerContext»:
CoordinatorState взагалі має лише Gameplay/CinematicActive/CinematicExiting.
Active Dialogue обробляється всередині Gameplay pipeline, а його samples не
повинні через це автоматично стати GameplayEligible. Після writer-triggered
Dialogue reset поточний sample може ще містити setter-корекцію. Такий sample
має право залишатися excluded навіть після звільнення Dialogue ownership.

Правильне питання: чи узгоджені після транзакції **прийнятий ownership, походження
саме цього sample та рішення щодо baseline**? Вони не зводяться до однієї enum.

## 5. Карта чинних повноважень

| Вузол | Реальна відповідальність | Ризик на межі |
| --- | --- | --- |
| RuntimeState | Єдиний фізичний owner lifecycle/settings/resources | Саме по собі спільне зберігання не дає спільної семантики рішень |
| Runtime writer adapter | Збирає input, класифікує Dialogue/recovery ДО транзакції | Класифікація може застаріти в цій транзакції |
| CinematicLifecycleTransaction | Приймає ENTER/EXIT, generation, selection, recovery transfer | Технічна generation не тотожна строку довіри до Gameplay reference |
| ProcessCameraWriter/recovery | Підтверджує owner/source, завершує recovery, вибирає вихідну policy | Не всі гілки однаково повертають остаточну admission-семантику |
| DialogueBoundaryTransaction | Candidate/descent/rearm, policy capture та setter correction | Candidate може потребувати іншої boundary для завершення; phase не є доказом відношення до всіх writer-ів |
| GameplayBaselineStore | Перевіряє sample, зберігає native/reference/source/current | Зараз частково виводить втрату довіри з coarse writerContext, а не лише з явної причини invalidation |
| CinematicEnterActivation | Доказ походження corrected start та mapping | Доступність числа не доводить native/ownership; потрібен activation/source witness |
| Camera State / diagnostics | Публікує вже отримані факти | Не має ставати другим arbiter або реконструювати missing native |

Не знайдено підтвердження кількох незалежних RuntimeState. Точніше формулювання
структурної проблеми: рішення розподілені між стадіями, а проміжна класифікація
інколи використовується як остаточний дозвіл/заборона після зміни lifecycle.

## 6. Гіпотези щодо спільної причини

| Гіпотеза | За | Проти / межа | Висновок |
| --- | --- | --- | --- |
| Неправильна HorPlus/Reduced математика | Неправильне видиме число | Те саме native 130.311 успішно дає 153.931; помилка виникає при виборі authored reference 90 | Не пояснює цей дефект |
| Потоки змінюють coordinator між перевірками | Значення розійшлися | Gate серіалізує callback-и; конкретний шлях пояснюється всередині одного виклику | Не доведено як причина |
| Candidate — єдина першопричина | Після нього admission заблоковано | Reference уже не current до його створення в tx2259 | Candidate — другий необхідний механізм утримання помилки, не перша причина цього випадку |
| Будь-який неоднозначний sample означає застарілий reference | Чинна fail-closed політика | Незавершений/завершений transition, втрата source та непридатність sample мають різну семантику | Контракт потребує окремої оцінки, а не безумовного збереження чи invalidation |
| Немає єдиної межі остаточного рішення для writer/admission | Entry classification переживає transition; аналогічно carry впливав на baseline, prepared значення — на native transform | Окремі source/epoch/association owners працюють правильно; це не доказ необхідності нового глобального coordinator | Найсильніший напрямок архітектурного рішення для обговорення |

Ці дефекти мають спільний клас змішування ідентичності/стадії/авторитетності,
але не доведено, що одна зміна автоматично усуне кожен із них.

## 7. Суміжні переходи, які модель має пояснити

| Шлях | Поточна поведінка / що перевірити | Не можна робити автоматично |
| --- | --- | --- |
| Source-owned native recovery completes | Останній patch виправляє відомий completion path; новий runtime відсутній | Перевести будь-який RecoveryOwned sample в GameplayEligible |
| Recovery owner retired через source replacement | Coordinator може змінитися раніше за input context; новий source потребує власного evidence | Успадкувати baseline старого source |
| Exiting без recovery owner | Cleanup змінює coordinator й повертається без звичайного Gameplay projection | Вважати, що такий callback уже зробив fresh admission |
| AspectRecalculation recovery endpoint | Окрема гілка завершує lifecycle і повертається раніше Gameplay dispatch | Вимагати однакової кількості writer/callback між усіма режимами |
| Provider handoff в реальний Dialogue | Native endpoint може бути 70, а не Gameplay; є source/component/thread/generation guards | Допускати provider endpoint у Gameplay лише через завершення cinematic recovery |
| Active Dialogue reset усередині writer | Поточний input може бути setter-corrected; його exclusion інколи необхідний | Повторно класифікувати всі inputs за новою phase без provenance |
| Writer-empty EXIT/ENTER | Baseline, continuation ownership та corrected-start witness мають окремі строки життя | Очищати все на EXIT чи прирівнювати carry до writer authority |
| Already-corrected native blend start | Prepared target не стає native від нового ENTER | Ще раз застосовувати HorPlus за числовою схожістю |
| Невідомий/втрачений source або відсутній callback | Немає достатніх доказів нового ownership | Назвати timeout чи FOV equality завершенням камери |

Це перелік пов'язаних contract surfaces та прогалин перевірки, не список дев'яти
нових доведених production-багів.

## 8. Варіанти рішення — ще не реалізація

**A. Залишити локальні reconciliation guards.** Найменший diff, наявний 67E56485
вже реалізує один такий guard. Ризик: кожна рання return/transition гілка знову
мусить вручну пам'ятати про всі споживачі старої класифікації. Це виправляє
доведений шлях, але не виключає клас помилок за конструкцією.

**B. Остаточне рішення щодо writer/admission у наявній транзакції.** Adapter
передає початкові факти та entry context; після lifecycle-рішень ProcessCameraWriter
формує єдиний остаточний контракт: хто авторизував обробку цього sample, яка
його provenance, що робити з baseline — Update/Retain/Invalidate із причиною.
BaselineStore зберігає захисну валідацію та єдине зберігання reference, але не
повинен незалежно відновлювати lifecycle із застарілої entry enum. Публікація
отримує це саме рішення. Це можуть бути поля/дозволені шляхи існуючих result-ів,
не обов'язково нова структура чи фізичний snapshot.

Ризики B: потрібна явна семантика ВСІХ ранніх return та правильне збереження
поточного setter-corrected input як excluded після reset. Не можна зробити
«refresh writerContext за coordinator» і назвати це реалізацією B. Lifecycle
commit тут означає прийняте production-рішення під gate; фактичний output store
і readback залишаються пізнішими окремими фактами. Diagnostics не стає умовою commit.

**C. Нова спільна машина станів усіх режимів.** Дозволяє сильніше централізувати
владні рішення, але має найбільший обсяг зміни й ризик стерти необхідну різницю
між setter, writer, provider та native blend. Поточні докази не вимагають нового
глобального owner. Розглядати лише якщо B не може описати потрібні дозволені
переходи без прихованого другого arbiter.

Рекомендація для обговорення — B. Спочатку погодити модель переходів і
незалежні admission/provenance інваріанти. Це рекомендація, не прийняте рішення
й не дозвіл одразу починати рефакторинг.

## 9. Модель перевірки перед наступною реалізацією

Потрібен bounded generator подій через реальні production entry points:
ENTER/EXIT, aspect association, raw setter, provider armer/setter, writer,
post-store/helper, settings mutation, receiver/output replacement. Зберігаються
реальні причинні залежності; неможливий порядок теж тестується як reject path,
а не як вимога завжди застосувати корекцію.

Обов'язкові властивості:

1. Завершений owner не дає подальших дозволів; retained metadata не є authority.
2. Придатність поточного sample та актуальність prior reference — різні рішення.
3. Fresh Gameplay admission має native provenance та актуальний source; чужий
   baseline не відроджується від правильного числа.
4. Prepared output не стає новим native reference через generation/phase change.
5. Neutral Candidate не може без доказу конфлікту керувати іншою камерою.
6. Кожен підтверджений terminal event залишає стан придатним для наступного
   дозволеного callback без вимоги додаткового «ремонтного» Gameplay writer.
7. Вставлення зайвого нейтрального writer з тими самими input/source/settings
   після завершення не повинно міняти reference origin або policy наступної сцени.
   Порівнювати обидва порядки, а не лише один happy path.
8. Diagnostics OFF/ON дають ті самі production-рішення; readback може бути
   Unavailable, але не підмінюється Planned Output.

Окремо розрізняти safety та liveness: відсутність ownership evidence не дозволяє
вигадати новий Gameplay, але тоді ambiguity повинна мати конкретний scope/reason.
Не можна довести безумовне завершення невідомого engine lifecycle лише з тиші
callback-ів. Це межа спостереження, а не підстава для глобального veto чи таймера.

Генератор має варіювати нуль writer між подіями, writer-empty generations,
нейтральні callbacks після completion, source interleaving, зміни FOV/aspect/settings
та справжній provider Dialogue. Кожна знайдена мінімальна послідовність зберігається
як постійна регресія. Не вставляти ResetDialogueBoundaryState у fixture для
імітації відсутньої production-події і не вважати це перевіркою lifecycle.

Попередні 32 seed/23 transitions переважно перевіряли Cinematic continuity, а
новий довгий сценарій спочатку мав зайвий writer перед setter. Тому кількість
seed/62 harness сама по собі не гарантувала покриття цієї взаємодії.

## 10. Межа наступного кроку

Зараз завершено реконструкцію та підготовлено варіанти для обговорення. У плані
зафіксований user gate: ніяких наступних production/test змін чи ASI до вибору
рішення користувачем. Поточні правила збереження режимів, формул, source guards,
налаштувань та окремої readback-доказовості залишаються чинними. LoadGame не
додається до P0 як обхід завершення звичайних lifecycle.
