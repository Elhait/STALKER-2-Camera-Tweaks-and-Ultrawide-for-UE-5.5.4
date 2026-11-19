# Overlay OQ-01–OQ-09 Repair Plan

## Objective

Implement the accepted code-quality repairs OQ-01 through OQ-09 without changing the accepted UI appearance, runtime settings semantics, selector content, or camera behavior. This is bounded maintenance and failure-path work, not another Overlay redesign.

## Evidence and current owners

- Renderer::Render owns the ImGui/D3D12 presentation frame and notification rendering; RendererLifecycle owns renderer lifecycle state.
- InputState owns visibility, mouse capture, and hotkey rebinding.
- LocalizationManager owns embedded catalog readiness and lookup.
- Selector choice arrays currently feed ordinal casts into config enums; setting_tooltip_content separately owns tooltip names/content.
- Placement persistence is delegated to placement_config and the config repository. UI dirty state is currently owned by Renderer.
- test.cmd and build.cmd are the supported validation entry points.

## Batches and acceptance

0. OQ-04: document selector capture timing and narrow BeginCombo result semantics; explain the Examples inset/Indent geometry. No behavior change.
1. OQ-05: replace brittle renderer spelling checks with contract checks, broaden UI ownership/localized prose policies to presentation modules, and add practical negative fixtures and bounded geometry/interaction coverage.
2. OQ-01: ensure exceptions from optional overlay work do not terminate inside noexcept or escape native callbacks; preserve original native calls, isolate camera behavior, define partial-resource failure handling, and do not convert access violations into C++ exceptions.
3. OQ-02/OQ-03: terminal disable transitions the input owner to hidden, restores cursor and cancels rebinding; resource recreation remains nonterminal. Catalog initialization failure gates localized settings presentation without introducing C++ prose or external fallbacks.
4. OQ-06/OQ-08: make selector option-to-config mapping explicit and tested; use stable action-based hotkey widget IDs while preserving labels/size.
5. OQ-07: name repeated semantic palette and tooltip policy values without changing values or adding a design-token framework.
6. OQ-09: represent failed placement persistence as pending/retryable through a normal subsequent persistence opportunity, with no per-frame disk retry.

## Expected files

