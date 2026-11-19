# Post-EXIT blend post-setter observation — implementation and validation

Date: 2026-11-12  
Status: production Release rebuilt with INI-gated observer; one runtime capture analyzed.

## Purpose

This change adds one diagnostic-only observation point immediately after the
validated native camera setter call. Its purpose is to test whether the blend
sample observed before that call is reflected in `camera+0x230` after the call.
It does not change production FOV policy, HorPlus math, Cinematic/Gameplay
recovery decisions, or Dialogue behavior. It does not claim to fix the observed
post-EXIT writer discontinuity.

## What changed

- Added a unique `.text` resolver for the native indirect call. It validates the
  six-byte `CALL [RAX+0x608]`, then decodes the following `UCOMISS XMM6,[RSI+0x2c]`
  contract and places the mid-hook at the continuation. Zero, duplicate,
  truncated, or mismatched candidates fail closed. No game-image hash or fixed
  RVA gate was added.
- Added bounded per-thread LIFO pairing for the pre-call and post-call callbacks.
  A pair must match thread, stack position, component, and a still-active,
  tick-matched activation epoch/generation before the observer reads the carried
  receiver. Overflow, mismatch, or retired activation does not produce a joined
  observation.
- The receiver identity comes from the pre-call frame; it is not independently
  re-derived from volatile registers after the call. The field at
  `receiver+0x230` is read only at the validated post-call continuation.
- Records explicitly label the easing branch, FOV space, native caller,
  same-invocation writer relation, and intervening writes as unknown or
  unestablished where the observer cannot prove them. XMM register values are
  samples only.
- The observer is included in the normal production profile. Its hooks are
  installed only when `[Diagnostics] Enabled=true`; with Diagnostics disabled,
  the adapter confirms no observer reads or records. Installation failure
  disables this optional observation only.

Relevant implementation: [post-setter resolver](../../src/hooks/post_exit_blend_post_setter_resolver.hpp), [bounded pairing](../../src/diagnostics/post_exit_blend_setter_pairing.hpp), and [runtime integration](../../src/plugin/runtime.cpp).

## Static resolver check

Target executable:

`E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe`

SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`

The post-setter pattern was found once in the executable `.text` section, at
raw section offset `0xCCDA73`, and its call and continuation operands matched
the expected contract. This is evidence for the currently inspected executable
only; it is not a fixed runtime address or a compatibility guarantee for future
patches.

## Offline validation

- Focused `runtime_native_adapter_harness diagnostic-work on`: PASS.
- Focused `runtime_native_adapter_harness diagnostic-work off`: PASS. The test
  verifies no observer reads, logging, or pairing work while Diagnostics is
  disabled.
- The ON stress case exercised 10,000 callbacks and produced 13 log lines
  (8,359 bytes), within the configured bound.
- The focused tests cover resolver uniqueness/ambiguity/invalid operands and
  truncation; nested pairing; thread, stack, and component mismatch; overflow;
  activation retirement; and draining an in-flight pair after observation is
  disabled without another memory read or record.
- Canonical `test.cmd -NonGraphics`: registry audit passed for 59 harnesses / 62
  invocations, but the run stopped at invocation 10/62 in
  `config_persistence_harness`: `hardlink_fixture_setup_error=5` and
  `staging_name_ownership_and_hardlinks=FAIL`. The first nine invocations
  passed. The cause of Win32 error 5 was not established in this work. The
  remaining canonical invocations therefore did not run, and the suite is not
  recorded as a full pass.
- The corrected ordinary production Release profile was rebuilt. Its compile
  flags include `POST_EXIT_BLEND_OBSERVATION`; the standard ASI contains the
  observer marker and INI-gated log text. The agent did not launch the game,
  WinDbg, WARP, or graphics fixtures, and did not install an ASI. The subsequent
  user-run runtime capture is documented below. No Git operations were performed.

## Release artifacts and hashes

Earlier production profile (observer excluded; superseded, not for this capture):

`build-artifacts/cm/post-exit-setter-production-20261112/bin/STALKER2CameraTweaks.asi`  
Size: 3,586,560 bytes  
SHA-256: `F50D146FDB3ABF5FE0930DFC7904DB43B8145F286A48A367C4DB1939DAF74C91`

Earlier diagnostic-profile artifact (superseded for runtime capture; not installed):

`build-artifacts/cm/post-exit-setter-diagnostic-20261112/bin/STALKER2CameraTweaksDiagnostic.asi`  
Size: 3,682,816 bytes  
SHA-256: `DB35A0154426F12C2F7833E486C2C3B0DB9D17EA74821B53A68AE09E0CAF4AD3`

The earlier artifacts above are retained as historical outputs. The corrected
production artifact containing the INI-gated observer is listed in the
production-profile correction below. No claim of identity with a previously
supplied or published ASI is made.

## Runtime capture — 2026-11-12

The supplied game log identifies the loaded ASI as
`39422DF9331083D420902004B739D96C4DFC748E6412AC9B5BA1A3AF5BE8E0D9`, matching
the corrected production artifact above. It records `Diagnostics.Enabled=true`,
both blend observer hooks as available, and both resolver capabilities as
validated with one match.

For the relevant `generation=2, activation_epoch=3` transition (`start=70`,
`target=90.3115`):

- Six matched blend samples have six post-setter records. In all six,
  `setter_argument == camera_field_after_setter`, with `field_minus_setter=0`.
  The observed values include `70.0071`, `70.0316`, `70.6007`, `71.7454`,
  `87.4343`, and the terminal `90.3115`.
- Five corresponding `POST_EXIT_BLEND_WRITER` records report writer field and
  writer input equal to the sample; the first four remain pass-through while
  recovery is holding. This is runtime correlation across observer records, not
  proof that the writer call is the same native invocation as the setter.
- At `02:22:06.420`, recovery hands ownership back to Gameplay at writer input
  `90.3088`; `RUNTIME_COMBINE` reports output `127.117`. The terminal native
  setter sample `90.3115` is observed about 1 ms later, followed by another
  Gameplay result `127.119`. This establishes the writer-side policy handoff in
  the log, not the rendered pixel value or a user's visual perception.
- `caller_kind=unmatched_return`; `exit_attribution`, `sample_space`, easing
  branch, and intervening writes remain unknown. The post-setter event correctly
  leaves `writer_same_invocation=not_established`.

This capture confirms the observer's core measurement: on the captured samples,
the setter-return field contains the setter argument. It strengthens the link
between the native blend trajectory and the camera field while preserving the
remaining provenance and causality limits. It does not authorize a recovery
math change by itself.

## What the runtime capture can establish

Use the corrected production artifact listed in the correction note below for
the authorized manual capture, and correlate its bounded events for the same
activation epoch and invocation context:

1. Pre-setter blend sample and captured register values.
2. Post-setter `receiver+0x230` readback at the validated continuation.
3. Subsequent Gameplay writer observation, reported separately.

This can show whether the setter-return field readback equals the observed sample
for the captured invocation and whether later writer samples correlate in time
and receiver metadata. It still cannot, by itself, prove the sample's native vs
transformed FOV space, the active easing selector, exclusive blend ownership,
absence of intervening writes, exact causal identity with a later writer call,
or the final rendered FOV. Those remain separate unknowns unless the capture
provides independent evidence.

## Next step

The requested runtime capture is complete. Preserve this log as the reference
for the `generation=2, epoch=3` sequence. Do not change production recovery based
only on equality between the sample and setter-return field. No artifact was
installed by the agent.

## Production-profile correction — 2026-11-12

The initial implementation incorrectly compiled this observer only into the
separate diagnostic profile. The project contract is that production diagnostics
are selected by `[Diagnostics] Enabled` in the INI. The normal production profile
now includes the observer code and its `HookSet` fields; actual hook installation
and event collection remain runtime-gated by that INI setting. No observer hook
is installed when Diagnostics is disabled.

Corrected ordinary production artifact:

`build-artifacts/cm/production-ini-gated-20261112/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`  
Size: 3,682,816 bytes  
SHA-256: `39422DF9331083D420902004B739D96C4DFC748E6412AC9B5BA1A3AF5BE8E0D9`

The `CMakeCache.txt` identifies this build as `CAMERA_TWEAKS_PROFILE=production`;
the generated compile flags include `POST_EXIT_BLEND_OBSERVATION`. Binary inspection
found the post-setter observer marker and `availability=ini-gated` log text.
Focused adapter cases passed in both modes after the correction:

- ON: 10,000 callbacks, 13 lines / 8,359 bytes.
- OFF: 10,000 callbacks, zero lines / zero diagnostic reads.

Use this standard production ASI for the manual capture, with the INI's
`[Diagnostics] Enabled=true`. The prior `STALKER2CameraTweaksDiagnostic.asi` is
not needed. The agent did not install the artifact.


