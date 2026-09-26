# 18-Locale Overlay Localization — Task Plan

Status: implementation and scoped offline validation complete; task-log closure is pending user confirmation. Runtime/visual evidence is intentionally not part of this task.

## Objective

Add the 16 missing overlay localization catalogs to the existing data-driven locale registry, validate and embed all 18 catalogs in the combined overlay integration ASI, and establish script-specific font/rendering limits without claiming behavior that has not been demonstrated.

## Established evidence and current state

- `locales/en.json` is the canonical catalog; `locales/uk.json` is the only current translation. Both contain 164 keys.
- `src/localization/locale_registry.hpp` describes display name, code, filename, numeric embedded-resource identity and canonical status. `LocalizationManager`, configuration persistence and the selector already use locale-code/registry-driven paths.
- `src/overlay/localization_resources.rc` has one declarative RCDATA entry for each catalog. The production runtime loads embedded resources, not JSON files from disk.
- `tests/runner/localization_catalog_audit.ps1` discovers locale descriptors from the registry and checks UTF-8, canonical key parity, placeholders and resource associations. Its technical-identifier checks and registry parsing need review before relying on them for 18 locales.
- Current font setup in `src/overlay/localization_font.cpp` uses ImGui's default ProggyClean, merges Proggy Vector Cyrillic, and obtains supplemental punctuation/symbol glyphs from Windows Segoe UI. It does not currently establish Arabic, Japanese, Korean or Chinese glyph coverage, shaping, bidi or RTL correctness.
- `assets/fonts/NotoSansMono/` exists, but current production overlay font loading and resource embedding do not use it. Its needed script coverage, license/distribution implications, resulting atlas size and combined ASI size have not been established.
- `build.cmd` produces the camera-only ASI and is not the target. The agreed target is the combined overlay integration artifact produced by `tools/build/build-overlay-settings.cmd`; this task does not redesign the release/build pipeline.
- Serbian's agreed script/locale identity is `sr-Cyrl`.
- Batch 0 representative cmap probing found Proggy Vector and Noto Sans Mono sample coverage for Latin Extended/Cyrillic/Turkish, but not Arabic/Japanese/Korean/Chinese. Windows Segoe UI covered sampled Arabic but is machine-local and not embedded; it did not cover sampled CJK/Hangul. This does not establish complete codepoint coverage or shaping/rendering.
- The combined build currently uses stb and embeds Proggy Vector only for Cyrillic; FreeType is not integrated in the target build. Arabic shaping/bidi/RTL and CJK/Hangul rendering remain not established and do not block catalog work.
- The user subsequently approved a minimal correction to two existing Ukrainian runtime tooltips to retain the literal `ADS`; the corresponding audit exceptions were removed, so all 18 catalogs now receive the same technical-identifier check.
- Batch 1/2 implementation contains all 18 registry descriptors, the 16 new catalogs and neutral embedded resource associations, plus registry-wide deterministic localization and persistence coverage.
- Final offline validation passed on 2026-09-24: `tests/runner/localization_catalog_audit.ps1` passed for 18 locales and 164 keys; `test.cmd` discovered/compiled/executed all 41 runner sources and all harnesses passed; `build-overlay-settings.cmd` completed and produced `STALKER2CameraTweaksOverlayIntegration.asi` (2,367,488 bytes). Existing third-party compiler warnings were observed for Zydis anonymous structs and an unused DX12 backend local.
- No game was launched and no Git command was used. The resulting build is not runtime/visual validation.

## Approved scope

- Add exactly these 16 locale catalogs and registry entries, bringing the registry to 18:
  - `ar`, `cs`, `fr`, `de`, `it`, `ja`, `ko`, `pl`, `pt-BR`, `ru`, `sr-Cyrl`, `zh-Hans`, `es-419`, `es-ES`, `zh-Hant`, `tr`.
- Preserve canonical English keys, placeholder names/semantics, technical identifiers, aspect-ratio literals, existing `en`/`uk` wording, persistence semantics and `[missing: key]` diagnostics.
- Add one declarative embedded-resource mapping per catalog and keep all runtime catalog loading inside the single combined overlay ASI.
- Keep locale handling generic: do not add per-locale runtime branches, catalog members, API parameters, selector branches or validation paths.
- Examine and, only where evidence supports a bounded integration, extend font glyph coverage for text actually used by catalogs. Record Arabic shaping/bidi/RTL separately; do not represent glyph presence as proof of correct Arabic presentation.
- Run localization audit, deterministic test suite and the combined overlay build. Do not launch the game in this task.
- Do not use Git.

