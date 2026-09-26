# Task Plan — Wrap Runtime Mouse-Capture Notice

- Objective: wrap the localized Runtime settings mouse-capture notice within the overlay content width at any translated text or hotkey length, preserving its inline keycap styling.
- Evidence/current state: the user's runtime screenshot shows the notice extending past the right edge. The renderer formats one string, splits around `{key}`, and emits unwrapped text segments.
- Approved scope: presentation-only change in `src/overlay/renderer_runtime.cpp`, relevant deterministic/source audit if practical, full test suite, and overlay ASI build.
- Non-goals: localization catalog wording/schema, input behavior, hotkey behavior, window/layout sizing, other UI text, game launch, or Git interaction.
- Expected files: `src/overlay/renderer_runtime.cpp`, `tests/runner/localization_catalog_audit.ps1` only if a bounded check is warranted, task log, and refreshed `STALKER2CameraTweaksOverlayIntegration.asi`.
- Batches/validation: (1) apply ImGui wrap boundary around the notice's before/key/after segments; (2) run `test.cmd` and catalog audit; (3) run `build-overlay-settings.cmd` and verify the ASI output.
- Risks/safe failure: avoid changing wrap state for subsequent widgets by balancing push/pop in the same drawing branch. On test/build failure, report without game launch.
- Stop conditions: preserving inline keycap requires changing input or overlay layout semantics, or any validation fails.
- Final review: verify only approved presentation/build artifact paths, tests and build. Git review omitted per user's explicit instruction not to touch Git.
