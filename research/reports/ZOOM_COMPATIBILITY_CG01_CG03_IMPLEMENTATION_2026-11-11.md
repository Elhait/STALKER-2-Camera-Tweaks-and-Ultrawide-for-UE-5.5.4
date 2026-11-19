# CG01–CG03 — реалізація та офлайн-перевірка

Дата: 2026-11-11. [План](../plans/ZOOM_COMPATIBILITY_CG01_CG03_2026-11-11.md), [початковий аудит](PRODUCTION_COMPATIBILITY_GATES_AUDIT_2026-11-11.md), [результати та ідентичності джерел](../evidence/zoom-compatibility-2026-11-11/validation-summary.json).

Production-виправлення CG01–CG03 реалізовані та офлайн-перевірені. Ручне приймання нового ASI в грі не виконано. Причина першого запуску без прийнятого cinematic ENTER залишається окремою відкритою задачею.

## Що змінено

**CG01.** `src/hooks/zoom_transition_resolver.hpp` знаходить по одному IN/OUT match, перевіряє executable `.text` span та декодує обидві MOVSS і UCOMISS. Перевіряються довжини, регістри, memory base/index/displacement: XMM0 з RAX+4C/50, XMM1 з RSI+138/13C, далі UCOMISS XMM1,XMM0. Hook лишається перед native compare. API resolver і caller не приймають SHA. Помилка обчислення SHA більше не пропускає Zoom resolution; SHA лишається provenance в журналі.

`InstallZoomTransitionObservation` створює обидва hooks через StartDisabled. Capability відкривається після успішного enable обох. Після відмови створення/активації або C++ exception capability лишається закритою, hooks відкочуються. Повторний виклик для вже committed активної пари ідемпотентний. Стан observer належить RuntimeState.

Callback перевіряє capability/stopping до та після захоплення існуючого CameraCorrectionGate. Під час install не утримується camera mutex. До commit та після невдалого install callback не торкається observation/counter state. Reset закриває capability перед існуючим global native-callback drain; час життя native continuations лишається під поточними гарантіями SafetyHook. Нових shutdown hooks, threads, polling або менеджерів lifetime немає.

**CG02.** Startup `Diagnostics` тепер описує обраний глобальний режим незалежно від Zoom; `ZoomObserver=AVAILABLE/UNAVAILABLE` — окреме поле. Resolver та install failures мають власні причини (`image-span-unavailable`, `signature-cardinality`, `instruction-contract`, `hook-create`, `hook-enable`, `installation-exception`). Деградація фіксується також при Diagnostics OFF. Успіх повідомляє native-contract validation і знайдені RVA.

**CG03.** `CameraStatePublicationInput` містить окрему observer capability. Підтвердження нейтрального `CameraState.evidence.gameplayFov` потребує доступного observer, валідного спостереження та inactive Zoom. Без цих доказів нове значення не підтверджується; попереднє стає historical. Поточні native/output FOV-факти продовжують публікуватися за власними контрактами.

До першого валідного inactive/OUT спостереження нейтральний Gameplay FOV може залишатися unavailable. Якщо OUT є першим таким спостереженням і старого нейтрального значення ще немає, запускається звичайна кваліфікація трьома стабільними samples: hold більше не очікує повернення до неіснуючого reference. Для вже відомого reference збережено hold/release contract. Це observation/presentation state; окремий production GameplayBaselineStore та алгоритми camera correction не змінено.

Оновлено поточні architecture/safety/manifest contracts. Інваріант явно забороняє production admission через SHA, номер патча або доступність disk hashing. Ізольовані fixed-RVA research probes не просувалися в production.

## Перевірки

