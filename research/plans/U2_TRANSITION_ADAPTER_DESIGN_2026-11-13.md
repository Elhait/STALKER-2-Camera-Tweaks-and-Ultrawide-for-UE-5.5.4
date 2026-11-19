# U2 — два transition adapters

Статус: design для перегляду, не production contract і не виконане виправлення.
Scope: Cinematic→Dialogue та Dialogue→Cinematic; спільні вимоги до evidence,
окремі adapters до native lifecycle. Production, hooks, ASI, settings і U1-A
у цьому етапі не змінювалися. Tests/build/game/debugger не запускалися.

## 1. Передумови та межі доказів

Endpoints і формули — [U2_POLICY_ENDPOINTS_2026-11-13.md](../reports/U2_POLICY_ENDPOINTS_2026-11-13.md).
Runtime і visual acceptance — [поточний план](U2_TRANSITION_POLICY_BOUNDARIES_2026-11-13.md).
Обидві візуальні межі залишаються OPEN.

| Потрібний факт | Поточна реалізація | Статус |
| --- | --- | --- |
| Cinematic ENTER reference і effective selection | ProcessAcceptedCinematicEnter, authored R, generation, settings epoch | Є; не доводить native blend commit |
| Provider activation→setter→writer для EXIT1 | PostExitProviderHandoff | Є bounded mechanical evidence; не renderer identity |
| Endpoint за обраною Dialogue policy | AdaptiveTarget/ReducedTarget/Disabled branch | Формули є; policy-qualified B конкретного переходу не встановлено |
| Після recovery не приймати ту саму activation як новий Dialogue | CompleteRecovery/BlocksRepeatedAdmission | Реалізовано, routing підтверджено; visual defect лишився |
| Current ENTER activation→setter→writer | Не передається до cinematic writer як production evidence | Відсутній integration contract |
| Corrected start попереднього режиму | Current continuityStart читається з receiver+0x230 | Не підтверджений як applied corrected output |
| Writer application | PRE/POST trace у runtime.cpp | Diagnostics-gated, bounded; не production authority |
| Active view / projection consumption на двох межах | Немає синхронізованого доказу для цих samples | Невідомо |

Таблиця не дозволяє підміняти відсутні production evidence diagnostic snapshots.
У runtime.cpp BeginCameraWriterPostStoreTrace та ObserveCameraWriterPostStore
мають Diagnostics gate і record budget. Їхні останні записи не можна прямо
використати для ввімкнення transition mapping.

## 2. Спільні вимоги, без глобального ownership manager

Runtime camera integration залишається власником рішення; cameraCorrectionGate
серіалізує lifecycle, settings і writer mutation. Dialogue boundary постачає
policy context; adapter визначає єдину boundary для своєї mutation. Overlay і
Diagnostics лише читають результат. Native blend володіє часом та easing.

Transition context має містити тільки generation/activation-scoped факти:

| Група | Необхідний зміст |
| --- | --- |
| Identity | Локальний activation epoch із фактичної initialization; source/receiver/component; generation; thread/execution relation; admitted output поточного invocation |
| Native | S, T, sample F, domain, validated branch, rearm/cut status; доказ setter→current writer, не лише timestamp/float equality |
| Corrected start | Значення попереднього policy output, boundary, source/generation/input sequence/settings epoch/aspect; окремо рівень application/consumption evidence |
| Destination | Effective policy/epoch, її reference B або R з provenance, effective aspect, corrected target |
| Delivery | Останній admitted writer invocation; terminal/handoff/cancel status; explicit destination owner |

Ні pointer, ні monotonic FOV interval, ні same-thread/близький час окремо не
доводять activation lineage. Component address може повторно використовуватися.
Activation epoch сам по собі також не виключає intervening writes.

Corrected start не береться з latest global FOV. Потрібен причинний зв'язок
із попереднім output на цій source і boundary та узгоджений aspect. PRE-store
підтверджує лише переданий аргумент; POST-store — застосований store, але не
використання view. Якщо є лише writer evidence, contract обіцяє лише writer
continuity; visual acceptance лишається окремим критерієм.

## 3. Стани та правила відмови

