# Єдиний Lifecycle Authority: проєкт контракту

2026-11-17. Статус: модель для обговорення; реалізація НЕ дозволена цим документом.
Редакція 2: уточнено working-state/commit, provenance після cancellation,
scoped invalidation та effect failures за зауваженнями користувача. Це нормативні
правила запропонованої моделі, не нові встановлені факти engine runtime.
Це не варіант B з попереднього звіту: writer — лише одна подія спільної системи.
Production, тести та ASI під час підготовки не змінювалися.

## 1. Архітектурна межа

Authority — єдине право прийняти, завершити, відкликати або перенести camera
lifecycle. Воно не означає один універсальний camera mode чи нову копію даних.
RuntimeState може залишатися фізичним owner. Існуючі підструктури зберігаються
або замінюються в межах одного owner; двох незалежних копій «старої» та «нової»
моделі під час production-обробки бути не може.

Adapters захоплюють bounded native facts, перевіряють безпечність ABI/читання
та передають подію. Вони не виставляють coordinator/phase, не скидають handoff,
не вирішують baseline trust. Policies обчислюють числа за прийнятим дозволом,
але не визначають, який lifecycle активний. Diagnostics/UI спостерігають.

Заборонено реалізувати authority як facade, що просто викликає старі компоненти
з тими самими mutable state views. Така обгортка не змінює розподілу повноважень.

## 2. Інвентаризація нинішніх переходів та доля прав

Прямі записи шукали у runtime aliases, трьох production transaction headers,
BaselineStore, CinematicWriterAssociation/EnterActivation/RecoveryOwner,
Candidate/RecoveryRearm/Exclusion, ProviderHandoff та settings commit. Нижче
названі не лише hook entry points, а й внутрішні мутації та ранні виходи.
Це source-level inventory; воно не стверджує повноти engine writer coverage.

Скорочення: A = єдиний authority, P = pure proposal/guard/calculation,
C = приватне сховище/commit без власного рішення. Кожне «A» нижче — одне й те
саме право, а не окремий authority для кожного рядка.

