# TEST_RUNNER_FAILURE_ROOT_CAUSE_AUDIT

Date: 2026-10-11  
Scope: read-only diagnosis after SRC_STRUCTURE_CONSISTENCY batches D → L  
Repository: `E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4`  
Decision: audit complete; remediation proposed, not implemented

## Conclusion

Two confirmed runner defects explain the failure chain:

1. The inventory gate rejects `n06_m01_zoom_game_derived_harness.cpp`, which is present under `tests/` but absent from `test.cmd`.
2. The current LF-only `test.cmd` does not execute its failure branch correctly under the observed Windows command processor. After the inventory failure, execution reaches the candidate-hardening compiler command before `COMMON` is assigned. The actual compiler invocation therefore contains no `/std:c++latest`, uses the compiler's default C++14 language mode, and returns exit 2.

The C++17/string_view diagnostics are downstream effects of the second defect, not evidence that MSVC rejects a correctly supplied `/std:c++latest`.

A controlled comparison using disposable diagnostic copies established the difference: LF reached compilation and exited 2; CRLF stopped at the inventory failure and exited 1. The repository runner was not converted or repaired.

No missing active source path caused the first failure. The inventory discrepancy predates batches D/L, as shown by the earlier captured output. The exact edit that first produced the LF-only working copy cannot be attributed from the available historical snapshots.

## Findings and proposed disposition

| ID | Problem / status | Root cause or evidence limit | Minimum proposed remediation | Validation | Potential side effects |
| --- | --- | --- | --- | --- | --- |
| T01 | Unregistered N06-M01 harness — confirmed | Inventory audit enumerates all `tests/**/*_harness.cpp`; canonical batch has 59 unique harnesses, repository has 60. | Add a canonical build and four argument-bearing executions for the existing fixture; extend run-command recognition in the audit to accept arguments. Retain its separate game-image provenance tool. | Inventory equality 60/60; each of four executions checked immediately for failure; full suite. | Additional compilation and four offline executable processes; parser must continue detecting missing checks and missing executions. |
| T02 | LF-only failure control flow, absent COMMON flags, C++14 compiler errors — confirmed | Current script has zero CR bytes and 289 LF bytes. On the failing branch it reaches line 101 before line 34. Process command line confirms flags are absent. LF/CRLF control isolates line endings. | Convert only `test.cmd` to CRLF and add a narrowly scoped `test.cmd text eol=crlf` attribute so the requirement survives checkout. Preserve all commands and ordering. | With the inventory defect still present, exit 1 and no compiler launch. After T01, capture actual compiler args and verify `_MSVC_LANG`; run full suite. | A line-ending-only physical rewrite; normalized source content should remain unchanged. Attribute affects future checkout behavior, not camera behavior. |
| T03 | Installed Git diff unavailable inside sandbox — confirmed environmental combination | Git 2.56 fails worktree diff in the sandbox, succeeds outside it. Bundled Git 2.53 succeeds inside it against the same repository. Exact low-level permission/parser cause not established. | Use the available bundled Git by absolute path for read-only diff review; no repository or index repair is indicated. | Same worktree diff returns 0; index hash unchanged. | None to repository state when used with `--no-optional-locks`. |
| T04 | Historical staged/unstaged changes — confirmed difference; actor unknown | Earlier snapshot differs from current index-backed status. Current index is stable during this audit. Git does not record ordinary staging in reflog. | No stage/unstage/reset action. Preserve current index and use explicit snapshots for subsequent tasks. | Compare index SHA-256 and normalized blobs/status before and after each task. | None; deliberately leaves existing user work intact. |

T01 and T02 are actionable implementation defects. T03 is a bounded tooling issue. T04 is a provenance gap, not evidence of corrupt files or index.

## Exact failure sequence

All commands below run from the repository root.

### Primary failure: inventory gate

Location: `test.cmd:25–26`; implementation: `tests/runner/test_cmd_audit.ps1:37–40,54–60`.

Exact command:

```cmd
PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_audit.ps1 -RunnerPath test.cmd
```

Observed output:

```text
runner_sources=59 runner_compiled=59 runner_executed=59 compile_checks=True run_checks=True sets_equal=True repository_inventory=False
unregistered_repository_harnesses:
n06_m01_zoom_game_derived_harness
```

Direct subprocess exit: **1**.

These counts are static sets parsed from batch text. `runner_compiled=59` and `runner_executed=59` do not mean 59 compilations or executions actually occurred.

The next command, `if errorlevel 1 goto :fail`, should reach `test.cmd:285` and return the original failing code. The observed LF working copy instead proceeds to a compilation later in the file.

### Secondary failure: actual compiler launch

Declared source location: `test.cmd:101`.

Declared command:

