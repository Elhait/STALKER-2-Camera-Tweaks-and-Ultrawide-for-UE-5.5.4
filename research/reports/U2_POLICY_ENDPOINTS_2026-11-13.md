# U2 — native та corrected endpoints

Аналіз поточного working tree, 2026-11-13. Production-код не змінено;
гру, debugger і збірки не запускали. Числа нижче розділено на runtime evidence
та обчислення чинних policy. Результат writer не є виміром rendered FOV.

## Позначення та authority

- `B` — baseline прийнятого Dialogue lifecycle до setter-корекції. У чинному
  classifier це перший candidate input, а не незалежно підтверджений Gameplay
  reference. Його не можна автоматично прирівнювати до historical Gameplay
  baseline або transformed `camera+0x230`.
- `N=70°` — endpoint, на який орієнтуються чинні Dialogue helpers. Саме число
  не доводить Dialogue ownership. Для поточного producer literal70 підтверджено
  попереднім native дослідженням і runtime activation.
- `H(x,A)` — camera::HorPlus(x,A,16/9). Застосування нижче передбачає чинну
  Gameplay correction, допустимі flags/source та підтверджений aspect. Для
  RuntimeCombine потрібна успішна синхронізація aspect. Інакше наведені corrected
  endpoints не гарантуються.
- `R` — власний authored Cinematic ENTER reference, `C` — результат cinematic
  policy. Результат попереднього режиму та native start — окремі величини.

## Dialogue policies: фактичні формули

`Q=tan(90°/2)/tan(70°/2)`, `D(B)=2 atan(tan(B/2)/Q)`,
`L(B)=2 atan(tan(B/2)/sqrt(Q))`; кути у формулах записані в градусах.

| Effective selection | Native target | Setter endpoint за чинним контрактом | Writer endpoint за HorPlus/RuntimeCombine | Що означає |
| --- | --- | --- | --- | --- |
| Native або Dialogue.Enabled=false | N | N без Dialogue mutation | H(N,A) | Dialogue не змінює zoom; Gameplay correction незалежна |
| Adaptive | N | D(B) | H(D(B),A) | Зберігає native zoom ratio відносно B |
| Reduced | N | L(B) | H(L(B),A) | Зменшує zoom відносно B |
| Disabled | N | B | H(B,A) | Вимикає Dialogue zoom, не всю FOV/aspect correction |

Джерела: src/plugin/dialogue_boundary_transaction.hpp, ProcessDialogueBoundary
(AcceptedStart, activePolicy і обробка Disabled); src/dialogue/dialogue_fov.cpp,
AdaptiveTarget/ReducedTarget/TransformProjectionSample; src/gameplay/horplus_gameplay.cpp,
EvaluateHorPlus. Native target тут не тотожний аргументу setter після mutation.
Adaptive/Reduced мають додаткові умови TransformProjectionSample щодо interval
і невиродженого span. Наприклад, baseline близько70 не означає коректний
independent reference: саме повторне захоплення такого B викликало попередній dip.

### Числовий приклад із явними передумовами

При `B=90.3115°`, `A=32/9`, native reference aspect16/9:

| Policy | Native target | Setter endpoint | Writer endpoint |
| --- | ---: | ---: | ---: |
| Native | 70° | 70° | 108.940710° |
| Adaptive | 70° | 70.292987° | 109.235084° |
| Reduced | 70° | 80.150877° | 118.555157° |
| Disabled | 70° | 90.311500° | 127.118692° |

Це обчислення формул із поточного коду, не чотири runtime captures і не
підтвердження, що саме B90.3115 належить кожному переходу. Без встановленого B
коректний endpoint Adaptive/Reduced/Disabled у конкретній activation невідомий.

## Cinematic modes і aspect

| Effective selection | Native ENTER reference | Обчислений endpoint |
| --- | --- | --- |
| NativeHorPlus, correction увімкнена та aspect override дозволений | R | H(R,A_cinematic) |
| Legacy GameplayHorPlus | R | Та сама canonical NativeHorPlus семантика після migration |
| Cinematic correction вимкнена або Cinematic Aspect=Native | R | ENTER operand R без mod transform; це не доказ негайного запису R у camera field |

