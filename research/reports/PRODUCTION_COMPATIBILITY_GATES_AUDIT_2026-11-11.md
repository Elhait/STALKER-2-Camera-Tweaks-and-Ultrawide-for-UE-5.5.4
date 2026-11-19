# Аудит production-механізмів сумісності

Дата: 2026-11-11. Статус: статичний аудит завершено; виправлення та runtime-приймання не виконані.

Подальший стан 2026-11-11: CG01–CG03 реалізовані й офлайн-перевірені в [окремому звіті](ZOOM_COMPATIBILITY_CG01_CG03_IMPLEMENTATION_2026-11-11.md). Ручне приймання pending. Нижче збережено стан і докази саме на момент аудиту.

## Обсяг і докази

Перевірено проєктний `src/`, реєстр `tools/build/production_sources.json`, CMake-профілі та фактичний запис компіляції `runtime.cpp` у `build-artifacts/cm/candidate/production-tOFF-dOFF/compile_commands.json`. Охоплено пошук SHA/version gates, game RVA, сигнатур, конкретних байтів, відносних зміщень, callback-контрактів, локальних відмов та залежностей компонентів. Сторонні бібліотеки не переаудитовано; їхні версії не є версіями гри.

Production-код, Git, ASI та конфігурацію не змінено. Компілятор, тести, гру, debugger і graphics fixtures не запускали. Поточний EXE не дизасемблювали; цей аудит не підтверджує сумісність Zoom-сигнатур із ним.

Ідентичність наданих runtime-спостережень:

- ASI: `A9A38A68E4F69FFB6C0127E4C8A430268C79C60AB7619BCF008E2805B91F9842`.
- Гра: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Перший прочитаний журнал: 14:08:10–14:08:11, лише startup у головному журналі, без зафіксованого cinematic ENTER.
- Оновлений журнал другого запуску: startup 14:17:27; та сама SHA-відмова Zoom; о 14:18:34.471 є `CINEMATIC_LIFECYCLE_ENTER_CONTEXT generation=1` та `Global cinematic ENTER: aspect=3.55556 authoredFov=90 transformedFov=127.119`.

Другий журнал перезаписав файл за тим самим шляхом. Перший запуск тут представлено попередньо прочитаними записами, а не окремо збереженим повним артефактом. Позитивний ENTER другого запуску не пояснює відсутність ENTER у першому.

## Підтверджені дефекти

| ID | Місце | Механізм і наслідок | Виправлення |
| --- | --- | --- | --- |
| CG01 | `src/plugin/runtime.cpp`, `kZoomTransitionGameSha256`, `InstallZoomTransitionObservation`, виклик в `Initialize` | Єдина дозволена SHA — `61BC…D56B` (історичний образ 2.0.6). Інша SHA відхиляється до пошуку обох сигнатур. Навіть помилка обчислення SHA через `gameHashAvailable && …` повністю пропускає resolver. Контракт інструкцій може залишатися валідним, але компонент не має шансу встановитися. | Зробити рішення залежним від унікального пошуку та перевіреного native-контракту. SHA залишити лише provenance. Перед видаленням gate забезпечити достатність resolver-перевірок; додавання нового дозволеного хешу не усуває дефект. |
| CG02 | `runtime.cpp`, `diagnosticStatus` в `Initialize` | Глобальний `diagnostics::SetEnabled(true)` незалежний від Zoom. Але підсумкове `Diagnostics=DISABLED` лишається після відмови Zoom. Другий запуск безпосередньо демонструє активні cinematic-логи при такому статусі. | Окремо показувати master diagnostics selection і capability/status кожного observer; розрізняти disabled, unresolved/ambiguous, contract failure та hook-install failure. |
| CG03 | `runtime.cpp`, `snapshotZoomActive`, `snapshotZoomHold`, `PublishCameraStateSnapshot`; `src/camera/camera_state_publication.hpp`, `neutralContext` | Після відмови observer початкове `zoomActive=false` використовується як відсутність Zoom. Наявність capability не входить у predicate нейтрального FOV; три стабільні zoom-семпли з рештою відповідних ознак можуть стати `snapshot.evidence.gameplayFov`. Це конкретна помилка достовірності Camera State, а не доказ зміни cinematic correction. | Передавати доступність/валідність Zoom observation в умови підтвердження нейтрального FOV. Без потрібного доказу не публікувати новий нейтральний FOV як Current; зберігати historical/unavailable відповідно до контракту. |

