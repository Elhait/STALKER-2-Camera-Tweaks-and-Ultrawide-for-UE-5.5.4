# Зміни STALKER 2 Camera Tweaks після 3.2.2

База порівняння — тег `v3.2.2` (`54212ee`). Документ описує стан робочого дерева на 19 листопада 2026 року. Наприкінці є повний реєстр файлів, включно з тестами, дослідницькими матеріалами та незатреканим B-логом.

## Керування станами камери

- **Єдиний власник lifecycle.** Активні Writer, Dialogue setter, provider armer, Cinematic ENTER/EXIT, aspect і blend callbacks, налаштування та завершення роботи проходять через Lifecycle Authority під `CameraCorrectionGate`. Overlay, zoom і telemetry читають committed проєкції. Попередні mutable-шляхи залишено у characterization fixtures для порівняння поведінки.
- **Узгоджені рішення.** Один Writer приймає спільне рішення щодо recovery, ownership, provenance та Gameplay admission. Вкладений callback пропускає нативний виклик без змін. Зміна налаштувань відкликає дозволи попередньої epoch. Під час shutdown спочатку закривається допуск callbacks, потім завершуються активні callbacks і workers.
- **Фізичні ефекти.** Підготовка FOV, виконання native запису й підтвердження результату — окремі етапи. Дозвіл Writer одноразовий. Ідентичність invocation працює з Diagnostics OFF і ON. Брак місця для спостереження readback не перериває корекцію. Історія ефектів, arming і camera scopes обмежена.
- **Наступність і завершення.** Candidate та provider claim обмежені джерелом. Сторонній Writer на іншому потоці та сторонній setter на тому самому receiver не можуть спожити або завершити чужу активацію. Підтверджене завершення recovery допускає корекцію на першому придатному Writer тієї самої транзакції. Повторні та порожні Cinematic generations, reentry, зміни source/output і скидання покрито окремими контрактами.
- **Gameplay baseline.** Підтверджене значення зберігається під час сумісного переходу, зокрема під час легітимного Dialogue між катсценами. Заміна primary receiver відкликає придатність старого reference навіть за однакового числового FOV. Дозвіл скоригувати конкретний Writer нового receiver відокремлено від допуску зразка для нового baseline. Зміни ownership, налаштувань та достовірності джерела перевіряються для відповідного споживача.

## Gameplay, катсцени й діалоги

- **Gameplay.** Режими `HorPlus`, `AspectRecalculation` і `RuntimeCombine` збережено. Для першого Writer нового receiver у `RuntimeCombine` дозволено синхронізувати native aspect з viewport і застосувати HorPlus після успішного readback. У перевіреному випадку це дає `120° → 147.796°` уже на першому Writer. Старий baseline не переноситься. Під час recovery власник утримує скориговане зображення в межах підтвердженої інтерполяції, уникає повторного HorPlus і захищає baseline від перехідних зразків.
- **Cinematic FOV.** `GameplayHorPlus` використовує придатний Gameplay reference на ENTER, зберігає авторські зміни FOV і переходить на authored reference за відсутності придатного Gameplay baseline. `NativeHorPlus` використовує authored ENTER FOV незалежно від Gameplay. ENTER фіксує політику й reference для конкретної активації. Cinematic source і фактичний output підтверджуються для Writer. Зміни output у підтвердженого source та швидкі EXIT/ENTER обробляються з перевіркою наступності й відповідного aspect store.
- **Cinematic → Dialogue.** Підтверджений provider може отримати Dialogue correction після EXIT, поки post-cinematic exclusion ще захищає Gameplay baseline. Вибір відповіді всередині активної катсцени залишається в Cinematic. Повторний Writer після підтвердження native Dialogue endpoint і до setter admission може утримувати скоригований FOV у межах того самого receiver, потоку, generation та epoch.
- **Dialogue → Gameplay / Cinematic.** Prepared і native FOV розрізняються, щоб не застосовувати `Reduced` двічі при виході з діалогу. Перший Cinematic blend після Dialogue може починатися з підтвердженого скоригованого output. Це покриває і перший Cinematic у сесії, коли попередній Dialogue відбувся в generation 0. Cinematic бере власну політику й придатний baseline. Завершені Dialogue claims не повинні блокувати незалежний Gameplay або наступний Dialogue.
- **Політики Dialogue.** Збережено `Native`, `Adaptive`, `Reduced` і `Disabled`. Підтвердження lifecycle та залежності конкретної корекції від reference перевіряються окремо.

