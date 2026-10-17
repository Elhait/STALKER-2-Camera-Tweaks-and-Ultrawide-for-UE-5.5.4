# Gameplay / Cinematics / Dialogue Modular Refactor — Task Plan

**Status:** In progress  
**Created:** 2026-10-02  
**Scope:** Implement the agreed module-independence refactor, `Dialogue.Enabled`, legacy `[Dialogue] Zoom` migration, integrated regressions, offline suite, and production build. Do not launch the game.

## Governing invariant

Shared observation establishes lifecycle and source; the coordinator establishes the current owner; only the active owner's enabled correction may change camera output. Disabling a correction module must not disable observation or transfer ownership.

`RuntimeState` remains the integration owner. This work must not introduce a universal camera framework.

## Baseline and preservation

- The repository already contains staged and unstaged user changes before this task. Preserve both layers; do not reset, revert, stage, or unstage them.
- Preserve the entire `research/` tree unchanged.
- Keep the explicitly excluded risks out of scope: `continuationInterpolation` semantic completion, baseline/source freshness contract, and `Cinematics.Enabled` atomicity.
- Preserve rapid `EXIT → ENTER` behavior and the confirmed native animation hitch behavior.
- No runtime game launch.

## Stages

| Stage | Work | Status | Evidence / notes |
|---|---|---|---|
| 0 | Inspect active rules, project contracts, current worktree/index state, and relevant production/test paths; establish preservation boundaries. | **Complete** | Rules/contracts reviewed. Existing staged and unstaged edits are pre-existing and must be preserved. `research/` is out of scope. |
| 1 | Dialogue config schema and migration: add authoritative `Dialogue.Enabled`; canonical `Mode` values Adaptive/Reduced/Disabled; migrate legacy `Zoom` before defaults; atomic/idempotent write-back; preserve effective settings on failed write-back. | **Complete** | Fresh-object migration/write-back/reload cases added; full final suite remains pending. |
| 2 | Writer dispatch and module ownership: observe raw writer independently; dispatch CinematicActive before Gameplay gates; move cinematic retention/continuation out of Gameplay HorPlus correction; preserve fail-closed source mismatch. | **Complete** | Cinematic dispatch is evaluated before Gameplay enabled/mode gates; focused regressions added. |
| 3 | CinematicExiting: separate shared native-recovery validation/ownership release from Gameplay-specific recovery transforms; keep Dialogue excluded until validated recovery; preserve confirmed Gameplay ON + HorPlus order. | **Complete** | Recovery lifecycle releases ownership with Gameplay correction OFF; existing HorPlus recovery branch retained. |
| 4 | Dialogue.Enabled runtime and UI: preserve lifecycle/ownership observation while OFF; immediate stop of new correction writes on ON→OFF; deferred OFF→ON activation until next accepted start; Mode changes while OFF only persist; effective status; localized Enabled + mode controls. | **Complete** | Runtime activation state and UI effective status implemented; Native removed from the user-facing selector. |
| 5 | Integrated regression coverage and harness isolation: exercise actual shared dispatch/ownership paths and complete requested matrix; ensure each migration case uses a fresh config object and write-back/reload. | **Complete** | Full `test.cmd` suite PASS, including 48/48 runner inventory, 18/18 locale audit, Dialogue fresh-object round trips, module ownership integration, cinematic recovery, and existing regressions. |
| 6 | Full offline suite, `git diff --check`, production build, scoped diff/research preservation review, and final status update in this plan. | **Complete** | `cmd /c test.cmd` PASS; `git diff --check` PASS; production `cmd /c build.cmd` PASS. Production artifact: repository-root `STALKER2CameraTweaks.asi`, SHA-256 `7AB6403CEAE46532590298FAFED8AB6138B4BF22AD7D02A515D1F86147288949`. `git diff --exit-code -- research` returned 0; no game launch. Architecture remains subject to the separate fresh adversarial audit. |

## Acceptance criteria

- `Gameplay.Enabled=false` prevents Gameplay correction but does not block Cinematic-owned retention or validated lifecycle completion.
- `Gameplay.Mode=AspectRecalculation` does not block Cinematic-owned retention.
- Cinematics OFF suppresses Cinematic corrections, not shared lifecycle/ownership observation.
- Dialogue OFF suppresses Dialogue output writes without changing observed lifecycle/owner; Dialogue Mode remains stored.
- Dialogue ON uses Adaptive/Reduced/Disabled only; `Disabled` remains an active policy.
- In-cinematic Dialogue never takes ownership from Cinematics, including when either correction module is disabled.
- Legacy `[Dialogue] Zoom` migration follows the requested precedence, is atomic/idempotent, and preserves effective semantics if write-back fails.
- Required integrated regressions and full offline suite pass; production build succeeds.
- No unrelated worktree layers or `research/` content are changed by this task.

## Excluded work

Do not fix continuation interpolation completion, baseline/source freshness, or settings atomicity. Do not add timers, frame-count/debounce heuristics, alter rapid EXIT→ENTER semantics, change the known native hitch, launch the game, or claim the architecture is fully closed. A fresh adversarial audit is a separate next stage.
