# Post-EXIT blend activation observation — results

Date: 2026-11-12  
Status: diagnostic Release artifact prepared; manual runtime observation pending.

## Implemented scope

Added exactly one optional native observation point, after the native start-FOV
store and before later activation/query branches. It is compiled only under
`POST_EXIT_BLEND_DIAGNOSTIC`, resolved from a unique executable-section byte
sequence, and checked by instruction/operand validation. It has no game hash or
fixed RVA gate. Resolution or hook-installation failure disables only this
diagnostic capability.

Each observation receives a session-monotonic candidate epoch and records the
component, receiver, captured start/target, thread and bounded caller evidence.
The epoch is attached to the existing tick observation only after component,
receiver, start and target match. The tick callback's RSI is `T=S+0x28`, while
the new capture callback's RSI is component base `S`; the adapter now converts
`T` back to `S` with an underflow check and reads component fields at `+0x58`,
`+0x54` and `+0x50`. Both addresses remain in the log so the distinction is
visible.

The existing writer observation can carry the most recent pending sample's
epoch when camera and thread match. This is correlation evidence, not strict
causal proof: the current hook is before the setter call, there is no
post-setter readback, and intervening native writes are unobserved. The new
point does not establish that initialization completed, that a blend is the
exclusive camera owner, or what final pixels were rendered. A matching later
tick confirms only that compatible captured state reached the active tick path.

## Offline image and test evidence

- Game image: `Stalker2-Win64-Shipping.exe`, SHA-256
  `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- The new byte sequence occurs once in the file's `.text` raw data at offset
  `0x30DAD68`; the runtime resolver validates decoded instructions and
  operands on the mapped executable section before installing the hook.
- Focused `runtime_native_adapter_harness` `diagnostic-work on` and `off` both
  passed with process cleanup verified. The ON case asserts candidate capture,
  matched tick, EXIT caller evidence and epoch propagation into the existing
  writer record. It also exercises resolver ambiguity/operand rejection,
  frame-slot bounds, re-arm, two interleaved contexts, unrelated receiver,
  generation retirement, unknown caller and capacity exhaustion. OFF confirms
  the added observation path performs no reads or records.
- `test.cmd -NonGraphics`: registry audit and harness build passed; its
  canonical runner stopped at `config_persistence_harness` (test 10/62) because
  the hardlink fixture setup returned Win32 error 5. Nine preceding scenarios
  passed. The modified native adapter was then rebuilt and run directly in both
  focused modes; this does not convert the full canonical suite to PASS.
- Separate Release profile builds produced internal artifacts. The production
  ASI `.text` SHA-256 is identical to the existing project ASI's `.text`
  (`4E6C96426A039DF9B90DDC68FF28E41EAF2475983BD8C870BFDCDA540FC60DBA`), and
  the production image contains no new observer marker. The complete binary
  hashes differ because the newly built artifact includes current build
  metadata/resources; no claim of whole-file identity is made.
- Diagnostic Release ASI: 3,660,288 bytes, SHA-256
  `58DB7E9E98107D89050ECDED277C902CD2553645CA03541B9F557FB392158825`.
  Artifact: `build-artifacts/cm/post-exit-observation/diagnostic-tOFF-dOFF/bin/STALKER2CameraTweaksDiagnostic.asi`.
  It was built in the isolated profile tree and was not installed. Previous
  ASIs and the published project-root ASI were left untouched.
- No game, WinDbg, WARP or graphics fixture was launched. Test/build child
  processes were checked after runs; none remained.

## Manual runtime protocol

The user can choose whether and when to perform this run. Keep the current
installed ASI backed up and use only the isolated diagnostic artifact above.
Enable `[Diagnostics] Enabled=true`, launch the game normally, and reproduce
the same post-cinematic weapon handoff that showed the FOV discontinuity. Keep
the corresponding runtime log. Do not change recovery settings for this
observation.

Check for this bounded sequence:

1. `POST_EXIT_BLEND_START_CAPTURE` with `disposition=candidate`, epoch,
   component/receiver, start/target, and caller disposition.
2. `POST_EXIT_BLEND_SAMPLE` with the same epoch and
   `activation_match=matched_tick`, including both tick context and normalized
   component base.
3. `POST_EXIT_BLEND_WRITER` carrying that epoch, camera and thread evidence.

The result can establish the observed candidate-to-tick match and the existing
same-camera/thread writer correlation for that run. It cannot establish
post-setter state, exclusive native blend ownership, exact producer-to-writer
causality, or rendered output. If those are still needed to select recovery
math, an additional observation point would require a separate scope decision.
