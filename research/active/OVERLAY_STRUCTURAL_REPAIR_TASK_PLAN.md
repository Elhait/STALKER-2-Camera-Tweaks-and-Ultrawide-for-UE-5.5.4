# Overlay structural repair task plan

Date: 2026-09-26
Status: Implementation and required validation complete; awaiting user acceptance
Authorization: User-approved structural repair request in Codex attachment
`97baebb9-8305-4004-abac-14ff883716da`.

## Goal and invariants

Move the two accepted A-class presentation responsibilities out of
`renderer_runtime.cpp` while preserving all accepted runtime and visual
contracts. Verify each B-class boundary and keep C-class composition and
D-class non-abstractions as specified by the audit.

No changes to runtime/config semantics, labels, localization content, layout,
dimensions, spacing, typography, selector lifecycle, tooltip content or
behavior, Camera State facts, hotkey capture, notification behavior, or
graphics lifecycle. No game launch, release, or package work. Do not stage,
revert, or overwrite pre-existing user changes.

## Batches

### Batch 1 — Camera State view

- Add `src/overlay/camera_state_view.hpp/.cpp`.
- Move existing Camera State presentation/formatting mechanically.
- Input: const `OverlaySemanticSnapshot` and const `LocalizationManager`.
- Keep snapshot acquisition and `TreeNodeEx` ownership in Renderer.
- No semantic derivation or state ownership in the view.
- Review exact moved code and run focused available overlay harnesses plus
  structural source checks; compile the production translation unit/profile.
- Result: complete; the full 45-harness runner, localization/font audit, and
  production build passed before proceeding.

### Batch 2 — Selector documentation view

- Add `src/overlay/selector_documentation_view.hpp/.cpp`.
- Move selector documentation rendering, Examples geometry and selector-only
  spacing; preserve option-content data ownership and explicit interaction
  input.
- Keep `DrawSelectorCombo`, selection mutations, and ordinary-item tooltip
  hover lifecycle in Renderer.
- Preserve `hovered && !active && !popupOpen`, all canonical identifiers and
  content, Examples associations, 10x8 inset, conditional 12 UI px gap,
  three-column alignment, wrapping, widths and accepted runtime behavior.
- Keep shared aligned-row drawing single-sourced without merging tooltip
  lifecycle decisions.
- Update structural source audit to follow the new owner.
- Run focused harnesses/source audit and production compile.
- Result: complete; the full 45-harness runner, localization/font audit, and
  production build passed with both view translation units included.

### Batch 3 — B/C/D disposition

- Verify `DrawSelectorCombo`, Examples, hotkeys, notifications, generic row
  helpers and runtime rows in the post-extraction structure.
- Retain already-cohesive private helpers; make no module solely for symmetry.
- Confirm runtime panel, feature sections, top information area and two-column
  composition remain in `Renderer::Render`.
- No additional extraction unless a concrete boundary failure is found within
  the approved scope.
- Result: complete; all B boundaries retained or placed with their natural
  owner, C composition remains in `Renderer::Render`, and no D abstraction was
  introduced.

## Final validation and acceptance

- Full `test.cmd`.
- Localization catalog audit across all 18 catalogs.
- Embedded resource/glyph/font coverage audit (including the audit checks run
  by `test.cmd`/`build.cmd`).
- Production `build.cmd`.
- `git diff --check` only; do not stage or alter pre-existing staged changes.
- Review the complete task diff and changed-path list. No game launch.
- Report tests/build as evidence for this workspace artifact only; runtime
  visual behavior remains untested in this task.

## Pre-existing state and scope control

Before edits, the repository already contained a large staged change set,
including the overlay sources, localization/font assets, build scripts,
tests, and research records. Those staged contents are user baseline and must
remain untouched in the index. This task may modify only this plan, the two
new view pairs, `tooltip_row_layout.hpp` (to keep existing shared row layout
single-sourced), `renderer_runtime.cpp`, `build.cmd`, and the localization
source-policy audit as required to preserve structural regression coverage.
