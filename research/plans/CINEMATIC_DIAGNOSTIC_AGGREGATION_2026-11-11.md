# Cinematic diagnostic aggregation

Approved by the project owner on 2026-11-11. Status: implementation and offline validation complete. See [implementation report](../reports/CINEMATIC_DIAGNOSTIC_AGGREGATION_2026-11-11.md); in-game volume/performance measurement has not been performed.

Evidence: ASI `55DB…D872`, runtime log 8,486,585 bytes / 14,228 lines. 6,866 BEFORE/AFTER trace lines and 6,835 identity-change lines account for 97.6% of bytes; 6,832 identity events only change RBX. Production source/output ownership already permits these transitions.

Contract: source observations, ownership changes/conflicts, actual correction-decision changes and ENTER/EXIT retain full records. Same-source output transitions and repeated samples aggregate in fixed-size observational state. Numeric samples retain first/last/extrema without driving emission through an arbitrary FOV delta threshold. Diagnostic selection never changes writer registers, state, ownership or recovery. No unbounded destination cache, timers or new shutdown hook.

Scope: runtime diagnostic owner/helpers/call sites, one pure aggregation header, existing diagnostic/actual-adapter harnesses and current diagnostic documentation. No camera algorithms, hooks, resolver, input, Overlay geometry, INI or Git operations. Preserve current ASIs. Missing cinematic ENTER remains an independent open issue.

Implementation sequence:
1. Add bounded per-source/generation/lifecycle aggregation; full events remain visible and ordinary A/B output cycling only updates counters. Publish counters on scope change, new generation, EXIT, disable and existing controlled Runtime reset.
2. Select full cinematic writer records from production semantic decisions, including exact mutateFov and validity changes, rather than output pointer or numeric drift. Full BEFORE/AFTER snapshots are sampled only for such records; raw numeric first/last/extrema remain in summaries. Retain independent existing continuation trace.
3. Prove with thousands of alternating outputs through the real adapter: bounded emitted record count, correct aggregate counts, immediate source/conflict/decision events including tiny numeric Apply→Skip, diagnostics OFF/ON production parity, lifecycle/cleanup flush and reset isolation. Run contained focused tests and canonical `test.cmd -NonGraphics` sequentially on the confirmed-free PC. No game, debugger or graphics fixtures.

Expected files: `src/diagnostics/writer_diagnostic_aggregation.hpp`, `src/plugin/runtime.cpp`, `tests/diagnostics/camera_writer_identity_harness.cpp`, `tests/plugin/runtime_native_adapter_harness.cpp` and included helper, test dependency registry, architecture/safety diagnostic contract and implementation report.

Failure/lifetime boundaries: existing CameraCorrectionGate serializes adapter aggregation; existing runtime callback drain precedes controlled reset. Emit records through the existing deferred Log path. No file I/O is added to DllMain; uncontrolled Windows process termination cannot guarantee a final summary, so EXIT and generation boundaries retain the primary evidence. No task closure or runtime performance claim from offline PASS alone.

Verified outcome: focused 11/11 and final canonical `test.cmd -NonGraphics` 62/62 PASS, 224 fresh native-adapter processes with graphics omitted. On 10 000 output-cycling calls with numeric drift, ON emitted one writer-state record and one BEFORE/AFTER pair; the boundary summary retained 9 999 output transitions. Full fixture including additional semantic/lifecycle events wrote 96 lines / 36 439 bytes ON, 12 operational lines / 1 957 bytes OFF. Source/conflict/one-ULP Apply→Skip events, lifecycle flushes and controlled reset passed. No standalone Release build or installation; current ASIs preserved. Runtime log-volume confirmation and the independent missing-ENTER investigation remain follow-ups.
