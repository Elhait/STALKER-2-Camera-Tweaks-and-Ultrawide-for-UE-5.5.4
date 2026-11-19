# Post-EXIT bounded blend observation

Date: 2026-11-11. Scope: diagnostic-only observation through existing native hook points. Camera recovery/math are unchanged. This is an observation artifact, not a fix or a completed native-ownership investigation.

## What changed

- `CMakeLists.txt`: the diagnostic profile defines `POST_EXIT_BLEND_DIAGNOSTIC`; the production profile excludes the added observer code/state.
- `src/diagnostics/post_exit_blend_observation.hpp`: fixed-storage episode/pair/sample tracking, current-camera/current-thread writer correlation, and a session-wide record budget. A writer consumes all superseded pending candidates for its camera/thread, so a later writer cannot fall back to an older candidate.
- `src/plugin/runtime.cpp`: accepted EXIT arms an observation episode; the existing Dialogue boundary captures raw values and its current receiver; the existing writer callback records its own field/input and final callback register. All access is under the existing correction gate. No new worker, lock, timer, native hook point or camera write was added by the observer.
- Diagnostic startup can use the same existing boundary when Dialogue policy is disabled. The observation-only path bypasses Dialogue state publication/correction until the user activates a real policy; subsequent policy changes use the ordinary existing callback/reset path. Observation does not grant Dialogue capability or mark disabled policy as available.
- Focused model and actual-adapter checks were added to existing harnesses, with registry dependency updates. No camera policy transaction, SafetyHook, Overlay, input hook, startup toast or INI behavior was edited.

## Exactly what a record means

| Record/data | Observed boundary | What is not established |
|---|---|---|
| `POST_EXIT_BLEND_ARM` | Accepted mod EXIT callback, before the native call | That the following blend was armed by this EXIT |
| `POST_EXIT_BLEND_SAMPLE` | Existing blend callsite, before camera setter executes | Native cleanup or actual post-setter camera field |
| `context`, `receiver`, `vtable` | Tick RSI, call receiver RCX and current receiver vtable | Persistent UObject/allocation identity or absence of address reuse |
| `raw_xmm6`, `raw_xmm1` | Before any Dialogue policy processing for this callback | An explicit native/transformed FOV-space tag |
| `setter_argument` | XMM1 at callback return, including any ordinary Dialogue modification | That the native setter subsequently returned successfully |
| `start`, `target`, `remaining` | Checked reads at current tick context +0x30/+0x2C/+0x28 | Duration/easing-provider identity, native alpha, or exclusive EXIT arming |
| `receiver_field_before_setter` | Current receiver +0x230 while inside the pre-call hook | Immediate readback after setter |
| `POST_EXIT_BLEND_WRITER` | Subsequent existing writer callback on the same receiver address/thread | Absence of intervening writes/recalculation or causality |
| `writer_field`, `writer_input` | Field at writer callback entry and incoming XMM0 | That the field is the unchanged result of the previous setter |
| `writer_callback_output` | XMM0 at writer callback return | Final displayed pixels or absence of later engine changes |

`input_minus_setter` is a recorded difference, not an admission tolerance or production decision. Rejected/missing reads retain an explicit readable mask and NaN values. Logs label `exit_attribution=unknown`, `sample_space=unknown`, `after_setter=unobserved`, and `intervening_writes=unknown` where appropriate. A same-camera/thread match is a candidate link, not an exclusive execution lineage.

The pre-call mid-hook cannot directly observe the setter's return without a different observation point or altered call routing. This phase does neither. Likewise, no saved instance/provider pointer is dereferenced in a later callback: only current callback pointers are read. These limits are deliberate and must remain visible during log interpretation.

## Compatibility and bounds

The optional layout check requires the existing call opcode plus nearby concrete endpoint-load and countdown instruction sequences, inside a bounded executable span. It has no game SHA/version/RVA gate. Failure disables this observation only, not the pre-existing Dialogue/camera hook. Diagnostics OFF skips its new callback reads and records.

The current image `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479` independently passes those byte-window checks at its already recorded hook; the RVA is evidence for that image, not a runtime compatibility condition.

