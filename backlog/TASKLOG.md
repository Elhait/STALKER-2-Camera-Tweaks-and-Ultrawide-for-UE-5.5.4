# Task Log — Current

This is the active task log. New completed implementation, bounded research, and
approved cleanup batches are recorded here from 2026-09-22 onward.

## 2026-09-24 — Proggy Vector-only experimental ASI

- Scope: build one isolated experimental ASI using Proggy Vector as the sole
  overlay font source, retaining the existing runtime text-size control. No
  stable ASI/source changes, layout/localization/camera changes, game launch,
  or Git interaction.
- Evidence: user supplied Proggy Vector TTF/license are present in the prior
  experimental area; the output path was absent before the build. Static
  loader check confirms one memory-font load and no default font, merge, or
  additional font loader.
- Changed: experimental-only font loader, resource script/build script under
  `build-artifacts/experimental-font-ab/ProggyVectorOnly/`; output requested at
  `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationProggyVectorOnly.asi`;
  this log and plan archived under `research/completed/`.
  Intentionally untouched: stable ASI and all production sources/resources,
  localization catalogs, UI layout, camera semantics, and earlier experiments.
- Git: branch, commit and working-tree state not inspected, per explicit user
  prohibition; no Git commands were run.
- Validation: pre-build localization audit PASS (166/166 keys); experimental
  loader static checks PASS; build command exited successfully. Existing
  Zydis and ImGui vendor warnings only. No post-build inspection or game launch,
  per the requested stop boundary.
- Completed: experimental Proggy Vector-only ASI build command succeeded.
- Remaining: user visual/runtime comparison across text sizes. Deferred:
  deciding whether this font replaces the current production pairing. Blocked:
  none.
- Not runtime-validated: ASI not launched or injected.
- Patch summary: created a one-font experimental build path with runtime font
  size retained and embedded locale/font/license resources.
- Changelog summary: provides a Proggy Vector-only overlay ASI for visual
  comparison against the stable mixed-font version.

## 2026-09-24 — Font cold-start/rebuild parity investigation

- Scope: compare cold-start atlas creation against a different-size build,
  clear, and runtime-style rebuild to the same target size. No production
  behavior change, layout scaling, game launch, or Git interaction.
- Evidence: both paths use `AddProggyCleanWithCyrillic()` with the same size
  and source configs. Runtime waits for GPU completion, invalidates DX12
  objects, clears/rebuilds the atlas, and calls the same backend device-object
  creation routine as initial setup. Deterministic CPU atlas comparisons passed
  for 12, 16, 20, and 24 px: alpha texture dimensions/bytes, glyph metrics, and
  font source configs matched after intermediate-size rebuilds.
- Changed: only `tests/overlay/localization_harness.cpp` parity coverage, this
  task log, and plan archived at
  `research/completed/OVERLAY_FONT_REBUILD_PARITY_TASK_PLAN.md`.
  Intentionally untouched: production font/runtime code, layout, localization,
  stable ASI, and game installation.
- Git: branch, commit and working-tree state not inspected, per explicit user
  prohibition; no Git commands were run.
- Validation: `test.cmd` PASS, including all 40 runner sources and the new
  deterministic atlas parity assertions. No build requested because no
  production discrepancy was isolated.
- Completed: CPU-side startup/rebuild atlas equivalence is established for the
  tested sizes; backend code uses the same font texture creation routine.
- Remaining: user's claimed visual difference is not reproduced by static
  evidence. A direct runtime A/B (cold start at X vs change to X) is needed to
  determine whether a GPU/driver or observation/timing factor remains.
  Deferred: layout adaptation. Blocked: none.
- Not runtime-validated: no game launch or in-game A/B was performed.
- Patch summary: added byte-level atlas pixel, glyph metric, and source-config
  parity checks for cold-start and clear/rebuild creation paths.
- Changelog summary: tests now guard against regressions where runtime font
  rebuilding produces a different CPU atlas than cold initialization.

## 2026-09-24 — Proggy Vector stable font and integer text sizing

- Scope: promote the user-approved ProggyClean + Proggy Vector font pairing to
  the stable single-file ASI and rebuild the ImGui font atlas at the selected
  integer text size instead of scaling rasterized glyphs. No localization
  content, camera/runtime semantics, game launch, or Git interaction.
- Evidence: Proggy Vector includes all required Ukrainian glyphs; its supplied
  license is retained. Fractional `FontGlobalScale` was scaling the finished
  13 px atlas and degrading both Latin and Cyrillic text. The renderer already
  owns GPU synchronization and DX12 font-device-object recreation.
- Changed: embedded source font/license assets, font loader/resources,
  renderer atlas rebuild/rollback, localization font harness/audit,
  `THIRD_PARTY_NOTICES.md`, this log, and the plan archived at
  `research/completed/OVERLAY_PROGGY_VECTOR_FONT_SIZE_TASK_PLAN.md`.
  Intentionally untouched: localization strings/catalogs, camera behavior,
  input policy, font-size range/default, game installation, and prior test ASIs.
- Git: branch, commit and working-tree state not inspected, per explicit user
  prohibition; no Git commands were run.
- Validation: `test.cmd` PASS (all 40 runner sources compiled/executed and all
  deterministic harnesses PASS; catalog contract 166/166); catalog audit
  reports integer font sizes PASS; `build-overlay-settings.cmd` PASS. The
  stable ASI embeds the exact 399,528-byte Proggy Vector TTF and 5,148-byte
  license; both embedded SHA-256 values match their source files. Existing
  Zydis/ImGui vendor warnings only.
- Completed: stable ASI rebuilt with ProggyClean Latin, Proggy Vector
  Cyrillic, and atlas recreation at selected integer sizes with rollback.
- Remaining: user runtime visual check. Deferred: further visual font tuning.
  Blocked: none.
- Not runtime-validated: game was not launched; in-game appearance and live
  size changes await the user's test.
- Patch summary: replaced post-rasterization font scaling with synchronized,
  integer-size atlas rebuilding and safe restoration of the last known-good
  font when rebuilding fails.
- Changelog summary: the stable ASI now uses the approved Cyrillic companion
  font and supports sharper user-selected text sizes without fractional atlas
  scaling.

## 2026-09-24 — Proggy Vector Cyrillic companion test

- Scope: verify supplied official Proggy Vector glyph coverage/license and
  build one isolated test ASI with built-in ProggyClean retained for Latin.
  No production changes, game launch, or Git interaction.
- Evidence: supplied archive contains the official regular TTF and its license;
  cmap audit passed for all 66 uppercase/lowercase Ukrainian Cyrillic letters.
- Changed: extracted TTF/license, test-only resource script and build script,
  experimental resource ID, object files and ASI under
  `build-artifacts/experimental-font-ab/`; plan archived at
  `research/completed/PROGGY_VECTOR_CYRILLIC_TEST_TASK_PLAN.md`.
  Intentionally untouched: production font loader, production ASI and camera,
  localization and runtime behavior.
- Git: branch, commit and working-tree state not inspected, per explicit user
  prohibition; no Git commands were run.
- Validation: catalog audit PASS (166/166 keys); experimental ASI build PASS.
  PE data-only inspection confirms embedded Proggy Vector resource (399,528
  bytes; SHA-256 matches source) and license notice resource (5,148 bytes;
  SHA-256 matches source). Existing vendor warnings only. No game launch.
- Completed: `STALKER2CameraTweaksOverlayIntegrationProggyVector.asi` is ready
  for user visual comparison. English remains on the built-in ProggyClean.
- Remaining: user screenshot/visual assessment. Deferred: any production font
  promotion. Blocked: none.
- Not runtime-validated: the game was not launched; actual in-game appearance
  remains for the user to judge.
- Patch summary: added one isolated Cyrillic-only Proggy Vector test build and
  retained its licensing notice inside the ASI.
- Changelog summary: provides a Proggy Vector Cyrillic visual test while
  preserving the original ProggyClean Latin rendering and production ASI.

## 2026-09-24 — Build ProggyClean Cyrillic runtime A/B variants

- Scope: build separate GohuFont Unicode 14 and Terminus 4.49.1 experimental
  overlay ASIs while preserving ProggyClean Latin and all overlay semantics. No
  production font choice/output changes, game launch, or Git interaction.
- Evidence: exact downloaded TTFs passed a cmap audit for 74 required Cyrillic
  code points. Terminus is SIL OFL 1.1; the Gohu TTF is a contributor conversion
  under WTFPL, explicitly marked unsupported by GohuFont upstream.
- Changed: ignored experiment files/artifacts under
  `build-artifacts/experimental-font-ab/`; report at
  `research/reports/PROGGYCLEAN_CYRILLIC_RUNTIME_AB_BUILD.md`; this task log;
  plan archived as `research/completed/CYRILLIC_FONT_AB_TASK_PLAN.md`.
  Intentionally untouched: production `src/overlay/localization_font.cpp`,
  production build scripts and root production ASI.
- Git: branch, commit and working-tree state not inspected, per explicit user
  prohibition; no Git commands were run.
- Validation: `test.cmd` PASS (40/40 sources compiled/executed; all
  deterministic harnesses PASS); catalog audit PASS (166/166); both named ASIs
  built successfully. PE data-only inspection confirms English, Ukrainian and
  correct candidate-font resources are embedded in each ASI. Existing Zydis and
  ImGui vendor warnings only. No game launch/injection.
- Completed: `STALKER2CameraTweaksOverlayIntegrationGohu.asi` and
  `STALKER2CameraTweaksOverlayIntegrationTerminus.asi` are ready for separate
  user runtime comparison.
- Remaining: user screenshot/visual assessment of the two variants.
- Deferred: native BDF bitmap comparison, candidate size sweep, production font
  promotion. Blocked: none.
- Not runtime-validated: neither ASI was loaded into the game; readability and
  in-game rendering remain for the user to verify.
- Patch summary: added isolated test-only font-loading/build inputs and emitted
  two single-file overlay ASIs with matching ProggyClean, locale resources and
  rasterization policy.
- Changelog summary: provides GohuFont and Terminus Cyrillic A/B builds for
  direct in-game readability comparison without changing the production ASI.

## 2026-09-24 — Ukrainian catalog review and runtime language switch

