# CMake + Ninja Build Migration — Targeted Audit

Date: 2026-10-11  
Status: Audit complete; migration implementation proposed, not performed.

## Recommendation

Proceed with a staged CMake + Ninja migration alongside the current build, and replace the implementation behind `build.cmd` only after command, artifact and regression parity checks. Keep MSVC, the canonical ASI filename, the existing diagnostic wrapper, startup journal and runtime diagnostics model. Use one explicit production source manifest, separate build trees, and dependency-driven generation/resource compilation.

No build, test suite, CMake configure, game launch or ASI installation was run for this audit. Inspection and tool version queries establish feasibility, not the behavior or speed of a future build.

## Current baseline and provenance

The subject is the current dirty working tree, after Diagnostics Unification, not HEAD alone. HEAD is `db67e4dfc56cfe06394c38413f5bb460f797d210`; existing staged/unstaged changes were preserved. The index SHA-256 before this audit is `18FEC796063117252E9CEB62A18F3784ADCF1E01FBF803E442A0AF6D0F089C43`.

At final inspection the four script hashes below were unchanged and HEAD was unchanged. The index hash had become `C657B65B0C4CA88C3FDC1F2AE8C16A9BBFB18F60B46F672C4A2C93786D51E6BA`, and previously unstaged changes appeared staged. No index-writing command was issued by this audit; the origin of the concurrent index update was not investigated or reverted.

| Input | SHA-256 |
|---|---|
| `build.cmd` | `BC235F718CF97C754FF0DF25CDF061A70F40B01658A58806216D79AEC8E0D218` |
| `test.cmd` | `BB5561E5F863D0184A561A951CE6D372A907EB38C26A5AAE459A9AF492D6FE98` |
| `tests/runner/runtime_native_adapter_inputs.ps1` | `B78D99ED20CDD249199CD8DF4F6601C05D60C7342F8936F61C215C4B67F95AF1` |
| `tools/generate_ini_documentation.ps1` | `C1701D9DA457EBE33A14399A432AC05F8A39E1BA696C166AA2F68965DA2F1E07` |

Installed tooling observed: Visual Studio at `E:/Work/Visual Studio Community`, default MSVC tools version `14.44.35207`, bundled CMake `3.31.6-msvc6`, bundled Ninja `1.12.1`. CMake/Ninja were not resolved through the inspected ordinary PATH; discover their bundled executable paths explicitly. The CMake executable is under `Common7/IDE/CommonExtensions/Microsoft/CMake/cmake/bin/`; Ninja is under `Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/`.

The preceding [Diagnostics Unification report](DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_REPORT.md) records suite exit 0, 60/60 inventory, 102 adapter cases × OFF/ON, four N06-M01 cases, and both build exits 0. Those are historical baseline evidence, not checks of CMake output. Its ordinary ASI hash is `3693A6A014AE41787CFBBDC4C4823B046D86AE0880DE27DE3B827C7511EBA077`; wrapper artifact hash is `4A9177C3C000132B39DD5EF6EA230885B8B421CE7740C61B384EB113F76C0574`.

## Actual build graph

| Responsibility | Current owner / behavior | Migration consequence |
|---|---|---|
| Toolchain bootstrap | `build.cmd:5–19`; VSDevCmd, host/target x64; vswhere fallback requests VS `[17.14,18.0)` | Preserve discovery and validate any existing VSINSTALLDIR branch; configure CMake only inside the initialized environment. |
| Production source selection | `build.cmd:48–50`; 61 explicit unique source paths | Move the list to an explicit manifest consumed by CMake and PowerShell. |
| Compilation/link | `build.cmd:47–51`; one `cl /LD` invocation with all sources | Compile per translation unit using Ninja dependencies, then link the same DLL/ASI. |
| Locale/resource preflight | `localization_catalog_audit.ps1` before compilation | Preserve failure propagation and coverage; distinguish verification from generated-file rules. |
| INI documentation header | `generate_ini_documentation.ps1`, from `locales/en.json`; fixed output under `build-artifacts/generated/` | Add configurable binary-tree output, explicit dependencies and content-stable writes. |
| Embedded resources | `rc` always reruns; shared production `.res` path | Isolate per tree; declare `.rc`, resource IDs, 18 catalogs, fonts and license text dependencies. |
| Production artifacts | Default ASI in repository root; objects in `build-artifacts/obj`; environment overrides | Build internally in an isolated tree, publish the canonical filename only after successful validation/build. |
| Adapter dependencies | `runtime_native_adapter_inputs.ps1:3–8` parses batch indentation/source lines | Replace textual batch parsing with manifest consumption. |
| Test registration | `test.cmd`; 60 harnesses and immediate failure checks, audited by regex | Keep the canonical entry point and inventory equality; migrate audit to explicit target/case data if commands change. |
| Focused native compilation | `native_runtime_evidence.ps1:64–96`; response files and optional basename-based object reuse | Give actual-adapter targets their own build objects and flags; retire unvalidated ad-hoc object reuse for migrated targets. |
| Compatibility wrappers | `build-diagnostic.cmd`, `build-overlay-settings.cmd` delegate to `build.cmd` | Keep names, output defaults, exit propagation and supported environment options. |

