# Cinematic FOV authority — план реалізації для Luna

Дата: 2026-11-13. Статус після додаткового аудиту: **BLOCKED — ADDITIONAL EVIDENCE REQUIRED**. Не READY FOR IMPLEMENTATION. Нижче наведено конкретизоване доповнення, яке має пріоритет над попередніми альтернативами migration та відкритими implementation choices. Production-код у фазі аудиту не змінено.

Незалежні актуальні статуси (§10.5): **ENTER selection — ROOT CAUSE CONFIRMED; U1-A — BOUNDED CONTRACT CANDIDATE; U1-B — UPDATE PROVENANCE UNKNOWN; U2 — RECOVERY UNRESOLVED**. Узагальнений BLOCKED стосується повного безпечного FOV fix; він не забороняє підготовку обмеженого ENTER retention scope. Повторний загальний аудит selection не потрібний.

Фактологічне джерело: [CINEMATIC_FOV_AUTHORITY_AUDIT.md](../reports/CINEMATIC_FOV_AUTHORITY_AUDIT.md). Перед роботою прочитати workspace `agent-rules/AGENTS.md`, чинні ARCHITECTURE/SAFETY_INVARIANTS/SUPPORTED_BUILD_MANIFEST, перевірити current working tree і зберегти сторонні зміни. Не виконувати Git mutations. Робоче дерево вже має staged RSI/RBX admission fix і unstaged diagnostics; не відкотити їх як частину цієї задачі.

## 1. Root cause і очікуваний результат

`SelectCinematicBaseline` (`src/cinematics/cinematic_fov.cpp:345`) у GameplayHorPlus віддає перевагу latest eligible Gameplay native baseline. `ProcessAcceptedCinematicEnter` (`src/plugin/cinematic_lifecycle_transaction.hpp:160`) передає його як targetBaselineFov у cinematic transform. Runtime gen2: authored ENTER=90, baseline=70, output=108.941; 1169 admitted writer samples утримували цей output. Для own native 90 при 32:9 очікується приблизно 126.87. Числа є прикладом, не умовою production-коду.

Власний native cinematic reference нової generation повинен визначати корекцію. Lifecycle визначає режим незалежно від FOV. Foreign Gameplay/Dialogue samples не можуть підміняти reference. Виправлення ENTER не означає автоматичного закриття окремого EXIT cliff 90.308→127.116 або доведення першого rendered кадру.

## 2. Target architecture — варіант C

Використати existing Runtime owner, correction gate, effective selection, generation та CinematicWriterAssociation. Authored ENTER уже зберігається в `enterNativeFov` і association.authoredEnterFov; corrected target має окремі representations. Узгодити їх publication в одній serialized transaction. За потреби додати невеликий typed reference record у чинний owner: native value, validity, origin boundary, generation, reference revision, confirmed source/binding status. Не створювати незалежний глобальний manager.

Інваріанти:

1. Кожен accepted ENTER незалежно оцінює власний input навіть за числової рівності з попередньою generation.
2. ENTER input може бути provisional до aspect-store/source binding. Provisional reference не авторизує foreign writer.
3. Qualified native UPDATE змінює reference лише у чинних generation/source. Ordinary writer input, semantic equality і temporal proximity не є достатнім доказом UPDATE.
4. Corrected target походить тільки від qualified native reference, validated aspect та active effective policy. Cached transformed value ніколи не подається як native reference.
5. Per-invocation admission залишається generation/source/output/sequence-bound. Source conflict invalidates; output A→B→A не invalidates source.
6. Повторний input/aspect/policy revision не створює зайвих state publications. Не пропускати необхідний writer store лише через числову рівність із минулим output: native код може його перезаписувати.
7. EXIT snapshots актуальний confirmed corrected endpoint перед очищенням active state; recovery працює з current writer, не розіменовує історичний RBX.
8. Наявність Diagnostics не змінює authority, формулу, lifecycle або результат.

## 3. Change map

| Файл / функція | Передбачена зміна |
| --- | --- |
| `src/cinematics/cinematic_fov.hpp/.cpp`: selection, association, initial retention, dispatch | Own-reference selection; розділити qualification writer output і qualification нового native reference; прибрати gameplay numeric reference як target authority |
| `src/plugin/cinematic_lifecycle_transaction.hpp`: accepted ENTER, aspect store, accepted EXIT | Coherent native/target generation state; qualified updates; reset/invalidation; чинний EXIT snapshot |
| `src/plugin/camera_writer_transaction.hpp`: CinematicActive section:1623 | Зберегти output admission; змінити baseline-specific retention/continuation assumptions; повертати observability результат із reference origin/revision |
| `src/plugin/runtime.cpp`: TraceCinematicEnter, ApplyCinematicAspectStore, writer adapter, settings integration | Передати boundary evidence до transaction; bounded logs; перерахунок effective aspect у межах active selection |
| `src/gameplay/gameplay_state.*`, recovery transaction helpers | Спочатку characterize. Змінювати лише доведено порушений handoff після нового endpoint; не розширювати numeric classifier заради fixture |
| `src/config/config_template.cpp`, `feature_config.*`, Overlay labels/locales, README/config docs | Явно узгодити semantics legacy GameplayHorPlus із own-reference контрактом; зберегти parsing старих INI та задокументувати migration |
| `src/camera/camera_state_publication.hpp`, cinematic observation consumers | Публікувати own native/target окремо від retained Gameplay; UI observation не стає authority |
| Tests, registry, docs contracts | Actual lifecycle/writer adapter cases, policy semantics, provenance та negative cases; registry тільки за потреби нових cases |

## 4. Етапи й критерії приймання

### Етап 0 — зафіксувати evidence і semantic delta

Зберегти копію актуального game log у новому task evidence каталозі з SHA-256; CDB transcript уже окремий файл в workspace `02-Research/evidence/downstream-runtime-20261112/cdb-live.log`. Занести current HEAD, staged/unstaged production diff, hashes ASI/EXE/config, коли файли доступні. Не відновлювати відсутній installed ASI hash із попередньої розмови.