- Scope: apply the user's approved Ukrainian editorial corrections and add an
  immediate English/Ukrainian overlay selector persisted as `en`/`uk`. Keep a
  single-file ASI deployment; no Auto detection, other locales, camera changes,
  game launch, or Git interaction.
- Changed: `locales/en.json`, `locales/uk.json`, `locales/README.md`, config
  parsing/repository/template code, runtime settings API and owner,
  localization keys/manager, overlay renderer, localization/config/runtime
  harnesses, and the catalog audit. Plan archived at
  `research/completed/UKRAINIAN_RUNTIME_LANGUAGE_SWITCH_TASK_PLAN.md`.
- Git: not inspected or touched, per explicit user instruction.
- Validation: catalog audit passed with 165/165 English and Ukrainian keys,
  placeholder parity, embedded-resource checks, and zero legacy/filesystem
  localization references. `test.cmd` passed all 40/40 deterministic
  harnesses, including language config fallback/persistence and runtime
  language switching. `build-overlay-settings.cmd` exited successfully and
  produced `STALKER2CameraTweaksOverlayIntegration.asi` (2,278,400 bytes).
  Existing Zydis/ImGui compiler warnings remain. No game launch or injection.
- Completed: both catalogs and the bundled font remain embedded in the ASI;
  language changes apply immediately and persist in `[Overlay] Language=en|uk`;
  missing/invalid config values default to English; `English` and `Українська`
  remain invariant native language names. User-requested copy edits are applied.
- Remaining: user runtime/screenshot verification of Ukrainian UI and layout.
- Deferred: Auto/game-language detection and all additional locales. Blocked:
  none.
- Not runtime-validated: no game session was launched; visual wrapping and
  runtime language persistence await the user's screenshot test.
- Patch summary: integrated the reviewed Ukrainian catalog with typed runtime
  locale selection and stable config identifiers, while preserving the single
  ASI install contract.
- Changelog summary: overlay language can now be switched live between English
  and Ukrainian and is remembered across launches; both catalogs are embedded
  in the ASI.

## 2026-09-24 — Adjustable overlay text size

- Scope: increase the bundled font's default size and add a live, persistent
  user control. No font-family/catalog changes, camera behavior, game launch,
  or Git interaction.
- Changed: `src/overlay/renderer_runtime.cpp`, config parsing/repository/template
  code, runtime settings API/owner, localization key and English/Ukrainian
  catalogs, config/runtime/localization harnesses, and `backlog/TASKLOG.md`.
  Plan archived at `research/completed/OVERLAY_FONT_SIZE_TASK_PLAN.md`.
- Git: not inspected or touched, per explicit user instruction.
- Validation: localization/catalog audit passed with 166/166 keys and matching
  placeholders. `test.cmd` passed all 40/40 deterministic harnesses, including
  font-size boundaries/default/persistence and Ukrainian glyph coverage at
  16 px. `build-overlay-settings.cmd` passed and produced a fresh
  `STALKER2CameraTweaksOverlayIntegration.asi`. Existing Zydis/ImGui vendor
  warnings remain. No game launch or injection.
- Completed: default bundled font increased from 13 px to 16 px; Overlay has a
  12–24 px live text-size slider persisted as `[Overlay] FontSize`; missing or
  invalid values use 16, and out-of-range runtime values are rejected.
- Remaining: user visual/runtime confirmation and selection persistence check.
- Deferred: font-family replacement and additional locales. Blocked: none.
- Not runtime-validated: no game session was launched; visual appearance and
  user preference remain to be checked in game.
- Patch summary: exposed bounded ImGui global font scaling through the existing
  typed runtime/config path while baking the embedded Unicode font at 16 px.
- Changelog summary: overlay text now defaults to a larger 16 px size and can be
  adjusted live from 12 to 24 px; the choice is remembered across launches.

## 2026-09-24 — Replace hard-to-read overlay font

- Scope: replace the embedded monospaced font with a familiar Windows system
  UI font, while preserving Ukrainian glyph support, adjustable text size, and
  single-ASI installation. No Git operations or game launch.
- Changed: `src/overlay/localization_font.hpp/.cpp`,
  `src/overlay/localization_resources.rc`,
  `src/overlay/localization_resource_ids.h`,
  `src/overlay/renderer_runtime.cpp`,
  `tests/overlay/localization_harness.cpp`,
  `tests/runner/localization_catalog_audit.ps1`, and this task log. Plan
  archived at `research/completed/OVERLAY_READABLE_FONT_TASK_PLAN.md`.
- Git: not inspected or touched, per explicit user instruction.
- Validation: catalog/source audit passed (166/166 keys, placeholders, embedded
  locale resources, system font loader). `test.cmd` passed all 40/40 harnesses,
  including font atlas construction and English/Ukrainian glyph coverage.
  `build-overlay-settings.cmd` passed and produced a fresh
  `STALKER2CameraTweaksOverlayIntegration.asi`. Existing Zydis/ImGui vendor
  warnings remain. No game launch or injection.
- Completed: Segoe UI is used from the standard Windows Fonts directory, with
  Arial fallback; the font itself is no longer embedded in the ASI. No font
  sidecar is needed. Font size remains 16 px by default and adjustable 12–24 px.
- Remaining: user's in-game readability and visual confirmation.
- Deferred: removal of the unused licensed Noto Sans Mono source asset; it was
  intentionally preserved. Blocked: none.
- Not runtime-validated: no game session was launched; in-game look awaits the
  user's screenshot test.
- Patch summary: switched UI rendering to proportional system fonts without
  changing catalog embedding or the user-facing one-file mod installation.
- Changelog summary: overlay now uses Windows Segoe UI, falling back to Arial;
  the built ASI no longer embeds the hard-to-read monospaced font.

The previous task log is preserved unchanged at
[TASKLOG_PREVIOUS.md](TASKLOG_PREVIOUS.md).

## 2026-09-22 — Task log rollover

- Scope: preserve the previous task history and start a fresh active log.
- Changed: moved the previous TASKLOG.md to TASKLOG_PREVIOUS.md without
  changing its contents; updated the backlog navigation.
- Validation: archived file SHA-256 and byte length match the pre-move file.
- Completed: future task entries have a fresh active destination; historical
links to TASKLOG.md continue to resolve to this current log.

- Remaining: implementation batches will be added after their scoped validation
  and final Git review.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: not applicable; this was filesystem/documentation
  organization only.
- Patch summary: archived the oversized previous task log and created a clean
  current log while retaining the full historical file.
- Changelog summary: task progress now continues in a fresh TASKLOG.md; the
  earlier history is available in TASKLOG_PREVIOUS.md.

## 2026-09-23 — English overlay localization foundation

