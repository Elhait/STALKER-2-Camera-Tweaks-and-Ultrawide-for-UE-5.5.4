# Remaining Subsystem Automated Coverage Audit — 2026-11-15

## Objective

Continue the automation audit outside the completed camera writer transaction
batch. Find concrete gaps between production invariants, component interactions
and registered tests. Add a regression only when the production contract has a
deterministic seam and current tests do not exercise it. Do not add a generic
framework or broaden coverage by assertion count alone.

## Excluded completed scopes

- Camera writer / receiver identity / provider matrix from
  `AUTOMATED_CAMERA_TRANSACTION_COVERAGE_BATCH_2026-11-15.md`.
- Diagnostics OFF/ON unification, whose implementation and offline validation
  are recorded in `DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_REPORT.md`.
- Previously completed hook reset, SafetyHook lifetime, and overlay composition
  repair tests unless new current-source evidence identifies a gap.

## Bounded audit areas

1. Native hook installation, feature availability and partial failure:
   compare runtime wiring with cinematic transactional tests, feature-status
   tests, actual-runtime adapter scenarios and callback reset ownership.
2. Configuration and runtime settings interaction: inspect setting mutation,
   persistence and effective-state publication for missed cross-boundary
   failures; keep host-specific hardlink behavior explicitly SKIP where the
   environment denies the fixture.
3. Overlay/input lifecycle: map owner-thread admission, host rebind, failure,
   and capture revocation contracts to deterministic tests and existing
   actual-Renderer fixtures. Do not launch graphics workloads during this
   audit.
4. Localization/resource and packaging boundaries: compare generated/runtime
   inputs with existing catalog and release audits; do not repeat completed
   catalog checks without a source change or uncovered contract.

## Concrete gaps accepted within this audit

The localization/resource boundary is already enforced by the catalog/resource
audit and localization harness. The release archiver
`tools/build/package_release.ps1` had no registered deterministic contract
test. A bounded, process-supervised PowerShell check is now registered in
canonical `test.cmd`. It covers the exact ZIP allowlist (including INI
exclusion), ASI content hash, invalid extension, missing required input,
rejection of an input path as output without modifying the source, and
preservation of a pre-existing archive. The exact-path collision guard itself
is shadowed by the `.zip` extension check for the fixed `.asi` input; the test
does not claim to reach that guard. No release behavior change was needed.

The release publisher `tools/build/publish_asi.ps1` also had no registered
contract test despite owning destination serialization, atomic replacement,
and staging-file cleanup. A bounded PowerShell check is now registered in
`test.cmd`; it verifies replacement and source hash, identical-input no-op,
missing-source preservation, and rejection of a concurrent publisher holding
the destination mutex. No publisher behavior change was needed.

## Decision rule

For each area, report concrete production entry points, the invariant, existing
tests and exact uncovered branch. Classify the result as defect, test gap,
accepted runtime-only boundary, or adequately covered. If an implementation
change is needed, stop and prepare a scoped follow-up plan; this audit itself
does not authorize unrelated production behavior changes.

## Validation

- Read current architecture/safety contracts, source and canonical registry.
- Use static review first. Run only focused, bounded harnesses needed to
  adjudicate a concrete deterministic gap; supervise and verify process cleanup.
- Do not run a game or graphics fixture without a separately established need
  and available PC.
- Preserve staged and unstaged user work; do not reset, stage, or clean.

## Initial triage

- Native cinematic install/commit failure ordering is covered by the actual
  `InitializeTransactional` helper under returned failures and exceptions at
  each phase, including reverse rollback. Runtime-adapter coverage separately
  exercises partial startup and reset ownership. No additional hook-transaction
  test is justified by the current bounded inspection.
- Runtime-setting mutation plus persistence ordering is covered by
  `semantic_snapshot_harness`, including serialized concurrent commits and the
  accepted-but-unsaved state after persistence failure. No uncovered
  deterministic Apply/Persist branch was established in the reviewed API/UI
  path.
- Overlay lifecycle tests cover host rebind, minimize/restore, DPI/geometry,
  capture revocation, attachment commit failures, and owner finalization paths;
  existing actual-Renderer/DComp fixtures cover selected hidden-window resource
  lifecycles. Live driver/DComp presentation and visual behavior remain runtime
  evidence limits, not deterministic harness omissions.
- Evidence and disposition are recorded in
  `research/reports/REMAINING_SUBSYSTEM_AUTOMATED_COVERAGE_TRIAGE_2026-11-15.md`.
  The bounded production subsystem inventory is now recorded there; it is not
  an exhaustive runtime correctness claim.
- Localization catalog/resource contracts are adequately covered by the
  current harness and registry audit. The release archive contract test was
  the sole concrete follow-up from that area and has now been added.
- The focused release package contract passed all five assertions. The full
  non-graphics canonical suite passed 62/62 registered invocations with 0
  failures, 1 explicitly skipped hardlink subcase, and WARP graphics coverage
  omitted by request. PowerShell parsing and `git diff --check` passed;
  process-tree cleanup was verified. No production source was changed.
- The focused release publisher contract passed all four assertions. After
  registering both release contracts, the full non-graphics suite again
  passed 62/62 harness invocations, 0 failures, 1 explicit hardlink SKIP, with
  WARP omitted; process-tree cleanup was verified.
- Remaining production areas were mapped to current coverage: hook scanning
  and instruction validation (`signature_scanner_harness`,
  `instruction_validator_harness`, actual runtime adapter); memory and
  viewport selection (`memory_harness`, `viewport_harness`); config parsing,
  migration and persistence (`config_persistence_harness`, settings API and
  semantic snapshot tests); startup, DLL/module residency and worker shutdown
  (`startup_timeline_harness`, startup journal, runtime adapter, module
  ownership and worker lifecycle); diagnostics (diagnostics gate, telemetry,
  evidence replay and actual adapter cases); release packaging and publishing
  (the two new contracts). No additional deterministic gap was established in
  this bounded source-to-test inventory.
- Runtime reachability against a live game image and actual display-driver
  presentation remain runtime-only evidence. The non-graphics run does not
  claim those behaviors or WARP graphics validation; the hardlink fixture is
  still an explicit host SKIP.
