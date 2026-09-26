# Task Plan — Canonical English Catalog and Localization Object Model

## Objective

Turn the current first-pass English overlay catalog into the canonical translation schema: remove redundant/non-localizable entries, correct and simplify its English, organize it as nested domain objects, and split localization ownership into explicit catalog, formatter, validator, and manager objects. Migrate all call sites to typed localization keys and preserve current UI behavior except for the requested wording corrections.

## Established Evidence and Current State

- The first-pass foundation currently has a flat 211-key `locales/en.json`, with irregular comma formatting, duplicated concepts, technical mode identifiers in the catalog, and the specific tooltip/status wording identified by the user.
- `src/overlay/localization.hpp/.cpp` currently combines catalog storage/parsing/formatting in `EnglishCatalog`; the parser accepts only flat string maps and a global `GetEnglishCatalog()` exposes mutable state.
- Renderer code uses free `Tr()` helpers and raw key literals; `Renderer` does not own localization state.
- Notifications carry string title/value payloads, including delimiter-encoded values.
- `build-overlay-settings.cmd` and `test.cmd` include the localization source and deterministic harness. The current source audit checks missing flat keys.
- The prior accepted scope is English-only. No language resolver, additional catalog, language selector, runtime language switching, font expansion, or Auto detection is included.
- User has explicitly prohibited Git interaction. No Git commands, state inspection, staging, commits or cleanup will be performed.

## Approved Scope

- Audit and replace the flat `en.json` schema with readable nested JSON domains and conventional formatting.
- Remove duplicated strings by sharing keys only where semantics/context agree; retain separate text where translation context differs.
- Remove technical identifiers and layout/presentation tokens from localization (`HorPlus`, `AspectRecalculation`, `GameplayHorPlus`, `NativeHorPlus`, numeric aspect identifiers, `(?)`). Keep localized option words (Auto, Native, Adaptive, Reduced, Disabled) in the catalog.
- Apply requested corrections: remove misleading “only” and obsolete “recommended for most users”; simplify engineering language; replace “re-arm” and other implementation terminology with user-facing text.
- Introduce explicit `LocalizationCatalog`, `LocalizationFormatter`, `LocalizationValidator`, and `LocalizationManager` objects. The catalog flattens nested JSON internally; the manager owns current locale and catalog; the renderer owns its manager instance. No global mutable catalog or renderer-facing file/parser access.
- Replace raw localization string keys at C++ call sites with compile-time typed `LocKey` constants grouped by domain.
- Represent overlay notifications semantically, avoiding string-delimited payloads and prose construction in runtime publishers; compose setting/action labels through named placeholders.
- Strengthen deterministic validation for nested catalog parsing, duplicate/invalid entries, canonical-key parity, placeholders, UTF-8, missing-key fallback and locale ownership.
- Preserve behavior, technical identifiers and existing module-relative `locales/en.json` loading path.

## Explicit Non-Goals

- Additional locales, language selection, runtime switching among available locales, game-language detection/resolver, Unreal getter research or S2AE reuse.
- Font atlas/glyph work, Ukrainian rendering validation, pluralization/ICU or external translation tools.
- Camera, settings, hotkey capture/action, input, runtime hook, persistence, overlay placement or engine behavior changes.
- Game launch/injection, release packaging changes beyond the current ASI-adjacent locale directory, or Git interaction of any kind.

## Expected Files or Areas

- `locales/en.json`.
- `src/overlay/localization*.hpp/.cpp` and new typed localization-key declarations.
- `src/overlay/renderer_runtime.cpp/.hpp`, `feature_presentation.hpp/.cpp`.
- `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/discovery_runtime.cpp` for semantic toast payloads only.
- `build-overlay-settings.cmd`, `test.cmd`, localization test harness and source/catalog audit.
- `backlog/TASKLOG.md` after relevant validation passes.
- This plan, archived to `research/completed/` on completion.

## Batches and Validation

### Batch 1 — Contract audit and object model

- Inventory current key references, duplicate semantic values, dynamic toast payloads and JSON-parser assumptions.
- Implement nested-object catalog loading, typed key constants, separate formatter/validator/manager responsibilities and Renderer-owned manager lifecycle.
- Validation: compile/run isolated localization harness covering nested flattening, invalid shapes, duplicate flattened paths, placeholders, UTF-8, missing fallback and manager lifecycle.

### Batch 2 — Canonical English schema and UI migration

- Create the curated nested English catalog; update C++ references and semantic notification payloads.
- Preserve canonical identifiers in selectors/diagnostics, localize only the intended option labels and user-facing prose.
- Validation: enforce catalog key inventory equality and placeholder validity; audit technical identifiers/presentation markers are not catalog entries; inspect all source references for typed keys.

### Batch 3 — Integration validation and final record

- Run complete deterministic test suite and overlay build; compare changed files and outcomes against this plan; update task log; archive plan.
- No runtime/game test or Git review due explicit scope and user instruction.

## Risks and Safe-Failure Behavior

- Nested JSON recursion can accidentally accept unsupported data; restrict leaves to strings and objects, reject arrays/scalars, duplicate flattened keys, malformed UTF-8 and malformed placeholders.
- English key consolidation can merge strings whose grammar differs in another language; reuse only semantic labels with the same grammatical/UI role; keep full natural-language sentences as single translation units.
- Translation catalog failure must remain non-fatal to camera/runtime behavior and produce visible `[missing: key]` markers.
- Semantic notification refactoring must preserve event ordering/status and only alter presentation payload shape.
- Renderer must not perform parsing, file I/O, game-language detection or locale branching per frame.
- Rollback is limited to localization files/catalog/call sites/notification presentation and test wiring; do not revert user-owned unrelated work.

## Stop Conditions and Phase Gates

- Stop if a notification change requires altering mutation/hotkey action semantics.
- Stop if nested schema requires an external JSON dependency or touches unrelated packaging.
- Stop before adding another language, game-language resolver, font changes, runtime injection, game launch or any Git command.

## Expected Final Git Review

The workspace normally requires a read-only Git review. The user has expressly prohibited all Git interaction, so that review and any Git-state inspection are omitted. Final review will instead check exact touched paths, tests/build results and alignment with this plan.