- Scope: create an English-only catalog and renderer-independent lookup/formatting layer for all current overlay UI, feature-status copy, Camera State labels, and notifications. No additional locales, game-language Auto detection, font work, camera behavior, or runtime language switching.
- Changed: `build-overlay-settings.cmd`, `locales/en.json`, `src/overlay/localization.hpp`, `src/overlay/localization.cpp`, `src/overlay/renderer_runtime.cpp`, `src/overlay/feature_presentation.hpp`, `src/overlay/feature_presentation.cpp`, `src/overlay/discovery_runtime.cpp`, `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `test.cmd`, `tests/overlay/localization_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`; plan archived at `research/completed/I18N_ENGLISH_CATALOG_TASK_PLAN.md`.
- Git: not inspected or otherwise touched, per explicit user instruction. Branch, HEAD and pre-existing working-tree state are intentionally unreported.
- Validation: `test.cmd` passed (40/40 harnesses); localization catalog audit found 211 keys and zero missing source references; `build-overlay-settings.cmd` passed and produced `STALKER2CameraTweaksOverlayIntegration.asi`. Existing compiler warnings in Zydis/ImGui remain unrelated. No game launch, injection, visual review, locale-missing runtime test, or in-game validation.
- Completed: catalog loading is one-shot and module-relative at `locales/en.json`; renderer uses semantic keys, named placeholders, UTF-8 strings and visible missing-key fallback; invalid JSON, duplicate keys, malformed UTF-8 and invalid surrogate escapes are rejected. The current English overlay text and toast copy were migrated. Canonical mode/config identifiers remain unchanged.
- Remaining: verify the packaged layout in the user's install and visually confirm English UI/toasts in game. The current catalog resolver expects the `locales` directory beside the ASI.
- Deferred: Ukrainian and other game-supported catalogs, language selector/Auto resolver research, game-language detection, font/glyph coverage, runtime locale switching and pluralization.
- Blocked: none.
- Not runtime-validated: the new resource path and rendered copy have only source/build/harness evidence; no injected game session was launched.
- Patch summary: added strict UTF-8 JSON catalog loading, semantic lookup/formatting and deterministic checks; migrated overlay prose and notifications to a single English catalog.
- Changelog summary: overlay UI text is now catalog-backed through `locales/en.json`, with dynamic hotkey/settings messages formatted using named placeholders; no camera or settings behavior changed.

## 2026-09-23 — Overlay mouse notice and configurable single-key bindings

- Scope: add a visible mouse-capture notice and configurable overlay-toggle key; extend the shared keybinding model to validated ordinary single keyboard keys across five actions. No camera behavior or mouse input policy changes.
- Changed: config VK parsing/display/persistence/defaults and duplicate repair; runtime settings snapshot/mutation and WndProc capture ordering; overlay toggle row and dynamic mouse notice; focused config/input/API tests, test runner, and README/config-template descriptions. Plan: `research/completed/OVERLAY_TOGGLE_KEY_AND_MOUSE_NOTICE_TASK_PLAN.md`.
- Paths: `README.md`, `test.cmd`, `src/config/feature_config.hpp`, `src/config/feature_config.cpp`, `src/config/config_repository.cpp`, `src/config/config_template.cpp`, `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/input_state.hpp`, `src/overlay/input_state.cpp`, `src/overlay/discovery_runtime.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/config/config_persistence_harness.cpp`, `tests/config/runtime_settings_api_harness.cpp`, `tests/overlay/input_state_harness.cpp`.
- Git: branch `main`, HEAD `3120a05`; working tree already contained broad staged and unstaged changes from earlier overlay work. No unrelated paths were intentionally changed or reverted; no commit/reset/cleanup performed.
- Validation: `test.cmd` passed (39/39 harnesses, including key identity round-trip, duplicate repair, input capture and Escape recovery); `build-overlay-settings.cmd` passed; `git diff --check` on the complete pre-existing staged tree reports trailing whitespace in unrelated historical files. No game launch or runtime test.
- Completed: configurable default-Insert Overlay Toggle works independently of `Hotkeys.Enabled`; bindings are exclusive; ordinary single keys are represented as stable `VK_XX` values and displayed via Win32; notice reflects the current toggle key. Capture keyup is consumed before overlay toggle processing.
- Remaining: user-run visual/runtime confirmation in game; no code-side blocker identified.
- Deferred: modifier combinations and mouse-button bindings remain out of scope.
- Blocked: none.
- Not runtime-validated: no injected/game session was launched; build and harness success do not prove in-game cursor/input behavior.
- Patch summary: generalized the single-key binding path to five mutually exclusive actions and documented mouse capture with a dynamic overlay-toggle keycap.
- Changelog summary: the overlay toggle is configurable (default Insert), ordinary keyboard keys can be rebound without modifier combinations, and the overlay explains that it captures mouse input while visible.

## 2026-09-23 — Overlay key labels and Camera State layout repair

- Scope: correct navigation-key display labels, keep the mouse notice readable, and constrain Camera State fields to an explicit label/value layout. No Git commands were used for this follow-up.
- Changed: `src/config/feature_config.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/config/config_persistence_harness.cpp`; plan archived at `research/completed/OVERLAY_DISPLAY_AND_CAMERA_STATE_LAYOUT_TASK_PLAN.md`.
- Validation: `test.cmd` passed all 39 harnesses; `build-overlay-settings.cmd` passed. No game launch or runtime visual confirmation.
- Completed: Insert/Delete/Home display stable names rather than numpad scan-code names; notice is split into short rows; Camera State now uses bounded two-column tables per section with wrapped values.
- Remaining: user-run visual check of the rebuilt overlay, especially expanded Camera State on the narrow layout shown in the screenshots.
- Deferred: none. Blocked: none.
- Not runtime-validated: no injected/game session was launched; the layout repair is build- and harness-validated only.
- Patch summary: fixed navigation-key labels and replaced manual Camera State text positioning with bounded table rows.
- Changelog summary: Insert/Delete display correctly, the mouse-capture notice fits more reliably, and Camera State fields stay aligned and wrap within the right column.

## 2026-09-23 — Separate Overlay Toggle from optional Hotkeys

- Scope: clarify visually that the overlay visibility binding is independent of runtime hotkey enablement.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived at `research/completed/OVERLAY_TOGGLE_GROUPING_TASK_PLAN.md`.
- Completed: moved `Overlay Toggle` into its own `Overlay` section above `Hotkeys`; tooltip explains the toggle remains active when runtime hotkeys are disabled, and the checkbox tooltip names the affected action groups.
- Validation: `build-overlay-settings.cmd` passed. No game launch. No Git commands or Git-state operations were performed.
- Remaining: user visual confirmation in game. Deferred/blocked: none.
- Not runtime-validated: no injected/game session was launched.
- Patch summary: separated overlay visibility control from optional gameplay/cinematic/dialogue hotkeys without changing behavior.
- Changelog summary: users can now see at a glance that disabling runtime hotkeys does not disable the overlay toggle.

## 2026-09-23 — OEM punctuation hotkey display names

- Scope: fix OEM punctuation labels without changing input binding identity, capture, persistence, or action routing.
- Changed: `src/config/feature_config.cpp`, `tests/config/config_persistence_harness.cpp`; plan archived at `research/completed/OEM_HOTKEY_DISPLAY_TASK_PLAN.md`.
- Completed: key labels use the active Windows keyboard layout for printable OEM keys, with readable standard-symbol fallbacks. Added round-trip coverage for OEM punctuation VK identities.
- Validation: `test.cmd` passed all 39 harnesses; `build-overlay-settings.cmd` passed. No in-game runtime was launched.
- Remaining: user visual confirmation for punctuation and any available numpad keys. Deferred/blocked: none.
- Not runtime-validated: no game session was launched; Ctrl/Shift remain intentionally non-bindable.
- Patch summary: resolved OEM VK codes into readable keyboard-layout-aware symbols while preserving their canonical persisted identity.
- Changelog summary: `[ ] ; ' , . / \ - = and backtick` bindings now receive readable labels instead of `?`.

## 2026-09-22 — Camera Integration Gameplay FOV source audit

- Scope: identify existing authoritative sources for the three requested FOV values; read-only source review only.
- Inspected: `camera::CameraFovObservation`, `camera::GameplayBaseline`, `OverlaySemanticSnapshot`, and the HorPlus writer/diagnostic path. No source files changed.
- Git: branch `main` at `3120a05`; pre-existing modified and untracked work preserved.
- Validation: source/reference search and inspection only; no build or game launch.
- Completed: existing mechanism retains a native writer input and HorPlus result pair with validity/provenance.
- Remaining: identify the distinct in-game selected Gameplay FOV setting source before labeling a third value; current `configuredGameplayFovKnown` is false and diagnostics report it as unknown.
- Deferred: overlay/API exposure until all three source mappings are confirmed.
- Blocked: exact producer for selected in-game Gameplay FOV is not established in this checkout; awaiting user clarification of the intended existing field/mechanism.
- Not runtime-validated: no UI or runtime changes made.
- Patch summary: confirmed the existing live input/result pair and rejected treating it as three distinct values without evidence.
- Changelog summary: Camera Integration FOV panel remains pending identification of the authoritative selected-setting value.

## 2026-09-22 — Gameplay FOV candidate source/history audit

- Scope: read-only follow-up to identify the remembered v1.0 Gameplay FOV value; no camera, overlay, or UI changes.
- Changed: added `research/reports/OVERLAY_GAMEPLAY_FOV_SOURCE_AUDIT.md` with the source/history mapping and remaining evidence gate.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved.
- Validation: source/history search and review only; no build or game launch.
- Completed: identified the likely candidate as writer source `+0x234`, historically logged as `secondaryFOV` and currently observed by diagnostics as `cameraFirstPersonFov`; confirmed the coherent Before/After pair already exists in `CameraFovObservation`.
- Remaining: runtime-confirm that `+0x234` follows the selected Gameplay FOV and remains the intended baseline through ADS/zoom and camera lifecycle changes before exposing it as `Game FOV`.
- Deferred: read-only observation at the existing CameraWriter boundary, then projection through the semantic snapshot if the source contract passes.
- Blocked: the evidence does not establish a retained mod-owned global selected-FOV scalar or ADS invariance; no UI label change is justified yet.
- Not runtime-validated: no new runtime evidence collected.
- Patch summary: narrowed the missing third FOV to a historically correlated camera-source field without conflating it with the dynamic writer input.
- Changelog summary: documented the `secondaryFOV` candidate and a minimal same-observation read-only overlay projection, pending runtime confirmation.

## 2026-09-22 — Gameplay FOV source diagnostic ASI

- Scope: add isolated, read-only, change-only telemetry for candidate `source+0x234` at the existing CameraWriter callback; no production behavior, UI, hook resolution, or game launch.
- Changed: `src/plugin/runtime.cpp`, `build.cmd`, new `build-gameplay-fov-source-diagnostic.cmd`, archived task plan, and this entry. Build outputs are in the dedicated `build-artifacts/test-asi/fov-source-diagnostic/` and `build-artifacts/obj-gameplay-fov-source-diagnostic/` paths.
- Git: branch `main` at `3120a05`; pre-existing staged/modified/untracked work preserved; no staging or commit performed.
- Validation: isolated diagnostic build PASS; embedded marker check PASS; scoped `git diff --check` PASS with LF/CRLF normalization warnings. Existing `STALKER2CameraTweaksDiagnostic.asi` and shared object directory were not targeted.
- Completed: verified regular HorPlus flow reaches `ApplyHorPlusGameplay`; trace records event/writer sequence, source/thread, `+0x234` candidate/read-validity, `+0x230` comparison field, writer input, HorPlus result, aspect/flags, coordinator, Gameplay Enabled/Mode, and eligibility/application state. FOV changes are coalesced with a 1-degree event threshold; selected-candidate changes use 0.01-degree threshold.
- Remaining: user-run one combined 90 → ADS/binocular → 100 → ADS → 110 → cinematic/source-return session and provide `STALKER2CameraTweaks.log` for semantic assessment.
- Deferred: selected Gameplay FOV naming and overlay snapshot/UI projection until runtime invariants pass.
- Blocked: none for producing the diagnostic artifact.
- Not runtime-validated: no game launch; `+0x234` meaning and ADS/binocular persistence remain unknown.
- Patch summary: added compile-gated observation-only telemetry and an isolated ASI/object build path without adding a hook or changing production camera behavior.
- Changelog summary: new `STALKER2CameraTweaksGameplayFovSourceDiagnostic.asi` captures bounded FOV-source events for the upcoming user-controlled validation run.

## 2026-09-22 — Unify passive FOV diagnostics under the normal ASI

