# Test Infrastructure Hybrid Migration — Implementation Report

**Completed:** 2026-10-12  
**Decision:** Hybrid — CMake + Ninja builds the test executables; `test.cmd` remains the canonical entry point and a serial registry runner executes them.  
**Scope:** H0–H4 only. No production/runtime behavior, camera semantics, test assertions, N03–N06 statuses, or release artifacts were changed.

## Result

The canonical test build is now incremental. A warm run does not compile or link unchanged harnesses, and any nonzero child result—including signed Windows exception codes—fails the run. The full post-migration suite passed with the approved H0-adjusted inventory.

| Measure | Legacy captured baseline | Hybrid result |
|---|---:|---:|
| Full run | 451.452 s, one captured run, exit 0 | 108.336 / 115.401 / 123.486 / 145.589 s, all exit 0 |
| Median of reported runs | Earlier audit median: 391.224 s over two runs | 119.444 s over four runs |
| Compile/link on unchanged warm build | Repeated full compilation | 0 C++ compiles and 0 links |
| Full initial Ninja candidate build | — | 79.117 s, exit 0, 374 Ninja actions |
| Standalone runner, canonical TEMP/TMP | — | 107.822 s, exit 0 |
| No-op configure + build stage | — | 19.689 s, exit 0; includes VS environment setup, configure and localization preflight |

Against the single captured 451.452-second legacy run, the four-run hybrid median is 3.78× faster (73.54% less elapsed time). The earlier audit’s two-run median gives a more conservative comparison: 3.28× (69.47% less). The isolated phase measurements are from separate invocations and are not additive; the whole `test.cmd` timings are the primary result. Variance between the four warm runs is material, so these figures describe this machine/session, not a guarantee for other systems.

## H0 — Exit-status correctness and duplicate matrix

- Replaced positive-only batch exit checks with exact nonzero checks. The `:fail` path captures and returns the original status, and its local failure variable is cleared at entry so an inherited environment value cannot shadow it.
- Confirmed that the old second native-adapter compile/run was an accidental duplicate: same executable, sources, current definitions, cases and fresh-process behavior, with no distinct state or purpose. Removed it as a separate approved H0 decision.
- Preserved the old 64 direct invocations / 408 adapter child processes as historical evidence. The approved post-H0 parity contract is 63 direct invocations and 204 adapter child processes.

The exit-contract suite passed all seven checks:

1. Success (`0`) continues.
2. Positive failure (`7`) stops and propagates.
3. Negative NTSTATUS (`-1073741819`, access violation) stops and propagates.
4. Simulated compile failure (`19`) does not execute a stale output executable.
5. Registry runner preserves a negative child exit.
6. An actual access-violation fixture is reported as `-1073741819`.
7. Timeout terminates the direct harness process and returns `124`.

The timeout fixture exposed that this Windows environment rejected `taskkill /T /F` with an authentication error. Since the registered harnesses are direct executables and do not spawn child processes, the runner now calls `Process.Kill()` on the timed-out harness itself and then returns `124`. The timeout regression passes. The same TEMP/TMP isolation as `test.cmd` is required for direct runner-only measurements: without it, the placement persistence harness correctly failed its environment-dependent checks; with canonical isolation it passed.

## H1 — Canonical test registry

Added `tests/test_harnesses.json` as the build/run contract for all 60 harnesses. It records each entry source, ordered additional sources, target-specific definitions and include directories, libraries, embedded resource usage, common compiler/runtime contract, final output path, ordered invocations/arguments, named adapter scenarios and four N06-M01 arguments. The production source registry remains the authority for the 59 shared adapter dependencies; N06 BuildProbe no longer scrapes raw `build.cmd` lines.

The registry audit passed:

- 60 registry targets = 60 repository harnesses.
- 63 ordered runner entries, with four Zoom arguments: `zoom-in off`, `zoom-in on`, `zoom-out off`, `zoom-out on`.
- 102 unique adapter case names, each tested with Diagnostics OFF and ON: 204 fresh child processes.
- Three resource consumers and zero missing inputs.
- Source order, flags, per-target definitions, include paths, libraries/resources, runner multiplicity/order/arguments, and runtime owner include-once contract matched the captured legacy invocation inventory.

The test inventory is not conflated with process count: the native adapter is one registry invocation that itself executes 204 isolated scenario processes; the four N06 entries are four additional direct runner invocations.

## H2 — CMake/Ninja graph and contracts

- Added an aggregate `test_harnesses` target for all 60 manifest harnesses and a separate non-test `test_cmd_exit_status_probe` support executable.
- Harness executables remain under `build-artifacts/tests/`, so test identity and launch arguments do not depend on the build-system change.
- Ninja objects, dependency databases, generated headers and resource intermediates live in `build-artifacts/cm/test-suite`; they are not reused from production or diagnostic build trees.
- The initial candidate completed 374 actions: 310 object compilations, 61 executable links (60 harnesses plus the support probe), and three generation/preflight actions. Exit 0, 79.117 seconds.
- Effective test options include `/O1 /MT /std:c++latest /EHsc /W4 /utf-8`; the generated commands use MSVC 19.44.35224.0, CMake 3.31.6-msvc6 and Ninja 1.12.1. Target definitions and include paths come from the reviewed manifest. No global `NDEBUG` was added; only the two adapter harnesses retain the `NDEBUG` present in their legacy contract.
- The two adapter targets share the 59 matching production dependency objects. Each still compiles its own harness translation unit, where `runtime.cpp` is included exactly once. Ninja’s compiler dependency database tracks that include and ordinary transitive headers.
- The native adapter, N06-M01 and localization harness link the generated `.res`, not merely depend on its build target. Legacy and CMake resource outputs match byte-for-byte: SHA-256 `45EF935B38DDD0460F4A4CE75303583270DB86CE9DE8950C42952A91F2D6B2F3`.
- The resource command depends on the `.rc`, resource-ID header, all 18 locale JSON files and embedded font/license inputs. The generated INI header is an explicit generated input with automatic recovery if missing.
- Ninja’s localized MSVC scanner is active (`deps = msvc`, `/showIncludes`) with the configured Russian prefix `Примечание: включение файла:`. This avoids dependence on an English-only scanner.
- The registry JSON files are explicit CMake configure dependencies, so changes reconfigure the graph rather than leaving stale target/source lists.

