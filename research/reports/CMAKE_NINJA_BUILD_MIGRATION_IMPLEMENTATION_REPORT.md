# CMake + Ninja Build Migration — Implementation Report

Date: 2026-10-11
Status: M0–M5 implemented and offline validation passed. Live in-game validation was not performed.

## Result

`build.cmd` now remains the normal build entry point and delegates to an isolated CMake + Ninja graph. It retains the supported Visual Studio/MSVC toolchain, production ASI name, diagnostic wrapper, environment options, resource compiler, startup journal, and always-compiled passive diagnostics. `test.cmd` remains the canonical full test runner.

The migration changed build and test infrastructure only. All 61 production translation-unit inputs match the immutable M0 baseline hashes (61/61); no camera/runtime C++ source or vendor file was changed by this migration.

## M0–M5 completion

| Stage | Result |
|---|---|
| M0 — comparison inputs | Captured the dirty-tree source manifest, input hashes, toolchain, and legacy artifacts in ignored `build-artifacts/cmake-migration/baseline/`. Existing staged and unstaged work was preserved. |
| M1 — explicit registry | Added `tools/build/production_sources.json`: 61 explicit production sources (54 project + 7 vendor), exactly one `runtime_owner`, one `dll_entry`, and 59 actual-adapter dependency inputs. Runtime remains included once through the harness; `dll_entry.cpp` is excluded from that harness. |
| M2 — parallel CMake build | Added the CMake project, four isolated presets, candidate/build helpers, content-stable generated INI documentation, explicit RC inputs, profile-specific object/resource/output trees, and atomic ASI publishing. |
| M3 — test integration | Added isolated CMake actual-adapter and N06-M01 Zoom targets. The helper configures under the initialized VS environment and checks all five CTest entries. The original `test.cmd` registry and coverage remain intact. |
| M4 — parity and incrementality | Verified no-op, translation-unit, shared-header, resource-only, missing-generated-output, path-with-spaces, and failed-publication/configuration cases. PE and resource comparisons are recorded below. |
| M5 — canonical switch | `build.cmd` now delegates to CMake + Ninja and publishes only after a successful build. A configuration failure propagated as exit 1 and left the previous canonical ASI hash unchanged. |

## Dependency-tracking issue found and fixed

The first shared-header test exposed a serious locale interaction: MSVC emitted Russian `/showIncludes` notices while CMake detected/stored a mojibaked prefix. Ninja therefore recorded `runtime.cpp.obj: #deps 0`, so a header edit could be missed.

The build helpers now select console code page 65001 before CMake configuration. With this setting, the generated rule contains the exact UTF-8 prefix `Примечание: включение файла: ` and Ninja recorded 241 dependencies for `runtime.cpp.obj`. A timestamp-only change to `src/plugin/runtime.hpp` then rebuilt exactly its five includers (`dll_entry.cpp`, `game_language_reader.cpp`, `composition_runtime.cpp`, `renderer_runtime.cpp`, `runtime.cpp`) and relinked. Source bytes and original timestamp were restored after each trigger test.

A second graph defect was found in the resource-only test: the `.res` was passed as a linker option, so rebuilding it did not invalidate the link edge. It is now a generated external-object source of the production target. Touching a locale input produces one RC rebuild and one relink; test harnesses do not receive production resources they do not consume.

## Validation results

| Check | Result |
|---|---|
| Production manifest | 61/61 input hashes match the immutable M0 baseline; no missing/extra source entries. |
| Adapter dependency contract | 59 entries; `runtime.cpp` included once through the production adapter harness; `dll_entry.cpp` excluded. |
| Canonical `test.cmd` | Exit 0. 60/60 harness inventory and failure checks passed; 102 actual-adapter cases × Diagnostics OFF/ON (204 fresh processes); four game-derived Zoom IN/OUT × OFF/ON cases passed. Full output: ignored `build-artifacts/cmake-migration/test-cmd-final.log`; exit marker `test-cmd-exit.txt` contains `0`. |
| CMake candidate tests | Exit 0; 5/5 CTest entries passed, including the 204-process actual-adapter matrix and all four N06-M01 Zoom invocations. Final elapsed CTest time: 137.39 s. |
| Production build via `build.cmd` | Exit 0; ordinary ASI produced and atomically published. |
| Diagnostic build via `tools/build/build-diagnostic.cmd` | Exit 0; diagnostic wrapper produced and published. |
| Localization/resource audit | PASS for all 18 catalogs, embedded resources, glyph coverage, and right-panel tooltip coverage. |
| Missing generated files | Deleting only the generated INI header and `.res` in the production build tree regenerated both; only `config_template.cpp` recompiled and the ASI relinked. Exit 0. |
| CMake output override with spaces | Build and publish passed to `build-artifacts/cm migration output/STALKER2CameraTweaks.asi`; repeated no-op preserved both its SHA-256 and timestamp. |
| Failure preservation | Forced an invalid object-directory configuration: build exited 1 and canonical ASI hash was unchanged. Separately, publisher with a missing source exited 1 and likewise preserved the canonical output. |
| INI generator stability | Re-running the generator against identical input preserves generated content; the build no-op also skipped generation, compilation, RC, and link. |
| Game / ASI installation | Not performed, as requested. |

