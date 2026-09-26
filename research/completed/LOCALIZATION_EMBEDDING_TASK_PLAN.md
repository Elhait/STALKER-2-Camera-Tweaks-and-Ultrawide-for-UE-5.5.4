# Task Plan — Embed Localization Catalogs in Overlay ASI

## Objective
Keep the user distribution single-file by embedding build-time JSON catalogs into `STALKER2CameraTweaksOverlayIntegration.asi` and removing all production filesystem catalog loading.

## Established Evidence and Current State
- `Renderer::BuildImGui` resolves `locales/en.json` beside the ASI; missing files leave every valid key rendered as `[missing: ...]`, matching the user's runtime screenshot.
- `build-overlay-settings.cmd` checks that source JSON exists but neither embeds nor deploys it.
- `LocalizationManager` currently accepts a filesystem path; `LocalizationCatalog::LoadFile` implements runtime disk loading.
- `locales/en.json` is the canonical source catalog and currently passes the audit at 162/162 typed-key parity.
- `docs/ARCHITECTURE.md` and `docs/SAFETY_INVARIANTS.md` were reviewed. The referenced implementation/testing/style guidance files are absent in this checkout.
- User explicitly prohibits all Git interaction; no Git commands or state inspection will be used.

## Approved Scope
- Embed English JSON as a Windows RCDATA resource in the overlay integration ASI.
- Read the resource from the ASI module and pass its bytes to the existing catalog parser/validator.
- Remove runtime filesystem catalog loading; preserve missing-key diagnostics and the current typed-key/catalog architecture.
- Make build validation fail on catalog contract violations and ensure the resource is linked into both production ASI and deterministic localization harness.

## Non-Goals
- Additional languages, selector, Auto detection, font work, translations, camera/runtime semantics, release packaging, game launch/injection, or Git interaction.

## Expected Files or Areas
- `src/overlay/localization_catalog.hpp/.cpp`, `localization_manager.hpp/.cpp`, `renderer_runtime.cpp`.
- New `src/overlay/localization_resources.rc`.
- `build-overlay-settings.cmd`, `test.cmd`, localization harness and catalog audit.
- This plan, archived to `research/completed/` when complete.

## Batches and Validation
1. Convert manager/catalog API to in-memory JSON only; add deterministic manager tests. Validate with localization harness.
2. Embed `locales/en.json` as named RCDATA, load it from the ASI module, and link the resource in build/test scripts. Validate that the harness loads and validates the actual embedded catalog and that source audit forbids runtime file dependencies.
3. Run `test.cmd`, catalog audit, and `build-overlay-settings.cmd`; verify successful ASI output metadata. Do not launch the game.

## Risks and Safe-Failure Behavior
- Resource lookup must use the overlay module, not the host executable; resolve the module from a function address.
- RCDATA must preserve the exact JSON bytes; use `FindResourceW`/`LoadResource`/`LockResource` and bounded size checks.
- Missing/invalid embedded resource leaves diagnostic `[missing: key]` behavior intact and must not affect camera runtime functionality.
- Build output remains one ASI; no locale directory is copied to the game.

## Stop Conditions and Phase Gates
- Stop if the resource cannot be resolved to the ASI module or is not linked into the output.
- Stop on any catalog audit, deterministic test, or build failure. Do not run the game.

## Final Review
Verify tests, resource linkage/build output, source audit for filesystem locale loading, and exact plan scope. Git review is omitted as explicitly prohibited by the user.
