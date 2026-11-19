# Bounded post-EXIT blend observation

Scope: the user's next observation iteration, using existing hooks only. CPU-only compilation/checks explicitly authorized while PC is free. No game, graphics, debugger, installation, Git operation, recovery/math change or new native hook.

## Contract and evidence limits

The [native data-flow investigation](../reports/POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md) identifies T=RSI blend context, camera receiver RCX and raw output XMM6/XMM1 at the existing Dialogue boundary. Existing Cinematic EXIT and writer callbacks provide lifecycle/camera anchors. None observes the return from the camera setter or proves exclusive arming by EXIT.

Add observation only to the diagnostic build profile. Production profile excludes the new code. Never feed a diagnostic match into camera decisions. Record raw/pre-setter and final callback arguments separately. Writer readback is labelled writer-time field observation, never immediate post-setter readback. EXIT attribution remains unknown even when camera and target match. No enum/flag may turn proximity/numeric equality into causal proof.

## Batches

1. Fixed-storage trace model and tests: accepted EXIT arms a bounded episode; track a maximum of four blend/receiver pairs. Sample at bounded callback ordinals, keep aggregate counts and latest values. Match writer by current receiver and thread; report value equality separately. No retained-pointer reads, allocations, clocks, polling or locks inside the model. A session-wide record budget survives episode changes. New ENTER/EXIT/reset retires the episode; state changes cannot reset the session budget.
2. Integrate existing EXIT, Dialogue and writer callbacks behind `POST_EXIT_BLEND_DIAGNOSTIC` defined only for diagnostic profile (and explicit test compilation). Read only current callback pointers via checked arithmetic/SafeRead. Validate the bounded local native layout before interpreting new offsets; failure disables observation only. Dialogue policy enablement must not gate observation: diagnostic build may install the existing boundary for observation even when policy is disabled, while reporting the policy as disabled.
3. CPU-only focused tests including thousands of changing instances/samples and actual adapter checks for unchanged register results. Compile production and diagnostic Release outputs separately without replacing preserved ASI. Hash the diagnostic artifact. Review all touched files and verify owned process cleanup.

## Files

Expected: `src/diagnostics/post_exit_blend_observation.hpp`, `src/plugin/runtime.cpp`, diagnostic profile definition in `CMakeLists.txt`, focused harness/registry additions, this plan and a result report. No gameplay/cinematic/dialogue policy transaction edits.

## Acceptance and outstanding runtime work

- Diagnostic OFF does no new callback memory reads or records.
- Record budget is global to the runtime session; stress with different instances and generations remains bounded.
- Raw XMM6/XMM1, final setter argument, start/target/remaining, receiver/vtable, source field at observation and writer-time field/input/output are distinguished.
- Unknown/unreadable data stays unknown. No direct post-setter observation, native easing-provider state, native cleanup confirmation, or EXIT arming identity is claimed when unobserved.
- Existing native layout checks are independent from game SHA/version/RVA. An unavailable observation contract does not disable existing camera hooks.
- Tests/build are not runtime proof. A separate manual session remains necessary. Its first purpose is camera-instance/value correlation; it cannot by itself prove the missing EXIT-arming path.

Status: bounded observation implementation and CPU-only validation complete; see [result](../reports/POST_EXIT_BLEND_OBSERVATION_2026-11-11.md). Final focused checks: 21/21 PASS; registry audit PASS; production and diagnostic Release builds PASS. The preserved production binary and rebuilt production output have identical `.text` hashes, and the production output has no new trace marker. Root/installed ASI was not replaced. No in-game validation is claimed. Native EXIT arming identity, immediate post-setter readback, provider/easing state and FOV-space classification remain explicit follow-up evidence gaps, not implemented capabilities.