Джерела: [поточна архітектура FOV](CAMERA_FOV_CURRENT_ARCHITECTURE.md), [production cutover](research/reports/LIFECYCLE_AUTHORITY_M2_PROGRESS_2026-11-17.md#production-cutover-2026-11-18), [M3 і відтворені дефекти](research/reports/LIFECYCLE_AUTHORITY_M3_VALIDATION_2026-11-18.md), [генерація callbacks](research/reports/LIFECYCLE_CALLBACK_GENERATION_2026-11-18.md).

## Camera State, оверлей, локалізації та INI

- **Camera State.** Gameplay FOV показує допущений production reference незалежно від нейтрального zoom observation. Окремо показуються оригінальний Native FOV, підготовлений Writer Input, Planned Output, Effective FOV, нейтральне Gameplay-спостереження і, за наявності, post-store та post-helper readback. Підтверджений readback прив’язано до відповідної транзакції та межі спостереження. Значення може мати статус Current, Retained, Historical або Unavailable. Оригінальний FOV публікується лише за наявності прямого захоплення.
- **Оверлей.** Оновлено read-only Camera State, статуси й підказки, зокрема пояснення та числові приклади Cinematic FOV. Оновлено всі 18 каталогів, п’ять підмножин шрифтів Noto і контракт покриття гліфів.
- **Клавіші.** Рядок перев’язування Cinematic FOV прибрано із загального списку гарячих клавіш в оверлеї. Runtime продовжує обробляти налаштований `CinematicFovCycle`. У прикладах README і GitHub release body залишився коментар «accepted but has no action», який суперечить цьому коду та описам у тих самих документах.
- **INI.** Сумісні варіанти `Cinematics.FovMode` і далі читаються. Під час звичайного збереження значення приводиться до канонічної назви. Оновлено пояснення режимів у генераторі документації INI.

Джерела: [архітектура публікації](docs/ARCHITECTURE.md#camera-state-fov-publication), [каталог підказок](locales/en.json), [список клавіш оверлею](src/overlay/presentation/hotkey_binding_presentation.hpp), [INI repository](src/config/config_repository.cpp).

## Запуск, native hooks і діагностика

- **Паралельний запуск процесів гри.** Конфлікт доступу до основного логу більше не має переривати ініціалізацію мода. Runtime, Startup і Overlay використовують ідентифікацію сесії через PID. У разі конфлікту відкриття є один багаторазовий файл `.PID.log` для кожного каналу. Успішне відкриття основного логу запускає спробу очищення резервного файла, а живий власник захищений правилами спільного доступу.
- **Обсяг логів.** Додано обмеження розміру, маркер досягнення ліміту, агрегацію повторюваних Writer і Cinematic записів, ідентичність транзакції та окремий worker діагностичної публікації. Відкриття логів та наявність діагностичного readback не визначають право на корекцію.
- **Native observation.** Додано або розширено перевірені точки blend producer/activation, post-exit setter, Cinematic consumer, post-store та post-helper. Підтверджений blend activation використовується стандартним ASI для provider handoff. Розширені записи залежать від `[Diagnostics] Enabled`. Read-only accessor SafetyHook відкриває метадані relocation для перевірки trampoline.
- **Zoom.** Парний IN/OUT resolver перевіряє унікальність сигнатур, межі виконуваного коду й інструкції. SHA використовується для походження запису. IN та OUT публікують спостереження після спільного commit. Доступність Zoom observer відображається окремо від режиму Diagnostics.
- **Контракти.** Оновлено архітектуру, інваріанти безпеки, manifest підтримуваних native меж, README, наявні release-тексти та індекс досліджень. Додано опис поточної Camera FOV архітектури, історію розробки, плани, звіти, докази та інструменти для Zoom, post-exit blend і Cinematic capture.

Джерела: [native logs](src/platform/win32/native_log_file.hpp), [manifest native меж](docs/SUPPORTED_BUILD_MANIFEST.md), [інваріанти](docs/SAFETY_INVARIANTS.md), [дослідження](research/README.md).

## Автоматизація і перевірки

- `test.cmd` залишається відкритим до натискання клавіші, показує підсумок і повертає сукупний код результату. Виконавці продовжують незалежні перевірки після FAIL, звітують про іменовані SKIP і перевіряють завершення власних дерев процесів.
- CMake, реєстр і CTest підключають Authority core, parity, native-shaped transactions, production cutover та transition model. Characterization старого адаптера і перевірки shipping callback branches мають окремі цілі. Додано перевірки ABI/trampoline, source/generation/epoch guard, фізичних аспектних ефектів, readback, telemetry, UI, локалізацій, zoom, startup і persistence.
- Модель переходів перевіряє короткі комбінації режимів, довгі детерміновані послідовності, pressure, settings/shutdown і значення на проміжних Writer/Setter кадрах. Незалежний callback generator перебирає окремі події та їх порядок. На збірці `12E8A333` п’ять контрольних мутацій відтворили порушення без сценаріїв конкретних катсцен.
- Нові release-contract тести перевіряють склад ZIP, ASI SHA-256, відсутність користувацького INI, некоректні шляхи та збереження попереднього ZIP при відмові. Окремий контракт перевіряє сценарії публікації ASI в build pipeline.

### Зафіксовані результати й межі

| Перевірка | Результат |
| --- | --- |
| Останній перевірений Release ASI | SHA-256 `974DEB38CA3FEBE8DA4B07AEC20D7C470FDAA18DC643077A4A9343610AC9A548` |
| Canonical non-graphics | 64/64 PASS, 0 FAIL, один hardlink-підвипадок SKIP (Windows error 5), WARP пропущено |
| Focused CTest | 6/6 PASS |
| Native adapter | 232 ізольовані процеси, Diagnostics OFF/ON |
| Матриця M3 | 15 спостережуваних етапів, 25/25 оголошених ребер протоколу, 9/9 пар режимів, 388 послідовностей, 23 020 перевірок стану, 8 548 змодельованих FOV-кадрів |
| Генерація окремих callbacks | 28 станів, 54 ребра, 185 коротких траєкторій на профіль, 3 008 послідовностей, 99 536 перевірок стану, 68 432 FOV-кадри, 48 запусків із фіксованими seed |

Виправлення першого Writer у кандидата `974DEB38` перевірено офлайн. Його live-перевірка ще очікується. A/B між `2C9660AB` і `12E8A333` відкладено. У B-прогоні спостерігалися окремі видимі ривки. Їхню причину і зв’язок із конкретною правкою не встановлено. Writer log фіксує FOV, aspect і flags на відомих межах. Остаточну проєкцію та відповідність показаному кадру не підтверджено.

Джерела: [журнал останнього офлайн-прогону](build-artifacts/cm/test-suite/authority-native-aspect-replacement-nongraphics.txt), [план A/B](research/plans/CAMERA_TRANSITION_FRAME_CORRELATION_2026-11-19.md), [збережений B-лог](research/evidence/camera-transition-ab-2026-11-19/B-observed-STALKER2CameraTweaks.log).

## Повний реєстр файлів

Статуси: `A` — додано, `M` — змінено, `?` — файл поза індексом Git. Реєстр охоплює вихідний стан, цей документ до нього не входить. Ігноровані збірки та інші локальні артефакти поза реєстром.

**Усього 624 файли.**

<details>
<summary>Код мода й залежності (62)</summary>

~~~text
M  external/safetyhook/safetyhook.hpp
M  src/camera/camera_state_publication.hpp
M  src/camera/camera_state_snapshot.cpp
M  src/camera/camera_state_snapshot.hpp
M  src/camera/fov_observation.hpp
M  src/camera/gameplay_baseline.cpp
M  src/camera/gameplay_baseline.hpp
M  src/cinematics/cinematic_aspect.cpp
M  src/cinematics/cinematic_aspect.hpp
A  src/cinematics/cinematic_enter_activation.hpp
M  src/cinematics/cinematic_fov.cpp
M  src/cinematics/cinematic_fov.hpp
M  src/config/config_repository.cpp
A  src/diagnostics/camera_state_writer_log.hpp
A  src/diagnostics/cinematic_consumer_observation.hpp
A  src/diagnostics/dialogue_boundary_observation.hpp
M  src/diagnostics/dialogue_source_provenance.hpp
A  src/diagnostics/post_exit_blend_activation_observation.hpp
A  src/diagnostics/post_exit_blend_observation.hpp
A  src/diagnostics/post_exit_blend_setter_pairing.hpp
M  src/diagnostics/startup_journal.hpp
A  src/diagnostics/u2_shadow.hpp
A  src/diagnostics/writer_diagnostic_aggregation.hpp
M  src/dialogue/dialogue_fov.cpp
M  src/dialogue/dialogue_fov.hpp
M  src/dialogue/dialogue_state.cpp
M  src/dialogue/dialogue_state.hpp
A  src/dialogue/post_exit_provider_authorization.hpp
A  src/dialogue/post_exit_provider_handoff.hpp
M  src/gameplay/gameplay_camera.cpp
M  src/gameplay/gameplay_camera.hpp
M  src/gameplay/gameplay_state.cpp
M  src/gameplay/gameplay_state.hpp
A  src/hooks/blend_producer_resolver.hpp
A  src/hooks/cinematic_consumer_resolver.hpp
M  src/hooks/hook_set.hpp
A  src/hooks/post_exit_blend_activation_resolver.hpp
A  src/hooks/post_exit_blend_post_setter_resolver.hpp
M  src/hooks/signatures/signature_definitions.hpp
A  src/hooks/zoom_transition_resolver.hpp
M  src/overlay/composition/composition_runtime.cpp
M  src/overlay/localization/localization_keys.cpp
M  src/overlay/localization/localization_keys.hpp
M  src/overlay/presentation/camera_integration.cpp
M  src/overlay/presentation/camera_integration.hpp
M  src/overlay/presentation/camera_state_view.cpp
M  src/overlay/presentation/camera_state_view.hpp
M  src/overlay/presentation/hotkey_binding_presentation.hpp
M  src/overlay/presenter/renderer_runtime.cpp
M  src/platform/win32/native_log_file.hpp
M  src/plugin/camera_correction_gate.hpp
M  src/plugin/camera_writer_transaction.hpp
M  src/plugin/cinematic_lifecycle_transaction.hpp
M  src/plugin/dialogue_boundary_transaction.hpp
A  src/plugin/lifecycle_authority_cinematic_port.hpp
A  src/plugin/lifecycle_authority_core.hpp
A  src/plugin/lifecycle_authority_dialogue_port.hpp
A  src/plugin/lifecycle_authority_gameplay_effects.hpp
A  src/plugin/lifecycle_authority_policy.hpp
A  src/plugin/lifecycle_authority_writer_port.hpp
M  src/plugin/runtime.cpp
M  src/plugin/worker_lifecycle.hpp
~~~

</details>

<details>
<summary>Збірка, тести й інструменти (46)</summary>

~~~text
M  CMakeLists.txt
M  test.cmd
M  tests/camera/camera_state_snapshot_harness.cpp
M  tests/camera/gameplay_baseline_harness.cpp
M  tests/cinematics/cinematic_fov_harness.cpp
M  tests/cinematics/horplus_post_exit_recovery_harness.cpp
M  tests/config/config_persistence_harness.cpp
M  tests/diagnostics/camera_writer_identity_harness.cpp
M  tests/diagnostics/dialogue_source_provenance_harness.cpp
M  tests/dialogue/dialogue_fov_harness.cpp
M  tests/lifecycle/worker_lifecycle_harness.cpp
M  tests/overlay/camera_integration_harness.cpp
M  tests/overlay/input_state_harness.cpp
M  tests/overlay/localization_harness.cpp
M  tests/plugin/camera_correction_gate_harness.cpp
M  tests/plugin/camera_writer_transaction_harness.cpp
A  tests/plugin/cinematic_consumer_cases.hpp
A  tests/plugin/cinematic_diagnostic_aggregation_cases.hpp
M  tests/plugin/cinematic_lifecycle_transaction_harness.cpp
M  tests/plugin/dialogue_boundary_transaction_harness.cpp
A  tests/plugin/lifecycle_authority_callback_model.hpp
A  tests/plugin/lifecycle_authority_cinematic_cases.hpp
A  tests/plugin/lifecycle_authority_corrective_cases.hpp
A  tests/plugin/lifecycle_authority_m1_harness.cpp
A  tests/plugin/lifecycle_authority_native_cinematic_cases.hpp
A  tests/plugin/lifecycle_authority_parity_cases.hpp
A  tests/plugin/lifecycle_authority_physical_cases.hpp
A  tests/plugin/lifecycle_authority_transition_model.hpp
A  tests/plugin/post_exit_blend_activation_cases.hpp
A  tests/plugin/post_exit_blend_observation_cases.hpp
M  tests/plugin/runtime_native_adapter_harness.cpp
A  tests/plugin/u2_shadow_cases.hpp
A  tests/plugin/zoom_compatibility_cases.hpp
M  tests/regression/module_ownership_harness.cpp
M  tests/regression/runtime_evidence_replay_harness.cpp
A  tests/runner/authority_callback_mutations.ps1
M  tests/runner/localization_catalog_audit.ps1
A  tests/runner/publish_asi_contract.ps1
A  tests/runner/release_package_contract.ps1
M  tests/runner/test_cmd_audit.ps1
M  tests/runner/test_cmd_exit_code_contract.ps1
M  tests/runner/test_cmd_exit_status_probe.cpp
M  tests/runner/test_cmd_orchestrator.ps1
M  tests/runner/test_harness_runner.ps1
M  tests/test_harnesses.json
M  tools/generate_ini_documentation.ps1
~~~

</details>

<details>
<summary>Локалізації та ресурси (25)</summary>

~~~text
M  assets/fonts/CatalogSubsets/NotoSansJP-Catalog.ttf
M  assets/fonts/CatalogSubsets/NotoSansKR-Catalog.ttf
M  assets/fonts/CatalogSubsets/NotoSansMono-Catalog.ttf
M  assets/fonts/CatalogSubsets/NotoSansSC-Catalog.ttf
M  assets/fonts/CatalogSubsets/NotoSansTC-Catalog.ttf
M  assets/fonts/CatalogSubsets/README.md
M  assets/fonts/CatalogSubsets/glyph-contract.json
M  locales/ar.json
M  locales/cs.json
M  locales/de.json
M  locales/en.json
M  locales/es-419.json
M  locales/es-ES.json
M  locales/fr.json
M  locales/it.json
M  locales/ja.json
M  locales/ko.json
M  locales/pl.json
M  locales/pt-BR.json
M  locales/ru.json
M  locales/sr-Cyrl.json
M  locales/tr.json
M  locales/uk.json
M  locales/zh-Hans.json
M  locales/zh-Hant.json
~~~

</details>

<details>
<summary>Документація (8)</summary>

~~~text
A  CAMERA_FOV_CURRENT_ARCHITECTURE.md
A  DEVELOPMENT_STORY.md
M  GITHUB_RELEASE_BODY.md
M  README.md
A  RELEASE_NOTES_v3.2.2.md
M  docs/ARCHITECTURE.md
M  docs/SAFETY_INVARIANTS.md
M  docs/SUPPORTED_BUILD_MANIFEST.md
~~~

</details>

<details>
<summary>Плани, звіти й інструменти досліджень (80)</summary>

~~~text
M  research/README.md
A  research/deferred/OVERLAY_DISCOVERY_CINEMATIC_ASPECT_TASK_PLAN_2026-11-13.md
A  research/deferred/U2_CINEMATIC_TO_DIALOGUE_RENDERED_FOV_JUMP_2026-11-15.md
A  research/plans/ASI_UPDATE_RUNTIME_FAILURE_ANALYSIS_2026-11-16.md
A  research/plans/AUTOMATED_CAMERA_TRANSACTION_COVERAGE_BATCH_2026-11-15.md
A  research/plans/CAMERA_STATE_FOV_SEMANTICS_2026-11-16.md
A  research/plans/CAMERA_TRANSITION_FRAME_CORRELATION_2026-11-19.md
A  research/plans/CINEMATIC_DIAGNOSTIC_AGGREGATION_2026-11-11.md
A  research/plans/CINEMATIC_FOV_AUTHORITY_IMPLEMENTATION_PLAN.md
A  research/plans/CINEMATIC_LIFECYCLE_CONTINUITY_2026-11-16.md
A  research/plans/CINEMATIC_OUTPUT_OWNERSHIP_2026-11-11.md
A  research/plans/CONFIRMED_DIALOGUE_FOV_CONTINUITY_2026-11-16.md
A  research/plans/GAMEPLAY_BASELINE_DIALOGUE_TRUST_REGRESSION_2026-11-16.md
A  research/plans/LIFECYCLE_AUTHORITY_DESIGN_2026-11-17.md
A  research/plans/LIFECYCLE_AUTHORITY_DIFFERENTIAL_PARITY_2026-11-17.md
A  research/plans/LIFECYCLE_AUTHORITY_M15_BINDING_2026-11-17.md
A  research/plans/LIFECYCLE_AUTHORITY_M16_COMPOSITE_WRITER_2026-11-17.md
A  research/plans/LIFECYCLE_AUTHORITY_M1_HARDENING_2026-11-17.md
A  research/plans/LIFECYCLE_AUTHORITY_MIGRATION_2026-11-17.md
A  research/plans/LIFECYCLE_CALLBACK_GENERATION_2026-11-18.md
A  research/plans/LIFECYCLE_EVIDENCE_AND_TRANSITION_MODEL_2026-11-17.md
A  research/plans/LIFECYCLE_NATIVE_EVIDENCE_AUDIT_2026-11-17.md
A  research/plans/NATIVE_REFERENCE_ADMISSION_P0_2026-11-13.md
A  research/plans/P0_U2_NATIVE_REFERENCE_INTEGRATION_2026-11-15.md
A  research/plans/POST_EXIT_BLEND_ACTIVATION_OBSERVATION_2026-11-12.md
A  research/plans/POST_EXIT_BLEND_OBSERVATION_2026-11-11.md
A  research/plans/POST_EXIT_BLEND_POST_SETTER_OBSERVATION_2026-11-12.md
A  research/plans/POST_EXIT_RECOVERY_CONTRACT_PROPOSAL_2026-11-11.md
A  research/plans/REMAINING_SUBSYSTEM_AUTOMATED_COVERAGE_AUDIT_2026-11-15.md
A  research/plans/RUNTIME_CAMERA_CONTEXT_ISOLATION_2026-11-17.md
A  research/plans/U2_CAMERA_STATE_DIAGNOSTIC_LOG_2026-11-14.md
A  research/plans/U2_DIALOGUE_TO_CINEMATIC_START_HANDOFF_2026-11-14.md
A  research/plans/U2_PROVIDER_DIALOGUE_WRITER_CONTINUATION_2026-11-14.md
A  research/plans/U2_TRANSITION_ADAPTER_DESIGN_2026-11-13.md
A  research/plans/U2_TRANSITION_POLICY_BOUNDARIES_2026-11-13.md
A  research/plans/V4_01_UNIFIED_DIALOGUE_POLICY_SELECTION_2026-11-15.md
A  research/plans/ZOOM_COMPATIBILITY_CG01_CG03_2026-11-11.md
A  research/reports/CAMERA_FRAMEWORK_ARCHITECTURE_AUDIT_2026-11-13.md
A  research/reports/CINEMATIC_DIAGNOSTIC_AGGREGATION_2026-11-11.md
A  research/reports/CINEMATIC_FOV_AUTHORITY_AUDIT.md
A  research/reports/CINEMATIC_NATIVE_OUTPUT_PROVENANCE_2026-11-11.md
A  research/reports/DEVELOPMENT_HISTORY_RECONSTRUCTION_2026-11-09.md
A  research/reports/DIALOGUE_CANDIDATE_TERMINATION_VALIDATION_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_DESIGN_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_DIFFERENTIAL_PARITY_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_INDEPENDENT_REVIEW_SOL_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M15_BINDING_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M16_COMPOSITE_WRITER_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M1_HARDENING_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M1_ISOLATED_CORE_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M2_PROGRESS_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_M3_VALIDATION_2026-11-18.md
A  research/reports/LIFECYCLE_AUTHORITY_RECONSTRUCTION_2026-11-17.md
A  research/reports/LIFECYCLE_AUTHORITY_SHADOW_REPLAY_2026-11-17.md
A  research/reports/LIFECYCLE_CALLBACK_GENERATION_2026-11-18.md
A  research/reports/LIFECYCLE_EVIDENCE_AND_TRANSITION_MODEL_2026-11-17.md
A  research/reports/LIFECYCLE_NATIVE_EVIDENCE_AUDIT_2026-11-17.md
A  research/reports/NATIVE_FOV_IMMUTABILITY_INVARIANT_AUDIT_2026-11-13.md
A  research/reports/NATIVE_FOV_OVERLAY_PROVENANCE_AUDIT_2026-11-13.md
A  research/reports/P0_CAMERA_CONTEXT_CLASSIFICATION_2026-11-13.md
A  research/reports/P0_LIFECYCLE_ISOLATION_PROOF_2026-11-13.md
A  research/reports/P0_LIFECYCLE_REFERENCE_BOUNDARIES_2026-11-13.md
A  research/reports/POST_EXIT_BLEND_ACTIVATION_OBSERVATION_2026-11-12.md
A  research/reports/POST_EXIT_BLEND_LIVE_CORRELATION_2026-11-11.md
A  research/reports/POST_EXIT_BLEND_OBSERVATION_2026-11-11.md
A  research/reports/POST_EXIT_BLEND_POST_SETTER_OBSERVATION_2026-11-12.md
A  research/reports/POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md
A  research/reports/POST_EXIT_NATIVE_BRANCH_FEASIBILITY_2026-11-11.md
A  research/reports/PRODUCTION_COMPATIBILITY_GATES_AUDIT_2026-11-11.md
A  research/reports/REMAINING_SUBSYSTEM_AUTOMATED_COVERAGE_TRIAGE_2026-11-15.md
A  research/reports/RUNTIME_CAMERA_CONTEXT_ISOLATION_AUDIT_2026-11-17.md
A  research/reports/U2_BOUNDARY_INTEGRATION_FEASIBILITY_2026-11-13.md
A  research/reports/U2_EXIT_DIALOGUE_BOUNDED_ASSESSMENT_2026-11-13.md
A  research/reports/U2_POLICY_ENDPOINTS_2026-11-13.md
A  research/reports/U2_SHADOW_IMPLEMENTATION_2026-11-13.md
A  research/reports/V4_00_AUTOMATED_TEST_COVERAGE_AUDIT_2026-11-15.md
A  research/reports/ZOOM_COMPATIBILITY_CG01_CG03_IMPLEMENTATION_2026-11-11.md
A  research/tools/ArmCinematicFovCapture.ps1
A  research/tools/CheckZoomSignaturePortability.java
A  research/tools/verify_post_exit_blend_fixture.py
~~~

</details>

<details>
<summary>Дослідницькі докази (403)</summary>

~~~text
A  research/evidence/asi-restart-issue-2026-11-16/STALKER2CameraTweaks.log
A  research/evidence/asi-restart-issue-2026-11-16/STALKER2CameraTweaksOverlay.log
A  research/evidence/asi-restart-issue-2026-11-16/STALKER2CameraTweaksStartup.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/RTXMFG-stalker2-win64-shipping-289237615407A3BD.25220.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/RTXMFG-stalker2-win64-shipping-289237615407A3BD.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/STALKER2CameraTweaks.ini
A  research/evidence/asi-startup-failure-2026-11-16-180927/STALKER2CameraTweaks.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/STALKER2CameraTweaksOverlay.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/STALKER2CameraTweaksStartup.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/Slop_yCam_Loader.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/Stalker2-backup-2026.11.16-16.09.31.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/Stalker2.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/UE4SS.log
A  research/evidence/asi-startup-failure-2026-11-16-180927/causal-assessment.md
A  research/evidence/asi-startup-failure-2026-11-16-180927/repair-validation.json
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaks.PID.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaks.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaksOverlay.PID.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaksOverlay.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaksStartup.PID.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/STALKER2CameraTweaksStartup.log
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/assessment.md
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/manifest.json
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/state-1.png
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/state-2.png
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/state-3.png
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/state-4.png
A  research/evidence/camera-state-fov-runtime-2026-11-16-213855/state-5.png
A  research/evidence/camera-state-fov-semantics-2026-11-16/STALKER2CameraTweaks.PID.log
A  research/evidence/camera-state-fov-semantics-2026-11-16/STALKER2CameraTweaksOverlay.PID.log
A  research/evidence/camera-state-fov-semantics-2026-11-16/STALKER2CameraTweaksStartup.PID.log
A  research/evidence/camera-state-fov-semantics-2026-11-16/assessment.md
A  research/evidence/camera-state-fov-semantics-2026-11-16/production-artifact.json
A  research/evidence/camera-state-fov-semantics-2026-11-16/production-release-validation.json
A  research/evidence/camera-state-fov-semantics-2026-11-16/production-suite-validation.json
A  research/evidence/camera-state-fov-semantics-2026-11-16/production-validation.md
A  research/evidence/cinematic-consumer-outcome-2026-11-13/candidate-metadata.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/final-consumer-resolver-image.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/final-diagnostic-work-off.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/final-diagnostic-work-on.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/final-non-graphics.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/release-build-final.json
A  research/evidence/cinematic-consumer-outcome-2026-11-13/runtime-111855/STALKER2CameraTweaks.log
A  research/evidence/cinematic-consumer-outcome-2026-11-13/validation-summary.json
A  research/evidence/cinematic-diagnostics-aggregation-2026-11-11/baseline-log-statistics.json
A  research/evidence/cinematic-diagnostics-aggregation-2026-11-11/canonical.log
A  research/evidence/cinematic-diagnostics-aggregation-2026-11-11/focused.log
A  research/evidence/cinematic-diagnostics-aggregation-2026-11-11/validation-summary.json
A  research/evidence/cinematic-weapon-handoff-2026-11-11/STALKER2CameraTweaks.log
A  research/evidence/d1-caller-attribution-2026-11-13/adapter-final.json
A  research/evidence/d1-caller-attribution-2026-11-13/adapter-regression.json
A  research/evidence/d1-caller-attribution-2026-11-13/build-final.json
A  research/evidence/d1-caller-attribution-2026-11-13/focused-off.json
A  research/evidence/d1-caller-attribution-2026-11-13/focused-on.json
A  research/evidence/d1-caller-attribution-2026-11-13/release-result.json
A  research/evidence/d1-caller-attribution-2026-11-13/runtime-012334/STALKER2CameraTweaks.log
A  research/evidence/d1-caller-attribution-2026-11-13/runtime-012334/constant-read-run.json
A  research/evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-and-enter-guard.txt
A  research/evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-caller-constant.txt
A  research/evidence/d1-caller-attribution-2026-11-13/runtime-012334/native-decode-run.json
A  research/evidence/d1-caller-attribution-2026-11-13/validation-summary.json
A  research/evidence/dialogue-provider-after-recovery-2026-11-16/STALKER2CameraTweaks.log
A  research/evidence/dialogue-provider-after-recovery-2026-11-16/assessment.md
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/STALKER2CameraTweaks.PID.log
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/STALKER2CameraTweaks.log
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/assessment.md
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/cinematic-continuity-red.json
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/cinematic-continuity-release.json
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/cinematic-continuity-suite-final.json
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/cinematic-start-red.json
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/manifest.json
A  research/evidence/interactive-cinematic-reentry-2026-11-16-230941/repair-validation-2026-11-17.md
A  research/evidence/lifecycle-authority-differential-2026-11-17/authority-parity.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/build.cmd
A  research/evidence/lifecycle-authority-differential-2026-11-17/build.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-before.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-build.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-ctest.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-final-m1.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-final-parity.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-final-production.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-final-shadow.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-m1.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-parity.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-port-build.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-production.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-registry-audit.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-shadow.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-source-identities.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/corrective-unchanged-identities.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/default.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/initial-red-summary.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/m1-default.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/m1-shadow.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/m2-aspect-mode-red.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/production-default.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/red.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/registry-audit.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/source-identities.json
A  research/evidence/lifecycle-authority-differential-2026-11-17/summary.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/build.cmd
A  research/evidence/lifecycle-authority-hardening-2026-11-17/build.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/default.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/hardening.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/r1-identities.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/red.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/registry-audit.json
A  research/evidence/lifecycle-authority-hardening-2026-11-17/shadow-replay.json
A  research/evidence/lifecycle-authority-m1-2026-11-17/authority_m1.js
A  research/evidence/lifecycle-authority-m1-2026-11-17/native-binding-sheet.md
A  research/evidence/lifecycle-authority-m1-2026-11-17/r1-projection.json
A  research/evidence/lifecycle-authority-m1-2026-11-17/result.json
A  research/evidence/lifecycle-authority-m15-2026-11-17/authority-bindings.json
A  research/evidence/lifecycle-authority-m15-2026-11-17/build.cmd
A  research/evidence/lifecycle-authority-m15-2026-11-17/build.json
A  research/evidence/lifecycle-authority-m15-2026-11-17/default.json
A  research/evidence/lifecycle-authority-m15-2026-11-17/review-identities.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/build.cmd
A  research/evidence/lifecycle-authority-m16-2026-11-17/build.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/composite-writer.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/default.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/red.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/registry-audit.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/shadow-replay.json
A  research/evidence/lifecycle-authority-m16-2026-11-17/unchanged-identities.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/aspect-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/build-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/build.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/cinematic-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cinematic-progress.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-debug-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-debug-test.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-baseline-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-baseline-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-chain-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-chain-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-exit-departure-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-exit-departure-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-foreign-source-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-foreign-source-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-invalid-baseline-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-invalid-baseline-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-native-mode-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-native-mode-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-no-validated-exit-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-no-validated-exit-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-output-roundtrip-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-output-roundtrip-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-policy-change-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-cinematic-continuation-policy-change-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-green-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-red-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-red-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-verified-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/continuation-verified-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/ctest-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-abi-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-abi-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-build.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-final-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-final-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-final-regressions.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-fixture-build.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-fixture-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-fixture-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-installed-abi-run.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-installed-abi.py
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-owner-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-owner-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-owner-source-audit.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-regressions.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-regressions.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-shipping-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-shipping-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/cutover-shipping-regressions.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-green-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-green-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-red-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-red-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-red-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-red.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/delayed-provider-regressions.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/fact-boundaries-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/fact-boundaries-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/gates-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/gates-green.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/gates-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/m1-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/m1.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/native-boundary-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/native-cinematic.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/parity-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/parity.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/passthrough-observation-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/physical-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/physical.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/prepared-argument-characterization-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/production-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/provider-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/provider-native.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/provider-scope-transfer-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/provider-settings-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/provider-settings-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/receiver-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/receiver-green.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/receiver-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/receiver-setter-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/recovery-focused.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/registry-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/resumed-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-build.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-cinematic-continuation-chain-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-cinematic-continuation-chain-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-dialogue-active-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-dialogue-active-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-dialogue-baseline-context-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-dialogue-baseline-context-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-evidence-dialogue-exclusion-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-evidence-dialogue-exclusion-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-evidence-dialogue-reset-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-evidence-dialogue-reset-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-lifecycle-chain-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-lifecycle-chain-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-recheck-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-recheck-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-recovery-foreign-source-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-recovery-foreign-source-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-writer-combine-off.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-capture-writer-combine-on.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-setter-binding-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/runtime-setter-binding-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/shadow-final.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transfer-red-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-contract-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-contract-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-contract-regressions.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-foreground-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-foreground-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-foreground-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-invariants-green-build.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-invariants-green-ctest.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-invariants-red.cmd
A  research/evidence/lifecycle-authority-m2-2026-11-17/transition-invariants-red.json
A  research/evidence/lifecycle-authority-m2-2026-11-17/unchanged-runtime-oracle.json
A  research/evidence/lifecycle-native-evidence-audit-2026-11-17/check_evidence_model.js
A  research/evidence/lifecycle-native-evidence-audit-2026-11-17/corpus-identities.json
A  research/evidence/lifecycle-native-evidence-audit-2026-11-17/model-result.json
A  research/evidence/lifecycle-native-evidence-audit-2026-11-17/native-projection-collisions.json
A  research/evidence/lifecycle-native-evidence-audit-2026-11-17/source-identities.json
A  research/evidence/lifecycle-termination-2026-11-17/artifact.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-controls.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-green.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-red.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-release.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-sequence.json
A  research/evidence/lifecycle-termination-2026-11-17/lifecycle-termination-suite-final.json
A  research/evidence/lifecycle-transition-model-2026-11-17/native-archive-identities.json
A  research/evidence/lifecycle-transition-model-2026-11-17/oracle-freeze-R1.json
A  research/evidence/lifecycle-transition-model-2026-11-17/provider-quarantine-red.json
A  research/evidence/lifecycle-transition-model-2026-11-17/research-artifact-identities.json
A  research/evidence/lifecycle-transition-model-2026-11-17/research-model-red-result.json
A  research/evidence/lifecycle-transition-model-2026-11-17/result.json
A  research/evidence/lifecycle-transition-model-2026-11-17/source-identities.json
A  research/evidence/lifecycle-transition-model-2026-11-17/transition-coverage.md
A  research/evidence/lifecycle-transition-model-2026-11-17/transition_model.js
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaks.PID.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaks.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaksOverlay.PID.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaksOverlay.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaksStartup.PID.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/STALKER2CameraTweaksStartup.log
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/assessment.md
A  research/evidence/multisave-cinematic-baseline-2026-11-17-012536/manifest.json
A  research/evidence/native-reference-p0-2026-11-13/build-final-recheck.json
A  research/evidence/native-reference-p0-2026-11-13/build-final.json
A  research/evidence/native-reference-p0-2026-11-13/build.json
A  research/evidence/native-reference-p0-2026-11-13/legacy.json
A  research/evidence/native-reference-p0-2026-11-13/native-origin-contract.json
A  research/evidence/overlay-cinematic-aspect-2026-11-13/STALKER2CameraTweaks.log
A  research/evidence/overlay-cinematic-aspect-2026-11-13/STALKER2CameraTweaksOverlay.log
A  research/evidence/overlay-cinematic-aspect-2026-11-13/STALKER2CameraTweaksStartup.log
A  research/evidence/p0-u2-native-reference-2026-11-15/STALKER2CameraTweaks.log
A  research/evidence/p0-u2-native-reference-2026-11-15/assessment.md
A  research/evidence/p0-u2-runtime-2026-11-13/STALKER2CameraTweaks-232230.log
A  research/evidence/p0-u2-runtime-2026-11-13/STALKER2CameraTweaks-retest-2026-11-14.log
A  research/evidence/p0-u2-runtime-2026-11-13/candidate-baseline-admission-fix.json
A  research/evidence/p0-u2-runtime-2026-11-13/candidate-manifest.json
A  research/evidence/p0-u2-runtime-2026-11-13/focused-runtime-records-2026-11-14.txt
A  research/evidence/p0-u2-runtime-2026-11-13/focused-runtime-records.txt
A  research/evidence/p0-u2-runtime-2026-11-13/operator-runbook.md
A  research/evidence/p0-u2-runtime-2026-11-13/runtime-2026-11-14-005523.log
A  research/evidence/p0-u2-runtime-2026-11-13/runtime-log-assessment.md
A  research/evidence/p0-u2-runtime-2026-11-13/runtime-retest-assessment-2026-11-14.md
A  research/evidence/p0-u2-runtime-2026-11-16/STALKER2CameraTweaks-dialogue-followup.log
A  research/evidence/p0-u2-runtime-2026-11-16/STALKER2CameraTweaks-followup.log
A  research/evidence/p0-u2-runtime-2026-11-16/STALKER2CameraTweaks.log
A  research/evidence/p0-u2-runtime-2026-11-16/assessment-dialogue-followup.md
A  research/evidence/p0-u2-runtime-2026-11-16/assessment-followup.md
A  research/evidence/p0-u2-runtime-2026-11-16/assessment.md
A  research/evidence/p0-u2-runtime-2026-11-16/dialogue-policy-before-fix-runtime.log
A  research/evidence/post-exit-blend-live-2026-11-11-231304/STALKER2CameraTweaks.log
A  research/evidence/post-exit-blend-live-2026-11-11-231304/fixture-integrity-result.json
A  research/evidence/post-exit-blend-live-2026-11-11-231304/records.json
A  research/evidence/post-exit-blend-live-2026-11-11-231304/regression-fixture.json
A  research/evidence/post-exit-blend-observation-2026-11-11/adapter-aggregation-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/adapter-aggregation-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/adapter-work-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/adapter-work-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/baseline-identities.json
A  research/evidence/post-exit-blend-observation-2026-11-11/build-focused.cmd
A  research/evidence/post-exit-blend-observation-2026-11-11/build-release.cmd
A  research/evidence/post-exit-blend-observation-2026-11-11/candidate-identity.json
A  research/evidence/post-exit-blend-observation-2026-11-11/cleanup-summary.json
A  research/evidence/post-exit-blend-observation-2026-11-11/current-image-layout.json
A  research/evidence/post-exit-blend-observation-2026-11-11/diagnostic-release.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-active-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-active-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-disabled-in-flight-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-disabled-in-flight-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-off-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/dialogue-off-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/focused-build.json
A  research/evidence/post-exit-blend-observation-2026-11-11/lifecycle-off-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/lifecycle-off-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/lifecycle-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/lifecycle-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/production-isolation.json
A  research/evidence/post-exit-blend-observation-2026-11-11/production-release.json
A  research/evidence/post-exit-blend-observation-2026-11-11/recovery-foreign-source-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/recovery-foreign-source-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/recovery-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/recovery-on.json
A  research/evidence/post-exit-blend-observation-2026-11-11/run-focused.ps1
A  research/evidence/post-exit-blend-observation-2026-11-11/run-release.ps1
A  research/evidence/post-exit-blend-observation-2026-11-11/trace-model.json
A  research/evidence/post-exit-blend-observation-2026-11-11/writer-combine-off.json
A  research/evidence/post-exit-blend-observation-2026-11-11/writer-combine-on.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/STALKER2CameraTweaks.log
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/analysis-source-identities.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/artifact.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/assessment.md
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/authority-design-source-identities.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/recovery-completion-focused.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/recovery-completion-red.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/recovery-completion-release.json
A  research/evidence/same-save-recovery-completion-2026-11-17-020947/recovery-completion-suite.json
A  research/evidence/u2-activation-ownership-runtime-2026-11-14/candidate-manifest.json
A  research/evidence/u2-activation-ownership-runtime-2026-11-14/operator-runbook.md
A  research/evidence/u2-camera-state-diagnostic-2026-11-14/assessment.md
A  research/evidence/u2-camera-state-diagnostic-2026-11-14/candidate-manifest.json
A  research/evidence/u2-camera-state-diagnostic-2026-11-14/runtime-startup-crash-assessment.md
A  research/evidence/u2-confirmed-dialogue-transition-2026-11-16/STALKER2CameraTweaks.5552.log
A  research/evidence/u2-confirmed-dialogue-transition-2026-11-16/STALKER2CameraTweaksOverlay.5552.log
A  research/evidence/u2-confirmed-dialogue-transition-2026-11-16/STALKER2CameraTweaksStartup.5552.log
A  research/evidence/u2-confirmed-dialogue-transition-2026-11-16/assessment.md
A  research/evidence/u2-confirmed-dialogue-transition-2026-11-16/repair-validation.json
A  research/evidence/u2-continuity-runtime-2026-11-16-192205/STALKER2CameraTweaks.25756.log
A  research/evidence/u2-continuity-runtime-2026-11-16-192205/STALKER2CameraTweaksOverlay.25756.log
A  research/evidence/u2-continuity-runtime-2026-11-16-192205/STALKER2CameraTweaksStartup.25756.log
A  research/evidence/u2-continuity-runtime-2026-11-16-192205/memory-and-log-followup.md
A  research/evidence/u2-dialogue-cinematic-retest-2026-11-14/assessment.md
A  research/evidence/u2-dialogue-cinematic-retest-2026-11-14/runtime.log
A  research/evidence/u2-dialogue-cinematic-retest-2026-11-14/validation-summary.json
A  research/evidence/u2-enter-activation-runtime-2026-11-14/STALKER2CameraTweaks.log
A  research/evidence/u2-enter-activation-runtime-2026-11-14/runtime-assessment.md
A  research/evidence/u2-exit-dialogue-2026-11-13/STALKER2CameraTweaks-141503.log
A  research/evidence/u2-provider-continuity-runtime-2026-11-14/STALKER2CameraTweaks.log
A  research/evidence/u2-provider-continuity-runtime-2026-11-14/assessment.md
A  research/evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/STALKER2CameraTweaks.log
A  research/evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/assessment.md
A  research/evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/build-release.cmd
A  research/evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/validation-summary.json
A  research/evidence/u2-shadow-2026-11-13/clean-build.log
A  research/evidence/u2-shadow-2026-11-13/clean-focused-results.json
A  research/evidence/u2-shadow-2026-11-13/final-incremental-check.json
A  research/evidence/u2-shadow-2026-11-13/stale-cache-failure.json
A  research/evidence/u2-shadow-2026-11-13/validation-summary.json
A  research/evidence/u2-startup-crash-fix-2026-11-14/assessment.md
A  research/evidence/u2-startup-crash-fix-2026-11-14/candidate-manifest.json
A  research/evidence/u2-transition-policy-2026-11-13/artifact.json
A  research/evidence/u2-transition-policy-2026-11-13/before-admission-repair.log
A  research/evidence/u2-transition-policy-2026-11-13/build.json
A  research/evidence/u2-transition-policy-2026-11-13/camera_writer_transaction_harness.json
A  research/evidence/u2-transition-policy-2026-11-13/dialogue_boundary_transaction_harness.json
A  research/evidence/u2-transition-policy-2026-11-13/runtime-182050.log
A  research/evidence/u2-writer-site-inventory-2026-11-15/assessment.md
A  research/evidence/u2-writer-site-inventory-2026-11-15/fov-provenance-chain.md
A  research/evidence/u2-writer-site-inventory-2026-11-15/post-process-observer-implementation.md
A  research/evidence/u2-writer-site-inventory-2026-11-15/scalar-store-scan.txt
A  research/evidence/u2-writer-site-inventory-2026-11-15/scan_all_direct_stores.py
A  research/evidence/u2-writer-site-inventory-2026-11-15/scan_getcameraview_virtual_callers.py
A  research/evidence/u2-writer-site-inventory-2026-11-15/scan_offset30_reads.py
A  research/evidence/u2-writer-site-inventory-2026-11-15/scan_scalar_fov_stores.py
A  research/evidence/u2-writer-site-inventory-2026-11-15/startup-crash-assessment.md
A  research/evidence/zoom-compatibility-2026-11-11/current-image-scan.json
A  research/evidence/zoom-compatibility-2026-11-11/release-build.json
A  research/evidence/zoom-compatibility-2026-11-11/retained-image-scan.json
A  research/evidence/zoom-compatibility-2026-11-11/validation-summary.json
?  research/evidence/camera-transition-ab-2026-11-19/B-observed-STALKER2CameraTweaks.log
~~~

</details>