`snapshot.evidence.gameplayFov` і `g_gameplayBaselineStore` — різні власники. Перевірені використання Zoom active/hold ведуть до Camera State publication; зв'язок SHA-відмови з вимкненням основної cinematic FOV-корекції не встановлений. Формулювання `production behavior unchanged` при відмові Zoom приховує втрату його функціональних спостережень і потребує точнішого тексту.

## Інвентар resolver і gate

| Компонент | Фактичний gate | Висновок та межа доказів |
| --- | --- | --- |
| Gameplay writer | `src/gameplay/gameplay_camera.cpp`: один повний match у `.text`, wildcard displacement JNZ, Zydis-перевірка `MOVSS [RBX+0x30], XMM0` | SHA/version gate не знайдений. RSI source і додаткові поля підтверджуються повною сигнатурою та прийнятим native-контрактом; декодування однієї store-інструкції саме по собі не доводить усі семантичні властивості source. |
| Cinematic aspect | `cinematic_aspect.cpp`: один match, межі `.text`, executable, prefix/immediate, decode `MOV [RAX+0x254], imm` | SHA/version gate не знайдений. `+0x19` є зміщенням усередині знайденого шаблону, не абсолютним RVA гри. |
| Cinematic ENTER/EXIT | `runtime.cpp`, `Validate*Boundary`/`ResolveCinematicFovCallsites`: cardinality, span, MOVSS operands, rel32 CALL, bounded vcall-byte search, executable shared target | SHA/version gate не знайдений. Legacy EXIT має fallback до indexed лише при нульових legacy matches; неоднозначний legacy не ігнорується. Це fail-closed. Валідація не доводить, що кожна сцена гри проходить саме ці callsites. |
| Dialogue | `InstallDialogueBoundary`: один match, конкретні шість байтів у `match+9`, hook success | SHA/version gate не знайдений. Перевірка байтів і контекстна сигнатура є native-контрактом; відсутність Zydis сама по собі не дефект. Межа hook лежить у matched signature. |
| Zoom IN/OUT | Exact SHA до пошуку; потім один match кожної сигнатури та три байти `UCOMISS` у `+13` | CG01. Повні 16-байтові шаблони вже кодують два MOVSS і UCOMISS, але перед переносимим admission слід явно перевірити spans, operands та контракт потрібних полів/контексту. Немає підстав сліпо видалити SHA і вважати portability доведеною. |
| Auto language reader/free | `src/overlay/localization/game_language_reader.cpp`: PE/layout validation, унікальні сигнатури reader/anchor, memory ranges, function entry через PE exception table, точний prologue, атомарна публікація пари | SHA/version/fixed-game-RVA gate не знайдений. Точний prologue — залежність від форми машинного коду: зміна компілятора може спричинити безпечну відмову. Fallback English з повторною спробою; детальна причина resolver-відмови не потрапляє в окремий журнал. Це прогалина спостережуваності, а не доказ некоректного reader. |
| Overlay/input/window discovery | Win32 window eligibility, owner dispatch/readiness, DComp initialization, API/resource results | Gate за версією або SHA гри не знайдений. HWND/focus/visibility та resource failure — окремі умови придатності, не patch allowlist. Невдача Overlay не повинна вимикати camera core. |
| Три Post-EXIT research probes | Exact SHA `E7B4…9293` і фіксовані RVA `0x318DCD4`, `0x3DB2CE7`, `0x32B779D` | Ізольовано `POST_EXIT_FOV_STATE_CONSUMER_TRACE`, `POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE`, `POST_EXIT_FOV_PRODUCER_STORE_TRACE`. Цих defines немає в перевіреному production compile command/CMake definitions. Це прив'язані до образу дослідницькі інструменти, не додаткові активні production gates. Їх не можна переносити у production без resolver. |

Проєктний пошук не виявив runtime-порівнянь номера патча, PE timestamp або FileVersion як умов встановлення production-модулів. Поля версії власного ASI, версії Windows/DX API, shader data і vendor-константи не є gates версії гри.