### Source inventory

61 production inputs comprise 54 project and 7 vendor translation units:

| Area | Count | Files (under the listed directory unless qualified) |
|---|---:|---|
| `src/plugin` | 5 | `runtime.cpp`, `runtime_settings.cpp`, `worker_lifecycle.cpp`, `feature_status.cpp`, `dll_entry.cpp` |
| `src/config` | 3 | `feature_config.cpp`, `config_repository.cpp`, `config_template.cpp` |
| `src/diagnostics` | 4 | `diagnostic_runtime.cpp`, `cinematic_continuation_trace.cpp`, `dialogue_source_provenance.cpp`, `performance_telemetry.cpp` |
| `src/hooks` | 3 | `signature_scanner.cpp`, `instruction_validator.cpp`, `hook_set.cpp` |
| `src/gameplay` | 4 | `gameplay_state.cpp`, `gameplay_camera.cpp`, `aspect_policy.cpp`, `horplus_gameplay.cpp` |
| `src/cinematics` | 4 | `cinematic_fov.cpp`, `cinematic_selection.cpp`, `cinematic_aspect.cpp`, `cinematic_initialization.cpp` |
| `src/dialogue` | 2 | `dialogue_fov.cpp`, `dialogue_state.cpp` |
| `src/camera` | 6 | `camera_state_snapshot.cpp`, `fov_observation.cpp`, `gameplay_baseline.cpp`, `gameplay_aspect_restoration.cpp`, `presentation_state.cpp`, `horplus.cpp` |
| `src/platform/win32` | 4 | `memory.cpp`, `sha256.cpp`, `window.cpp`, `viewport.cpp` |
| `src/overlay/presentation` | 6 | `camera_integration.cpp`, `feature_presentation.cpp`, `camera_state_view.cpp`, `selector_documentation_view.cpp`, `overlay_layout_metrics.cpp`, `placement_config.cpp` |
| `src/overlay/localization` | 7 | `localization_catalog.cpp`, `localization_formatter.cpp`, `localization_validator.cpp`, `localization_manager.cpp`, `localization_font.cpp`, `localization_keys.cpp`, `game_language_reader.cpp` |
| Remaining `src/overlay` | 6 | `composition/composition_runtime.cpp`, `presenter/composition_presenter_state.cpp`, `input/input_state.cpp`, `presenter/renderer_state.cpp`, `presenter/renderer_runtime.cpp`, `presenter/imgui_d3d11_renderer.cpp` |
| Vendor | 7 | `external/safetyhook/safetyhook.cpp`, `external/safetyhook/Zydis.c`, `external/imgui/imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `external/imgui/backends/imgui_impl_win32.cpp` |

No recursive glob should add sources to production automatically. Inventory checks may scan the tree to detect missing classifications, but membership must stay explicit; research watchers/experiments must not enter production through discovery.

## Confirmed constraints and migration hazards

| ID | Finding / evidence | Required handling |
|---|---|---|
| C01 | `build.cmd` always passes all 61 sources to `cl`; existing objects do not select compilation work | Introduce a dependency graph. Speedup is expected, but remains unmeasured until the candidate exists. |
| C02 | Adapter registry is inferred from batch text and ignores `runtime.cpp`/`dll_entry.cpp` by string filtering | Manifest must encode their roles explicitly and produce the same 59 dependency inputs. |
| C03 | Generator always uses `WriteAllText`, even for identical content | Avoid rewriting unchanged output; express script/catalog dependencies so no-op builds skip generation. |
| C04 | `CAMERA_TWEAKS_OBJECT_DIR` isolates objects only; generated header and production resource output remain shared | Isolate all generated files, `.res`, objects, linker outputs and caches per configuration. |
| C05 | All test harnesses currently share `build-artifacts/tests` object names despite different flags/subsets | Do not link production objects into test targets or share test objects across incompatible compile contracts. |
| C06 | Actual adapter includes production `runtime.cpp` inside its harness TU; N06-M01 includes that harness after renaming main | Do not compile `runtime.cpp` separately in either adapter target. Keep `runtime_owner_binding_probe.cpp`, exclude `dll_entry.cpp`, and keep test-only SafetyHook fault-injection objects separate. |
| C07 | Last `test.cmd` compilation of the adapter repeats the earlier command with the same defines | Preserve current execution coverage initially; do not label the duplicate as a distinct compile-time diagnostics profile. OFF/ON are runtime cases. Deduplication requires explicit equivalence evidence. |
| C08 | CMake configuration defaults can change CRT, optimization, language flags, DLL definitions or linker behavior | Compare effective commands; do not assume CMake Release matches this build. |
| C09 | `.rc` embeds relative paths rooted in the repo and includes `src/localization/localization_resource_ids.h` | Preserve resolution using explicit working directory/include roots, plus every embedded-file dependency. Verify from a path containing spaces. |
| C10 | Profile, timeline, discovery and output environment options are evaluated every batch invocation today | Map them into cache/preset configuration explicitly every invocation; prevent sticky values after an option is cleared. |

### Compiler and linker preservation contract

Preserve x64 MSVC and actual effective C/C++ language handling, `/nologo`, `/std:c++latest` for C++, `/O1`, `/MT`, `/EHsc`, `/W4`, `/utf-8`, `/DNDEBUG`, current include paths, and generated-header include availability. Keep `Zydis.c` compiled as C; do not force all files to C++.

Preserve `OVERLAY_PRODUCTION`, `OVERLAY_SETTINGS_FRONTEND`, `OVERLAY_COMBINED`. Default `OVERLAY_STARTUP_JOURNAL`; when `CAMERA_TWEAKS_STARTUP_TIMELINE=1`, replace it with `OVERLAY_STARTUP_TIMELINE` as today. All approved passive diagnostics are compiled unconditionally and gated by INI. `DIALOGUE_DISCOVERY_DIAGNOSTIC` remains opt-in within the compatibility diagnostic profile and is not promoted by a default CMake option.

Preserve DLL entry/CRT startup behavior and `.asi` suffix with the canonical filename `STALKER2CameraTweaks.asi`; preserve wrapper default `STALKER2CameraTweaksDiagnostic.asi`. Link `user32`, `psapi`, `bcrypt`, `dxgi`, `d3d11`, `dcomp`, `d3dcompiler` and the embedded resources. Current script has no explicit `/GL`, `/LTCG`, `/ENTRY`, `/DEBUG`, `/INCREMENTAL` or DEF file; record effective legacy linker defaults before deciding CMake equivalents. Additional CMake-generated definitions/options, automatic exports, manifests and import-library outputs need explicit comparison rather than silent acceptance.

Configure static CRT via `MSVC_RUNTIME_LIBRARY=MultiThreaded` and applicable policy before language enablement. Explicitly prevent conflicting CMake Release `/O2` or `/MD` defaults; do not accept last-flag-wins as unreviewed parity. Use inspected `/std:c++latest`, not a guessed C++23 mapping that could select another switch on this compiler.

## Proposed architecture

Use single-config Ninja with separate preset binary directories under `build-artifacts/cmake/production`, `diagnostic`, and `tests`; timeline/discovery variants must have separate trees or an explicit complete cache reconfiguration. Target-local object paths naturally distinguish files/targets. Keep generated data beneath each tree. No production object target is shared with a fault-injected or macro-substituted adapter.

Add one versioned `tools/build/production_sources.json` listing the exact 61 production sources with roles `runtime_owner`, `dll_entry`, `dependency`, `vendor`. Both CMake and PowerShell consume it directly. Reject duplicate/missing paths and unknown roles; check the baseline set exactly during migration. Tests may have a separate explicit harness/case registry because their sources, arguments and flags are a different contract. The production file membership must not be duplicated there.

CMake targets select from this manifest: the ASI uses all 61 inputs; actual adapters use the 59 dependencies plus their wrapper/probe, whose included runtime supplies the owner implementation exactly once. Pure harnesses retain their current named subsets/compile contracts, checked against the production manifest where applicable. Do not create one universal shared library that changes linking/initialization or inadvertently mixes injection flags.

`build.cmd` remains a thin Windows bootstrap: locate supported VS and bundled/explicit CMake/Ninja, initialize x64 environment, map supported options, run preflight, configure the selected preset, build the ASI target, publish the requested output after success, and propagate failures. It should not contain another production source list or individual compiler commands. Preserve `CAMERA_TWEAKS_OUTPUT_NAME`; define and document how `CAMERA_TWEAKS_OBJECT_DIR` maps to an isolated binary-tree root before replacing the entry point, since its former `/Fo` meaning cannot be retained literally by Ninja.

Prefer an ASI CMake `MODULE` target with explicit name/prefix/suffix and verified native DLL startup/link options. Build internal artifacts first; use a content-aware publish step to preserve the root filename and avoid rewriting it on no-op builds. Concurrent configurations may build independently, but publishing to the same destination must be serialized or rejected. Failed builds must not replace the previous usable ASI.

Generation should be an `OUTPUT` custom command with explicit `DEPENDS` on generator and canonical catalog, binary-tree output parameter, and content-stable writes. Resource compilation should have a single owner per tree, source-root resolution, and dependencies on the RC, ID header, all catalogs and embedded font/license files. An explicitly bounded custom RC command is acceptable for the initial parity candidate if it best preserves current behavior. A missing output must regenerate before consumers compile/link.

Keep locale/resource validation at canonical entry invocation initially; measure its time separately. A no-op compiler build need not mean zero preflight work. Do not replace checks with a stale success stamp without tracking every audit input. Avoid always-running generation targets whose unchanged-file writes would invalidate C++ compilation.

CMake floor recommendation: 3.31, compatible with inspected 3.31.6; Ninja 1.12.1 is present. Do not depend on new 4.x-only test preparation features. Build test targets explicitly before running CTest if CTest is introduced. `test.cmd` remains the canonical failure-propagating entry point, and inventory equality/argument-bearing N06 cases remain mandatory.

## Proposed implementation stages and acceptance

| Stage | Scope | Acceptance before advancing |
|---|---|---|
| M0 — Immutable comparison inputs | Record dirty-tree input hashes, full legacy commands/toolchain/options, outputs and elapsed times in isolated directories | Old/new candidate must use the same immutable source snapshot, toolchain and profile. Preserve existing changes and installed/release artifacts. |
| M1 — Explicit source registry | Introduce manifest, readers and equality checks; retain canonical legacy build initially | Exact 61-source production set; 59-source adapter set; runtime included once; no discovery sources. |
| M2 — Parallel CMake candidate | Add root CMakeLists, presets, flags, ASI target, generation/RC dependencies and isolated trees; retain old canonical entry | Ordinary and wrapper candidate build successfully; flags/link/resources/CRT inspected against old build. |
| M3 — Test integration | Use manifest in test infrastructure; introduce incremental test targets while preserving case coverage and test-only defines | 60/60 inventory, 102×OFF/ON adapter matrix and four N06-M01 cases; focused regressions, all immediate failures propagate. Validate missing-input/failed-build behavior. |
| M4 — Parity and incremental measurements | Compare old/new artifacts, run change matrix and whole suite on candidate; record warnings/hashes | Correct dependency invalidation, no unintended flags or resources; no-op skips C/C++/RC/link; measured distributions; candidate regressions pass. |
| M5 — Canonical entry switch | Replace build implementation behind entry/wrappers only after parity gates | Output/env compatibility, clean machine/path-with-spaces flow, preserved previous ASI on failure, full tests/builds pass. Update build docs and retain documented rollback path. |

### Required incremental matrix

| Trigger | Expected candidate work |
|---|---|
| Second build with unchanged inputs | Preflight as documented; no C/C++ compilation, RC or link; output content unchanged |
| One production `.cpp` edit | That TU and link only, plus any declared dependent target work |
| Shared header edit | Recompile actual includers and relink; no unrelated generation |
| `locales/en.json` edit | INI header generation, affected header consumers if content changes, RC/link, audit |
| Non-English catalog / embedded font/license edit | RC/link and audit; INI header unchanged |
| Generator edit with identical emitted content | Run generator; preserve header timestamp/content to avoid unrelated C++ rebuild |
| Delete generated header or `.res` | Regenerate missing output before consumers; successful clean recovery |
| Manifest edit | Reconfigure graph; reject duplicate/missing/unclassified input |
| Toggle timeline/profile/discovery | Reconfigure selected isolated variant; no stale defines/objects |
| Production → wrapper → production | No mixed objects/resources; ordinary output keeps full passive diagnostics |
| Build/test failure or output path containing spaces | Nonzero exit reaches caller; old canonical ASI remains intact; quoted paths work |

Measure clean compilation, unchanged invocation, one-TU rebuild, header rebuild and resource-only rebuild separately. Use matched snapshots/tool versions, multiple repetitions, and report median/range with preflight/configure/compile/link boundaries. Do not invent speedup numbers or equate the earlier diagnostics runtime microbenchmark with build time.

### Old/new artifact and behavior parity

Require recorded verbose compile/link/RC commands, compiler version, `_MSVC_LANG` check, profile definitions and input hashes. Compare architecture/PE imports/exports/entry characteristics, static CRT choice, embedded locale/font/license contents, generated INI documentation, native callback/link symbols where observable, and warnings. Record both SHA-256 values; differing hashes alone do not demonstrate a regression because timestamps/link metadata can differ. Investigate code/metadata differences; do not demand byte identity as a substitute for behavior validation.

Run full canonical tests against the candidate test targets, including Diagnostics OFF/ON, real production callbacks, native register/store ordering, camera lifecycle/recovery and game-derived Zoom continuation. A separately authorized matched live smoke of the current canonical-named ASI with Diagnostics OFF/ON follows the migration: readiness, Gameplay/ADS/Zoom, first/chained Cinematics, Dialogue, recovery, resolution/aspect change, Overlay and normal exit. Record current artifact identity. This does not close unrelated N03–N06 gates automatically.

## Expected files and boundaries

Implementation candidates: root `CMakeLists.txt`, `CMakePresets.json`, manifest and narrow CMake helpers under `tools/build/`, `build.cmd`, compatibility wrappers only as needed, `test.cmd`, `tests/runner/runtime_native_adapter_inputs.ps1`, `native_runtime_evidence.ps1`, `test_cmd_audit.ps1`, INI generator output handling, and build documentation. Keep physical CRLF for `test.cmd` and current Git attributes.

Camera algorithms, native hooks/vendor code, runtime ownership, INI semantics, localization text and release contents are outside this migration. Passive diagnostics remain always compiled. Research profiles are not automatically added to CMake default targets. Preserve N03 `Partially mitigated` and the existing N04–N06 evidence status. The already completed runtime decomposition stays closed.

## Documentation consulted

- Current `docs/ARCHITECTURE.md`, `docs/SAFETY_INVARIANTS.md`, `TESTING_AND_RESEARCH.md` (historical entries distinguished from current script state).
- Current build/wrapper scripts, canonical test registry, actual-adapter wrappers, resource script and generator listed above.
- [CMake Ninja generator](https://cmake.org/cmake/help/latest/generator/Ninja.html): generates the Ninja graph in the binary tree.
- [CMake MSVC runtime library](https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html): `MultiThreaded` selects static CRT; default may select DLL CRT, so it must be explicit.
- [CMake custom command](https://cmake.org/cmake/help/latest/command/add_custom_command.html): output/dependency/byproduct rules and shared-output ownership.
- [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html): configure/build preset model.
- [Ninja manual](https://ninja-build.org/manual.html): dependency-driven work selection and diagnostic inspection.

No canonical script, source, Git index, configuration or binary was changed by this audit. Only this report was added. Recommendation: approve the bounded M0–M5 plan before implementation.

## Post-audit implementation status

The approved M0–M5 implementation was subsequently completed in the dirty working tree. Its final checks, artifact identities, incremental measurements, and remaining runtime-validation boundary are recorded in [CMAKE_NINJA_BUILD_MIGRATION_IMPLEMENTATION_REPORT.md](CMAKE_NINJA_BUILD_MIGRATION_IMPLEMENTATION_REPORT.md). This implementation addendum does not alter the historical audit observations above.
