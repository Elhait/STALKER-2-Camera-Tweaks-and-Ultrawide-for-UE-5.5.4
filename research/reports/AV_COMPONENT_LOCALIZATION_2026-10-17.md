# AV localization before architectural simplification

## Scope and result

The user requests active investigation and permits justified simplification.
This follow-up compares exact binary composition and the necessity of new
mechanisms. It does not roll back 3.2, disable protection, upload files or
weaken shutdown/input contracts. The Ulise cause is **not established**.

Prepared nine byte-exact raw section samples and a manifest under
`build-artifacts/av-section-evidence-20261017/`. These are **not ASI candidates**
and must never be installed or loaded. They preserve existing bytes rather
than producing alternative executable implementations to seek a lower score.

## Exact subjects

| Subject | ASI SHA-256 |
| --- | --- |
| v3.1.0 archive member | `ffb03f35d0c04a5a609fcb47b3a9024e2da268fa40bb431186903233498e545a` |
| Published-subject v3.2.0 archive member | `97696dba3c40067f4f89d8c8f62d2d93c5ad2a97707860492e723a4e2016f5d4` |
| Current user-built root ASI | `f68f40f9392d00911c89eaf93fee95c38c34ab5aac4a4c1dab6f639a2c9bac2a` |

The user's clean v3.1 observation concerns its ZIP. Do not silently turn that
into an independently measured clean verdict for its extracted ASI. This does
not invalidate the earlier release as a control; it limits attribution.

## Binary comparison

| File | .text bytes | .rdata bytes | .rsrc bytes |
| --- | ---: | ---: | ---: |
| v3.1 | 1,526,784 | 796,672 | 1,044,480 |
| Published v3.2 | 1,532,416 | 801,280 | 1,048,576 |
| Current | 1,517,568 | 800,256 | 1,048,576 |

The 11 font/license resources are identical between v3.1 and current. The
18 locale resources differ. This distinguishes concrete resource changes from
claims that new fonts necessarily caused detection. It does not exclude a
context-sensitive classifier operating on the complete PE.

The current binary no longer imports `ShellExecuteW`, `TerminateThread`,
`ResumeThread` or `LoadLibraryA`; Ulise remains in the user's current VT result.
Removing these capabilities was useful hardening, but was not sufficient to
remove this classification. Do not present them as the established cause.

D3D11/DComp, executable memory patching and VEH existed in v3.1. Their presence
alone does not explain a change between these releases. The new Overlay window
does add window management and thread-specific subclass installation imports.

## Necessity and alternatives

| Mechanism / source | Current purpose | Simplification assessment |
| --- | --- | --- |
| `module_residency.hpp::PinModuleForProcessLifetime`; runtime initialization | Keep code alive while resident callbacks/VEH can reference it | Removing PIN alone reopens callback lifetime risk. A balanced reference is not an equivalent unload guarantee. |
| `composition_runtime.cpp::InstallInputHook`, `InstallInputBindingOnGameThread`; `game_thread_subclass.hpp::InstallGameThreadSubclass` | One-shot WH_GETMESSAGE handoff to install WndProc on the actual window thread | The hook is thread-specific and removed after completion. Cross-thread direct replacement reopens predecessor publication/foreign-subclass races. An alternative requires a guaranteed existing callback on that HWND thread; none established here. |
| `safetyhook.cpp::TrapManager`, `trap_threads_result` | Route execution while executable permissions/patch bytes change | Replacing this with unconditional memcpy, thread termination or blind library substitution is not equivalent safety. The new shutdown probe avoids taking a potentially orphaned mutex during process termination. Removed only an unreachable destructor flag/branch; handler and patch behavior are unchanged. |
| `native_file.hpp`, `native_log_file.hpp`, `startup_journal.hpp` | Explicit Win32 ownership, bounded writes and no CRT stream teardown | Returning to global fstreams would reopen SL01. Retain the protections independently of AV classification. |
| ImGui upstream shell/file/gamepad helpers | No product dependency | Already compiled out by the previous hardening; no further architecture change needed for these capabilities. |

The game-thread handoff remains one-shot and is removed after installation. Its
callback runs on the HWND-owning thread, where it installs and publishes the
actual predecessor. Cross-thread direct replacement reopens the ordering race.
No duplicate Overlay owner or equivalent redundant teardown transition was
found.

The INI path is derived from the loaded module directory. Migration reads a
known legacy file only through its explicit migration path. Reads are bounded;
staging uses exclusive creation; writes are flushed and atomically replaced.
The focused harness covers hard-link refusal, preservation on failed staging or
replacement, migration and idempotent synchronization. No extra file operation
or safer equivalent path was established in this pass.

The unreachable destructor-state flag and branch have now been removed. That
small simplification does not eliminate VEH, executable patching or the
shutdown dependency, and is not presented as a solution to Ulise.

Moving the entire UI into another process/DLL or changing the hook engine would
be materially larger work, with new IPC/distribution/lifetime obligations.
Neither is justified by a positive whole-file verdict alone. They are not
claimed impossible; they require evidence identifying the contract to replace.

## Optional raw-section evidence

The raw sections are secondary evidence only. The primary work in this pass is
source-based engineering review and the 3.2.1 candidate. A scanner verdict on a
section cannot establish subsystem causality because the PE context is absent.
If a scanner reproduces the label locally, these samples can be used as an
optional byte-localization check, with the following limits:

A positive `.text` sample narrows machine-code bytes, not a specific function.
A positive `.rdata` sample narrows constants/metadata, not INI behavior.
A positive `.rsrc` sample directs review to data that can be matched to individual
resource hashes. A negative result clears no subsystem. No section scans have
been run.

The manifest preserves PE imports, resource hashes, offsets and RVAs for all
three subjects. It enables exact attribution without executing any mod code.

## Validation and safety

- Preparation ran in one externally supervised CPU-only process with a
  60-second timeout and 1 MiB output limit.
- Exit 0, no timeout or output truncation; process-tree cleanup verified.
- All nine output hashes verified against their source section bytes.
- Root ASI and both release ZIP hashes rechecked after preparation: unchanged.
- `git diff --check` passed, with existing line-ending warnings.
- Release 3.2.1 build passed in an isolated tree. ASI size 3,552,768 bytes;
  SHA-256 `A4D3E60231BF151E4A929312E9000F9C1F220DDE972F9DD99CF6A8DD93E1A24E`.
- Built PE VERSIONINFO verified as FileVersion `3.2.1.0`, ProductVersion `3.2.1`,
  CompanyName `Elhait`, FileDescription `STALKER 2 Camera Tweaks`.
- The configured build emitted a non-fatal CMake warning about long object paths
  for an unrelated registered test target; the production ASI linked successfully.
- No full test suite, game/GPU fixture, DLL load, system-setting change,
  installation, automatic AV scan or upload.
- Detector localization remains unknown; the 3.2.1 candidate has not been
  scanned. No new AV-safety claim is made.
