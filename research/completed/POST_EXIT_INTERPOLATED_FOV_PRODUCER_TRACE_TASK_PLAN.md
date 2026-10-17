# POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE

## Objective

Identify the immediate caller/producer path that supplies the already
interpolated FOV to the validated camera writer during final
`CinematicExiting` recovery on Steam 2.0.5.

## Established evidence

- The direct store candidate at `0x3DB2CE7` installed successfully but produced
  zero runtime hits and is rejected as the recovery writer.
- Four additional direct `+0x230` candidates were statically audited and did
  not expose a justified live per-frame recovery path.
- The validated camera writer observes `XMM0 == CameraComponent +0x230` across
  the complete `126.87 -> 90` recovery stream.

## Approved scope

- Research-only instrumentation using the existing validated camera-writer
  hook.
- Final post-EXIT `CinematicExiting` sequence only.
- Add immediate return address/caller and compact producer correlation data to
  the existing raw writer observation.
- Preserve all production behavior when the compile-time flag is absent.

## Explicit non-goals

- No new guessed native hook or direct-store enumeration.
- No timers, clamps, writes, FOV transforms or behavior changes.
- No changes to gameplay, dialogue, cinematic ENTER or EXIT behavior.
- No stack walking, page guards, VEH or broad camera tracing.

## Expected files

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- One research build script and artifact under `build-artifacts/research/`

## Batches

### Batch 1 — writer producer telemetry

- Read the return address at the existing writer entry.
- Log writer RIP, return address, source camera, XMM0, `+0x230`, elapsed time,
  coordinator and thread ID while the final post-EXIT trace is armed.

Validation: source review and compile-time gated build.

### Batch 2 — user runtime validation

- Run one clean final cinematic EXIT with only this research build active.
- Correlate caller identity with the `126.87 -> 90` stream.

Validation: exact user-supplied runtime log and 2.0.5 executable identity.

Runtime result: final `seq=4` confirmed the complete `126.87 -> 90` stream at
the known writer on the same CameraComponent source. The attempted return
address values were invalid (`0x0`, `0x8`, `0x18D000017A2B`), so immediate
caller provenance remains unresolved. Durable report:
`02-Research/reports/POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE_205.md`.

Disposition: writer-side producer correlation passed; caller capture via the
current RSP read is invalid and must not be interpreted.

## Risks and rollback

The existing validated writer hook remains the only execution point. The new
code is observation-only and compile-time gated. Disable the research flag or
remove the research artifact to roll back; the stable ASI is not overwritten.

## Stop conditions

- If no stable caller correlates with the stream, defer the producer branch.
- Do not expand into new stores or broad camera tracing from a negative result.

## Final review

Perform read-only Git status/diff review and report completed, remaining,
deferred, blocked and not-runtime-validated items.