Прочитати config contract: GameplayHorPlus обіцяє matching з Gameplay; NativeHorPlus — authored cinematic HorPlus. Новий контракт own-reference змінює колишню numeric matching policy. У документації явно описати, які старі налаштування матимуть нову поведінку. Якщо обидва modes після зміни еквівалентні, не вигадувати штучну відмінність: зберегти backward parsing і задокументувати compatibility alias/deprecation як конкретне рішення до реалізації. Якщо зберігається окремий matching mode, дозволений лише independently qualified Gameplay reference; latest baseline fallback заборонений.

Приймання: scope і semantics описані; acceptance oracle для own-reference не залежить від 70/90; regression cases із custom Gameplay FOV передбачені. Архітектурний refactor не починається, поки цей semantic delta не сформульований.

### Етап 1 — ENTER reference і generation ownership

Реалізувати own native ENTER reference через existing state. Від'єднати target calculation від GameplayBaseline value. Залишити Gameplay baseline для його власних задач. Перевірити всі читання preEnter*, gameplayContinuationSource, association.gameplayBaseline*: кожне або лишається явно historical/qualification evidence, або замінюється own-reference даними; заборонений fallback target=latest Gameplay.

Не обмежувати diff `SelectCinematicBaseline`: реальний writer після ENTER може залишитися на native 70, коли ENTER був 90. Наявна generation/source/output authorization має дозволити необхідне встановлення own cinematic target, не оголошуючи 70 новим authored input. Підтвердити retention contract на native boundary evidence з аудиту provenance; якщо можливість відрізнити old native state від справжнього authored UPDATE не встановлена, зберігати ENTER reference і явно позначати UPDATE unavailable.

Приймання: однаковий ENTER input/source/aspect із довільними попередніми Gameplay/Dialogue FOV дає однакові cinematic target та reference provenance; 90→126.87 є одним прикладом. Foreign output/source, invalid target, stale generation не отримують correction. Actual adapter перевіряє XMM0 і original store, а не лише pure helper.

### Етап 2 — UPDATE authority evidence gate

Не перетворювати `CinematicActive + valid float` на native provenance. Дослідити наявні ENTER/shared consumer/native setter data-flow та diagnostic captures. Визначити існуючу boundary, яка передає authored cinematic зміни всередині generation, і її source/generation relation. Якщо таких доказів немає, цей етап потребує окремого targeted observation і не може отримати PASS за синтетичним тестом.

План targeted observation, тільки після окремого дозволу runtime/інструментування:

- Використати known lifecycle і writer PRE/POST; простежити candidate authored update callsite в статично підтвердженому producer, без global downstream correction.
- Для кожної події зберігати generation, source, sequence/thread, boundary address, raw argument до модифікації, reference revision, raw writer sample та active policy. Corrected endpoints зберігати окремо.
- Контрольні сценарії: authored FOV change без ENTER; ordinary native blend/copy без authored change; replay corrected input; source mismatch. Нуль events не доводить відсутність native UPDATE.
- Критерій: producer/path відрізняє authored changes від ordinary writer samples і прив'язується до чинного source. Числовий збіг, pointer і timestamp окремо недостатні.
- Budget: один bounded capture до 60 секунд, обмежений число записів; тільки одноразові/подієві точки. Поточний досвід показав 0 FPS від conditional software breakpoints на projection hot path — таку схему не повторювати. Cleanup і detach перевіряти; гру не terminate.

Після підтвердження додати qualified update transaction у чинний owner. Якщо доказів бракує, зупинити лише dependent UPDATE implementation, залишивши чіткий incomplete criterion; не оголошувати весь контракт реалізованим.

### Етап 3 — aspect, invalidation і continuation

При validated effective aspect change перерахувати target із qualified native reference, узгоджено оновити всі target representations і publication. Auto aspect може змінюватися без нового ENTER; forced/native policies мають власні правила. Active settings залишаються lifecycle-scoped; requested mode не має випадково застосуватися всередині generation.

Повторний ENTER обробляється existing selection rules; новий accepted lifecycle завжди має власний reference. Перевірити baseline-specific reentry fallback та continuation interpolation: старий cached output не може скасувати новий authored intent. Зберегти admission для кожного output; source change invalidates і вимагає нового підтвердження, а не автоматичного numeric rebind. Invalid/missing reference не замінюється Gameplay/Dialogue або output field.

Приймання: coherent target після aspect update; raw native reference не змінено HorPlus-result; stale source/generation/revision не впливає; A→B→A outputs працюють; source/target conflict fail closed; same-valued new generation переоцінена незалежно.

### Етап 4 — EXIT/recovery і evidence publication

Snapshot own corrected target на EXIT перед reset. Перевірити native exit target та continuation carry для empty/interrupted generations, незалежність Dialogue exclusion. Не підміняти native recovery input corrected endpoint. Відтворити стару траєкторію 106.688→90 і нову 70→90.3115; перевірити, чи власний новий cinematic endpoint узгоджується з діапазоном існуючого recovery. Native timing/easing не змінювати без окремого доказу.

Якщо існуючий EXIT cliff зберігається, задокументувати його як окремий незакритий дефект; виправлення ENTER не дозволяє заявляти no-regression лише через відсутність змін classifier. Будь-який новий regression від target change блокує випуск цієї реалізації до усунення залежності.

Приймання: actual EXIT transaction captures latest qualified target; recovery owner generation/source правильний; exclusion має чинний release; no stale continuation; Gameplay enabled/disabled та всі Gameplay modes залишаються незалежними.

## 5. Regression matrix

