# Overlay / Shared Camera State — F9 Consolidation

**Updated:** 2026-10-07  
**Status:** Offline implementation complete for the proven duplicate numeric FOV storage; distinct writer/transformed freshness metadata remains.  
**Scope:** Consolidate only state proven to represent the same semantic fact. Do not change camera correction, owner/coordinator behavior, or F10 session-boundary semantics.

## F10 boundary carried into F9

The known observable camera lifecycle is characterized offline: Gameplay → ENTER → Cinematic/Dialogue/continuation → EXIT/Recovery → Gameplay. A session-boundary invalidation event for Main Menu/save-load is still unknown and remains **OPEN**. F9 must not invent a MainMenu owner, timeout, or callback-silence rule. See [F10 lifecycle plan](OVERLAY_SHARED_CAMERA_STATE_F10_LIFECYCLE_PLAN.md).

## Method

For every candidate, identify authoritative producer, semantic role, lifetime/validity, consumers, and any second stored copy. Migrate consumers to the selected authority before removing duplicate storage. Keep fields whose producer boundary or validity contract is materially different, even if values often compare equal. Add production-equivalent lifecycle regressions for each actual migration.

## Initial authoritative-state map

| State | Producer / owner | Meaning and lifetime | Decision |
|---|---|---|---|
| `CameraFovFact::gameplayFov` | shared writer publisher; stable neutral Gameplay writer observations | Retained Gameplay baseline; Current only after the existing same-source, three-sample neutral qualification; transitions retain it. Carries value, validity, freshness, provenance, source, sequence together. | Canonical shared/presentation fact. |
| Former `retainedGameplayFov` scalar + validity/provenance/source + freshness member | same shared publisher and same neutral qualification as `gameplayFov` | Duplicate backing storage for the canonical Gameplay role; publication copied scalar metadata into the fact and transaction/lifecycle code copied it back. | **Consolidated** into `gameplayFov`; qualification and zoom/source safety gates preserved. |
| `GameplayBaselineStore` | production observation projection in runtime | Cinematic production input baseline. Accepts eligible native or transformed writer observations (including pass-through/native aspect) and has explicit invalidation. Its eligibility is not the shared stable-neutral qualification. | Keep separate. It is a production authority with a different contract, not an Overlay duplicate. |
| `CameraFovFact::nativeFov` | full writer and transaction publication | Native input on the latest applicable camera transaction; transaction owner/source/sequence can differ from the last full publication. Current/retained/historical follows transaction lifecycle. | Canonical four-role FOV fact. |
| Former `nativeWriterFov` scalar + validity/provenance | full writer publication only | Its numeric sample duplicated Native FOV on full publication. The needed full-writer clock/validity is represented by `freshness.writerObservation` and `writerObservationHasEvidence` alongside aspect/flags metadata; transaction-only publication may update Native FOV without refreshing that group. | **Consolidated** numeric storage into `nativeFov`; keep writer-observation freshness/evidence metadata independent. |
| `CameraFovFact::horPlusTargetFov` | applied writer/transaction target publication | Target value and its own Current/Retained/Historical lifecycle; can survive pass-through as non-current historical context. | Canonical target fact. |
| Former `transformedFov` scalar + validity/provenance; `freshness.transformedResult` group | full writer publication only | On an applied full writer publication its number duplicates actual output and target values. On pass-through the transformed-result group becomes unavailable even when the prior target remains valid but historical. | **Consolidated** numeric storage into `currentOutputFov` / `horPlusTargetFov`; retain transformed-result freshness as distinct observation metadata. |
| `CameraFovFact::currentOutputFov` | full writer and transaction publication | Actual output returned by the current applicable transaction. | Canonical output fact. |
| Transition Evidence | ENTER boundary publication | Captured comparison operands and result; independent lifetime from live roles. | Keep separate; F9 does not rebind evidence to live facts. |

## F9 bounded work

