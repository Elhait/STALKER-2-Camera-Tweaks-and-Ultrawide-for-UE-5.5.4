# Runtime decomposition — Batch 0 / V0 results

Date: 2026-10-08. Approved scope: baseline and actual-adapter validation only. **B1 is not authorized and has not started.**

## Disposition

- **Batch 0: complete.** [Baseline](../evidence/runtime-decomposition/BASELINE_2026-10-08.md) and [individual move ledger](../evidence/runtime-decomposition/MOVE_LEDGER_2026-10-08.md) record identities, source preservation, 172 function/template definitions, 159 RuntimeState members, alias/storage/guard dispositions and startup/callback/reset order.
- **V0-A: actual API and persistence failure paths expanded and passing.** The production callback bundle is published through the real `RuntimeSettingsApi`; Apply, readback, semantic readback, successful persistence, locked-target atomic replacement failure, missing-file failure, duplicate publication rejection and last-known-good preservation are exercised.
- **V0-B: expanded, still partial.** Actual Dialogue Exiting/Rearm/disable-in-flight, source-change cancellation, cinematic association conflict, and recovery-owner rejection of a foreign writer are exercised. This does not complete every mode, fallback, invalid input, or native source/RSI/RBX combination in the full plan matrix.
- **V0-C: read-only orchestration review complete; orchestration itself remains unexercised.** Existing production leaf boundaries and their current tests are classified below. No production seam was added. Actual Initialize/partial-install/reset failure interleavings remain runtime validation gates, so V0 is still not closed.
- **B1 gate: not satisfied.** Review the remaining validation strategy before approving extraction. No callback moved; no runtime owner, correction gate, calculation or shared-state contract changed.

## Test mechanism — what is real and what is controlled

`tests/plugin/runtime_native_adapter_harness.cpp` includes the unchanged production `src/plugin/runtime.cpp` exactly once. It does not link that file separately and does not include `dll_entry.cpp`, invoke DllMain/Initialize, load an ASI or attach game hooks. It calls the actual private writer, Cinematic aspect/ENTER/EXIT, Dialogue and Zoom adapters, actual setting handler/readers, actual publication and semantic readback. Writer also goes through its installed `SafeMidHookEntry` template. Real SafetyHookContext, real resident RuntimeState, correction gate, transactions and domain implementations are used.

The runner input script obtains 58 actual dependency/vendor translation units from the supported build source list, excluding runtime.cpp and dll_entry.cpp. There is no copied callback algorithm or alternate RuntimeState. Contexts contain controlled native pointers plus general-register/XMM-lane sentinels. Buffers use VirtualAlloc/VirtualProtect and actual Win32 memory validation. Fresh fixture processes avoid copying/resetting mutex-bearing resident state. Child processes have a 10-second timeout so gate regressions do not hang the suite; only the fixture-owned child can be terminated.

| Test-only leaf boundary | Mechanism | Evidence limit |
|---|---|---|
| Viewport | Preprocessor remaps the runtime's viewport leaf calls to fixed fixture input or NaN | Does not test real HWND/display discovery; actual viewport harness remains separate |
| Overlay wake delivery | Runtime wake symbol maps to a recorder inspecting publication/context/gate state at the call | Tests publication decision/order, not presenter delivery or GPU work |
| Memory read | Runtime leaf maps to a wrapper calling actual platform::win32::ReadMemory and recording result/order | One named case intentionally rejects only the aspect readback after a successful write |
| Aspect write | Runtime leaf maps to a wrapper calling actual gameplay::WriteAspectOnly | Real memory/protection/write results are used; no alternate RuntimeCombine decision/transform |
| Logging | Real production Log/LogImmediate/gate deferral; logger has a passive test sink | Tests emission after gate release, not log file formatting/failure modes |

Macro substitutions live only in the test TU, after real leaf declarations are included; definitions/calls of the runtime's aspect utility receive the same test-only spelling. They do not replace ProcessCameraWriter, lifecycle transactions, publication, setting handlers or callback bodies. No production file was instrumented. The production branches and ordering remain unchanged; test event recording adds only fixture observation. This is actual-adapter offline characterization, **not** live hook/trampoline or game behavior acceptance.