- 17/17 focused invocations PASS: Diagnostics OFF/ON, resolver cardinality/operands/bounds, diskless synthetic executable, змінений timestamp та unrelated image bytes при незмінному native-коді, missing/duplicate/wrong-register/wrong-offset/truncated patterns, повторний committed install, injected first/second enable refusal.
- Під час справжнього production install callback викликається з окремого потоку до commit, зокрема коли перший real hook уже активний. Перевіряються відсутність нових sequence/counters, закрита capability, відсутність hooks після rollback і byte-identical native code. Окремо викликається callback після rollback. Це контрольовані відмови enable через adapter seam, не симуляція реального збою ОС.
- Camera State regression перевіряє unavailable capability, installed-but-unobserved state, початковий hold без reference та збереження historical reference після втрати capability. Старі fixtures, що потребують відомого inactive Zoom, тепер задають цей доказ явно.
- Чотири game-derived IN/OUT fixtures перевіряють реальні relocated compare/branch continuations, регістри, SIMD lanes і stack при Diagnostics OFF/ON.
- Повний `test.cmd -NonGraphics`: **62/62 PASS**, native matrix **222 fresh processes**, graphics case omitted. Canonical registry audit: 59 targets, 114 cases / 228 nominal child invocations.
- Початкові запуски в обмеженому середовищі отримали Access denied на hardlink/INI fixtures. Ті самі filesystem binaries окремо пройшли поза цими обмеженнями; після цього повний canonical прогін у таких умовах пройшов. Production filesystem code не змінювався.
- Isolated production Release build: exit 0. У виводі є vendor Zydis C4201 warnings; власних compiler errors немає.
- Supervisors підтвердили cleanup власних process trees. Release cleanup завершив очікуваний `vctip.exe`; фінальна перевірка не знайшла залишених compiler/test/Java процесів.

## Статична переносимість Zoom-пари

| Образ | IN / OUT matches | Hook RVA IN / OUT | Статус доказу |
| --- | --- | --- | --- |
| 2.0.2, SHA `F075…CC0C` | 0 / 0 | — | Цей native variant відсутній; observer fail-closed |
| 2.0.3, SHA `8196…3C69` | 0 / 0 | — | Цей native variant відсутній; observer fail-closed |
| 2.0.4, SHA `2ECC…A409` | 0 / 0 | — | Цей native variant відсутній; observer fail-closed |
| 2.0.5, SHA `E7B4…9293` | 1 / 1 | `0x604EF9` / `0x60509F` | Exact signature cardinality на retained Ghidra image |
| 2.0.6, SHA `61BC…D56B` | 1 / 1 | `0x602A79` / `0x602C1F` | Exact signature cardinality на retained Ghidra image; існуючі game-derived fixtures |
| Актуальний EXE, SHA `2BD3…5479` | 1 / 1 | `0x531AF1` / `0x531C9E` | Offline PE scan executable sections, обидва matches у `.text`; runtime pending |

[Актуальний PE scan](../evidence/zoom-compatibility-2026-11-11/current-image-scan.json), [retained read-only scan](../evidence/zoom-compatibility-2026-11-11/retained-image-scan.json). Headless Ghidra використовував наявні imports, `-readOnly -noanalysis`, один CPU; latest EXE не імпортувався. Початковий launcher завис і був завершений supervisor за timeout із verified cleanup; запуск із явним локальним JDK завершився. Ghidra також вивів compile errors іншого наявного research script; наш script виконався на всіх п'яти images та повернув наведені результати. Той сторонній script не змінювався.

Cardinality/байти не є доказом поведінки всіх native callers чи повної сумісності гри. Для 2.0.2–2.0.4 нові signature variants не додавалися. Версія latest EXE не вгадується за SHA.

## Артефакт і ручне приймання

ASI: `build-artifacts/cg01-cg03/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`.

- SHA-256: `55DB2E4C4DEB92F2BD2F7C1EE99D2E5336DB51C9F9C0331E3887DD2DB6C7D872`.
- FileVersion: `3.2.2.0`; розмір: 3 569 152 bytes. Номер версії не змінювався в цьому ремонті.
- Попередні root та встановлений ASI з SHA `A9A38A68E4F69FFB6C0127E4C8A430268C79C60AB7619BCF008E2805B91F9842` збережені незмінними. Автоматичного встановлення не було.

Для одного ручного запуску на актуальному EXE:

1. Звірити mod SHA в startup log; очікувати `ZoomObserver=AVAILABLE` та конкретні resolved RVA. Diagnostics status має відповідати INI незалежно від Zoom capability.
2. Виконати звичайний Zoom IN/OUT, перевірити bounded ZOOM events і факти Camera State. До першого достовірного inactive observation нейтральний reference може бути unavailable; після OUT і стабілізації має підтвердитися.
3. Повторний Zoom не повинен замінити neutral reference на zoom FOV. Поточний output і reference лишаються різними ролями.
4. Перевірити роботу Overlay та штатний вихід. Пропущений ENTER не закривати лише успішним запуском: за повторення потрібні окремі bounded callback-entry/early-return докази.

Гру, WinDbg, WARP та graphics fixtures автоматично не запускали. Жодного твердження про виправлення AV-детекцій цей етап не робить.
