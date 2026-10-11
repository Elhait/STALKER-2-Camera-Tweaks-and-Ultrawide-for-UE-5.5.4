# Test Infrastructure Hybrid Migration Plan

**Status:** Complete — H0–H4 validated (2026-10-12). Hybrid CMake/Ninja compilation is canonical behind `test.cmd`; the manifest runner remains serial.
**Decision direction:** Option B — CMake + Ninja builds the canonical harnesses; `test.cmd` remains the public entry point and executes the existing tests in their established order.
**Evidence:** [Architecture audit](TEST_INFRASTRUCTURE_ARCHITECTURE_AUDIT.md), [performance baseline](TEST_EXECUTION_PERFORMANCE_BASELINE.md), [options comparison](TEST_INFRASTRUCTURE_MODERNIZATION_OPTIONS.md).

## Goals

- Make unchanged-input test runs incremental through Ninja dependency tracking.
- Keep the 60 current harnesses, their compiler contracts, assertions, launch arguments, execution order, fresh-process isolation, and current execution multiplicity unless a separate decision approves a change.
- Keep `test.cmd` as the canonical command users run.
- Make every failed compile or test process, including a negative Windows exception status, stop the run and produce a nonzero result.
- Keep test objects separate from production/diagnostic objects and from incompatible harness flag groups.
- Establish complete old/new inventory and runtime parity before changing the canonical build path.

## Baseline and decisions to settle

Recorded baseline: 60 unique harness sources, 61 compile commands, 430 source-compilation occurrences, 64 direct executable invocations, 102 native-adapter cases in OFF/ON modes, 408 child processes across the two currently registered matrix invocations, and four N06-M01 Zoom invocations. `test.cmd` median was 391.224 seconds over two runs. Instrumented compile/link time was 296.647 and 303.243 seconds. These are two-run measurements, not a guarantee for future hardware or builds.

**H0 decision and adjusted parity baseline (2026-10-11):** The duplicate second native-adapter compile and matrix run are removed. The two invocations had the same production flags, source inputs, executable output and no distinct arguments; both ran the same self-contained scenarios in fresh child processes. The second command had previously carried diagnostic-only compile definitions, but Diagnostics Unification removed those definitions and moved the OFF/ON behavior to the runtime gate, making the two current compile/run blocks identical. No comments, runner contract or fixture state indicates that repetition is an intentional independent regression. Preserve the earlier 64/408 figures above as the pre-H0 historical baseline; the approved post-H0 parity contract is **60 unique harnesses, 60 compile targets, 369 legacy source-compilation occurrences, 63 direct executable invocations, 102 named native-adapter cases × OFF/ON = 204 child processes, and four N06-M01 Zoom invocations**. The first complete old-runner baseline captured before H0 changes completed with exit 0 in 451.452 seconds; its log SHA-256 is recorded in the implementation report.

Before implementation:

1. **Negative exit codes:** treat as an independent correctness prerequisite. The approved change should detect every nonzero result and preserve failure propagation through the batch failure path. Do not silently couple this fix to source-registry or Ninja changes.
2. **Second adapter matrix — resolved in H0:** removed as an accidental duplicate after checking the staged change history, runner order, harness process isolation, current flags, and absence of any distinct purpose or shared state. This is recorded as a separate H0 runner change; H1–H4 compare against the adjusted 63-command / 204-child parity contract above.
3. **Performance expectation:** 98–178 seconds plus future build/configure overhead is a measured-budget subtraction model. A 2–3 minute warm-run objective is a target to measure after parity, not a promise or a reason to skip tests.

## Proposed batches

Each batch has an explicit stop condition. Passing one authorizes only the next batch in this plan, not a later full CTest conversion.

### H0 — Exit-status correctness and duplicate-run decision — Complete

**Scope:** repair nonzero exit handling in `test.cmd` and its audit; separately document the duplicate-matrix decision.

**Completed 2026-10-11:** all 133 positive-only `if errorlevel 1` checks were changed to an exact nonzero test. An isolated subprocess regression passed for success, positive failure 7, negative NTSTATUS `-1073741819`, propagated failure through the batch failure path, and failed compilation while a stale output stub existed (stale output was not run). The duplicate adapter compile/run was removed after the separate review documented above. The first unchanged legacy full run before these edits completed exit 0 in 451.452 seconds.