- Scope: prepare the passive Gameplay FOV source trace for INI-controlled use; no runtime launch.
- Non-goals: retain separate compile-time diagnostic profiles, change camera behavior, change overlay UI, or overwrite the existing diagnostic ASI.
- Changed: `src/plugin/runtime.cpp`, `build.cmd`, `build-gameplay-fov-source-diagnostic.cmd`, and the archived task plan `research/completed/GAMEPLAY_FOV_DIAGNOSTICS_UNIFICATION_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged/modified/untracked work preserved; no staging or commit performed.
- Validation: normal-profile ASI build PASS; `test.cmd` PASS with 39/39 sources compiled and executed; artifact exists at `build-artifacts/test-asi/fov-source-diagnostic/STALKER2CameraTweaks.asi`, size 1,192,448 bytes, SHA-256 `0AB084262CFD6DA8AF8CF2BC1F66CED6480DD2D7520D3ACC7D9D9E67959BC76C`; `git diff --check` PASS with existing LF/CRLF normalization warnings.
- Completed: passive FOV trace is compiled into the normal ASI; `[Diagnostics] Enabled=false` returns before candidate reads and trace work; the wrapper now builds the normal ASI to an isolated artifact. Other compile-time diagnostic features remain unchanged.
- Remaining: user runtime test with `[Diagnostics] Enabled=true`, followed by the planned 90 → ADS/binocular → 100 → ADS → 110 sequence.
- Deferred: naming `source+0x234` as selected Gameplay FOV and exposing it in the overlay until runtime evidence confirms its semantics.
- Blocked: none.
- Not runtime-validated: the unified normal ASI has not been injected into the game in this batch.
- Patch summary: moved the passive FOV source experiment from a separate compile-gated ASI into the existing runtime diagnostics gate.
- Changelog summary: one normal ASI can now provide the FOV source trace when `[Diagnostics] Enabled=true`; no special diagnostic ASI is required for this trace.

## 2026-09-22 — Promote CameraStateSnapshot to shared production runtime

- Scope: convert the existing central camera snapshot from diagnostic-only composition into the shared production runtime model; add retained Gameplay FOV and coherent live Native/HorPlus FOV projection; promote minimal zoom evidence; no camera-behavior changes and no game launch.
- Non-goals: no changes to GameplayBaselineStore, HorPlus math, gameplay/cinematic/dialogue/recovery behavior, hooks, or native reverse-transition semantics.
- Changed: `src/camera/camera_state_snapshot.hpp/.cpp`, `src/plugin/runtime.cpp`, `src/plugin/runtime_settings.hpp`, `src/overlay/camera_integration.hpp/.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/camera/camera_state_snapshot_harness.cpp`; task plan created as `CAMERA_STATE_PRODUCTION_RUNTIME_TASK_PLAN.md` and archived after completion.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` PASS with existing LF/CRLF normalization warnings; no game launch.
- Completed: production snapshot is maintained independently of `[Diagnostics] Enabled`; diagnostic logging remains gated and consumes the shared state; retained Gameplay FOV updates only after bounded neutral Gameplay captures and is held through zoom, dialogue, cinematic, recovery, and transient transitions; Native FOV and HorPlus FOV come from one CameraWriter observation; diagnostic-only duplicate snapshot composition was removed; overlay now presents the three FOV values with unavailable handling.
- Remaining: one combined runtime validation for FOV trio, zoom/transition retention, overlay projection, and Diagnostics ON/OFF equivalence.
- Deferred: runtime validation only; no further source or UI scope is opened until that run.
- Blocked: none.
- Not runtime-validated: in-game injection and lifecycle behavior remain intentionally untested in this batch.
- Patch summary: moved camera-state ownership into the always-on central snapshot, retained neutral Gameplay FOV without `GameplayBaseline.nativeFov`, `source+0x234`, or a parallel store, and kept diagnostic output as an optional consumer.
- Changelog summary: overlay Camera Integration now reports `Gameplay FOV`, `Native FOV`, and `HorPlus FOV` from shared runtime camera evidence; diagnostics no longer control whether that state exists.

## 2026-09-22 — Add full Camera State overlay details

- Scope: add a separate bottom read-only `Camera State` presentation section using the shared production snapshot; no camera behavior or state ownership changes.
- Changed: `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/overlay/semantic_snapshot_harness.cpp`; task plan archived as `research/completed/CAMERA_STATE_OVERLAY_DETAILS_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: the overlay receives a read-only copy of the shared `CameraStateSnapshot` and displays presentation, gameplay mode, zoom evidence, dialogue state, FOV/aspect evidence, provenance, source tokens, and generation values; invalid state is shown as unavailable.
- Remaining: visual runtime review of the expanded panel and any layout tuning.
- Deferred: no new camera runtime behavior or diagnostic changes.
- Blocked: none.
- Not runtime-validated: the new expanded UI has not been injected into the game.
- Patch summary: exposed the existing central snapshot through the semantic read path and added a separate full-state UI panel without introducing a parallel store.
- Changelog summary: overlay now includes a detailed `Camera State` section below Camera Integration for inspection of the complete shared runtime snapshot.

## 2026-09-22 — Overlay semantic deduplication

- Scope: remove proven FOV duplication from `OverlaySemanticSnapshot`, keep distinct settings/capability/pending owners, and collapse technical Camera State details by default.
- Changed: `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/camera_integration.cpp`, `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_SEMANTIC_DEDUPLICATION_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` PASS; no game launch.
- Completed: Camera Integration now reads Gameplay/Native/HorPlus FOV directly from `CameraStateSnapshot`; the three duplicated semantic facts and their extra aggregation path were removed. Technical Camera State remains available but is collapsed by default.
- Intentionally retained: coordinator, dialogue phase, hook/capability availability, pending transitions, configured-next policies, and baseline usability because their freshness or ownership semantics are not equivalent to the current snapshot.
- Remaining: optional future deduplication after a separate freshness audit; runtime visual review of the collapsed details section.
- Deferred: no camera behavior, hooks, or RuntimeSettings changes.
- Blocked: none.
- Not runtime-validated: the post-cleanup UI has not been injected into the game.
- Patch summary: reduced overlay aggregation to a projection of authoritative sources and removed only the FOV fields proven identical to central camera evidence.
- Changelog summary: Camera Integration no longer maintains duplicate FOV facts; detailed Camera State is now an opt-in inspection panel.

## 2026-09-22 — Add top-center overlay notifications

- Scope: add generic non-interactive ImGui toast notifications at the top center and connect existing F9–F12 hotkey mutations; no hotkey configuration UI and no game launch.
- Changed: `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/renderer_runtime.hpp`, `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_NOTIFICATION_SYSTEM_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: bounded notification queue with id, title, value, semantic status, timestamp, max-three visible stack, expiry, fade, top-center placement, and `NoInputs` rendering. F9–F12 publish after the existing mutation boundary: Gameplay Mode reports Pending/Applied/Error; next-cinematic/dialogue settings report Ready/Error.
- Remaining: one combined runtime review of F9–F12 while the main overlay is hidden, including toast stacking and expiry.
- Deferred: hotkey rebinding UI, menu-control toasts, Windows notifications, and runtime error notifications.
- Blocked: none.
- Not runtime-validated: in-game hotkey/toast behavior remains intentionally deferred.
- Patch summary: introduced optional UX notifications without making them a settings owner or changing existing mutation semantics.
- Changelog summary: F9–F12 now have visible top-center feedback even when the main overlay window is closed.

## 2026-09-23 — Improve overlay parameter label readability

- Scope: presentation-only styling for parameter labels; no settings, camera, hotkey, or toast behavior changes.
- Changed: `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_PARAMETER_LABEL_STYLE_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: controls, Runtime labels, and Camera Integration labels now use a consistent accent color while values, statuses, tooltips, and toast presentation remain unchanged.
- Remaining: optional visual runtime confirmation of label contrast.
- Deferred: no new UI layout or typography system.
- Blocked: none.
- Not runtime-validated: the updated label styling has not been injected into the game.
- Patch summary: separated parameter names visually from their values using a reusable renderer helper.
- Changelog summary: overlay settings and integration values are easier to scan without changing their semantics.

## 2026-09-23 — Highlight tooltip parameter names

- Scope: presentation-only tooltip cleanup; highlight option names while keeping descriptions and wrapping unchanged.
- Changed: `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_TOOLTIP_PARAMETER_LABEL_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: the standardized tooltip renderer now draws each parameter/option name with the overlay accent color and the description with normal text styling.
- Remaining: optional visual confirmation across all tooltip catalogs.
- Deferred: no tooltip wording or catalog changes.
- Blocked: none.
- Not runtime-validated: updated tooltip styling has not been injected into the game.
- Patch summary: separated tooltip parameter names from explanatory text without changing tooltip semantics.
- Changelog summary: tooltip entries are easier to scan because names such as `HorPlus`, `Auto`, and `Adaptive` are visually distinct from their descriptions.

## 2026-09-23 — Improve tooltip hover areas and neutral label styling

- Scope: presentation-only correction from runtime UI review; neutralize parameter label colors and make each setting tooltip respond to the complete control row.
- Changed: `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_TOOLTIP_HOVER_AREA_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: parameter labels and tooltip option names use neutral text styling; each tooltip handler is now attached to a grouped checkbox/combo row covering control, value, arrow, and label.
- Remaining: optional visual runtime confirmation that hovering the value field and arrow opens the same tooltip.
- Deferred: no tooltip content, settings semantics, or input behavior changes.
- Blocked: none.
- Not runtime-validated: updated hover areas have not been injected into the game.
- Patch summary: removed the overly prominent cyan label treatment and expanded tooltip hit regions without duplicating handlers.
- Changelog summary: tooltips are now easier to discover and parameter labels no longer compete visually with blue controls.

## 2026-09-23 — Standardize tooltip two-column text layout

- Scope: presentation-only tooltip formatting; align descriptions after the longest option name and remove recommendation/default prefaces.
- Changed: `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_TOOLTIP_TWO_COLUMN_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` pending final review; no game launch.
- Completed: shared tooltip renderer measures the longest option name per catalog and aligns all descriptions in a consistent second column; `Recommended` and `Default with GameplayHorPlus` wording was removed without changing behavior descriptions.
- Remaining: optional visual runtime confirmation across the long Gameplay and Cinematic FOV tooltips.
- Deferred: no tooltip catalog expansion or settings semantics changes.
- Blocked: none.
- Not runtime-validated: updated two-column tooltip layout has not been injected into the game.
- Patch summary: converted tooltip entries from free-flowing inline text into a readable name/description catalog layout.
- Changelog summary: long tooltips now retain a stable left parameter column and cleaner line wrapping.

## 2026-09-23 — Split overlay settings and runtime into two columns

- Scope: presentation-only layout change; left column contains Gameplay/Cinematics/Dialogue controls, right column contains Runtime, Camera Integration, and expandable Camera State. No Hotkeys editor or game launch.
- Changed: `src/overlay/renderer_runtime.cpp`; task plan archived as `research/completed/OVERLAY_TWO_COLUMN_SETTINGS_RUNTIME_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, modified, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS with 39/39 sources compiled and executed; production build PASS with existing Zydis warnings; overlay build PASS with existing Zydis/ImGui warnings; `git diff --check` PASS with existing line-ending warnings; no game launch.
- Completed: existing UI blocks now use a 45/55 ImGui table split with a subtle vertical divider. Settings mutation paths, semantic reads, tooltip handlers, toast behavior, and camera-related presentation remain unchanged.
- Remaining: one user-authorized runtime visual review of the new two-column window and expanded Camera State.
- Deferred: Hotkeys configuration/rebinding UI and any Camera Runtime expansion.
- Blocked: none.
- Not runtime-validated: in-game layout and responsive sizing remain intentionally deferred.
- Patch summary: separated user controls from runtime observation without introducing another state source or changing camera behavior.
- Changelog summary: overlay now presents configuration on the left and actual runtime/integration state on the right for faster scanning.

