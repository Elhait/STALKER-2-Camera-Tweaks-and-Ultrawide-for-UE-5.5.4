# Native dataflow та незалежна transition model

2026-11-17. Обмежений дослідницький етап виконано. Це не реалізація Authority
і не дозвіл на міграцію. Production, production-тести, hooks, INI, дизайн
редакції 2 та ASI не змінено. Збережені проміжні помилки research-моделі
не оголошуються дефектами production.

## 1. Результат для архітектурного рішення

Єдина система прийняття переходів залишається обґрунтованою ціллю. У перевіреній
абстрактній relation немає знайденого порушення перевірених інваріантів у
зазначених кінцевих доменах. Пошук чутливий до відомих класів суперечностей:
навмисні помилки дають короткі counterexamples.

Однак модель не створює відсутніх native фактів. Для нового source незалежний
Gameplay proof не встановлений. Для callback-silent Active Dialogue не знайдено
доведеного terminal ownership proof. Наявний native blend completion — інша
подія, і використання її як Dialogue END було б новим необґрунтованим правилом.

Абсолютного доказу всіх engine-переходів не потрібно. Потрібно окремо назвати
підтримувані позитивні evidence paths, на яких спиратиметься міграція, та
поведінку поза ними. Поточний звіт не надає цим двом прогалинам статус PASS
і не обирає новий hook наперед.

## 2. Ближній native dataflow: конкретні результати

Перевірено adapters writer/setter/provider, класифікацію native caller та
архівні інструкції без повторного сканування executable. Архів прив'язаний до
образу SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
Новий game-image decode або runtime у цьому етапі не виконувався. Ідентичності
прочитаних файлів: [native archive manifest](../evidence/lifecycle-transition-model-2026-11-17/native-archive-identities.json).

### 2.1. Позитивний зв'язок source існує; позитивний Gameplay role — ні

У tick `0xCCE4FA` hook `0xCCE673` має blend-context T в RSI та camera receiver
в RCX. Безпосередньо перед викликом setter native код розв'язує camera weak
reference з T+0x18/T+0x1C: index, serial comparison з object-table item, flags,
потім camera pointer та vtable+0x608. Це сильніший identity/lifetime lead,
ніж порівняння двох сирих адрес. Він може зв'язувати конкретний blend з
receiver; додатковий hook для самого факту такого зв'язку не доведено потрібним.

Але успішне resolution доводить придатність receiver для setter, а не його
роль Gameplay. Цей tick використовується для різних FOV blends. Writer/GetCameraView
також не приносить незалежної ролі: він отримує camera і output pointer,
обчислює/копіює FOV, а `+0x230`, `+0x234`, aspect чи flags не є mode tag.
Розташування weak-reference полів відоме в цьому шляху, але надійна standalone
ASI access/resolution capability та її повнота для всіх source тут не реалізовані
й не перевірені. Engine lifetime generation не підмінюється лічильником мода.

### 2.2. Native completion знайдено, але це завершення blend

Архів `blend-tick.txt` показує:

- `0xCCE679` порівнює XMM6 з target `[RSI+0x2C]` після setter;
- за equality, валідного delegate state та owner переходить до virtual +0x60;
- `target-arming.txt`, `0x31FD92E–0x31FD939`, містить cleanup:
  `[provider+0x2CF8]=0; RET`.

Completion delegate не є безумовним на всіх гілках: перевіряються також
`[RSI+0x10]` та `[RSI+8]`. Крім того, cleanup обриває/прибирає FOV blend,
а не доводить завершення самого Dialogue. Досягнутий target може лишатися
потрібним протягом активного Dialogue. Збіг target або зникнення наступного
setter не стає Gameplay proof. Runtime-доказу terminal Dialogue через цей
cleanup в досліджених captures немає.

### 2.3. Той самий component може обслуговувати наступний blend

У `target-state.txt`, `0x30DB9E5`, provider читає свій `+0x2CF8`.
`0x30DBAC3–0x30DBADA` перевіряє наявний component і порівнює його target
`+0x54` з новим аргументом. Якщо target відрізняється, гілка повертається до
arming `0x30DBCD2` з тим самим component.

