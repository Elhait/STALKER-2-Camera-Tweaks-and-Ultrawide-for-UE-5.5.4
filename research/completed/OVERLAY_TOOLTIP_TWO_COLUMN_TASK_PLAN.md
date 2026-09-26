# Overlay Tooltip Two-Column Layout — Task Plan

## Objective

Standardize tooltip content as a two-column catalog: option names form a left column and descriptions begin at one aligned column position.

## Approved scope

- Update the shared tooltip renderer to measure the longest option name per catalog.
- Align every description after that name column and preserve wrapping.
- Remove non-essential recommendation/default wording from the existing Gameplay and Cinematic FOV descriptions.

## Non-goals

- No tooltip semantics beyond wording cleanup.
- No settings, camera, hotkey, notification, input, or lifecycle changes.
- No game launch.

## Expected files

- `src/overlay/renderer_runtime.cpp`

## Validation

- Run `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.
- Archive this plan under `research/completed/` after validation passes.
