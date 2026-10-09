# Cinematic skip / 0 FPS та FOV jump — read-only аудит

Дата: 2026-10-09. Обсяг: поточний production source, наданий runtime log, відповідні Overlay/Startup logs та скриншот. Production code, конфігурація й встановлені артефакти не змінювалися. Тести, build, профілювання та гра не запускалися. Створено лише цей звіт.

## Висновок

Підтверджено надлишкове діагностичне логування стабільного restoration state та синхронні записи/примусові flush на callback thread. Їхній внесок у 3977,5 мс frametime **не виміряний**; називати логування встановленою причиною зависання зараз не можна.

Окремо підтверджено розрив cinematic writer output під час generation 2: target залишається 123,105°, але output стрибає до 95,2186° при валідній association та `CinematicActive`. Поточний source пояснює це завершенням initial establishment і відсутністю дозволеної continuation. `Dialogue.Mode=Disabled` не є достатнім поясненням: Dialogue handler у цьому ownership відмовляється від обробки незалежно від вибраної політики.

## Ідентичність та межі evidence

- Runtime log: `E:/Steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl/Stalker2/Binaries/Win64/STALKER2CameraTweaks.log`.
- Log SHA-256: `4DDC969AD30910D9698FC46BA947A17821EB41F6A028407877F0D356B8F360B4`; 433642 bytes; 3024 timestamped records. Хеш залишився однаковим після аналізу.
- У log зафіксовано ASI SHA-256 `34816FF486AF670791CB9A4255958821610142FC527868791CF8606136F95A3E`. Такий самий хеш мають встановлений ASI та поточний repository-root `STALKER2CameraTweaks.asi`.
- Game SHA-256 із runtime identity: `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`; окремо executable не хешувався.
- Startup configuration: Gameplay `RuntimeCombine`, Cinematics `Auto / GameplayHorPlus`, Dialogue `Enabled=true / Disabled`, Diagnostics `Enabled=true`. У runtime aspect=3, тобто 27:9; retained native Gameplay baseline=95,1311°. Це спостережене значення, а не доказ точного slider setting.
- Overlay log підтверджує приватний DComp renderer, без game swapchain/factory hooks, та останню зміну `VISIBLE → HIDDEN`. QPC закриття передує першому ENTER на 11,422327 с; пізнішого відкриття в наданому Overlay log немає.
- Скриншот показує 0 FPS та 3977,5 мс frametime, але не містить timestamp. Точний skip/input event у log не маркований, тому прив'язка цього кадру до конкретної generation не встановлена.
- Робоче дерево має попередні staged/unstaged зміни. Source перевірений у фактичному поточному стані; однаковий ASI hash не є самостійним доказом його відтворюваної збірки з кожного поточного source-файлу.

## Findings