Отже, component pointer сам по собі НЕ ідентифікує окремий blend episode.
Це позитивний статичний факт локального шляху, а не припущення про address reuse.
Для зв'язування потрібна прийнята activation/arming history. Caller classifier
в existing hook розрізняє точні return sites ENTER/EXIT/provider, але не всі
можливі engine modes. Новий return-site label теж не дорівнює Dialogue ownership.

### Межа першого напряму

| Потреба | Що реально є | Чого все ще немає |
| --- | --- | --- |
| Новий source | Native weak receiver relation і setter/view dataflow | Незалежний Gameplay role/context proof без старого B |
| Завершення без setter | Native blend completion/cleanup та інші прийняті ownership transfers | Зв'язок cleanup з END саме Active Dialogue |
| Fast reentry / provider | Arming history, exact caller sites, receiver/component/thread relation | Універсальна повнота path coverage; component lifetime як episode proof непридатний |

Наступна невстановлена ланка — durable provider/context role, а не ще одне
число FOV. Безпечний native access до такого контексту на існуючій boundary
може виявитися достатнім; цей аудит не доводить ані його наявності, ані потреби
нового hook. Не робився global Unreal/image search.

## 3. Незалежна модель переходів

[transition_model.js](../evidence/lifecycle-transition-model-2026-11-17/transition_model.js)
— окрема research relation, без production imports. Вона працює з двома source,
scoped Cinematic/Dialogue/Recovery/provider claims, одним Gameplay reference,
prepared/original provenance, settings gates, activation policy та кореляцією
effect observations. Executable permission існує лише в рішенні поточного
виклику; збережений `pending` у моделі — observation correlation record,
не дозвіл повторного native effect.

Oracle assertions відокремлені від reducer і перевіряють:
source scope, baseline provenance/quarantine, відсутність чужого Candidate veto,
terminal completion без repair writer, transfer без проміжного Gameplay,
policy latch, no partial commit, correction gates та correlated effect facts.
Ідентичності component/thread/provider generation мають окремі негативні inputs.

Для всіх T01–T36 є явний handler і рядок
[coverage map](../evidence/lifecycle-transition-model-2026-11-17/transition-coverage.md).
T10/T12/T33 мають відмовну семантику, T32 — observation; T35 — inactive за
source inventory, T36 — excluded legacy/experimental profile. Це не 36 однаково
складних реалізованих native автоматів. `exact`, `valid`, `nativeProof`,
`original` є символічними передумовами. Вони не обчислюються з engine пам'яті.
Матриця явно називає абстракції кожного переходу.

### Виконані перевірки

| Перевірка | Фактична межа | Результат остаточної research relation |
| --- | --- | --- |
| Основний BFS | 42 event variants, Ready root, depth ≤6; 7644 states / 125328 edges | Завершено цей домен, counterexample не знайдено |
| BFS всіх T-family | 121 event variants, depth ≤3; 407 states / 9559 edges | Завершено цей домен, counterexample не знайдено |
| Пошук зі станів після прийнятих переходів | 10 reachable seeds: focused depth3, all-family depth2; 34608 + 19723 edges | Усі задані searches завершено без counterexample |
| Guard fixtures | 7560 calls, 135 окремих component/thread/generation mismatch checks | Без порушення oracle assertions |
| Abort injection | 3780 checks у 7 абстрактних precommit cuts | Жоден staged W не опубліковано |
| Stateful streams | 4 fixed seeds × 2 alphabets × 1200 steps | Без порушення assertions у цих потоках |
| Довга послідовність без LoadGame | 32 події: Gameplay → Cine1 → provider Dialogue → Cine2 → Gameplay → ordinary Dialogue → Cine3 → Gameplay | Перевірено ownership, reference, policy і prepared mapping permissions |
| Effects та identities | Refused/Partial/Unavailable/Argument/Store; revoke→late result; N−1; обидві named readback boundaries | Немає fabricated Applied/permission resurrection або підміни N−1 |

State counts різних searches не сумуються як унікальні engine states.
Числові FOV/formulas/animation і floating-point trajectories тут не перевіряються.
32 події — реконструкція підтримуваного контракту, не executable replay log.
Усі proof preconditions у сценаріях позначають припущення про надані докази,
а не нововстановлені native можливості.

