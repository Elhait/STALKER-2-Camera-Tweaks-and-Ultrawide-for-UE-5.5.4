# Dialogue Candidate termination — P0 validation

Date: 2026-11-17. Implemented; offline validation complete, new runtime capture
pending. This report does not close the broader lifecycle/context isolation task.

## Priority and contract

User clarification makes ordinary Dialogue/Cinematic lifecycle termination P0.
Save loading exposed the defect; it is not a prerequisite or the only trigger.
The previous audit's world-identity evidence gap must not defer a repair that
can be justified by existing Gameplay lifecycle evidence.

A neutral setter callback after recovery may create a pending Candidate even
though it contains no accepted Dialogue descent. The writer can now terminate
that hypothesis before classification, without another setter callback, when:

- Coordinator is Gameplay, with no exclusion or protected provider endpoint.
- Candidate has a validated receiver binding at the current Dialogue settings
  epoch and cinematic generation, matching this actual writer's receiver.
- The existing production Gameplay baseline is valid, current, usable, from
  that receiver, and has a nonzero admitted observation sequence.
- Candidate start, target, previous sample and writer native input remain at
  that reference within the existing strict recovery endpoint tolerance.
- Current writer aspect agrees with the reference; no accepted active policy
  or provider-owned Dialogue is present.

This uses source/provenance evidence before numeric consistency checks. It
does not infer identity from FOV, create a new baseline, revive a stale one,
expire by time/callback count, reset on every receiver change or add a manager.
Unknown/foreign ownership, real descent, active Dialogue and Cinematic/recovery
retain their existing protection. The source-bound correction formulas, user
modes, configuration and native hooks are unchanged.

Implementation: CandidateTracker::IsStationaryAtReference in dialogue_state.*;
RetireStationaryDialogueCandidateOnGameplayWriter in dialogue_boundary_transaction.hpp;
actual runtime writer invocation before ClassifyDialogueWriterContext. Existing
ResetDialogueBoundaryState owns termination. Diagnostic retirement is emitted
after the correction gate and cannot change the decision.

## Reproduction and tests

The runtime adapter red case failed before the production repair with:
"Stationary Candidate survived a proven Gameplay return without more setters".
It first admitted native 130.308, then invoked the actual setter adapter with
130.311 target/start and validated receiver identity. It deliberately supplied
no more setters before the real writer adapter. After repair it also verifies
fresh admission on receiver B and the following GameplayHorPlus target rather
than authored-90 fallback. See lifecycle-termination-red.json.

The same registered case now continues in one process with no save-load or
manual lifecycle reset: repeated Cinematic -> embedded choice -> short reentry
-> Gameplay -> ordinary Dialogue -> Gameplay -> new stationary callback ->
writer completion, with native FOV 90.3115, 100.3115 and 130.3115. It checks
released policy/owner/exclusion, usable reference and later Cinematic target.
The 32:9 Cinematic targets use an independent double tangent-space oracle.
Engine native field updates are emulated in the fixture. Dialogue target
formula tests remain separate; this case checks actual policy execution.

Dialogue boundary harness adds one positive and 20 negative controls for
foreign/unvalidated receiver, stale/missing reference evidence, changed epoch
or generation, exclusion/provider, active policy/Dialogue, real target descent,
different input/aspect, non-Gameplay coordinator and invalid observations.

Existing true provider Cinematic -> Reduced -> Cinematic regressions and
writer-empty generation tests remain in the full suite. They were not replaced
by the ordinary Dialogue sequence above.

Final test.cmd: 62/62 PASS, zero FAIL, one explicit hardlink SKIP (error=5).
Runtime adapter: 234 fresh child processes, Diagnostics OFF/ON, including the
new case in both modes. non_graphics=0; WARP_case_omitted=0. Cleanup verified.

During integration, the extended Dialogue harness required the production
Gameplay baseline and telemetry translation units; its link dependencies were
added to the canonical registry. Registry audit also exposed stale scenario
metadata: it is now aligned with the actual case array, and future audits
compare exact scenario sets and derive the OFF/ON child count rather than
accepting a matching hard-coded total. No executed scenarios were removed.

## Boundaries and remaining work

This repairs the demonstrated stationary post-recovery Candidate path. It is
not a new general rule to discard a genuine pending descent or active Dialogue
when setter callbacks stop, and does not prove every possible termination path.
The broader audit retains separate dispositions for these states and load-
during-active-Cinematic/Dialogue or same-address reload evidence. No fake LoadGame
event, automatic Gameplay-before-Cinematic requirement or global epoch was added.

The captured game's receiver validation result was not separately recorded;
the new adapter test explicitly supplies the validation required by production.
The next capture must confirm the retirement event and next scene's baseline
selection; offline tests are not rendered-view or universal engine proof.

Artifact identity, final build result and host cleanup are recorded in the
evidence directory research/evidence/lifecycle-termination-2026-11-17/.
No game-folder ASI is installed by this task.

Standard production Release built successfully, exit 0, no timeout, cleanup
verified. Root/candidate SHA-256 both:
3D30F28619EFFA64DAAF9256A683BB19C6C2BB8C6760A3113F733B3B368F2262.
Final host check found no compiler, CMake/Ninja, vctip or harness processes.
git diff --check passed with only line-ending conversion warnings.
