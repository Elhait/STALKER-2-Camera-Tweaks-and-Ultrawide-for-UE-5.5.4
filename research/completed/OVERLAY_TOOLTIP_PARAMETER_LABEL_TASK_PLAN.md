# Overlay Tooltip Parameter Labels — Task Plan

## Objective

Visually distinguish option names from their descriptions in the existing standardized overlay tooltip renderer.

## Scope

- Update only `ShowOptionListTooltip` in `src/overlay/renderer_runtime.cpp`.
- Render each option name with the existing parameter-label accent color.
- Keep descriptions, wrapping, tooltip lifetime, and tooltip behavior unchanged.

## Non-goals

- No tooltip catalog or semantic text changes.
- No settings, camera, hotkey, notification, or input changes.
- No game launch.

## Validation

- Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.
- Archive this plan under `research/completed/` after validation passes.