## 2026-09-23 — Add live hotkey rebinding to the overlay

- Scope: add Hotkeys controls after Dialogue in the left configuration column; retain Runtime, Camera Integration, and Camera State on the right. Worker lives for the plugin lifetime; `Hotkeys.Enabled` gates actions only. No game launch.
- Changed: `src/config/feature_config.cpp`, `src/config/feature_config.hpp`, `src/plugin/runtime.cpp`, `src/plugin/runtime.hpp`, `src/plugin/runtime_settings.hpp`, `src/plugin/worker_lifecycle.hpp`, `src/overlay/input_state.cpp`, `src/overlay/input_state.hpp`, `src/overlay/discovery_runtime.cpp`, `src/overlay/renderer_runtime.cpp`, `src/overlay/renderer_runtime.hpp`, `tests/config/config_persistence_harness.cpp`, `tests/config/runtime_settings_api_harness.cpp`, `tests/lifecycle/worker_lifecycle_harness.cpp`, `tests/overlay/input_state_harness.cpp`; plan archived as `research/completed/HOTKEY_REBINDING_UI_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; existing staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 sources compiled/executed and all harnesses PASS); production build PASS; overlay build PASS; `git diff --check` PASS with existing LF/CRLF warnings. Builds retain existing Zydis warnings and overlay ImGui unused-local warning. No game launch.
- Completed: always-alive hotkey worker reads a synchronized runtime-settings snapshot; Enabled gates actions while rebinding stays available. Supported keys are F1-F12, 0-9, A-Z; Escape cancels, Insert stays reserved, modifier combinations and duplicates are rejected, the captured key press/release is consumed, and binding changes are applied and persisted immediately. Capture cancels on overlay close, focus loss, and teardown. Existing F9-F12 mutation/toast paths are preserved. Four-worker capacity covers the hotkey worker plus three diagnostic workers.
- Remaining: one combined runtime review of all four rebinding flows, conflict/cancel feedback, closed-overlay actions, live enable/disable, and persistence after restart. Check the intended behavior under Native camera policies when workers/hooks initialize independently of the initial hotkey setting.
- Deferred: modifier bindings, mouse/numpad keys, per-action None, automatic key swapping, and further hotkey UI settings.
- Blocked: none.
- Not runtime-validated: in-game rebinding, toast presentation, and camera pass-through/capability behavior were not tested in this batch.
- Patch summary: added rebinding and persistence to the existing authoritative runtime configuration path with one plugin-lifetime worker and isolated overlay capture state.
- Changelog summary: users can rebind the four existing actions in the overlay, receive success/error feedback, and keep rebinding available even while hotkey actions are disabled.

## 2026-09-23 — Align tooltip separator column

- Scope: adjust only the shared tooltip option-row presentation; preserve catalog text, tooltip behavior, and all settings/camera semantics.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/TOOLTIP_SEPARATOR_ALIGNMENT_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; all pre-existing staged/unstaged/untracked changes preserved; no staging or commit performed.
- Validation: `build-overlay-settings.cmd` PASS with existing Zydis and ImGui warnings; `git diff --check` PASS with existing line-ending warnings. No game launch.
- Completed: tooltip option names, the hyphen separator, and descriptions now occupy aligned columns; wrapped description text no longer carries the separator to the next line.
- Remaining: optional runtime visual confirmation on the longest Gameplay/Cinematic tooltip.
- Deferred: no tooltip wording, catalogs, or other UI changes.
- Blocked: none.
- Not runtime-validated: visual rendering in-game was not tested.
- Patch summary: split the separator from the wrapped description item in the shared tooltip renderer.
- Changelog summary: long tooltip descriptions now wrap cleanly beneath their own column without a stray leading hyphen.

## 2026-09-23 — Repair hotkey rebind cancellation

- Scope: fix Escape/cancel handling and retain the last accepted key for the active rebinding session; no camera or hotkey action changes.
- Changed: `src/overlay/input_state.hpp`, `src/overlay/input_state.cpp`, `src/overlay/discovery_runtime.cpp`, `tests/overlay/input_state_harness.cpp`; plan archived as `research/completed/HOTKEY_REBIND_CANCEL_RECOVERY_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; existing staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 sources compiled/executed; all harnesses PASS); `build-overlay-settings.cmd` PASS with existing Zydis and ImGui warnings; `git diff --check` PASS with existing line-ending warnings. No game launch.
- Completed: fixed the recursive mutex-lock path on Escape; rebind state now updates its remembered key only after accepted runtime mutation and restores that accepted value on Escape, Insert-close, focus loss, and teardown. Added regression coverage for rejected candidate → Esc and later cancellation after a successfully accepted rebind.
- Remaining: user should confirm the exact conflict → Esc flow in-game; supplied discovery log has no key-level or crash diagnostics.
- Deferred: no persistent per-action cache separate from authoritative runtime configuration; camera/runtime behavior unchanged.
- Blocked: none.
- Not runtime-validated: the reported game freeze/crash was not reproduced; the source deadlock path is deterministic and covered by harness.
- Patch summary: cancellation no longer recursively locks `rebindMutex_`; only accepted key mutations advance the remembered binding.
- Changelog summary: Escape after a hotkey conflict safely cancels rebinding and leaves the last accepted key selected.

## 2026-09-23 — Promote Camera Transition status

- Scope: present the existing overall aspect/FOV compatibility verdict as a top-level user-facing summary and remove its duplicate from technical Camera Integration. No projector or camera runtime behavior changes.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/CAMERA_TRANSITION_OVERLAY_STATUS_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; prior staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 sources compiled/executed; all harnesses PASS); `build-overlay-settings.cmd` PASS with existing Zydis and ImGui warnings; `git diff --check` PASS with existing line-ending warnings. No game launch.
- Completed: new top-level Camera Transition status distinguishes Aligned, Not aligned, and Cannot assess, includes concise details and a dedicated help tooltip; Camera Integration now contains only aspect/FOV component facts. User runtime-confirmed Aspect is Matched for Auto and forced 32:9 on a 32:9 viewport. Existing aspect projector behavior was verified by source and deterministic tests and left unchanged.
- Remaining: none for this batch.
- Deferred: projector matrix expansion or repair unless Auto runtime evidence contradicts the existing deterministic Auto/forced/Native/invalid cases.
- Blocked: none.
- Not runtime-validated: other viewport/policy combinations and broader camera-transition presentation remain outside the user's reported runtime check.
- Patch summary: surfaced the projector's existing overall verdict without duplicating or changing its semantics.
- Changelog summary: the overlay now explains camera-path alignment at the top, while Camera Integration remains a concise technical breakdown.

## 2026-09-23 — Runtime and Camera Integration tooltips

- Scope: replace boolean literals in checkbox tooltips and add full-row tooltips for Runtime and Camera Integration facts; no camera, settings, or projector behavior changes.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/RUNTIME_INTEGRATION_TOOLTIP_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged, unstaged, and untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 compiled and executed, all harnesses PASS); `build-overlay-settings.cmd` PASS with existing Zydis/ImGui compiler warnings; `git diff --check` PASS (line-ending notices only). No game launch.
- Completed: checkbox tooltip choices now read Enabled/Disabled. Runtime viewport and all Camera Integration rows have standardized explanations, and hovering anywhere across each label/value row invokes its tooltip. Cinematic comparison remains visible as unavailable when the necessary data is invalid; FOV explanations distinguish retained Gameplay baseline from live Native/HorPlus values.
- Remaining: user visual confirmation of tooltip appearance and whole-row hover target.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: in-game tooltip positioning, wrapping, and hitbox were not visually checked.
- Patch summary: standardized help text for runtime facts without changing their semantic sources.
- Changelog summary: checkbox tooltips use user-facing state names, and every runtime/camera integration value now explains its meaning on row hover.

## 2026-09-23 — Simplify Camera Transition summary

- Scope: replace one user-facing summary sentence only; no assessment or tooltip changes.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/CAMERA_TRANSITION_SUMMARY_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged and unstaged work preserved; no staging or commit performed.
- Validation: exact string diff reviewed; `git diff --check` PASS with line-ending notices. No build or game launch; change is a single display string.
- Completed: the Aligned detail now reads “Cinematic and Gameplay camera paths are aligned.” Assessment/projector logic and tooltip explanation are untouched.
- Remaining: none for this wording change.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: no in-game visual validation was needed for this text-only change.
- Patch summary: shortened the Camera Transition aligned-state detail.
- Changelog summary: clearer, user-facing Camera Transition summary wording.

## 2026-09-23 — Stabilize Camera State disclosure layout

- Scope: prevent expanded technical Camera State content from changing the two-column proportions and make the disclosure visibly interactive; no camera-state behavior changes.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/CAMERA_STATE_LAYOUT_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 compiled/executed, all harnesses PASS); `build-overlay-settings.cmd` PASS with existing Zydis/ImGui compiler warnings; `git diff --check` PASS with line-ending notices. No game launch.
- Completed: table columns now use explicit per-frame widths based on available content width (45/55), so long expanded state rows cannot auto-resize their proportions. Camera State uses a full-width framed tree row to communicate clickability.
- Remaining: user visual confirmation that proportions restore after collapse and the framed row has the desired emphasis.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: visual expand/collapse behavior was not checked in-game.
- Patch summary: decoupled column sizing from expanded Camera State content and styled its disclosure row.
- Changelog summary: expanding Camera State should no longer permanently widen or rebalance the settings/runtime columns; the disclosure is visibly interactive.

## 2026-09-23 — Bound overlay width to viewport