| Сценарій | Обов'язковий oracle |
| --- | --- |
| Gameplay→Cinematic, arbitrary prior baseline | Native origin=ENTER; new generation; own transform; current source/output admission |
| Cinematic1→Dialogue→Cinematic2 | Dialogue sample не змінює cinematic reference; own ENTER 90 окремий від writer70; output≈126.87 при32:9 |
| Дві катсцени з однаковим FOV | Різні generations/revisions; без stale inherited target |
| FOV update без ENTER | Потрібний independently qualified UPDATE; правильний native input, а не output або blend |
| Невідомий update / invalid float / delayed reference | Native pass-through для неавторизованого invocation; valid ENTER reference не перетворюється на довільний sample |
| Same native FOV repeated | Немає нового reference revision; necessary writer retention зберігається |
| Corrected sample на вході | Немає другого HorPlus; classification не базується лише на equality |
| Aspect 16:9→21:9→32:9, Auto/forced/Native | Own reference незмінний; target відповідає effective aspect; aspect/FOV snapshot узгоджений |
| Output A→B→A / B до EXIT | Source association живий; кожен output admitted; EXIT historical output актуальної sequence |
| Foreign source / target conflict / address reuse | Без stale rebind чи чужого cached target; new generation invalidates старе evidence |
| OFF / settings change in-flight / Diagnostics OFF | Existing gate semantics; без нових post-OFF correction; ON/OFF diagnostic math однакова |
| EXIT recovery 106.688→90 і70→90.3115 | Перевірити input space, owner, output policy, completion, exclusion; не маскувати discontinuity |
| Empty generation / duplicate EXIT / interrupted reentry | Correct cleanup і обмежений carry; без чужого reference |
| Gameplay custom FOV, Gameplay disabled, all modes | Явно очікувана нова cinematic policy; власні Gameplay semantics збережені |
| NativeHorPlus та legacy GameplayHorPlus INI | Parsing і migration відповідають documented decision; жодної прихованої зміни назв |

## 6. Validation strategy

Спочатку pure/helper tests та actual transaction/adapter tests: `cinematic_fov_harness`, `cinematic_lifecycle_transaction_harness`, `runtime_native_adapter_harness`, `camera_writer_transaction_harness`, `module_ownership_harness`; далі `cinematic_exit_authority_harness`, `horplus_post_exit_recovery_harness`, `coordinator_recovery_harness`, `post_cinematic_exclusion_harness`, `recovery_rearm_harness`, selection/aspect та camera snapshot cases. Перевірити actual registry `tests/test_harnesses.json` і supported runner invocation перед запуском. Старі matching expectations треба переглянути як semantic delta, а не механічно видаляти negative cases.

Фіксувати input origin/boundary, generation/source/output/sequence, effective mode/aspect, decision/revision, XMM0 до/після та native POST store. Replay runtime fixture не доводить нового native UPDATE producer. Source-level provenance перевірки й реальні adapter assertions важливіші за одну фінальну цифру.

Після пов'язаних фаз — один canonical `test.cmd -NonGraphics` (коли він дозволений), bounded sequential runs із process cleanup. Потім одна Release build, SHA-256 та release contents inspection. Не створювати окремого diagnostic ASI як необхідної умови: звичайний ASI з діагностикою через INI, зі звичною назвою `STALKER2CameraTweaks.asi`; не встановлювати автоматично.

Runtime validation потребує окремого дозволу: один combined сценарій Gameplay→Cinematic→Dialogue→Cinematic→Gameplay; зафіксувати EXE/ASI SHA, settings/aspect, visual event marker і game log. Довести own ENTER reference, admitted writer store та, за наявності точного capture, downstream consumption. Не називати ручні CDB labels lifecycle evidence; не вимагати постійних projection breakpoints. Visual absence/presence дефекту та numerical output — різні claims.

## 7. Failure і rollback

Відсутній або невалідний native reference: не використовувати last Gameplay FOV, dialogue constant чи cached corrected value як заміну. Неавторизований writer проходить native. Раніше прийнятий власний reference може утримуватись лише у ще валідній generation/source за явними retention правилами; source conflict скасовує authority. Delayed reference promotion дозволений лише від qualified boundary.

Перед реалізацією зберегти попередній ASI й SHA, конфігурацію та scoped diff snapshot. Безпечне повернення binary — закрити гру штатно і вручну повернути збережений ASI/config; live unloading не використовувати. Source rollback — тільки scoped inverse patch конкретної реалізації після review, зі збереженням staged/unstaged чужих змін; не `reset/restore/stash`. Новий unsafe fallback або runtime перемикач latest-baseline behavior не є rollback стратегією.

## 8. Відкриті питання й завершення

Не доведено authored UPDATE producer у межах generation, потребу/безпечний native contract source switching та відповідність першого projection capture першому rendered кадру ENTER #2. UPDATE evidence gate обов'язковий для заяви про повний контракт. Підтверджений ENTER root cause можна реалізовувати незалежно після прийняття плану й дозволу implementation scope; цим документом production edits не виконуються.

Закриття: усі рядки regression matrix мають PASS, accepted limitation або окремий погоджений follow-up із evidence; неперевірені UPDATE claims не отримують PASS. Звіт має назвати artifact/hash, фактичні checks, runtime result і залишковий EXIT cliff. Без нового runtime не заявляти «візуальний дефект виправлено». Не змінювати антивірусні або суміжні архітектурні механізми в цій задачі.

## 9. Уточнення після додаткового native audit — обов'язкове для handoff

### 9.1 Остаточні рішення та незакриті gates

Залишається варіант C: Runtime owner / existing correction gate / accepted generation / existing aspect-target association / per-call writer admission. Новий global FOV owner не потрібен. Уточнення нижче є планом, не виконаною production-зміною.

- ENTER native input має власну boundary authority; source binding deferred до existing aspect/writer relation. ENTER RSI не переносити як writer RSI.
- Lifecycle не визначається float values. Generation, writer identityEpoch і blend activation epoch — різні сутності.
- UPDATE має бути подією qualified native producer; сама source ownership авторизує output destination, але не робить raw sample authored input. Це **gate U1**, який ще не закрито.
- Unknown input не виправляється latest Gameplay/Dialogue reference або corrected cache. Немає погодженого production способу одночасно утримати own126.87 при raw70 і гарантувати authored UPDATE через той самий writer без evidence producer. Luna не повинна придумувати цю policy під час implementation.
- Source/target conflict invalidates; автоматичний rebind у scope не входить. RBX A→B→A лишається допустимим за кожним invocation.
- New target змінює EXIT recovery geometry. Власний endpoint126.87 при native-start70→90.3115 не усуває interval gap; потенційний додатковий visual jump на початку recovery потребує evidence. Це **gate U2**.