| ID / severity | Статус | Evidence | Причина / механізм | Мінімальне підтвердження |
| --- | --- | --- | --- | --- |
| P01 / P2 | Confirmed: надлишкове логування; вплив на stall unknown | 2583 `RESTORATION_STATE` із 3024 records (85,42%); усі мають `UPDATE aspect=3 valid=true source=0x1a939733400`, відрізняється sequence. Це приблизно 295938 bytes із 433642 | `ReplayManualTransition` перевіряє `aspectRestorationObserved`, але ігнорує `aspectRestoration.changed`. Сам store правильно визначає semantic change; adapter логуватиме кожне спостереження | Причина повторів уже встановлена source + log. Для впливу на FPS — той самий сценарій/ASI з Diagnostics ON/OFF та зовнішній thread/File I/O trace під час stall |
| P02 / P1 potential stall | Confirmed: blocking-capable I/O на callback thread; багатосекундна затримка hypothesis | Синхронний `basic_logger_mt`; 7 flush markers: 4 EXIT + 3 first-post-EXIT writer. Під час gen2→gen3→gen4 — 55 records у 26 мс | `Log` відкладає запис лише до звільнення correction gate. Guard виконує deferred actions на тому самому thread. `LogImmediate` створює stream/string; sink бере mutex і викликає `fwrite`. Probe додатково викликає `OutputDebugStringA` та `fflush` | Зовнішній trace/stack у зависанні: callback → logger/sink/CRT I/O або debugger wait. A/B Diagnostics OFF звужує пошук, але сам по собі не відрізняє I/O від інших diagnostic costs |
| P03 / P2 | Confirmed evidence gap | Єдиний `PERF_RUNTIME_SUMMARY` — 00:06:33.342, перед першою катсценою. `log_calls=0`, хоча вже є restoration logs | `ScopedRuntimeLog` вимірює тільки всередині активного `ScopedRuntimeMutex`. Deferred logs виконуються після виходу з цих scopes. Correction gate, sink wait та probe flush не входять у цей вимір. Summary одноразовий | Для причини stall потрібен trace в самому skip window. Не трактувати `log_calls=0` як відсутність логування чи виміряний нульовий overhead |
| P04 / P2 potential contention | Confirmed shared lock; участь у цьому stall hypothesis | `OverlayWindowProc` бере `g_rendererMutex` до і після native WndProc; presenter утримує той самий mutex протягом `Renderer::Render` і geometry/resource work | Game window message може чекати presenter, якщо він затримується всередині graphics/resource operation. Окремий renderer thread сам по собі цього не виключає. У цьому прогоні UI hidden, тож постійне UI rendering не підтверджено | Той самий сценарій із startup `Overlay.Enabled=false`; за потреби thread trace, що показує window thread у wait на mutex та власника в DComp/D3D11. Відсутні device-loss/resize errors у наданому Overlay log |
| P05 / P3 | Confirmed hot-path work; суттєвий cost unknown | Shared snapshots/association/memory validation активні у production; viewport query виконується для HorPlus/RuntimeCombine поза `CinematicActive` | Callback бере correction gate та кілька коротких state locks. Window lookup може перейти до `EnumWindows`; fallback display lookup використовується Auto cinematic resolution. Deferred logging використовує `vector<function>`/captured tuples. Обсяг queue у callbacks не є frame-history backlog | Sampling/CPU attribution саме під час відтворення. Немає підстав оптимізувати ці операції лише за фактом їхньої наявності |
| F01 / P1 | Confirmed cinematic output discontinuity; не доведена Dialogue-caused ownership loss | 00:09:29.024: generation 2, association valid/conflict=0, `CinematicActive`, target=123,105°, output=95,2186°, `authored_baseline_departure`, `PassThroughUnclassified` | 0,05° initial Gameplay-baseline guard завершує establishment, коли input відходить від 95,1311°. Continuation не дозволяється: predicate вимагає semantic equality EXIT native target=95,1311° та authored ENTER=90° (різниця 5,1311° > 1°). Output після цього pass-through | Production-adapter replay rapid EXIT→ENTER із тією самою validated association та inputs 95,1311→95,1534→95,2186→…→123,105. Потім повторити runtime-сценарій. Не послаблювати глобальні tolerances або foreign-source захист без окремого контракту |

P2 для P01 означає дефект emission/deduplication, а не доведений performance release blocker. P1 для performance-гіпотез описує можливий вплив при підтвердженні механізму; причина спостереженого 0 FPS залишається unknown.

## Частота та поведінка logging paths

| Path | У цьому log | Умови та emission |
| --- | --- | --- |
| Restoration | 2583 | Diagnostics ON; adapter обходить `changed`, тому фактично per-observation |
| `RUNTIME_COMBINE` | 65 | Diagnostics ON; compare snapshot із float epsilon 0,001. 61 record — recovery interpolation, 4 — Gameplay FOV changed. Native input змінюється, тому під час recovery output залишається стабільним, а записи повторюються |
| `CAMERA_FOV_STATE` | 55 | Diagnostics ON; semantic projection changed. Gen2 native/output ramp породжує багато справжніх value changes |
| `CINEMATIC_ASSOCIATION_WRITER_STATE` | 25 | Diagnostics ON; structural/policy changes або semantic FOV difference >1°. Не кожний sample |
| `GAMEPLAY_FOV_SOURCE_TRACE` | 25 | Diagnostics ON; change-based trace |
| `CAMERA_STATE_SNAPSHOT` | 8 | Diagnostics ON; semantic snapshot change |
| Global cinematic ENTER / EXIT | 4 / 4 | Базові lifecycle logs залишаються й без Diagnostics |
| `CAMERA_LOG_PROBE` | 11 records, з них 7 forced-flush markers | Diagnostics ON; first writer markers та superseded marker не всі самостійно викликають flush |
| Continuation dump | 0 | Якщо trace активований, fixed capacity=512 samples; завершений trace форматується одним блоком. Такий dump не спостерігається у цьому log |

