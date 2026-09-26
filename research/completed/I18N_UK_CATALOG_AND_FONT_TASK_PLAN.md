# Task Plan — Ukrainian Catalog Draft and Embedded Cyrillic Font

## Objective

Prepare and validate a complete Ukrainian translation catalog and bundled-font support for Ukrainian glyphs as independent workstreams in one bounded batch. Stop after deterministic validation and present `locales/uk.json` for the user's editorial review. Do not integrate runtime language selection or produce a runtime ASI build in this batch.

## Established Evidence and Current State

- `locales/en.json` is the canonical nested English catalog and has 162 typed localization keys.
- The catalog contract requires exact key paths and placeholder names to match English, preserves technical identifiers, and retains uncertainty distinctions.
- English is currently the only `Locale` in `LocalizationManager`; the English catalog is embedded as RCDATA resource 101.
- The current renderer relies on Dear ImGui's default font. No project-owned `.ttf` or `.otf` asset was found in `external/`; the default glyph range does not establish Ukrainian Cyrillic coverage.
- `localization_catalog_audit.ps1` currently validates one catalog against typed keys and English-specific invariants; no locale-to-locale parity validation exists yet.
- `docs/assistant/implementation-guidelines.md`, `docs/assistant/testing-guidelines.md`, `docs/assistant/architecture-guidelines.md`, and `docs/code-style.md` are absent in this checkout. `docs/ARCHITECTURE.md`, `docs/SAFETY_INVARIANTS.md`, `LOCALIZATION_EMBEDDING_TASK_PLAN.md`, `I18N_CATALOG_SCHEMA_REFACTOR_TASK_PLAN.md`, `locales/README.md`, and the current catalog/resource/audit sources were reviewed instead.
- The user prohibits all Git interaction. No Git commands, state inspection, staging, commits, or cleanup will be performed.

## Approved Scope

- Create `locales/uk.json` translating the complete canonical English catalog, including the new language-setting label if needed by the schema for the eventual selector.
- Add deterministic catalog checks for exact English/Ukrainian key parity, placeholder parity, valid UTF-8, and retained canonical technical identifiers.
- Select one suitable redistributable Unicode font with a clear embedding license, bundle it as a build-time/embedded resource, and establish deterministic Ukrainian glyph coverage.
- Update only the documentation needed to record the translation/font resource contract if the existing locale README requires it.
- Use stable locale identifiers `en` and `uk` as the future persistence contract; display names are `English` and `Українська`. No selector/config/runtime switching is implemented in this batch.
- After catalog/font tests pass, stop and provide `uk.json` for editorial review.

## Explicit Non-Goals

- Language selector, `[Overlay] Language` parsing/persistence, locale manager switching, or renderer integration.
- Auto detection, `auto`, or any language other than English and Ukrainian.
- Final production/runtime ASI build, game launch, injection, or runtime smoke test.
- Changes to camera, gameplay, cinematics, dialogue, hotkey, input-capture, or other runtime semantics.
- Git interaction of any kind.

## Expected Files or Areas

- `locales/uk.json` and possibly `locales/README.md`.
- `src/overlay/localization_resources.rc` and resource ID declarations, only as needed to embed the font while preserving the existing English resource.
- A legally redistributable font asset and its license/attribution in the project's asset/vendor area.
- `tests/runner/localization_catalog_audit.ps1` and localization test harness/build wiring for deterministic locale and glyph coverage checks.
- `backlog/TASKLOG.md` for this completed, validated preparation batch.
- This plan, archived under `research/completed/` only after the bounded batch passes; if stopped for user review, retain it as an in-progress plan until that checkpoint is reported.

## Batches and Validation

### Batch 1 — Independent catalog and font preparation

- Translate all canonical English keys into natural Ukrainian without changing English meanings or technical identifiers.
- In parallel, select and bundle a Unicode/Cyrillic font with an explicit redistributable license; add Ukrainian glyph-range support and a deterministic check against the actual font resource.
- Extend catalog validation to require identical flattened key sets, identical placeholder multisets, valid UTF-8, and unchanged canonical identifier values in both catalogs.
- Validation: run catalog audit and localization/font deterministic tests only. Do not run the full application build or launch the game.
- Stop after tests pass and present the complete `uk.json` to the user for editorial review. No runtime-language integration follows until the user reviews the catalog.

## Risks and Rollback / Safe-Failure Behavior

- Font license or source provenance must be unambiguous before embedding. If that cannot be established quickly, stop and report the font blocker rather than using a system font or unclear asset.
- A font may claim Cyrillic support but omit Ukrainian-specific letters or symbols used in strings. Validate glyph indices for every Unicode scalar in both catalogs, including `ІіЇїЄєҐґ`, before declaring coverage.
- Translation may inadvertently strengthen certainty or alter camera terminology. Preserve the glossary and semantic distinctions in `locales/README.md`; user editorial review is a required gate.
- Failed validation must not produce a user-facing runtime artifact. Keep changes confined to catalog, font resource, and localization test/build wiring so the batch can be corrected without touching camera logic.
- Existing English resource ID and single-ASI deployment contract must remain intact; this batch does not create a distributable ASI.

## Stop Conditions and Phase Gates

- Stop if font redistribution rights are unclear, a required glyph is missing, catalog parity/placeholder/UTF-8 checks fail, or the resource wiring would require runtime locale/config integration.
- Stop after successful deterministic validation and await the user's editorial review of `locales/uk.json`.
- Do not proceed to language/config/renderer integration, full production build, or runtime testing in this batch.

## Expected Final Git Review

The user expressly prohibits all Git interaction, so no Git review or Git-state inspection will be performed. Final review is limited to the exact touched paths, deterministic validation outcomes, retained English catalog/resource behavior, and alignment with this plan.

## Batch Outcome

- Added the complete 162-key `locales/uk.json`; key parity and placeholder parity passed.
- Bundled Noto Sans Mono Regular v2.014 under SIL OFL 1.1 with adjacent license and provenance notes. The font is embedded with both catalogs as RCDATA and registered with explicit Latin, punctuation, arrow, and Cyrillic glyph ranges.
- Extended deterministic localization coverage to compare the catalogs and build the actual ImGui atlas from the embedded font, then verify every Unicode scalar in every catalog value.
- Validation passed: test-runner source audit (`40/40` sets equal), localization catalog audit (`162/162`, placeholders PASS, embedded resources/font license PASS), and focused localization/font harness PASS.
- No full `test.cmd`, production ASI build, runtime launch, or Git interaction was performed. The task stops here for user editorial review of `locales/uk.json`; selector/config/renderer locale switching remains deferred.
