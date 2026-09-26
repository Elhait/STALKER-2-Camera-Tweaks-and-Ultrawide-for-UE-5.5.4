# Movable Overlay Window Task Plan

## Objective
Allow the main overlay to be moved by its title bar, keep it reachable within the current game viewport, and restore its last position on later runs.

## Established Evidence And Current State
- `Renderer::Render` set the main window position to `(30, 30)` with `ImGuiCond_Always` every visible frame, overriding drag movement.
- The main window used `ImGuiWindowFlags_NoSavedSettings`, so ImGui could not restore its position.
- The width is constrained against viewport size; position must be independently clamped when viewport dimensions change.
- Toast windows opt out of ImGui saved settings and remain unaffected.

## Approved Scope
- Main overlay window movement and persistence only.
- Persist only the main window position through Unicode-safe Win32 profile APIs to a dedicated file beside the ASI, not the game's working directory.
- Clamp the main overlay's position to the game viewport, including safe behavior if it is larger than the viewport.
- Preserve title bar, NoCollapse, visibility hotkey, input policy, width sizing, camera/settings state, and toast behavior.

## Explicit Non-Goals
- No localization inspection or implementation; that is a separate task per user instruction.
- No changes to camera, settings, hotkey capture, or input routing.
- No game launch or injected runtime validation.
- No Git commands or Git state changes, per explicit user instruction.

## Files / Areas
- `src/overlay/renderer_runtime.hpp` and `.cpp`: Unicode-safe placement path, first-use default position, viewport clamp, and custom persistence.
- `backlog/TASKLOG.md`: factual implementation and validation record.

## Batches And Validation
1. Remove per-frame forced placement, load/save the main window position from an ASI-adjacent Unicode path, clamp it to the active viewport, and keep ImGui's general saved settings disabled. Validate with `build-overlay-settings.cmd`.
2. Run `test.cmd` for regression coverage and verify the generated overlay ASI. Do not launch the game.

## Risks And Safe-Failure Behavior
- If the ASI path cannot be resolved, movement remains available for the current session and custom persistence is skipped rather than writing settings to the game's current working directory.
- If the saved position is outside the current viewport, clamp it before displaying the window; if the window exceeds viewport dimensions, pin its origin to the viewport origin.
- No changes are made to the game configuration or behavior outside overlay placement.

## Stop Conditions / Phase Gates
- Stop if placement persistence would require modifying gameplay/configuration semantics or changing overlay input capture.
- Stop if the module path cannot be safely established beside the ASI.
- No runtime/game validation is authorized or claimed.

## Expected Final Review
- Compare touched paths and outcomes to this plan using direct file inspection only. Git review is intentionally omitted because the user explicitly prohibited Git interaction.
