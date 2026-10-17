# POST_EXIT_FOV_PRODUCER_STORE_TRACE_205

## Objective

Validate whether the statically identified store at RVA `0x32B779D` is the
physical per-frame producer for the post-EXIT `126.87 -> 90` FOV stream.

## Scope

- Steam 2.0.5 only, with exact SHA-256 gate.
- One SafetyHook observation at `0x32B779D`.
- Log every hit without coordinator filtering: destination, previous FOV,
  incoming XMM0 FOV, elapsed post-EXIT time, armed/coordinator state and thread.
- Research artifact only; stable production ASI remains untouched.

## Non-goals

- No writes, clamps, timers, stack walking or caller inference.
- No additional candidate hooks or broad tracing.
- No gameplay, dialogue, cinematic behavior or writer logic changes.

## Validation

- Compile with a dedicated research flag and the established VS 2022 toolchain.
- Verify artifact hash and source diff.
- User runs one clean final cinematic EXIT with only this research ASI active.

## Stop conditions

- Zero hits: candidate rejected for the tested recovery path.
- Hits on another object/value: classify and stop; do not expand automatically.
- Matching hits on the active CameraComponent and writer stream: candidate
  confirmed; only then audit `FUN_1432B7842`.
