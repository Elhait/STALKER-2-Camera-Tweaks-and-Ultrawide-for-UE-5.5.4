# Overlay Tooltip Hover Area — Task Plan

## Objective

Improve tooltip readability and hit areas without changing tooltip content or settings semantics.

## Approved scope

- Return parameter labels and tooltip option names to neutral ImGui text styling.
- Make each settings tooltip respond to the complete control row: control, value field, arrow, and label.
- Keep each existing parameter-specific tooltip handler as the single content source.

## Non-goals

- No tooltip wording/catalog changes.
- No settings, camera, hotkey, notification, input, or lifecycle changes.
- No game launch.

## Expected files

- `src/overlay/renderer_runtime.cpp`

## Validation

- Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.
- Archive this plan under `research/completed/` after validation passes.