1. Consolidate duplicate numeric FOV storage into the canonical facts without changing the three-sample neutral qualification, source checks, zoom hold, lifecycle retention, or production `GameplayBaselineStore`.
2. Preserve `writerObservation` validity as group metadata for the full writer aspect/flags observation; do not infer it from whichever FOV transaction most recently updated `nativeFov`.
3. Preserve `transformedResult` as distinct from target/output freshness: pass-through invalidates the current transformed-result role while the prior target may remain historical.
4. Add/extend production-equivalent regressions across Gameplay, ENTER, CinematicActive, Dialogue, continuation, Recovery, and Gameplay handoff; check freshness/provenance/source/sequence plus unchanged output.
5. Update the architecture audit and F10/F9 status notes with the final authoritative map, proven deletions, and residual open boundaries.

## Acceptance

- Canonical FOV facts alone own numeric values and role metadata; no parallel numeric FOV aggregate remains in shared Camera State.
- Production `GameplayBaselineStore` contract and production consumers remain unchanged.
- Writer observation and transformed-result lifetimes remain independently represented where their contracts differ from canonical semantic roles.
- Existing camera behavior and coordinator ownership are unchanged.
- Focused harnesses, full test runner, and production build are reported with exact pass/fail boundary. No runtime/installed artifact claim without an actual game run.

## Execution record

- Removed the duplicated `retainedGameplayFov` scalar, valid/provenance/source tuple, and parallel freshness field. The canonical `CameraFovFact gameplayFov` now owns value, validity, provenance, freshness, source, and sequence.
- Kept the original neutral Gameplay qualification (same-source stable samples, valid writer boundary/result semantics, no Dialogue/recovery/mode-transition/zoom exclusion). Kept `GameplayBaselineStore` and its consumers unchanged; it has different production eligibility and invalidation semantics.
- Removed `nativeWriterFov` and `transformedFov` numeric aggregates. Retained `writerObservation` and `transformedResult` freshness groups because those clocks differ from transaction Native FOV and HorPlus Target; `writerObservationHasEvidence` preserves the old invalidation behavior for partial full-writer samples without retaining another FOV value.
- Production-equivalent regression remains in `camera_writer_transaction_harness`: stable A qualification → settings off → source B without publication → first B sample → B confirmation → lifecycle/Dialogue/recovery and Gameplay handoff; assertions inspect canonical fact freshness/source/value. Snapshot and integration harnesses use only canonical `gameplayFov`.
- `test.cmd`: all 58 registered harnesses compiled and executed. All camera, projection, persistence, and other non-localization checks passed; suite exit is nonzero at embedded-font localization glyph coverage (known CJK gaps). Full suite status: **NOT PASS**.
- Locale/resource audit: **PASS** (18 catalogs). `build.cmd`: **PASS**; production ASI emitted (3,387,392 bytes; SHA-256 `EA719B59E632C3ABD7B827F6910A5FAF70712BB4D7AFEB5360F9A9B1EC9F15BC`). No game launch or ASI installation.
- F10 known camera lifecycle remains offline-closed; Main Menu/save-load session-boundary invalidation remains **OPEN / authoritative event unknown**. No guessed owner or timeout added.
- Independent read-only Sol audit: **no correctness blocker** to the planned runtime lifecycle acceptance. It confirmed the F9 writer invalidation predicate and lifecycle coverage. It identified two pre-existing, non-blocking presentation risks outside F9: summary viewport aspect is sampled at semantic read while expanded details use the last writer-published viewport aspect; RuntimeCombine diagnostic telemetry has no explicit freshness row after lifecycle changes. These risks do not change camera behavior and were not modified in F9.

## F9 disposition

| Candidate | Disposition |
|---|---|
| `retainedGameplayFov` storage vs `gameplayFov` | **Consolidated** — one semantic baseline and one owner; qualification unchanged. |
| `GameplayBaselineStore` vs shared `gameplayFov` | **Kept separate** — production cinematic baseline accepts a different observation contract and serves production correction; shared fact is a qualified stable neutral baseline. |
| `nativeWriterFov` vs `nativeFov` | **Numeric value consolidated** — the fact owns the FOV; writer-observation freshness/evidence independently preserve the full-writer aspect/flags clock. |
| `transformedFov` vs `horPlusTargetFov` / `currentOutputFov` | **Numeric value consolidated** — applied output/target are owned by canonical facts; transformed-result freshness independently records whether the latest full writer sample transformed. |

No broader universal-state refactor is justified by this batch. Session-boundary invalidation, other audit findings, independent Sol audit, and runtime acceptance remain separate follow-ups.
