# Task Plan — Refresh Overlay ASI

- Objective: build the current `STALKER2CameraTweaksOverlayIntegration.asi` from the working source and canonical English catalog.
- Evidence/current state: `build-overlay-settings.cmd` targets the named ASI in the repository root; `locales/en.json` has the latest editorial updates and passed catalog audit. The referenced testing-guidelines file is absent from this checkout.
- Approved scope: run the catalog audit and overlay build; replace only the named ASI output.
- Non-goals: no source changes, game launch/injection, packaging, cleanup, or Git commands.
- Expected paths: `STALKER2CameraTweaksOverlayIntegration.asi`; archive this plan under `research/completed/` after validation.
- Batches/validation: (1) catalog audit and JSON parse; (2) run `build-overlay-settings.cmd`, verify successful exit and output timestamp/size.
- Risks/safe failure: the build replaces the existing ASI only after the user-requested rebuild; on build failure report it and do not launch the game.
- Stop conditions: missing toolchain, catalog audit failure, or build failure.
- Final review: verify build exit code and resulting artifact metadata. Git review omitted per user's standing instruction not to touch Git.
