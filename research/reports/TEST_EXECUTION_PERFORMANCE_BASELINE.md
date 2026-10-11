# Test execution performance baseline

Date: 2026-10-11. Research only. Sources, canonical runner, assertions and CMake files unchanged.

## Real canonical full-suite time

| Run | Unmodified test.cmd wall time, s | Exit | Inventory | Native children | Zoom tuples |
| --- | ---: | ---: | --- | ---: | --- |
| 1 | 355.019 | 0 | 60/60 PASS | 408 | 4/4 PASS |
| 2 | 427.428 | 0 | 60/60 PASS | 408 | 4/4 PASS |
| Median of two | 391.224 | — | — | — | — |

Approximately **6 min 31 s**, range **5 min 55 s–7 min 7 s**. These are true canonical totals, not timing-copy totals or the CMake subset.

Both canonical raw logs are byte-identical: 18 locales / 212 keys PASS, no missing_text_glyph records, two 102-case OFF/ON matrix completions and all four Zoom runs. Each contains 18 existing C4201 warnings from the SafetyHook/Zydis dependency. PASS does not mean warning-free.

## Environment and measurement method

- Dirty HEAD `db67e4dfc56cfe06394c38413f5bb460f797d210`; existing source changes retained.
- Windows NT 10.0.26200.0, x64, 16 logical processors; CPU environment AMD64 Family 25 Model 97 Stepping 2. CIM hardware query unavailable: no unsupported RAM/product-name claim.
- VSDevCmd 17.14.28; cl.exe file version 19.44.35224.0, toolset directory 14.44.35207; bundled CMake 3.31.6-msvc6/Ninja; RC SDK 10.0.26100.0.
- Actual language probe with /std:c++latest: _MSC_VER=1944, _MSVC_LANG=202400, __cplusplus=199711. The last value is legacy reporting without /Zc:__cplusplus, not old language mode.
- Two sequential unchanged canonical runs, then two sequential timing replays, then two warm CMake subset runs, then three repeats of each script micro measurement. No competing full suites; no source edits/timestamp touches/cache deletion.
- An ignored copy preserves original commands/flags/scenarios, adds QPC markers around **131 commands**, plus timing-only cl /Bt+ and link /time. End markers preserve command exit status.
- Linker **process wall time** comes from /Bt+. Nested /time phase subtotals are not added again. Compiler/driver = cl-command wall minus linker-process wall; includes compiler setup.
- Executable timings include launch/child orchestration, not just assertions. PowerShell timings include shell startup; RC timings include process startup.
- Residual includes marker/CMD/setup overhead. Timed stages also include end-marker launch latency; no guessed correction factor is subtracted.
- A small compiler/exit-code probe and read-only inventory work overlapped parts of the early canonical baseline. Instrumented suites were sequenced, but host load, antivirus/filesystem state and clocks were not controlled. Variance cannot be assigned solely to instrumentation.
- Two samples are a baseline, not a confidence interval. No all-60 cold Ninja or full parallel execution benchmark was performed.

## Instrumented full-suite breakdown

Keep these replay totals separate from true canonical totals above.

| Component | Replay 1, s | Replay 2, s | Median, s |
| --- | ---: | ---: | ---: |
| MSVC environment | 3.004 | 7.259 | 5.131 |
| Compilation + cl driver, excluding link | 289.101 | 292.849 | 290.975 |
| Linker process | 7.546 | 10.394 | 8.970 |
| All 64 test commands, including native children | 82.618 | 150.824 | 116.721 |
| Four PowerShell preflight/generation processes | 6.662 | 7.815 | 7.239 |
| Resource compilation | 0.113 | 0.166 | 0.140 |
| Residual / markers / CMD orchestration | 6.018 | 12.269 | 9.144 |
| Instrumented full suite | 395.061 | 481.576 | 438.319 |

Both replays and all **262 measured command instances** returned **0**. Every original executable/assertion/argument remains present.

Frontend sum: 238.413 / 239.985 s; backend sum: 39.317 / 41.236 s. These are subdivisions, not extra wall-time categories. Frontend/header/template work dominates compilation.

### Critical path

Serial environment → audits/generation/RC → 60 initial executable compilations → ordinary tests → native matrix → four Zoom runs → duplicate native compilation → duplicate matrix. Without a parallel graph the critical path is essentially their sum.

