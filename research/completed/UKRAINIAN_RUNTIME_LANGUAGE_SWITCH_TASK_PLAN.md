# Task Plan — Persisted English/Ukrainian Runtime Language Switch

## Objective

Apply the approved Ukrainian editorial pass, integrate an immediate English/Ukrainian selector backed by stable `en`/`uk` config identifiers, preserve the one-ASI deployment model, validate the change, and produce one fresh ASI for the user's first Ukrainian screenshot test.

## Established Evidence and Current State

- The user reviewed all 162 Ukrainian strings and approved an editorial pass with specific wording and terminology corrections. The remaining runtime integration is explicitly authorized by that review.
- `locales/en.json` is the typed canonical catalog; `locales/uk.json` matches its current 162 keys/placeholders. The preceding batch embedded English, Ukrainian, and the OFL-licensed Noto Sans Mono font as RCDATA and validated glyph coverage.
- `LocalizationManager` currently supports only English and a one-shot catalog load. The renderer already owns it and performs translations at draw time.
- `FeatureConfig` is parsed from INI; managed template synchronization knows five sections and currently has no `[Overlay] Language` key. The existing runtime-settings API supports typed mutations, snapshots, and persistence.
- Config persistence uses staged temporary writes and replacement, preserving the last-known-good file on failure. Language is presentation-only and must not affect camera/runtime semantics.
- `test.cmd` is the repository's full deterministic validation entry point; `build-overlay-settings.cmd` creates `STALKER2CameraTweaksOverlayIntegration.asi` and embeds resources.
- `docs/assistant/implementation-guidelines.md`, `docs/assistant/testing-guidelines.md`, `docs/assistant/architecture-guidelines.md`, and `docs/code-style.md` are absent in this checkout. Existing `docs/ARCHITECTURE.md`, `docs/SAFETY_INVARIANTS.md`, localization plans/contracts, config/runtime source, and related harnesses were reviewed.
- The user prohibits all Git interaction. No Git commands or state inspection will be used.

## Approved Scope

- Apply the reviewed Ukrainian corrections to the identified translation units; retain the approved `Native FOV` technical label and `Gameplay camera writer` meaning.
- Add typed catalog keys for the language setting and native language names. Keep `English` and `Українська` as stable self-names in both catalogs.
- Add `config::OverlayLanguage` values persisted only as `en` and `uk`; missing/invalid values fall back to English.
- Add `[Overlay] Language=en` to new and synchronized managed INI templates without disturbing existing user values or unknown sections.
- Extend the typed runtime-settings mutation/snapshot/persistence path for the presentation-only locale setting.
- Load and validate both embedded catalogs once, allow in-memory locale switching without restart or filesystem access, and update the renderer immediately when selection changes.
- Place the selector in the existing Overlay settings section. Do not add Auto or other locales.
- Extend localization, config, runtime-settings, resource, and UI source audits as needed.
- Run `test.cmd`, `build-overlay-settings.cmd`, and read-only path/diff checks without Git; deliver only the fresh ASI and source changes. Do not launch the game.

## Explicit Non-Goals

- Auto/game-language detection, additional languages, external locale files, language downloads, or third-party runtime dependencies.
- Camera, gameplay, cinematic, dialogue, hotkey, input-capture, or overlay placement semantics.
- Game launch, injection, or agent-run screenshot capture. The user performs the runtime screenshot test with the delivered ASI.
- Git interaction of any kind.

## Expected Files or Areas

- `locales/en.json`, `locales/uk.json`, `locales/README.md`.
- `src/config/feature_config.hpp/.cpp`, `config_repository.cpp`, `config_template.cpp`.
- `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`.
- `src/overlay/localization_keys.hpp/.cpp`, `localization_manager.hpp/.cpp`, `renderer_runtime.cpp`.
- `tests/config/config_persistence_harness.cpp`, `tests/config/runtime_settings_api_harness.cpp`, `tests/overlay/localization_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`, `test.cmd`, `build-overlay-settings.cmd`.
- `STALKER2CameraTweaksOverlayIntegration.asi` build output and `backlog/TASKLOG.md` after relevant validation.
- This plan, archived under `research/completed/` after completion.

## Implementation Batches and Validation

### Batch 1 — Editorial corrections and config contract

- Apply only the user's listed wording/terminology changes.
- Add validated `en`/`uk` parsing/serialization, default/fallback behavior, `[Overlay] Language=en`, and managed template synchronization.
- Validation: config persistence harness covers new, missing, invalid, and preserved user-config cases; catalog audit checks exact typed key and placeholder parity.

### Batch 2 — Runtime manager/API/renderer integration

- Load and validate both embedded catalogs once; switch the active catalog in memory.
- Add the typed runtime setting and snapshot value, atomic runtime ownership, and INI persistence through the established API.
- Add a native-name language selector in the Overlay section. It must apply immediately and all later UI/notifications use the new locale; no language name is localized into a different language.
- Validation: localization harness verifies both embedded resources, immediate switch in both directions, missing-key diagnostics, and Ukrainian string retrieval; runtime-settings harness verifies typed mutation/snapshot/persistence routing.

### Batch 3 — Full validation and single-file artifact

- Run catalog/source audit, `test.cmd`, and `build-overlay-settings.cmd`.
- Verify the ASI contains both catalogs and font resources and that no locale sidecar is required. Do not launch the game.
- Review changed paths and task plan scope without Git.

## Risks and Rollback / Safe-Failure Behavior

- Invalid or missing config values resolve to English, never to an unavailable locale. Both catalogs are validated before switching is enabled.
- The UI selection must update the locale immediately while persistence uses the existing safe staged-write path. If persistence fails, report/log the failure without corrupting the prior INI; the current session may retain the selected locale.
- Text length may affect layout. Preserve bounded parent/column widths and existing wrapping; do not allow Ukrainian text to resize the overlay from content.
- Dear ImGui owns no copy of the font bytes because they reside in the process-resident ASI resource. Preserve the ASI lifetime contract.
- Any failure in config, locale validation, test, or build stops delivery; do not launch the game to compensate.
- Rollback remains limited to localization/config presentation paths and tests; do not revert unrelated user work.

## Stop Conditions and Phase Gates

- Stop if accepting the language mutation requires camera/runtime behavior changes, if the managed template would not preserve existing user settings, or if either catalog/resource fails validation.
- Stop on any deterministic test or production build failure.
- On successful build, prepare the ASI for the user and stop before game launch; runtime visual behavior is not agent-validated.

## Expected Final Git Review

The user expressly prohibits all Git interaction, so no Git review or Git-state inspection will be performed. Final review will compare exact touched paths against this plan and report validation limits; the Git review requirement is omitted by user instruction.
