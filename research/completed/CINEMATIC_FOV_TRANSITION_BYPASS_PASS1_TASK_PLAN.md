# CINEMATIC_FOV_TRANSITION_BYPASS_PASS1

## Objective

Instrument the validated combined atomic candidate to observe cinematic ENTER
and EXIT FOV transitions and the known handoff consumer without changing
runtime behavior.

## Established evidence and current state

- The combined atomic candidate is visually stable in gameplay and after
  cinematic EXIT.
- EXIT native FOV recovery is known to interpolate approximately `126.87 -> 90`.
- The interpolation owner and any ENTER-side transition remain unresolved.

## Approved scope

- Build from the combined atomic candidate source/defines.
- Add observation-only logging around cinematic transition boundaries and the
  already validated FOV consumer.
- Log source, FOV, aspect, flags, state pointer fields and timing where safely
  available.
- Preserve all atomic gameplay/cinematic behavior.

## Explicit non-goals

- No writes, suppression, clamping, duration/alpha modification or bypass.
- No new hooks beyond the already validated consumer instrumentation.
- No broad memory scan or renderer/camera-system crawl.
- No production or release changes.

## Expected files or areas

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `build-artifacts/research/build-cinematic-fov-transition-bypass-pass1.cmd`
- One separate trace ASI under `build-artifacts/research`.

## Implementation batches

### Batch 1 — observation instrumentation

Extend the existing bounded consumer trace to capture the transition-relevant
ENTER/EXIT context while preserving the current writer telemetry.

### Batch 2 — build and review

Build the trace candidate, verify its hash, and perform read-only Git review.

### Batch 3 — user runtime validation

Capture one clean cinematic ENTER and final EXIT. Classify whether ENTER is
instant or interpolated and whether consumer state fields correlate with EXIT
progress.

## Risks and safe failure

- Trace failures disable only the optional instrumentation and preserve the
  combined atomic behavior.
- The previously validated combined candidate remains the rollback/control
  artifact.

## Stop conditions

- No writes or behavior changes may be added in Pass 1.
- If the consumer hook cannot be safely extended, retain writer-only telemetry
  and report the limitation.

## Expected final Git review

Compare changed paths with this plan, preserve unrelated dirty work and report
instrumentation/build evidence separately from runtime validation.
