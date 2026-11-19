# Native FOV — аудит інваріанта незмінності

Дата: 2026-11-13. Read-only source/evidence audit поточного working tree.
Production/UI/формули/adapters не змінювалися. Tests/build/game/debugger не
запускалися. Цей звіт уточнює попередній Native FOV overlay audit новим доказом,
знайденим у вже збереженому capture, не у новій runtime-сесії.

## 1. Інваріант користувача та критерій перевірки

Гра визначає вихідний FOV до відповідної корекції мода. Мод читає/зберігає його
та обчислює окремий corrected output. Зміна mod settings або повторне читання
власного результату через native pipeline не є підставою оновлювати оригінальний
native reference. Нове game-origin значення може законно оновити reference.

Робочий camera field, змінений для доставки output, не обов'язково є immutable
reference. Критична заборона — повторно прийняти mod-derived value за новий
game-origin reference без domain/provenance distinction. Наявні tags та labels
оцінюються за цим змістом, а не за словом Native у назві структури.

## 2. Підтверджений runtime контрприклад

Evidence: [before-admission-repair.log](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log).
Це попередній кандидат, не новий F07C4FC1 capture. Ланцюжок:

| Час | Рядок | Подія |
| --- | ---: | --- |
| 18:03:39.745 | 583 | Dialogue AcceptedStart, selectedPolicy=Reduced, incoming70.0005, result_mutated=1, result_FOV=60.9227 |
| 18:03:39.745 | 587 | Transaction publication: native70.0005, setter output60.9227 |
| 18:03:39.760 | 596 | CameraWriter input60.9227 позначено Native/NativeRegisterInput; result99.2615 позначено Transformed/ModTransformResult |
| 18:03:39.760 | 597 | GameplayBaseline projection UPDATE eligible=true native60.9227 horPlus99.2615 |

Тут прийняття результату мода як baseline не лише можливе: journal прямо
фіксує UPDATE з60.9227 після відповідної setter mutation. Static native setter
data-flow та поточний source пояснюють цей шлях. Сам temporal/float match не
видається за повний causal proof усіх native instructions, але baseline UPDATE
і mod-origin значення в попередній transaction зафіксовані явно.

Поточний Batch1 пригнічує цей конкретний repeated AcceptedStart; у новому
runtime-182050.log цього dip немає. Це не змінило загальний domain admission
GameplayBaselineStore. Не стверджуємо, що60.9227 знову потрапило в baseline у
новому capture або що старий випадок пояснює обидва поточні visual defects.

## 3. Перевірка шляхів

| Шлях | Що зберігається до mutation | Що змінюється для output | Повторний вхід / native reference | Оцінка |
| --- | --- | --- | --- | --- |
| Gameplay HorPlus / RuntimeCombine | CameraWriterInput.incomingFov, result.inputFov | XMM0 для output+0x30; RuntimeCombine також camera aspect/flags | Observation безумовно тегує raw input NativeRegisterInput; baseline приймає за Gameplay eligibility | Local input/output separation є; game-origin invariant не забезпечений при upstream mutation |
| AspectRecalculation / replay path | Incoming writer FOV, passthrough observation | Aspect/flags та replay behavior | CameraWriter observation також NativeRegisterInput; baseline admission не містить upstream domain proof | Немає незалежної гарантії reference domain; нова конкретна contamination тут не доведена |
| Cinematic ENTER / U1-A | Authored before operand, generation R, optional Gameplay witness окремо | ENTER XMM0 стає transformed target C, переданим native consumer | Native blend може пізніше видавати C як writer raw; snapshot називає його native | Власний ENTER reference не перезаписується C у цій transaction; end-to-end input domain не зберігається |
| Cinematic writer retention/continuation | Поточний raw input окремо від stored R/C | Output XMM0 або pass-through; не завжди нове H(raw) | Raw blend samples публікуються як NativeNumericObservation навіть за transformed destination | UI/native факт не є доказом original reference; U1-A own R лишається окремим |
| Dialogue Zoom | XMM6 incoming та captured Candidate baseline B | XMM1 setter argument, native setter camera+0x230/derived optical state | Наступний writer отримує mod-derived setter result та може оновити Gameplay baseline | Контрприклад порушення є у saved runtime |
| U2 provider handoff | Activation S/T, receiver field як continuityStart; current paired sample | Final writer output через mapping до H(T) | Прямого запису output у GameplayBaseline всередині mapping немає; input domain до admission не доведений | Не знайдено direct result→reference assignment; відомий transformed-start risk |
| HorPlus recovery | Exit native target, cached corrected cinematic endpoint окремо | Mapped XMM0; під час recovery GameplayBaseline не оновлюється цим mapping | Після resume звичайний Gameplay admission знову тегує raw input Native | Recovery зберігає endpoint roles локально; не встановлює upstream domain для наступного sample |
| Camera-state/Overlay | Observation.inputFov або transaction.nativeInput | Окремі nativeFov/currentOutputFov facts у snapshot | Один Native FOV row отримує різні boundaries, у тому числі mod-influenced writer input | Не відповідає вимозі завжди показувати game-origin reference |

