# Native runtime evidence B1–B7 — approved task plan

Date: 2026-10-09. Authority: user-supplied Comprehensive Native Runtime Evidence
Audit & Coverage Expansion task. Independent of decomposition implementation.

## Scope and acceptance

1. Inventory current B1–B7 responsibilities, actual adapter assertions, domain
   harnesses, source contracts and historical runtime evidence. Record exact
   source locations and evidence limits rather than inferring coverage from names.
2. Review native register/ABI/trampoline, pointer validity, synchronization,
   lifecycle, publication, settings/persistence and shutdown contracts.
3. Add deterministic test-only coverage for meaningful missing cases through
   actual production adapters or explicitly identified production leaf helpers.
   Preserve existing tests. Label isolated/domain evidence separately.
4. Run focused tests, full test.cmd, ordinary/supported-diagnostic actual-adapter
   matrices with Diagnostic Mode OFF/ON, locale/resource audit, production and
   diagnostic builds. Capture raw logs, exit codes and artifact/source hashes.
5. Produce research/evidence/runtime-decomposition/
   NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md with responsibility and native
   contract matrices, existing/new assertions, findings, unverified/blocked
   branches and concrete live-runtime procedures. No skipped/incomplete check
   counts as PASS.

## Expected files and order

- Read src/plugin/runtime.cpp and its existing helpers, SafetyHook implementation,
  build/test registry, decomposition records and relevant domain harnesses.
- Write tests/plugin/runtime_native_adapter_harness.cpp, narrowly scoped new
  test fragments/tools if needed, and the dedicated evidence report.
- Keep generated executables, build products, raw logs and hash manifests under
  ignored build-artifacts/native-runtime-evidence/2026-10-09/.
- Use CAMERA_TWEAKS_OUTPUT_NAME and CAMERA_TWEAKS_OBJECT_DIR to build separate
  audit ASIs without overwriting root/release/installed artifacts.
- Review final relevant changes and verify production-source preservation.

## Non-goals and stop conditions

No production source, camera semantics, ABI/hook behavior, defaults, additional
runtime owners, synchronization changes, release files, game launch or ASI
installation. Do not change B1–B4 closure decisions, implementation plan or
move ledger; do not implement or approve B5–B7 decomposition.

A branch requiring production edits, owner API, synchronization redesign,
unverified ABI assumptions or live game execution is documented as blocked or
live-runtime-required; independent safe work continues. Tests exercising an
offline callback body do not prove generated trampoline execution. Static
analysis and historical runtime observations remain distinct evidence levels.

## Current status

Evidence work and safe test-only expansion complete; awaiting user review/closure.

- Dedicated report: `research/evidence/runtime-decomposition/NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md`.
- 27 new names; existing 64 retained. 91 cases × OFF/ON × 2 profiles = 364
  focused fixture processes PASS; full test.cmd separately repeated both matrices.
- Full test.cmd exit 0; 59/59 registered harnesses PASS; locale/resource audit
  18/18 PASS; glyph harness PASS; production and diagnostic builds exit 0.
- Final preservation PASS: 313 protected production/vendor/catalog/docs/root
  release/config/decomposition files unchanged, identical manifest hashes.
- N01–N03 static conditional risks and other live/blocked branches are documented,
  not repaired or silently counted as verified runtime behavior.
- No production changes, game launch, ASI installation, release updates or
  decomposition approvals/closures were performed.

Task-log closure remains contingent on user-confirmed task closure. Remaining
blocked/live branches require separate authority; the evidence task does not
implement or close B5–B7.