- Scope: address user-reported runtime growth of the overlay by making its width independent of content-driven table sizing; preserve auto-height, column ratio, and Camera State presentation.
- Evidence: runtime screenshots showed the previous table sizing changes caused the overlay to grow across the 32:9 viewport. Source inspection confirmed the main window uses `AlwaysAutoResize`, while table widths came from available content width. Vendored ImGui source confirms window size constraints are applied after auto-fit sizing and clamp the resulting width.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/OVERLAY_AUTHORITATIVE_WIDTH_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; existing staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 compiled/executed, all harnesses PASS); final `build-overlay-settings.cmd` PASS with existing Zydis/ImGui warnings; `git diff --check` PASS with line-ending notices. Initial build attempt exposed Windows `min`/`max` macro collisions; replaced those calls with equivalent comparisons and rebuilt successfully. No game launch.
- Completed: applied a width-only min/max constraint before main-window `Begin`, deriving a bounded preferred width from the ImGui viewport with horizontal margins; vertical sizing remains auto-resized. The 45/55 table now fits within this authoritative parent width, so Camera State content cannot feed back into the window's width.
- Remaining: user should repeat the runtime expand/collapse observation; no runtime PASS is claimed.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: actual 32:9 overlay size and Camera State expand/collapse behavior after this correction.
- Patch summary: bounded the auto-resizing overlay width using viewport dimensions, breaking the content-to-window-width feedback loop.
- Changelog summary: overlay remains compact on ultrawide displays while retaining responsive width and automatic height.

## 2026-09-23 — Structure Camera State inspector content

- Scope: keep all Camera State details readable inside the existing bounded runtime column; no snapshot or camera behavior changes.
- Evidence: user runtime screenshot confirms the parent width now remains bounded, but prior dense single-line state fields are clipped by the right column. `DrawCameraState` consumed typed `CameraStateSnapshot` fields directly but rendered combined diagnostic-style lines.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/CAMERA_STATE_CONTENT_LAYOUT_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; pre-existing staged/unstaged/untracked changes preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 compiled/executed; all harnesses PASS); final `build-overlay-settings.cmd` PASS with existing Zydis/ImGui warnings; `git diff --check` PASS with line-ending notices. No game launch.
- Completed: Camera State is now grouped under Presentation, Gameplay, Zoom, Dialogue, FOV, Sources, and Generation. Every snapshot datum is rendered as a muted label/value row; mode displays `HorPlus`/`AspectRecalculation`; booleans display Yes/No; invalid or non-finite numeric values display `unavailable`; long values wrap within the value column. The outer width and 45/55 split are untouched.
- Remaining: user visual confirmation that the inspector remains readable in the fixed right-hand column.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: no in-game visual confirmation after restructuring.
- Patch summary: transformed dense camera-state lines into a typed-field inspector with bounded wrapping and validity-aware values.
- Changelog summary: Camera State is easier to scan and no longer exposes invalid floats as `nan` or numeric gameplay-mode codes.

## 2026-09-23 — Disable main overlay collapse

- Scope: prevent the main settings window from collapsing to title-only while preserving Insert visibility behavior and the nested Camera State disclosure.
- Changed: `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/OVERLAY_NO_COLLAPSE_TASK_PLAN.md`.
- Git: branch `main` at `3120a05`; existing staged/unstaged/untracked work preserved; no staging or commit performed.
- Validation: `test.cmd` PASS (39/39 compiled/executed, all harnesses PASS); `build-overlay-settings.cmd` PASS with existing Zydis/ImGui warnings; `git diff --check` PASS with line-ending notices. No game launch.
- Completed: added ImGui's standard `NoCollapse` flag to the main settings window only. Title bar/name, Insert whole-overlay visibility path, input handling, and Camera State expand/collapse remain unchanged.
- Remaining: user visual confirmation that the title-bar collapse affordance is gone.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: no game launch or in-game visual check.
- Patch summary: disabled collapsing the visible settings window without changing overlay visibility/input semantics.
- Changelog summary: the main overlay can no longer appear hidden while still capturing mouse input; use Insert to hide/show it.

## 2026-09-23 — Make hotkey labels English and layout-independent

- Scope: replace active-layout/system-localized hotkey display names with deterministic English labels; preserve binding identity, capture, support policy, persistence, and routing.
- Evidence: `HotkeyName` previously queried the active keyboard layout for OEM symbols and used the OS key-name API as a fallback. Bindings already persist by VK code (`VK_XX`).
- Changed: `src/config/feature_config.cpp`, `tests/config/config_persistence_harness.cpp`; plan archived as `research/completed/ENGLISH_HOTKEY_LABELS_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (39/39 sources compiled and executed; all harnesses PASS); `build-overlay-settings.cmd` completed and produced the updated `STALKER2CameraTweaksOverlayIntegration.asi` (1,608,192 bytes, 2026-09-23 17:51:36); static search confirms the key-name implementation no longer uses active-layout or OS-localized lookup APIs. Existing Zydis/ImGui compiler warnings only. No game launch.
- Completed: supported key families now display fixed English labels, including punctuation, navigation keys, function keys, numpad, browser, media, and launch keys. OEM tests assert exact English symbols and persistence/reload identity; A-Z, digits, F24, and numpad naming have deterministic assertions.
- Remaining: user runtime confirmation under a Ukrainian keyboard layout.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: no in-game or live layout-switch test was performed.
- Patch summary: decoupled user-facing key labels from the active Windows keyboard layout while preserving VK binding identity.
- Changelog summary: hotkeys now retain English menu labels regardless of the selected keyboard language; punctuation no longer degrades to `?`.

## 2026-09-23 — Make the overlay window movable

- Scope: allow title-bar dragging, persist the main window position beside the ASI, and clamp it to the active viewport. Localization was explicitly excluded as a separate task. No camera/settings/input changes.
- Evidence: the main window was forced to `(30, 30)` every frame and had `NoSavedSettings`; this overrode dragging and prevented restoration.
- Changed: `src/overlay/renderer_runtime.hpp`, `src/overlay/renderer_runtime.cpp`; plan archived as `research/completed/OVERLAY_WINDOW_DRAG_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (39/39 compiled/executed, all harnesses PASS); final `build-overlay-settings.cmd` completed and produced `STALKER2CameraTweaksOverlayIntegration.asi` (1,609,728 bytes, 2026-09-23 18:15:05), with existing Zydis/ImGui warnings. Direct source review confirms the main window no longer receives a per-frame forced position and is clamped before content drawing. No game launch.
- Completed: title-bar movement is no longer overwritten each frame; the position loads/saves as Unicode-safe pixel coordinates in `STALKER2CameraTweaksOverlay.ini` beside the ASI; positions are clamped to the game viewport after resolution/size changes. If module-path resolution fails, movement still works for the session and persistence is skipped.
- Remaining: user runtime confirmation of dragging, restart persistence, and recovery after changing resolution.
- Deferred: localization, per user instruction.
- Blocked: none.
- Not runtime-validated: drag/clamp/reload behavior was not exercised in-game.
- Patch summary: removed the fixed-position feedback and added viewport-aware saved placement for the main overlay only.
- Changelog summary: drag the overlay by its title bar; its position is restored next run and brought back into view when the viewport changes.

## 2026-09-23 — Complete canonical English localization migration

- Scope: finish the English-only localization architecture/catalog migration and production renderer/notification call sites. No language selector, additional locales, auto-detection, camera/runtime semantics, game launch, or Git interaction.
- Evidence: final audit reports 162 catalog keys matching 162 typed keys; no legacy localization consumers, raw renderer key strings, obsolete typed keys, or direct hardcoded renderer UI strings. Four user-facing literals remain only in the non-production developer fallback path.
- Changed: `locales/en.json`; `src/overlay/localization_catalog.hpp/.cpp`, `localization_formatter.hpp/.cpp`, `localization_validator.hpp/.cpp`, `localization_manager.hpp/.cpp`, `localization_keys.hpp/.cpp`, `feature_presentation.hpp/.cpp`, `renderer_runtime.hpp/.cpp`; `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, `src/overlay/discovery_runtime.cpp`; `tests/overlay/localization_harness.cpp`, `tests/overlay/feature_presentation_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`; `test.cmd`, `build-overlay-settings.cmd`. Plan archived as `research/completed/I18N_CATALOG_SCHEMA_REFACTOR_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (40/40 runner sources compiled and executed; all deterministic harnesses PASS); catalog audit PASS (`162/162`, legacy=0, raw_keys=0, obsolete=0, direct_ui_literals=0); `build-overlay-settings.cmd` PASS. Existing vendored Zydis/ImGui compiler warnings only. No game launch.
- Completed: production renderer owns `LocalizationManager`; renderer and feature presentation use typed keys; notifications carry semantic kinds/actions and are composed/localized in the UI; the legacy localization layer is removed; English catalog is nested, canonical, and exactly aligned with typed-key inventory.
- Remaining: none within approved English-only migration scope.
- Deferred: additional languages, runtime language selection/detection, font/glyph work, and runtime visual validation remain separate future work.
- Blocked: none.
- Not runtime-validated: no in-game rendering/interaction test was performed.
- Patch summary: completed renderer and semantic-notification migration to the typed, validated, renderer-owned localization system and removed the production legacy path.
- Changelog summary: the overlay now uses one canonical nested English catalog with typed keys, validated placeholders, semantic notifications, and English fallback.

## 2026-09-23 — Embed localization catalog in overlay ASI

