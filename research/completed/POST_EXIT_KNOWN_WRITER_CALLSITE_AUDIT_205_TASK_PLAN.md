# POST_EXIT_KNOWN_WRITER_CALLSITE_AUDIT_205

## Objective

Identify the containing function and direct callsites of the validated 2.0.5
camera writer at RVA `0xA9F0FB`, and classify whether any caller forms FOV in
`XMM0` immediately before the call.

## Established evidence

- Runtime confirms the writer consumes the complete post-EXIT `126.87 -> 90`
  interpolated stream.
- Stack-derived return values at the current hook placement are invalid and
  are discarded.
- Direct `+0x230` store-owner search did not identify the active writer.
- Native UE camera/view blending remains a working hypothesis, not a proven
  conclusion.

## Approved scope

- Read-only Ghidra analysis of validated Steam 2.0.5 image only.
- Resolve containing function for `0xA9F0FB`.
- Enumerate direct code callsites to that function.
- Inspect caller instructions around each callsite, including XMM0 formation,
  object/register provenance and camera/PCM/ViewTarget-related accesses.
- Decompile only the containing function and direct callers.

## Explicit non-goals

- No runtime build or hook.
- No writes, calls, guessed offsets or production changes.
- No broad camera/renderer scan, indirect-call expansion or stack tracing.
- No claim that a caller is a producer without direct static evidence.

## Expected files

- One read-only Ghidra script and evidence output under `02-Research/`.
- This task plan.

## Validation and stop conditions

- Identity must pass SHA-256, image base and `.text` size before analysis.
- Report all direct callsites, even if unresolved.
- Stop at the direct-caller layer unless one caller clearly forms the FOV value
  and exposes a narrow, justified next target.

## Final review

Perform read-only Git status/diff review and report completed, remaining,
deferred, blocked and not-runtime-validated items.

## Result

Identity `PASS`. The known writer at RVA `0xA9F0FB` has exactly one direct
callsite in `FUN_1453A7C88` at RVA `0x53A7CAA`. Immediately before that call,
the caller invokes `FUN_1432B7706` at RVA `0x53A7C9C`. That function computes a
value through `FUN_1432B7842(param_1, 0)` and writes `param_1 + 0x230`, then
updates `+0x254`.

`FUN_1432B7706` is the strongest current producer candidate, but runtime
ownership is not yet proven. The next bounded runtime test, if pursued, should
instrument only store RVA `0x32B779D`. No multi-candidate probe is justified.

## Follow-up runtime batch

The static result justifies one separate research-only probe at store RVA
`0x32B779D` (`MOVSS [RSI+0x230], XMM0`). The probe will log every hit with the
destination object, previous and incoming FOV, elapsed time from final EXIT,
armed/coordinator state and thread ID. It will not alter execution or infer a
caller from the stack.

Runtime result is not yet available; no production behavior is changed.