Точні байти та object offsets потрібні для fail-closed native-контракту. Wildcard усіх байтів або відмова від перевірки полів не є коректним усуненням залежності від білда. Треба відділяти адресу функції в конкретному образі від структури потрібної інструкції та перевірених field offsets.

## Залежності та локальність відмов

- Cinematic aspect та ENTER/EXIT встановлюються як одна транзакція через `cinematic_initialization` і `CommitCinematicComponents`. Gate відкривається після активації всіх трьох hooks; failure викликає rollback. Часткове коригування без lifecycle-контракту не є допустимою альтернативою.
- Non-native Dialogue вимагає cinematic lifecycle observation та Gameplay recovery observation. Відсутність залежності явно повідомляється; це обґрунтований функціональний dependency, а не довільне прив'язування до патча.
- Gameplay writer має локальний failure; відмова не зупиняє весь startup. Локальна доступність camera modules відображається в initialization summary.
- Zoom observation встановлюється незалежно від Diagnostics selection, бо постачає дані Camera State. Відмова не вимикає основні camera hooks, але несе CG03; загальний текст про незмінну поведінку неповний.
- Auto language resolver повертає English при відмові; camera corrections не залежать від його успіху. Діагностичні причини fallback варто зробити bounded і видимими.

## Окреме розслідування відсутності ENTER

Статус: **причину не встановлено**. Другий запуск того самого ASI на тому самому EXE з тією самою Zoom-відмовою має прийнятий ENTER і корекцію `90 → 127.119`. Це суперечить поясненню «Zoom SHA mismatch завжди блокує ENTER».

Перші callback-перевірки `TraceCinematicEnter` можуть повернутися без запису: закритий hook gate, stopping, invalid prepared selection; rejected accepted-enter transaction також завершується до загального ENTER-маркера. Startup resolver PASS не доводить виконання callback. Наявна діагностика не відрізняє «hook не викликано» від раннього return у цих місцях. Відсутність запису прийнятого ENTER не слід називати доказом, що raw callback не спрацював.

За нового відтворення потрібні bounded counters/first-occurrence reasons на самому вході callback і його ранніх виходах, статус commit/gate та ідентичність процесу/артефакту. Вони мають лишатися observational, не змінювати синхронізацію або settings/generation authority. Нових hooks чи polling для цього не потрібно. Даних поточного першого журналу недостатньо для вибору однієї причини.

## План виправлення та перевірок

1. Зафіксувати production-інваріант: зміна SHA/номера патча або неможливість обчислити SHA за незмінного валідного native-контракту не змінює рішення про встановлення компонента. Research probes лишаються явно ізольованими.
2. Винести Zoom resolution в тестований компонент із bounded executable spans, unique IN/OUT matches, перевіркою повної послідовності/register operands/offsets та paired install/rollback. Перевірити retained game images і актуальний образ у погодженому static scope. SHA прибрати з admission та його caller short-circuit; не замінювати новою allowlist.
3. Додати явну capability Zoom observation та коректну семантику unavailable для Camera State/нейтрального FOV. Відокремити її статус від Diagnostics selection. Повідомляти деградацію і при Diagnostics OFF без щокадрового log spam.
4. Детерміновані перевірки: той самий executable code з різними SHA/metadata і недоступною SHA дає однакове admission; missing/duplicate/truncated/wrong-register/wrong-offset patterns відхиляються; failure другого hook не залишає активного першого. Не тестувати лише наявність прапорця або імені resolver.
5. Окремо перевірити presentation: без Zoom capability три стабільні zoom FOV не стають підтвердженим neutral GameplayFov; при валідних zoom edges зберігається hold/release semantics. Diagnostics OFF/ON не змінює production-рішення.
6. Після targeted CPU-only regression — релевантний non-graphics canonical suite та окремий ASI; потім погоджений ручний запуск на актуальному білді. Offline PASS не закриває runtime acceptance. Перша відсутність ENTER лишається окремим відкритим пунктом і потребує її власних доказів.

Цей звіт завершує аудит, а не виправлення CG01–CG03. Релізна сумісність із поточною грою та причина першого пропуску ENTER залишаються непідтвердженими в зазначених межах.
