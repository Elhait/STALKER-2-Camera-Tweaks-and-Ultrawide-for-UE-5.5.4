# Overlay / Shared Camera State F10 — Lifecycle Characterization

**Status:** CLOSED offline for observable camera lifecycle; Main Menu/save-load
session-boundary invalidation remains OPEN because no authoritative production
event was found.

## Objective

Characterize the independent lifetimes of shared Camera State roles and
Transition Evidence across cold start, Gameplay, Cinematic, Dialogue,
continuation, recovery, and Gameplay handoff. Determine what can be safely
invalidated from authoritative production events. Do not change camera
correction, coordinator ownership, or transaction semantics.

## Evidence established so far

- `RuntimeState` owns `GameplayBaselineStore`, the shared `CameraStateSnapshot`,
  coordinator, and accepted transition-boundary record.
- The coordinator has only `Gameplay`, `CinematicActive`, and
  `CinematicExiting`; it initializes to `Gameplay` and has no Main Menu state.
- Shared fact freshness changes on explicit Gameplay/Cinematic lifecycle,
  Dialogue, zoom, writer-source, and configuration events. No shared snapshot
  clear/reset is connected to a game Main Menu or save-load event.
- The production integration path examined so far has no Main Menu/save-load
  callback. Absence of writer callbacks is not authoritative evidence of a
  menu transition, so no silence timeout or blanket snapshot clear is in scope
  without additional source evidence.
- Viewport aspect is sampled from current client geometry during semantic read;
  it is independent of camera writer lifetime.
- Transition Evidence is a separate accepted boundary record with its own
  lifecycle generation. It must not be treated as live FOV or reset merely
  because live writer observations move through EXIT/Recovery.

## Characterized lifetimes and remediation

| Lifecycle point | Gameplay FOV | Native / Output | HorPlus Target | Writer aspect / flags | Viewport | Transition Evidence / metadata |
| --- | --- | --- | --- | --- | --- | --- |
| Cold start, before publication | Unavailable | Unavailable | Unavailable | Unavailable | AtRead if client geometry is usable | No accepted boundary; coordinator storage nevertheless defaults to Gameplay and semantic read currently marks that enum value valid. |
| Gameplay writer publication | Current only after the shared neutral/stable qualification; production baseline store has its own broader eligibility | Current from the writer transaction | Current if applied; otherwise prior target is Historical | Current from full writer publication | Sampled for that observation and separately AtRead in semantic projection | Last accepted boundary is retained independently. |
| Accepted Cinematic ENTER, before next writer transaction | Retained if a valid Gameplay baseline exists | Prior owner becomes Historical | Retained, not relabeled as a live sample | Historical until a new full writer observation | AtRead | ENTER creates/replaces the accepted boundary evidence and generation. |
| Cinematic / Dialogue transaction | Retained Gameplay baseline | Current for the published transaction; Dialogue lifecycle alone does not invalidate independent FOV facts | Current when supplied by the transaction, otherwise Retained | Writer metadata may remain Historical because transaction publication does not refresh aspect/flags | AtRead | ENTER evidence remains separate from live roles. |
| Accepted EXIT / recovery boundary | Retained | Prior owner becomes Historical until a new transaction | Retained | Historical until refreshed | AtRead | Existing ENTER evidence remains stored; EXIT does not clear it. |
| Gameplay handoff and new full publication | Returns Current only under its own confirmation contract | Current from the new writer transaction | Current if applied | Current from the full observation | AtRead | Prior transition evidence remains the last accepted transition until a later accepted ENTER replaces it. |
| Main Menu / save-load with no recognized callback | No contract can be established for a new session | Prior facts may remain marked Current | May remain Retained | May remain Current | Still reflects current client geometry | Prior boundary may remain visible indefinitely; no reset event or session generation exists in the inspected production path. |