```cmd
cl %COMMON% tests\dialogue\candidate_hardening_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\candidate_hardening_harness.exe
```

A read-only Windows process query captured the real executable and command line:

```text
Executable:
E:\Work\Visual Studio Community\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe

CommandLine:
cl   tests\dialogue\candidate_hardening_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\candidate_hardening_harness.exe
```

The extra spaces are the empty `%COMMON%` expansion. No `/std`, `/EHsc`, `/utf-8`, common includes or object-output directory is present.

First warning:

```text
...\MSVC\14.44.35207\include\string_view(12):
warning STL4038: The contents of <string_view> are available only with C++17 or later.
```

First compiler error:

```text
...\src\config\../localization/locale_registry.hpp(14):
error C2039: "string_view" is not a member of "std".
```

The raw compiler emits this diagnostic in Russian on this machine. Its original first-error text is `"string_view": не является членом "std".`; later C3646, C4430 and C7525 diagnostics are cascading failures.

Repeating the captured compiler command directly through the same initialized MSVC environment returned **2**. The observed full runner subprocess also returned **2**. One initial tool/shell wrapper surfaced exit 1; direct `System.Diagnostics.Process.ExitCode` measurements establish the native exit 2 used here.

No harness runtime assertion is the first failing event. The run has already failed at the inventory gate, and this compilation is unintended downstream execution.

## Flags, language mode and environment

### Intended flow

| Location | Responsibility | Observed on failing path |
| --- | --- | --- |
| `test.cmd:5–17` | Discover VSDEVCMD and call it with x64 host/target | Executed; correct compiler found |
| `test.cmd:25–26` | Child PowerShell inventory audit; fail on nonzero result | Audit returns 1; failure control flow misbehaves |
| `test.cmd:29–31` | Generate ignored documentation headers, then extend INCLUDE | Not reached before the unintended first compile |
| `test.cmd:34` | Assign COMMON, including `/std:c++latest` | Not reached before the unintended first compile |
| `test.cmd:36` | Child PowerShell writes dependency source response file | Not part of the observed first compiler launch |
| `test.cmd:38` | Intended first compilation: actual-adapter harness | Not the observed first compile |
| `test.cmd:101` | Candidate-hardening compilation using COMMON | Actual first compile, COMMON empty |

COMMON's literal intended value is:

```cmd
/nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui
```

The ordinary harness compiler commands are batch commands. COMMON is not passed through PowerShell to these compiler processes. The child inventory audit performs text inspection only. `runtime_native_adapter_inputs.ps1` writes source paths to the response file; it does not write compiler options or override COMMON. A normal child PowerShell environment also cannot assign the parent's batch variables.

### Measured environment

| Fact | Observed value |
| --- | --- |
| VS developer environment | 17.14.28 |
| VCToolsVersion directory | 14.44.35207 |
| Actual cl banner | 19.44.35224, x64 |
| `_MSC_FULL_VER` | 194435224 |
| CL environment variable after VsDevCmd | Undefined |
| _CL_ environment variable after VsDevCmd | Undefined |
| COMMON exact variable before batch assignment | Undefined; `set COMMON` prefix query lists unrelated CommonProgramFiles variables |
| Default `_MSVC_LANG` | 201402L |
| Explicit `/std:c++latest` `_MSVC_LANG` | 202400L |
| `__cplusplus`, without `/Zc:__cplusplus` | 199711L in both probes |

Language modes were measured with a disposable ignored source containing pragma messages and `cl /Zs`; no object or executable was needed. The `__cplusplus` value alone would be misleading here, so the conclusion uses `_MSVC_LANG`.

Control command:

```cmd
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cl /nologo /Zs /std:c++latest /O1 /MT /EHsc /W4 /utf-8 tests\dialogue\candidate_hardening_harness.cpp src\dialogue\dialogue_state.cpp
```

Result: **exit 0**, no compiler warnings/errors. This is syntax validation of the affected inputs, not a harness execution or full suite PASS.

No flag override was found in the relevant scripts or measured CL/_CL_ environment. The confirmed mechanism is skipped flag assignment, not unsupported compiler capabilities.

## Line-ending control

The authoritative working `test.cmd` has:

```text
Bytes: 23723
CR bytes: 0
LF bytes: 289
git ls-files --eol: i/lf w/lf
SHA256: 88305D5F476E1F13652D988357D82016DBF90A7C963C46EE0A4062A8AC13DF28
```

Two disposable copies were created only under ignored `build-artifacts/test-runner-audit/`. Both copies rebase ROOT to the actual repository so their different directory does not affect path resolution. Their commands are otherwise identical; one uses LF and the other CRLF.