**Acceptance:** isolated runner regressions cover ordinary positive failure, a negative NTSTATUS such as `0xC0000005`, success, and compile failure with an old executable already present. Every nonzero child/compiler status is surfaced, later tests do not run, and the runner itself exits nonzero. Recheck the `:fail` capture/propagation path, including batch/call boundaries. The runner audit must reject a guard that accepts negative values. Record whether duplicate execution is retained or removed and why.

**Stop:** do not migrate compilation until the negative-status regression proves fail-fast behavior. **Passed:** `test_cmd_exit_code_contract.ps1` returned 0; all four fixtures PASS.

### H1 — Canonical harness registry and compile-contract mapping

Create one explicit test-build manifest for all 60 unique harnesses. It must describe, per harness, entry source, additional compiled sources, production-registry-derived inputs where applicable, definitions, include directories, language/runtime/warning/optimization flags, libraries, resource inputs, generated inputs and output name. Record ordered runner invocations and arguments as an auditable parity contract, including repeated invocations if still approved.

Use a registry audit to compare the manifest against repository harness files and the executable/argument sequence in `test.cmd`. Inventory counts must distinguish unique sources, build targets, direct runner commands, named native scenarios, diagnostic modes, and child-process executions. Do not infer completeness from basename-set equality alone.

Search auxiliary build/evidence scripts for code that parses raw `cl` lines from `test.cmd`. The historical N06 `BuildProbe` is known to do this. Update any affected helper to read the authoritative production/test registry while preserving its build identity checks and output contracts; do not reclassify or overwrite N03–N06 evidence statuses.

**Important source rule:** the native-adapter and N06-M01 executables each include the actual `src/plugin/runtime.cpp` through their harness TU. Compile it exactly once per executable under that executable's intended test macros; do not also add it as a standalone source. The other 59 production registry inputs are shared only when their effective source, flags, includes and definitions match.

**Stop:** missing source, unexpected definition, ambiguous argument list, duplicate output, stale registry, or a target whose old compiler contract is not understood.

**Completed 2026-10-11:** `tests/test_harnesses.json` now contains the 60 reviewed targets, their ordered translation units, per-target definitions/includes/link inputs/resource usage, common MSVC contract, 63 ordered runner invocations with argument tokens, 102 named native adapter cases and the four N06-M01 runs. `tests/runner/test_cmd_audit.ps1` matched this registry against the current 60 source files and the pre-migration `test.cmd`; compiler source order, common flags, per-target definitions, include order, libraries/resources and run order/arguments all passed. The historical N06 BuildProbe now reads `tools/build/production_sources.json` rather than scraping `build.cmd`; its output identity checks and status records are unchanged. H1 audit output: 60 registry targets, 60 repository harnesses, 63 invocations, 102 cases / 204 child processes, three resource consumers, zero audit failures.

### H2 — Complete isolated Ninja candidate — Complete

Add all 60 unique executable targets and one aggregate test-harness build target. Emit harness executables at the current runner paths under `build-artifacts/tests/` so its execution commands remain unchanged. Place Ninja objects, dependency databases, generated files and intermediate resources below a dedicated ignored test-build directory. Never reuse production or diagnostic objects merely because source names match.

Preserve `/std:c++latest`, `/O1`, `/MT`, `/EHsc`, `/W4`, `/utf-8`, effective include paths, per-harness definitions and explicit libraries. Do not apply production `NDEBUG` to ordinary test targets: language-state-transition and localization assertions are part of current semantics. Keep distinct flag groups isolated.

Preserve the current resource contracts. The current CMD links `localization_resources.res` for the native-adapter matrix, game-derived Zoom, and localization harness. CMake currently does not link it to its two adapter targets. Attach the resource object to every corresponding target and validate its contents/IDs; a build-order dependency by itself is insufficient. Keep locale/font assets, RC source, resource-ID header and generator inputs in the dependency graph.

Track direct and transitive headers with MSVC/Ninja dependencies. Keep the localized UTF-8 `/showIncludes` prefix handling. Explicitly track `production_sources.json` as a configure input (or provide equivalent configure-dependency invalidation) before removing routine reconfiguration. Track generated INI header and all RC/resource inputs. Preserve a recovery path for old/stale Ninja caches and verify separate production/diagnostic/test build directories.

