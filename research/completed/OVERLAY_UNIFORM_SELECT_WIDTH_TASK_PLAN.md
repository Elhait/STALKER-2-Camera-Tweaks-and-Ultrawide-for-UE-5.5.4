# Uniform Select Width And Equal Columns Task Plan

## Objective
Make all production select controls in the Configuration column share one locale/font-aware width derived from the widest option across those controls, and make the Configuration/Runtime columns equal width (50/50) while retaining automatic content sizing and the viewport-only window cap.

## Established evidence and current state
- The prior fix removed the former 680 px cap and scale, and changed table columns to `SizingFixedFit`, which sizes each column independently from its contents.
- `IntrinsicComboWidth` is currently applied independently to each select's own option list, so controls vary in width and labels move when different controls/options appear.
- Vendored ImGui source documents `ImGuiTableFlags_SizingFixedSame` as auto-fitting both fixed columns to the maximum width of all table contents, producing equal-width columns without hard-coded ratios.
- Runtime font-size application is deferred until slider edit ends and is out of scope to change.
- User explicitly forbids Git operations; do not inspect or change Git state.

## Approved scope
- Compute one common width from all production Configuration select option strings in the active locale/font metrics, including arrow/frame padding.
- Apply that exact width to Gameplay, Cinematics, Dialogue and Overlay language selects.
- Switch the two main table columns to equal, content-fit width via the vendored ImGui sizing policy.
- Preserve viewport width clamping, existing padding, font-size-release behavior, language/camera semantics and the experimental font controls' status.
- Run `test.cmd` and `build-overlay-settings.cmd`; do not launch the game.

## Explicit non-goals
- No fixed pixel/percentage split, old 680 px cap, font-size scaling, content/localization semantic changes, camera logic, slider behavior, or Git commands.

## Expected files/areas
- `src/overlay/renderer_runtime.cpp`
- Task plan and `backlog/TASKLOG.md`
- Archive this plan to `research/completed/` after successful validation.

## Batches and validation
1. Hoist the production select option lists to the configuration render scope, compute a shared option width once per frame from the widest entry across all lists, and pass it to every production select. Validate source compilation.
2. Set the main table sizing policy to fixed-same intrinsic sizing; confirm from vendored ImGui source that this is equal auto-fit sizing. Run `test.cmd` and build the stable overlay ASI.
3. Review touched paths and summarize the evidence/limits. Stop after build; runtime appearance remains for user confirmation.

## Risks and safe failure
- A very long option in any one list makes every select wider; that is intentional, deterministic and avoids per-control jitter.
- Equal columns can make the combined window wider when one column has long content. The existing viewport cap remains the safety bound; content wraps/clips only once that bound is reached.
- Width is recomputed from active ImGui text metrics, so locale/font-size changes naturally produce a new common width; changing the selected item does not.

## Stop conditions and phase gates
- Stop if the ImGui version does not support equal auto-fit columns as documented or if common width causes select controls to exceed the viewport cap in ordinary resolution without a safe wrap behavior.
- Do not change unrelated controls or behavior.
- No game launch or runtime claims.

## Final review
- User prohibits Git interaction; omit Git status/diff/log review.
- Compare changed files with this plan, record test/build outcomes and runtime-validation limits in `backlog/TASKLOG.md`, and archive this plan.
