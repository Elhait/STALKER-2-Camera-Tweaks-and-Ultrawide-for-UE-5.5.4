# Overlay Interactive Shell Task Plan

## Objective

Extend the runtime-validated experimental D3D12/ImGui overlay POC with a
minimal toggle and input-capture shell. Keep the shell isolated from the
production camera ASI and defer settings integration.

## Established evidence/current state

- The experimental renderer displays a static ImGui window in STALKER 2.
- Swapchain, device, DIRECT queue, Present and repeated resize/rebuild are
  runtime-validated.
- The production ASI does not include the overlay sources.
- The current window is non-interactive and has no input hook.

## Approved scope

- Toggle the experimental overlay with Insert.
- Route window messages through the Dear ImGui Win32 backend while visible.
- Consume overlay UI input only while visible; preserve the game's normal
  message path while hidden.
- Add pure input/capture state tests.
- Keep rendering, resize and fail-closed behavior intact.

## Explicit non-goals

- No settings/configuration integration.
- No camera/gameplay/cinematic/dialogue changes.
- No raw-input hook, cursor clipping, focus manipulation or new graphics path.
- No production build or release artifact changes.
- No game launch in this batch.

## Expected files/areas

- `src/overlay/input_state.*` and focused harness.
- `src/overlay/renderer_runtime.*` and `discovery_runtime.cpp`.
- `build-overlay-poc.cmd`, `test.cmd`, overlay report and task log.

## Validation

- Focused input and renderer harnesses plus full `test.cmd`.
- Production `build.cmd` unchanged and successful.
- Experimental POC build successful.
- `git diff --check` and read-only Git review.
- No game launch; provide the next runtime toggle/input gate separately.

## Risks and safe failure

WndProc installation failure leaves the overlay visible but non-interactive and
does not affect the camera mod. ImGui handler failure leaves the game message
path intact. Renderer failure still disables only the overlay.

## Stop conditions

Stop if input routing requires raw-input interception, cursor ownership,
additional hooks or production settings integration. Do not expand into a full
UI in this batch.

## Final review

Compare changed paths with this plan, update the overlay report and task log,
and move this plan to `research/completed/` after validation.