| Largest commands | Replay 1, s | Replay 2, s |
| --- | ---: | ---: |
| C1 — tests/plugin/runtime_native_adapter_harness.cpp (compile+link) | 53.314 | 50.502 |
| C2 — tests/plugin/n06_m01_zoom_game_derived_harness.cpp (compile+link) | 50.115 | 51.255 |
| C61 — tests/plugin/runtime_native_adapter_harness.cpp (compile+link) | 52.591 | 51.789 |
| First 204-child matrix | 33.151 | 41.500 |
| Repeated 204-child matrix | 36.836 | 46.586 |

Cl/link total **296.647–303.243 s**. Link is only 7.5–10.4 s: reducing linker time is not the main opportunity. Three native compilations recompile 183 source occurrences.

## PowerShell / audits: three standalone repeats

| Operation | Repeat 1, s | Repeat 2, s | Repeat 3, s | Median, s | Exit |
| --- | ---: | ---: | ---: | ---: | --- |
| powershell-startup | 1.107 | 0.686 | 0.682 | 0.686 | 0/0/0 |
| inventory-audit | 1.644 | 1.543 | 1.586 | 1.586 | 0/0/0 |
| localization-audit | 5.834 | 5.871 | 5.980 | 5.871 | 0/0/0 |
| ini-generation | 1.603 | 1.643 | 1.698 | 1.643 | 0/0/0 |
| adapter-inputs | 1.600 | 1.618 | 1.607 | 1.607 | 0/0/0 |

Empty-shell command: powershell.exe -NoProfile -Command "exit 0". Subtracting its median is only an estimate, not a controlled script-CPU measurement. Localization scans source-policy/resource contracts as well as JSON. These separate micro samples and canonical stage totals vary with host conditions; do not combine them as one run.

## Existing warm CMake subset

| Stage | Run 1, s | Run 2, s | Median, s |
| --- | ---: | ---: | ---: |
| MSVC environment | 5.111 | 5.321 | 5.216 |
| Configure/generate | 0.906 | 0.912 | 0.909 |
| Ninja build: locale preflight only | 6.159 | 6.318 | 6.239 |
| CTest | 49.426 | 49.521 | 49.474 |
| Full helper including orchestration | 65.660 | 65.279 | 65.469 |

Both helper exits **0**, **5/5 entries PASS**. CTest reported 49.13 / 49.21 s. Ninja printed only `[1/1] Validating localization catalogs and embedded resources`: **no C/C++ compilation or linking**.

This is positive incremental reuse evidence for **two existing executables**, not 60. The subset has one 204-child matrix and four Zoom cases; canonical has two matrices plus 58 ordinary executables. Native resource linking also differs (TIA-06). **65 s versus 391 s is not a measured full-suite speedup.**

## Quantified opportunities / limits

- Warm B/C model: subtracting only cl/link from the corresponding replay, while retaining all current runs, leaves **98.415 / 178.333 s** — roughly **98–178 s**, before future configure/build overhead. Compile/link is **75.1% / 63.0%** of replay totals. This is a subtraction model, not an implemented full-suite result.
- Separately removing duplicate compile + second matrix saves **89.427 / 98.375 s** in these replays. Requires approval of multiplicity; do not double-count that compile saving after removing all warm compilation.
- Existing object grouping needs 59 shared dependencies + two harness + two binding-probe compilations, versus three 61-input CMD commands: 63 vs 183 source occurrences, saving 120. Full-suite cold parallel speedup is **unmeasured**, not inferred from counts.
- Parallel assertion execution is **unmeasured** and independent of compilation. Preserve fresh native subprocesses and timeout/serialization semantics.
- Script startup/audits are smaller than the roughly 300 s compile/link budget. Do not eliminate localization/glyph/resource correctness gates to chase a few seconds.

## Appendix A — each harness compilation / link

C IDs map to exact commands and sources in the architecture audit. Includes the duplicate C61. Compiler/driver excludes link; median combines both. All codes 0/0.

