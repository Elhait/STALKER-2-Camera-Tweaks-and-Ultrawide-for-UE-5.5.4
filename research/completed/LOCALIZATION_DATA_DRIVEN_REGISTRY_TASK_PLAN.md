# Data-Driven Locale Registry Refactor Task Plan

## Objective
Replace per-language C++ enums, catalog members, resource parameters, selector branches and validation branches with a shared locale metadata registry keyed by stable locale code. Prove the generic path with only the existing English and Ukrainian catalogs.

## Established evidence and current state
- `LocalizationManager` accepts two explicit resource IDs, stores two named catalog members, validates those catalogs through explicit calls, and selects via an English/Ukrainian conditional.
- `config::OverlayLanguage` is a two-case enum used by persistence, runtime atomics, snapshots, mutation variants and UI selection; parsing/serialization and validation explicitly enumerate `en` and `uk`.
- The UI builds a two-entry language array manually and maps indices through a two-branch conditional.
- `.rc`, resource ID header, test/build scripts and localization audit each name `en` and `uk` independently.
- Catalogs are already embedded in the single ASI; JSON source files are not runtime-loaded.
- English is canonical/fallback. Invalid config code leaves the default English setting; invalid manager selection returns false and preserves the active locale. Missing keys render as `[missing: key]`.
- No changes to translation text, font, layout or camera behavior are approved. User explicitly prohibits Git commands.

## Approved scope
- Add one declarative registry for display name, stable code, JSON filename, canonical role and neutral embedded-resource identity.
- Convert configuration/runtime persistence and settings APIs from language enum to locale-code strings/descriptor lookup.
- Load/validate all registered embedded catalogs generically into a catalog collection; use canonical English as fallback and preserve missing-key behavior.
- Generate the UI language selector by iterating registry metadata.
- Convert resource IDs to language-neutral identifiers and preserve one declarative `.rc` entry per embedded catalog.
- Make localization audit discover registry entries and validate every registered catalog/resource generically.
- Add deterministic harness coverage for en/uk registry lookup, active-catalog switching, invalid-code behavior, registry-driven selector data and config fallback.
- Run localization audit, `test.cmd`, and `build-overlay-settings.cmd`; do not launch the game or use Git.

## Explicit non-goals
- No third or other new locale, translation additions/rewrites, font/layout/camera changes, external runtime JSON reads, or multi-file distribution.
- No Git status/diff/log or other Git commands.

## Expected files/areas
- New `src/localization/locale_registry.hpp`.
- `src/overlay/localization_manager.hpp/.cpp`, `localization_resource_ids.h`, `localization_resources.rc`, `renderer_runtime.cpp`, `localization_keys.hpp/.cpp`.
- `src/config/feature_config.hpp/.cpp`, `config_repository.cpp`, `config_template.cpp`.
- `src/plugin/runtime_settings.hpp`, `runtime.cpp`.
- Existing `locales/en.json` and `locales/uk.json` only for moving language display-name metadata out of catalogs while keeping displayed strings identical.
- `tests/overlay/localization_harness.cpp`, `tests/config/config_persistence_harness.cpp`, `tests/config/runtime_settings_api_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`, `test.cmd`, `build-overlay-settings.cmd`, `backlog/TASKLOG.md`.
- This plan, archived under `research/completed/` after validation.

## Batches and validation
1. Introduce the registry and generic manager loading/validation/selection API; test en/uk switching, missing keys and invalid-code retention with embedded resources.
2. Convert config/runtime/UI to locale-code identifiers, preserve config fallback and persistence, build selector from registry, and use neutral resource IDs. Extend config/runtime regressions.
3. Make the audit parse the registry and validate all entries/resources/catalogs generically; update build/test invocations. Run localization audit and `test.cmd`.
4. Build the stable single ASI and inspect changed paths/source references against scope. Stop; no game launch.

## Risks and safe-failure behavior
- Unknown locale codes must fail lookup without changing the active catalog; config loading retains canonical `en` as the default on unknown codes.
- Any missing/invalid embedded catalog or incompatible translation fails localization initialization safely, leaving existing missing-key diagnostics rather than reading external JSON.
- Registry resource IDs must match both the neutral RC identifiers and numeric resource header; audit verifies each association.
- Runtime locale state uses a pointer to immutable registry metadata internally to retain atomic snapshot behavior; APIs and persistence carry the stable code, not a per-language enum.
- Translation display names move to registry metadata with exactly the existing visible values; all other translation strings remain untouched.

## Stop conditions and phase gates
- Stop if implementing a generic registry requires runtime filesystem locale loading, multiple release files, language-specific runtime branches or changes outside the localization/config/UI plumbing listed above.
- Stop on failed audit/test/build; report the evidence and do not claim completion.
- No additional languages and no runtime/game validation.

## Final review
- User explicitly prohibits Git interaction; omit Git review and record that constraint.
- Compare actual changed paths and language-specific identifiers against this plan, append factual test/build results and validation limits to `backlog/TASKLOG.md`, then archive this plan.
