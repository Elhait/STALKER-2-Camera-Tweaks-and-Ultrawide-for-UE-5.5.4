# Comprehensive ASI security audit and hardening — 2026-10-17

## Executive assessment

**Implemented hardening: offline PASS. New binary: not installed and not live-qualified.**

No unexpected network client, child-process launcher, remote-process injection,
autostart/service/task persistence, privilege escalation, executable payload or
packer was identified in the inspected ordinary Windows production paths and
release contents. This is a bounded source/build/PE assessment, not a guarantee
that the binary is harmless under every possible condition or that AV detections
are false positives. The mechanism behind the Ulise labels remains **unknown**.

Seven source-backed safety/capability findings were addressed. They concern
file/staging ownership, bounded reads/logs, unnecessary ImGui capabilities,
instruction-cache synchronization, suspended-worker termination, failed VEH
registration, and background hotkey sampling. None is established as the cause
of the VirusTotal detections. Camera algorithms, graphics hooks, toast readiness,
TTL, menu layout, user INI, installed ASI and published ZIP were not changed.

**Coverage qualification:** all 61 registered production translation units and
their actual in-repository header dependencies are inventoried. The per-file
appendix distinguishes execution-boundary review from structural/contract review
and limited vendor-use review. It does **not** assert an exhaustive independent
implementation audit of every vendor algorithm, template or every branch in the
large runtime/transaction modules. Therefore this report must not be described
as an unrestricted, fully independent line-by-line clearance of the entire
source tree. The hardening/qualification batch is complete; that stronger audit
claim and mathematical absence-of-defects claims are not established.

## 1. Scope, subjects and evidence identity

Baseline HEAD: `2644b0e06ed71f39b5f03e93dc98a81692da270b` (clean at start).
Changes in this report are an uncommitted working-tree delta from that commit.
The supplied pasted task explicitly authorized production safety repairs,
bounded CPU/NonGraphics tests, an isolated Release build, and this report.

| Subject | Bytes | SHA-256 |
| --- | ---: | --- |
| Local v3.1.0 ZIP | 1,444,757 | `5045B7B33C6492CE4FF1D316F47358A907B5DB9F932509199203EF68815516C2` |
| ASI inside v3.1.0 ZIP | 3,555,840 | `FFB03F35D0C04A5A609FCB47B3A9024E2DA268FA40BB431186903233498E545A` |
| Local v3.2.0 ZIP / supplied VT subject | 1,448,275 | `7085EE28F93056E4C748904256A7C53D45CE5CE00D86617B162ABF1D47928EF1` |
| Original ASI inside v3.2.0 ZIP | 3,568,128 | `97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4` |
| New isolated hardening ASI | 3,551,744 | `86C234BBB04B3597EA77F7F16413A99DC7B025CAD960AE17D44F1C23104248DD` |