| Стан | Умова входу | Дозволена дія | Завершення |
| --- | --- | --- | --- |
| Idle | Немає transition | Чинна policy boundary | Lifecycle event → AwaitEvidence |
| AwaitEvidence | Прийнятий lifecycle/request | Збирати потрібні факти; не вводити speculative mapping | Повний evidence → Mapping; invalidation → Cancelled |
| Mapping | Підтверджені start, destination, native sample lineage і domain | Одне перетворення поточного admitted sample | Terminal sample → Handoff; missing/conflicting fact → Cancelled |
| Handoff | Термінальний sample належить transition, destination ready | Поточний invocation записує destination-equivalent output і передає policy context | Наступний writer обслуговує destination owner |
| DestinationOwned | Lifecycle наступного режиму чинний | Та сама destination policy для кожного допустимого sample | Exit/rearm/source/settings event |
| Cancelled | Cut, stale identity/epoch, unsafe read, невідомий domain, непідтверджений sample | Прибрати лише adapter authority; повернути invocation чинному dispatcher | Нова незалежна activation → AwaitEvidence |

Fallback — відмова від нового mapping, а не обіцянка безшовності. Не повертати
автоматично raw, якщо інший чинний dispatcher уже має право коригувати invocation;
і не утримувати старий FOV, якщо він утратив authority. Для окремого невідомого
sample не створювати implicit resume старої activation. Відсутній corrected
start не компенсується target snap, timer, extrapolation або inverse HorPlus.

## 4. Adapter A: Cinematic → Dialogue

### Admission і endpoints

Прийнятий EXIT і recovery owner дають право розглядати source, але не доводять
Dialogue producer. Потрібна та сама механічна provider attribution та activation
lineage, що вже кваліфікують U2; нова policy не розширює source admission.

Native S/T беруться з поточної activation. Corrected start C0 — output попереднього
власника з достатнім boundary evidence, не receiver+0x230. Destination baseline B
має бути наданий/прийнятий lifecycle з явною семантикою native reference.
Не вважати B=S: S може бути transformed-start або проміжним cinematic sample.
Не вважати historical Gameplay baseline достатнім без встановленого lineage.

Destination endpoint визначає effective Dialogue selection:
Native → native zoom endpoint; Adaptive → D(B); Reduced → L(B);
Disabled → B. Gameplay correction застосовується відповідно до effective
Gameplay mode після setter-domain policy. Таблиця HorPlus endpoints діє лише
за його admission; AspectRecalculation не можна прирівнювати до тієї ж формули.

### Mutation і destination ownership

Кандидат інтеграції: adapter формує final output на writer boundary, а setter
залишає native sample незміненим для цієї activation. Це дає одне місце mutation
і не допускає повторного HorPlus. Проте це пропозиція, не доведена еквівалентність
setter correction для native consumers: поведінка camera field/view має бути
перевірена. Якщо потрібна setter-owned корекція, writer має явно отримувати
її sample-space/policy context; два adapters одночасно коригувати sample не можуть.

На endpoint recovery звільняється як зараз, але destination policy context
передається одному власнику в тому самому serialized invocation. Context не
зникає разом із exclusion. Ідентичність уже обробленої activation не дозволяє
звичайному classifier повторно створити AcceptedStart. Просте повторне ввімкнення
чинного Reduced classifier із baseline близько70 заборонено.

Наступний допустимий writer повинен застосовувати destination policy, а не
безумовний H(raw70). Native/Disabled лишаються різними. Destination context не
дає права коригувати всі наступні samples на тому самому receiver: current sample
повинен зберігати підтверджений lifecycle/domain. Коли native tick завершився,
продовження steady Dialogue authority потребує окремо встановленої межі lifecycle;
component pointer і retained terminal value самі по собі її не створюють.

## 5. Adapter B: Dialogue → Cinematic

Прийнятий ENTER зберігає own authored reference R та target C1=cinematicPolicy(R).
Ця lifecycle authority незалежна від успішного native blend commit — зокрема,
CreationGuardRejected не вимикає U1-A. Але вона не дає права вважати кожен raw
sample частиною поточної interpolation або authored UPDATE.

Для Mapping потрібні current ENTER activation, її initializer result, receiver,
source/generation/selection epoch та setter→writer evidence. Native S може бути70,
T — уже126.87, якщо гра отримала transformed ENTER operand. Не застосовувати
HorPlus до T повторно й не підміняти S authored90. C0 — qualified corrected
Dialogue output та aspect перед цією activation, C1 — власний cinematic target.

