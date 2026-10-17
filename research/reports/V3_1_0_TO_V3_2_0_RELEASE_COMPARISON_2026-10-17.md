# v3.1.0 → Proposed v3.2.0 — Release Comparison

**Status:** Draft comparison of the current staged source diff; v3.2.0 is a recommended next version, not yet an official release.

**Baseline:** tag `v3.1.0`, commit `e756442` (`release: prepare v3.1.0`). The checked-out `HEAD` is exactly this tag.

**Compared state:** staged changes at the time of this report. The staged diff contains 236 paths, approximately 24,358 insertions and 1,167 deletions. It includes product code, tests, documentation, research/evidence, and local generated state. This comparison describes the product-relevant result and identifies material that must stay out of release packaging.

## Executive comparison

Version 3.1.0 shipped the unified Gameplay, Cinematics and Dialogue camera corrections, the localized settings Overlay, passive diagnostics in the standard ASI, and the build/test workflow described in its release notes.

The proposed 3.2.0 retains those camera policies and adds a substantial Overlay architecture and lifecycle migration, a richer read-only Camera State panel, safer configuration ownership/migration, broader failure handling, and stronger test/build containment. The user-facing change is primarily in the Overlay, its startup notification and installation/configuration experience; the camera correction modes and their formulas are not intentionally changed by the accumulated work.

**Recommended version:** `3.2.0`. This is a feature release with a major internal architecture change and several new user-visible Overlay/configuration behaviors. The comparison does not establish that the release is ready to publish; final release qualification and package creation remain separate decisions.

## User-facing changes since 3.1.0

### Overlay display and interaction

- The Overlay now renders through a dedicated, unowned top-level Overlay HWND with a persistent DirectComposition target. The game HWND remains responsible for game input, host discovery, language access and host geometry; the Overlay HWND is the display surface.
- The display window remains click-through. Input admission is coordinated through the game-window callback path and only becomes active when the Overlay is ready to receive interaction. Failure paths close input admission and hand graphics cleanup back to the Overlay owner thread.
- The Overlay lifecycle now handles host activation/rebinding, minimize/restore, geometry updates, DPI scaling, negative virtual-desktop coordinates and z-order reconciliation. Its scheduler avoids unnecessary placement updates.
- The startup welcome notification has a readiness gate separating runtime initialization, Overlay readiness and observed game data. The one-shot toast keeps its existing 10-second draw/commit-gated lifetime and Auto locale behavior.
- The settings window can expand rightward to show a third, independently scrollable Camera State panel. The two existing columns remain in place; a localized “Camera Details / Open / Close” control distinguishes the panel toggle from ordinary section headers. Compact geometry is restored when the panel closes.
- Camera State remains observational and read-only. Unavailable values continue to be shown as unavailable rather than inferred from unrelated state.

### Configuration and installation

- `STALKER2CameraTweaks.ini` is generated beside the ASI and is user-owned; the release ZIP and Vortex copy instructions contain only the ASI plus documentation/license notices.
- Known older INI layouts are rebuilt into the current documented structure while compatible settings are preserved. Unreadable/unknown input is backed up before recovery where supported. Current INI files are left untouched when no managed change is required, and Overlay changes persist through the configuration owner.
- Configuration file reads/writes use the native file path, preserve recognized user values and relevant line-ending shape, and stage replacement before promotion. The generated template documents the settings and Boolean behavior.
- Vortex deployment metadata no longer directs the manager to install the INI as a managed release file.

### Localization and diagnostics

- All 18 embedded locale catalogs add the Camera State panel toggle labels and a persistence-failure message.
- The existing standard ASI diagnostics remain controlled by `[Diagnostics] Enabled`; this work does not require a separate diagnostics build for the supported passive diagnostics.
- Startup/readiness and Overlay lifecycle diagnostics were extended during investigation. Instrumentation is observational; it is not evidence by itself that a frame reached the physical display.

## Reliability and engineering changes

- Game-window callbacks use explicit admission/drain contracts and enqueue owner-thread commands rather than performing persistence or graphics teardown synchronously. The command/owner boundary preserves fail-open behavior for the game.
- The Overlay owner loop and finalizer handle terminal requests independently of ordinary message-queue capacity. Resource cleanup remains on the thread that owns ImGui, D3D11, DirectComposition and the display HWND; completion states distinguish released, retained and unverified resources.
- Partial initialization and optional Overlay failures have explicit recovery paths. Camera correction availability is kept separate from optional Overlay availability.
- INI and log I/O were moved away from CRT `FILE` stream teardown paths. Selected mutable global/static containers and the SafetyHook trap manager use process-lifetime storage to avoid unsafe automatic destruction during process termination.
- SafetyHook's resident vectored exception handler checks the Windows shutdown state before accessing potentially interrupted trap state. The module is pinned after initialization begins so process-lifetime callbacks do not outlive an unloadable image. This does not claim support for arbitrary manual unload or guarantee every third-party/OS teardown path.
- Native hook setup, camera-state publication, input rebinding, viewport geometry, DPI behavior and overlay layout gained focused contracts and regression coverage. Existing Gameplay/Cinematics/Dialogue correction policies are intended to remain unchanged.

