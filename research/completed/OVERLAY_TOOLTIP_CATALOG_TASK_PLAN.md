# Overlay Tooltip Catalog — Task Plan

## Objective

Give every overlay tooltip a dedicated control-specific handler and standardize all tooltip content as one named option per line: `name - description`. Preserve each setting's current meaning and apply-time semantics.

## Established evidence and current state

- `renderer_runtime.cpp` currently routes six controls/status tooltips through a single generic text helper, with prose summaries rather than a complete option catalog.
- `src/config/config_template.cpp` contains the authoritative INI descriptions for Gameplay Enabled/Mode, Cinematic Aspect/FOV, and Dialogue Zoom.
- The user requests a separate handler for each tooltip and an INI-style description for every selectable mode/value.
- Overlay sources are currently untracked in this working tree, alongside other existing dirty/untracked work; preserve that state and review exact changed source.

## Approved scope

- Replace the generic free-text tooltip call pattern with a shared standardized option-list renderer plus a dedicated handler for each tooltip.
- Add one entry per selectable option for Gameplay Enabled, Gameplay Mode, Cinematic Aspect, Cinematic FOV Mode, and Dialogue Zoom.
- Give Camera Integration status explanation its own handler and standardized status entries.
- Preserve bounded wrapping and the ImGui standard tooltip hover behavior.
- Make text concise but semantically faithful to the INI template and current runtime semantics.

## Explicit non-goals

- No changes to settings, camera behavior, lifecycle, hooks, semantic projection, persistence, or menu layout.
- No changes to INI/README descriptions.
- No game launch or injected runtime test by Codex.

## Expected files

- `src/overlay/renderer_runtime.cpp`
- `OVERLAY_TOOLTIP_CATALOG_TASK_PLAN.md` (archive on completion)
- `backlog/TASKLOG.md` completion note, if the existing large working file can be safely updated through the approved patch workflow.

## Batches and validation

1. Implement `TooltipOption`/shared bounded list rendering and dedicated named handlers for each UI control/status tooltip. Route each hovered item to exactly its matching handler. Inspect all entries against `config_template.cpp` and the current setting apply-time semantics.
2. Build with `build-overlay-settings.cmd`; check trailing whitespace in the untracked overlay source and run `git diff --check`; inspect scoped status/source and ensure no non-tooltip code changed.

## Risks and safe failure

- Risk: incorrect option descriptions or accidental association of a tooltip with the wrong control. Dedicated handler names, full option lists, and source review mitigate this.
- Risk: wrapping or hover timing regression. Reuse existing ImGui `ForTooltip` flags and the bounded tooltip renderer.
- On build/validation failure, keep the source edits isolated to tooltip presentation; do not modify camera/runtime behavior.

## Stop conditions and phase gates

- Stop if an option description conflicts with the INI semantics or requires a camera behavior decision; report the discrepancy instead of inventing wording.
- Do not launch the game. After build, hand off for visual review of representative short and long option lists.

## Final Git review

- Review `git status`, `git diff --check`, the exact `renderer_runtime.cpp` changes, and recent commits. Identify that the overlay source files are untracked in the pre-existing worktree and do not imply a clean baseline. Report task-log update separately if the large dirty task log cannot be safely patched.
