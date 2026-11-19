# Cinematic lifecycle continuity repair — offline validation

Date: 2026-11-17. Status: implemented and offline validated; original runtime
repeat pending. This is validation evidence, not task closure.

## Confirmed defect and repair

The original PID 3120 capture is assessed in assessment.md. An activation with
no writer samples incorrectly excluded an otherwise usable Gameplay baseline
from the following ENTER. The native-start adapter also treated a previously
corrected Cinematic sample as fresh native input and applied HorPlus again.

ProcessAcceptedCinematicEnter now consumes continuation carry separately from
GameplayBaselineStore usability. The existing CinematicEnterActivation owner
retains one bounded, admitted writer provenance record through compatible
reentry. The native-start adapter uses that corrected result directly when its
identity is proven. Unknown inherited provenance cannot authorize another
HorPlus transformation. Empty-generation carry does not grant recovery ownership.

Identity checks cover admitted generation/source, receiver/output, thread,
settings epochs, aspect, flags and known component. EXIT association conflicts
discard the witness. Genuine fresh Gameplay context clears predecessor state.
No new manager, hooks, cooldown, scene IDs, modes or correction formulas.

Production files: src/plugin/cinematic_lifecycle_transaction.hpp,
src/cinematics/cinematic_enter_activation.hpp,
src/plugin/camera_writer_transaction.hpp and src/plugin/runtime.cpp.
Architecture rationale is recorded in docs/ARCHITECTURE.md.

## Regression evidence

Both saved red runs failed before the repair: lifecycle high-baseline selection
and actual private runtime-adapter double-HorPlus reproduction (Diagnostics
OFF/ON). See cinematic-continuity-red.json and cinematic-start-red.json.

The existing lifecycle harness now exercises 32 deterministic seeds, each with
23 repeated transitions, including writer-empty generations and admitted writer
samples. Representative paired native/aspect cases are 90 at 16:9, 100 at 21:9
and 130.3115 at 32:9; this is not a full Cartesian matrix. An independent
double-precision tangent-space oracle checks targets. Negative cases include
source/receiver, thread, aspect, flags, settings epoch, component and ownership
conflicts. Tests distinguish predecessor writer input from successor start.

The runtime adapter harness exercises actual production adapters, including
16 repeated EXIT/ENTER cycles for RuntimeCombine at native 130.3115 and 32:9,
with Diagnostics OFF/ON. Existing true Dialogue handoff controls also pass.
The fixture emulates native field/target updates; it is not the Unreal Engine.

Final full test.cmd: 62/62 PASS, 0 FAIL, one explicitly reported hardlink SKIP
(error=5). Runtime adapter: 232 fresh processes. non_graphics=0,
WARP_case_omitted=0. See cinematic-continuity-suite-final.json.
No production or test changes followed that final passing run.

## Release artifact and cleanup

Standard single production Release ASI built successfully; repository-root
artifact and production candidate hashes match.

- Artifact: STALKER2CameraTweaks.asi
- Size: 4,016,128 bytes
- SHA-256: 67F3CFE4351AC3F73FC7C2F1F01C1DA5CE12FF9A1E5292A378ADBE793C19994E
- Build profile: production-tOFF-dOFF; runtime diagnostics remain INI-controlled.
- Build result: exit 0, no timeout, CleanupVerified=true.
- Build supervisor removed its expected residual vctip descendant.
- Final host process check found no compiler, build or harness processes.
- git diff --check passed; only line-ending conversion warnings.
- Game-folder ASI was not replaced.

See cinematic-continuity-release.json. Tests and build ran serially under
bounded external process-tree supervision. No ETW capture was started.

## Remaining runtime check

Repeat the same failing interactive cinematic at in-game FOV 130, with existing
RuntimeCombine / GameplayHorPlus / Reduced selections and diagnostics enabled.
Its embedded reply choice must retain Cinematic ownership. For the unchanged
recorded context, neither a second HorPlus expansion to 166.795 nor a fallback
decline to 126.87 is expected. Inspect the new log and visual result before
moving to another save. No claim yet of rendered continuity, universal engine
writer coverage, native camera lifetime coverage or measured frame-time cost.
