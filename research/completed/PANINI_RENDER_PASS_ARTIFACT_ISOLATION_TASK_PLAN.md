# Panini Render-Pass Artifact Isolation — Task Plan

Status: Bounded isolation pass complete — no tested CVar isolated the
artifact; responsible render pass remains unknown.

## Objective

Identify whether the detached/duplicated tree-like shadow seen when Panini is
enabled is produced by a specific shadow, temporal, screen-space, depth,
normal or lighting contribution that does not share the Panini warp.

This task investigates the artifact only. It does not implement Panini,
change the ASI or alter production defaults.

## Established evidence and current state

- Shipping STALKER 2 / UE 5.5.4 exposes and executes
  `r.LensDistortion.Panini.D`.
- The artifact is absent at `D=0` and present at every tested positive `D`.
- `r.LensDistortion.Panini.S` from `0` to `1` does not remove it.
- `r.LensDistortion.Panini.ScreenFit` from `0` to `10` has no observable
  effect in the tested scene.
- `r.Shadow.Denoiser 0` does not remove it.
- The likely class is a render-pass or auxiliary-buffer mismatch, but the
  specific contribution is not identified.

## Recorded isolation result

- With `r.LensDistortion.Panini.D 0.25`, setting `r.ContactShadows 0` produced
  no visible change to the detached/duplicated tree artifact.
- Contact shadows are therefore `NOT CAUSAL IN TESTED SCENE` within this
  bounded comparison. This does not exclude other shadow, temporal or
  screen-space contributions.
- With `r.LensDistortion.Panini.D 0.25`, setting
  `r.TemporalAA.Upsampling 0` produced no visible change to the artifact.
- Temporal upsampling is therefore `NOT CAUSAL IN TESTED SCENE` within this
  bounded comparison. This does not exclude other temporal history or
  upscaler paths.

- `r.Lumen.ScreenProbeGather.ScreenTraces 0` produced no visible change.
- `r.Lumen.Reflections.ScreenTraces 0` produced no visible change.
- Lumen GI and reflection screen-trace paths are therefore `NOT CAUSAL IN
  TESTED SCENE` within this bounded comparison.

Final result: `research/reports/PANINI_RENDER_PASS_ARTIFACT_ISOLATION_RESULT.md`
records the complete bounded isolation pass. No tested candidate changed the
artifact; the responsible pass remains unknown and the CVar-only branch is
closed.

## Approved scope

- Fixed 32:9 resolution and one repeatable cinematic scene.
- Keep `r.LensDistortion.Panini.D 0.25` as the Panini control.
- Change one candidate rendering contribution at a time.
- Test only a small, hypothesis-driven set of categories:
  shadow/contact-shadow contribution, temporal/upscaling history and
  screen-space/depth-dependent lighting or reflection contribution.
- Record whether the detached tree disappears, changes, or remains unchanged.
- Restore every changed CVar immediately after its A/B comparison.

## Explicit non-goals

- No random CVar search or broad renderer sweep.
- No ASI/source/INI/build/release changes.
- No renderer hook, shader modification or post-process replacement.
- No Panini `D/S/ScreenFit` tuning; that branch is closed.
- No production recommendation from one isolated positive result.

## Batch 1 — Controlled isolation matrix

Status: `PENDING USER RUN`.

For every candidate:

1. Establish baseline with `D=0`.
2. Enable `D=0.25` and record the artifact.
3. Change one candidate CVar only.
4. Compare the same camera moment.
5. Restore the candidate and `D=0` before moving on.

Use the console-enabler and record the exact command, value and result. Start
with the lowest-risk candidate available in the game; if a CVar is unknown or
has no effect, record that fact rather than substituting an unrelated one.

Suggested hypothesis order:

- shadow/contact-shadow contribution;
- temporal accumulation or upscaling history;
- screen-space/depth-dependent lighting or reflection contribution.

### Per-test record

```yaml
candidate:
baseline_D: 0.25
candidate_value:
artifact: absent / changed / unchanged
tree_alignment:
center_blur:
gameplay_affected: YES / NO
restored: YES / NO
```

## Acceptance criteria

- A candidate is considered causal only if the artifact disappears or changes
  reproducibly while the fixed Panini control remains active.
- A candidate with no effect is recorded as negative, not as evidence against
  the broader render-pass class.
- If no candidate changes the artifact, the isolated CVar branch is negative
  and the responsible pass remains unknown.
- Any apparent fix that materially damages gameplay, shadows, temporal
  stability or cinematic EXIT is not production-suitable without a separate
  lifecycle-gated design.

## Risks and safe failure

- Rendering CVars can affect the whole game, performance or image quality.
- A CVar may be unavailable, overridden or effective only in another render
  path.
- Never leave a diagnostic CVar changed after a comparison; restore the prior
  value or restart the game if restoration is uncertain.
- A visual change is correlation only until repeated at the same scene moment.

## Stop conditions and next gate

- Stop after the bounded hypothesis-driven matrix. This stop condition is now
  reached: the tested candidate categories did not isolate a cause.
- Stop if testing becomes a general renderer/CVar search.
- If one candidate is reproducibly causal, prepare a separate feasibility task
  for that pass; do not modify the ASI here.
- If no candidate is causal, defer the render-pass root cause and keep current
  Full Hor+ as the production baseline.

## Expected final review

- Record exact user-run commands, scene/resolution and observations.
- Separate confirmed effects from hypotheses and unavailable CVars.
- Confirm no production files or release artifacts changed.
- No commit, tag or push is part of this task.