Протягом Mapping initial-establishment retention не має окремо вставляти C1
перед першим проміжним sample. Обидві гілки мають одного per-invocation owner;
неможлива послідовність retained C1 → raw S → gradual C1. У terminal invocation
mapping передає керування звичайній cinematic policy з тим самим C1, generation
і source admission. Усі чинні output admission guards зберігаються.

Якщо native consumer відхилив створення blend або provenance відсутня, adapter
не вигадує native interpolation. Чинний U1-A retention/dispatch лишається
fallback існуючої системи; цей fallback не оголошується smooth-transition fix.
Невідомий raw sample не стає authored UPDATE. Підтримка довільних U1-B updates
не додається через цей adapter.

## 6. Mapping та cancellation

Лише для validated branch `F=S+(T-S)w` та чисельно придатного span:
`w=(F-S)/(T-S)`, `O=C0+w(C1-C0)`. Це eased weight, не власний таймер.
Sample domain визначає adapter; не змішувати degree-linear weight і
tan(F/2)-normalized progress. Монотонність сама по собі не є admission.
S=T потребує explicit native terminal/cut evidence, а не ділення чи довільного
epsilon-переходу. Зміна duration/easing не інвалідує відомий eased weight сама
по собі, але зміна endpoints/branch/rearm вимагає повторної qualification.

Скасовують або завершують відповідний context: новий ENTER/EXIT, інша generation,
rearm навіть на тих самих адресах/endpoints, source/receiver change, current
output admission rejection, settings epoch або effective aspect change,
непідтверджений intervening writer, cut/branch/domain conflict, unreadable fields,
shutdown. Нова activation не успадковує old C0 без окремого lineage evidence.
Aspect change означає повторну policy evaluation на підтвердженій boundary,
а не інтерполяцію градусів із різних projection conventions.

Не існує правила «велика зміна FOV означає cut». Якщо наявні observation points
не розрізняють cut та blend, contract має явно вказувати непокритий випадок;
не оголошувати його безпечним через числовий interval. Різні потоки потребують
execution-order evidence; timestamps не встановлюють причинний порядок.

## 7. Acceptance і наступна інтеграційна межа

| Перевірка | Очікування |
| --- | --- |
| First/middle/terminal outputs | Той самий native eased weight; правильні destination settings |
| First post-handoff writer | Та сама endpoint policy; жодного проміжного raw/іншого target |
| Native vs Disabled | Native zoom збережений vs zoom вимкнений; Gameplay correction незалежна |
| Adaptive/Reduced | Policy-qualified B, відмінні endpoints; не target108.941 для всіх |
| Переривання, rearm, source/epoch change | Стара authority не відновлюється із cache |
| Чужий sample/два contexts на receiver | Receiver equality не дає exclusive ownership |
| Diagnostics OFF | Ті самі production decisions; немає залежності від trace budget |
| Native camera cut | Не згладжується без transition evidence |
| Active view | Реально спожиті FOV/aspect узгоджені; writer continuity не замінює visual PASS |

Необхідні sequence tests охоплюють обидва adapters окремо й compositional
Cinematic1→Dialogue→Cinematic2→Gameplay. Positive fixtures повинні містити
весь admitted sample stream, policy owner і перший post-handoff invocation;
negative fixtures — missing start/native activation/domain, same-address rearm,
interleaved setters, invalid output admission та camera cut. Просто monotonic
outputs або правильні endpoints не достатні.

Наступний обмежений integration scope: встановити production application record
для corrected start, provenance current ENTER activation→setter→writer та джерело
policy-qualified B, з перевіркою наявних hooks і без використання Diagnostics як
authority. Спершу визначити відсутні факти, які вже можуть дати existing boundaries;
цей документ не дозволяє нові hooks або автоматичний runtime capture.
Для visual proof потрібна конкретна невизначеність active-view consumption на
момент стрибка; повторення тих самих endpoint logs не закриє її.

## 8. Підсумковий статус дизайну

Структура двох adapters, endpoints, ownership/handoff, cancellation, fallback
і acceptance matrix визначені для перегляду. Production feasibility НЕ доведена:
немає qualified corrected/view start, current ENTER invocation provenance і
доведеного lifecycle для steady destination policy після завершення native tick.
Це точні integration gaps, а не вимога реконструювати весь закритий рушій.
Нові зміни коду, tests, ASI та runtime у цьому design batch відсутні.

