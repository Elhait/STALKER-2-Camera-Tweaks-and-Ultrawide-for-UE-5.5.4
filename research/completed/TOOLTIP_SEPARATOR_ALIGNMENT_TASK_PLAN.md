# Tooltip Separator Alignment Task Plan

## Objective

Adjust the shared option-list tooltip so the `-` separator has its own aligned column, leaving wrapped description lines visually under the description rather than beginning with a hyphen.

## Established evidence and current state

- `src/overlay/renderer_runtime.cpp` contains `ShowOptionListTooltip`, which measures the longest option name and aligns descriptions.
- The renderer currently passes `" - %s"` as a single wrapped text item, causing the separator to travel with description wrapping.
- The source file already has unrelated staged and unstaged changes from prior overlay work; preserve all of them.

## Approved scope

- Change only the shared tooltip row presentation: name column, separator column, description column.
- Keep tooltip wording, catalogs, hover behavior, settings, and camera behavior unchanged.

## Explicit non-goals

- No tooltip content or sizing redesign beyond the separator column.
- No other overlay layout, color, hotkey, toast, settings, or camera changes.
- No game launch or runtime validation.

## Expected files

- `src/overlay/renderer_runtime.cpp`
- `backlog/TASKLOG.md` after validation.

## Batches and validation

1. Render the separator as its own aligned item between option names and descriptions. Validate with the overlay build and `git diff --check`.
2. Review the final diff/status against this plan and record the bounded result in TASKLOG.

## Risks and safe-failure behavior

- A spacing regression could cause column overlap in narrow tooltips. Keep columns based on the existing longest-name measurement and preserve current wrap width.
- If build or diff validation fails, retain the existing renderer behavior and do not launch the game.

## Stop conditions and phase gates

- Stop if the desired separator alignment requires changing tooltip text, catalog semantics, or unrelated layout.
- Stop after successful static validation; runtime review remains separate.

## Expected final Git review

- Confirm only the planned renderer behavior and task record changed for this batch; preserve all pre-existing work and do not stage or commit.