Усі constraints на Gameplay.Mode=RuntimeCombine зберігаються. Зміни Cinematics.FovMode не є зміною рекомендованого Gameplay mode.

### 9.2 Конкретна config/UI migration

Обране для review рішення: canonical `Cinematics.FovMode=NativeHorPlus`, що використовує own qualified cinematic reference. Legacy `GameplayHorPlus` та parser alias `MatchGameplay` читаються як alias до цієї policy. На наступному звичайному save canonical value=`NativeHorPlus`; саме load не переписує INI. Один bounded migration notice. Старий Gameplay numeric matching припиняється явно за погодженим own-reference контрактом; не описувати це як збереження його старої семантики.

UI більше не показує два однакові choices. Зберегти одну cinematic own-reference policy/readback; FOV-cycle control прибрати. Deprecated FOV-cycle hotkey/config key приймати без помилки, виконувати no-op з bounded notice; aspect cycling лишається незалежним. Existing locale keys можна зберегти, але visible текст, default config, README/release notes і RuntimeSettings readback повинні відповідати canonical policy. Не додавати новий gameplay-matching mode в цьому scope.

Тести migration: усі old/new spellings→одна effective policy; round-trip штатного save; unknown value→documented default; custom Gameplay FOV не змінює own cinematic target; native aspect/OFF не активують HorPlus. Відсутність mutation Gameplay.Mode перевірити явно.

### 9.3 Aspect transaction — точна схема

Додати до existing owner reference revision + effective aspect revision/validity, якщо наявних typed records недостатньо. Не копіювати всю state machine в окремий store.

Спільний helper target reevaluation викликається під correction gate з `TraceCinematicEnter`, із accepted aspect store після binding і перед admitted cinematic writer dispatch. Він використовує **active latched** policy і validated own native reference. Auto aspect отримувати через existing viewport resolver; forced policy через ResolveAspect. Порівняти effective aspect/revision, а не raw requested settings. При invalid domain — fail closed для відповідної mutation, без нового fallback target. Native/OFF має чинну native pass-through семантику.

Єдиний commit оновлює Runtime transformedFov, association.targetFov, effective aspect та observational target metadata/revision. Gate захищає весь read/compute/commit, association mutex береться в established order; не викликати helper рекурсивно з уже утриманим association mutex. No-op reevaluation не означає skip необхідного writer overwrite. EXIT snapshots останній committed endpoint/revision до cleanup.

Acceptance: actual adapter виконує ENTER→aspect change→writer→EXIT; усі representations збігаються за revision, own native reference не змінено; concurrent OFF/requested mode mutation зберігає чинний gate/latching contract. Early aspect callback без valid reference не створює authority; перший admitted writer може виконати aspect commit після готовності reference.

### 9.4 Evidence phase перед full implementation: конкретні boundaries

Дозвіл на production fix, game, debugger чи нові hooks у поточному аудиті відсутній. Наступний evidence етап потребує окремого дозволу. Спочатку виправити instrumentation coverage D1 у окремому diagnostic scope, використовуючи **існуючі hooks** звичайного ASI й INI diagnostics; не робити новий production FOV behavior.

**D1 validation до runtime:** actual SafetyHook adapter має виконати relocated rel32 CALL через ENTER/EXIT-like hook, зайти в target із відомим prologue, прочитати справжній saved return і класифікувати його через exact validated relocated-call mapping. Поточний synthetic memory test не достатній. Mapping повинен включати original callsite, relocated instruction, decoded call target та exact return; невідома адреса лишається unknown. Перевірити stale/destroyed hook та negative return у сусідній trampoline region. Не використовувати context.rip як native PC і не викликати довільні native getters із observer.

**Один bounded capture (до60 секунд):** від моменту перед Dialogue→Cinematic ENTER, а не лише після EXIT. Межі для поточного image hash `2BD3…5479`:

| Точка | Що записувати | Яку невизначеність усуває |
| --- | --- | --- |
| Installed ENTER (native callsite RVA0x35C7DA1) PRE/AFTER callback | generation, native/corrected argument, current mapping, active policy, event sequence/thread | Який target фактично пішов shared consumer |
| Existing successful-init observer RVA0x30DBD6D | S, receiver, weak identity якщо доступна без native call, start/target/remaining, exact mapped caller return, epoch | Чи цей ENTER створив actual activation для writer camera, або current start observer її не бачив |
| Existing blend setter PRE0xCCE670 / existing post-setter continuation | raw sample, actual modified XMM1, component/epoch/receiver, camera+230 after, caller evidence | Чи ENTER blend оновлює ту саму камеру, чи інша activation перезаписує поле |
| Existing writer PRE/POST | RSI/RBX, source/generation/admission sequence, +230 input, corrected XMM0, actual output+30 | Зв'язок actual write з current output ownership; відрізняє own intent від raw value |
| Existing EXIT before cleanup | own reference/revision, native EXIT target, actual final target, recovery owner | Inputs recovery після own-target change |

RVA — evidence anchors цього image, не production compatibility gates. Existing structural resolvers залишаються обов'язковими. Трасування увімкнути для active cinematic generation; чинний post-EXIT-only observation window пропускає потрібну ENTER фазу. Логи: максимум256 structured records, first/last та bounded samples, жодного per-frame file output. Caller attribution mapping exact; timestamp не замінює sequence/thread та receiver identity.

Гіпотези й результати:

1. ENTER activation того самого receiver існує і змінює+230, потім інша activation повертає70: producer overlap CONFIRMED, authored semantic class другої activation ще треба встановити за caller path.
2. ENTER activation працює з іншим receiver: current aspect/writer association не доводить його actual camera target; full implementation BLOCKED до native relation.
3. ENTER arg дійшов consumer, але successful init відсутній: це не доказ no-call; потрібен bounded existing `target-state.txt` branch/caller inspection (early same-target, invalid provider, failed camera resolve). Лише за цим результатом обґрунтована додаткова одноразова observation point перед early return.
4. Same generation отримує новий target через shared initializer без ENTER: UPDATE existence CONFIRMED; cinematic authorship лише після caller classification, не за FOV різницею.
5. У capture немає authored change: U1 залишається open. Потрібен конкретний known changing-FOV cinematic або статично встановлений authored producer; не додавати broad watchpoints лише тому, що capture порожній.