Наступний запитаний feasibility audit завершено у
[U2_BOUNDARY_INTEGRATION_FEASIBILITY_2026-11-13.md](../reports/U2_BOUNDARY_INTEGRATION_FEASIBILITY_2026-11-13.md).
Production application і ENTER activation мають existing integration seams,
але consumption/current-invocation proof не надається їхнім існуванням.
Steady Dialogue authority незалежною lifecycle-подією на цих seams не встановлена.
Writer-only correction залишається кандидатом через derived setter optical
side effects та повторне читання camera field при reinitialization.

## Offline continuation after P0 status split — 2026-11-13

P0 is offline-validated and runtime-pending; its unresolved `camera+0x230`
producer/receiver relation does not prevent bounded deterministic U2 work.
The first offline regression batch now verifies natural Dialogue recovery
completion restores Gameplay baseline admission and that the current EXIT1
provider mapper rejects stale generation, foreign thread and missing evidence.
The existing positive EXIT1 fixture still verifies paired writer mapping and
recovery ownership. These fixtures test the adapter contract with explicit
evidence inputs; they do not claim production discovers that evidence
correctly or that the active view consumes the output.

Remaining offline matrix work: endpoint/midpoint values, interruption/rearm/
cancellation and Diagnostics OFF/ON parity across mapped and rejected cases.

### Implemented policy seam — 2026-11-13

`dialogue::ResolveDialogueDestinationTarget` is now the shared endpoint rule
for Dialogue policy: Native returns the supplied native target; Adaptive and
Reduced evaluate their established transforms from the supplied baseline;
Disabled returns that baseline. Invalid endpoints fail as NaN. The ordinary
Dialogue boundary transaction uses this resolver, so runtime and U2 policy
fixtures share one table. This does not manufacture a qualified U2 baseline.

The EXIT1 provider mapper remains a single writer owner when admitted:
`ProcessCameraWriter` returns immediately after its successful mapping, before
normal Gameplay recovery dispatch. Regression coverage asserts the mapped
output is not subsequently Gameplay RuntimeCombine transformed, and rejects
stale generation, foreign thread and missing provider evidence. These checks
establish only the existing invocation transaction with supplied fixture
evidence.

No production U2 blend mapping was enabled or changed. The corrected start is
still sourced from `receiver+0x230` without proof that it is the prior policy's
applied output; current ENTER activation→setter→writer provenance is absent;
the destination baseline for this activation is not established. Therefore a
native-eased weight and policy endpoint cannot yet be safely applied to a
corrected start in production. The existing projection-space mapper remains a
candidate and neither visual boundary is closed.

## Controlled runtime candidate prepared — 2026-11-13

The user authorized one controlled runtime experiment. A current-tree ordinary
Production Release candidate is prepared at
`build-artifacts/p0-u2-runtime-2026-11-13/STALKER2CameraTweaks.asi`,
SHA-256 `2D952B287FB9DB55A7FA789F767F5CA9E6395EF1D41F0BB6EDA5BF7C3D23295A`.
The only change for this experiment is Diagnostics-only correlation of
Gameplay baseline disposition with `writerContext` and input observation
sequence; production FOV policy/mapping is unchanged. The candidate is not
installed. The exact bounded primary sequence, two control windows, capability
checks and evidence return instructions are in
`../evidence/p0-u2-runtime-2026-11-13/operator-runbook.md`.

The primary capture targets Cinematic1→Dialogue and Dialogue→Cinematic2 in one
one-shot window. The two control phases compare post-recovery Dialogue with an
ordinary Gameplay Dialogue. Runtime reports must be assessed independently:
provider/activation association, branch/progress availability, per-policy
writer endpoint, Gameplay baseline disposition and visible result. A writer
output is not active-view evidence. No production transition fix is authorized
by this diagnostic run alone; after log analysis, implement only rules whose
corrected start, destination reference and invocation evidence are established.

Before implementing any production mapping change, the batch must settle the
destination-policy table and single-writer handoff for each boundary. Keep
Native, Disabled, Adaptive and Reduced semantics distinct; preserve current
admission guards and fail closed for missing activation, foreign/interleaved
writer, stale generation/epoch, and cut/domain ambiguity. Any case that needs
the real final producer or view consumption remains runtime-pending. This is a
scoped continuation, not closure of either visual boundary or U2.