| ID | Подія / нинішній шлях | Хто зараз змінює стан | У цільовій моделі |
| --- | --- | --- | --- |
| T01 | Runtime initialization/admission open | RuntimeState initialization, native callback gates | A створює початкові camera permissions; lifecycle ресурсів/threads лишається platform owner |
| T02 | Cinematic ENTER accepted/rejected | PrepareCinematicEnter, ProcessAcceptedCinematicEnter | A приймає generation, selection і transfers; P перевіряє вхід; reject не створює ownership |
| T03 | ENTER baseline/reference selection | ProcessAcceptedCinematicEnter + BaselineStore read + adapter snapshots | A обирає допустиме джерело і reason; policy лише формула; невідомий reference не реконструюється |
| T04 | ENTER corrected-start witness | runtime CaptureDialogueCorrectedStartCandidate, EnterActivation::Arm/Capture | A дозволяє перенос witness; activation record — C/P, не незалежний авторизатор |
| T05 | Aspect-store first source / conflict | ObserveCinematicAspectStore, Association::ObserveAspectStoreTarget | A приймає binding або scoped conflict; C зберігає результат; effect permission окремий |
| T06 | Cinematic writer association/admission | Association::ObserveWriter/AdmitWriterOutput/AuthorizeInitialEstablishmentRetention | A приймає per-invocation permissions; helper повертає facts/proposal, не змінює ownership сам |
| T07 | Mapping progress / prepared endpoint | EnterActivation::ObserveWriter/RecordWriterResult | A приймає сумісність provenance; C зберігає один bounded witness; формула — P |
| T08 | EXIT з admitted source | ProcessAcceptedCinematicExit, RecoveryOwner::Capture | A завершує activation і приймає передачу source recovery; cached RBX не стає live identity |
| T09 | EXIT без writer, continuity carry | ProcessAcceptedCinematicExit, ClearCameraWriterRecoveryState, RetireForReentry | A розділяє відсутність writer authority та можливість зберегти reference/witness; carry не дає recovery права |
| T10 | Invalid/duplicate/stale EXIT | Dispatch/IsAcceptedCinematicExit + adapter early return | A: Reject/NoChange для lifecycle; факт відхилення не симулює завершення |
| T11 | Writer source/output replacement | Association::ObserveWriter, RetireRecoveryForWriterRsiChange | A оцінює старий claim і новий source; output change не є автоматично receiver/world change |
| T12 | Foreign recovery writer | ShouldRejectPostRecoveryForeignWriter, RecoveryForeignSource returns | A: pass-through/не допускати sample; не стирати незалежний чинний claim без доказу його втрати |
| T13 | Native recovery interpolation/endpoint | ProcessHorPlusRecovery, CompleteCameraWriterRecovery | A приймає progress або terminal transfer; одразу визначає фінальний sample/baseline permission |
| T14 | Recovery без owner | ClearCameraWriterRecoveryState + coordinator store + early return | A: Retire/Unresolved/PassThrough з явною baseline дією; «coordinator=Gameplay» не означає fresh admission |
| T15 | AspectRecalculation recovery endpoint | Окрема гілка ProcessCameraWriter із early return | Той самий A і terminal contract, інші effect/sample permissions; не копія правил завершення |
| T16 | Exclusion progress/converged/cancelled | PostCinematicRecoveryExclusion::ObserveValidated/Arm/Reset | P оцінює факти, A приймає scoped guard/retirement; query не скидає lifecycle сам |
| T17 | Atomic EXIT handoff | ProcessAtomicExitHandoff, aspect/flags writes, pending/source fields | A приймає pending/application/cancel; effect result не вигадується при failed write |
| T18 | Provider armer | runtime activation callback, ProviderHandoff::ObserveArmer | A приймає пропозицію за owner/component/source/thread/evidence; rejection/cancellation — явні різні результати |
| T19 | Provider setter matched/mismatch | ObserveSetterSample, implicit Reset branches | P дає matching evidence; A вирішує, чи mismatch стосується чинного claim і чи його можна retired |
| T20 | Provider writer confirmation / trajectory | TakeWriterSample + TryMapPostExitProviderDialogueSample | A приймає перехід/одноразове consumption; mapping — P; C зберігає progress |
| T21 | Provider recovery complete / Dialogue admission | CompleteRecovery, ConsumeDialogueAdmission, ProcessDialogueBoundary | A комітить transfer до Dialogue та consumption разом; endpoint quarantine може лишитися scoped guard |
| T22 | Repeated admission / corrected setter evidence | BlocksRepeatedAdmission (може Reset), Record/ClearCorrectedDialogueSetterSample | Pure query не мутує; A приймає epoch retirement/consumption та provenance updates |
| T23 | Ordinary Candidate creation/pending | ProcessDialogueBoundary, CandidateTracker::Begin/BindReceiver | A приймає лише scoped hypothesis; не глобальне ownership |
| T24 | Candidate descent/stability/conflict | CandidateTracker::Observe, phase/policy/correction writes | P оцінює trajectory; A приймає Activate/Cancel/Retain та associated permissions |
| T25 | Stationary Candidate + proven Gameplay return | RetireStationaryDialogueCandidateOnGameplayWriter, ResetDialogueBoundaryState | A має terminal правило на writer evidence; не окремий mutator перед classification |
| T26 | Active Dialogue / ascent / rearm | ProcessDialogueBoundary, RecoveryRearm::Begin/Observe | A приймає фазу, права setter/writer та release policy; інший callback може дати terminal proof |
| T27 | Gameplay context invalidates Dialogue | resetDialogueIfActive callback у runtime, gameplay context evaluator | P пропонує evidence; A відрізняє interleaving, incompatible claim і corrected current input |
| T28 | Gameplay baseline Update/Retain/Invalidate | GameplayBaselineStore::Project/Invalidate та mode-switch callback | A приймає trust дію з причиною; Store — C із defensive validation, не другий lifecycle interpreter |
| T29 | Gameplay enable/mode pending transitions | ApplyCameraSetting, SelectGameplayModeForRuntime, ProcessGameplayEnable/ModeTransition | A приймає camera-setting event і pending/active permissions; persistence лишається поза camera transaction |
| T30 | Cinematic/Dialogue enable/policy changes | ApplyCameraSetting, active snapshots/epochs/correction activation | A відкликає mutation gate негайно на disable; selection/policy lifetime за agreed activation contract |
| T31 | Post-store/helper observation | runtime correlation frames, Camera State publisher | Observation event; A за потреби приймає bounded provenance fact, але readback не обов'язковий для корекції при Diagnostics OFF |
| T32 | Zoom observation/neutral Gameplay evidence | Zoom callbacks, neutral observation/publication | Факт спостереження; не незалежне право перемикати Dialogue/Cinematic чи baseline trust |
| T33 | Invalid read, nested callback, closed gate, stopping | Native callback wrappers, ранні відмови | NoIntervention/NoAcceptedLifecycleEvent; explicit terminal result для вже прийнятої події; не неявне old-state clear |
| T34 | Runtime reset / initialization failure / shutdown | ResetAllRuntimeResources, callback drain, hook gates | A відкликає camera permissions; platform owner серіалізує drain/join/release. Це не LoadGame |
| T35 | Historical reset/writer helpers | ObservePostCinematicDialogueRecovery, ClearCinematicRecoveryAuxiliaryState, CompleteCinematicRecovery, ArmAtomicExitHandoff, ApplyHorPlusGameplay | У поточному пошуку немає production callers; не залишати прихованим альтернативним authority під час міграції |
| T36 | Legacy replay / conditional experiments | ReplayManualTransitionOriginal та POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST path | Не активний звичайний production path; будь-який підтримуваний camera-mutating profile мусить підкорятися A або бути явно поза migration/release scope |

`POST_EXIT_BLEND_OBSERVATION` зараз увімкнений у звичайному ASI й містить
production provider handoff. Його не можна помилково виключити як «лише diagnostics».
Diagnostic one-shot markers/log counters не є lifecycle і не потребують права A.

