# U2 shadow: реалізація та ручний експеримент

Дата: 2026-11-13. Scope погоджений у чаті після [оцінки B](U2_EXIT_DIALOGUE_BOUNDED_ASSESSMENT_2026-11-13.md): мінімальна shadow instrumentation, focused regression, звичайний Release ASI та три контрольні переходи. FOV policy, U1-A, Gameplay RuntimeCombine, Dialogue guards і native hooks не змінюються. Runtime експеримент виконує користувач; у цьому етапі гру не запускаємо й ASI не встановлюємо.

## Реалізація

- [Producer resolver](../../src/hooks/blend_producer_resolver.hpp) шукає provider-resolution / constant-load / wrapper-CALL sequence у `.text`, валідує decode, CALL targets, nonnull branch, readable operand span і унікальність. Wrapper походить із чинного linked cinematic consumer resolver. Класифікація не залежить від числового FOV або SHA. Неоднозначність/невалідність залишає caller Unknown.
- У поточному actual EXE structurally resolved return дорівнює RVA `0x2E54582`. Операнд містить70 у **writable `.data`**, а не immutable immediate: попереднє слово «literal» не доводить незмінність цього значення. Read-only image test підтверджує request path, не semantic Dialogue ownership.
- Наявний successful-start hook класифікує return як `provider_producer_return`, зберігає activation epoch, receiver та settings epochs. Нового hook немає. Rearm/invalid fields залишаються під чинним tracker contract.
- Наявний blend sample містить conditional algebraic model: `w=(f-S)/(T-S)`; `H=C+(HorPlus(T)-C)*w`. C читається зі збереженого corrected EXIT endpoint лише для чинного recovery/source; settings/aspect мають відповідати спостережуваній activation. Невалідні/вироджені endpoints, позадіапазонний sample, невідома activation, інший receiver/policy/settings → unavailable.
- Native easing branch у наявній точці не встановлена. Тому результат завжди позначено `conditional-affine-hypothesis`, `shadow_mapping_verified=0`, `shadow_branch=unobserved`, `rendered_start=unobserved`. Значення XMM7/XMM8 не видаються за підтверджений progress. Числова крива — математична гіпотеза, не підтверджене виправлення чи вимір проєкції.
- Наявний writer correlation record оцінює pure recovery classifier на hypothetical input: `would-remap`, `would-pass-through`, `would-resume-gameplay` або unavailable/stale. Це прогноз classifier за спостереженим domain, не виконання всього writer з побічними ефектами. `would-remap` показує ризик повторного mapping; pass-through сам по собі не доводить безпечного routing. Settings/aspect change робить прогноз stale. Intervening writes і current-writer invocation lineage лишаються unknown.
- Instrumentation не присвоює XMM0/XMM1, FOV, camera fields чи production lifecycle. Звичайні correction policies продовжують працювати; `shadow_mutations=0` стосується тільки доданого observer.

## Межі запису та контрольні фази

Primary capture: чинні60 секунд /192 combined records; ручне одноразове озброєння на наступному accepted ENTER. Shadow поля додаються до sampled records, не генерують окремі записи щокадру. Deadline/budget не подовжуються при новій generation.

Контролі мають окремий auto-reset event `Local\STALKER2CameraTweaks.U2Control.<PID>`. Доступні рівно дві фази на процес, кожна20 секунд /128 provenance records. Event споживається існуючим Dialogue callback; polling worker відсутній. Handle належить Runtime і закривається на reset/rollback та через RAII. Controls не перезапускають primary capture і не вмикають shadow authority.

`U2_CONTROL_BEGIN` записує coordinator/exclusion/recovery state. `control_phase=1/2` у provenance records дає змогу відокремити окремі діалоги від post-EXIT phase0. Generation0 допустима лише як metadata для Gameplay control. Якщо coordinator не Gameplay або exclusion ще активна, фаза не є чистим контролем після recovery.

Sampler тепер порівнює FOV із **останнім записаним** sample: дрібні накопичені зміни не губляться назавжди. Deadline також обмежує тривалість контрольного observation window. При budget/deadline відсутність записів не означає відсутності callback.

Diagnostics OFF: нові reads/evaluations/event polling gated; event створюється лише під час Diagnostics ON initialization. Збирається один `STALKER2CameraTweaks.asi`, production profile. Створення окремого diagnostic ASI не передбачено.

## Один ручний сценарій

Підготовка: встановити кандидат вручну зі штатною назвою; `[Diagnostics] Enabled=true`; однакові настройки Gameplay/Cinematics/Dialogue та aspect у всіх фазах. Після запуску перевірити runtime SHA, `U2_SHADOW_CAPABILITY` (resolver validated), `FOV_CAPTURE_CONTROL available=1` і `U2_CONTROL_CAPABILITY available=1`.

Команди виконуються з кореня репозиторію. Скрипт сам знаходить єдиний Shipping process; якщо їх кілька, використати `-GameProcessId <PID>`.

1. Завантажити збереження перед Cinematic1. **До початку ENTER1** виконати:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\research\tools\ArmCinematicFovCapture.ps1
   ```

   Пройти Cinematic1 → проблемний Dialogue → Cinematic2 → EXIT2. Очікується `CINEMATIC_BLEND_CAPTURE_ARM` та start/sample/writer records. Якщо60с/192 records не охопили потрібні події, позначити прогалину; повторний сигнал не поновлює capture.

2. Дочекатися завершення EXIT2 recovery: `CINEMATIC_RECOVERY_COMPLETED`, coordinator Gameplay, exclusion не активна. Перед окремим звичайним діалогом виконати:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\research\tools\ArmCinematicFovCapture.ps1 -Control
   ```

   Запустити діалог найближчими секундами. `U2_CONTROL_BEGIN phase=1` зафіксує фактичний стан на першому callback. Невалідний/очищений recovery owner не заперечує попереднє completion — оцінювати разом із coordinator/exclusion і журналом переходу.