- src/overlay/renderer_runtime.cpp/.hpp
- src/overlay/renderer_state.cpp/.hpp
- src/overlay/input_state.cpp/.hpp
- src/overlay/discovery_runtime.cpp
- src/overlay/localization_manager.cpp/.hpp
- src/overlay/selector_tooltip_state.hpp
- src/overlay/selector_documentation_view.cpp
- src/overlay/setting_tooltip_content.hpp
- src/overlay/hotkey_binding_presentation.hpp
- tests/overlay/*_harness.cpp
- tests/runner/localization_catalog_audit.ps1
- test.cmd

Build/source registration may change only when required for the bounded tests or existing modules.

## Non-goals

- No visual/layout/content/localization redesign.
- No changes to settings, camera semantics, game callbacks' original behavior, or accepted selector tooltip geometry.
- No generic UI framework, declarative schema, theme, or broad component extraction.
- No access-violation swallowing or claim that these changes fix the previously observed native crash.
- No game launch, release, package, or Git operation.

## Validation

Run focused harnesses after each batch. At closure run test.cmd, the 18-catalog/resource/glyph/font audits, and the production build. Review every changed path and check whitespace without invoking Git, because the user explicitly excludes Git operations in this task.

## Execution record

- Batch 0 (OQ-04): comments document Combo-item capture before popup option/group rendering, the path-specific `BeginCombo` meaning, prohibition on late `LastItemData` hover queries, and the explicit Examples content inset versus Indent/wrap behavior. No behavior changed. `test.cmd`: all 45 harnesses compiled and executed; localization/resource/font audit passed.
- Batch 1 (OQ-05): selector eligibility now has exhaustive eight-state truth-table coverage; Examples frame/content geometry has explicit symmetric 10×8 inset assertions at 14px and 21px UI scale. Source audit now checks selector capture ordering, presentation ownership, Camera State read-only boundary, localized prose across renderer/view modules, and shared tooltip-row use without pinning most checks to local variable names or exact gap formulas. Negative fixtures reject late selector capture, Camera State mutation, and raw user-facing prose. `test.cmd`: all 45 harnesses compiled and executed; 18-catalog/resource/glyph/font audit passed.
- Batch 2 (OQ-01): added an explicit optional-overlay C++ exception boundary with a defined Failed result and failure transition; renderer initialization and frame rendering terminate/tear down after exceptions instead of unwinding through `noexcept` or continuing a partial frame. DXGI factory/present/resize and Win32 message paths isolate overlay work while keeping native callbacks outside the catch scope; discovery-thread initialization is bounded too. Teardown inspects each ImGui backend's own initialized IO slot, so partial backend initialization has deterministic cleanup. The harness covers success, throwing overlay work, failure-transition containment, and exactly-once native pass-through placement; source audit also verifies callback ordering. `test.cmd`: all 45 harnesses compiled and executed; localization/resource/font audit passed. Production translation-unit compilation remains for the final production build.
- Batch 3 (OQ-02/OQ-03): added an input-owner `Close()` transition (hide, cancel rebind, clear consumed key) used only by terminal renderer disable; discovery restores the game cursor after the catch scope, while normal resize/recreation still uses the nonterminal shutdown path. Catalog initialization failure now aborts renderer initialization before the input hook can be installed, so no normal UI can show `[missing: …]`; camera features are untouched. Input harness covers hidden/unbound/no-capture state, and localization harness covers failed embedded-catalog readiness; source audit checks fail-closed ordering. `test.cmd`: all 45 harnesses compiled and executed; localization/resource/font audit passed.
- Batch 4 (OQ-06/OQ-08): replaced enum ordinal casts and parallel C-string lists with typed selector bindings that explicitly pair config enum value and documentation entry; combo labels and item IDs derive from the canonical entry. Localization harness checks every mapping against the exact documentation option and expected enum. Hotkey button keeps its visible localized/current key label and measured width but now has a stable hidden label plus action-scoped ImGui ID. Audit includes a negative unstable-ID fixture. `test.cmd`: all 45 harnesses compiled and executed; mapping, audit, localization/resource/font checks passed.
- Batch 5 (OQ-07): named and reused only shared semantic contracts: notification status colors now use the existing feature-status palette, tooltip text wrapping shares one named font-relative width, and Examples inset/next-option gap use explicitly named scale policy values. Existing numerical values and per-component geometry are unchanged. Localization/geometry harness and 18-catalog audit passed in the Batch 4 full test run.
- Batch 6 (OQ-09): replaced the placement dirty boolean with explicit Clean → PendingAfterMovement → FailedUntilNextMovement disposition. Save remains gated until left-button release; failed state does not retry on subsequent frames. A later actual position change re-arms one save of the latest coordinates; atomic INI save implementation is unchanged. Placement harness tests initial state, drag/release gating, a 120-frame no-retry window, movement-triggered recovery, successful completion, plus the existing atomic/concurrent write tests. `test.cmd`: all 45 harnesses compiled and executed; placement, localization, glyph/font, and 18-catalog checks passed. The source policy audit also passed.

## Closure

- Final `test.cmd`: 45/45 harnesses compiled and executed; all PASS.
- Final localization/catalog/resource/glyph/font audit: 18 catalogs, 182 keys, parity/placeholders/resources/profile coverage PASS. Arabic shaping/bidi/RTL and runtime rendering remain outside this offline audit.
- Final production `build.cmd`: PASS; refreshed `STALKER2CameraTweaks.asi` in the repository root.
- Manual changed-source whitespace scan: 18 intended source/plan paths reviewed; no trailing whitespace.
- No Git command (including `git diff --check`), game launch, release, or package operation was performed. No unresolved OQ finding remains. Native D3D12/Win32 failure injection is not available offline; those failure transitions are covered by deterministic state/boundary harnesses and source-contract audits rather than a live game session.