## 4. Пошук коротких контрприкладів

BFS навмисно помилкових variants знайшов:

| Дефект relation | Найкоротший знайдений шлях у відповідному alphabet |
| --- | --- |
| Entry RecoveryOwned після completion | ENTER → bind → writer → EXIT → completion: 5 подій |
| Cancellation створює native admission | Candidate → cancel: 2 |
| Foreign mismatch стирає старий H | ENTER → bind → writer → EXIT → armer → mismatch: 6 |
| Candidate A блокує proven Gameplay B | Candidate A → native-proof writer B: 2 |
| Uncorrelated result стає Applied | Result без кореляції: 1 |
| Частковий W виходить після exception | Aborted admission: 1 |
| Provider quarantine ігнорується наступним writer | Із accepted Armed seed: completion → writer: 2 |

Мінімальність — BFS за кількістю подій у вказаному кінцевому alphabet.
Для останнього прикладу seed фіксований; не стверджується мінімальність усієї
історії від запуску гри. Native reachability цих штучних variants не доведена.

Останній випадок спершу виник у самій незалежній relation, не лише deliberate
fault variant: completion коректно зберігав H, але наступний T28 не перевіряв
provider quarantine. Seeded search це виявив, хоча shallow search від Ready
не знаходив. Relation виправлено відповідно до УЖЕ наявного §10.1 дизайну;
нормативний контракт не змінювали. Збережено red counterexamples і відтворюваний
`quarantine-bypass` variant. Інша проміжна помилка стосувалася надто широкого
oracle для unmatched transfer request; вона також зафіксована окремо.

Це конкретна відповідь на питання про наступну транзакцію: локально правильний
completion недостатній, якщо admission наступної події ігнорує retained claim.
Але це НЕ новий доказ такого production дефекту: production reducer не запускався.

## 5. Що перевірка не доводить

- Не доведена reachability кожного abstract proof у грі. Модель не є Gameplay
  detector і не вирішила дві native evidence прогалини.
- Немає повного перебору довільної довжини, довільної кількості source,
  allocation reuse, memory aliases чи всіх числових trajectories.
- Семантика component/thread/gen mismatch перевірена в абстракції; конкретні
  guards кожного native adapter та policy epoch compatibility не верифіковані
  побітово. Релевантні Gameplay settings abstracted; Dialogue policy latch окремий.
- 7 failure cuts — логічні стадії Authority, а не всі C++ early returns/SEH
  locations. Physical writes, partial memory effects, ABI, platform drain/join,
  actual publication concurrency та diagnostic queue не емулюються.
- T35/T36 coverage означає явне рішення inactive/excluded, не PASS цих профілів.
- Правильність FOV-чисел або плавності у грі не випливає з model checks.

Отже, перевірено переходи та їхні взаємодії в названій абстракції, а не повністю
верифіковано всі production paths T01–T36. Єдиного загального «Authority PASS»
цьому результату не присвоюється.

## 6. Артефакти та незмінність

[result.json](../evidence/lifecycle-transition-model-2026-11-17/result.json)
містить coverage, seeds, counterexample paths, scenario traces та limits.
`provider-quarantine-red.json` і `research-model-red-result.json` зберігають
проміжні невдачі. Для відтворення evaluate research JS у звичайному V8 та
викликати `runTransitionStudy()`; функція не читає файлів, не імпортує production
і не запускає процесів. Фіксовані seeds відтворювані; wall-clock timeout —
лише межа виконання, не engine timing model.

Поточні SHA-256 source/design/ASI записані в
[source-identities.json](../evidence/lifecycle-transition-model-2026-11-17/source-identities.json).
10 попередньо зафіксованих source-файлів незмінні. ASI залишився
`67E56485DAE42842331FD034AD1A6B574E63265C69CEF54F0B779513BB714DDE`.
Build, test.cmd, гра, debugger, decoder, graphics і capture не запускалися.
Checker виконувався в завершуваному V8 isolate; довгоживучих native процесів
не створювали. Зміни — лише research plan/report/evidence.
