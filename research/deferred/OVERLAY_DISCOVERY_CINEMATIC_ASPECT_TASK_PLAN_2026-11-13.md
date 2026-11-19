# Deferred task: Overlay discovery and cinematic aspect regression

Status: Deferred until the current EXIT1 → Dialogue → ENTER2 correction is implemented and visually validated.

## Evidence

Snapshot: [`research/evidence/overlay-cinematic-aspect-2026-11-13/`](../evidence/overlay-cinematic-aspect-2026-11-13/).

The captured production ASI initialized successfully. Gameplay, Cinematics and Dialogue reported AVAILABLE; cinematic signature validation passed. The capture contains no subsequent cinematic runtime events, so it does not establish why the reported scenes rendered at 16:9.

The overlay log reports repeated `OVERLAY_DISCOVERY_SCAN ... eligible=0 selected=0` and `OVERLAY_REFRESH_STATE state=no_eligible_game_window`. The same run therefore explains the missing overlay at capture time by failure to select a visible eligible game window; it does not establish the underlying window-classification cause.

The user reports prior public comments about this behavior. Those comments are not included in this evidence snapshot and should be gathered when this task becomes active.

## Scope when resumed

1. Reproduce missing overlay discovery after a normal game start and identify the rejected game HWND and the exact eligibility predicate.
2. Separately reproduce cinematic aspect falling back to 16:9; inspect the runtime Cinematic ENTER/aspect policy records and confirm which viewport source/policy was selected.
3. Determine whether the two symptoms share an HWND discovery/lifecycle cause or are independent.
4. Compare against the user-referenced public reports before changing behavior.
5. Implement only after the failing predicate and aspect-selection path are established; retain existing camera and overlay failure isolation.

## Acceptance

- Overlay selects the visible game window and appears in the intended session.
- Cinematics apply the configured ultrawide aspect policy in a captured runtime ENTER.
- The fix does not disturb ordinary Gameplay, Dialogue, Cinematic FOV ownership or optional Overlay isolation.
- Attach a runtime log and visual confirmation for the tested game/session; offline tests alone do not establish these outcomes.

## Revisit condition

Resume after the active U2 Dialogue correction has been visually validated, or earlier only if the user explicitly reprioritizes this issue.
