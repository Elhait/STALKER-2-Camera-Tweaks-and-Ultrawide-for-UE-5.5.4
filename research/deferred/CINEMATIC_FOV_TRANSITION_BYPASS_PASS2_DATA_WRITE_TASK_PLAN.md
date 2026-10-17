# CINEMATIC_FOV_TRANSITION_BYPASS_PASS2_DATA_WRITE

## Objective

Capture the physical writes to the confirmed active
`CameraComponent + 0x230` during the final cinematic EXIT recovery.

## Established evidence and current state

- The combined atomic candidate is stable and remains the control build.
- Pass 1 confirmed the asymmetric lifecycle: ENTER supplies transformed FOV
  immediately, while EXIT supplies target 90 and a separate per-frame recovery
  stream follows.
- The known writer consumes the interpolated `+0x230` value; its RIP is not the
  physical write owner.
- Previous blind scalar-store probes did not hit the active recovery object.

## Approved scope

- One data-write trace on the confirmed live source address `source + 0x230`.
- Final `CinematicExiting` window only: first downward write and a few following
  writes.
- Record write RIP/RVA, caller, thread, old/new value, source and elapsed time
  from final EXIT.

## Explicit non-goals

- No ASI writes, bypass, clamp, timer or new production hook.
- No renewed consumer audit or broad scalar-store scan.
- No production or release changes.

## Expected files or areas

- Debugger/runtime session or equivalent data-breakpoint instrumentation.
- Optional research notes/evidence only after a trace is captured.

## Implementation batches

### Batch 1 — environment readiness

Confirm an available debugger capable of a hardware data breakpoint and a
running game process with the confirmed source address.

### Batch 2 — bounded data trace

Arm the breakpoint only for `source + 0x230`, capture the first downward write
and several subsequent writes, then remove the breakpoint and preserve the
session state.

### Batch 3 — static owner audit

After a stable write RIP is found, inspect only its owning function and direct
callers for transition state.

## Current validation and blocker

- No x64dbg/WinDbg command or debugger window is available in the current
  session.
- No data breakpoint has been armed.
- The trace is therefore blocked before runtime execution; no ASI substitute is
  justified.

## Risks and safe failure

- Use a hardware data breakpoint rather than PAGE_GUARD/VEH instrumentation.
- Limit the watch to the confirmed object and remove it after the bounded
  capture.
- Keep the combined candidate as the rollback/control path.

## Stop conditions

- Do not proceed without a real debugger/data-breakpoint mechanism.
- Do not infer the physical writer from the consumer or known writer RIP.
- Do not broaden to all `+0x230` stores without a new anchor.

## Expected final Git review

No source/build changes are authorized for this blocked environment pass.
Record only the concrete tool availability and runtime limitation.