- Scope: keep `locales/*.json` as source/build-time catalogs and distribute a single ASI. No additional languages, selector, Auto detection, font work, camera/runtime behavior changes, game launch, or Git interaction.
- Evidence: runtime screenshot showed `[missing: ...]` for every key. `Renderer::BuildImGui` loaded `locales/en.json` from beside the ASI; build emitted only the ASI and did not deploy that external file.
- Changed: `src/overlay/localization_catalog.hpp/.cpp`, `localization_manager.hpp/.cpp`, `localization_resource_ids.h`, `localization_resources.rc`, `renderer_runtime.cpp`; `tests/overlay/localization_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`; `test.cmd`, `build-overlay-settings.cmd`; output `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/LOCALIZATION_EMBEDDING_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: catalog audit PASS (`162/162`, filesystem_locale_refs=0, embedded English resource PASS); `test.cmd` PASS (40/40 runner sources and all deterministic harnesses; localization harness initializes from the actual compiled RCDATA resource); `build-overlay-settings.cmd` PASS and linked the same resource into the ASI. Existing Zydis/ImGui warnings only. No game launch.
- Completed: runtime locale loading now uses `FindResourceW`/`LoadResource` from the overlay ASI module; filesystem catalog loading was removed. The canonical JSON is embedded as RCDATA; no locale folder/file is required beside the ASI.
- Remaining: user runtime smoke test with only the refreshed ASI.
- Deferred: additional catalogs/locales, language selection/detection, and translated font coverage.
- Blocked: none.
- Not runtime-validated: the game was not launched; user will test the one-file artifact.
- Patch summary: moved English catalog delivery from a sidecar JSON dependency into the ASI's RCDATA resources and added deterministic resource-loading coverage.
- Changelog summary: overlay installation again requires only one ASI; localization source JSON remains build-time-only.

## 2026-09-23 — Wrap localized Runtime input notice

- Scope: make the mouse-capture notice wrap to available overlay width while retaining the highlighted hotkey when it fits on one line. No localization wording/schema, input behavior, sizing, or camera/runtime changes.
- Evidence: user runtime screenshot showed the localized notice clipped at the window's right edge; renderer drew split text pieces without a wrap boundary.
- Changed: `src/overlay/renderer_runtime.cpp`; refreshed `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/RUNTIME_NOTICE_WRAP_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (40/40 runner sources and all harnesses); catalog audit PASS (`162/162`); `build-overlay-settings.cmd` PASS with existing Zydis/ImGui warnings. No game launch.
- Completed: notice width is measured against current content width; if it fits, existing inline colored keycap presentation is preserved; otherwise the full notice, including the bracketed key label, uses ImGui word wrapping.
- Remaining: user runtime confirmation with the refreshed single-file ASI.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: no in-game visual test was performed.
- Patch summary: added adaptive single-line/wrapped rendering for the localized mouse-capture notice.
- Changelog summary: long Runtime settings notices and key labels now wrap within the overlay instead of clipping off-screen.

## 2026-09-23 — Prepare Ukrainian catalog and embedded font

- Scope: prepare the complete Ukrainian source catalog and bundled Cyrillic-capable UI font; stop for the user's translation review before locale/config/renderer integration. No Auto detection, additional locales, runtime behavior changes, final ASI build, game launch, or Git interaction.
- Evidence: English is canonical with 162 typed keys; the current RCDATA resources contained only English; the ImGui default range and project assets did not provide the required Ukrainian glyph coverage. The official Noto Sans Mono upstream release supplies a static regular font under SIL OFL 1.1.
- Changed: `locales/uk.json`, `locales/README.md`; `assets/fonts/NotoSansMono/NotoSansMono-Regular.ttf`, `OFL.txt`, `README.md`; `src/overlay/localization_resource_ids.h`, `localization_resources.rc`, new `localization_font.hpp/.cpp`, `renderer_runtime.cpp`; `tests/overlay/localization_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`; `test.cmd`, `build-overlay-settings.cmd`. Plan archived as `research/completed/I18N_UK_CATALOG_AND_FONT_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: test-runner source audit PASS (40/40 source/compile/run sets match); catalog audit PASS (162 English keys, 162 Ukrainian keys, exact key and placeholder parity; embedded resources and OFL license present); focused localization/font harness PASS (actual embedded font built into ImGui atlas and every Unicode scalar from both catalogs found). Full `test.cmd`, production ASI build, and game launch were not performed.
- Completed: drafted all 162 Ukrainian strings; selected and bundled Noto Sans Mono Regular v2.014 with its OFL 1.1 license/provenance; embedded English, Ukrainian, and font resources; initialized the overlay font with required Latin, punctuation, arrow, and Cyrillic ranges; added deterministic coverage. User review of the Ukrainian wording is the next gate.
- Remaining: user editorial review of `locales/uk.json`; later integration of stable `en`/`uk` selection, `[Overlay] Language` persistence, runtime locale switching, and the selector UI.
- Deferred: Auto detection and other languages. No runtime smoke test or full production build in this batch.
- Blocked: none.
- Not runtime-validated: visual readability/font sizing, Ukrainian line wrapping, and actual in-game language switching.
- Patch summary: added the full Ukrainian translation source and embedded OFL-licensed font with deterministic character-coverage validation.
- Changelog summary: Ukrainian catalog and bundled Cyrillic font are prepared for editorial review; no language selector or runtime switch is enabled yet.
# 2026-09-24 — Restore ProggyClean with merged Cyrillic

- Scope: restore the ImGui default ProggyClean as the primary font and merge Cyrillic/supplemental symbols only; preserve the current font-size setting and scaling logic. No game launch and no Git interaction.
- Evidence: historical renderer used ImGui's default ProggyClean (13 px, pixel-snapped, 1x oversampling); user runtime screenshots showed the later whole-font replacements were substantially less readable.
- Changed: `src/overlay/localization_font.hpp/.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/overlay/localization_harness.cpp`, `tests/runner/localization_catalog_audit.ps1`; plan archived as `research/completed/PROGGYCLEAN_CYRILLIC_MERGE_TASK_PLAN.md`; rebuilt `STALKER2CameraTweaksOverlayIntegration.asi`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: catalog audit PASS (166/166 keys, ProggyClean-merge source contract PASS); `test.cmd` PASS (40/40 runner sources compiled/executed, all harnesses PASS, including atlas source/settings and EN/UK glyph coverage); `build-overlay-settings.cmd` PASS (exit 0; existing vendored Zydis/ImGui warnings only). No game launch.
- Completed: Latin/default glyphs come from unmodified `AddFontDefault()` ProggyClean; Segoe UI contributes only Cyrillic, general punctuation, and arrows with pixel snap enabled and 1x oversampling. The font-size control and `FontGlobalScale` code were not changed.
- Remaining: user visual A/B confirmation, especially Cyrillic glyph style/metrics alongside ProggyClean.
- Deferred: integer-sized atlas rebuild for the font-size control; any font styling adjustment pending the runtime comparison.
- Blocked: none.
- Not runtime-validated: in-game visual quality and language-switch rendering were not exercised by the agent.
- Patch summary: restored ImGui's original ProggyClean base and restricted the supplemental font to non-Latin glyph ranges.
- Changelog summary: the overlay returns to its original crisp Latin text while retaining Ukrainian coverage through merged Cyrillic glyphs.

## 2026-09-24 — Build ImGui rasterization experiment

- Scope: build one isolated Proggy Vector-only ASI with four atlas rasterization modes and the existing text-size control. No stable output, production font choice, localization, layout, camera behavior, or Git interaction.
- Evidence: Dear ImGui 1.91.9b uses stb_truetype for atlas construction; oversampling and PixelSnapH are configurable per font. The existing DX12 backend uploads the completed atlas.
- Changed: `src/overlay/renderer_runtime.cpp/.hpp` (experiment-macro-gated font loader, selector and mode-change atlas rebuild/rollback); `build-artifacts/experimental-font-ab/ProggyVectorRasterization/` (font loader, mode API, resource script, preflight audit, build script); `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayRasterization.asi`. Plan archived as `research/completed/IMGUI_RASTERIZATION_EXPERIMENT_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: experiment preflight PASS (166 English/Ukrainian catalog keys match; Proggy Vector and license resources present); isolated experimental ASI build PASS (2,165,760 bytes; standard vendored Zydis/ImGui warnings). The production-bound localization audit intentionally expects the production ProggyClean/system supplemental font configuration, so this build uses a scoped catalog/resource preflight instead. No game launch.
- Completed: experimental selector covers Pixel Crisp (snap, 1x/1x), Smooth 2x (no snap, 2x/2x), Smooth H2 (no snap, 2x/1x), and Smooth 3x (no snap, 3x/3x). Existing Text size persistence remains active. Rasterization selection is in-memory only and rebuilds the atlas using synchronized renderer recovery.
- Remaining: user's runtime visual A/B comparison.
- Deferred: selecting any production rasterization/font configuration; no production changes were promoted.
- Blocked: none.
- Not runtime-validated: in-game rendering, switching, and font readability; game was not launched.
- Patch summary: added one isolated diagnostic ASI to compare four stb_truetype atlas rasterization configurations without altering the stable artifact.
- Changelog summary: experimental build now offers four runtime-selectable rasterization modes while retaining the existing text-size control.

## 2026-09-24 — Build experimental font playground