| Compiler | Harness | Compile run 1, s | Link run 1, s | Compile run 2, s | Link run 2, s | Median combined, s |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| C01 | `tests/plugin/runtime_native_adapter_harness.cpp` | 53.032 | 0.282 | 50.132 | 0.370 | 51.908 |
| C02 | `tests/plugin/n06_m01_zoom_game_derived_harness.cpp` | 49.843 | 0.272 | 50.921 | 0.334 | 50.685 |
| C03 | `tests/lifecycle/worker_lifecycle_harness.cpp` | 0.658 | 0.103 | 0.761 | 0.143 | 0.832 |
| C04 | `tests/feature_status/feature_status_harness.cpp` | 0.315 | 0.119 | 0.404 | 0.145 | 0.492 |
| C05 | `tests/plugin/optional_overlay_startup_harness.cpp` | 0.682 | 0.103 | 0.782 | 0.160 | 0.864 |
| C06 | `tests/plugin/camera_correction_gate_harness.cpp` | 1.507 | 0.117 | 1.675 | 0.149 | 1.724 |
| C07 | `tests/plugin/camera_setting_transaction_harness.cpp` | 1.660 | 0.106 | 1.762 | 0.153 | 1.841 |
| C08 | `tests/plugin/dialogue_boundary_transaction_harness.cpp` | 2.055 | 0.113 | 2.178 | 0.157 | 2.252 |
| C09 | `tests/plugin/camera_writer_transaction_harness.cpp` | 9.298 | 0.124 | 9.464 | 0.166 | 9.526 |
| C10 | `tests/plugin/cinematic_lifecycle_transaction_harness.cpp` | 10.604 | 0.124 | 11.003 | 0.175 | 10.953 |
| C11 | `tests/overlay/startup_timeline_harness.cpp` | 1.022 | 0.108 | 1.148 | 0.150 | 1.214 |
| C12 | `tests/diagnostics/startup_journal_harness.cpp` | 0.448 | 0.121 | 0.551 | 0.141 | 0.630 |
| C13 | `tests/config/config_persistence_harness.cpp` | 5.533 | 0.115 | 6.133 | 0.171 | 5.976 |
| C14 | `tests/config/gameplay_mode_harness.cpp` | 1.569 | 0.108 | 1.671 | 0.167 | 1.758 |
| C15 | `tests/config/runtime_settings_api_harness.cpp` | 1.910 | 0.115 | 2.030 | 0.165 | 2.110 |
| C16 | `tests/gameplay/aspect_policy_harness.cpp` | 0.768 | 0.109 | 0.863 | 0.161 | 0.950 |
| C17 | `tests/gameplay/horplus_gameplay_harness.cpp` | 3.587 | 0.110 | 3.688 | 0.157 | 3.771 |
| C18 | `tests/gameplay/zoom_transition_harness.cpp` | 3.382 | 0.114 | 3.433 | 0.160 | 3.544 |
| C19 | `tests/gameplay/gameplay_mode_transition_harness.cpp` | 3.934 | 0.117 | 4.057 | 0.152 | 4.130 |
| C20 | `tests/regression/module_ownership_harness.cpp` | 4.526 | 0.111 | 5.061 | 0.154 | 4.926 |
| C21 | `tests/gameplay/gameplay_camera_safety_harness.cpp` | 1.774 | 0.101 | 1.763 | 0.147 | 1.893 |
| C22 | `tests/platform/memory_harness.cpp` | 0.632 | 0.097 | 0.707 | 0.138 | 0.787 |
| C23 | `tests/platform/viewport_harness.cpp` | 1.245 | 0.110 | 1.371 | 0.158 | 1.442 |
| C24 | `tests/platform/signature_scanner_harness.cpp` | 1.408 | 0.116 | 1.491 | 0.150 | 1.582 |
| C25 | `tests/platform/instruction_validator_harness.cpp` | 1.667 | 0.119 | 1.686 | 0.165 | 1.818 |
| C26 | `tests/cinematics/cinematic_initialization_harness.cpp` | 0.953 | 0.112 | 1.016 | 0.160 | 1.120 |
| C27 | `tests/cinematics/coordinator_recovery_harness.cpp` | 1.370 | 0.111 | 1.484 | 0.145 | 1.555 |
| C28 | `tests/cinematics/cinematic_exit_authority_harness.cpp` | 3.268 | 0.129 | 3.303 | 0.167 | 3.433 |
| C29 | `tests/cinematics/horplus_post_exit_recovery_harness.cpp` | 5.041 | 0.118 | 5.044 | 0.175 | 5.189 |
| C30 | `tests/cinematics/cinematic_selection_harness.cpp` | 1.418 | 0.109 | 1.402 | 0.161 | 1.544 |
| C31 | `tests/dialogue/post_cinematic_exclusion_harness.cpp` | 1.451 | 0.112 | 1.527 | 0.151 | 1.621 |
| C32 | `tests/dialogue/recovery_rearm_harness.cpp` | 1.397 | 0.117 | 1.527 | 0.162 | 1.602 |
| C33 | `tests/dialogue/candidate_hardening_harness.cpp` | 1.463 | 0.111 | 1.536 | 0.161 | 1.636 |
| C34 | `tests/dialogue/policy_snapshot_harness.cpp` | 1.455 | 0.114 | 1.459 | 0.155 | 1.592 |
| C35 | `tests/camera/camera_state_snapshot_harness.cpp` | 2.344 | 0.116 | 2.352 | 0.161 | 2.486 |
| C36 | `tests/camera/fov_observation_harness.cpp` | 2.016 | 0.112 | 2.130 | 0.152 | 2.205 |
| C37 | `tests/camera/gameplay_baseline_harness.cpp` | 2.360 | 0.112 | 2.473 | 0.161 | 2.553 |
| C38 | `tests/camera/gameplay_aspect_restoration_harness.cpp` | 2.468 | 0.114 | 2.563 | 0.162 | 2.653 |
| C39 | `tests/cinematics/cinematic_fov_harness.cpp` | 3.972 | 0.116 | 4.045 | 0.166 | 4.149 |
| C40 | `tests/cinematics/cinematic_aspect_harness.cpp` | 2.405 | 0.101 | 2.502 | 0.153 | 2.581 |
| C41 | `tests/dialogue/dialogue_fov_harness.cpp` | 0.520 | 0.100 | 0.603 | 0.150 | 0.687 |
| C42 | `tests/diagnostics/diagnostics_gate_harness.cpp` | 0.798 | 0.124 | 0.875 | 0.158 | 0.978 |
| C43 | `tests/diagnostics/dialogue_source_provenance_harness.cpp` | 1.071 | 0.111 | 1.185 | 0.149 | 1.258 |
| C44 | `tests/diagnostics/camera_writer_identity_harness.cpp` | 0.960 | 0.121 | 1.078 | 0.162 | 1.160 |
| C45 | `tests/diagnostics/cinematic_lifecycle_context_harness.cpp` | 0.692 | 0.112 | 0.768 | 0.160 | 0.866 |
| C46 | `tests/diagnostics/performance_telemetry_harness.cpp` | 1.669 | 0.122 | 1.755 | 0.163 | 1.855 |
| C47 | `tests/diagnostics/language_state_transition_harness.cpp` | 0.171 | 0.098 | 0.243 | 0.139 | 0.326 |
| C48 | `tests/diagnostics/matchgameplay_prediction_harness.cpp` | 0.720 | 0.124 | 0.754 | 0.161 | 0.879 |
| C49 | `tests/regression/runtime_evidence_replay_harness.cpp` | 5.117 | 0.115 | 5.326 | 0.167 | 5.362 |
| C50 | `tests/gameplay/gameplay_camera_resolver_harness.cpp` | 1.721 | 0.102 | 1.816 | 0.153 | 1.895 |
| C51 | `tests/overlay/composition_presenter_state_harness.cpp` | 2.581 | 0.123 | 2.804 | 0.160 | 2.834 |
| C52 | `tests/overlay/activation_trace_harness.cpp` | 0.755 | 0.100 | 0.853 | 0.150 | 0.929 |
| C53 | `tests/overlay/overlay_layout_metrics_harness.cpp` | 0.968 | 0.114 | 1.004 | 0.157 | 1.122 |
| C54 | `tests/overlay/selector_tooltip_state_harness.cpp` | 0.689 | 0.113 | 0.767 | 0.156 | 0.862 |
| C55 | `tests/overlay/input_state_harness.cpp` | 2.496 | 0.112 | 2.477 | 0.167 | 2.626 |
| C56 | `tests/overlay/camera_integration_harness.cpp` | 1.909 | 0.115 | 2.063 | 0.157 | 2.122 |
| C57 | `tests/overlay/placement_config_harness.cpp` | 6.434 | 0.122 | 6.741 | 0.169 | 6.733 |
| C58 | `tests/overlay/feature_presentation_harness.cpp` | 2.504 | 0.116 | 2.491 | 0.157 | 2.634 |
| C59 | `tests/overlay/semantic_snapshot_harness.cpp` | 1.132 | 0.120 | 1.192 | 0.147 | 1.296 |
| C60 | `tests/overlay/localization_harness.cpp` | 7.502 | 0.202 | 7.625 | 0.273 | 7.801 |
| C61 | `tests/plugin/runtime_native_adapter_harness.cpp` | 52.252 | 0.340 | 51.332 | 0.456 | 52.190 |