Overhead stop: при FPS degradation негайно прибрати owned instrumentation і detach; не повторювати conditional software breakpoints у projection hot path. Не terminate game, не rearm нескінченно. Зберегти exact process/session identities, artifact hashes, mapping та остаточний detach/exit evidence. Формула/mode correction у цьому evidence capture не змінюється.

### 9.5 Implementation phases після evidence

1. Закрити U1: назвати exact qualified producer/authority predicate, raw≠ENTER retention rule та invalidation. Якщо це неможливо, залишити full implementation blocked; не передавати Luna довільний choice між force-retain і raw HorPlus.
2. Оновити reference/target transaction у existing owner; виконати migration9.2 і aspect commit9.3. Усі numeric baseline dependencies із audit map отримують disposition: target authority removed, historical evidence retained або continuation predicate replaced by proven reference relation.
3. Окремо узгодити continuation: new accepted ENTER завжди reevaluates own intent; inherited/empty carry не має пріоритету над ним; strict source/admission guards зберігаються. Якщо native producer доводить continuity, вона може керувати mapping, але не вибором чужого native reference.
4. Закрити U2 на існуючих native-start/transformed-start fixtures та встановленому provenance contract. Власний новий endpoint зберігати при EXIT, а не повертати108.941 для сумісності classifier. Якщо policy cliff не усунутий або worsens, full candidate не вважається готовим.
5. Виконати focused adapters→один canonical NonGraphics→Release→окремо дозволений combined runtime. Однакова final цифра без source/generation/origin/POST-store доказів не є PASS.

Change map додатково включає `src/diagnostics/post_exit_blend_activation_observation.hpp`, runtime mapping initialization (`runtime.cpp:3187,3480`) та `tests/plugin/post_exit_blend_activation_cases.hpp` для D1; `external/safetyhook/safetyhook.cpp:565` є source evidence, його core relocation behavior у scope змінювати не потрібно. Зміни framework capability/native-hook topology розглядати тільки якщо результати U1 доведуть їх необхідність.

### 9.6 Додаткові acceptance cases

| Case | Обов'язкова перевірка actual transaction |
| --- | --- |
| ENTER90 / writer70 / arbitrary previous baseline | Own target126.87; authorized current output; raw70 не promoted до new native reference; retention rule підтверджений U1 |
| Same source / authored UPDATE та ordinary blend | Різні provenance predicates; тільки qualified UPDATE збільшує reference revision |
| Mapped relocated return | Actual executed CALL, exact source mapping; foreign trampoline return не прийнятий |
| Aspect update + source conflict + EXIT | Target replicas coherent; conflict invalidates mutation; EXIT не snapshot stale revision |
| Output A→B→A / B на EXIT | Per-call sequence admission; recovery source той самий; historical output не розіменовується |
| Native-start70→90.3115 / own endpoint126.87 | Повна output trajectory і disposition на кожному sample; не лише endpoint equality; native timing лишається власником |
| Legacy INI GameplayHorPlus / MatchGameplay / custom Gameplay FOV | Canonical own-reference semantics; bounded migration notice, штатний round-trip; RuntimeCombine не змінений |

### 9.7 Статус передачі

**BLOCKED — ADDITIONAL EVIDENCE REQUIRED.** Закрито механічний ENTER graph, source/output identity contract, aspect update design і конкретну migration proposal. Виявлено D1 caller-attribution defect, який пояснює межу поточного evidence. U1 та U2 критичні: full plan не READY, Luna не повинна самостійно вигадувати UPDATE authority або recovery mapping під час реалізації. Найкоротший наступний крок — виправлення/перевірка relocation-aware diagnostics у separately authorized scope та один описаний capture через existing observations.

## 10. Дозволений D1 diagnostic етап — поточний результат

Користувач дозволив D1 fix, actual adapter validation і підготовку bounded runtime capture зі збереженням production FOV policy. Реалізовано exact original/relocated CALL mapping через read-only MidHook trampoline accessor; existing hooks і native call topology не змінено. Оновлені diagnostic files: activation observation, observation window, runtime mapping/arming, actual adapter cases; vendor header змінився лише accessor-ом. Додатково усунуто повторний PRE log pending frame без POST, оскільки regression виявив порушення output budget.

Виконано focused ON/OFF і224-process adapter NonGraphics regression, PASS з verified cleanup. Initial wider regression FAIL через PRE flood зафіксований і усунутий; фінальний PASS стосується виправленого коду. Full canonical suite не запускався. Evidence збережено в `research/evidence/d1-caller-attribution-2026-11-13/`.

### Наступний ручний capture

1. Використати підготовлений **production-profile** `STALKER2CameraTweaks.asi` зі звичною назвою; діагностика вмикається existing INI setting. Game installation автоматично не виконується; попередній ASI зберегти. Не змінювати Cinematic/Gameplay policy для цього експерименту.
2. Завантажити save, що дозволяє почати цільовий Dialogue→Cinematic без попередньої довгої катсцени. Window починається на першому accepted ENTER і не поновлюється. Якщо попередня катсцена вже витратила60 секунд або192 blend records — потрібен свіжий process для нового capture, а не unlimited rearm. Не ставити projection breakpoints і не залишати гру на debugger pause усередині window.
3. Перевірити startup `BLEND_CALL_RETURN_MAPPING`: relocated адреси обох callsites ненульові, validation exact_E8_same_target. Нуль — unavailable mapping, а не PASS. Перевірити availability existing activation/setter/post-store hooks.
4. Відтворити одну цільову sequence до60 секунд; позначити візуальний дефект та його ENTER/EXIT фазу. Зберегти весь game log після sequence, hashes EXE/ASI/settings/aspect. Blend records≤192; writer store samples≤8 на generation, інші diagnostic records мають окремі existing budgets. Для цього короткого одногенераційного capture ціль —≤256 structured capture records; загальна кількість рядків game log не є цим cap.
5. Шукати mapped ENTER activation→receiver/start/target→tick/setter POST→current writer PRE/POST; потім відповідний EXIT activation і recovery. Native raw, forwarded corrected argument і actual output — різні поля. Candidate/matched_tick без caller/receiver relation не закриває authorship. Якщо authored UPDATE не спостерігався, U1 не закрито.