## 3. Формальна модель: добуток станів, а не загальний enum

Нехай S = (O, D, R, H, P, B, K, G). Це логічні компоненти, не дизайн C++ класу.

| Компонент | Зміст |
| --- | --- |
| O | Scoped ownership claims: Cinematic activation, Gameplay processing permission; observed/unknown source relation явно позначена |
| D | Dialogue hypothesis/accepted phase: None, Candidate, Active, Exiting, RearmPending, з receiver/evidence scope |
| R | Recovery: None, Pending/Progress, Terminal. Completed metadata відокремлене від чинного дозволу |
| H | Provider handoff: None, Armed, SetterMatched, WriterConfirmed, Transferred, Retired; consumption окремий факт |
| P | Provenance witnesses: original capture / prepared setter result / inherited corrected writer / unknown, з boundary та identity |
| B | Єдиний Gameplay reference: value, source, validity/currency і consumer-specific permissions, плюс reason |
| K | Effective settings/capabilities та activation policy snapshots; релевантна сумісність замість одного універсального epoch |
| G | Intervention admission: ready/stopping/capability gates та scope pending physical transitions |

Ідентичність native source та output різна. `RSI + 0x230` і `RBX + 0x30` не
об'єднуються в один об'єкт. Generation — технічна activation identity, writer
transaction — окремий виклик, settings epoch — зміна налаштувань. Жодне з них
не оголошується world lifetime чи доказом allocation reuse.

Scope claim містить лише доведені зв'язки: native source, за потреби activation,
provider component/thread, релевантні settings та boundary. Відсутнє поле —
Unknown, не wildcard-дозвіл. Output identity авторизується для writer invocation;
історичний output не розіменовується як чинна камера.

### Обмеження сумісності

1. Candidate не є Active Dialogue і не має setter correction permission.
2. Accepted Cinematic і Active Dialogue не можуть одночасно коригувати той самий
   sample. Наявність UI вибору не є Dialogue event. Різні source claims можуть
   співіснувати; їхнє відношення має бути explicit, не глобальний mode switch.
3. Recovery/provisional provider evidence можуть співіснувати. Поточний sample
   має одну прийняту effect policy; transfer endpoint не є Gameplay admission.
4. Recovery metadata може лишитися після completion, але recovery write/gating
   permission уже відкликаний. Scoped endpoint quarantine має власну причину.
5. Valid/current Gameplay reference може зберігатися через accepted compatible
   Cinematic/Dialogue без права оновлювати його поточними samples.
6. Prepared provenance може співіснувати з історичним original capture. Prepared
   value не перезаписує original і не стає новим native через зміну phase.
7. Native-register observation — назва boundary, не гарантія відсутності upstream
   mod correction. Для Gameplay admission потрібна прийнята provenance.
8. Disable correction відкликає mutation permission, але не вигадує завершення
   нативної катсцени. Observation/capability та user selections зберігаються окремо.
9. No fresh Gameplay reference не забороняє прийняти новий Cinematic/Dialogue,
   але fallback і недоступні permissions мають бути explicit; чужий reference не
   стає придатним лише заради потрібного числа.

Невизначене співіснування claim-ів не перетворюється на заборону для ВСІХ source.
Водночас source mismatch сам по собі не створює новий Gameplay claim.

## 4. Події та transition relation

