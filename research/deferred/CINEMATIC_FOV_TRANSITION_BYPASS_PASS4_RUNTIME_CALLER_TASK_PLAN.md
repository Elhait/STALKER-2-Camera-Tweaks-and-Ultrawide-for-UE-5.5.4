# CINEMATIC_FOV_TRANSITION_BYPASS_PASS4_RUNTIME_CALLER

## Objective

Capture the raw runtime return address and caller instructions for the exact
physical FOV setter during cinematic ENTER and final EXIT recovery.

## Established evidence

- Current 2.0.5 image identity is validated by SHA-256.
- The live CameraComponent is written by `MOVSS [RCX+0x230], XMM1` at RVA
  `0x205FCC8`.
- Runtime captured `XMM1≈126.828` during recovery and `XMM1=90` at completion.
- The setter body is a standalone store/return function; interpolation is
  upstream and remains unresolved.

## Final status

- Physical setter confirmed at RVA `0x205FCC8`:
  `MOVSS [RCX+0x230], XMM1; RET`.
- The same live `CameraComponent` was captured during gameplay, cinematic
  ENTER (`~126.870`), EXIT recovery (`~126.831`) and final recovery (`90`).
- The raw stack value at `[RSP]` resolves to a `MEM_PRIVATE`
  `PAGE_EXECUTE_READWRITE` hook/trampoline region, not a reliable native
  return address. Its code contains hook stubs, including a known writer
  pattern; ownership is not attributed to a specific module.
- The additional stack value `0x7ff6d3c5206a` is native executable code but
  its inspected context is AK/plugin-list handling, not the FOV path.
- Native interpolation owner, caller, duration, alpha and completion state
  remain unresolved.
- Pass 4 result: `PARTIAL PASS — setter and recovery writes confirmed; native
  caller not recovered`. Native FOV bypass remains deferred.

## Approved scope

- One WinDbg hardware data-breakpoint session on the current live
  `source + 0x230`.
- Capture raw `RIP`, `RSP`, `[RSP]`, `RCX`, `XMM1`, and disassembly around the
  raw return address for one ENTER and one final EXIT recovery hit.
- Compare the caller/callsite and the instructions preparing `XMM1`.

## Explicit non-goals

- No production ASI changes or new runtime hooks.
- No writes, clamps, timer guards or FOV bypass.
- No broad breakpoint scan or renewed Ghidra callsite enumeration.
- Do not infer caller identity from Shipping symbol names or `kv` alone.

## Validation and stop conditions

- ENTER capture is useful but not required if it causes excessive noise.
- PASS requires a final EXIT hit with `XMM1` in the recovery range and a raw
  return address that disassembles in the game image.
- If the raw return address is invalid or unstable, retain only the confirmed
  setter evidence and stop the caller-capture branch.
- Do not treat arbitrary stack values as caller evidence without a validated
  call/return relationship and relevant FOV context.

## Risks and rollback

- Disable/clear the hardware breakpoint after capture.
- Keep the stable combined candidate as the only behavior control.
- Do not use PAGE_GUARD/VEH instrumentation.

## Expected final review

Record the captured addresses and caller classification in research evidence;
leave production source and release assets untouched. Mark the native bypass
research as deferred unless a new independent runtime or static anchor appears.
