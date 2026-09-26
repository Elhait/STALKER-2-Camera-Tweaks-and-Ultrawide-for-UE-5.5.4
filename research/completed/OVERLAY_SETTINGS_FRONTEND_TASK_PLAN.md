# Overlay Settings Frontend Task Plan

## Objective

Convert the runtime-validated Overlay POC into an experimental combined ASI
settings frontend for Gameplay, Cinematics and Dialogue.

## Established evidence and current state

- D3D12 presentation path, ImGui rendering, Insert toggle, input coexistence
  and resize/rebuild behavior were runtime-validated.
- `RuntimeState` and the existing runtime settings mutation owner are private
  to the production translation unit.
- The standalone OverlayPOC cannot safely share that process-local instance.

## Approved scope

- Add a typed snapshot/read boundary and typed mutation boundary for the five
  user-facing settings in the task.
- Reuse existing config persistence and production setters.
- Replace POC test content with a settings UI in a combined experimental ASI.
- Add deterministic API/UI-facing harness coverage where practical.
- Add a combined build entrypoint and factual report.

## Explicit non-goals

- Do not migrate or change F9-F12.
- Do not change camera math, hooks, lifecycle semantics, or production release
  artifact behavior.
- Do not create a second settings owner or a cross-DLL pointer bridge.
- Do not launch the game in this batch.

## Expected files/areas

- `src/plugin/runtime_settings.hpp` and production runtime integration.
- `src/overlay/renderer_runtime.*` and overlay frontend integration.
- combined build script, deterministic harnesses, and report.

## Batches and validation

1. Extend typed settings API with coherent snapshot and validated operations.
   Validate with the API harness and `test.cmd`.
2. Add settings UI and connect it to the combined runtime owner.
   Validate by compilation and static source review.
3. Build production, diagnostic, and combined experimental artifacts; run
   `test.cmd`, `git diff --check`, and inspect the final diff.

## Risks and safe failure

- Persistence failure must not crash or leave the renderer disabled; mutation
  reports failure and retains the effective runtime value.
- Invalid enum/value input is rejected by the typed API.
- Overlay initialization failure leaves camera runtime independent.
- The normal production ASI is built without overlay sources.

## Stop conditions and phase gates

- Stop if sharing the private runtime owner would require unsafe cross-DLL
  state or duplicated authoritative state.
- Stop after static validation and builds; runtime is explicitly deferred to
  the single combined overlay/settings test.

## Final Git review

Review status, changed paths, diff summary and validation results. Confirm that
F9-F12 and the production release artifact were not modified by this batch.