Логічна функція: T(S, E) -> (S', Decision, Effects).
E містить захоплені факти з названої boundary, не готовий lifecycle verdict адаптера.
Decision включає accepted/rejected/retained/retired, scope, причину, дозволи на
provenance/FOV/baseline та їхні evidence dependencies. Не всі поля потрібні кожній події.

### Основні переходи

| Подія + guard | Прийнятий перехід | Результат для sample/reference |
| --- | --- | --- |
| Valid ENTER, settings/capability admission | Новий Cinematic claim; сумісні witnesses retained; incompatible Dialogue permission revoked | Обрати reference за trust, authored original зберегти; підготувати target лише за дозволеною policy |
| Valid aspect association | Bind native source для activation | Ще не admission довільного output; conflict відкликає відповідне право |
| Valid Cinematic writer | Admit поточний output та provenance | Одна mapping policy або named pass-through; witness лише від admitted decision |
| Accepted EXIT + admitted source | Завершити activation, створити source-owned recovery | Retain compatible B/P окремо; не видавати carry за recovery |
| Accepted EXIT без writer | Завершити activation без вигаданого owner | Retain reference тільки за незалежною trust; наступний ENTER не потребує fake Gameplay між ними |
| Proven native recovery endpoint | Retire recovery permission; committed next ownership | Під час ТІЄЇ САМОЇ транзакції вирішити Update/Retain/Exclude. Entry RecoveryOwned не керує наступною projection |
| Provider endpoint + correlated handoff | Retire recovery authority, transfer до прийнятого Dialogue | Не допускати native provider 70 у Gameplay; policy activation та consumption узгоджені |
| Neutral Candidate + current Gameplay witness | Retire scoped hypothesis | Current native sample може одержати Gameplay permission без ще одного setter |
| Candidate + accepted descent | Прийняти Active Dialogue, захопити policy | Setter correction і writer provenance узгоджені; samples не оновлюють Gameplay B |
| Candidate conflict/stability | Cancel або scope-limited retain за explicit evidence | Cancellation не створює Gameplay/native proof; конкретні правила наведені в §10.3 |
| Dialogue terminal native recovery | Retire active/correction permissions, release captured policy | Поточний input окремо перевірити: prepared input може лишитися excluded; prior B не invalidated без причини |
| Writer source/output change | Reassess конкретні claims і P/B relationships | Відокремити source replacement від output replacement/interleaving; unknown не глобальний дозвіл/заборона |
| Camera setting accepted | Оновити K та потрібні scopes, revoke mutation на disable | Provider activation зберігає accepted policy; user selection не підміняє activation policy заднім числом |
| Unrelated/invalid event | Reject або NoChange для непов'язаних claims | Не застосовувати FOV, не стирати чужий owner «за компанію» |

Різниця Reject/Retire принципова. Сьогодні ObserveArmer та частина provider query
методів можуть очистити попередній стан під час перевірки. У моделі це має бути
окреме прийняте рішення: що саме доводить втрату чинного claim, чи стосується
подія його scope. Сам факт неуспішного запиту не доводить потребу глобального reset.

## 5. Атомарність і зовнішні effects

### 5.1. Один послідовний робочий стан і одна точка commit

Для E authority бере committed S_n та створює приватний W_0 = S_n. Усі внутрішні
кроки цієї події читають актуальний W_i й формують W_(i+1), а не незалежні
копії початкового S_n. Робочий стан — логічна вимога; вона не вимагає копіювати
всі runtime resources або створювати другий production owner.

Порядок залежностей:

1. Перевірити admission події та identity/evidence scope.
2. Оцінити lifecycle progress, termination та transfers на робочому стані.
3. Узгодити співіснування claims і визначити provenance permissions sample.
4. Обчислити policy/effect proposals за актуальним робочим рішенням.
5. Визначити baseline Update/Retain/Invalidate та причини.
6. Перевірити інваріанти S', Decision і Effects як одного результату.
7. Один commit: опублікувати S_(n+1), Decision та дозволені effect intents.

Pure calculations можна виконати раніше, але їхній результат не є дозволом.
Кожна локальна пропозиція має перелік залежностей: source/activation, релевантні
settings, provenance та lifecycle facts. Якщо наступний внутрішній крок змінив
залежність, стару пропозицію потрібно відкинути або переобчислити до commit.
Не можна зберегти entry RecoveryOwned як чинний дозвіл після прийнятого terminal
transfer лише тому, що його обчислили першим.

EXIT та пізніший provider armer зазвичай є різними native events, не одним
штучно об'єднаним callback. Кожен має власний commit. Якщо один реальний writer
event підтверджує і recovery completion, і provider transfer, усі його внутрішні
кроки послідовно працюють на одному W; зовнішнім споживачам не видається
проміжний Gameplay permission перед кінцевим Dialogue permission.

Жодний adapter, store, publisher, UI чи інша policy не читає W як committed truth.
Внутрішній pure helper читає лише переданий йому актуальний working-state input;
не перечитує globals. Зовнішні readers бачать попередній або новий committed
стан, але не довільну суміш. Техніка публікації ще не обрана: наявність кількох
atomic fields сама по собі не забезпечує атомарності пов'язаних фактів.

Початкові context/evidence залишаються immutable історією E. Чинні permissions
походять тільки з фінального рішення. Логічний номер n — позначення порядку
commit, не вимога нового глобального engine epoch і не доказ world lifetime.

### 5.2. Reject, ранній вихід і exception

Кожна admitted подія завершується одним результатом: Accepted, Rejected/NoChange
або ScopedRetirement. Навіть NoChange явно несе відмову видати нові permissions.
Ранній return не може обійти перевірку/фіналізацію рішення. Callback, відхилений
до admission, не створює робочої транзакції й лишає native pass-through.

Помилка до commit не публікує W і не виконує його effects. Якщо встановлено
недостовірність уже committed claim, це оформлюється мінімальним валідним
ScopedRetirement/Unknown рішенням, а не публікацією частково обчисленого W.
Після commit його не «відкочують» зручним старим snapshot: failure зовнішнього
effect є новим корельованим фактом. Exceptions не виходять за native ABI.

CameraCorrectionGate уже дає serialization camera callbacks/settings, але це
не заміна working-state правила. Не можна виправити його ще одним mutex.

### 5.3. Commit наміру не дорівнює виконанню native effect

Результат події розділяє Requested/PreparedArgument, AppliedAtNamedBoundary,
Refused/NotAttempted та Unknown/Partial. Register assignment, native store і
post-helper readback є різними boundaries. Applied завжди називає boundary;
запис аргументу не підтверджує запис camera field або rendered FOV.

Effect виконується тільки за дозволом committed Decision, scoped до виклику,
source, target field/ABI boundary та релевантних settings/activation. Provisional
permission з W назовні не виходить. Залежний від ще не виконаного effect claim
залишається pending і не використовується як підтверджена provenance.

Перед застосуванням effect перевіряються чинність per-call permission та його
релевантні залежності в серіалізованому control path. Відкликаний або несумісний
дозвіл не виконується за збереженою копією старого Decision. Відсутність змін у
непов'язаних спостереженнях не вимагає штучного глобального epoch.

EffectResult — окрема logical event з власним одним commit; вона не є повторним
викликом native callback або приводом рекурсивно захопити не-recursive gate.
Синхронний результат може бути оброблений у вже серіалізованому control path;
пізніший native/post-store результат потребує точного зіставлення. До нього
наступний споживач бачить coherent Pending/Unknown, а не вигадане Applied.

Виконуваний effect permission не переживає native invocation і не переноситься
на наступний callback. Відсутній результат на завершенні invocation означає
Unavailable/Unknown для залежного факту; це не глобальний pending lock. Explicit
retained lifecycle witness — інший дозвіл зі своїми guard-ами, не повторне
використання старого per-call effect token.

Не обіцяємо атомарності з усією Unreal Engine memory. Diagnostics OFF може
означати відсутність фактичного readback: корекція від цього не вимикається,
але StoreObserved/Effective FOV не вигадується. Очікування readback не тримає gate.
Детальні дозволи при failures визначені в §10.5.

Поточний implementation 67E56485 має локальний guard completion. Це evidence
для одного правила T, не реалізація загального authority.

## 6. Safety і liveness окремо

**Safety:** unknown/foreign sample не стає Gameplay; prepared input не
трансформується повторно без прийнятої mapping; старий source/witness не
відроджується; одна подія не застосовує несумісні policies; guard failure не
вимикає незалежний scoped owner; stale state не дає прав після retirement.

**Liveness за наявності terminal evidence:** після accepted terminal event
дозволи відкликані й наступний дозволений callback не потребує випадкового
«ремонтного» writer. Candidate може завершитися на writer/provenance/lifecycle
evidence, не лише наступним setter. Gameplay з незалежним підтвердженням не
блокується чужою гіпотезою. Для pending claim явно вказані scope, missing proof
та можливі terminal events.

**Liveness без спостережуваних доказів:** мовчання callback-ів не доводить engine
EXIT, Gameplay чи LoadGame. Тут модель повинна обмежити вплив невідомого claim
й назвати відсутній proof. Неможливо обґрунтовано гарантувати будь-який новий
ownership після довільної тиші. Не маскувати цю межу таймером або auto-reset.

## 7. Усі класи раннього виходу

| Клас виходу | Обов'язкова семантика |
| --- | --- |
| Callback не admitted / reentrant / runtime stopping | Native pass-through, не прийнято lifecycle event; не залишити new partial claim |
| Невалідні ABI/read/value facts | Reject або scoped invalidation тільки за доведеним зв'язком; відсутність readback — не Applied |
| Invalid/stale/duplicate ENTER/EXIT | Явний NoChange/Reject; технічна generation не знищує незалежний B |
| Foreign owner / association conflict | Refuse конкретний intervention; рішення про retirement відокремлене від refusal |
| Відсутній recovery owner | Явно retire недоведені permissions, без автоматичного fresh sample admission |
| Exclusion suppressed / provider pending | Retain scope та очікувані докази; не глобальна нескінченна заборона |
| Recovery completed / provider transferred | Terminal decision і його final permissions завершені до повернення |
| Mapping outside trajectory / unknown corrected start | Named pass-through, не повторний HorPlus і не зміна lifecycle лише за числом |
| Settings capability refusal / no-op | Старий effective state або explicit revoke за правилом; persistence failure не змінює уже прийняті settings заднім числом |
| Effect failure / native callback exception | Визначений aborted/refused result; не Applied. Якщо зовнішній effect частковий, позначити uncertainty, не виконувати вигаданий rollback |
| Completed callback без наступного writer | Наступний event бачить завершений permission state; publication відсутнього writer не симулюється |

До реалізації потрібно прив'язати кожен конкретний return до одного рядка та
явного Decision. Категорія без визначеної дії — незакритий дизайн, а не дозвіл
залишити старий early return поза authority.

## 8. Перевірка моделі на збережених послідовностях

Це письмова перевірка transition relation, не executable replay/model checker.
Стандартні джерела та chronology наведені у
LIFECYCLE_AUTHORITY_RECONSTRUCTION_2026-11-17.md. Числа нижче — факти captures
або очікувані наслідки правил, що явно розрізняються.

| Corpus | Спостережений успіх/дефект | Як модель його пояснює / виключає |
| --- | --- | --- |
| PID 3120, interactive reentry | Writer-empty generation втрачала reference; inherited start отримував другий HorPlus | T09 не змінює B лише через no-writer; T04/T07 не створюють fresh-native із prepared witness. Обидва порушення формально заборонені |
| PID 13472, multi-save | Candidate після реального Dialogue блокує admission source B | D — scoped hypothesis; T11 переоцінює claims, T25 використовує current proof, чужий B не переноситься. Якщо proof незалежного Gameplay відсутній — чесне Unknown, не вигаданий LoadGame |
| PID 7372 g1–g4 | Target 153.931 збережений через повторні ENTER та порожню g3 | O/R/P/B мають окремі строки; сумісний reference retained, recovery без witness не вигадується |
| PID 7372 tx2259 | Recovery completes, але Gameplay projection отримує RecoveryOwned, B стає non-current | T13 final permissions визначаються після terminal evidence. Entry context історичний; це суперечливе споживання неможливе за контрактом commit |
| PID 7372 g5 | Neutral Candidate після completion без current B; authored 90 -> 126.87 | Наслідок попереднього порушення. Модель зберігає/приймає B за recovery evidence до callback, а не примусово відроджує його у Candidate |
| PID 7372 ordinary Dialogue | Accepted descent до 70, Reduced; rearm завершує policy; fresh Gameplay writer; два Candidate retired | D active/exiting/rearm, native/prepared P та fresh B розділені; відтворюється без прирівнювання coordinator Gameplay до sample eligibility |
| PID 7372 source B, g6–g9 | Fresh reference 130.311 -> target 153.931; empty g8 не шкодить | Стара identity не є дозволом для B; fresh admission B і наступна compatible continuity окремі переходи |
| PID 26108, combined validated runtime | Cinematic -> true Dialogue Reduced -> Cinematic, Gameplay і standalone Dialogue візуально коректні | T18–T22 transfer не підміняється Gameplay endpoint; policy snapshot/provenance зберігаються; цей позитивний corpus обов'язково лишається контролем |

Не кожна подія provider/component/binding детально записана в кожному capture.
Не заповнювати пропуски зручними значеннями. Правило для неповного evidence має
давати Unknown/scoped refusal, а оцінка corpus — Not established для такого
переходу, а не вигаданий PASS. Авторський UI вибір не є proof Dialogue.

Додаткові необхідні перевірки моделі після погодження: completion -> immediate
setter проти completion -> redundant neutral writer -> setter; нуль writer після
EXIT; accepted new ENTER без Gameplay; CandidateCancelled перед іншим native
blend; settings disable під час handoff; interleaved receiver без source ownership
transfer; native effect refusal; missing post-store; callback exception.

## 9. Як не залишити старих незалежних authority

До початку реалізації мають бути погоджені T01–T36 і destinations усіх старих
mutators. Критерій дизайну — немає рядка «старий компонент вирішує як раніше».

Для майбутнього коду потрібні окремі докази, яких документ не підміняє:

1. Writable lifecycle state доступний лише authority/його private commit code.
   Старі mutable views та g_* aliases не дозволяють адаптерам/policies stores.
2. Candidate/association/provider/recovery helpers можуть бути приватними
   subroutines authority, але не публічними незалежними шляхами commit. Read-only
   query не скидає state; mutating validation повертає proposal або викликається
   тільки як частина вже прийнятого commit.
3. Settings, startup/reset та optional profiles входять у ту саму access map.
   Необмежений reset callback, переданий leaf policy, — також bypass.
4. Store захищає структуру/identity/sequence, але не створює новий lifecycle
   verdict з entry WriterContext. Diagnostics/UI не мають mutation capabilities.
5. Source scan/access checks плюс реальні entry-point tests перевіряють цю карту.
   Назва нового класу або facade не є доказом відсутності старих writers.
6. Не запускати production з двома authoritative state copies. Якщо потрібне
   порівняння моделей, стара — лише read-only oracle/recorded observation, без
   права впливати на FOV або permissions.

Наразі старі компоненти, звісно, ще мають права, перелічені в інвентаризації:
код не міг змінитися в design-only етапі. Доведено лише повноту запропонованого
розподілу для знайдених source paths; відсутність bypass у майбутній реалізації
потрібно перевірити після неї до Release. Не оголошувати це вже виконаним.

## 10. Докази, дозволи та поведінка без доказів

Редакція 2 замінює загальний список питань нижченаведеними запропонованими
правилами. Для кожного вказано наявний доказ, його силу та fallback. Пропозиція
fallback не доводить, що наявні hooks забезпечують потрібний позитивний доказ
в усіх ігрових сценаріях. Невстановлене покриття винесене в §11.

### 10.1. Новий Gameplay source без baseline

| Доступний доказ | Що дозволено | Без цього доказу |
| --- | --- | --- |
| Source-owned native recovery endpoint, прийнятий terminal transfer, native provenance цього sample, сумісні settings і відсутній provider quarantine | Fresh Gameplay admission для цього source/sample; зайвий наступний writer не потрібен | Не називати source Gameplay за самим completion flag без identity/provenance |
| Чинний source-bound Gameplay reference плюс прийнятий writer із сумісною нативною provenance, без intervening prepared mutation/ownership conflict | Продовжити Gameplay claim і за його правилами оновити reference | Числова близькість або попереднє current число не замінюють provenance |
| Незалежний validated native Gameplay-context proof для нового source | Початковий admission без чужого B; proof повинен називати реальну boundary і identity | Такий універсальний proof для всіх нових source зараз не встановлений; не вигадувати його з default coordinator або адреси |
| Лише нові RSI/RBX, flags/aspect, FOV 130, завершений claim іншої камери або відсутність callback | Лише observation та scoped reassessment | NoGameplayWitness для нового source; не Update B, не запозичувати старий source. Застосувати лише policy, для якої незалежно є дозвіл; інакше native pass-through |

Позитивний proof має бути causal та source-scoped, із вказаним рівнем provenance
і сумісністю залежностей. «Немає відомого конфлікту» не дорівнює «підтверджений
Gameplay». Відсутність proof B не відкликає прийнятий незалежний claim A.
Підтверджена заміна контексту може інвалідувати A, але це окрема прийнята подія,
не висновок із кожного interleaved writer B.

### 10.2. Pending/Active Dialogue без наступних setter callbacks

| Доказ / подія | Дозволена зміна | За відсутності |
| --- | --- | --- |
| Candidate + сумісний validated current Gameplay witness та stationary native endpoint | Retire гіпотезу на writer boundary без наступного setter | Candidate не активується й не завершується лише за тишею або таймером |
| Прийнятий Dialogue native recovery completion | Retire correction/ownership, release active policy; current sample перевірити окремо | Active/Exiting metadata не проголошується завершеним за одним FOV числом |
| Accepted Cinematic transfer на тому самому scope або прийнята incompatibility/revocation event | Відкликати несумісні Dialogue permissions без вимоги старого Dialogue EXIT | Непідтверджений source mismatch не симулює transfer |
| Writer іншого source з незалежним прийнятим Gameplay proof | Обробляти його за його claim; старий Dialogue не має права veto | За відсутності власного proof причина відмови — NoGameplayWitness цього source, а не «десь існує Dialogue» |
| Writer того самого scope, несумісний з потрібними guards старої policy | Не використовувати стару policy для цього sample; explicit Unknown/conflict | Це саме по собі не доводить engine termination і не дозволяє Gameplay admission |

Термін життя metadata, дозвіл корекції та scope veto — різні властивості.
Невстановлене завершення A не блокує підтверджений незалежний B. Старий claim
може вплинути на B тільки за прийнятим cross-source зв'язком цієї активації.
Відсутність глобального veto не означає автоматичного визнання будь-якого B.

### 10.3. CandidateCancelled: окремий контракт provenance

Cancellation відкликає права саме скасованої гіпотези D. Воно НЕ змінює P,
не створює NativeGameplayWitness і не робить поточний або наступний sample
GameplayEligible через саму фазу None.

| Причина / доступні факти | D / P | B та поточний sample |
| --- | --- | --- |
| Непідтверджена trajectory, зміна callback source/target, identity conflict | Cancel відповідну гіпотезу; prepared або unresolved provenance залишається такою | Не Update B на підставі cancel. Раніше trusted B — Retain, якщо немає незалежної причини invalidation |
| Stationary cancel і окремо прийнятий current Gameplay proof для цього sample | Cancel D; P одержує дозволи ВІД proof, а не ВІД cancel | Update можливий у цій же події тільки за незалежним proof; не вимагати зайвого «ремонтного» writer |
| Sample походить від підтвердженого prepared setter | Cancel D не стирає prepared evidence | Не допускати prepared sample як новий native reference; не застосовувати повторну корекцію |
| Доведено релевантну втрату source/context/settings trust | Retire залежні claims/witnesses за explicit reason | Invalidate відповідний B; cancel не може зробити його current заднім числом |

Якщо після cancel лишається unresolved producer/transition evidence, його
записують у P зі scope та причиною, а не залишають удаваний Active Dialogue чи
глобальний quarantine. Завершують цей unresolved relation новий accepted native
proof, належно підтверджений ownership transfer або explicit scoped invalidation.
Сам наступний writer, stable FOV чи перехід phase=None доказом не є.

Таким чином проміжок у PID 7372 між CandidateCancelled та новим descent більше
не одержує Gameplay admission автоматично. Чинний baseline може лишатися
придатним для наступного Cinematic, навіть коли цей конкретний sample excluded.
Це правило моделі, а не твердження, що існуючий код уже так працює.

### 10.4. Provider mismatch: Reject не означає Retire

| Подія / доказ | Новий evidence | Старий підтверджений H |
| --- | --- | --- |
| Чужий component/source/thread, unrelated EXIT setter або endpoints іншої activation | Reject як unmatched | Retain; не очищати його через невдачу чужого зіставлення |
| Unreadable/invalid input чи unauthorized request без прийнятої зміни його залежностей | Не видавати permission поточному запиту | Не вважати сам rejection доказом завершення старого H |
| Authority вже прийняв settings/capability revoke, generation/owner retirement або підтверджений source conflict, від якого H залежить | Відхилити stale evidence | Retire ЛИШЕ H та permissions, залежні від цього прийнятого факту |
| Accepted нова activation того самого scope з явним supersession | Прийняти нове evidence | Retire старе й установити нове одним commit; не залишити подвійне consumption право |
| Повторне використання вже consumed admission | Reject/idempotent NoChange | Не повторювати transfer та не скидати вже прийнятий Active Dialogue |
| Exact matched setter/writer trajectory | Прийняти matching/consumption | Progress/transfer лише за agreed transition, а не як side effect read-only query |

Значення epoch/generation з неперевіреного запиту не може самостійно відкликати
claim. Причина retirement повинна посилатися на вже прийнятий authoritative
dependency change, а не лише на нерівність двох переданих чисел.

### 10.5. Native effect failure: права за рівнем підтвердження

| Доступний результат | Дозволений факт / дія | Заборонене припущення |
| --- | --- | --- |
| Intent прийнятий, effect ще не виконаний | Requested + лише per-invocation effect permission | Applied, camera-field provenance або fresh B із запланованого output |
| Підтверджено підготовку аргументу/регістру | ArgumentPrepared на конкретній ABI boundary | NativeStoreObserved або rendered FOV |
| Відомо, що запис не почався або був відхилений | Refused/NotAttempted; відкликати невиконаний effect permission | Що native state змінився; повторна корекція за planned числом |
| Можливий частковий write або exception після початку effect | Unknown/Partial для його залежностей; не використовувати їх як confirmed provenance | Повний успіх, гарантована відсутність змін або вигаданий rollback |
| Correlated post-store/helper readback | StoreObserved лише на названій boundary, з transaction/source/output identity | Повнота writer coverage або фінальна rendered projection |
| Result/readback відсутній, зокрема Diagnostics OFF | Unavailable для непідтвердженого рівня; незалежні lifecycle/native facts зберігаються | Автоматичний Applied чи глобальне блокування модифікації камери |
| Result запізнився після retirement/несумісної зміни scope | Historical observation або Reject за correlation contract | Відродження старого H/O/P/B чи дозвіл на новий effect |

Незалежно доведене завершення native recovery не скасовується лише через failed
mod output write. Навпаки, claim, для прийняття якого необхідний непідтверджений
effect, не підвищується з pending у confirmed. Baseline native reference та
розрахований HorPlus target не називаються фактично записаним output.

## 11. Перевірки несуперечливості та межа готовності

Критерій для всіх T01–T36: після commit будь-який наступний callback може
одержати лише permissions, сумісні з committed claims, provenance та dependencies
саме його scope. Entry classifications і локальні W-proposals не є джерелом прав.

Для незалежної моделі перевірки потрібні контрприклади до кожного правила:

| Контрпослідовність | Обов'язковий результат |
| --- | --- |
| Recovery completion -> provider transfer в одній прийнятій writer події | Зовні лише кінцевий transfer decision; проміжного Gameplay admission немає |
| CandidateCancelled -> writer з тим самим числом | Cancel не створює native proof; B не Update без незалежного evidence |
| CandidateCancelled -> наступний Cinematic з чинним prior B | Скасована гіпотеза не знищує B без причини; sample exclusion і придатність reference різні |
| Active Dialogue A мовчить -> independent confirmed Gameplay B | A не veto B; A не вважається завершеним лише через B |
| Нова адреса B без proof | Немає ні foreign B reuse, ні fabricated Gameplay admission |
| Unrelated provider mismatch -> exact old matched setter/writer | Старий H лишається доступним; чужа відмова не знищила його |
| Accepted settings revoke -> старий matched result | Відкликаний permission не відновлюється пізнім result |
| Effect refused/partial -> наступний callback | Немає Applied/provenance прав від planned output; незалежний owner не губиться |
| Early return / exception на будь-якому внутрішньому кроці | Не видно часткового W; немає escaped provisional effect permission |
| Completion -> immediate setter проти completion -> redundant writer -> setter | За однакових доказів після завершення не потрібний «ремонтний» writer |

Це специфікація майбутньої перевірки, не виконані тести й не формальний доказ.
Письмове зіставлення з corpus §8 залишається evidence review, не executable replay.

Знято неоднозначність правил: cancellation не дає native admission; unrelated
provider reject не стирає accepted H; effect intent не дає Applied; одна подія
використовує один послідовний W та один commit без зовнішніх проміжних дозволів.

Залишаються НЕ доведені позитивні можливості:

1. Яка реальна native boundary дає незалежний Gameplay proof новому source без B
   у всіх потрібних сценаріях. Наявність такого універсального сигналу не встановлена.
2. Які з описаних terminal proofs доступні для справжнього Active Dialogue після
   зникнення setter callbacks у кожному потрібному переході. Тишу proof не замінює.
3. Повнота зіставлення кожного конкретного early return та exception/effect path
   з цим контрактом, і відсутність mutation bypass у майбутньому коді.
4. Незалежна перевірка transition relation на негативних послідовностях і corpus.

Без доказів для потрібних позитивних сценаріїв fail-closed fallback забезпечує
лише safety, не повну працездатність. Тому контракт лишається DRAFT FOR DISCUSSION;
T01–T36 не оголошені погодженими до міграції. Наступний етап перевірки моделі та
будь-яка реалізація потребують окремого погодження користувача.