CPU fixture довів relocation/frame-read механізм, але ще не виконання конкретного game caller. При frame/mapping mismatch, receiver mismatch, budget exhaustion або FPS degradation зупинити capture й оцінити лише фактично отримане evidence. Не створювати нових hooks або альтернативної FOV policy без окремої доведеної необхідності.

U1/U2 та full implementation status залишаються **BLOCKED — ADDITIONAL EVIDENCE REQUIRED** до аналізу нового runtime log. Передача Luna для FOV behavior implementation поки не відбувається.

Підготовлений Release для цього capture: `build-artifacts/d1-diagnostics/STALKER2CameraTweaks.asi`; SHA-256 `818C76E05A899E98A9F5D28FAF4767A82730A3106235A563E9B728043AE5EB29`, 3,729,920 bytes. Build та224-case adapter regression PASS, cleanup verified: [validation-summary.json](../evidence/d1-caller-attribution-2026-11-13/validation-summary.json). Це handoff для ручної installation/capture, не завершення U1/U2; runtime не запускався. Зберегти звичну назву ASI та previous artifact, використати existing INI Diagnostics ON до запуску процесу.

### 10.1 Після отриманого runtime capture 01:23:33

Користувач виконав capture цим artifact; evidence і висновки додані в той самий аудит. D1 game-runtime підтверджено mapped EXIT activation. Native producer70 ідентифіковано точно: CALL RVA0x2E5457D→generic wrapper0x30DB9B8 з literal70 ізRVA0x9EDE384; observed return0x2E54582, receiver той самий, що cinematic writer. Його semantic Dialogue role не підтверджено. Цей producer спрацював післяEXIT1, тому не є доказом authored cinematic UPDATE.

60-second window почалось на ENTER1 о01:25:34.953; ENTER2 о01:26:28.267 залишило6.686 сек; EXIT2 о01:26:46.259 вже за межамиwindow. Повторити ту саму sequence без іншого start condition не дасть потрібного U2 evidence. Автоматично збільшувати або поновлювати window не дозволено.

Уточнений мінімальний наступний diagnostic proposal (ще не реалізовано):

1. На existing ENTER callback за валідованим static consumer data-flow зафіксувати safely readable provider/component identity, saved target і validity flags **до** native CALL. Це read-only evidence, не новий FOV reference. Не викликати virtual getter повторно з observer.
2. Якщо pre-state доводить exact existing-component/same-target branch, зафіксувати цю branch attribution. Якщо ні, потрібна одна bounded paired native observation для реального wrapper/guard result: positive-target guard0x30DBB08, provider-return/equality результат і шлях camera-init. Точку structurally resolve; не breakpoint на кожному projection кадрі. Така додаткова native point потребує окремого погодження після конкретного instrumentation plan.
3. Для future capture стартувати в Dialogue без попереднього cinematic ENTER; якщо такого save немає, потрібен явний one-shot manual arming mechanism у diagnostic scope, який починає нове60-second вікно лише після дії користувача. Не renew window кожнимENTER. Спершу обрати й підготувати цей mechanism, потім просити повторне проходження.
4. Captured authored producer зміну всередині generation відрізняти за native caller/path і receiver від identified producer70. Немає такої події — U1 open. Не оголошувати кожен generic rearm cinematic UPDATE.

Без нового target/arming evidence Luna не може вибрати між force retention і qualified UPDATE. Статус повного FOV mechanism залишається **BLOCKED — ADDITIONAL EVIDENCE REQUIRED**. Цей етап аналізу не змінював source, ASI, game process або INI; виконано лише archive/log analysis і два bounded read-only native decode runs із verified cleanup.

### 10.2 Вузький diagnostic контракт consumer outcome та одноразового arming

Це specification наступної diagnostic реалізації, не вже реалізована можливість ASI818C…EB29. Scope: перевірити прийняття forwarded ENTER target та actual consumer/guard outcome; cinematic FOV behavior не змінювати. Source/UPDATE framework не перепроєктовувати. Existing current-image artifacts `target-forwarding.txt`, `target-state.txt`, `target-arming.txt` і `native-caller-and-enter-guard.txt` є instruction anchors, не fixed-RVA production gates.

**Outcome evidence:** кожен armed ENTER отримує diagnostic invocation ID і bounded TLS frame, прив'язаний до native thread/stack context та cinematic generation. До native CALL зберегти own native operand, реально forwarded argument і current source association validity. Результат відстежувати до фактичного native return/fallthrough; original/relocated CALL mapping D1 обов'язковий. Shared consumer є void-like forwarding path: RAX після його повернення не трактувати як bool success.

| Підшлях | Точне питання | Потрібне actual evidence |
| --- | --- | --- |
| Shared consumer0x6AFFF7C | Чи дійшов виклик до wrapper? | Same-invocation entry/wrapper/return markers; provider null branch0x6AFFFC2..0x6AFFFD1 або tail dispatch0x6AFFFBD. Один відсутній marker не є доказом return |
| Wrapper provider | Чи resolver0x2036BA6 повернув придатний об'єкт? | Actual resolver result RAX перед test0x30DB9CF; flags bit30 перед branch0x30DB9DF; matched wrapper return marker підтверджує закінчення invocation |
| Existing component | Чи saved target збігся з forwarded target? | Actual path0x30DBAC3..DA: component identity, flags, saved+54, XMM6 requested; recorded compare outcome та повернення без initializer. Pre-ENTER numeric equality окремо недостатня |
| Creation guard0x30DBB08 | Чи guard відхилив нову activation? | Actual AL на return до0x30DBA00 та subsequent wrapper outcome. Лише якщо він false — деталізувати positive-target/provider/getter+818 equality за виконаною гілкою, не викликати getter повторно |
| Initializer0x30DBCD2 | Чи стався виклик, який не дав camera-start capture? | Same-invocation initializer entry, resolved camera/result та actual failure/return path; existing successful capture0x30DBD6D — окремий outcome, не універсальний success return |