3. Для Gameplay → Dialogue завантажити звичайне gameplay-збереження без катсцени в цьому переході, дочекатися Gameplay, повторити команду `-Control`, запустити інший діалог. Очікується phase2. Це контроль нового gameplay-епізоду в тому самому процесі, а не доказ відсутності будь-якої попередньої катсцени за життя процесу.

Для кожної фази окремо повідомити видиме кадрування, показники Overlay, початок діалогу та чи відбувся новий cinematic ENTER. Контрольні фази дають boundary/provenance records, а не новий activation capture після завершення primary window. Не робити висновок про producer з відсутніх activation records у контролях.

## Критерії оцінки та зупинки

1. Producer path підтверджений саме для цього запуску лише за successful-start caller evidence, matching epoch/tick та receiver. Target70 або близький timestamp недостатні.
2. Наявність finite shadow output встановлює тільки узгодженість умовної affine моделі. Динамічна branch/FOV-space/rendered-start evidence може залишитися unavailable; не додавати нові hooks заради її вичерпного відновлення.
3. Прогноз `would-remap` відхиляє наївне setter-only застосування моделі без узгодженого writer routing. Unknown invocation provenance не компенсується числовою рівністю.
4. Якщо окремі діалоги працюють за Gameplay, а проблемний post-EXIT епізод — ні, це відрізняє умови transition і є підставою для обмеженого product рішення, але не є самостійним ownership proof.
5. Після цього одного експерименту зафіксувати результат або межу спостережуваності. Не вмикати автоматичну корекцію й не послаблювати guards як побічний результат shadow PASS. Новий live intervention scope потребуватиме явного рішення щодо math/routing і regression ризиків ENTER2/EXIT2.

## Validation

Чиста Release-збірка у новому `build-artifacts/cm/u2-shadow-final`: PASS,131 build steps, parallel1. Фінальна bounded incremental перевірка трьох targets: exit0, без timeout, cleanup verified. Попередній ASI `DB25DC0D…11D1FC01` збережений у своєму каталозі.

Кандидат: `build-artifacts/candidate-u2-shadow/STALKER2CameraTweaks.asi`, FileVersion3.2.2.0. SHA-256 `DFC1B098D610AE3E5BE14C6BE38CA3B190A34AA6AD7F6C858EED392B0F8D74F7`. Published artifact і внутрішній build output збігаються за hash. Game image SHA-256 незалежно перевірено: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.

**16/16 focused process runs PASS**, усі без timeout, `CleanupVerified=true`, remaining processes empty:

- `u2-shadow` ON/OFF: structural resolver негативні випадки, незалежність від literal FOV, native/transformed-start conditional math, відмова при недостатніх даних, double-mapping forecast,2 control events/cleanup, generation0 і deadline/cumulative sampler; actual adapter ON/OFF з перевіркою незмінних context/native camera, settings invalidation.
- `diagnostic-work` ON/OFF: існуючі native adapter fixtures і10000 callbacks з bounded file output; ON —13 sampled file records, OFF —0 у цьому fixture.
- `recovery`, `recovery-foreign-source`, `cinematic-initial-gameplay-baseline`, `cinematic-continuation-combine`, `dialogue-exiting`: кожен ON/OFF.
- `consumer-resolver-image`: read-only SEC_IMAGE_NO_EXECUTE mapping, unique producer return `0x2E54582`, observed operand70; native game code не виконувався.
- `dialogue_source_provenance_harness`: PASS.

Збережено [результати](../evidence/u2-shadow-2026-11-13/clean-focused-results.json), [збірку](../evidence/u2-shadow-2026-11-13/clean-build.log), [incremental/cleanup check](../evidence/u2-shadow-2026-11-13/final-incremental-check.json) і [artifact/source metadata](../evidence/u2-shadow-2026-11-13/validation-summary.json).

Під час першого прогону старий `test-suite` cache давав access violation на Diagnostics ON. Конкретний build defect: shared `dialogue_source_provenance.cpp.obj` датований02:12, header зі зміненим layout —13:42, Ninja повідомляв `#deps 0`. Це дозволяло з'єднати різні ABI layouts структури. У новому каталозі зі штатною UTF-8 console code page цей object має83 tracked dependencies; fresh diagnostics ON проходить без поведінкового виправлення цього шляху. [Початкову невдачу збережено](../evidence/u2-shadow-2026-11-13/stale-cache-failure.json). Точну faulting instruction не відлагоджували; успішний cached PASS не використовувався як фінальний доказ. Build scripts проєкту не змінювали; власні build commands використовували `chcp65001` після VsDevCmd, як штатний build helper.

`git diff --check`: PASS (лише попередження LF/CRLF). Script syntax: PASS. Git index/branch не змінено. Full NonGraphics, visual correctness і три ручні game phases — **not exercised**, очікують користувацького runtime. Гру не запускали, ASI не встановлювали. Успішні focused checks підтверджують instrumentation/model contract у fixtures, не безпечну live FOV correction.
