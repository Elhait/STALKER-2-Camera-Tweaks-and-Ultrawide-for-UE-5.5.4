# Overlay Parameter Label Style — Task Plan

## Objective

Improve readability of the existing overlay by visually distinguishing parameter labels from their values and statuses with one consistent accent style.

## Established evidence and current state

- Settings, semantic status text, Camera Integration values, and top-center toast notifications are runtime-validated.
- Current labels and values use nearly identical text styling, reducing scanability.

## Approved scope

- Add a reusable parameter-label presentation helper in the overlay renderer.
- Apply it to settings labels, Runtime labels, Camera Integration labels, and technical Camera State labels where appropriate.
- Preserve all values, status colors, tooltips, notification behavior, and layout semantics.

## Explicit non-goals

- No runtime settings, camera state, hotkey, toast, or camera behavior changes.
- No new fonts, textures, or external UI dependencies.
- No automatic game launch.

## Expected files or areas

- `src/overlay/renderer_runtime.cpp`
- presentation-only deterministic/build validation

## Validation

- Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.
- Confirm existing semantic and notification harnesses remain unchanged and passing.

## Risks and safe failure

- Risk: label coloring reduces contrast. Mitigation: use a restrained accent color and preserve default value/status colors.
- Safe failure: presentation-only helper can be removed without affecting runtime behavior.

## Stop conditions and final review

- Stop if label styling requires changing semantic/status strings or layout ownership.
- Archive this plan under `research/completed/` after validation passes.