- Scope: create one isolated in-overlay font chooser for ProggyClean + Proggy Vector Cyrillic, Proggy Vector, Gohu and Terminus. Keep language and text-size controls; omit FreeType if dependency setup is not already present. No stable output, camera/localization semantics, layout scaling, game launch, or Git interaction.
- Evidence: all four candidate paths (including the existing ProggyClean+Cyrillic production path) and font license files were already available locally. ImGui 1.91.9b uses stb_truetype; the repository contains no FreeType headers/library, and ImGui documents that FreeType needs separately supplied headers and library.
- Changed: `src/overlay/renderer_runtime.cpp/.hpp` (compile-flag-gated selection and safe atlas rebuild/rollback); `build-artifacts/experimental-font-ab/FontPlayground/` (selector/loader, RCDATA resources, preflight audit and build script); `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayFontPlayground.asi`. Plan archived as `research/completed/FONT_PLAYGROUND_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: playground preflight PASS (166 EN/UK keys match; Proggy Vector, Gohu, Terminus and license resources present); isolated ASI build PASS (2,757,632 bytes; existing vendored Zydis/ImGui warnings only). No game launch.
- Completed: runtime font choice is in-memory only and supports the four requested variants; the existing Language and Text size controls remain. Atlas selection/size changes use the synchronized rebuild and rollback path. The displayed rasterizer is fixed to stb Pixel Crisp.
- Remaining: user's one-session runtime visual comparison.
- Deferred: FreeType because its library/header dependency is absent and integration would require new build/dependency work; production font/rasterizer choice remains unchanged.
- Blocked: none.
- Not runtime-validated: in-game font appearance and runtime switching; game was not launched.
- Patch summary: added one experimental font playground ASI with four selectable existing candidates and retained language/text-size controls.
- Changelog summary: font candidates can now be compared in one run; FreeType was deferred and the stable build was left untouched.

## 2026-09-24 — Set stable overlay text default to 15 px

- Scope: promote the user-selected ProggyClean + Proggy Vector Cyrillic combination already present in production, set fresh/default text size to 15 px, and keep diagnostic font-choice code unavailable in the stable UI. Preserve existing saved sizes. No camera, localization semantics, layout scaling, game launch or Git interaction.
- Evidence: production `AddProggyCleanWithCyrillic` already uses ProggyClean for the base font and merges the embedded Proggy Vector Cyrillic range; production resources already embed the font and license. The font selector is guarded by `OVERLAY_FONT_PLAYGROUND_EXPERIMENT` and is not enabled by `build-overlay-settings.cmd`.
- Changed: `src/config/feature_config.hpp`, `src/config/config_template.cpp`, `src/config/config_repository.cpp`; `tests/config/config_persistence_harness.cpp`; rebuilt `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/FONT_DEFAULT_15_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: localization catalog audit PASS (166/166 EN/UK keys); `test.cmd` PASS (40/40 runner sets compiled/executed; all harnesses PASS); `build-overlay-settings.cmd` PASS (stable overlay ASI rebuilt; existing Zydis/ImGui warnings only). No game launch.
- Completed: fresh/absent/invalid configuration now defaults to 15 px; template-generated INI and managed template state `FontSize=15`. Existing saved `FontSize` is preserved. Stable font remains ProggyClean + Proggy Vector Cyrillic; no diagnostic Font selector is compiled into the stable UI, and playground selector implementation remains in code.
- Remaining: none in the approved build scope.
- Deferred: user runtime confirmation of the stable build.
- Blocked: none.
- Not runtime-validated: in-game appearance and persisted configuration behavior were not tested inside the game.
- Patch summary: changed the fresh/default overlay text size to 15 px and rebuilt the stable overlay artifact without changing the selected font.
- Changelog summary: new/default overlay configurations now start at 15 px; diagnostic font choice remains experimental-only.

## 2026-09-24 — Scale overlay horizontal layout with text size

- Scope: derive preferred overlay width, 45/55 columns and text-dependent horizontal spacing from active font size using 13 px as reference; clamp to viewport safe margins. No font, language, camera semantics, vertical scaling, game launch or Git interaction.
- Evidence: overlay width was fixed to a viewport-only 560-720 px preference, while columns remained a fixed 45/55 split and several horizontal style/control metrics did not follow font size. Active font size is available immediately after the synchronized atlas rebuild; window position is already corrected after Begin each frame.
- Changed: `src/overlay/overlay_layout_metrics.hpp/.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/overlay/overlay_layout_metrics_harness.cpp`, `test.cmd`, `build-overlay-settings.cmd`; rebuilt `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/OVERLAY_HORIZONTAL_LAYOUT_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: catalog and runner audits PASS; `test.cmd` PASS (41/41 harness sources compiled and executed, including 13/15/18/24 px, 640 px viewport clamp, narrow columns, 13 px reference split and safe-position clamp); `build-overlay-settings.cmd` PASS (ASI 2,167,808 bytes; existing Zydis/ImGui warnings only). No game launch.
- Completed: preferred width scales from the existing viewport-based 13 px reference and clamps to viewport width minus safe margins. Main columns keep the 45/55 split while honoring scaled minimums when space permits and falling back to nonnegative proportional widths when constrained. Horizontal style spacing/padding derives from a stored unscaled baseline; hotkey label/button offsets and notice key gaps scale with font size. Window position is clamped after Begin, so resized width is re-clamped in the same frame. Width calculation takes viewport and font size only, not content measurements.
- Remaining: user runtime confirmation at 13 → 15 → 18 → 24 → 13 px.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: actual overlay appearance, wrapping and window persistence while changing size; the game was not launched.
- Patch summary: made overlay width, columns, horizontal padding/spacing and fixed control offsets responsive to active font size with viewport-safe bounds.
- Changelog summary: increasing text size now expands available horizontal layout proportionally until viewport limits, without letting Camera State content set window width.

## 2026-09-24 — Replace scaled overlay layout with content-fit columns

- Scope: remove the font-size horizontal scale and 560–720 px preferred-width cap; let each column and its controls fit their own content/locale, with 6 px cell padding and only a viewport safety cap. Defer font-size persistence/atlas rebuild until slider edit ends. No camera or localization semantics, game launch or Git interaction.
- Evidence: prior code imposed a 720 px window maximum, fixed a 45/55 split, sized controls as 72% of each column, scaled horizontal metrics each frame, and applied every intermediate slider value (causing repeated atlas rebuilds while dragging).
- Changed: `src/overlay/renderer_runtime.cpp/.hpp`, `src/overlay/overlay_layout_metrics.hpp/.cpp`, `tests/overlay/overlay_layout_metrics_harness.cpp`, rebuilt `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/OVERLAY_CONTENT_WIDTH_TASK_PLAN.md`. `test.cmd` was not changed because the existing helper target already covered the updated harness.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (41/41 harnesses; updated viewport-cap tests PASS); `build-overlay-settings.cmd` PASS (ASI 2,167,296 bytes; existing vendored Zydis/ImGui warnings only). No game launch.
- Completed: removed font-size-derived horizontal scaling and the fixed preferred width/45-55 split. ImGui now auto-fits the two columns independently, controls size from their labels/options, and table cells have 6 px horizontal padding; the window may grow to the viewport-safe maximum. Font-size atlas rebuild is deferred until slider interaction ends.
- Remaining: user runtime confirmation that locale-specific content fits and the window no longer jitters while changing size.
- Deferred: any follow-up needed if actual in-game content exceeds viewport bounds; no further layout or camera work.
- Blocked: none.
- Not runtime-validated: in-game column sizing, wrapping at viewport limit, slider-release behavior and CPU impact; the game was not launched.
- Patch summary: replaced scaled/fixed-ratio layout with independent content-fit columns and removed repeated atlas rebuilds during font-size dragging.
- Changelog summary: overlay columns and controls now derive width from their content and locale, bounded only by available viewport width; font size applies after the slider edit completes.

## 2026-09-24 — Uniform select width and 50/50 columns

- Scope: use one Configuration select width computed from the widest option across all production select lists using current ImGui font metrics; make the two main columns equal-width auto-fit. Preserve viewport cap, font slider commit behavior, camera/localization semantics and avoid game launch/Git.
- Evidence: production select controls each used their own option-list width, causing visibly different widths; vendored ImGui documents `SizingFixedSame` as auto-sizing fixed columns to the maximum width of all contents.
- Changed: `src/overlay/renderer_runtime.cpp`; rebuilt `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/OVERLAY_UNIFORM_SELECT_WIDTH_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: `test.cmd` PASS (41/41 harnesses); `build-overlay-settings.cmd` PASS (ASI 2,167,808 bytes; existing vendored Zydis/ImGui warnings only). ImGui table behavior confirmed against vendored source. No game launch.
- Completed: Gameplay, Cinematics, Dialogue and Overlay language selects share the maximum width computed over all their displayed option strings, including active font/locale metrics; changing selected values does not affect width. The two primary columns now share the same auto-fit width, with the existing viewport bound.
- Remaining: user runtime confirmation of alignment, width and wrapping.
- Deferred: none.
- Blocked: none.
- Not runtime-validated: actual in-game appearance at both locales/font sizes and viewport limits.
- Patch summary: aligned all production select controls to a common content-derived width and changed the main table to equal auto-fit columns.
- Changelog summary: Configuration select geometry is now stable across option changes, while both overlay columns remain content/viewport adaptive and evenly sized.

## 2026-09-24 — Data-driven localization registry

- Scope: replace per-language production C++ branches and catalog/resource parameters with registry-driven locale-code lookup for existing `en` and `uk` only. Preserve embedded JSON, single-ASI distribution, runtime switching, persistence and missing-key diagnostics. No new translations, font/layout/camera changes, game launch or Git interaction.
- Evidence: prior manager/config/runtime/UI represented English and Ukrainian as enum cases, separate catalog fields and explicit selection branches; locale catalogs were already embedded in the ASI and English was canonical.
- Changed: added `src/localization/locale_registry.hpp`; updated `src/overlay/localization_manager.hpp/.cpp`, `src/overlay/renderer_runtime.cpp`, `src/overlay/localization_resource_ids.h`, `src/overlay/localization_resources.rc`, `src/config/feature_config.hpp/.cpp`, `src/config/config_repository.cpp`, `src/config/config_template.cpp`, `src/plugin/runtime_settings.hpp/.cpp`, `src/plugin/runtime.cpp`, localization key inventory and both locale JSON catalogs to remove language-name selector keys; updated localization/config/runtime harnesses, `tests/runner/localization_catalog_audit.ps1`, `test.cmd` and `build-overlay-settings.cmd`. Built `STALKER2CameraTweaksOverlayIntegration.asi`. Plan archived as `research/completed/LOCALIZATION_DATA_DRIVEN_REGISTRY_TASK_PLAN.md`.
- Git: not inspected or changed, per explicit user instruction.
- Validation: localization audit PASS (`locales=2`, canonical `en`, 164 keys, registry lookup, parity, placeholders, UTF-8, embedded-resource association, generic manager and selector); `test.cmd` PASS (41/41 runner sources compiled and executed; all harnesses PASS); `build-overlay-settings.cmd` PASS (stable ASI built; existing vendored Zydis/ImGui warnings only). No game launch.
- Completed: locale metadata now centrally describes display name, stable code, catalog filename, canonical role and neutral embedded resource ID. Manager loads/validates registered catalogs into a collection, chooses catalogs by code, and falls back to canonical `en`; invalid explicit selection preserves the current locale. Runtime persistence stores locale codes and the selector iterates registry metadata. Existing `en` and `uk` pass the generic path.
- Remaining: none in the approved source/build scope.
- Deferred: adding the other 16 locales; user runtime confirmation.
- Blocked: none.
- Not runtime-validated: in-game locale switching and persistence; game was not launched.
- Patch summary: replaced language-specific localization plumbing with a locale-code registry and registry-driven embedded catalog loading, validation, persistence and UI selection.
- Changelog summary: localization now scales by adding registry/resource metadata and a catalog file rather than adding per-language branches throughout production C++.
