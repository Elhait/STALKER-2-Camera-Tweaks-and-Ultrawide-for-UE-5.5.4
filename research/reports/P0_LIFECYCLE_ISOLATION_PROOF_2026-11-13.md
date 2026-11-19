# P0 — перевірка lifecycle references та локальної ізоляції

2026-11-13. Read-only перевірка current working tree і збереженого capture.
Production edits, build/test/game/debugger та нових hooks не виконувалися.

## Де lifecycle вже зберігає ролі

**CONFIRMED.** `CinematicWriterAssociation::BeginGeneration` зберігає authored
R, transformed C і optional Gameplay witness окремо в state generation
(`src/cinematics/cinematic_fov.cpp:25–54`). `InvalidateGeneration` скасовує
admission на EXIT; `ProcessAcceptedCinematicExit` окремо переносить native EXIT
request і cached transformed endpoint у recovery state. Новий ENTER створює
власний R і не використовує старий witness як target.

**CONFIRMED.** `PostExitProviderHandoff::ObserveArmer` зберігає provider T,
S, receiver, component, thread, cinematic generation і settings epoch; rearm
скидає record. `CompleteRecovery` залишає identity для пригнічення повторного
Dialogue admission. Ці механізми вже дають scoped request evidence; додавати
global FOV cache для збереження R/C/T немає підстав.

**Межа:** provider T — target окремого запиту, не Dialogue baseline; S
прочитано з mutable camera field. Cinematic R стосується cinematic request,
а не автоматично поточного Gameplay input. Ні R, ні T не доводять, що
конкретний writer sample після native operations походить від певного setter.

## Контрприклад для повного P0

`tests/camera/gameplay_baseline_harness.cpp --native-origin-contract` містить
дві гілки після однакового baseline70.0005. Обидві передають `Project()`
ідентичний payload: input60.9227, result99.2615, source/aspect/sequence і
`NativeRegisterInput`. Зовнішня annotation першої гілки — результат Dialogue
mutation; другої — незалежний game change. Потрібні різні рішення, але
поточні аргументи store не містять цієї різниці.

Наявні R/C/T не усувають контрприклад: друга гілка може мати ті самі lifecycle
records, поки native code змінив поле на те саме число. Блокування всього
source/interval після Dialogue затримає genuine Gameplay change; дозвіл raw
без доказу прийме recorded mod-result. Значення, source, coordinator, час або
збіг endpoints окремо не створюють origin evidence. Тест моделює admission
seam; positive fixture має зовнішню annotation і не доводить production
game-origin callback.

## Чому callback lifecycle недостатній

**CONFIRMED runtime:** перед Batch1 Dialogue Reduced видав60.9227, writer
прийняв input60.9227, baseline оновився до60.9227
([лог, рядок583](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L583),
[рядок597](../evidence/u2-transition-policy-2026-11-13/before-admission-repair.log#L597)).
У поточному writer `EvaluateGameplayContextChange` може скинути active Dialogue
до `state.baseline.Project` (`camera_writer_transaction.hpp:1122–1127,1199`),
тому поточний Dialogue phase на admission не є достатньою causal міткою.

**CONFIRMED source:** production provider tracker парує validated activation і
pre-setter sample з writer за identity/thread/generation/settings і sample
(`post_exit_provider_handoff.hpp:115,144`), але не бачить усіх native writes між
ними. Setter-return trace `runtime.cpp:3623` є Diagnostics gated. Навіть його
production activation встановить setter return, але не останній def-use input.

Попередній native data-flow для цього game image описує derived GetCameraView,
який може переобчислити `camera+0x230` перед writer
([native data-flow](POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md)). Це можливий
проміжний write, не доведений шлях саме зафіксованої сцени. Writer PRE/POST
показує store output+0x30, не історію producer field.

## Рішення та мінімальна відсутня подія

**Часткова ізоляція можлива:** зберігати R/C і T scoped до generation/activation,
не перезаписувати їх repeated observations і не створювати global store. Це вже
частково реалізовано. Можна ізолювати конкретну mutation, коли є підтверджений
setter→writer lineage. Наявні observations не доводять такого lineage для
всіх потрібних callbacks і тому не забезпечують повний P0.

**Потрібне native evidence:** останній producer/write для конкретного
`camera+0x230`, який supply конкретний writer input після setter return, разом
із залежністю producer від setter output або незалежного game input.
Найменший наступний спосіб — bounded hardware write-watchpoint на поле
фактичного receiver у контрольному переході; записати instruction, receiver,
thread, call path, значення та порядок від setter return до writer. Якщо producer
обчислює нове значення, потрібні його inputs. Watchpoint — дослідницьке
evidence, не production mechanism. Після встановлення coverage можна вибирати
structural resolver/наявний hook або новий point.

ENTER2 retention та U2 endpoint policy залишаються окремими підтвердженими
дефектами; їхній дизайн можна готувати незалежно без неперевіреного baseline.
Цей звіт не доводить спільної першопричини або visual correctness.
