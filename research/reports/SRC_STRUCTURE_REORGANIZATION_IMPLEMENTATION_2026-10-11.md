# Source Structure Reorganization — Implementation Report

Date: 2026-10-11  
Scope: approved Batch D and Batch L from `SRC_STRUCTURE_REORGANIZATION_PLAN.md`  
Outcome: complete; four physical moves and path-only reference edits

## Changes

### Batch D — language-state research group

Moved, byte-for-byte:

- `src/diagnostics/language_state_watcher.cpp` → `src/diagnostics/language_state/language_state_watcher.cpp`
- `src/diagnostics/language_state_transition.cpp` → `src/diagnostics/language_state/language_state_transition.cpp`
- `src/diagnostics/language_state_transition.hpp` → `src/diagnostics/language_state/language_state_transition.hpp`

Updated the independent watcher build script's two source paths and include directory, the `test.cmd` classifier source path, and the transition harness include. The watcher remains an independent research build and does not enter production `build.cmd`.

### Batch L — shared localization resource IDs

Moved unchanged:

- `src/overlay/localization/localization_resource_ids.h` → `src/localization/localization_resource_ids.h`

Updated the locale registry include, Overlay font loader include, resource script include and localization audit lookup. Resource IDs and payload declarations were not changed.

All four moved files have exactly the pre-move SHA-256:

| File at target | SHA-256 |
| --- | --- |
| `src/diagnostics/language_state/language_state_watcher.cpp` | `7C6181B15AAA1075C153558830BC94E09BFAD198EEA7BC9B4BAE71A5277F8956` |
| `src/diagnostics/language_state/language_state_transition.cpp` | `A2D8D102998E7AE45DE6E3711F8B91B3DE8243688965D9FD864533C864E0995C` |
| `src/diagnostics/language_state/language_state_transition.hpp` | `D5FCB520182500FE3BFAFB6D496E1FBFC5E5E43C5F644BD860DE83FBA0FCFF3E` |
| `src/localization/localization_resource_ids.h` | `EC860DF4C3EF0009F17F7493AA9B2A9368CFF929F872F25D49AE74BA84728213` |

The source inventory remains 143 files. No file was renamed, split, merged or deleted.

## Baseline failure diagnosis

The earlier placement harness failure was environment-dependent. Running it with the sandbox system TEMP/TMP reproduced save and migration failures plus lost concurrent writes. Running the same existing harness with TEMP/TMP pointed to the workspace-owned `build-artifacts/tests/temp` passed. After the moves, the harness was rebuilt from current sources and rerun with that workspace path:

- Compile: exit 0.
- Run: exit 0.
- Save/load and migration: PASS.
- Concurrent configuration/placement updates: 24/24 PASS; no setting or coordinate lost.
- Failure preservation and save disposition: PASS.

This identifies the system temporary directory restriction in the sandbox as the cause of the placement result. No persistence code was changed.

The full `test.cmd` baseline remains unresolved and did not become green:

- Exit code: 2.
- Test audit: 59 registered harnesses; `n06_m01_zoom_game_derived_harness` is unregistered in the runner.
- The compiler log reports that C++17 language features are unavailable, despite the test script intending `/std:c++latest`.
- Direct compilation with the same MSVC installation and explicit `/std:c++latest` succeeds for the moved classifier and placement harnesses. Production and diagnostic builds using the project build entry points also succeed. The reason the full test invocation reaches an older effective language mode has not been established.
- No runner registration, test behavior or compiler setting was changed as part of this reorganization.

The complete captured output is in ignored `build-artifacts/tests/src-structure-final-test.log`. These failures are recorded as runner/environment findings, not attributed to the four path moves.

## Validation

| Check | Result |
| --- | --- |
| Language-state transition harness and standalone watcher link | PASS, exit 0 |
| Placement harness, current moved sources, workspace TEMP/TMP | PASS, exit 0 |
| Localization/resource audit | PASS, 18 locales / 212 keys |
| Production `build.cmd` | PASS, exit 0 |
| Diagnostic `tools/build/build-diagnostic.cmd` | PASS, exit 0 |
| Resource compiler in both supported builds | PASS |
| Full `test.cmd` | FAIL, exit 2; known inventory and C++ language-mode diagnostics above |
| Old active source paths across src/tools/tests/build/test/N06 runners | No stale references found |
| Source inventory and moved-file hash comparison | PASS, 143 files; all four contents unchanged |
| `git diff --check` | Unavailable; Git returns “this operation must be run in a work tree” (exit 128) |

Both ASIs were built only under ignored `build-artifacts`; neither was installed or run. The watcher ASI was only linked, not launched.

| Development artifact | SHA-256 |
| --- | --- |
| `build-artifacts/source-structure/STALKER2CameraTweaks.asi` | `A0A7311B9D867340FD9F5FBF57752E45AEDA53FAE0AAB40F137C2984A5E6C4B4` |
| `build-artifacts/source-structure/STALKER2CameraTweaksDiagnostic.asi` | `83C381BCB9FF8BA44D25683D8CC3DBC0273AC8A3E18285CE811030CCEC1CA5DF` |
| `build-artifacts/language-state-watcher/STALKER2LanguageStateWatcherDiagnostic.asi` | `F0C2BB10DC1C86C9747D5735E9304BF33172C8FEB01230F8190E728A2D8E0785` |

The only compiler warnings during production/diagnostic builds were two C4201 unnamed-struct warnings in external SafetyHook's Zydis header; no warning originated from the moved project files.

## Preservation and scope

All 143 source files remain present. Moved file contents match their captured pre-move hashes. No reset, clean, checkout or staging commands were used. Initial and final status snapshots differ in staging flags on some pre-existing paths; those paths were not intentionally modified by this task, and their final index state was left untouched. Exact staged/unstaged status preservation cannot therefore be claimed. `build.cmd` content hash and production source membership were unchanged. No release assets, configuration, vendor files, runtime semantics, module ownership or B1–B7 decisions changed.

No game launch, ASI installation or release packaging was performed.

