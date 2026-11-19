# U2 Dialogue → Cinematic retest and corrected-start repair

## Runtime evidence and user observation

The archived runtime log is `runtime.log`, 866,482 bytes, SHA-256
`DEF468228D2556563655D4F6BB86A20AE89DDFF36AB60230E5328F1C18C9E6B2`.
It identifies the loaded Release ASI as
`D206A6AF12882E8EE4C9E50E17D0CCD6211344EBA0EB878DE6983FC4343A66A5` and game
image as `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
Runtime settings were Gameplay RuntimeCombine, Cinematics Auto/NativeHorPlus,
Dialogue Reduced and Diagnostics enabled. The user reports the visible FOV bug
returned at Dialogue → Cinematic.

Generation2 Cinematic ENTER observed authored ENTER XMM0=90 and transformed
cinematic target126.87. The pre-ENTER writer capture recorded receiver
`0x1dcc0e70100`, writer input80.1509 and sequence7155. The later native blend
activation recorded start80.1509 and target126.87. Its
`corrected_start_valid=0`. The first CinematicActive writer sample stayed at
80.1509, with `production_mutation=0`; dispatch was `PassThroughUnclassified`
and HorPlus reported `applied=NO`. This confirms a missing corrected-start
mapping at the writer boundary. It does not establish which value the active
view rendered.

Value semantics correction: 80.1509 is recorded as the blend setter's start and
as the input to a specific CameraWriter invocation. A value like 80.2035 from
the earlier capture is a Dialogue callback result in that record, not proven
pristine game-authored Dialogue FOV before correction. Neither value should be
relabeled as such. The corrected start must come from a qualified Dialogue
writer result, not an inferred original-native number or historical Gameplay
baseline.

## Root cause

At accepted ENTER, the current implementation calculated a Dialogue start
candidate from `before`, which is authored ENTER XMM0=90. Later, the blend
setter supplied a different start80.1509. `CinematicEnterActivation::Capture`
correctly rejected the nonmatching candidate, so the writer fell through to
pass-through. Separately, the observation used for Gameplay baseline selection
intentionally rejects non-GameplayEligible Dialogue observations; reusing that
baseline-only optional as the Dialogue start would lose the needed Dialogue
writer result.

## Offline repair

The ENTER path now retains the latest CameraWriter observation independently
from the Gameplay baseline observation. It builds corrected C0 only when the
active Dialogue lifecycle remains correction-eligible and bound to the same
receiver/generation/settings epoch, and the observation identifies the
DialogueLifecycleObserved context, source, thread, writer input/result spaces
and provenance, sequence, aspect, policy aspect and flags. The later setter
activation must match the observed writer input, receiver and thread. The
activation's native eased sample weight then maps the admitted writer samples
from that observed corrected result to the current cinematic target. When
Dialogue context exists but the evidence is absent or mismatched, the existing
fail-closed path remains; current receiver-field and Gameplay-history fallback
cannot take over.

No hook, timer or cooldown was added. The Gameplay baseline admission path is
unchanged. Diagnostics now exposes the selected writer observation sequence,
thread, input, result and aspect for runtime review.

## Offline validation

- `cinematic_fov_harness`: PASS, including the runtime-shaped `80.1509 → 118.563`
  Dialogue writer observation and matching Cinematic blend start.
- `camera_writer_transaction_harness`: PASS.
- `dialogue_boundary_transaction_harness`: PASS.
- `runtime_native_adapter_harness`: PASS, 230 fresh processes with Diagnostics
  OFF and ON.
- `test.cmd -NonGraphics`: registry audit, CMake configure, full harness build
  and runner exit-contract fixtures passed. The canonical registry stopped at
  invocation10/62 because `config_persistence_harness` could not create its
  Windows hardlink fixture (error5, `occupied=1`). This is the same unrelated
  fixture limitation previously observed; the complete registry is not reported
  as passing.
- Production Release ASI linked successfully; the final incremental recheck
  exited0 with process-tree cleanup verified. PE inspection confirms x64,
  PE32+. The artifact was not installed over the game copy.
- Targeted `git diff --check` on tracked files changed in this batch: PASS. Whole `git diff --check HEAD` still reports whitespace in previously staged research transcripts/logs outside this batch; those historical files were left untouched.

Artifact and exact hashes are listed in the adjacent build-artifact
`candidate-manifest.json`.

## Remaining acceptance

Writer continuity is covered offline; rendered-view continuity is still
untested. The user needs to review this exact candidate in the same
Cinematic1 → Dialogue → Cinematic2 scenario. No P0 baseline-admission check needs
to be repeated for this focused U2 start repair. P0's unrelated native
`camera+0x230` origin proof remains runtime-pending.



