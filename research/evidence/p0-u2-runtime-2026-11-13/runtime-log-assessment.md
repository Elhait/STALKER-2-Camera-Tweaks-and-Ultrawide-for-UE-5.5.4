# P0 + U2 runtime log assessment — 2026-11-13

Artifact identity
- Candidate ASI hash matches `candidate-manifest.json`: `2D952B287FB9DB55A7FA789F767F5CA9E6395EF1D41F0BB6EDA5BF7C3D23295A`.
- Game image SHA-256 observed in startup record: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Supplied log SHA-256: `392182681FDA7BD8EA1E6BAFACBD26521EE40D893C3478DE736C698C6A5B5B10`; 3,884 lines, 1,368,868 bytes.
- Compact relevant records are retained in `focused-runtime-records.txt`.

Confirmed observations
- Both cinematic consumer captures reached `InitializerObservationReached`; both explicitly report `native_start_commit=unverified`. They do not prove initializer commit or rendered view consumption.
- U2 shadow capability resolved the provider call path and observed operand 70, but mapping is `conditional_affine_only`; branch and rendered start are unobserved.
- The post-exit Dialogue provider handoff was applied on generation 1, same thread/receiver, with Reduced policy. The logged output was 125.853 for sample 125.836 and target 108.941. This establishes writer transaction output only, not active-view FOV.
- Baseline started at 90.3115. Immediately after the post-exit transition, three updates admitted 70.0856, 70.0243 and 70.0002 as `GameplayEligible`; later it recovered to 90.3038. The distinct ordinary Gameplay control later admitted 90.3019 on a different source. Thus the capture confirms a transient baseline contamination during recovery/transition and confirms normal Gameplay re-admission in a control. The first control window's baseline update also occurred after contamination; it does not establish that contamination was harmless.
- Dialogue callback provenance at the first contaminated sample reports coordinator Gameplay, exclusion inactive, Dialogue phase Inactive and `HandledProviderReset`; its callback RSI differs from latest writer RSI, and engine identity link remains unproven.

Root cause
- At 23:21:08.918 the provider handoff reached its native target: input 70.1746 is within max(0.02, 0.25) degrees of target 70. `TryMapPostExitProviderDialogueSample` marked `postExitProviderRecoveryCompleted`, set coordinator-after to Gameplay, and cleared recovery state. Runtime logged the shared `recoveryRetired` result as a writer-RSI change, which produced the contradictory event despite unchanged RSI/RBX.
- Subsequent calls no longer had recovery context and were classified `GameplayEligible`; the completed provider record was only used at the Dialogue callback to suppress repeated Dialogue admission. This is why ~70 replaced the baseline.

Offline fix and validation
- A completed provider activation now keeps matching samples near its native target baseline-ambiguous when receiver, thread, cinematic generation and settings epoch match. Admission resumes outside that endpoint neighborhood. Gameplay HorPlus still runs during the quarantine.
- Recovery diagnostics now distinguish provider-endpoint completion from actual writer-RSI retirement.
- Regressions cover identity mismatches, endpoint bounds, ambiguous-context baseline retention, continued Gameplay HorPlus correction, and subsequent baseline admission.
- Canonical full `test.cmd`: PASS (59 targets, 62 runner invocations, graphics included; cleanup verified). The initial sandbox run was blocked by the NTFS hardlink fixture; the same canonical run passed on the authorized out-of-sandbox run.
- Fresh Production Release ASI: `build-artifacts/p0-u2-baseline-admission-fix-2026-11-13/STALKER2CameraTweaks.asi`; hash and build details are in `candidate-baseline-admission-fix.json`. Not installed or run in-game.

Status
- The specific post-exit endpoint admission defect is fixed offline and requires repeat runtime verification. P0 remains runtime-pending for final producer/def-use of `camera+0x230` and the separate general native-origin proof gap.
- U2 remains design/shadow-evidence only. Neither transition's active-view continuity nor visual result is established by this log. No production mapping is justified because corrected start, destination baseline and rendered consumer are still unknown.
- No U2 transition-mapping policy changed. The Release candidate contains the bounded P0 endpoint quarantine and corrected recovery diagnostics.