At confirmed presentation-owner boundaries, the shared snapshot now marks the
previous writer observation (including Native FOV and Current Output FOV) as
Historical. Gameplay FOV remains Retained and HorPlus Target remains Retained.
A subsequent FOV transaction refreshes its own live roles; a subsequent full
Gameplay writer publication refreshes writer aspect/flags. These are freshness
only changes and do not alter correction output or coordinator transitions.

The production coordinator remains `Gameplay` / `CinematicActive` /
`CinematicExiting`; it is not a game-session classifier. Semantic read also
reports the stored coordinator as valid even before the first camera
publication. This startup/menu ambiguity is recorded, not relabeled or
heuristically cleared in this batch.

## Bounded work

1. Trace each shared FOV role, writer/policy aspect, viewport, Transition
   Evidence, coordinator and publication metadata through known camera lifecycle
   events and existing invalidation calls.
2. **Complete:** add production-equivalent regressions for accepted presentation
   boundaries, transaction refresh, Dialogue independence, recovery boundary,
   Gameplay handoff, and retained Gameplay FOV. Snapshot semantic-change
   notification remains change-driven.
3. **Complete:** on accepted ENTER/EXIT and observed coordinator transitions,
   mark the prior writer observation Historical until a new writer observation
   is published. No camera output or ownership transition changes.
4. **Open:** Main Menu → Load Save and Gameplay → Main Menu remain insufficiently
   observable in current source. Do not infer them from coordinator default,
   callback silence, viewport stability, or a timeout.
5. **Complete offline / open session boundary:** known camera lifecycle is
   characterized offline; Main Menu/save-load reset behavior is not established.
   F9 may proceed using the known lifecycle contracts, but must preserve this
   limitation and must not infer a session reset.

## Acceptance

- A focused regression covers cold/unpublished, Gameplay publication,
  ENTER/active, Dialogue, continuation, EXIT/Recovery, and post-handoff states
  with per-role `Current / Retained / Historical / Unavailable` assertions.
- Transition Evidence remains separate from live FOV and its validity/lifecycle
  is asserted independently.
- Summary and expanded Camera State read the same shared snapshot semantics.
- Any freshness change wakes the Overlay only when visible semantics change.
- No correction math, production FOV output, aspect mutation, coordinator
  transition, or ownership behavior changes.
- Main Menu/save-load behavior is called runtime-confirmed only if an
  authoritative event and exact artifact/session evidence are available.

## Validation update — 2026-10-07

- `test.cmd`: all 58 registered harnesses compiled and executed; the focused
  Camera Writer Transaction harness and other non-localization harnesses
  passed. The command exits nonzero at `localization_harness` for existing CJK
  text-glyph gaps; full suite is not PASS.
- Locale catalog/resource audit: PASS (18 catalogs).
- `build.cmd`: PASS; production ASI emitted at 3,387,904 bytes, SHA-256
  `688384001FDE50737D087B44A9EF6874312129D8B95D37ADC1E7073F9529A382`.
- No game launch, ASI installation, or runtime Main Menu/session test.
- This artifact is an offline candidate only; do not attribute prior runtime
  validation to it.

## Follow-up status — 2026-10-07

F9 consolidation is tracked in
[the F9 consolidation plan](OVERLAY_SHARED_CAMERA_STATE_F9_CONSOLIDATION_PLAN.md).
This does not close Main Menu/save-load lifecycle. Offline evidence establishes
known camera-owner transitions only; session-boundary behavior still requires
an authoritative production event and matching-artifact runtime evidence.

## Non-goals

- No blanket `ClearSnapshot` at Main Menu.
- No timer-based staleness, callback-silence inference, or new guessed game
  lifecycle classifier.
- No RuntimeCombine, HorPlus, Cinematic, Dialogue, or recovery algorithm edits.
- No F9 consolidation, release promotion, game launch, or installed ASI update.

## Validation / reporting boundary

Run only focused harnesses and the project build when source changes require
them. Keep the current status distinction: focused regression PASS, locale
resource audit PASS, build PASS, full suite NOT PASS due CJK glyph coverage.
Do not attribute a prior full-suite result or runtime result to a changed
artifact.