| Diagnostic copy | Invocation | Result |
| --- | --- | --- |
| LF | `cmd.exe /d /c build-artifacts\test-runner-audit\test-lf-probe.cmd` | Inventory rejects N06; candidate-hardening compile is reached; exit 2 |
| CRLF | `cmd.exe /d /c build-artifacts\test-runner-audit\test-crlf-probe.cmd` | Same inventory rejection; no compiler reached; exit 1 |

This isolates the current file's failure-control-flow dependency on physical line endings. It does not establish a universal claim about every LF batch file or identify the command processor's internal parsing algorithm.

The actual repository script, its commands and its index entry were not modified. No diagnostic copy was promoted to the supported runner.

## N06 harness registration decision

The requirement comes from both current project policy and the existing audit:

- `docs/ARCHITECTURE.md`, Build and test policy: `test.cmd` is the single repository test entry point and builds/runs all current Windows harnesses.
- `tests/runner/test_cmd_audit.ps1:37–40`: recursively enumerates every `*_harness.cpp` under `tests/`; no N06/research exemption exists.
- `tests/plugin/n06_m01_zoom_game_derived_harness.cpp`: ordinary executable harness source in that inventory.
- `research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1:67–88`: separate build and four executions already exist, but the canonical audit does not parse this tool.

**Recommendation: include the existing N06-M01 offline harness in the canonical test registry.** Do not exempt it simply to silence the failure.

Integration requires attention to its real interface:

```cmd
build-artifacts\tests\n06_m01_zoom_game_derived_harness.exe zoom-in off
build-artifacts\tests\n06_m01_zoom_game_derived_harness.exe zoom-in on
build-artifacts\tests\n06_m01_zoom_game_derived_harness.exe zoom-out off
build-artifacts\tests\n06_m01_zoom_game_derived_harness.exe zoom-out on
```

Each execution needs its own immediate `if errorlevel 1 goto :fail`. Bare no-argument execution fails because `main` requires exactly three argc entries.

Current audit patterns at lines 14 and 26 only recognize bare executable lines. Supporting argument-bearing lines is a necessary part of T01, including applying the immediate error-check rule to those lines. Unique executable counts remain appropriate: four N06 invocations represent one compiled harness.

The compile should reuse the canonical actual-adapter flags, dependency response file, owner-binding probe and resource inputs. This source includes the actual adapter translation unit with a renamed original main; it must not separately compile that main/translation unit a second time.

Do not invoke the whole historical provenance script from `test.cmd`: it requires a pinned installed game EXE, fixed historical fixture hashes and previously generated executables. Those dependencies are unsuitable for the general repository suite.

The canonical test would replay the checked-in game-derived byte fixtures offline. The separate retained tool would still verify their association with an actual game image. Adding offline registration does not close N06 or any other native verification gate.

## Relationship to source moves

| Concern | Evidence | Conclusion |
| --- | --- | --- |
| N06 inventory mismatch | N06 source remains under unchanged `tests/plugin/`; earlier final-test log contains the same unregistered harness | Independent of the four D/L file moves |
| COMMON flags | COMMON and candidate-hardening command bodies are unchanged by D/L; actual failure occurs before moved classifier compilation | Flag content was not removed by a path edit |
| Physical batch line endings | Current file is LF-only; earlier log already exhibits unintended candidate compilation; no historical physical-byte snapshot identifies the first conversion | Confirmed current runner defect; exact conversion provenance unresolved |
| Locale header | `locale_registry.hpp` is found and parsed; first error is unavailable `std::string_view`, not missing resource ID header | Include resolution is not this error's root cause |
| Diagnostics classifier | Current unstaged runner diff changes only its source path to `src/diagnostics/language_state/language_state_transition.cpp` | Path update is the intended D move |
| Resource header | Current `src/localization/localization_resource_ids.h` retains SHA-256 `EC860DF4C3EF0009F17F7493AA9B2A9368CFF929F872F25D49AE74BA84728213` | No ID/body change |
| Generated headers | Failure bypasses ordinary generation/INCLUDE setup | No independent generated-header defect established by this run |

The earlier placement save failure was a separate temporary-directory observation documented in the source reorganization report. It is not the first failure of this runner invocation and was not reclassified by this audit.

## Git index and worktree

All investigative Git commands use read-only operations. Subsequent status/diff commands use `--no-optional-locks`.

### Current index evidence

```text
Index length: 154685 bytes
Index creation: 2026-10-11T13:41:22.4341111+03:00
Index last write: 2026-10-11T13:41:22.5876791+03:00
Index SHA256 before/through audit:
D5C1F09472C29ED0D4738C9184AE007D58EA89C05C151E37F8C1964A2A54CEA8
Unmerged entries: 0
```

The pre-existing final-test log was created at 13:52:35 and finalized at 13:52:39; the reorganization implementation report was created at 13:55:40. Thus the observed current index file predates those final checks.

