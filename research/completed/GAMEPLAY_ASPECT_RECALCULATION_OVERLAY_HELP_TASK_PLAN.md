# Task Plan: Explain AspectRecalculation Persistence in Overlay

## Objective

Add a concise user-facing overlay explanation that AspectRecalculation uses a
native game camera recalculation whose applied result may persist after
Gameplay is disabled and may require reloading the game or save for a full
reset.

## Established Evidence and Current State

- HorPlus and AspectRecalculation have different runtime semantics.
- Source/runtime evidence does not establish a safe reversible native
  AspectRecalculation transition.
- The overlay already exposes Gameplay Enabled and Mode controls.
- The explanation must describe observed/established behavior without claiming
  reload is the only possible camera lifecycle reset.

## Approved Scope

- Add a short tooltip/help description adjacent to the Gameplay Mode control in
  the overlay settings UI.
- Wording must state that HorPlus responds live, while an applied
  AspectRecalculation result may persist after disabling and may require a game
  or save reload for a full reset.

## Explicit Non-Goals

- No camera/gameplay behavior changes.
- No changes to Runtime Settings API, hotkeys, persistence, or mode transitions.
- No claim that reload is the only possible reset mechanism.
- No game launch, runtime validation, commit, release, or unrelated cleanup.

## Expected Files / Areas

- `src/overlay/renderer_runtime.cpp`
- This task plan, to be archived under `research/completed/` after validation.

## Batches

1. Add the bounded tooltip text next to the Gameplay Mode control.
2. Review the diff and run the relevant deterministic test/build validation.
3. Archive this plan and perform final read-only Git/diff review.

## Validation

- `test.cmd`
- `build-overlay-settings.cmd`
- `git diff --check`
- Read-only review confirming no production camera/runtime behavior changed.

## Risks and Rollback / Safe-Failure

- Risk: overstating reversibility or implying a guaranteed reset boundary.
- Mitigation: use qualified wording (“may persist”, “may require”) and describe
  reload as a practical full-reset path, not the only possible one.
- Rollback: remove only the added tooltip text if it is inaccurate or renders
  poorly; no runtime state is affected by this UI-only change.

## Stop Conditions and Phase Gates

- Stop if the UI cannot present the explanation without changing runtime
  behavior or expanding into a settings redesign.
- Stop after the bounded text change, validation, plan archival, and final
  review. Do not launch the game.

## Expected Final Git Review

- Confirm only the planned overlay source change and this archived plan are
  attributable to this task.
- Preserve all pre-existing dirty and untracked user work.
- Report validation results and state clearly that runtime was not performed.