Максимум у календарну секунду — 127 records (00:09:38); у 100-мс bucket — 56 (00:09:33.1). Час у log — emission time, зокрема після release gate, а не автоматично timestamp кожного camera sample. `fflush` тут означає CRT buffer flush; це не `FlushFileBuffers`/fsync і не доказ фізичного durable write для кожного record.

## Lifecycle та паузи

| Час | Подія / результат |
| --- | --- |
| 00:06:53.199 → .204 | gen1 ENTER → first writer за 5 мс. Native input=95,1311°, target/output=123,105°; initial establishment authorized. Початкове виправлення спрацювало |
| 00:09:28.983 → .987 → .992 | gen1 EXIT → gen2 ENTER за 4 мс → first writer за 5 мс. Output спершу 123,105° |
| 00:09:29.001 | gen2 input=95,1534° (baseline delta 0,0223°); output ще 123,105° |
| 00:09:29.024 | gen2 input=95,2186° (delta 0,0875° > 0,05°); establishment ended; output падає з 123,105° до 95,2186° (27,8864°). Association та ownership залишаються валідними |
| 00:09:29.981 | Native/output сходяться до 123,105°. Це pass-through convergence, а не відновлення Dialogue correction |
| 00:09:33.160 → .164 → .169 → .177 | gen2 EXIT → gen3 ENTER → gen3 EXIT без recovery owner → gen4 ENTER. Ланцюг швидкий, але скінченний |
| 00:09:33.186 | gen4 native/target/output=123,105°. Association valid; establishment ended, pass-through. Числова рівність сама по собі не доводить активне утримання target |
| 00:09:37.787 → 00:09:38.787 | gen4 EXIT → native recovery endpoint=95,1311°, recovery completed; RuntimeCombine output=123,105° |

Найважливіші emission gaps: 155779 мс (00:06:53.204→00:09:28.983), 3179 мс (00:09:29.981→00:09:33.160), 4569 мс (00:09:33.218→00:09:37.787). Це **не встановлені frame stalls**. У двох останніх проміжках writer sequence зростає відповідно 3220→3381 та 3385→3614; навіть у довгому першому gap він зростає 2581→3170. Отже, записи не є heartbeat і відсутність emission не означає відсутність callbacks. Без timestamp кожного sample це також не виключає окремої clustered pause всередині gap.

У наданому log немає повідомлень про exception/error/failure/rejected, queue overflow або нескінченну retry-послідовність. Відсутність таких повідомлень не доводить відсутність native/driver stall.

## Межі перевірених performance paths

- Callback/transaction paths camera writer, cinematic ENTER/EXIT та Dialogue не запускають signature scan або worker wait. `FindAll`/resolver work належить installation. Worker stop waits — окремі worker/control paths. У цьому сценарії немає source evidence, що skip запускає їх повторно.
- Camera gate звільняється **до** deferred logger I/O. Це зменшує час його утримання, але не забирає I/O із callback thread. Log sink mutex незалежний від correction gate.
- Continuation samples мають fixed capacity 512; overflow не збільшує storage. Deferred actions виконуються при завершенні поточного guard, не накопичуються як frame backlog. Дорогий bulk-format можливий лише для активного trace; у наданому run такого emission немає.
- `PERF_RUNTIME_SUMMARY` охоплює лише ранні 24563 мс після першого sample: 1310 Gameplay samples, Dialogue samples=0, cinematic callbacks=0. Найбільший виміряний wait серед перелічених locks — 1200 нс, camera-state hold max=113800 нс. Це evidence для раннього Gameplay window, не для skip і не для всіх locks.
- `ProcessDialogueBoundary` повертає `NonGameplayReset` при coordinator != Gameplay **до** читання selected policy та застосування transform. Під час зафіксованого FOV jump coordinator=CinematicActive. Немає accepted Dialogue lifecycle або ownership переходу в Dialogue у цьому window. В іншому, справді Gameplay-owned діалозі Disabled закономірно означає відсутність Dialogue intervention; це окремий випадок.

## Source anchors

Шляхи відносно canonical repository; номера рядків — стан на дату аудиту.