### Incremental timing samples

Times include VS environment startup, CMake configure/generate, and the always-run localization preflight where applicable. They are workflow measurements, not compiler-only timings or a claimed speedup over legacy.

| Trigger | Samples | Median | Range | Observed work |
|---|---:|---:|---:|---|
| No-op | 3 | 26.947 s | 26.838–38.629 s | No C/C++, RC, INI generation, or link; locale preflight remains. |
| One `.cpp` timestamp change | 3 | 34.596 s | 33.985–34.752 s | One translation unit + link. |
| `runtime.hpp` timestamp change | 2 | 50.629 s | 49.059–52.199 s | Five actual includers + link. |
| Locale catalog timestamp change | 2 | 38.304 s | 35.828–40.780 s | RC + link, no C/C++ compilation. |

The measured no-op includes about 20 seconds of process/toolchain and audit overhead. Incremental selection is correct, but this report does not claim a substantial wall-clock speedup for the current project until a controlled legacy-vs-Ninja timing study is performed.

## Toolchain and artifact comparison

- Visual Studio 2022 17.14.28; MSVC tools 14.44.35207 / compiler 19.44.35224.0.
- Bundled CMake 3.31.6 and Ninja 1.12.1.
- Production and diagnostic object trees remain separate. Production retains the legacy `/std:c++latest`, `/O1`, `/MT`, `/EHsc`, `/W4`, `/utf-8`, NDEBUG and overlay definitions; CMake-only Windows/module defines were explicitly removed. `/MANIFEST:NO` preserves the legacy no-extra-manifest behavior.
- Only existing vendor `Zydis.h` C4201 anonymous-struct warnings appeared (two occurrences), matching the legacy build; no new warnings were observed.
- Embedded `.res` SHA-256: `45EF935B38DDD0460F4A4CE75303583270DB86CE9DE8950C42952A91F2D6B2F3`.
- Final ordinary ASI SHA-256: `FFB03F35D0C04A5A609FCB47B3A9024E2DA268FA40BB431186903233498E545A`.
- Final diagnostic ASI SHA-256: `B3B5A224A4D77E74A425F4652123ED1C5EFA099997F56422F3DBC2815FF23E66`.
- Same-tree legacy comparison artifacts: ordinary `3693A6A014AE41787CFBBDC4C4823B046D86AE0880DE27DE3B827C7511EBA077`; diagnostic `4A9177C3C000132B39DD5EF6EA230885B8B421CE7740C61B384EB113F76C0574`.
- Ordinary PE comparison: both x64, identical recorded section sizes (`.text`, `.rdata`, `.data`, `.rsrc`, `.reloc`), same 264 non-empty import-table lines, and identical resource input hash. Entry point differs by `0x10`; final ASI hashes and code bytes are not identical. Do not treat this as byte-for-byte equivalence; runtime actual-adapter tests are the offline behavior evidence. A live smoke of this exact final binary remains separate.

## Changed files and preserved state

Migration implementation files:

- `CMakeLists.txt`, `CMakePresets.json`
- `build.cmd`, `README.md`
- `tools/build/build-cmake-candidate.cmd`, `tools/build/test-cmake-candidate.cmd`, `tools/build/production_sources.json`, `tools/build/publish_asi.ps1`
- `tests/runner/runtime_native_adapter_inputs.ps1` (registry reader)
- `tools/generate_ini_documentation.ps1` (optional output path and content-stable writes)
- `research/reports/CMAKE_NINJA_BUILD_MIGRATION_AUDIT.md` (implementation pointer) and this report

The rewritten/added Windows batch entry scripts use CRLF. No change was made to the pre-existing `build-diagnostic.cmd` wrapper.

The repository already contained staged/unstaged unrelated release, runtime-remediation, Overlay, and documentation work. No reset, clean, stage, unstage, or index-writing operation was performed. The 61 production translation-unit hashes still match M0. Generated build/test logs and artifacts are under ignored `build-artifacts/cmake-migration/` and `build-artifacts/cm/`.

The CMake build migration does not change N03–N06 evidence statuses. No live game test was run; exact-artifact live validation remains a separate gate. The runtime-decomposition campaign remains closed.
