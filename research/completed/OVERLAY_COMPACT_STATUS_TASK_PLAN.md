# Compact User-Facing Overlay Statuses — Task Plan

## Objective and evidence

Use the runtime-validated semantic snapshot to replace routine diagnostic rows in Gameplay, Cinematics, and Dialogue with compact user-facing status. Runtime screenshots established menu/no baseline, gameplay/baseline available, active Dialogue, and active Cinematic projections. The repository has unrelated dirty work; preserve it. The expected local style/implementation/testing modules named by AGENTS.md are absent in this repository copy.

## Approved scope

- Presentation-only mapping from existing semantic facts to Active, Disabled, Ready, Pending/Waiting, or Unavailable states.
- Compact section status text and tooltips that explain behavior and when changes apply.
- Keep Camera Integration logic and meaning; only presentation cleanup is allowed.
- Add deterministic status mapping tests and register them with existing test/build entrypoints.
- Update the overlay implementation report and task log; archive this plan after review.

## Invariants and non-goals

- UI presents existing facts only; no new state source, camera logic, hooks, lifecycle changes, persistence changes, or production behavior changes.
- Do not imply AspectRecalculation is reverted by Enabled=false; describe pending/reload semantics only when evidence supports it.
- Cinematic/Dialogue configured selection is not Active selection. Show Active only from observed lifecycle facts.
- Hide normal-state capability/debug rows; retain actionable reasons for unavailable/pending/error states.
- Preserve Camera Integration labels/semantics including Configuration aligned, Cannot assess, mismatch, independent path, and authored fallback. Do not add Camera Runtime panel.
- No game launch, Git state mutation, commit, or release.

## Expected areas and validation

- `src/overlay/feature_presentation.hpp/.cpp`, `src/overlay/renderer_runtime.cpp`
- `tests/overlay/feature_presentation_harness.cpp`, `test.cmd`, `build-overlay-settings.cmd`
- `research/reports/OVERLAY_SEMANTIC_SNAPSHOT.md`, `backlog/TASKLOG.md`
- Run focused harness, `test.cmd`, `build.cmd`, `build-overlay-settings.cmd`, `git diff --check`; inspect scoped diff for presentation-only changes. Runtime is not performed in this batch.

## Risks, rollback, stop conditions, final review

- Risk: status wording could overclaim applied camera effect; fail closed to Cannot assess/Unavailable and never infer current FOV/framing.
- Preserve the existing dirty tree; rollback only this batch scoped edits if tests fail.
- Stop if source changes are needed to camera/settings behavior or semantic snapshot.
- After validation, compare changed paths with this plan and report completed, remaining, deferred, blocked, and not-runtime-validated separately.
