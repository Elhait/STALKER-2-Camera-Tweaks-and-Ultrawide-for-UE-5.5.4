# Cinematic diagnostics — агрегація output transitions

Дата: 2026-11-11. [План](../plans/CINEMATIC_DIAGNOSTIC_AGGREGATION_2026-11-11.md). Реалізацію та офлайн-перевірки завершено. Фактичний розмір нового журналу в грі та вплив на FPS не вимірювали.

## Підстава

Наданий журнал ASI `55DB…D872`: 8 486 585 bytes / 14 228 lines. `HORPLUS_CINEMATIC_WRITER` — 6 866 lines / 5 096 550 bytes; `CAMERA_WRITER_IDENTITY_CHANGED` — 6 835 lines / 3 185 523 bytes. Разом 97,6% байтів. Із identity changes 6 832 змінювали тільки RBX. Чергування звичайних destinations повторно активувало single-snapshot dedup; BEFORE/AFTER також реагував на output pointer і FOV delta 0,01°.

[Ідентичність і статистика початкового журналу](../evidence/cinematic-diagnostics-aggregation-2026-11-11/baseline-log-statistics.json). Це окремий runtime-артефакт, не byte-for-byte workload нового offline fixture.

## Реалізація

- `src/diagnostics/writer_diagnostic_aggregation.hpp`: fixed-size counters у RuntimeState, без destination map, heap allocation, таймерів або нових locks. Scope — спостережений source, cinematic generation і lifecycle. Same-source output transitions накопичуються як факти викликів, не як конфлікт source ownership.
- Runtime зберігає повні initialization/source identity records. Подія `RbxOnlyChanged` класифікується, але замість повного identity record оновлює aggregate. Production writer-pair sequence/identityEpoch та source ownership не змінюються.
- Повний `CINEMATIC_ASSOCIATION_WRITER_STATE` вибирається за generation/source, підтвердженою association, conflict, initial-establishment state, lifecycle, production action, dispatch result, точним `mutateFov` та валідністю даних. RBX і numeric drift не є критеріями повторної емісії.
- BEFORE/AFTER snapshots читаються та пишуться лише для вибраного семантичного запису. `applied` у AFTER відповідає фактичному transaction decision, а не різниці понад 0,001°. Незмінний числовий результат не приховує зміну семантичної дії. Самі traces лишаються до native store, тому не доводять його майбутній результат.
- `CAMERA_WRITER_SUMMARY` містить точні output transition/sample/applied/pass-through counts, кількість повних writer-state records і suppressed samples, first/last destination, first/last input, last result та numeric extrema. `full_records` рахує саме writer-state records, не всі фізичні рядки журналу.
- Summary публікується/скидається на source/lifecycle change, новому прийнятому ENTER, EXIT, disable та в існуючому контрольованому Runtime reset після callback drain. Порожній повторний flush нічого не пише. Нових callbacks або I/O у DllMain немає. Під час ExitProcess виконання контрольованого reset не гарантоване; основні дані зберігаються на ENTER/EXIT, а не покладаються лише на фінальний запис.
- Незалежний bounded cinematic continuation trace, resolver/hook errors та lifecycle records збережено. Реальні часті source/decision transitions не обмежуються довільною квотою; existing 16 MiB file limit лишається останнім захистом журналу.

Усі зміни production-коду стосуються діагностичного owner та його викликів у `runtime.cpp`. Camera math, cinematic association/admission, Gameplay recovery, hooks/resolvers, INI та Overlay layout не змінено. Diagnostics OFF не виконує нової агрегації чи snapshot reads. Поточні architecture/safety contracts оновлено.

## Регресія та результати

Новий `diagnostic-aggregation` case запускається в окремих процесах OFF/ON через actual production adapter. До logger додається справжній NativeLogSink: перевіряються фізичні рядки файла, не лише виклики helper.

| Перевірка | Результат |
| --- | --- |
| 10 000 writer calls із A/B outputs і дрібним native input drift | ON: один повний writer-state record + два BEFORE/AFTER records; OFF: нуль цих записів |
| Output transition accounting | У boundary summary рівно 9 999 переходів і 10 000 samples; 9 999 повторних samples агреговані |
| Production parity | Для кожного з 10 000 calls перевірено фактичний XMM0 target та незмінність решти context; той самий результат OFF/ON |
| Source change / ownership conflict | Реальний adapter залишає foreign/conflicting input native та зберігає повні діагностичні записи |
| Tiny Apply→Skip | Реальна функція емісії отримує два різні exact decisions із різницею output в один float ULP; обидва записи видимі, наступний identical Skip не дублюється |
| Boundary accounting | EXIT, наступний прийнятий ENTER, source boundary, controlled Runtime reset публікують хвіст; empty flush не дублює; pure helper перевіряє нову generation/source без старих counters |
| Повний fixture із додатковими source/conflict/decision/lifecycle cases | ON: 96 lines / 36 439 bytes; OFF: 12 operational lines / 1 957 bytes |
| Pure aggregation test | 10 000 transitions, numeric first/last/extrema та 5 000 applied / 5 000 pass-through збережені |
| Focused checks | 11/11 PASS, включно з DiagnosticWork та output/recovery регресіями |
| Фінальний canonical `test.cmd -NonGraphics` | 62/62 PASS; 224 native-adapter fresh processes, graphics case omitted; registry 115 scenarios / 230 nominal children |

96 рядків охоплюють увесь fixture, а не лише 10 000 повторів: до них входять наступні source/conflict/decision та lifecycle переходи. Це не вимірювання нового розміру 7-хвилинного ігрового журналу.

Проміжна компіляція виявила declaration-order помилку в діагностичному helper; її виправлено. Один проміжний fixture помилково вимагав нової generation через duplicate ENTER у CinematicActive; production правильно відхилив такий ENTER. Fixture виправлено на EXIT → validated Gameplay recovery → наступний ENTER без зміни production lifecycle. Фінальний canonical PASS отримано після цих виправлень і останнього розширення numeric-drift fixture.

[Фінальний canonical output](../evidence/cinematic-diagnostics-aggregation-2026-11-11/canonical.log), [focused output](../evidence/cinematic-diagnostics-aggregation-2026-11-11/focused.log), [summary/source hashes](../evidence/cinematic-diagnostics-aggregation-2026-11-11/validation-summary.json). Supervisors підтвердили process-tree cleanup; фінальна перевірка не виявила залишених compiler/test helper processes.

## Межі та передача

Розмір і частота записів зменшені для перевіреного повторюваного workload. Кількісних тверджень про FPS, latency або зменшення ігрового log size немає. Повторний ручний сценарій дасть порівнюваний runtime-журнал після наступної збірки.

Standalone Release ASI у цьому етапі не збирали; `build.cmd` збере оновлений код. Попередні ASI не перезаписували й не встановлювали. Гру, debugger і graphics fixtures не запускали, Git не використовували. Початковий пропуск cinematic ENTER залишається незалежною відкритою задачею.
