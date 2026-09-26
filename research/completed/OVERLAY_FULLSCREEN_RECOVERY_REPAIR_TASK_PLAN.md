# Overlay Fullscreen/Recovery Repair Task Plan

## Objective

Diagnose and repair only the two runtime defects from the combined overlay
settings test: resize latency and deferred Gameplay mode recovery after a
fullscreen/resolution transition.

## Established evidence and current state

- Overlay rendering, input, settings mutation and Dialogue runtime changes
  passed in the first combined test.
- Resize stability passed, but user-visible resize latency was observed.
- The production log recorded `Gameplay mode transition deferred: reason=actual_runtime_aspect_not_established` after a fullscreen/resolution change.

## Approved scope

- Add bounded resize lifecycle timing.
- Confirm the existing D3D12 synchronization contract before any optimization.
- Preserve or minimally repair synchronization only if statically justified.
- Retain and retry pending Gameplay mode intent at the first established valid
  aspect boundary, with latest-request-wins semantics.
- Add pure deterministic lifecycle tests and build a combined artifact.

## Explicit non-goals

- No overlay layout/input/settings semantics changes.
- No F9-F12 changes.
- No HorPlus, AspectRecalculation, cinematic math or dialogue classifier changes.
- No new hooks, timer polling, stale-aspect fallback or graphics feasibility audit.
- No game launch in this batch.

## Expected files/areas

- `src/overlay/renderer_runtime.*`, resize interception and overlay tests.
- `src/plugin/runtime.cpp` and a small testable Gameplay transition helper if
  required.
- `tests/`, build script and a factual repair report.

## Implementation batches

1. Source audit and timing telemetry around the resize path.
2. Bounded pending Gameplay recovery only at an existing valid-aspect event.
3. Tests, production/diagnostic/combined builds and static review.

## Validation

- Focused lifecycle/recovery and overlay harnesses.
- `test.cmd`, `build.cmd`, `build-diagnostic.cmd`,
  `build-overlay-settings.cmd`, `git diff --check`.
- Runtime explicitly deferred to one combined regression after static PASS.

## Risks and safe failure

- Do not remove D3D12 waits without proving resource lifetime safety.
- Invalid aspect never triggers retry and never falls back to stale/monitor/16:9
  values.
- Pending mode is replaced by the latest authoritative request.
- Renderer remains fail-closed if timing or rebuild logic fails.

## Stop conditions and phase gates

- If latency cause is not statically established, keep synchronization and ship
  timing telemetry without speculative optimization.
- Stop after static validation/builds; do not launch the game.

## Final Git review

Review only the approved paths, distinguish completed/deferred/runtime-unvalidated
items, and confirm the settings frontend and F9-F12 paths were not broadened.