Current tracked-status categories, before writing this audit report:

| XY | Record count |
| --- | ---: |
| A. | 25 |
| AM | 3 |
| M. | 19 |
| MM | 3 |
| R. | 47 |
| RD | 1 |
| RM | 2 |
| .D | 3 |
| .M | 2 |

There were three untracked status records; directory records may contain several files. The new audit report adds one untracked file. These counts describe existing combined work, not changes performed by this audit.

Normalized blob comparison:

| Path | Index blob | Working content after Git normalization | Interpretation |
| --- | --- | --- | --- |
| `build.cmd` | `3a6dfe075e4d3bfb4223ecd12c70d2626fddfe36` | Same blob | Pre-existing staged content; no unstaged content delta |
| `src/plugin/runtime.cpp` | `ffb6a149b792169d10e3ec9d4a8b04804ca60706` | Same blob | Pre-existing staged content; no unstaged content delta |
| `test.cmd` | `c96782141821e2497fe6caeef585caca32325739` | `e95663cb9528161013806a686f81eb2918080729` | Both staged and unstaged work; unstaged text delta is the D classifier source path |

The earlier captured status had several of these paths as unstaged/untracked; the current index contains real staged additions/renames and updated blobs. `core.autocrlf=true` explains normalized LF blobs and LF→CRLF warnings, but cannot by itself stage new files or renames. An index-writing/staging action occurred between those historical observations. The responsible process/person and exact command are **unknown**: no earlier index snapshot or staging-operation trace is available, and ordinary staging is not a reflog event.

No stage, unstage, reset, clean, checkout, commit or index reconstruction was performed during this audit. The measured index hash remained identical.

### Diff tooling control

Exact failing command inside the sandbox:

```powershell
& 'C:\Program Files\Git\cmd\git.exe' --no-optional-locks diff --no-ext-diff --name-status
```

Observed: `fatal: this operation must be run in a work tree`, **exit 128**, despite `rev-parse --is-inside-work-tree` reporting true and cached diff working. Explicit `--git-dir=.git --work-tree=.` did not resolve it inside the sandbox.

Controls:

| Git/context | Worktree diff |
| --- | --- |
| Installed Git 2.56.0.windows.2, sandbox | Exit 128 |
| Same installed Git, outside sandbox | Exit 0 |
| Bundled Git 2.53.0.windows.3, sandbox | Exit 0 |

Bundled executable:

`C:\Users\enton\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\git\cmd\git.exe`

The repository is non-bare, cached diff works, and the bundled binary reads the unstaged delta. There is no evidence requiring repository/index repair. The exact lower-level reason this installed Git fails within the sandbox is not established.

## Minimal remediation plan for approval

1. **Restore batch failure behavior first.** Convert physical `test.cmd` to CRLF and add the narrowly scoped Git attribute. Confirm the unchanged inventory failure returns 1 and starts no compiler. This separates the original failure from downstream diagnostics.
2. **Integrate the existing N06-M01 offline fixture.** Add one compile using current canonical dependencies and four argument-bearing executions. Adjust only run-line recognition/immediate-check detection in `test_cmd_audit.ps1`; retain full repository inventory enforcement. Preserve the standalone provenance tool.
3. **Validate the runner changes.** Check CRLF bytes, normalized text diff, registry equality (60 unique harnesses), positive recognition of all four executions and failure detection when a required execution/check is absent in disposable audit copies. Capture a real initialized compiler invocation showing COMMON flags and verify `_MSVC_LANG`. Run the complete supported `test.cmd` and retain its native exit code/raw output. Any newly exposed failure gets its own diagnosis.
4. **Review with working read-only Git tooling.** Use the bundled binary for diff/rename review. Compare index SHA-256 before/after and leave existing staged/unstaged state untouched. No Git mutation is part of the proposed fix.

Expected changed authoritative files if approved: `test.cmd`, `tests/runner/test_cmd_audit.ps1`, narrowly scoped `.gitattributes`. No camera/runtime/vendor source change is needed. No harness rewrite or new state/API is proposed.

## Audit execution limits and preservation

The original failing runner was reproduced, not repaired. Diagnostic commands created disposable probes/copies only under ignored `build-artifacts/test-runner-audit/`; syntax probes used `/Zs`. The failing batch never reached the ordinary header generator, resource compile or first intended adapter compilation. No full suite PASS is claimed, and no production build was run for this audit.

The report is the only new authoritative file. Source, supported scripts and Git index were not edited. Repository runner SHA-256 and index SHA-256 remained identical. N03–N06 statuses, runtime architecture, release artifacts and configuration were not changed. The game was not launched and no ASI was installed.

Remediation remains pending approval; stop after this report.