- Maximum 192 episode/sample/writer/summary records across the runtime session; a new generation/reset does not replenish the budget.
- Maximum four tick-context/receiver pairs in an episode. Further identities are counted as rejected instead of allocating a growing cache.
- Maximum 4096 observed blend/writer callbacks per armed episode. After the cap, the observer skips further reads until another EXIT, subject to the unchanged session budget.
- Samples at power-of-two callback ordinals and the first observed exact target, rather than every frame or arbitrary numeric deltas.
- Capability/startup messages are separately bounded by hook initialization. Existing unrelated diagnostics are not included in the 192-record promise.
- New ENTER, accepted replacement EXIT and controlled reset retire the observation episode. No unsafe process-termination callback or final-log guarantee was added.

## Validation

Evidence: [run artifacts](../evidence/post-exit-blend-observation-2026-11-11/).

Final focused compilation and **21 CPU-only runs PASS**:

- Trace model: unarmed/retired rejection; receiver/thread/pending isolation; fixed pair storage; superseded candidate retirement; 10,000 generations without replenishing session budget; 20,000 invalid callbacks stopping at the work bound.
- Actual adapter, Diagnostics OFF/ON: layout acceptance and rejection/bounds; disabled Dialogue observation without register/camera/Dialogue snapshot changes; unreadable/overflow field handling; writer context neutrality; 10,000 callbacks through the real diagnostic/log sink path.
- Existing related adapter scenarios OFF/ON: RuntimeCombine writer, lifecycle, lifecycle disabled, recovery, foreign-source recovery, Dialogue disabled, Dialogue active, and disabled-in-flight Dialogue. The ordinary diagnostic-work/aggregation checks also pass.
- Real log stress: Diagnostics ON **13 lines / 5,954 bytes** for 10,000 blend callbacks; OFF **0 lines / 0 bytes**. Existing writer aggregation checks remain passing.
- Canonical registry audit PASS: 59 registered targets, 62 runner invocations, 115 native cases / 230 declared child invocations, no missing inputs. This was an audit, not an execution claim for the whole canonical suite.
- Production and diagnostic Release builds PASS, compiled sequentially. No game, debugger, WARP or graphical runtime fixture was run.

An initial adapter attempt failed because the new test called an existing one-shot Setup twice in the same process. The fixture was corrected; the complete focused set above was rerun on the final implementation. A review also found and fixed stale pending-candidate fallback, with its own deterministic regression. Neither issue is presented as a game defect.

All 24 final supervised build/test jobs report exit zero, no timeout and verified cleanup. Recorded PIDs were absent in the final check. Only owned compiler helper descendants were cleaned by the existing supervisor; no unrelated process was terminated.

## Artifacts and preservation

Separate Release diagnostic artifact, file version `3.2.2.0`, product version `3.2.2`, 3,616,256 bytes:

[`candidate/STALKER2CameraTweaks.asi`](../evidence/post-exit-blend-observation-2026-11-11/candidate/STALKER2CameraTweaks.asi)

SHA-256:

```text
9DAF593D9B852EA21307A08BBDC4E6FBF71C9F6938C1158160C7C1C3BD3C277E
```

The root ASI remains `8488120F244D377E9B39BFF8CFF941B6C6C106021B74D8AD75E3D62B5D89FC0C`. Previous production/diagnostic CMake outputs were copied before rebuilding. The rebuilt production output has the same `.text` hash as the preserved production ASI and no `POST_EXIT_BLEND_` string; its whole-file hash differs, so binary equality is not claimed. No installed game file was replaced. `build.cmd` in its usual production profile does not include this observer.

## Manual use and interpretation

1. Manually replace the game's existing ASI with the file from `candidate/`; do not leave both production and diagnostic copies loaded together.
2. Keep Diagnostics enabled. Startup should report `POST_EXIT_BLEND_CAPABILITY layout=validated`. An unavailable layout is an observation failure, not evidence that the blend is absent.
3. Replay the weapon-handoff cutscene and keep playing through the visible FOV jump. If practical, also replay a known working post-EXIT scenario in the same current game build, within the bounded session budget.
4. Save the main log before the next launch overwrites it. Normal exit is sufficient; no debugger or capture program is needed.
5. Compare camera receiver, tick context, raw/final setter argument, native field/input and writer output. Treat the logged candidate relation separately from proven EXIT arming, exact post-setter state or FOV-space provenance.

The artifact addresses the missing receiver/value observations. It cannot by itself close the stricter EXIT-arming and FOV-space questions. Those remain prerequisites for a recovery redesign; no numeric-range expansion, smoothing or endpoint-math decision is authorized by these offline checks.
