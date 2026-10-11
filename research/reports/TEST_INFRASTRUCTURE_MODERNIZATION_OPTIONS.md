# Test infrastructure modernization options

Date: 2026-10-11. Decision document only; no migration or optimization implemented.
Evidence: [architecture audit](TEST_INFRASTRUCTURE_ARCHITECTURE_AUDIT.md), [performance baseline](TEST_EXECUTION_PERFORMANCE_BASELINE.md).

## Recommendation

**Use B (Hybrid) as the first, bounded modernization step; consider C only after full compilation/execution parity is demonstrated.** Ninja's measured warm reuse is useful, and most canonical time is compilation rather than test assertions. There is no reason to replace the complete runner with the existing five-entry CTest subset.

Keep `test.cmd` as the user entry point. First independently approve fixing nonzero exit detection. Then, if migration is approved, introduce an explicit all-harness build registry and dependency-tracked compilation while preserving current runner ordering, arguments, process isolation, assertions and scenario multiplicity. A later CTest orchestration switch is optional and should earn its place through selection, failure isolation and parallel-run validation.

This recommendation is not approval to implement B or C.

## Alternatives

| Dimension | A — Keep CMD | B — Hybrid | C — Full CMake/CTest |
| --- | --- | --- | --- |
| Compilation | Existing serial, unconditional cl; narrow dedup/flag grouping possible | Ninja targets build every harness; current execution semantics retained | Same Ninja targets as B |
| Warm no-source-change benefit | Limited unless implementing a new dependency/cache mechanism inside CMD | Eliminates unchanged compiler/link commands; measured compile budget is the main removable cost | Same compile benefit as B; CTest does not make assertions faster |
| Cold-build benefit | Approved duplicate removal can save measured duplicate work | Shared identical-contract objects, bounded parallel compilation; exact full cold speedup unmeasured | Same as B |
| Runner behavior | Current positive fail-fast guards need nonzero correction | Existing execution layer still needs nonzero correction and a controlled build/run boundary | CTest detects crashes/timeouts, records test results; exact equivalent ordering/isolation must be specified |
| 60/60 inventory | Current audit already passes | New registry joins build inputs to canonical runner; require executable/scenario parity | Require all 60 exes and all intended command/argument entries, not today's 2 exes/5 entries |
| Selective runs | Manual executable invocation; no safe selective rebuild | Target-selective build + retained runner selection if separately approved | Target-selective build and named/label-filtered CTest |
| Parallelism | Unsafe shared /Fo compilation; execution presently sequential | Safe target-local compilation; serial execution initially | Bounded parallel execution only after temp/resource/timeout audit; retain native child isolation |
| Adding tests | Handwritten sources/compile line/run line, guards and audit | Explicit registry plus runner entry; less source-list duplication | Explicit registry generates build and test registration; best long-term consistency if implemented carefully |
| Complexity / regression risk | Low for guard fixes; grows quickly for handwritten incremental dependency handling | Medium; does not rewrite execution while changing build ownership | Higher; compile and runner contracts change, assertions/resources/process multiplicity can be lost |
| Maintenance | Repeated lists and bespoke parser conventions remain | Transitional two-layer system, but shared registry limits divergence | Lowest potential duplication; more complete CMake/CTest policy needed up front |

## What timing evidence permits

The performance report separates true unchanged canonical totals from instrumented command timing. Warm CMake measurements run **only the two existing native executables**, so their total cannot be presented as a measured full-suite speedup.

A full warm B/C suite retaining all current executions can approximately remove the measured canonical compile/link budget while retaining environment, audits, resource checks, every executable run and orchestration. Add future configure/build overhead; do not advertise the subtraction model as an observed full migration result.

Measured full canonical baseline: **355.019 / 427.428 s**, median **391.224 s**. Instrumented compile/link budget: **296.647 / 303.243 s**; execution: **82.618 / 150.824 s**. The same-replay warm subtraction model leaves **98.415 / 178.333 s**, plus future build/configure overhead. Thus B and C have a substantial compilation opportunity, but no measured difference in full-suite assertion speed. Current five-entry CMake helper: **65.660 / 65.279 s**, with no C/C++ recompilation; this subset is not full-suite timing parity.