**Stop:** any mismatch in output path, macro contract, assertion enablement, linked resource, runtime-owner count, include dependency, language mode or build-profile isolation.

**Completed 2026-10-11:** CMake defines all 60 manifest harness targets plus one non-test exit-status support executable under `build-artifacts/cm/test-suite`; final harness executables remain at the existing `build-artifacts/tests/` runner paths. All 374 initial Ninja actions completed with exit 0 in 79.117 seconds. Actual compile commands contain `/O1 /MT /std:c++latest /EHsc /W4 /utf-8`; no global `NDEBUG` was added. The two adapter targets share only their 59 identical production dependency objects; each still compiles its own harness TU, which includes `runtime.cpp` once. `ninja -t deps` confirms the include is tracked. Native-adapter, N06-M01 and localization harness link inputs include the `.res`; CMake and legacy resource outputs have identical SHA-256 `45EF935B38DDD0460F4A4CE75303583270DB86CE9DE8950C42952A91F2D6B2F3`. Resource generation tracks the RC, resource-ID header, all 18 locale catalogs, and embedded font/license assets. The initial no-op build emitted the localization preflight only and compiled/linked no C++ targets. Test objects remain isolated under the dedicated test build tree.

Localized MSVC dependency scanning was verified: Ninja dependency records include both normal headers and the actual `src/plugin/runtime.cpp` include, without assuming an English `/showIncludes` prefix. An overly deep throwaway source copy hit Windows path-length/compiler temporary-file limits; a short-path ignored copy compiled successfully. No original source files were changed during this check.

**H2 gate: PASS.**

### H3 — Keep `test.cmd` as build-and-run entry point — Complete

The batch file initializes the supported MSVC environment, performs required audits/generation, invokes CMake configure when inputs/toolchain require it, asks Ninja to build all canonical test targets, then executes the existing runner sequence serially. It must not build only the two current adapter targets. No CTest substitution is in this phase.

Retain immediate nonzero checks at every boundary: audit/generator, RC, configure, Ninja build, each test executable, and failure return. Do not let an old output binary make a failed build look successful. Preserve console diagnostics and sufficient raw logs to identify the first failed target or test.

Update `test_cmd_audit.ps1` to compare registry targets and actual run entries including arguments and multiplicity. Keep it fail-fast and ensure a newly added harness cannot be omitted from either the build or execution inventory.

**Stop:** exit-code regression, altered test order/arguments, stale binary acceptance, missing log detail or incomplete parity.

**Completed 2026-10-11:** `test.cmd` initializes the supported VS2022 MSVC environment, UTF-8 code page and isolated temporary directory; audits the registry, configures the dedicated Ninja tree, builds the complete `test_harnesses` aggregate, reruns status-contract fixtures, then runs all manifest entries serially. It no longer invokes `cl.exe` directly, repeats localization audit outside the build graph, or executes the duplicate adapter matrix. It remains CRLF, with `.gitattributes` declaring `text eol=crlf`. Audit passed 60/60 harnesses, 63 runner invocations, 204 native children, three resource targets and zero missing inputs. The first complete canonical run passed with exit 0. Warm timings are recorded in the implementation report.

The timeout fixture showed that this Windows environment rejects `taskkill /T /F` with an authentication error. The runner now directly terminates the harness process with `Process.Kill()` and returns 124; negative exit, access violation and timeout contract tests all pass. Test harnesses execute directly and do not spawn child processes.

**H3 gate: PASS.**

### H4 — Full parity and incremental validation — Complete

The old canonical path was recorded before H0; the approved post-H0 parity contract governs the new path. Compare:

- 60/60 unique harness sources and corresponding executable outputs.
- Effective compiler/link commands, definitions, include paths, libraries, generated inputs and resources.
- All approved direct run commands, arguments, order, process isolation and scenario/mode counts: 63 direct commands, 204 native-adapter child executions and four N06-M01 Zoom executions.
- Per-run exit status, named case output, localization/resource/glyph results, and absence of tests skipped after a failure.
- Failure injection for positive/nonzero/negative status, compiler failure, crash, and timeout; each must fail the aggregate command.