The supplied [VirusTotal URL](https://www.virustotal.com/gui/file/7085ee28f93056e4c748904256a7c53d45ce5ce00d86617b162abf1d47928ef1)
identifies the **ZIP, not the ASI**. VT's Relations view names the contained
`97696D…` ASI. Its bytes match the local root ASI, release-assets ASI and existing
ordinary Release output. A differently hashed stale candidate directory was
excluded from the baseline.

Nexus quarantined the upload and blocks downloading it, as the user confirmed.
No independent Nexus-server download was available or required as a workaround.
The evidence subject is the local ZIP with the exact user-supplied VT hash; an
independent assertion about Nexus's current server-side bytes is not made.

v3.2.0 ZIP entries, exactly:

| Name | Bytes | SHA-256 |
| --- | ---: | --- |
| STALKER2CameraTweaks.asi | 3,568,128 | `97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4` |
| README.txt | 1,847 | `96D02EFDD2C4E5954F5BB5F234F499D97A86098D2B344F781E7958A23A716EE3` |
| LICENSE.md | 7,669 | `13FD6AD21B410F6E2BD2858DB49F7719D6DB6DBAF6C2682C6C8ABBF01BE4B04E` |
| THIRD_PARTY_NOTICES.md | 8,118 | `8066CF765FC332AA8FDBE32A24D89BFBD266F09329945AF6E1C5BD51483F816F` |

No INI, encrypted entry, extra executable, installer or script is in this ZIP.
The old v3.1.0 ZIP additionally contains its historical INI; it was not repackaged.
All original artifacts remain unchanged. The new output is separate:

`build-artifacts/security-hardening/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

## 2. Threat model and trust boundaries

- The ASI runs with the game's existing user token. It neither requests elevation
  nor changes system security policy. Running the host elevated unnecessarily
  widens the consequences of any host/mod bug; normal user execution is preferred.
- Code, bundled resources and compile-time signatures are trusted deployment
  inputs. Configuration is user-editable data, not script or executable input.
  INI selections are typed/validated; they cannot name a DLL, executable or output
  directory. Localization/font input is embedded, not an arbitrary user font path.
- Game pointers/register values are validated according to the supported native
  boundary. A readable/writable region is not proof of a game object's continuing
  lifetime. Correct ABI, game-thread ownership and validated image topology remain
  important assumptions, particularly with other injectors.
- This is not a sandbox against malicious code already executing in the game or
  a directory owner able to replace the ASI and its parents. Parent junctions are
  supported. The file fixes prevent the demonstrated final-name/staging clobber;
  they do not claim race-free security against hostile directory administration.
- Dynamic unload is unsupported. Module pinning happens on the asynchronous
  initialization worker before hooks/presenter startup. Unload racing that initial
  handoff is outside the contract. Normal process termination is not an owner-thread
  graphics cleanup guarantee.

## 3. Production inventory and coverage method

`tools/build/production_sources.json` registers **61 TUs: 54 project + 7 vendor**.
The compiler dependency database for the isolated Release identifies 140
in-repository dependencies. Their union with the source registry and first-party
header/resource declarations is **203 files: 151 src files, 51 vendor files and
one generated INI-documentation header**. This is a superset: the extra
language-state-transition research header is explicitly NOT-PRODUCTION.
It is not attributed to the Release. See the [per-file appendix](COMPREHENSIVE_ASI_SECURITY_FILE_COVERAGE_2026-10-17.md)
and [hashed inventory](../evidence/security-hardening-2026-10-17/source-inventory.json).

The 151 src entries comprise 54 TUs, 96 headers (one research-only), and the resource script. The
startup-timeline header appears as a compiler dependency but its forensic body
is compiled out in this production profile. Language-state watcher TUs and
research executables are not production TUs. Their presence in the repository
does not make their APIs part of the ASI.

Review followed owners and sensitive operations across module boundaries, not
only the version diff. Source searches were cross-checked against real imports,
resource hashes, compilation inputs, existing failure harnesses and changed
control paths. A search hit in a vendor header does not by itself mean linked or
reachable behavior: for example, fmt's unused `os.h` is not a Release dependency.

### Reviewed subsystem boundaries

| Area | Source path / owner | Review result and qualification |
| --- | --- | --- |
| Entry/residency | plugin/dll_entry.cpp, runtime.cpp, module_residency.hpp | Async initialization, no detach join, pin-before-hook; unsupported pre-pin unload race remains explicit |
| Core callbacks/settings | runtime.cpp, camera_writer_transaction.hpp, lifecycle transaction headers, runtime_settings.hpp | Admission/drain, correction serialization, typed mutation/readback/persistence traced; relies on documented game-thread callback contracts |
| Camera domains | camera/, gameplay/, cinematics/, dialogue/ | Provenance/finite-value guards and output/store boundaries reviewed; no camera formula changed; regression is not adversarial game-memory proof |
| Hook resolution | hooks/, gameplay_camera.cpp, cinematic_aspect.cpp, runtime resolvers | Unique-match and relevant instruction/operand checks; loaded-image trust is not a general hostile PE parser |
| Native patching | SafetyHook + Zydis | Current-process allocations/patches, retained continuation storage, VEH and failure handling reviewed; SH04/SH06 repaired |
| File/config | NativeFile, config_repository, config_template, placement_config | Known paths, non-destructive staging, migration/backups/no-op persistence; SH01/SH02 repaired |
| Overlay/presenter | composition_runtime, renderer_runtime, imgui_d3d11_renderer | Private resources/owner, target on OverlayHwnd only, generation commits, fail-open admission/finalizer reviewed |
| Input | input_state, game_thread_subclass, composition callbacks | Own-process/thread hook scope, bounded queues, stable records, stale-generation forwarding, keyboard/mouse transfer reviewed |
| Locale/font/UI | locale registry/catalog/formatter/validator/font, presentation modules | Embedded data and typed keys; bounded game-language read and publication; no arbitrary format execution or font-path loading found |
| Diagnostics | diagnostic_runtime, telemetry, journal, NativeLogSink/NativeLogFile | No transmission, bounded storage, observational mode; raw addresses may appear in local diagnostics; SH02 cap tested |
| Build/package/tests | CMake, build.cmd, tools/build, canonical runner/supervisor | Explicit sources/resources, ordinary flags, bounded executed tests, allowlisted ZIP; provenance limitations below |

## 4. Security-sensitive API inventory

| Mechanism / source | Purpose and target | Restriction / failure behavior | Classification |
| --- | --- | --- | --- |
| DllMain / CreateThread | Transfer initialization off loader lock | No waits or COM/file teardown in detach; worker exceptions contained | EXPECTED MOD BEHAVIOR |
| GetModuleHandleEx(FROM_ADDRESS, PIN) | Keep this ASI's callbacks resident | Existing module only; refusal aborts initialization | EXPECTED MOD BEHAVIOR |
| VirtualQuery / bounded memcpy | Read/write validated game-camera fields | Current process, expected offsets/size/protection; native lifetime assumptions remain | EXPECTED MOD BEHAVIOR |
| Signature scan / Zydis | Locate and validate native camera instructions | Authored signatures, unique matches, bounded decode and call targets; invalid resolution refuses feature | EXPECTED MOD BEHAVIOR |
| VirtualAlloc/VirtualProtect/VirtualFree | Trampolines, stubs, local patch protection | Current process only; executable pools intentionally retained after publication; no remote-process handle | EXPECTED MOD BEHAVIOR |
| AddVectoredExceptionHandler | Route execute faults during local patch transitions | Check registration before transition; cached shutdown probe before trap lock | EXPECTED MOD BEHAVIOR, SH06 fixed |
| FlushInstructionCache | Synchronize generated/modified code | Self-process and exact ranges, errors propagate and uncertainty is retained | EXPECTED MOD BEHAVIOR, SH04 fixed |
| GetProcAddress / GetModuleHandle | Optional DComp clock and Windows shutdown probe | Already loaded named system DLLs, constant symbols; no user-supplied library/function name | EXPECTED MOD BEHAVIOR |
| SetWinEventHook | Discover/follow host HWND | Own process ID, out-of-context; self-window exclusion | EXPECTED MOD BEHAVIOR |
| SetWindowsHookEx(WH_GETMESSAGE) | Install subclass on game HWND thread | Own process/thread validated; not a desktop-global hook | EXPECTED MOD BEHAVIOR |
| SetWindowLongPtr/CallWindowProc | Nonblocking input bridge and native forwarding | Game-thread install, resident predecessor/generation, fail-open on terminal/stale owner | EXPECTED MOD BEHAVIOR |
| CreateWindowEx/SetWindowPos/ShowWindow | Private non-activating display | Single unowned Overlay HWND, always mouse-transparent; owner controls lifecycle | EXPECTED MOD BEHAVIOR |
| D3D11CreateDevice/DCompositionCreateDevice2 | Render own ImGui surface | No game swapchain/device/backbuffer acquisition or Present hook; HRESULT failure local to Overlay | EXPECTED MOD BEHAVIOR |
| D3DCompile | Build private UI shaders | Constant embedded shader text, no external source/includes | EXPECTED MOD BEHAVIOR |
| ReadProcessMemory(GetCurrentProcess) | Copy validated game language/FString | Validated image/prologues, <=64 wchar data, guarded game calls, English fallback | EXPECTED MOD BEHAVIOR |
| GetAsyncKeyState | Four configured action keys / rebind modifiers | Worker now enabled+foreground gated; WndProc modifiers only on own input messages | EXPECTED MOD BEHAVIOR, SH07 fixed |
| Clipboard/IMM32 | ImGui text/IME support | Normal in-process UI capability; no background clipboard collector or clipboard persistence found | EXPECTED MOD BEHAVIOR |
| NativeFile/CreateFile/WriteFile | Main INI and local logs | Fixed module-directory names, checked handle, size budgets, no handle inheritance requested | EXPECTED MOD BEHAVIOR, SH01/SH02 fixed |
| CopyFileW | Backup malformed/unrecognized INI | Local derived backup name, fail-if-exists, bounded 100 collision attempts; failed backup prevents recovery overwrite | EXPECTED MOD BEHAVIOR |
| MoveFileEx/DeleteFile | Commit/remove owned staged config | Exclusive staging, flush/close then replace; never delete a name after failed exclusive creation | EXPECTED MOD BEHAVIOR |
| BCrypt | SHA-256 artifact/game identity | Read-only file input, scoped provider/hash, backing storage lifetime maintained | EXPECTED MOD BEHAVIOR |
| ProcessLifetime | Avoid unsafe CRT owner destruction | Explicit owner cleanup remains possible; intentional process retention, not autostart/persistence | EXPECTED MOD BEHAVIOR |
| CRT LoadLibraryEx/TerminateProcess/ExitProcess/IsDebuggerPresent imports | /MT runtime/platform support | Import names alone are not custom anti-analysis, spawning or process-injection evidence | EXPECTED RUNTIME DEPENDENCY |

No production use of network APIs, OpenProcess/WriteProcessMemory, CreateProcess,
scheduled tasks/services, Run keys or security-policy mutation was identified.
Those APIs can exist in research/test tools; they were not conflated with ASI
runtime behavior. No claim is made that import inspection catches all possible
dynamically resolved behavior; constant symbol-resolution callsites were reviewed too.

## 5. Findings and implemented repairs

Confidence is in the described source mechanism, not attribution to an AV label.

| Finding | Severity / classification | Source / binary evidence | Security impact | AV relevance | Disposition |
| --- | --- | --- | --- | --- | --- |
| SH01 file/staging ownership | High conditional file-integrity risk; CONFIRMED SECURITY ISSUE | NativeFile::Open previously truncated before validation; both config writers reused `.tmp`; template removed it on open failure | Linked/stale staging could overwrite an unrelated accessible file or remove someone else's staging name | No causal AV evidence | FIXED for demonstrated final-name/staging paths |
| SH02 unbounded IO | Medium; CONFIRMED SAFETY DEFECT | ReadAll lacked input budget; both log writers lacked session budget | Oversized config allocation and unlimited diagnostic disk output | No causal AV evidence | FIXED with 4 MiB read / 16 MiB log budgets |
| SH03 unnecessary ImGui capabilities | Low; EXPECTED MOD BEHAVIOR, unnecessary capabilities | Default shell helper and XInput search shipped despite no product links/gamepad nav | Avoidable shell dispatch/DLL search/file helper surface, not demonstrated exploitation | Removed imports verified; no detection-causality claim | HARDENED through upstream compile switches |
| SH04 code-publication cache contract | High safety risk; CONFIRMED CONTRACT DEFECT | SafetyHook setup/enable/disable had no instruction cache flush | Unsupported generated-code publication assumptions; stale execution possible | No proof this caused hook failure or Ulise | FIXED with checked current-process flushes |
| SH05 suspended-worker termination | Medium; unnecessary unsafe operation | WorkerLifecycle::Start used CREATE_SUSPENDED + ResumeThread + TerminateThread failure branch | Forced thread termination unnecessary under serialized owner contract | TerminateThread/ResumeThread imports removed; no AV causality | REMOVED; ordinary worker creation retained |
| SH06 failed VEH registration | High safety risk; CONFIRMED FAILURE-PATH DEFECT | TrapManager ignored AddVectoredExceptionHandler failure before removing execute permission | Execute fault could occur without the required trap handler | No causal AV evidence | FIXED: refuse before VirtualProtect; retry allowed |
| SH07 background key sampling | Low; CONFIRMED SCOPE DEFECT | HotkeyLoop sampled/dispatched configured keys while disabled/background | Background input observation and unexpected setting changes outside game focus | Not evidence of a keylogger or AV cause | FIXED enabled+foreground sampling, consume held keys on return |
| SH08 local release tag mismatch | Medium; BUILD / SUPPLY CHAIN ISSUE | v3.1.0 and v3.2.0 both resolve to e756442; release HEAD is 2644b0e | Misleading checkout/rebuild provenance | Cannot explain AV by itself | OPEN metadata correction decision; no tag changed |
| SH09 vendor provenance / independent depth | Low / evidence gap; SUSPICIOUS / REQUIRES INVESTIGATION only as provenance gap, not maliciousness | Exact upstream SafetyHook revision/amalgamation provenance not established; vendor implementation review is bounded | Cannot independently attest every vendored line to a pinned upstream snapshot | Cannot establish compromise or AV cause | OPEN evidence limitation, not silently invented version |

### SH01 — exact repair and evidence

Locations: `src/platform/win32/native_file.hpp:30` CreateExclusive, `:36`
OpenFile; `src/config/config_repository.cpp` ReplaceConfig;
`src/config/config_template.cpp` SynchronizeManagedConfigTemplateBody;
`src/diagnostics/startup_journal.hpp` OpenFile/Write.

Regular writes now OPEN_ALWAYS without initial truncation, open the final
component itself, require a regular disk file with no final reparse/directory
attribute and exactly one hard link, then truncate the validated handle. Readers
also refuse final-component reparse points. A writer excludes simultaneous
writers/deletion while its handle is open. Both staging writers use CREATE_NEW.
An occupied `.tmp` name is neither overwritten nor removed on refusal. The
startup journal uses the same process-resident NativeFile primitive.

Regression: real hard-linked staging and log target, occupied normal staging,
both persistence and template-sync paths, unchanged victim bytes, existing INI
preservation, normal atomic persistence and no-op file identity all passed.
The sandbox refused hard-link creation with access denied during the first
attempt; that attempt was **not** counted as PASS. The supervised run with the
needed filesystem access then exercised real hard links successfully.

Residuals: parent junctions and close-to-rename attacks by a hostile directory
controller are not excluded. Symbolic-link refusal is source-proven but no
separate privilege-dependent symlink fixture was run. A stale occupied `.tmp`
now safely blocks persistence and needs deliberate operator recovery; the mod
does not delete unknown data to recover availability. User error notifications
and last-known-good INI preservation remain important.

### SH02 — budgets, partial failure and limits

Locations: NativeFile::ReadAll (`native_file.hpp:98`), NativeLogFile::Write
(`native_log_file.hpp:23`), NativeLogSink::sink_it_, composition WriteLog.

ReadAll checks GetFileSizeEx and cumulative bytes against 4 MiB; on failure the
caller-provided text is unchanged. Runtime and Overlay files keep complete
records within a 16 MiB session budget with a reserved limit marker. The startup
journal remains bounded by its independent 48-event storage.

Final review caught and repaired a bug in the initial cap implementation: a
failed WriteFile sequence may have already written a prefix. After the first
failed record **or failed limit marker**, the log now suppresses further writes.
It reports the original failure but does not retry with an underestimated
counter. The test injects a real eight-byte prefix followed by failure, then
verifies 100 subsequent calls produce zero further OS writes for both cases.

Regression: 4 MiB + 1 config refused without identity/timestamp changes;
256-byte test log has one limit marker and bounded bytes; partial-record and
partial-marker refusal passed; four concurrent logger writers retain all 512
normal records; reopening resets a session. NativeFile fault redirection is a
test-only preprocessing seam, absent from the Release compile definitions.

The cap bounds file output, not all formatting allocations or cumulative disk
usage across arbitrarily many separate game sessions/backup files. Power-loss
durability is not promised by normal log writes. Suppression intentionally loses
later diagnostics after the budget or first write error.

### SH03 — upstream capability switches

Location: `CMakeLists.txt:31`. All production and test targets consistently set
IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS, IMGUI_DISABLE_FILE_FUNCTIONS and
IMGUI_IMPL_WIN32_DISABLE_GAMEPAD. No library fork or icon/font change was needed.
Clipboard/IME remain; the product's own INI/log transport remains enabled.

The localization harness requires all three switches and calls the disabled
shell/file stubs only when compile guards prove they are stubs. It tests
refusal, not an actual shell launch. Final PE has no SHELL32 import and no
LoadLibraryA/XInput search. Unused ImGui DPI helpers remain vendor source but
their bare-name loads are not present in this linked artifact. Do not change
the game process's global DLL search policy to compensate for unused code.

### SH04 / SH06 — native publication and refusal

Locations: `external/safetyhook/safetyhook.cpp:542` complete trampoline flush,
`:989` stub flush, `:1288` Windows helper, `:1448` ensure_handler,
`:1630` pre-protection VEH check, `:1657` patch/restoration flushes;
InlineHook::enable/disable.

Generated storage is flushed before publication. Both relevant ranges are
flushed while patch routing is active, before restoring protection. Failure
returns an error and keeps uncertain executable ownership rather than treating
the operation as a clean success. Byte/cache restoration is tracked separately
from a subsequent protection-restore error. The existing retained-storage policy
is preserved. VEH registration must succeed before execute protection changes;
later installation may retry. No new handler/graphics hook is added.

The native trampoline evidence checks real local execution/ABI/continuation,
setup/enable/disable flush counts, injected flush failures, retry restoration,
missing VEH with **zero VirtualProtect calls**, and later recovery in Diagnostics
OFF and ON. This changes a Windows correctness contract; the
[Windows API documentation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-flushinstructioncache)
requires instruction-cache synchronization for modified/generated code.

The first cumulative run caught a regression in distinguishing successful
byte/cache restoration from a later reported protection failure. The implementation
was corrected; the existing post-write-disable assertion was not weakened.
Final cumulative runs pass it. These tests do not prove that all real protection
failures can be recovered, nor isolate the process from another injector's
concurrent patching. Linux cache-clear code is not Windows runtime evidence.

### SH05 — worker publication without forced termination

Location: `src/plugin/worker_lifecycle.cpp:26` WorkerLifecycle::Start.
CreateThread no longer starts suspended. The worker consumes only the copied
request and the already initialized stop event; lifecycle control is externally
serialized and does not depend on publishing the returned handle before entry.
On creation failure the request is deleted. Successful handles remain owned for
the existing stop/join path. No TerminateThread, new manager or detach wait.

Regression: 64 immediately completing start/join cycles, normal concurrent
workers, partial startup, stop-before-start, and existing real helper-DLL exit/
storage tests passed. Those helper-DLL tests are local CPU-only load/exit tests,
not injection into another process or the game. StopAndJoin can still wait for
a stuck worker on the explicit control path; it is never newly invoked in DllMain.

### SH07 — foreground-bounded key observation

Locations: `src/plugin/runtime_settings.hpp:57` SampleRuntimeHotkeys;
`src/plugin/runtime.cpp:6238` HotkeyLoop.
Sampling is skipped entirely if hotkeys are disabled or the foreground window
does not belong to the game process. Returning/enabling consumes held keys until
release. The game-WndProc overlay toggle remains independent of these action
hotkeys. The new helper test instruments the real reader boundary: zero calls
for disabled/background, exactly the configured four keys when admitted.
The usual unavoidable focus-change race between checking foreground and reading
a key is not represented as a security boundary. No key history or external
application text is logged.

## 6. Dependency and supply-chain review

| Component | Identified source version | Used surface / evidence | Qualification |
| --- | --- | --- | --- |
| Dear ImGui | 1.91.9b | Core/draw/tables/widgets + Win32 input backend; embedded fonts only; unused capabilities compiled out | Trusted-data UI, not a hostile-input sandbox |
| stb inside ImGui | imstb_truetype 1.26 and bundled rectpack/textedit | Existing rasterization/text editing paths; font resource hashes match source | No independent proof against arbitrary malicious TTF; arbitrary external TTF is not accepted |
| spdlog | 1.17.0 | Direct synchronous unregistered logger and owned NativeLogSink; registry/thread-pool/file-helper backends not product owners | Reachable formatting/sink/OS helpers reviewed by use, not a fresh full upstream audit |
| bundled fmt | 12.1.0 | Trusted format strings and arguments in spdlog; no untrusted executable formatting/plugin loader | All formatter algorithms not independently re-proven; unused os.h shell example is not a production dependency |
| Zydis | 4.0.0 | Decode bounded instructions in validated game image | Formatter-hook advisory below not applicable to this version/use |
| SafetyHook amalgamation | Exact upstream revision unknown | Bundled cpp/hpp, local retained-lifetime integration and new checked publication/refusal changes | SH09 provenance gap; no invented upstream commit or blind dependency update |
| Embedded fonts | Versions/license identities in THIRD_PARTY_NOTICES | Noto Mono + Arabic/CJK subsets, module-owned resources | Source-to-resource hashes matched; upstream binary authenticity not independently reconstructed |

Local vendor changes versus v3.1.0 were limited to SafetyHook integration and its
LOCAL_CHANGES record before this task. This task adds the documented cache/VEH
checks; ImGui/spdlog/fmt/Zydis implementation files were not refreshed wholesale.

[ImGui's policy](https://github.com/ocornut/imgui/security) explicitly limits its
security model to trusted code/data. [spdlog's security page](https://github.com/gabime/spdlog/security)
lists no published advisory; that is not evidence of universal safety.
[Zydis GHSA-q42v-hv86-3m4g / CVE-2021-41253](https://github.com/zyantific/zydis/security/advisories/GHSA-q42v-hv86-3m4g)
concerns pre-3.2.1 Zycore string use inside formatter hooks. The bundled 4.0.0
decoder use is not that affected version/path. These checks are not a complete
SBOM/CVE-service clearance or proof of upstream authenticity.

## 7. Build and packaging provenance

- CMake 3.31, Ninja, Visual Studio 2022 17.14.28; toolchain directory
  MSVC 14.44.35207, compiler identification 19.44.35224.0.
- Ordinary profile: Release, production, startup timeline OFF, Dialogue discovery
  OFF; /MT, /O1, /EHsc, /W4, /utf-8, /std:c++latest. No packer, obfuscator,
  explicit /GL or /LTCG stage found. The Windows link emits /MANIFEST:NO.
- Explicit product libraries: user32, psapi, bcrypt, dxgi, d3d11, dcomp,
  d3dcompiler, plus normal compiler/CMake system dependencies. Import count may
  differ from the link list because unused symbols are discarded.
- Sources and resources are explicit registries, not directory globs accepting
  arbitrary new binaries. Build actions generate the canonical INI documentation,
  audit locale catalogs, compile resources and compile/link local code. No
  FetchContent/ExternalProject/download/prebuilt payload acquisition found.
- build.cmd normally invokes the candidate compiler and atomic publisher;
  publish_asi.ps1 uses a per-destination mutex, unique staging name and atomic
  replacement. This task deliberately called the compiler-only entry in an
  isolated build root, **not** the root publisher.
- package_release.ps1 writes CreateNew ZIP, four-name allowlist, no INI, verifies
  inventory and packed ASI hash; removes only its own failed archive. It was
  inspected but no replacement release ZIP was created here.
- The isolated path causes a CMake warning about an unrelated, unbuilt test
  target's potential 250-character object path. The selected production target
  built successfully. Existing Zydis C4201 anonymous-structure warnings remain;
  neither warning is hidden or claimed as an AV cause.
- v3.1.0 = e756442; local v3.2.0 also = e756442, but release preparation HEAD =
  2644b0e. SH08 needs deliberate release metadata correction/annotation; no tag,
  branch, remote or Nexus metadata was altered.
- Matching archived ASI to an existing local build output is byte-identity
  evidence, not independent reproducible-build attestation of the old source.
  PE timestamps/tool installation are not cryptographic supply-chain evidence.

## 8. PE / ASI inspection

The CPU-only parser reads bytes and ZIP entries; it never LoadLibrary's the ASI.
It has bounded input/entry/iteration sizes and ran under the process-tree
supervisor. The baseline was also inspected with the installed MSVC dumpbin.
Raw parsed evidence: [pe-inspection.json](../evidence/security-hardening-2026-10-17/pe-inspection.json).

| Property | Original v3.2.0 ASI | Final hardened ASI |
| --- | --- | --- |
| Machine / type | AMD64 PE32+ DLL | Same |
| Sections | .text, .rdata, .data, .pdata, .fptable, .rsrc, .reloc | Same seven names |
| Static writable+executable section | None | None |
| .text entropy | 6.336 | 6.332 |
| .rsrc entropy | 6.923 | 6.923, matches embedded font/catalog data |
| DllCharacteristics | 0x160: ASLR, high-entropy VA, NX | Same |
| Security cookie / load config | Present, size 320, guard_flags 0x100 | Present; not a claim of full CFG enablement |
| Exports / delay imports / appended overlay | None / none / zero | Same |
| Authenticode certificate | None | None |
| TLS callback RVAs | 0x13d98c, 0x13d688 | 0x13d3dc, 0x13d0d8 |
| Embedded resource matches | 29/29 current source inputs | 29/29 |
| Imported DLLs | 8, includes SHELL32 | 7, no SHELL32 |
| ShellExecuteW / TerminateThread / ResumeThread / LoadLibraryA | Present | Absent |
| FlushInstructionCache | Absent | Present |

The two TLS callbacks in the baseline disassembly have the MSVC dynamic TLS
initializer/destructor shape, consistent with the installed vcruntime tlsdyn.cpp
and tlsdtor.cpp and the source's thread_local use. They are not evidence of a
hidden payload. The new linked callback locations move but the runtime model is
unchanged. The new binary was not dynamically loaded to investigate them.

No packer-specific section/overlay pattern or unexpected embedded executable was
identified. Entropy alone neither proves packing nor rules out malicious code.
Dynamic RWX trampoline pools still exist intentionally at runtime; absence of a
static W+X section does not mean the mod never generates executable memory.

Selected embedded URLs/strings correspond to library/license/documentation
material. No additional network or child-process imports were introduced.
CRT-origin imports and runtime DLL resolution must be interpreted with their
callers, not classified as malware just by name.

## 9. VirusTotal observations and limits

Observed UI snapshot on 2026-10-17 (Europe/Kyiv): ZIP **8/68**, contained ASI
**9/71**. UI last-analysis times were 04:10:39 and 04:13:33 respectively. Counts
can change; no rescan or new sample upload was requested.

| Engine | ZIP label | ASI difference |
| --- | --- | --- |
| ALYac | Gen:Variant.Ulise.608560 | Same |
| Arcabit | Trojan.Ulise.D94930 | Same |
| BitDefender | Gen:Variant.Ulise.608560 | Same |
| CTX | Zip.unknown.ulise | Dll.unknown.ulise |
| Emsisoft | Gen:Variant.Ulise.608560 (B) | Same |
| eScan | Gen:Variant.Ulise.608560 | Same |
| GData | Gen:Variant.Ulise.608560 | Same |
| VIPRE | Gen:Variant.Ulise.608560 | Same |
| Cynet | Not one of the eight ZIP detections | Malicious (score: 100) |

Common naming is a grouping observation, not proof of identical engine internals
or independent consensus. Arcabit's D94930 is the hexadecimal representation of
608560; this does not reveal which binary feature matched. v3.1.0 AV results were
not established, so the audit does not claim the detections first appeared in
v3.2.0. VT Behavior was still incomplete; NOT FOUND fields are not a clean sandbox
verdict. No new hardened-ASI VT result exists.

Plausible heuristic overlap from local code generation, executable-page changes,
VEH, pinning, input subclassing and an unsigned native DLL is **only a hypothesis**.
No engine rule, vendor explanation, controlled attribution experiment or AV
root-cause proof was obtained. Changes here were justified by safety/capability
contracts, not by evasion, import hiding, signature scrambling or lowering scores.

## 10. Validation and containment

Executed through the repository Job Object process-tree supervisor, finite
timeouts, bounded output, hidden consoles and serial builds. No game, GPU/WARP
fixture, ETW/PresentMon session, driver/power/registry change or ASI installation.

| Check | Final result | Evidence / scope |
| --- | --- | --- |
| Focused filesystem/log/worker/localization/native tests | PASS | Focused run; subsequently covered by final canonical run |
| Canonical test.cmd -NonGraphics | PASS | 59 registered targets, 62 invocations, 208 fresh adapter processes, 4 Zoom invocations |
| Missing-VEH refusal / cache failures | PASS in OFF/ON | Actual adapter hook paths with injected failures and real local execution |
| Partial record + partial limit-marker writes | PASS | Real prefix then failure; 100 suppressed retries, exact resulting file sizes |
| Existing failed-disable post-write contract | PASS in OFF/ON | Original assertion retained; implementation repaired after initial failure |
| Locale/resource preflight | PASS | 18 locales, 216 keys, placeholders/UTF-8/resources; not Arabic shaping/visual proof |
| Isolated ordinary Release | PASS | Production timeline/discovery OFF; final SHA listed above |
| Static final PE/resources | PASS for described properties | No ASI execution; all 29 resource hashes match |
| git diff --check | PASS | CRLF checkout warnings are not whitespace errors |
| Supervisor cleanup | Verified | Final regression/build exit 0, no timeout/output cap, cleanup true |
| Game/visual/driver behavior | NOT RUN | No previous artifact's live result attributed to this SHA |
| AV root cause / future detection score | UNKNOWN | No vendor rule or fresh scan |

There were earlier failed attempts: sandbox denial of hard-link setup and a
cumulative native-disable assertion. Neither is hidden or counted as PASS. The
final NonGraphics suite was rerun after the additional partial-log-write repair,
and Release was rebuilt from that exact final source. NonGraphics omits one
graphics scenario in both Diagnostic modes (210 registry children minus two =
208). CPU font-atlas construction is not a WARP/GPU fixture.

Durable validation results and output are in
[validation.json](../evidence/security-hardening-2026-10-17/validation.json).
The remaining environment check is restricted to our owned process identities;
no unrelated game/compiler/app is terminated as incidental cleanup.

## 11. Cross-subsystem interactions and accepted limits

1. **Config → Runtime → UI:** typed values and canonical migration remain;
   failed staging means runtime mutation may be active but persistence failed.
   Existing notifications must not misreport success. No setting reset or INI
   marker/layout change was part of this task.
2. **Logging → exception boundaries:** NativeLogFile is owner-serialized (spdlog
   sink mutex / Overlay log mutex), no new worker, and fails once then suppresses.
   Startup storage stays process-resident and uses the same checked file owner.
   A log size cap is not a no-allocation or no-I/O-latency guarantee.
3. **Hook publication → lifecycle:** cache/VEH refusal reports errors through the
   existing component transaction; published allocations remain retained to
   protect native continuations. Full recovery from a true OS protection-restore
   failure or unrelated injector mutation is not promised by green fault tests.
4. **Input → owner:** Game WndProc queues/tries and forwards native events without
   waiting for graphics finalization. The display HWND remains click-through.
   The heartbeat detects stalled ownership on incoming callbacks, not through a
   new watchdog thread. These existing limitations were not disguised as fixes.
5. **Pinning/process exit → CRT:** existing ProcessLifetime owners avoid their
   own destructive CRT teardown; OS reclamation at ExitProcess is not a driver
   completion signal. No new shutdown callback or loader-lock join was introduced.
6. **Trusted resource → third-party parser:** resource hashes identify what was
   built, not an adversarial font/parser proof. Same-process malicious code or
   replaced source/SDK/compiler is not contained by this mod.
7. **UI compatibility:** mouse/keyboard, locales, clipboard and IME remain; product
   gamepad navigation was not enabled and default shell/file helpers were unused.
   The new artifact still needs separate acceptance before distribution.

## 12. Disposition and next actions

- **Implemented:** SH01–SH07, focused regressions, final NonGraphics regression,
  isolated Release and static final-artifact inspection. No established open
  Critical defect was identified in the reviewed paths; the specified high
  failure-path contracts SH01/SH04/SH06 have targeted fixes and offline evidence.
- **Not silently closed:** exact SafetyHook upstream provenance, local release-tag
  mismatch, independent source-to-original-binary attestation, deeper vendor
  implementation coverage, AV detection cause and live acceptance of the new SHA.
- **No automatic publication/install:** original ZIP/root/release-assets ASI are
  preserved. The user chooses whether to live-check/promote the new binary.
- **Manual AV/Nexus evidence packet:** supply original ZIP/ASI hashes, four-file
  inventory, source commit/delta, this report, native-operation purpose/scope,
  dependency notices and PE evidence. Request manual sample analysis and the
  reason for the label. Do not present a test PASS or this report as a vendor
  false-positive determination, and do not claim the new ASI already passed VT.
- **No automatic tag rewrite or dependency upgrade:** repair release metadata and
  pin upstream provenance as separate, evidence-backed maintenance decisions.
- **No universal security clearance:** the appendix states review depth per file.
  A broader independent audit would need to close its remaining implementation
  depth/provenance gaps; that work must not be substituted by more green runtime
  harnesses or by another name for the same AV hypothesis.

This is a review handoff, not approval to replace an installed binary, re-upload
a quarantined file, mark Nexus clean, or declare all future shutdown/graphics/
AV behavior safe. Verified completion logging remains subject to user closure
under the workspace task-lifecycle rule.
