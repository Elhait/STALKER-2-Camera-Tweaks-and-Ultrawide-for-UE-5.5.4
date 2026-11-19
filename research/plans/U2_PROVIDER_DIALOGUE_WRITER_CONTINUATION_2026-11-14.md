# U2 provider Dialogue endpoint continuity — bounded repair plan

## Objective

Remove the confirmed writer discontinuity at the completed post-EXIT provider → Reduced Dialogue endpoint. Preserve the single ASI, current camera policy selection, baseline quarantine and ordinary Gameplay HorPlus behavior.

## Evidence and contract

The 2026-11-14 runtime log at ../evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/ shows the Gameplay context detector resetting the provider-started Dialogue lifecycle after callback output 80.2035. The next native writer sample 70.0357 then produces 108.977, after the prior writer produced 118.602. Baseline remained 90.3115 / 127.119.

The fix is authorized only for a consumed provider activation with matching receiver, thread, cinematic generation and Dialogue settings epoch, while the active Dialogue candidate remains bound to that receiver. First writer sample requires one-shot equality with the provider-start callback output. Later native samples map through the captured Dialogue destination and then the normal Gameplay HorPlus policy. No cooldown and no baseline substitution.

## Files

- src/dialogue/post_exit_provider_handoff.hpp
- src/plugin/camera_writer_transaction.hpp
- src/plugin/runtime.cpp
- 	ests/plugin/camera_writer_transaction_harness.cpp
- esearch/evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/

## Acceptance and validation

- Deterministic regression for seed sample 80.2035 followed by native endpoint 70.0357; no Dialogue reset and equal policy outputs.
- Existing baseline projection remains RETAIN through Dialogue-owned samples.
- Ordinary Gameplay remains eligible and uses existing HorPlus behavior.
- Run focused Dialogue/camera-writer harnesses, canonical non-graphics test registry, and one Release build. Keep all validation bounded and confirm owned process cleanup.
- Runtime/visual acceptance remains pending until the user exercises the new ASI. Do not ask for the missed control Dialogue again.

## Non-goals and limits

No new hooks, global timer, generic camera ownership inference, change to Cinematic→Dialogue provider trajectory, or claim of rendered continuity based solely on writer outputs. The independent Dialogue→Cinematic path remains governed by its existing corrected-start evidence and is not modified here.