## Explicit non-goals

- No changes to camera behavior, Gameplay/Cinematics/Dialogue semantics, unrelated overlay behavior, layout architecture or non-locale configuration identifiers.
- No camera-only `build.cmd` target changes or release/build-pipeline redesign.
- No additions beyond the specified 18 locales, and no new translation framework/service.
- No claim of visually correct Arabic shaping/RTL, CJK presentation or in-game rendering based only on offline validation.
- No game launch, runtime visual test, packaging/upload or Git operation.

## Expected files or areas

- `locales/{ar,cs,fr,de,it,ja,ko,pl,pt-BR,ru,sr-Cyrl,zh-Hans,es-419,es-ES,zh-Hant,tr}.json` (exactly one file per code).
- `locales/README.md` for the expanded catalog/script and translation contract.
- `src/localization/locale_registry.hpp`, `src/overlay/localization_resources.rc`, `src/overlay/localization_resource_ids.h` for declarative metadata/resources only.
- `src/overlay/localization_font.cpp/.hpp`, font assets/resource declarations and the relevant combined build inputs only if font coverage work is evidenced as necessary.
- `tests/runner/localization_catalog_audit.ps1`, localization/config/runtime-settings harnesses and `test.cmd` only as required for data-driven regressions.
- `tools/build/build-overlay-settings.cmd` only if embedding or font-resource integration requires an input-list change. `build.cmd` is intentionally untouched.

## Batches, validation and gates

### Batch 0 — Plan review and script/font evidence gate

- Confirm this plan before implementation.
- Inspect actual font cmap coverage and license for each required script, ImGui builder/backend selection, catalog-derived glyph demand, atlas-size behavior, and embedded font/ASI size impact. Static file inspection is sufficient; do not launch the game.
- Determine whether Arabic shaping/bidi/RTL requires a separate rendering subsystem. If so, proceed with catalog/registry work independently, keep Arabic catalog support distinct from Arabic rendering support, and report that presentation acceptance as deferred rather than adding an unvalidated workaround.
- Validation: representative font cmap/backend inspection was recorded above; font-dependent rendering decisions remain deferred where evidence is insufficient.
- Gate: stop only font-dependent implementation decisions that lack evidence; this must not block translation or generic catalog integration.

### Batch 1 — Locale catalog data and metadata

- Translate every canonical key into the 16 specified locales using the current localization contract. Preserve `{placeholder}` names and semantics, mode/config identifiers, aspect ratios, key names and certainty level. Use native display names and the agreed Serbian Cyrillic script.
- Add descriptors with unique stable codes, filenames, display names and unique neutral embedded resource IDs; retain English as the sole canonical catalog. Canonical persisted code spelling follows each descriptor, including BCP 47 casing for script/region subtags (`pt-BR`, `sr-Cyrl`, `zh-Hans`, `es-419`, `es-ES`, `zh-Hant`). Generic lookup accepts case-insensitive input and resolves it to canonical descriptor spelling; this must not add locale-specific branches.
- Add a declarative RCDATA association for every new JSON; runtime must not read locale files from disk.
- Validation: strict UTF-8/JSON parse, exact key-set and placeholder parity against `en`, unique code/name/file/resource metadata, and checks for required canonical identifiers. Final audit passes for all 18 locales; Ukrainian `ADS` strings were minimally corrected per later user direction.
- Safe failure: do not proceed to resource/runtime integration with malformed or incomplete catalogs; preserve the existing `en`/`uk` files unchanged.

### Batch 2 — Generic catalog loading, selector, persistence and tests

