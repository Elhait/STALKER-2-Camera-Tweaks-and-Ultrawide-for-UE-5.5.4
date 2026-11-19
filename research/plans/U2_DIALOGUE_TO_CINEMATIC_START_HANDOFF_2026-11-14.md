# U2 — Dialogue writer result to Cinematic ENTER start — 2026-11-14

Status: bounded offline implementation authorized by the active U2 task. Scope is
Dialogue → Cinematic corrected-start qualification only. Keep the preceding
Dialogue→Cinematic interval distinct from pristine authored/native FOV: a
CameraWriter `NativeRegisterInput` is the input to that writer invocation and may
already reflect a Dialogue callback result.

## Runtime evidence

The 2026-11-14 user run loaded ASI SHA-256
`D206A6AF12882E8EE4C9E50E17D0CCD6211344EBA0EB878DE6983FC4343A66A5`.
At generation2, Cinematic ENTER observed authored XMM0=90 and the next writer
sample was 80.1509. The latest pre-ENTER writer observation had sequence7155,
same receiver `0x1dcc0e70100`, and input80.1509. The blend activation captured
start80.1509 and target126.87. Corrected start was unavailable; the cinematic
writer reported `PassThroughUnclassified`, `applied=NO`, and unchanged output.
This is writer-level evidence, not rendered-view evidence.

User clarification: values such as 80.2035 are not established as pristine
native Dialogue FOV before correction. Treat them only as values observed at a
specific callback/writer boundary with the source's stated semantics.

## Confirmed source cause

`TraceCinematicEnter` retains only a Gameplay-eligible observation for the
Gameplay baseline path. DialogueLifecycleObserved writer observations are
excluded by that admission flag. `CaptureDialogueCorrectedStartCandidate` then
builds its candidate from ENTER XMM0 (authored90), while the later native blend
activation has a distinct start near80.15. `CinematicEnterActivation::Capture`
rejects that mismatched candidate, so the mapped writer path correctly fails
closed and passes through.

## Change and acceptance

Use a separate, current DialogueLifecycleObserved CameraWriter observation to
seed C0 from its actual writer result, while keeping the Gameplay baseline
observation path unchanged. Admit only when the active Dialogue lifecycle is
correction-eligible and receiver-bound, the observation has matching source,
valid input/result provenance, compatible settings epoch/aspect/flags, and the
later captured blend start matches that observation's writer input. Preserve
activation generation/source/thread/settings admission and the native sample
weight. Any missing or mismatched fact keeps the existing pass-through path.
Diagnostics do not supply authority. No new hook, timer, cooldown, baseline
fallback, or rendered-view claim.

Regression must reproduce the observed shape: Dialogue writer input80.1509,
corrected result~118.56, blend start80.1509,target126.87; first mapped output
must equal the qualified corrected Dialogue result and the terminal output the
cinematic target. Negative cases: wrong writer context/source, invalid result,
aspect/flags/settings mismatch, stale dialogue lifecycle and blend start
mismatch. Verify the normal Gameplay baseline observation remains unchanged.

Files expected: `src/cinematics/cinematic_enter_activation.hpp`,
`src/plugin/runtime.cpp`, `tests/cinematics/cinematic_fov_harness.cpp`, and this
plan plus dated evidence assessment. Run focused U2/camera writer harnesses,
production build and the canonical non-graphics harness if its known fixture
limitation permits. Runtime/visual retest is separate and requires the user's
next game pass; this offline batch does not claim rendered continuity.


## Implementation result — 2026-11-14

The latest log confirms the original candidate used authored ENTER XMM0=90 while
the native blend setter and first writer sample start at80.1509. The earlier
80.2035 value is not classified as pristine original-native Dialogue FOV.
Implementation now obtains C0 from the latest qualified Dialogue writer
observation and requires the later activation to match its input, receiver and
thread. Dialogue-context failure blocks receiver-field or Gameplay-history
fallback.

Focused cinematic FOV, camera writer transaction, Dialogue boundary and
runtime adapter OFF/ON harnesses pass. The non-graphics canonical suite again
stops at invocation10/62 on the Windows hardlink fixture error5 (`occupied=1`)
after registry audit/configure/build/exit-contract PASS. One Production Release
candidate is at
`build-artifacts/u2-dialogue-to-cinematic-start-fix-2026-11-14/production/`;
SHA-256 `7BFF0FEF153392EEE4876CB68D6B80A761B0E4D28D30D9786DF828C03C8BA9EF`.
It is not installed. Runtime and rendered acceptance remain pending.