## Appendix B — each direct executable invocation

Matrix-parent wall includes its 204 children; their time is not added twice. All codes 0/0. These are 64 runner commands, not assertion counts.

| Run command | test.cmd line | Executable / arguments | Replay 1, s | Replay 2, s | Median, s |
| --- | ---: | --- | ---: | ---: | ---: |
| R01 | 161 | `worker_lifecycle_harness` | 0.105 | 0.153 | 0.129 |
| R02 | 163 | `feature_status_harness` | 0.090 | 1.211 | 0.650 |
| R03 | 165 | `optional_overlay_startup_harness` | 0.098 | 0.137 | 0.118 |
| R04 | 167 | `camera_correction_gate_harness` | 0.098 | 0.173 | 0.136 |
| R05 | 169 | `camera_setting_transaction_harness` | 0.092 | 1.143 | 0.617 |
| R06 | 171 | `camera_writer_transaction_harness` | 0.097 | 1.182 | 0.639 |
| R07 | 173 | `cinematic_lifecycle_transaction_harness` | 0.095 | 1.213 | 0.654 |
| R08 | 175 | `startup_timeline_harness` | 0.133 | 0.188 | 0.161 |
| R09 | 177 | `startup_journal_harness` | 0.099 | 0.142 | 0.121 |
| R10 | 179 | `config_persistence_harness` | 0.790 | 1.171 | 0.981 |
| R11 | 181 | `gameplay_mode_harness` | 0.100 | 0.145 | 0.123 |
| R12 | 183 | `runtime_settings_api_harness` | 0.093 | 1.159 | 0.626 |
| R13 | 185 | `dialogue_boundary_transaction_harness` | 0.120 | 0.172 | 0.146 |
| R14 | 187 | `aspect_policy_harness` | 0.163 | 0.141 | 0.152 |
| R15 | 189 | `horplus_gameplay_harness` | 0.094 | 1.141 | 0.617 |
| R16 | 191 | `zoom_transition_harness` | 0.096 | 1.141 | 0.618 |
| R17 | 193 | `gameplay_mode_transition_harness` | 0.100 | 1.175 | 0.638 |
| R18 | 195 | `module_ownership_harness` | 0.101 | 1.151 | 0.626 |
| R19 | 197 | `gameplay_camera_safety_harness` | 0.094 | 1.170 | 0.632 |
| R20 | 199 | `memory_harness` | 0.099 | 1.160 | 0.629 |
| R21 | 201 | `viewport_harness` | 0.099 | 1.153 | 0.626 |
| R22 | 203 | `signature_scanner_harness` | 0.093 | 1.151 | 0.622 |
| R23 | 205 | `instruction_validator_harness` | 0.098 | 1.143 | 0.621 |
| R24 | 207 | `cinematic_initialization_harness` | 0.099 | 1.311 | 0.705 |
| R25 | 209 | `coordinator_recovery_harness` | 0.093 | 1.153 | 0.623 |
| R26 | 211 | `cinematic_exit_authority_harness` | 0.099 | 1.157 | 0.628 |
| R27 | 213 | `horplus_post_exit_recovery_harness` | 0.099 | 1.175 | 0.637 |
| R28 | 215 | `cinematic_selection_harness` | 0.101 | 1.165 | 0.633 |
| R29 | 217 | `post_cinematic_exclusion_harness` | 0.094 | 1.165 | 0.630 |
| R30 | 219 | `recovery_rearm_harness` | 0.095 | 1.161 | 0.628 |
| R31 | 221 | `candidate_hardening_harness` | 0.102 | 1.172 | 0.637 |
| R32 | 223 | `policy_snapshot_harness` | 0.094 | 1.147 | 0.620 |
| R33 | 225 | `camera_state_snapshot_harness` | 0.092 | 1.153 | 0.623 |
| R34 | 227 | `fov_observation_harness` | 0.101 | 1.149 | 0.625 |
| R35 | 229 | `gameplay_baseline_harness` | 0.163 | 1.170 | 0.666 |
| R36 | 231 | `gameplay_aspect_restoration_harness` | 0.094 | 1.183 | 0.639 |
| R37 | 233 | `cinematic_fov_harness` | 0.193 | 1.183 | 0.688 |
| R38 | 235 | `cinematic_aspect_harness` | 0.100 | 1.184 | 0.642 |
| R39 | 237 | `dialogue_fov_harness` | 0.101 | 1.161 | 0.631 |
| R40 | 239 | `diagnostics_gate_harness` | 0.097 | 1.156 | 0.626 |
| R41 | 241 | `dialogue_source_provenance_harness` | 0.101 | 1.185 | 0.643 |
| R42 | 243 | `camera_writer_identity_harness` | 0.096 | 1.166 | 0.631 |
| R43 | 245 | `cinematic_lifecycle_context_harness` | 0.105 | 1.187 | 0.646 |
| R44 | 247 | `performance_telemetry_harness` | 0.098 | 1.165 | 0.631 |
| R45 | 249 | `language_state_transition_harness` | 0.094 | 1.154 | 0.624 |
| R46 | 251 | `matchgameplay_prediction_harness` | 0.102 | 1.153 | 0.627 |
| R47 | 253 | `runtime_evidence_replay_harness` | 0.097 | 1.163 | 0.630 |
| R48 | 255 | `gameplay_camera_resolver_harness` | 0.171 | 1.163 | 0.667 |
| R49 | 257 | `composition_presenter_state_harness` | 0.124 | 1.153 | 0.639 |
| R50 | 259 | `activation_trace_harness` | 0.102 | 1.169 | 0.635 |
| R51 | 261 | `overlay_layout_metrics_harness` | 0.098 | 1.150 | 0.624 |
| R52 | 263 | `selector_tooltip_state_harness` | 0.098 | 1.175 | 0.637 |
| R53 | 265 | `input_state_harness` | 0.099 | 1.157 | 0.628 |
| R54 | 267 | `camera_integration_harness` | 0.107 | 1.167 | 0.637 |
| R55 | 269 | `placement_config_harness` | 0.550 | 1.843 | 1.197 |
| R56 | 271 | `feature_presentation_harness` | 0.106 | 1.264 | 0.685 |
| R57 | 273 | `semantic_snapshot_harness` | 0.104 | 1.156 | 0.630 |
| R58 | 275 | `localization_harness` | 1.716 | 2.838 | 2.277 |
| R59 | 278 | `runtime_native_adapter_harness` | 33.151 | 41.500 | 37.326 |
| R60 | 280 | `n06_m01_zoom_game_derived_harness zoom-in off` | 0.144 | 0.184 | 0.164 |
| R61 | 282 | `n06_m01_zoom_game_derived_harness zoom-in on` | 0.118 | 0.158 | 0.138 |
| R62 | 284 | `n06_m01_zoom_game_derived_harness zoom-out off` | 3.415 | 0.165 | 1.790 |
| R63 | 286 | `n06_m01_zoom_game_derived_harness zoom-out on` | 0.120 | 0.162 | 0.141 |
| R64 | 290 | `runtime_native_adapter_harness` | 36.836 | 46.586 | 41.711 |