- Keep existing generic runtime code; change it only if a concrete contract defect is demonstrated. Generalize deterministic coverage into registry sweeps for descriptor lookup, all embedded catalogs, locale switching, persistence round-trip, invalid-code fallback, canonical English fallback, missing-key output and selector metadata.
- Ensure tests do not create 16 per-locale branches and do not rely solely on source-regex assertions for runtime invariants.
- Extend the audit to check every registered locale for strict UTF-8, exact keys, placeholders, resource/file mapping, unique stable IDs, selector metadata and technical-identifier invariants. Avoid duplicating the locale list in validation code.
- Validation: audit passed; registry-wide localization and configuration persistence checks passed within the full 41-runner `test.cmd` suite.
- Safe failure: invalid/missing embedded resource or invalid catalog fails initialization safely; invalid persisted locale falls back to canonical `en` per existing behavior.

### Batch 3 — Bounded font/glyph integration

- Based on Batch 0 evidence, integrate only the font assets/ranges needed for the actual catalog glyphs and only if licensing, atlas/resource size and the current ImGui backend permit a maintainable single-ASI result. Preserve English/ProggyClean and existing Cyrillic presentation unless evidence requires a scoped change.
- Prefer validating glyph coverage from the complete translated catalog text rather than assuming a broad Unicode range guarantees a usable glyph. Include atlas size/limits and fallback/missing-glyph behavior in deterministic checks where feasible.
- Do not implement partial Arabic shaping/bidi as if it were correct. If correct RTL needs substantial architecture, leave Arabic catalog/locale integration in place, record Arabic presentation as explicitly unsupported/not established, and stop that sub-scope for a separate decision.
- Validation: existing English/Ukrainian font/atlas harnesses pass. No expanded font integration was made because available evidence does not establish complete embedded glyph coverage for Arabic/CJK/Hangul or shaping correctness.
- Safe failure: if font coverage or atlas impact is unacceptable, preserve the existing font path and clearly separate catalog availability from rendering limitations.

### Batch 4 — Final audit and combined overlay build

- Run the full localization audit, `test.cmd`, then `tools/build/build-overlay-settings.cmd` for the agreed combined overlay integration ASI. Inspect the exact artifact path and size. Do not run `build.cmd` as a substitute and do not launch the game.
- Validation passed: localization audit, `test.cmd`, and `build-overlay-settings.cmd`; output `STALKER2CameraTweaksOverlayIntegration.asi` (2,367,488 bytes). This proves offline catalog/resource/runtime contracts and build correctness only; it does not prove visual glyph presentation, Arabic shaping/RTL or game integration.
- Stop after successful scoped checks and report completed, deferred, blocked and not-runtime-validated criteria. Do not write a task-log completion entry until required validation passes and the user confirms task closure.

## Risks and rollback / safe-failure behavior

- Translation errors can change technical meaning despite passing structural audits. Require review of full catalog semantics, especially status certainty, Camera State distinctions and technical identifiers.
- Numeric RCDATA IDs can collide; uniqueness checks and resource-table verification must fail the audit/build before artifact handoff.
- Large CJK glyph sets can inflate atlas and ASI size or exceed renderer limits. Measure actual catalog-derived glyphs and resource impact before choosing ranges/assets.
- Arabic glyph coverage does not provide contextual shaping, bidi ordering or right-to-left layout. Do not claim those properties without an appropriate implementation and visual evidence.
- Existing Windows Segoe UI fallback is machine-dependent and is not embedded. Do not treat its presence on the developer machine as single-ASI glyph coverage.
- If any new catalog/resource/font path fails validation, do not hand off a partially localized artifact as supporting all 18 languages; retain source changes for diagnosis and stop at the failed batch.

## Stop conditions and phase gates

- Stop before implementation until the user approves this plan.
- Do not broaden into build-pipeline redesign, camera semantics, general layout work or game/runtime testing.
- Pause only the dependent font/rendering sub-scope if evidence shows substantial shaping or rendering work; continue independent catalog and generic registry work only within the approved task scope.
- Stop if a translation or font decision requires a product choice beyond the accepted locale list/script contract.
- Stop after audit, full tests and combined overlay build; wait for separate authorization for runtime/visual validation.

## Final review requirements

- Compare changed paths against this plan by read-only source/path review; Git remains unused as directed.
- State the exact combined artifact/profile built, audit/test/build results, warnings, and all rendering limitations. Do not call glyph coverage visual proof.
- Classify every batch and acceptance item as complete, incomplete or follow-up; leave completion/task-log closure pending user confirmation.