## H3 — Canonical `test.cmd`

`test.cmd` now performs, in order:

1. Supported VS2022 x64 developer environment and UTF-8 console setup.
2. Isolated temporary directory setup.
3. Canonical registry audit.
4. CMake configure in the dedicated test tree, followed by Ninja build of the complete aggregate target.
5. Exit-code/crash/timeout/stale-output regression fixtures.
6. Serial execution of the 63 registry entries with immediate nonzero failure propagation.

It does not invoke `cl.exe` directly, run a separate duplicate adapter matrix, or run the localization audit outside the build graph. The build’s locale preflight runs once as a graph dependency. CTest definitions remain available for focused development use; `test.cmd` does not substitute CTest for the established ordered runner.

The canonical registry audit and final full run both passed. `test.cmd` has 69 physical CRLF lines, zero bare LF lines, and `.gitattributes` confirms `text eol=crlf`.

## H4 — Parity and incremental invalidation

In a complete ignored short-path copy of the repository, the following dependency perturbations were performed without changing original source contents or timestamps:

| Isolated change | Observed Ninja plan/result |
|---|---|
| No inputs changed | Localization preflight only; no compile/link actions |
| One harness `.cpp` timestamp | Only that target’s object and link |
| Shared `aspect_policy.hpp` timestamp | Only dependent objects/links |
| Included `runtime.cpp` timestamp | Exactly two adapter harness objects and links |
| Locale JSON, RC source, or font asset | Resource compile plus the three resource-consumer links |
| Resource-ID header | Resource rebuild and its actual transitive dependents |
| Missing generated INI header | Header regenerated; dependent outputs rebuilt; build exit 0 |
| Test registry timestamp | CMake reran configuration; no C++ compilation |
| Fixture-only per-target definition change | Ninja command changed and only that target’s translation units/link became dirty; registry was restored and CMake regenerated |

The first fixture copy was nested too deeply and hit Windows path-length/compiler temporary-file limits; the isolated copy at a short ignored path built successfully. No original source files were touched to manufacture an incremental result.

Four canonical unchanged warm runs all returned exit 0. Their console logs were byte-identical, SHA-256 `397A7E76AB51DA36B34A03898044AE948AD6EA06B7240EF1D4D72453C733F135`. The full CMake candidate build log SHA-256 is `9D4E29B83BB6B6066BCB721032B02EA4DE40C1BE872A9F5F3119322923675E27`; the seven-fixture exit-contract log SHA-256 is `BC3969F4EE107CE850B2976760C11D4D8D7CAA18572A2B721A34BC0F9F791FB3`.

Legacy full-run log SHA-256: `47361FAC550C05C5CD59137F9F38EABF97F31C34B2F74A814750A572747E12BA`.

## Files changed for H0–H4

- `CMakeLists.txt` — complete manifest-backed test target graph and dependencies.
- `test.cmd` — canonical CMake/Ninja build plus serial runner entry point.
- `tests/test_harnesses.json` — 60-target/63-invocation registry and support target declaration.
- `tests/runner/test_cmd_audit.ps1` — registry/source/resource/run parity audit for both pre-migration baseline and hybrid entry point.
- `tests/runner/test_harness_runner.ps1` — serial process runner with signed exit propagation and timeout behavior.
- `tests/runner/test_cmd_exit_code_contract.ps1` and `tests/runner/test_cmd_exit_status_probe.cpp` — isolated nonzero, crash, stale-output and timeout regression support.
- `research/evidence/native-runtime/N06/tools/n06_validate.ps1` — BuildProbe source identity reads the production registry instead of parsing `build.cmd`; evidence/status behavior remains unchanged.
- `research/reports/TEST_INFRASTRUCTURE_HYBRID_MIGRATION_PLAN.md` — H0–H4 completion and evidence links.
- This implementation report.

## Preservation and remaining limits

The `.git/index` SHA-256 is unchanged from the captured starting value: `A7C0C921F1C90E638719EB6ED3C11DE3AC0073B33B79621CB6D0CBBD5CA05380`. Existing staged changes were preserved; nothing was staged, unstaged, reset or cleaned by Git commands. Build and fixture outputs are in ignored `build-artifacts/`.

N03 remains `Partially mitigated`; N04–N06 remain open, and no live game test was performed. This migration establishes offline build/test parity and incremental dependency evidence; it does not replace native runtime validation or close those evidence gates. The only approved test-multiplicity change is the documented duplicate matrix removal from H0.