## Raw evidence / preservation

Raw artifacts are ignored under `build-artifacts/test-infrastructure-audit/`. Durable measurements and exact command/source inventory are included in the three reports.

| File | SHA-256 |
| --- | --- |
| canonical-1.log and canonical-2.log | 47361FAC550C05C5CD59137F9F38EABF97F31C34B2F74A814750A572747E12BA |
| profile-1.log | F1477062A3D09F8B43AE3D54F78E36D9FB75BA333F0D571B2B64955A90DF15B1 |
| profile-2.log | 8DB0436191D697BFAAB004D1D1E9C6B30E55DF1DBBF327436F68A8B086C3E1B9 |
| cmake-1.log | 74B34FBFCA20862F1922799C2273B9A44033FC3050344F54F68869DDB3EA5D0F |
| cmake-2.log | 76C5C80AF1C89EC5D11A3F8FE6142F573B43DEFBBAF04B3F02A8CACBE16006CA |
| timing-events.csv | AB544F36C076231350C41605F1A144AEA90E2E7745981C2DE2B3C2B344597CF2 |
| inventory.json | F81A1D5F09B233080DBE9A2FB665E8401EEECDF2E4C91E64E7EBC929D7BF4AF2 |
| preserved-before.csv | 91FF4E83C3F85AD320FCF1EB43146C891B094C9D2E9BBC31802D85DF56230F9E |

**436 initial source/vendor/test/locale/tool/doc/root files unchanged by SHA-256.** Git index unchanged: `A7C0C921F1C90E638719EB6ED3C11DE3AC0073B33B79621CB6D0CBBD5CA05380`. Status compared with initial snapshot. Git diff returned a work-tree error despite successful status/rev-parse/ls-files; preservation is supported by hashes and status comparison, not a claimed clean diff.

Only the three requested reports are intended tracked additions. No game, production ASI publication/build, scenario/source edits, index writes or N03–N06 status changes. Stop after audit; implementation requires approval.

