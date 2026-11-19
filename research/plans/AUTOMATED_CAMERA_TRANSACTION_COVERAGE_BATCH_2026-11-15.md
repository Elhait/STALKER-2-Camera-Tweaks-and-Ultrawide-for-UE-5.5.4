# Automated Camera Transaction Coverage Batch — 2026-11-15

## Objective

Close the concrete deterministic coverage gaps left by the receiver-identity
repair, using existing production transaction APIs and the canonical test
runner. Do not add a test framework or alter camera algorithms.

## Scope and planned files

- Extend `tests/plugin/camera_writer_transaction_harness.cpp` with a bounded,
  deterministic matrix over receiver replacement, native FOV changes, aspect
  changes, lifecycle edges and existing policy choices.
- Compare the same deterministic transaction sequence with diagnostics OFF
  and ON. Compare production outputs and ownership state; diagnostic emissions
  may differ.
- Extend `tests/runner/test_cmd_exit_code_contract.ps1` so a successful
  harness with a skipped subcase must produce a named `SKIP` and nonzero
  `skipped_subcases` summary count.
- Update `research/reports/V4_00_AUTOMATED_TEST_COVERAGE_AUDIT_2026-11-15.md`
  with exact added coverage, remaining engine-only limits, and test results.

## Acceptance criteria

1. Each matrix row runs through production writer/lifecycle transactions,
   changes at least one FOV or aspect value, and asserts the selected policy,
   output and baseline source/freshness.
2. The matrix includes changes before, during and after Cinematic/Dialogue
   ownership, including recovery and a receiver replacement.
3. A deterministic fixed seed drives a bounded sequence; a failure reports the
   seed and step so it is reproducible. No random or timing-dependent test.
4. Diagnostics OFF and ON produce identical correction outputs, lifecycle
   decisions and baseline identity/freshness for the same inputs.
5. A skipped subcase is surfaced by name and in aggregate totals; overall test
   exit remains governed by FAILs, while SKIP is never represented as PASS.
6. Production behavior is unchanged in this batch. No Release ASI or game
   runtime run is part of this deterministic coverage task.

## Validation

- Build and run the changed transaction and runner-contract harnesses under the
  project process-tree supervisor.
- Run full `test.cmd` after targeted cases pass; record every invocation,
  aggregate PASS/FAIL/SKIP counts, timeout status and verified process cleanup.
- Review `git diff --check` and the complete relevant diff. Preserve existing
  staged and unstaged user changes.

## Boundaries

This establishes deterministic transaction behavior only. Native engine
callback ordering and rendered-view continuity remain runtime evidence, not
claims of this batch. The hardlink ownership case remains a named host-specific
SKIP when Windows denies fixture creation; the test runner must continue to
show that exclusion explicitly.

## Execution record

- Targeted `camera_writer_transaction_harness`: PASS after aligning disabled
  Gameplay pass-through expectations with the production contract and making
  camera readback aspect follow each provider-matrix row.
- Full `test.cmd`: PASS, 62/62 runner invocations, 0 FAIL, 1 named hardlink
  subcase SKIP (`error=5`). Runner contract tests passed synthetic
  skip-visibility and aggregate-fail-continuation cases.
- Fixed seed `0x4F9A31C7`: 48 transactions per diagnostics setting, with
  equivalent output, mutation decision, context, and baseline source/freshness
  under Diagnostics OFF and ON.
- Process-tree supervisor reported cleanup verified. Post-run checks found no
  game or build/test process left by the run.
- No production camera or FOV behavior changed in this batch.