## Implemented cases

36 scenarios run with Diagnostic Mode OFF and ON, in both ordinary-production and supported-diagnostic compile profiles. Each case has independent failure assertions; NDEBUG does not disable them.

| Actual surface | Exercised cases / assertions | Coverage status |
|---|---|---|
| Gameplay writer | HorPlus ultrawide/native pass-through, RuntimeCombine mismatch/success, read-only refusal, invalid viewport, simulated post-write readback failure, AspectRecalculation, disabled correction, closed gate, stopping, unreadable source, NaN FOV | Covered named cases, not all source/mode combinations |
| Native context/memory | Complete context sentinels except explicitly allowed low XMM0/XMM1 lanes and aspect RIP adjustment; no RBX output-buffer write; no unexpected source writes outside aspect/flags; actual protection refusal | Covered tested buffers/branches; no real trampoline execution |
| RuntimeCombine ordering | Actual successful aspect write → native aspect readback → shared publication wake; wake observes publication before writer XMM0 update; readback failure preserves written memory but does not confirm/apply HorPlus; flags remain unchanged | Covered this ordering and failure, not every read/write/instruction ordering |
| Cinematic aspect | Configured forced aspect, module OFF native store, read-only and unreadable target; exact RIP +10 and unchanged remaining context | Covered named cases; Auto/fallback association matrix incomplete |
| ENTER/EXIT | Actual accepted and duplicate ENTER, module OFF, hook gate OFF, native EXIT operand preserved and actual owner/generation/readback | Covered named cases; fallback/invalid/foreign/rejected combinations incomplete |
| Rapid generations | Actual ENTER/EXIT chain with recovery observation enabled, associated writers for generations 1/2/4 and no post-writer in generation 3; independent Gameplay operand and new ENTER operand remain valid with matching generation | Covered fixture chain; not a replay of the game event timestamps |
| Owned Cinematic writer/recovery/handoff | Actual aspect association and writer; four live/retained FOV roles; native EXIT → interpolation sample → endpoint → RuntimeCombine Gameplay; shared baseline unchanged during recovery; Gameplay requalification and independent Evidence retention | Covered one source-owned RuntimeCombine path; HorPlus/foreign/owner-retirement matrix incomplete |
| Dialogue | Actual native XMM6 input, target read, descending Candidate→Active, ascending Exiting, RearmPending, converged completion, foreign-source cancellation, disabled-in-flight pass-through; XMM1 and wake ordering | Covered named cases in both diagnostic modes/profiles; other source replacement/invalid-target combinations remain |
| Zoom | Actual IN/OUT observations, validity with readable/unreadable native fields, hold/hit counters, no register mutation | Covered named cases; does not establish thread-affinity/concurrency safety |
| Publication/metadata/wake | Actual writer and semantic reader, qualified Gameplay baseline, unchanged writer does not wake, metadata-only freshness mutation wakes without changing FOV, same-source Gameplay pass-through makes prior target Historical | Covered named cases; not every visible metadata mutation |
| Settings / published API | Actual one-time production callback-bundle publication, Apply, SettingsSnapshot, semantic snapshot, persistence; duplicate publish rejected; valid INI updated; locked-target replacement and missing-file failures return false; last-known-good bytes unchanged | V0-A named API/file paths covered; no exhaustive persistence OS-failure matrix |
| Cinematic association / recovery | Actual conflicting aspect-store targets invalidate generation-scoped association; foreign writer cannot revive it. A writer RSI change during `CinematicExiting` triggers production `RetireRecoveryForWriterRsiChange`: recovery ownership is retired and coordinator returns to Gameplay | V0-B cases added; this characterizes code policy, not live game RSI/RBX identity distributions |
| Shared gate | Writer/Dialogue/ENTER/EXIT/aspect/settings/semantic reader wait while the same resident gate is held, then complete after release; actual logging helper defers until release | Covered bounded threaded fixtures + static owner/alias evidence; scheduling-dependent waiting is not exhaustive concurrency proof |

## Validation results