## Build, tests and release packaging

- CMake/Ninja build and test outputs are isolated under the selected build directory rather than scattered through the source tree.
- Test invocation now supervises child process trees with bounded execution and cleanup checks; nonzero/crash exits are reported as failures. The canonical registry and runner were updated with the new focused cases.
- A release packaging script uses a strict file allowlist, verifies the archive inventory, excludes the INI and checks that the ASI hash in the archive matches its input.
- Research fixtures, ETW/PresentMon captures, logs, prototypes, reports and test evidence are development records and are not release contents.

## Camera behavior retained from 3.1.0

The documented Gameplay `HorPlus`, `AspectRecalculation` and `RuntimeCombine` choices; cinematic aspect/FOV modes; Dialogue zoom modes; independent module switches; and existing camera ownership/recovery policies remain the product baseline. The diff adds observational Camera State presentation and strengthens surrounding lifecycle/configuration contracts. No deliberate camera formula or supported mode change was identified in the current diff review.

This is a source-diff statement, not a fresh in-game proof of every camera mode on the proposed release artifact.

## Validation evidence and limits

Evidence attached to the current source state includes:

- G1–G5 Overlay-owned HWND architecture and integration reports, including offline lifecycle/geometry/regression work and a prior live compatibility observation for the then-tested candidate.
- F2/F4 focused remediation reports for configuration, native callbacks/hooks, input, lifecycle, build isolation and regression runner contracts.
- F7/F8 startup toast readiness/presentation investigations and recorded live evidence, including one successful visible-before-Delete run.
- Shutdown hardening evidence for focused checks and the safe non-graphics canonical suite: 62 ordered invocations, 208 adapter child executions and four Zoom fixtures. The graphics/WARP cleanup fixture was intentionally omitted from that safe run.
- A user-confirmed ordinary game exit for the shutdown-hardening candidate, SHA-256 `97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4`. The three runtime logs showed successful initialization and normal Overlay open/close transitions; the user confirmed the game exited normally.

Limits:

- The shutdown candidate's ordinary exit was manually confirmed, but that does not prove every process-termination, crash, third-party unload or graphics-driver path.
- The latest shutdown-safe canonical run omitted the WARP/graphics fixture; it must not be described as a full graphics-inclusive test run.
- Prior NVIDIA HUD/G-SYNC/Frame Generation observations apply to the exact earlier candidates recorded in their evidence. Do not transfer those observations to a new SHA without matching evidence.
- The staged current source has not been identified here as an official final release ZIP. A production artifact hash is not a substitute for archive creation/inventory validation, release-note reconciliation or final release authorization.

## Version and release recommendation

Use `3.2.0` as the working next version, subject to final release qualification. The change set is larger than a patch release because it introduces the Overlay-owned HWND architecture, the expanded Camera State interaction and revised configuration/install behavior. It does not appear to require a major-version bump because the established camera modes and their documented semantics are retained and the INI migration is designed to preserve compatible user settings.

Before publishing, prepare new `RELEASE_NOTES_v3.2.0.md` and `NEXUS_CHANGELOG_v3.2.0.md`, update the GitHub/Nexus release text and version references, create the allowlisted ZIP from the final candidate, and record the exact ASI/ZIP hashes. Do not repurpose the v3.1.0 release notes as the new release document.

## Current diff hygiene: release exclusions

The following current changes are useful to the repository but are not product payload:

- `research/evidence/` (72 changed paths), `research/reports/` (30), `research/plans/` (10), `research/tools/` (15) and `research/prototypes/` (10): retain as provenance/evidence in the source repository; exclude from the user archive.
- Test harnesses, runner scripts, CMake test targets and build artifacts: development/CI material; exclude from the user archive.
- `imgui.ini`: local generated UI state, not a user feature or release asset. Exclude it from packaging and consider whether it belongs in the source diff at all before finalizing the repository change.
- `external/safetyhook/LOCAL_CHANGES.md`: source provenance for the local vendor modification; retain in source distribution as appropriate, while the release archive remains governed by its strict allowlist.

The current package script's required-file allowlist is the production ASI, README, LICENSE and third-party notices; it rejects unexpected ZIP entries and validates the packed ASI hash. The INI is explicitly excluded.

## Review disposition

This document is a comparison aid for release planning. It does not mark the source diff approved, certify every staged file as release-ready, create an official v3.2.0 package, or authorize publication. The current release-facing documents still identify v3.1.0 and should be replaced or updated as a coordinated release-preparation step after the proposed version and final qualification are accepted.