For A, independently approved duplicate removal would save **89.427 / 98.375 s** in those replays, while unchanged remaining harnesses would still recompile. Implementing safe warm reuse in CMD would require a real header/flag/provenance cache, not simply an existence check; that largely recreates the build graph already supplied by Ninja.

Removing the duplicate native compilation and second matrix execution is a different optimization with its own approval: it changes command/process multiplicity, not just dependency scheduling. Its time saving is separately tabulated in the performance report and must not be counted twice when modeling Ninja's removal of all warm compilation.

Test execution itself may improve with approved bounded parallel scheduling, but this audit did not benchmark a parallel full suite. Native matrix child isolation, failure injection, serialization waits and output evidence must remain intact. No numerical test-execution speedup is claimed.

## Minimal proposed approval stages

1. **Correctness repair, independent of build choice:** signed/nonzero exit checks, propagation through fail labels, crash/timeout runner regression and revised audit contract. No runtime production edits.
2. **Explicit registry design:** 60 executable inputs, compiler language/options/defines/includes, libraries/resources, generated inputs, executable working directories, argument tuples, timeouts and any serialization requirements. Derive native dependencies from the current production registry, excluding runtime.cpp and dll_entry.cpp.
3. **Parallel B build candidate:** all 60 targets, isolated objects/flags, runtime owner included once per harness, ordinary assertions active. Share only compilation-equivalent object groups.
4. **Equivalence gate before canonical switch:** 60/60 inventory; retained 64 direct commands / 408 native child runs and four Zoom tuples unless dedup separately approved; comparable logs and all nonzero statuses. Compare resource presence as well as successful link.
5. **Incremental validation:** no-op; one harness cpp; shared hpp; included runtime.cpp; locale JSON; font/resource ID; source registry JSON; missing generated header/res; changed compiler flags/toolchain; localized showIncludes; separate profiles. These are proposed future checks, not audit work already performed.
6. **Optional C orchestration:** after B parity, register every intended test command with stable names/labels, crash/timeout behavior, fixture temp isolation and bounded concurrency. Preserve native subprocess reset/lifetime semantics; do not merge cases into one process.

## Mandatory safety details

- The shared /Fo CMD directory is not an object cache. Existence-only `-ReuseObjects` / `-ReuseDependencies` modes cannot establish fresh source/header/flag provenance.
- Do not inherit NDEBUG onto language transition or localization assertions. Current ordinary CMD contracts are the baseline.
- Current CMake native targets build, but do not link, localization_resources.res. Correct link/resource parity is a migration condition; today's CTest PASS is not embedded-resource equivalence.
- Explicit build-order dependencies are not substitutes for resource link inputs or generated-header dependencies.
- Current helpers always configure. Skipping configure requires tracking external source-registry JSON and toolchain/cache changes, not only CMakeLists.txt.
- Retain locale/resource/glyph gates. An always-run source-policy audit may be inexpensive relative to compilation; cache it only with a complete input/key contract or keep it in the execution preflight.
- Validate the UTF-8 localized MSVC include prefix and actual populated Ninja deps, including restored/migrated caches. Do not infer header tracking solely from /showIncludes being present.
- Serial child matrix fixtures and native fault injection remain process-scoped. Do not trade fresh-process isolation for a smaller scenario count.
- CTest raw logs/results and build-input manifest must remain sufficient to distinguish compile failure, assertion failure, crash, timeout and absent/stale executable.

## Open questions

- Is the duplicate matrix invocation intentional repetition? It is identical today; removing it needs explicit approval.
- Desired cold-build job limit and RAM budget are not yet measured; this audit did not benchmark an all-60 Ninja graph.
- Full-suite CTest label/resource/timeouts and future cross-suite concurrent temp policies are not currently declared.
- Cache regeneration policy when the MSVC language/codepage/toolchain or source registry changes requires design.
- Existing native tests do not prove full in-game engine-consumer/lifetime behavior. This audit changes none of N03–N06 evidence status.

Stop here: approve a specific implementation scope separately.

