# Lifecycle Authority — дизайн до реалізації

Статус: DRAFT FOR DISCUSSION. Концептуальна ціль погоджена користувачем;
конкретний контракт переходів і міграція ще не погоджені.

Редакція 2 за user review: уточнити один послідовний working state для всіх
внутрішніх кроків події, один commit та dependency invalidation локальних
proposals. Для нового source, callback silence, CandidateCancelled, provider
mismatch та native effect failure задати evidence -> permissions -> fallback.
Заборонити автоматичний native admission після cancel та implicit старий reset
після unrelated reject. EffectResult — окрема корельована logical event, не
вигадане підтвердження наміру. Поведінка за браку доказів не доводить позитивної
працездатності; missing native proofs і validation gates залишаються явними.

Ціль: один авторитет прийняття lifecycle-переходів для всіх camera event inputs,
який керує кількома пов'язаними станами. Не один enum, не дубльований manager,
не лише остаточний writer selector. Спеціалізовані політики та структури можуть
залишитися, незалежні права прийняття lifecycle-рішень — ні.

Дозволений обсяг цього етапу: читання source/evidence, інвентаризація, формальна
модель, письмова перевірка на збережених послідовностях, research/plan документи.
Не змінювати production, тести, чинні architecture contracts або ASI; не
запускати нову збірку чи гру. Перед реалізацією потрібні обговорення та явний
вибір користувача. Це продовжує gate у RUNTIME_CAMERA_CONTEXT_ISOLATION_2026-11-17.md.

Результат: research/reports/LIFECYCLE_AUTHORITY_DESIGN_2026-11-17.md.

Критерії дизайн-етапу:

1. Для всіх знайдених production і альтернативних mutator paths вказані подія,
   нинішній writer стану та майбутній власник права рішення; приховані mutating
   queries, settings, reset і ранні виходи включені.
2. State product, constraints, transition/evidence rules, safety/liveness та
   boundary commit описані без вигаданих engine lifetime фактів.
3. Відомі успішні та невдалі captures пояснені цією моделлю; пропущені дані й
   залежність від user annotations позначені. Не називати ручну перевірку
   моделі executable replay, model checking або новим PASS harness.
4. Є карта заборони старих незалежних прав: mutable views/aliases не можуть
   залишитися другим записувачем; кожен early return завершує подію явним рішенням.
5. Перед реалізацією всі відкриті guard/termination питання мають погоджений
   результат або конкретне evidence-based обмеження. Неповні пункти не приховувати.
6. Кожен callback після commit використовує лише сумісні committed permissions.
   Внутрішні кроки не перечитують stale entry state і не віддають provisional
   rights назовні. Один складний event не розпадається на видимі superposed modes.
7. Safety та працездатність потрібних позитивних сценаріїв перевіряються окремо.
   Без незалежного Gameplay proof fail-closed не вважати виконаною liveness.

Поточний disposition: документаційні уточнення §5, §10, §11 внесено; перевірено
узгодженість формулювань із source/evidence. Незалежна виконувана модель,
негативні тести, proof позитивних native boundaries та migration approval НЕ
виконані. Production, тести, налаштування й ASI цією редакцією не змінюються.

Подальший порядок лише після погодження: незалежний oracle/sequence model і
червоні production-bound регресії → реалізація accepted transitions → перевірка
реальної відсутності bypass mutation/access → повний suite → звичайний Release
ASI за посиланням → один цільовий runtime. Складені раніше 62/62 не доводять нову модель.
