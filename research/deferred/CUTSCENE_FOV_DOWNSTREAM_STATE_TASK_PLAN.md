# Cutscene FOV Downstream State Diagnostic Task Plan

Status: In progress — Batch 1 complete; Batch 2 downstream state/blend-owner tracing is active.

## Objective

Determine where the single transformed cinematic FOV value is consumed, stored or blended after `FUN_146B68976`, and identify the first durable state owner responsible for the visible post-cutscene FOV transition.

## Established evidence and current state

- The current executable is S.T.A.L.K.E.R. 2 version 2.0.3 at native `5120x1440` (32:9).
- A/B letterbox hooks produce native 32:9 cinematic rendering.
- The dynamic Hor+ conversion is applied once during cinematic setup: `90.0 → 126.869896`.
- No cinematic FOV-entry, A/B setter or global-writer hits occur during the observed 10-second post-`F8` window.
- The transition-boundary/repeated-conversion hypothesis is rejected by runtime evidence.
- Persistence or blending of the transformed value is a strong hypothesis, not yet a confirmed mechanism.

## Approved scope

- Static Ghidra/data-flow analysis starting at `FUN_146B68976` and its immediate callees/callers.
- Establish the calling convention and argument mapping for `FUN_146B68976`, including whether `XMM0` is an entry FOV argument or only a transient register before a downstream call.
- Identify argument semantics and the path of the transformed `XMM0` value.
- Find the first durable write, cache, blend source or projection-state consumer reachable from this path.
- Identify the writer or transition mechanism that returns the value to gameplay FOV.
- Tie conclusions to executable version 2.0.3 and existing runtime evidence.

## Explicit non-goals

- Do not change `src/cutscene_letterbox_fix.cpp` or the stable gameplay ASI.
- Do not change the Hor+ formula or A/B letterbox contract.
- Do not add a hook, tracer, timer, frame delay, replay or hard-coded FOV compensation.
- Do not call an inferred field a state owner without decoded instruction and caller/data-flow evidence.
- Do not claim that `126.869896` is stored persistently until a concrete write or cache owner is established.

## Expected files and areas

- `Dump/STALKER2-Ghidra` for the preserved 2.0.3 analysis project.
- `02-Research/Ghidra/ghidra-scripts/` only if a narrowly scoped reusable helper is required.
- A bounded research note or report under the project research area if the analysis creates durable findings.
- `backlog/TASKLOG.md` remains deferred until the bounded research result and any required manual validation are complete.

## Batches

### Batch 1 — Immediate call-path reconstruction

- Disassemble and decompile `FUN_146B68976`.
- Identify the meaning of its arguments and the downstream calls receiving the transformed FOV.
- Record relevant registers, return values and object/context pointers without assuming names from nearby metadata.

Validation: reproducible Ghidra references tied to the 2.0.3 executable identity.

Batch 1 result:

- In `Stalker2-Win64-Shipping.exe (2.0.3)`, `FUN_146B68976` decompiles as `FUN_146B68976(undefined4 param_1)` and preserves the incoming `XMM0` value in `XMM6` before moving it to `XMM1` for the downstream tail call.
- The caller at `FUN_142EE14BC` loads the game-owned cinematic FOV into `XMM0` immediately before calling `FUN_146B68976`.
- `FUN_146B68976` moves the preserved value into `XMM1` and tail-jumps to `FUN_1431D2094`; the downstream function therefore receives the FOV as its second floating-point argument under the observed ABI, while `FUN_146B68976` itself also receives a separate integer/context argument.
- `FUN_1431D2094` selects or creates a state object at `param_1 + 0x2CF8`, compares its field `+0x54` against the incoming FOV and tail-jumps to `FUN_1431D23AE` when an update is required.
- `FUN_1431D23AE` directly writes the incoming FOV to `state + 0x54`. It also writes camera-manager field `+0x2CE8` to `state + 0x50` and player-camera field `+0x230` to `state + 0x58`. The `+0x54` write is a confirmed durable state write; blend ownership and source/target semantics remain unresolved.

### Batch 2 — Durable state and blend-owner tracing

- Follow the transformed FOV to the first concrete field write, cache, blend input or projection consumer.
- Identify the blend source pair where possible: the transformed cinematic value used as the blend start and the gameplay FOV used as the blend target.
- Identify the blend/update function and callers/writers responsible for the later smooth return to gameplay FOV.

Batch 2 progress:

- The immediate downstream state path and durable `+0x54` write are confirmed.
- On the 2.0.3 Ghidra program, `FUN_1431D23AE` writes the incoming FOV directly to `state + 0x54`, then refreshes related camera/player fields at `+0x50` and `+0x58` and invokes a virtual update/notification path through the state subobject at `+0x28`.
- The direct caller set is limited to the state-dispatch path (`FUN_1431D2094`/tail path); no separate statically named blend reader was established from direct callers.
- The `+0x54` value is therefore confirmed as a durable state field, but its role as blend start, direct projection input or cached transition value remains unresolved.
- If a concrete virtual target reads `+0x50`, `+0x54` and `+0x58` together, classify that as strong evidence for camera-state reconciliation/blend semantics, while keeping the individual field roles provisional until the data flow is complete.
- The bounded virtual-owner check confirms `FUN_1431D21E4` is an FOV equality predicate against the current camera-manager value through vtable slot `+0x818`, not the downstream blend owner. `FUN_1431D224D` is the state creation/factory path. The post-write notification remains an indirect virtual dispatch; no concrete target reading all three state fields has yet been established.
- The next bounded target is the concrete virtual dispatch behind the post-write notification/update path, followed by the state object's downstream consumers if that dispatch can be resolved. Do not perform another global `+0x54` scan.
- Classify each candidate as `confirmed`, `likely`, `unresolved` or `rejected`, with a concrete reason.

Validation: decoded instructions, caller/data-flow evidence and explicit separation of fact from hypothesis. No runtime claim unless separately tested.

### Batch 3 — Architecture gate

- If a downstream owner is established, update the implementation plan with a minimal intervention contract that preserves cinematic framing and gameplay FOV behavior.
- If ownership remains unresolved, stop and report the blocker; do not implement a timing or value workaround.
- Keep any future runtime tracer or implementation as a separate approved task.

Validation: read-only Git review of research artifacts and plan comparison; `TASKLOG.md` only after the bounded task is complete and any manual validation is performed.

## Risks and safe failure

- Do not infer ownership from a single similar field offset or generic renderer function.
- Avoid modifying the original Ghidra project structure or executable data.
- If decompilation is ambiguous, preserve the ambiguity and record the exact unresolved edge.
- If a helper script is needed, keep it read-only, version-specific and bounded to the known call path.

## Stop conditions and phase gates

- Stop if the only evidence is the visible FOV symptom or the transformed scalar value without a concrete downstream write/consumer.
- Stop if analysis expands into broad renderer or generic FOV scanning without a new anchor.
- Stop if the proposed fix requires a timer, frame count, delayed replay or hard-coded multiplier.
- Do not reopen the accepted A/B or Hor+ formula contracts without contradictory evidence.

## Expected final Git review

After the bounded research batch, inspect Git status, relevant diff and recent commit; compare changed paths with this plan and classify completed, remaining, deferred, blocked and not-runtime-validated items. Update `backlog/TASKLOG.md` only after the research result, plan comparison and required manual validation are complete.