| Check | Result / evidence |
|---|---|
| Baseline full test.cmd | PASS, exit 0; baseline/test.log |
| Baseline ordinary production build.cmd | PASS, separate identified artifact; see baseline report |
| Baseline supported diagnostic build.cmd | PASS, separate identified artifact; see baseline report |
| Final full test.cmd including expanded V0 | PASS; all registered harnesses and both actual-adapter compile profiles pass (42 scenarios × Diagnostic Mode OFF/ON × 2 profiles = 168 fixture processes); `build-artifacts/runtime-decomposition/final/test-final.log` |
| Production build.cmd | PASS; separate artifact `build-artifacts/runtime-decomposition/final/production.asi`, 3,454,464 bytes, SHA-256 `B5847C31B09A862BAD369B1786DEF714E0D51A30B4B7A98ACD24C6427C24A212` |
| Supported diagnostic build.cmd | PASS; separate artifact `build-artifacts/runtime-decomposition/final/diagnostic.asi`, 3,526,144 bytes, SHA-256 `046EC65E80EE6DA28EFA2C03B1D59291A89F0452F7B1EFD0294A803A62977D22` |
| Runner registry audit | PASS: 59 unique source/compile/run registrations, immediate failure checks, repository inventory matches |
| Production preservation | No unstaged changes to `src/`; `runtime.cpp` not edited in this batch; camera semantics, owner/gate and production initialization untouched; root ASI not installed or overwritten |

The initial expanded test attempt caught a harness dispatch mistake: the new Dialogue lifecycle case was routed through a legacy refusal assertion. That was corrected in the test harness only. A later source-change recovery fixture incorrectly expected owner preservation; it failed, and source review established the actual policy is intentional retirement on writer-RSI change. The assertion was corrected to that current contract. Final `test.cmd` passes 42 scenarios (168 fresh child processes across profiles/modes); these are offline characterization, not ASI runtime acceptance.

Final comparison to the preserved source manifest finds only the intended `test.cmd` change among previously captured files. The 126 vendor source files have no hash mismatches. Staged binary patch before/after SHA-256 remains `4CF68405FE13D5CFA86649CC6C17DCCB655D32403E59728D495CD4EE0AEC7F0D`. With the ordinary Git worktree diff unavailable, complete relevant review used direct baseline-file comparison for test.cmd and inspection of new test/tools/records; no existing source edit was hidden by the Git limitation.

Raw outputs are local ignored evidence under `build-artifacts/runtime-decomposition/`. Intermediate fixture construction failures are retained in logs but are **not baseline defects**: initial enum/member/API spellings were corrected; Gameplay baseline freshness is Retained, while a prior HorPlus Target on full Gameplay pass-through is Historical; production logging need not emit on an otherwise quiet callback, so deferral is checked using the real Log helper explicitly. The strengthened rapid-chain fixture initially supplied old Gameplay output (~107 degrees) as a new authored ENTER operand despite supplying 90 degrees to the preceding native EXIT; IsCinematicOwnershipContinuation correctly refused that different intent. The corrected fixture supplies the same 90-degree native intent and asserts validated continuation exists before each reentry; it does not relax operand validity assertions. Focused supported-diagnostic lifecycle-chain and recovery then passed. Expectations were checked against current source contracts, not automatically regenerated goldens. No production behavior was repaired to make tests green.

## Remaining V0 gates and strategy review

| Gate / missing evidence | Why existing PASS is insufficient | Bounded next direction |
|---|---|---|
| V0-A exhaustive API/persistence matrix | Actual production API and representative success/failure paths now pass, but each setting kind, multi-key write partial failure and every filesystem error are not exercised | Keep remaining paths bounded to the settings component if that component is later moved; no broad failure-injection seam justified |
| V0-B complete native source/association/recovery matrix | Foreign recovery RSI retires the old handoff as specified and conflicting aspect targets invalidate association; not all RSI/RBX pairs, modes/endpoints, fallback and rejected EXIT permutations are tested | Add only callback-specific fixture cases when an identified batch needs them; do not claim the full matrix based on domain helpers |
| V0-B full wake/order matrix | RuntimeCombine native write→readback→publication wake and Dialogue post-XMM1 publication wake are asserted; not every callback side effect or every metadata mutation is instrumented | Treat assertions as named ordering evidence; preserve other source order through move ledger and compare per selected batch |
| V0-C actual Initialize/install/reset orchestration | No actual Initialize, installer commit/rollback, worker-start failure inside Initialize, or partial-initialization reset was run | No validated whole-flow seam. Keep as pre-B7 runtime validation gates; no production seam/framework added |