| Source | Перевірені функції / місця |
| --- | --- |
| `src/plugin/runtime.cpp` | `TraceGameplayAspectRestorationState` :838; `LogImmediate` :866; `Log` :880; `FlushCameraLogProbe` :921; `DeferCameraLogProbeFlush` :949; continuation dump/finish :1024–1072; `TraceCinematicEnter` :3047; `TraceCinematicExit` :3262; `TraceDialogueBoundary` :3657; `PublishCameraFovTransaction` :4266; `ReplayManualTransition` :4510, logging bypass :5118; semantic snapshot gate :5994; summary consumption :6275; sync logger setup :6428 |
| `src/plugin/camera_correction_gate.hpp` | `Guard::~Guard`, `DeferUntilReleased`: deferred actions після unlock, той самий thread, dynamic vector/function storage |
| `src/plugin/camera_writer_transaction.hpp` | restoration publication :1046; cinematic establishment/dispatch :1632–1680; RuntimeCombine synchronization and recovery |
| `src/camera/gameplay_aspect_restoration.cpp` | `GameplayAspectRestorationStore::Update` :48: semantic `changed` окремо від increment sequence |
| `src/cinematics/cinematic_fov.cpp` | Gameplay baseline tolerance :15; `AuthorizeInitialEstablishmentRetention` :151; `ResolveCinematicWriterSample` :249 |
| `src/plugin/cinematic_lifecycle_transaction.hpp` | `ProcessAcceptedCinematicEnter` :160; reentry/continuation classification :238–267; EXIT continuity staging :414 |
| `src/gameplay/gameplay_state.cpp` | `IsCinematicOwnershipContinuation` :18: native EXIT/authored ENTER equality та transformed-target equality |
| `src/plugin/dialogue_boundary_transaction.hpp` | `ProcessDialogueBoundary` :118: NonGameplay early return перед policy handling |
| `src/diagnostics/performance_telemetry.cpp` | `ScopedRuntimeMutex` :111; `ScopedRuntimeLog` :163; single summary :190 |
| `src/diagnostics/cinematic_continuation_trace.hpp/.cpp` | capacity=512; bounded Record; Finish/Discard |
| `src/overlay/composition_runtime.cpp` | `OverlayWindowProc` :578; presenter lock across Render :826–869; `NotifyOverlayCameraStatePublished` :926: asynchronous/coalesced visible redraw |
| `src/overlay/renderer_runtime.cpp` | DComp BeginDraw/EndDraw/Commit :1147–1194; `RenderImpl` :1600, settings panel лише коли visible |
| `src/platform/win32/viewport.cpp`, `window.cpp` | `ReadCurrentProcessClientViewportAspect`, `ReadClientViewportAspect`, `FindCurrentProcessWindow` |
| `external/spdlog/include/spdlog/sinks/base_sink-inl.h`, `basic_file_sink-inl.h`, `details/file_helper-inl.h` | sink mutex :26; file write :36; CRT fflush :72, fwrite path :96–105 |
| `build.cmd` | production source list :61–65; separate diagnostic macro profile :42–49 |

Найсуттєвіші current source SHA-256: runtime.cpp `F57B4418929BA2BC5A3C267E9FC1785EE8A646D0533A0A139FBA90EC9F66FEFC`; cinematic_fov.cpp `D6938560819957A6F1548BB2BBA109FFB1169763F99FF14E1269A6233511211D`; cinematic_lifecycle_transaction.hpp `D8EB9BA2A4C6221E147A7200E5DCC72C2EC95F35F9F37680A8A82E5EBE8ABFF2`; camera_writer_transaction.hpp `10F39B67ED895903F959C3D65764BF9479A28F67793B146914E54232E0B958E7`.

## Наступна мінімальна перевірка

Для stall: однаковий ASI та однаковий skip, із restart між startup settings; спершу Diagnostics ON/OFF при однаковому Overlay state. Якщо потрібно відокремити Overlay, окрема пара `Overlay.Enabled=true` (UI closed) / `false` при однаковому Diagnostic Mode. Багатосекундна затримка потребує зовнішнього CPU/thread/File I/O trace у момент відтворення, щоб установити thread і blocking operation. Нічого з цього аудит не запускав.

Для FOV: вузька production-adapter fixture описаної gen2 trajectory, потім той самий runtime сценарій. Root cause уже достатньо локалізований для окремого рішення щодо validated continuation; blanket widening 0,05° або 1° не випливає з цих доказів.
