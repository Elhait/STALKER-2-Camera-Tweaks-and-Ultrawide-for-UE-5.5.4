# Task Plan — English i18n Foundation and Overlay Catalog

## Objective

Add a small, renderer-independent localization foundation for the existing Camera Tweaks overlay, move all user-facing English copy into a packaged canonical `en.json` catalog, and preserve current visible wording and behavior.

## Established Evidence and Current State

- The settings overlay is built by `build-overlay-settings.cmd` as `STALKER2CameraTweaksOverlayIntegration.asi`.
- `renderer_runtime.cpp` contains user-facing labels, explanations, tooltips, Camera State labels, mouse-capture notice and notification presentation.
- `feature_presentation.cpp` currently owns user-facing status labels/details as string literals.
- Runtime notifications currently carry title/value strings through `runtime_settings.hpp`; runtime and discovery paths publish them.
- The current source tree has no localization subsystem or JSON dependency. ImGui rendering consumes UTF-8 `char` strings.
- User explicitly requires no Git interaction. No Git commands, staging, commits, or other Git state changes are authorized.
- A prior reference-mod inspection suggests an `Auto` language feature exists there, but that is not runtime evidence and is outside this task.

## Approved Scope

- English-only localization infrastructure and catalog for the existing overlay frontend.
- Semantic translation keys and named-placeholder formatting.
- Load and validate the English catalog once during overlay initialization; do not parse JSON or load files per frame.
- Safe, visible fallback for missing keys and safe failure for absent/malformed catalogs without affecting camera/runtime functionality.
- Resolve packaged locale resources relative to the ASI/module location, not the game current working directory.
- Migrate user-facing renderer, feature presentation, and toast/notification text while preserving canonical setting identifiers and behavior.
- Deterministic tests for JSON/catalog validation, required keys, duplicate/invalid input, substitutions, missing-key behavior, UTF-8 and load-once architecture.
- Relevant overlay build and test validation only.

## Explicit Non-Goals

- Additional locale catalogs, language selector, game-language Auto detection, Unreal/game getter investigation, or runtime localization switching.
- Font atlas/glyph changes, translated Cyrillic display validation, pluralization/ICU, or external translation tooling.
- Camera, settings, hotkey, input, runtime hook, or persistence behavior changes.
- S2AE implementation/code reuse.
- Game launch, injected runtime test, release upload, or stable gameplay ASI changes.
- Any Git operation, including read-only inspection.

## Expected Files or Areas

- New localization API/source under `src/overlay/` (or a narrowly scoped `src/localization/` module if source ownership supports it).
- New packaged `locales/en.json` and overlay build/package wiring.
- `src/overlay/renderer_runtime.cpp`, `feature_presentation.hpp/.cpp`, and notification model/publish sites only as needed to pass semantic message identifiers and parameters.
- New deterministic localization harness and the existing test runner/audit wiring.
- `backlog/TASKLOG.md` after implementation and relevant validation pass.
- This plan, archived to `research/completed/` when the task is complete.

## Batches and Validation

### Batch 1 — Catalog contract and localization core

- Inspect exact renderer, notification, build/package and test-runner ownership.
- Implement strict-enough UTF-8 JSON catalog parsing/validation without adding an unnecessary external dependency.
- Add semantic lookup, named-placeholder substitution, missing-key marker and safe initialization behavior.
- Add canonical English catalog and deterministic isolated tests.
- Validation: compile/run localization harness; test malformed, duplicate, missing, placeholder and UTF-8 cases.

### Batch 2 — Overlay text migration and resource integration

- Migrate all user-facing copy in the approved overlay UI and presentation/notification paths to catalog keys.
- Keep technical setting identifiers, raw diagnostics, log messages, parser/config tokens and ImGui IDs canonical/nonlocalized.
- Load catalog once from a stable module-relative locale path; keep renderer free from file/JSON/fallback policy.
- Validation: source audit for remaining user-facing literals; full existing test suite; overlay integration build; verify expected locale asset placement/copy behavior.

### Batch 3 — Final bounded review and record

- Compare changed areas against this plan, record validation evidence and limitations, and finalize the factual task log.
- Archive the plan under `research/completed/`.
- Git review is explicitly omitted per user instruction; no Git commands will be used.

## Risks and Safe-Failure Behavior

- A hand-rolled JSON parser can be unsafe or incomplete. Keep the accepted schema minimal, reject malformed/duplicate keys, validate UTF-8 and escapes, and cover boundaries deterministically; prefer an already-present suitable parser only if source inspection confirms one.
- Missing/corrupt `en.json` must not crash initialization or affect camera functionality. UI should expose a clear fallback (e.g. `[missing: key]`) and log catalog failure without per-frame retries.
- Dynamic notification text can bypass migration if still assembled in runtime publishers. Model messages semantically and format at presentation time where practical.
- Preserve canonical enum/config identifiers exactly; localize surrounding descriptions only.
- Do not expand into non-English font support; English-only catalog uses ASCII unless existing approved wording requires UTF-8 punctuation.
- Rollback is limited to the new localization module/catalog/build wiring and associated presentation migrations; no camera/runtime behavior should be changed.

## Stop Conditions and Phase Gates

- Stop if the existing build/package path cannot place `en.json` beside the correct ASI without changing unrelated stable-release packaging.
- Stop if a suitable parser or schema decision would require adding a broad dependency or architectural subsystem beyond the English-only overlay contract.
- Stop if migrating notifications requires altering camera/settings semantics rather than presentation payloads.
- Stop before any additional language, Auto resolver, font/glyph, runtime injection, game launch, release or Git action.

## Expected Final Git Review

The workspace rules normally require a read-only Git review after implementation. The user explicitly prohibited all Git interaction; therefore no Git commands or Git-state inspection will be performed. Final review will instead compare the exact edited paths and validation results against this plan, clearly noting this limitation.