### V0-C read-only startup / install / reset review

| Orchestration path | Existing production leaf evidence | What remains unestablished |
|---|---|---|
| Overlay enabled/disabled/throw | `TryStartConfiguredOverlay` is the exact leaf called from Initialize; `optional_overlay_startup_harness` covers disabled skips invocation, enabled invocation and exception isolation | Initialize's actual config load→Overlay call ordering and runtime-owned discovery/worker effects are not exercised |
| Cinematic install transaction | `InitializeTransactional` is the actual production transaction helper; cinematic-initialization harness covers native, success, aspect/FOV and commit failures | Native SafetyHook creation/enable/disable and Rollback* callbacks wired by Initialize are not run against an executable image |
| Gameplay writer install | signature/instruction validators, SafeMidHookEntry actual callback, and worker/feature helpers have offline coverage | VerifyExecutableAndInstruction→MidHook::create(StartDisabled)→enable→gate/capability commit and each native failure point are not driven through Initialize |
| Dialogue dependency/install | actual Dialogue callback and `DialogueLifecycleCapabilityAvailable`/leaf failure paths are covered | Cinematic/Gameplay capability combinations through Initialize and `InstallDialogueBoundary` native patch failures remain unexercised |
| Worker startup / stop / join | WorkerLifecycle actual class has capacity, start/stop observation and failure tests | Initialize's actual sequence of hotkey/diagnostic worker starts and reset-on-each-partial-failure is not exercised |
| Reset / shutdown / detach | source confirms ResetAllRuntimeResources closes readiness/gates before worker stop; failed join returns before hook reset; process-terminating detach only signals/returns | no synthetic partially initialized production RuntimeState with real active hooks/workers; controlled Shutdown and failure interleavings need named runtime or future safe leaf evidence |

This is a source-backed classification, not a PASS for production orchestration. Existing leaf tests can characterize pure transactions and helpers but do not prove their wiring or the partial-initialization order. Since the user explicitly prohibited new production seams absent approval, V0-C retains those cases as runtime gates before any installer/startup/reset movement. A runtime gate must name the supported game image, artifact/profile, forced failure point and expected surviving capabilities; no game was launched here.

`Initialize` hard-codes module/config/log setup, current image resolution, native SafetyHook creation/activation and optional Overlay worker startup. Calling it against a test executable cannot prove successful installation/partial commit paths; starting optional presentation is outside this fixture's intended isolation. There is no already validated leaf-injection surface for the complete orchestration. No broad production seam or fake installer/framework was added just to obtain green tests.

The current approval explicitly prefers reviewing validation strategy over substantial production intrusion. Therefore extraction remains stopped at B0/V0. Review the listed fixture extensions and especially exact V0-C leaves/files before completing V0. Ordinary and diagnostic builds prove compilation/linkage only; neither establishes game lifecycle, GPU/Overlay behavior, dynamic unload or crash freedom. Session-boundary invalidation remains OPEN / authoritative event unknown.

## Changed files and scope review

- Existing file: `test.cmd` — builds/runs actual adapter harness in ordinary and supported-diagnostic profiles, with failure checks.
- New test files: `tests/plugin/runtime_native_adapter_harness.cpp`, `tests/runner/runtime_native_adapter_inputs.ps1`.
- Records/tools: implementation plan status; this report; Batch 0 baseline and generated move ledger; `research/tools/capture_runtime_decomposition_baseline.ps1`, `research/tools/runtime_decomposition_move_ledger.ps1`.
- No `src/` edits, build.cmd changes, new production owner/gate, callback moves, camera calculation changes, INI/localization/UI cleanup, game launch or ASI installation.

Verified task log is not written: full V0 acceptance and user-confirmed closure remain outstanding.