У source/resolver зберігати адреси без пробілів; табличні anchors не є дозволом встановлювати hooks без decode/span/uniqueness checks. Після вибору мінімального набору actual boundaries перевірити overlap spans із existing hooks, relocated control flow і callback register preservation. Почати з outcome coverage, а не набору global camera write hooks. Додаткову деталізацію guard branches вводити лише якщо actual guard rejection спостерігався. Непокритий шлях позначати `OutcomeUnobserved`, не `EarlyReturn`.

Acceptance CPU adapter: справжній виконаний native-like CALL через MidHook; окремі paths consumer-null, provider-invalid, existing-target-match, guard-false, initializer failure і successful capture. Перевірити precise invocation correlation при nested/interleaved calls, timeout/overflow, stale generation, Diagnostics OFF, register/native-memory invariance та cleanup. Synthetic snapshots не замінюють actual execution outcomes. Runtime record має містити `original_native_input`, `forwarded_target`, `consumer_invocation`, `generation`, `thread`, `caller_mapping`, `path_observed`, `outcome`, source/receiver і record budget. Жоден із diagnostic outcomes не дозволяє production correction.

**Одноразове arming — рекомендований конкретний механізм:** diagnostic-only named auto-reset event `Local\STALKER2CameraTweaks.FovCapture.<PID>`, що створюється owner-ом лише при Diagnostics ON. Невелика user command відкриває цей існуючий event, робить Set і закриває handle. Не запускати polling worker: existing accepted ENTER callback неблокувально consume event через zero-timeout wait. Event означає «озброїти наступний accepted ENTER», а не поновлювати вже active window.

Стани diagnostic session: Dormant→Requested(next accepted ENTER)→Capturing→Spent. Максимум одна consumed request на процес; pending request не витрачає60 секунд, а countdown починається на потрібному ENTER2. Наступні ENTER/EXIT не renew timeout/record budget; Set під час Capturing/Spent ігнорується з bounded повідомленням. Failed event creation залишає capability unavailable без впливу на camera core. На reset закрити handle через owner після callback drain. Event name включаєPID, команда не створює відсутній event і не сигналить іншому процесу. Actual adapter має перевірити zero-wait, однократне споживання, відхилення повторного rearm і handle cleanup.

До runtime підготувати повний звичайний ASI, actual adapter results, hash і точну arm command. Тільки потім користувач сигналить event у Dialogue перед ENTER2 і проходить одну sequence до EXIT2/recovery. Загальний window≤60 секунд, structured capture records≤256 зі спільним бюджетом для нових consumer records та existing blend/writer records. Наявний cap192 можна використати як нижчий combined cap; окремі counters, сума яких непередбачувано перевищує256, не відповідають цьому diagnostic контракту. Не ставити hot-path conditional debugger breakpoints.

Результати оцінюються незалежно: (a) actual ENTER target прийнятий/відхилений із branch evidence; (b) U1 authored UPDATE observed чи not exercised; (c) U2 EXIT2/recovery covered чи coverage gap. Відсутність UPDATE не є failed capture, а встановлена branch причина не означає, що full fix READY. Якщо60 секунд не охоплюють фактичний сценарій, спершу змінити точку arm/save, а не мовчки збільшувати або renew budget.

Цим уточненням змінено лише аудит/план; diagnostic implementation, build та новий runtime не виконувалися. U1/U2 не закрито описом майбутнього механізму.

### 10.3 Реалізація consumer outcome diagnostics і manual arming — 2026-11-13

Етап 10.2 реалізовано в production-profile `STALKER2CameraTweaks.asi`; назва ASI звична, окремий diagnostic filename не використовується. За Diagnostics OFF додаткові probes не встановлюються. За Diagnostics ON чотири hooks прив'язуються через structural resolver; кінцева спільна точка — wrapper epilogue `0x30DBADC`, а не попередній conditional branch span, який перекривав incoming branch target. Resolver/image validation на поточному EXE підтвердив RVA `0x35C7DA6`, `0x30DB9CF`, `0x30DBA00`, `0x30DBADC`.

Consumer outcome корелюється bounded TLS stack-ом за invocation/thread/stack/generation. Неповна кореляція дає `OutcomeUnobserved`; null provider не класифікується як existing-target match через coincidental pointer/flags. Досягнення blend-start observer не подається як commit. Нова one-shot arm command використовує `Local\STALKER2CameraTweaks.FovCapture.<PID>` auto-reset event, zero-timeout consume на accepted ENTER, 60 секунд і спільний cap192 records; background polling нема. Команда: з root репозиторію `powershell -NoProfile -ExecutionPolicy Bypass -File "research\tools\ArmCinematicFovCapture.ps1"`, або з `-GameProcessId <PID>` при неоднозначному процесі.

Фінальна перевірка виконана після очищення test target, бо старий Ninja deps cache мав `deps=0`. Compilation актуального adapter пройшла; повний `--non-graphics` пройшов **224 fresh processes**, ON/OFF; focused ON/OFF та readonly current-image resolver також PASS. CPU cases охопили native-like hooked paths, shared-budget cap, nested LIFO frames, stale generation, capacity/mismatch, TLS isolation двох потоків, memory/register invariance та one-shot handle ownership. PowerShell arming script пройшов parser і event OpenExisting/Set smoke test.

Release build production-profile PASS. Артефакт: [STALKER2CameraTweaks.asi](../../build-artifacts/consumer-outcome/STALKER2CameraTweaks.asi), file version `3.2.2.0`, product version `3.2.2`, 3,764,224 bytes, SHA-256 `39114C22D4A3D036F0BBAFC6471A88B9C6DC27CCBACD40FC2D130A3532FC353C`. Попередній D1 candidate не перезаписаний: `818C76E05A899E98A9F5D28FAF4767A82730A3106235A563E9B728043AE5EB29`. Evidence: [validation summary](../evidence/cinematic-consumer-outcome-2026-11-13/validation-summary.json).