Exercise incremental invalidation in an isolated candidate: no-op build; one harness source; a shared production header; `runtime.cpp` as included by adapter harnesses; locale JSON; resource ID/RC/font input; generated INI header; registry JSON; missing generated output; changed compile definition/toolchain. Confirm Ninja rebuilds the required objects/outputs and does not rebuild unrelated targets. Use a throwaway build/source fixture or isolated copy for perturbation tests; never mutate and restore user source to manufacture a green result.

Capture at least three canonical full warm timings after correctness parity, with unchanged source. Report build time and execution time separately. Compare the median against the 2–3 minute objective; if tests dominate or environment noise keeps it higher, document the real result without reducing validation.

**Switch gate:** all parity items pass; fail-fast negative status works; no stale objects; resources and assertions match; no unresolved native-runtime or N03–N06 statuses are reclassified. Review complete diff, staged and unstaged changes, and the baseline index hash before changing the default path.

**Completed 2026-10-12:** H4 parity and incremental invalidation passed. The complete record, raw-log hashes, timings and limits are in [the implementation report](TEST_INFRASTRUCTURE_HYBRID_MIGRATION_IMPLEMENTATION_REPORT.md). Four unchanged canonical warm runs returned exit 0 in 108.336, 115.401, 123.486 and 145.589 seconds (median 119.444 seconds). A standalone runner measurement under the canonical isolated TEMP/TMP environment passed in 107.822 seconds. A no-op configure/build measurement completed in 19.689 seconds, with no C++ compile/link work; it includes MSVC environment setup, CMake configure and the localization preflight, so component timings are not additive or directly comparable to whole-run timing. The baseline legacy run captured before H0 returned exit 0 in 451.452 seconds; the earlier audit's two-run median was 391.224 seconds.

The full clean Ninja candidate built 60 harness executables plus the non-test status probe in 374 actions (310 object compiles, 61 executable links and three generated/preflight actions), exit 0 in 79.117 seconds. Registry audit passed 60/60 harnesses, 63 ordered runner entries, 102 native case names × two diagnostic modes = 204 native child processes, four N06-M01 invocations and three resource consumers. All four Zoom directions/modes and the 102-case actual-adapter matrix passed. The seven failure-contract fixtures passed: success, positive nonzero, negative NTSTATUS, stale executable after compile failure, registry runner negative status, actual access violation and timeout.

Incremental checks were performed in a complete ignored short-path source copy. No-op produced only the intended localization preflight. One harness source caused only that target's compile+link; a shared header caused its dependent targets; included `runtime.cpp` caused exactly the two adapter harness compile+links; locale JSON, RC and font assets caused the resource compile and its three consumer links; the resource-ID header caused its actual transitive dependents; a missing generated INI header was regenerated and only dependent targets rebuilt. A touched registry caused CMake to reconfigure without compiling C++; changing a fixture-only per-target definition updated the Ninja command and dirtied only that target's translation units/link. Original source content and timestamps were not modified for these perturbations.

All normal, error, resource and localization checks passed. The canonical `test.cmd` remained CRLF (`text eol=crlf`). `.git/index` SHA-256 remains the captured baseline `A7C0C921F1C90E638719EB6ED3C11DE3AC0073B33B79621CB6D0CBBD5CA05380`; pre-existing staged entries were preserved. The authorized duplicate matrix removal is explicit in the H0 record; no N03–N06 status changed. See the implementation report for the remaining measurement caveats.

**H4 gate: PASS. H0–H4 campaign: COMPLETE.**

## Out of scope

- Full CMake/CTest migration or replacing `test.cmd` as the user entry point.
- Parallel test execution, changed scenario multiplicity, or fewer assertion runs without separate evidence and approval.
- Production/runtime changes, live game tests, or edits to N03–N06 evidence statuses.
- Reusing objects across incompatible definitions/profiles or relying on output-file existence as dependency evidence.

## Required final report

Record each batch's changed files, actual flags/targets, compiler and runner exit codes, inventory and scenario counts, resource/assertion parity, incremental invalidation cases, raw-log hashes, old/new full timings and source/index preservation result. Report the implemented warm-run median rather than the 98–178 second subtraction model. State the duplicate-matrix decision explicitly. If any gate fails, keep the old canonical path active and stop for a focused review.

**Approval boundary:** H0–H4 implementation was explicitly approved in the user message dated 2026-10-11. Continue automatically through successful batches; stop only if a real ambiguity or parity gate failure appears. Do not interpret this approval as authorizing unrelated source/runtime changes.