Джерела: src/config/feature_config.cpp, ParseCinematicFovMode,
CinematicFovModeName/нормалізація; src/cinematics/cinematic_fov.cpp,
SelectCinematicBaseline і TryTransformSelectedCinematicFov;
src/plugin/cinematic_lifecycle_transaction.hpp, ProcessAcceptedCinematicEnter;
src/cinematics/cinematic_aspect.cpp, OverrideEnabled.

Для Auto береться resolved effective aspect, для Forced16x9/21x9/32x9 —
відповідний aspect. Не підставляти viewport aspect замість cinematic selection
автоматично. При R90 та A32/9 endpoint126.869898°; при A16/9 —90°.
Gameplay RuntimeCombine — окремий Gameplay mode, не Cinematic FOV mode.

## Дві межі: що реально відомо

| Межа | Native activation / samples | Current mapped/retained output | Невстановлений endpoint/context |
| --- | --- | --- | --- |
| EXIT1→provider70 | runtime182050: start126.211, target70, receiver збігається з recovery owner | continuityStart126.211 із receiver+0x230; target108.941; Gameplay після endpoint108.941 | corrected/rendered start; policy-qualified B; active view consumption |
| Dialogue→ENTER2 | raw70→70.1061→…→126.87; authored ENTER R90 | перші writer126.87, потім raw70.1061 після authored_baseline_departure | current ENTER activation/setter→writer provenance; corrected start/view |

Джерело: ../evidence/u2-transition-policy-2026-11-13/runtime-182050.log,
ASI SHA-256 F07C4FC1F44783A46379D059F84BD9C4A5B077AA51CD877EE488D56CE5849CFD.
User visual result: стрибок на обох межах, на другій також зменшення FOV.
Жодна межа не має visual PASS.

Current U2 не реалізує всю таблицю Dialogue: TryMapPostExitProviderDialogueSample
обирає H(activation.nativeTarget,A), незалежно від Adaptive/Reduced/Disabled.
Для Native handoff взагалі не допускається. Заміна endpoint лише на ReducedTarget
також недостатня: наступний Gameplay writer при raw70 знову дасть108.941, якщо
не передано destination policy/context. Потрібна рівність endpoint та наступного
маршруту корекції, а не локально інше число в handoff.

## Native easing та кандидат mapping

Якщо для підтвердженої native branch встановлено `F=S+(T-S)w`, то
`w=(F-S)/(T-S)` відновлює вже eased weight, навіть коли easing нелінійний у часі.
Нелінійність у часі сама по собі не спростовує такий mapping. Умови: ті самі
activation/endpoints/branch, відомий FOV domain, придатний span і поточний sample.
Повторна initialization, інший producer або camera cut не продовжують цей доказ.
Джерело статичного обґрунтування й меж: POST_EXIT_NATIVE_BRANCH_FEASIBILITY_2026-11-11.md.

Чинний MapProjectionTransitionSample натомість відновлює progress у просторі
tan(F/2). Він загалом відрізняється від weight, відновленого з degree-linear
native blend. Безперервність цього mapping не доводить збереження саме native
eased weight. Вибір простору mapping залишається окремим рішенням після
перевірки конкретної branch; нову формулу в production тут не впроваджено.

## Умови готовності до наступної production-зміни

Для кожної межі потрібні окремі adapter rules: admitted native sample та domain;
corrected start з відповідною source/generation/boundary; destination policy і
baseline; actual aspect; cancellation; endpoint/handoff та наступний writer.
Ця таблиця встановлює формули endpoints і семантику Disabled, але не створює
нової authority із припущення про B або historical writer output.

Regression matrix: усі чотири Dialogue policies з canonical NativeHorPlus,
correction OFF, cinematic aspect selections, обидва напрямки; first/middle/last
samples та перший post-handoff output; settings change, rearm, source change,
foreign output, cut, missing evidence, нульовий span. Матриця визначена, але нові
tests/runtime на цьому аналітичному етапі не виконувалися.
