# Cutscene Hor+ Projection Implementation

Status: In progress — Batches 1 and 2 are complete; Batch 3 manual runtime validation is pending.

## Objective

Add an experimental cutscene Hor+ projection policy that converts the live game-owned cinematic FOV for the current display aspect while preserving the game's intended vertical framing.

## Established evidence and current state

- Target executable: game version 2.0.3, SHA-256 `81961B7281C7CF528CE49C549CE086FCC684BD676F32FAF042BC743D939E3C69`.
- Native render/aspect transition is already confirmed by the existing A/B letterbox hooks and must remain unchanged.
- The cinematic entry path is uniquely identified by the validated signature and resolves to `0x142EE1510` (`RVA 0x2EE1510`) in 2.0.3.
- The entry path loads a game-owned cinematic FOV global and passes it through the downstream call sequence. Runtime logs showed the game updating that value from `80.0` to `90.0`, then using `90.0` at cinematic entry.
- Vanilla 16:9 and experimental native 32:9 both used the same cinematic-entry FOV input; the framing difference therefore remains after this input stage.
- No native downstream projection owner was established. Dynamic live-FOV conversion is the selected fallback architecture.
- FOV semantics contract: the game-owned cinematic value is treated as horizontal FOV in degrees, consistent with the validated reference path and Unreal camera FOV convention. The conversion input is the live game-owned cinematic FOV at the post-load boundary; the native baseline aspect is `16:9`; the output is horizontal FOV in degrees for the live display aspect.
- Conversion contract: `targetHFov = 2 * atan(tan(sourceHFov / 2) * targetAspect / (16/9))`, with degree/radian conversion performed explicitly. At equal aspect the output equals the input; at `32:9`, `90°` maps to approximately `126.8699°` and `110°` maps to approximately `141.4093°`, while the equivalent vertical framing is preserved.
- Post-load contract: the validated load at `0x142EE1510` is 8 bytes, followed by the downstream call at `0x142EE1518`; the intervening instructions do not reload `XMM0`. A hook at that call boundary can modify the loaded FOV argument before the call, while the original load is not executed again at that boundary.

## Approved scope

- Modify only the experimental cutscene letterbox ASI path to apply the selected cinematic FOV conversion.
- Keep A/B resolver signatures, setter validation, native render behavior and gameplay ASI behavior unchanged.
- Use the actual FOV value present in `XMM0` after the validated game-owned FOV load, not a hard-coded `80` or `90`.
- Use live display aspect and a mathematically exact conversion from the game's native 16:9 cinematic framing.
- Live aspect contract: reuse the existing experimental letterbox ASI display-aspect owner/state (`g_displayAspect`), initialized from the current display before hook installation and refreshed by its existing owner thread. The cinematic hook reads this atomic state; it does not read arbitrary resolution memory from the game hook context or create a second aspect source.
- Runtime value safety contract: if source FOV or live aspect is non-finite or outside the validated range, preserve the original game-owned FOV for that invocation. A bad runtime value is a per-call safe no-op; resolver/decode/installation failure refuses the complete experimental feature.
- Preserve the original call/return flow and fail safely if the expected post-load boundary, decode or resolver contract is not present.

## Explicit non-goals

- No changes to stable gameplay aspect logic.
- No weapon/viewmodel FOV changes, `+0x234` policy, ADS handling or attachment ancestry logic.
- No changes to A/B setter hooks or their two-pass behavior.
- No fixed FOV multiplier, guessed scene-specific values, delayed replay, pause/ADS simulation or synthetic timing workaround.
- No release integration or compatibility claim beyond the tested executable identity.

## Expected files or areas

- `src/cutscene_letterbox_fix.cpp` for the experimental implementation.
- `build-artifacts/test-scripts/` for the bounded test build command if required.
- `build-artifacts/test-asi/` for the disposable experimental output.
- This plan and the existing cutscene research plan.

## Batches

### Batch 1 — Static implementation contract

- Reconfirm the unique cinematic signature and decode the `MOVSS XMM0,[game-owned-global]` instruction.
- Validate the post-load call boundary and its continuation before selecting the intervention point.
- Define the exact Hor+ conversion contract: live input FOV, live aspect, native cinematic aspect baseline and vertical-framing invariant.
- Define initialization, display-aspect refresh, thread/lifetime behavior and safe refusal.

Validation: static/decode evidence against the 2.0.3 executable. Batch 1 complete; no source edits were made during this batch.

### Batch 2 — Minimal implementation

- Implement only the dynamic FOV conversion at the validated post-load cinematic boundary.
- Preserve the original instruction/call semantics, registers and return flow required by the trampoline contract.
- Machine-flow contract: `MOVSS XMM0,[gameOwnedFov]` executes normally; the mid-hook at the following `CALL` reads only `XMM0.f32[0]`, writes only that scalar lane when conversion is valid, and lets the original `CALL` execute exactly once through the normal trampoline/continuation. Upper XMM lanes, game-owned global memory and downstream call ownership are untouched.
- Keep installation all-or-nothing with the existing A/B hooks; if the FOV target or either A/B target fails validation, install no experimental cutscene hooks.
- Do not alter the game-owned global; modify only the downstream FOV value passed into the cinematic projection path.

Validation: source review and successful test ASI build. Build success is not runtime evidence.

Batch 2 result:

- `src/cutscene_letterbox_fix.cpp` now installs the dynamic cinematic FOV hook at the validated post-load `CALL` boundary together with A/B, using all-or-nothing rollback.
- The hook converts only `XMM0.f32[0]` from the live game-owned cinematic FOV using live display aspect and preserves the upper XMM lanes, global FOV memory and original downstream call execution.
- Invalid runtime FOV/aspect values are handled as a per-call no-op; resolver, decode, executable-boundary or installation failure refuses the complete experimental feature.
- Build succeeded with output `build-artifacts/test-asi/STALKER2ExperimentalLetterboxFix.asi`.

### Batch 3 — Manual runtime validation

- Test at `5120x1440` with the existing experimental A/B letterbox behavior.
- Compare vanilla 16:9 and experimental native 32:9 rendering.
- Exercise multiple cinematic FOV values, including the observed `90.0` and at least one distinct game-owned value if the game produces one.
- Confirm native 32:9 render, preserved vertical framing, horizontal expansion and no gameplay regression after cutscene.
- Confirm safe refusal/rollback using resolver/decode failure conditions where practical without modifying the game executable.

Validation: user-run runtime evidence with executable identity, reproduction steps, logs and visual result. Do not claim compatibility beyond this evidence.

## Risks and rollback or safe failure

- The FOV hook must not run if the expected signature is ambiguous, the decoded load/call boundary differs or the target is not executable.
- The implementation must not use the stale `80.0` initialization value; it must consume the runtime `XMM0` input.
- If FOV conversion installation fails, reset any already-created FOV/A/B hooks and leave the game unmodified by this feature.
- If manual testing shows altered vertical composition, scene-specific breakage or gameplay regression, disable/revert the experimental FOV policy without changing the validated A/B render path.

## Stop conditions and phase gates

- Do not edit source until Batch 1 confirms the exact post-load intervention contract.
- Do not build until the plan and source diff are limited to this experimental FOV policy.
- Do not claim success from build/load alone; manual runtime testing is required.
- Stop implementation if the only safe option becomes a hard-coded FOV/multiplier, gameplay replay or timing workaround.

## Expected final Git review

After manual testing, inspect Git status, relevant diff and recent commit; compare all changed paths with this plan and classify completed, remaining, deferred, blocked and not-runtime-validated items. Only then update `backlog/TASKLOG.md`.
