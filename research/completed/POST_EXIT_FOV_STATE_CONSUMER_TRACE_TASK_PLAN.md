# POST_EXIT_FOV_STATE_CONSUMER_TRACE

## Objective

Instrument the validated Steam 2.0.5 post-EXIT FOV handoff consumer to identify whether `FUN_14318DCD4` receives the observed `126.87 -> 90` stream and to record its immediate caller/state context.

## Established evidence

- Steam 2.0.5 executable identity is validated by SHA-256, `.text` size and image base.
- Integrated production telemetry confirmed a roughly one-second post-EXIT writer stream from `126.87` to `90`.
- `FUN_14318D9BA` forwards the changing float to `FUN_14318DCD4` when its state value differs.
- `FUN_14318DCD4` writes state `+0x54` and synchronizes PCM/CameraComponent-related fields, but static analysis did not show interpolation arithmetic.

## Approved scope

- Research-only instrumentation build based on the current production source.
- Observe `FUN_14318DCD4` entry during the final post-EXIT handoff.
- Log elapsed time from the final EXIT, incoming FOV, state pointer, state `+0x50/+0x54/+0x58` before and after, immediate return address if safely available, and thread id.
- Preserve existing bounded change-only writer telemetry for correlation.

## Explicit non-goals

- No gameplay, dialogue or cinematic behavior changes.
- No FOV writes, clamps, timers controlling behavior, frame counters or suppression guards.
- No full stack capture, broad native tracing, new virtual calls or guessed pointers.
- No changes to the stable release ASI or release archive.

## Batches and validation

1. Add the observation-only trace at the validated consumer boundary and build a separate research ASI.
2. Verify source compilation/build success and hash the research artifact.
3. User runs one clean final cinematic EXIT with production transform enabled; compare consumer input with writer samples.

## Risks and safe failure

- The function target and calling convention are version-specific; enforce the 2.0.5 identity gate and fail closed on resolution ambiguity.
- State pointers may be invalid outside the validated path; read only after null/readability checks and only while the final post-EXIT trace is armed.
- If instrumentation cannot be installed safely, do not create a runtime probe.

## Stop conditions

- Success: one immediate upstream caller is correlated with the changing incoming FOV values.
- Pivot: `FUN_14318DCD4` receives no changing stream, or target/calling convention cannot be validated.
- Stop after this consumer boundary; any upstream static audit requires a new bounded approval.

## Final review

- Inspect research build output and runtime limits.
- Perform read-only Git status/diff review against this plan.
- Record completed, remaining, deferred, blocked and not-runtime-validated items before reporting.