## 4. Конкретне місце втрати provenance

src/plugin/camera_writer_transaction.hpp:

- ProcessGameplayHorPlusWriter, рядки1172–1173: result.inputFov автоматично
  отримує FovSpace::Native/FovProvenance::NativeRegisterInput.
- Рядок1199: observation передається state.baseline.Project з eligibility
  coordinator==Gameplay.
- ProcessGameplayAspectRecalculationWriter, рядки1301–1314: incomingFov має
  аналогічні native tags і baseline projection на своєму admitted Gameplay path.

src/camera/gameplay_baseline.cpp:

- IsEligibleObservation, рядок29: перевіряє boundary/tags/result/source/aspect/
  publicationSequence та gameplayEligible; не перевіряє game-origin domain
  upstream setter або native initialization.
- Project, рядок62: baseline_.nativeFov=observation.inputFov.
- IsUsableGameplayBaseline перевіряє ті самі declarative tags і валідність,
  тому не може виявити value, якому попередній publisher уже присвоїв Native tag.

git diff HEAD для gameplay_baseline.cpp порожній; останній commit цього файла
56dbeb9 (release v2.0.0). Admission gap не доданий останньою правкою suppressing
AcceptedStart. Це не доводить, що будь-який caller у старому релізі міг викликати
саме новий runtime сценарій: baseline store і mutation callers — різні шари.

## 5. Де зберігається, а де втрачається original input

Local original зберігається коректно:

- ReplayManualTransition бере writerInputXmm0 до власної поточної mutation.
- ProcessAcceptedCinematicEnter зберігає authored R, corrected C та witness
  окремо; runtime capture before→after не переприсвоює before.
- TraceDialogueBoundary зберігає incoming XMM6 окремо від result.outputFov/XMM1.
- GameplayBaselineStore має окремі nativeFov/horPlusFov, а snapshot — окремі
  nativeFov/currentOutputFov. Сам факт окремих полів не забезпечує provenance.

Domain knowledge втрачається між native calls: native camera+0x230 і blend
target/start не мають mod-owned tag. Новий raw register input публікується як
Native незалежно від попередньої mutation. Це semantic reclassification,
а не обов'язково overwrite тієї самої C++ змінної.

Чинний docs/SAFETY_INVARIANTS.md уже застерігає, що native-register provenance
описує boundary і не доводить game-authored значення. Отже, локальні tags
відповідають цій слабшій задекларованій гарантії; вони не виконують сильнішу
вимогу користувача щодо original reference. Не вважати слово Native доказом
виконання інваріанта або підставою оголосити current code коректним.

## 6. Чи є підтверджена подвійна корекція

Reduced setter mutation плюс Gameplay aspect correction — дві різні свідомі
операції. Послідовність60.9227→99.2615 не доводить double HorPlus; вона доводить,
що already-modified setter-domain число може стати native baseline.
Прямий ланцюжок H(H(original)) у stable Gameplay у наданих captures не доведено.
Не ототожнювати цей інваріант із вимогою вимкнути одну з цих policies навмання.

Cinematic raw126.87 також не доводить повторний H(126.87): continuation може
пропускати його без нової арифметичної корекції. Проте native overlay label для
цього sample вже не гарантує original game-origin90.

## 7. Зв'язок з U2 та disposition

Confirmed: end-to-end original Native reference invariant не забезпечений
поточними publishers/admission; для Dialogue→GameplayBaseline є saved runtime
контрприклад. Confirmed: current U1-A authored R та current Gameplay input/output
поля фізично розділені. Unknown: вплив contamination на обидва visual jumps
після останнього Batch1 та causal stable-Gameplay HorPlus toggle behavior.

U2 не читає UI, але використовує native receiver field як continuityStart та
GamePlayBaseline eligibility; ці факти не стають original-domain proof від
того, що їх читає adapter. Нові adapters не повинні успадковувати цю підміну.

Наступна залежна зміна потребує явного правила native-reference admission,
mutation lineage та domain-preserving handoff. Це не дозвіл додати global cache,
заморозити reference, відновлювати original через inverse HorPlus або вважати
однакове число доказом domain. Число оновлюється за новим game-origin evidence,
а не просто тому, що native engine знову передав його в регістрі.

Повна реалізація adapters залишається відкладеною до цього рішення. Це аудит
інваріанта, не production fix; visual acceptance обох переходів лишається OPEN.