No game/debugger/graphics run; no automatic ASI installation. Відтак це готовий для ручного runtime capture **діагностичний кандидат**, не доказ FOV fix. U1 та U2 лишаються відкритими; наступний крок — користувач запускає гру з Diagnostics ON, у Dialogue виконує команду arming для поточного PID, потім один раз проходить цільову Dialogue→Cinematic→EXIT2 sequence. Capture оцінюється окремо за ENTER outcome, authored UPDATE (може бути not exercised) і EXIT2/recovery coverage. Не збільшувати/поновлювати budget автоматично й не трактувати `InitializerObservationReached` як успішний native commit.


### 10.4 Runtime 10.2 отримано: authority та continuity decision

Користувач вручну озброїв PID12036. Evidence та обмеження: [аудит §10.4](../reports/CINEMATIC_FOV_AUTHORITY_AUDIT.md#104-ручний-runtime-capture-enter2--exit2--2026-11-13), [архів](../evidence/cinematic-consumer-outcome-2026-11-13/runtime-111855/STALKER2CameraTweaks.log).

- ENTER selection: ROOT CAUSE CONFIRMED (native90 → baseline70 →108,941).
- Consumer: CreationGuardRejected CONFIRMED, coherent invocation/return; inner resolver invalidity versus virtual getter equality UNKNOWN. Wrapper resolver validity не доводить inner resolver validity. Flags без comparison_seen не є existing-target evidence.
- U1: IMPLEMENTATION BLOCKED; authored UPDATE не встановлено. Retention не можна виводити з одного raw70 або guard false.
- U2: RECOVERY UNRESOLVED; EXIT2 coverage CONFIRMED. Native70→90,3115 та boundary passthrough90,2877→HorPlus127,115 при input90,3074 записані. Final projection не виміряна.

Наступна обмежена decision phase: описати, чому accepted cinematic lifecycle може або не може authorize writer correction незалежно від native blend commit; узгодити retention/UPDATE і native-start EXIT mapping зі source/generation/invocation admission. Не використовувати diagnostics outcome як production authority. Порівняти старий transformed-start сценарій106,688→90 із native-start70→90,3115; кілька контекстів, rearm, foreign source, interruptions та справжній camera cut мають зберігати окрему семантику. Не додавати interpolation лише для приховування розриву.

Поточне evidence достатнє для фіксації результату10.2, але не для повної behavior implementation. Новий широкий capture не запланований. Якщо конкретний authority choice залежатиме від точної причини guard, спершу сформулювати discriminating outcome (inner resolver null/flags проти getter equality) та мінімальну observation, після чого окремо погодити scope. Цей review лише зберіг лог і оновив документи; нових build/test/game/debugger sessions та production changes немає.

### 10.5 Обмежений U1-A scope та U2 contract gap

Це аналітичне уточнення має пріоритет над нерозділеним U1 gate у §9/10. Production implementation не авторизується цим документом. Детальний аналіз, точні code owners і контрприклади: [аудит §10.5](../reports/CINEMATIC_FOV_AUTHORITY_AUDIT.md#105-розділення-u1-au1-b-та-u2-ownership--bounded-analysis).

**U1-A — bounded contract candidate.** Використати own accepted ENTER operand як R, current policy/aspect(R) як C; pre-ENTER native witness W дозволено використовувати лише для чинного initial-retention admission, не як cinematic target. R90/W70 може утримувати C≈126,87 незалежно від native guard result. Зберегти exact generation/source/aspect-store/output/sequence admission, departure fail-closed, invalidation та незалежність diagnostics. Чинний continuation/reentry не може підмінити новий R/C попереднім target. Це semantic change selection + retention integration; однієї заміни baseline selection недостатньо.

**U1-B — окремий unknown.** Unknown raw не просувається в reference. Межа scope: не обіцяти підтримку authored changes без нового validated boundary. Контрприклад — dialogue residue70 і справжній unobserved authored UPDATE70 за однакових source/generation/flags. Універсальний retention не доведений. До implementation прийняти bounded limitation; не оголошувати UPDATE PASS за відсутністю подій.

Мінімальна change surface U1-A після дозволу: cinematic selection/association/dispatch, lifecycle transaction і cinematic writer transaction; узгоджені reference/target та continuation state. Gameplay reference не видаляти — він може залишатися witness інших механізмів. Config migration §9.2 і спільні semantic changes не включати мовчки: перед реалізацією явно обрати виконання migration або вужчий semantics scope та відповідну документацію. Чинний Gameplay RuntimeCombine не змінюється через вибір cinematic policy.

**U2 — окремий mapping/ownership gap.** EXIT owner authorizes розгляд source, activation evidence — кандидат producer, current invocation provenance — право застосувати mapping. Наявні same-receiver/thread/epoch/float matches не доводять відсутності intervening writes. Diagnostic cache не стає production state. Для implementation contract потрібні generation-bound EXIT endpoint, current activation/rearm relation, setter→writer execution relation, підтверджений FOV-space/progress, invalidation/cancellation і handoff без зміни policy. Camera cut не згладжувати. Native70→90,3115 та transformed-start106,688→90 перевіряти окремо. Як саме забезпечити invocation relation без залежності від Diagnostics — не встановлено; це точна перешкода готовності U2, не потреба повторити той самий capture.

Порядок наступних рішень: прийняти межі U1-A та недоведені можливості U1-B; підготувати proportionate implementation batch для selection/retention/continuation з tests із матриці §10.5 аудиту; U2 оцінити окремо за invocation provenance та FOV-space. Реалізацію, запуск тестів і гру в цьому аналізі не виконувати. Обмежений ENTER batch не означає закриття U2 або готовність релізу. Внутрішня причина CreationGuardRejected не досліджується далі, доки від неї не залежить конкретне рішення.
