# Production security audit — Wacatac behavioral overlap

Date: 2026-10-17. Branch: `safetyhook-r1-r3-hardening`; HEAD `2644b0e`
with existing staged/unstaged work. This is a fresh source and binary review;
earlier reports and green tests were treated as evidence, not acceptance of the
current implementation. No production code, build input or ASI was modified.
No rebuild, game, WinDbg, WARP, graphics fixture, native test or fresh AV scan
was run. Only this report, its evidence record, and related status notes were
written.

## Follow-up — WB01/WB02 disposition

The findings below describe the implementation inspected on the audit date;
they are not the current status. WB01 and WB02 were subsequently corrected in
the staged SafetyHook source. The focused retry-generation and pre-write route-
lifetime scenarios, canonical non-graphics suite, and isolated Release build
passed as recorded in [SafetyHook WB01/WB02 correction](SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md).
The two findings are therefore **CLOSED by offline evidence**, not by this
historical audit alone. Live game qualification remains separate, and neither
finding has been causally connected to Microsoft or Cynet classification.

## Result

No source/binary evidence of a downloader, credential stealer, remote-process
injection, Windows persistence, network exfiltration, or AV-disabling mechanism
was identified in the inspected production paths. Several mechanisms overlap
with capabilities also used by malware, but have traceable mod purposes and
current-process boundaries. This does not establish absence of every possible
malicious or vulnerable implementation.

Two concrete SafetyHook defects were found: WB01, a retry ticket that rejects a
later valid protection epoch; WB02, continuation lifetime/routing after a
pre-write failure. Both have specific source execution paths. Neither was
reproduced at runtime in this read-only audit, and neither is established as
the cause of either AV verdict. The prior WC01 test proves its single delayed
dispatch ordering, not these additional orders. R2/lifetime acceptance must be
reopened accordingly; WC02's reservation RAII fix remains intact.

Static evidence and identities:
[wacatac-behavioral-overlap-audit.json](../evidence/security-hardening-2026-10-17/wacatac-behavioral-overlap-audit.json).

## Mechanism matrix

Locations refer to the current working tree. A listed risk is not itself a
claim that the mechanism is malicious.

| Механізм | Місце в коді | Призначення | Потенційний ризик | Докази | Рішення |
| --- | --- | --- | --- | --- | --- |
| DLL attach / worker handoff | `src/plugin/dll_entry.cpp:5`; `src/plugin/runtime.cpp:6831` | Record bounded startup milestones and hand initialization to a worker | Loader-lock deadlock if complex initialization/waits run in DllMain | Attach records memory-only milestones, publishes module, creates a thread and closes its handle; initialization/flush work is in InitializeThread; no join in DllMain | Necessary; retain current handoff. Unsupported unload-before-worker-pin remains a documented boundary |
| CRT TLS callbacks | PE TLS directory; MSVC `vcruntime/tlsdyn.cpp`, `tlsdtor.cpp` | C++ per-thread initialization/destruction | TLS can conceal arbitrary entrypoints in other software; CRT teardown has process-exit limits | Exactly two callback entries in all three candidates; bounded disassembly of current callbacks matches CRT guard/initializer and destructor-list walks; no custom `.CRT$XL` callback registration in project source | Expected CRT behavior. Does not prove every C++ TLS destructor harmless; no new malicious callback identified |
| DLL pinning | `src/platform/win32/module_residency.hpp:10`; `runtime.cpp:6438` | Keep callbacks and retained continuations mapped for process lifetime | Unsupported hot-unload; resident resources | FROM_ADDRESS + PIN targets the already loaded ASI, before hook/Overlay startup; failure aborts initialization | Necessary lifetime guarantee; not startup persistence across process exit/reboot |
| Resident Runtime / log owners | `runtime.cpp:370`; `src/platform/process_lifetime.hpp`; `composition_runtime.cpp:61` | Avoid unsafe CRT destruction of shared state after worker termination | Intentional bounded retention; explicit unloading unsupported | Runtime and notification owner remain resident; Win32 log handles avoid CRT FILE teardown; identified finalizers execute outside DllMain | Retain; OS process reclamation is not proof of an orderly module-wide shutdown |
| Worker threads / stop coordination | `src/plugin/worker_lifecycle.cpp`; `runtime.cpp:758`; `composition_runtime.cpp:1696` | Hotkeys, window discovery, UI ownership and pacing | A stuck worker can delay a controlled join; callbacks may outlive owners | Entries catch C++ exceptions; stop event; camera admission/drain before reset; controlled joins outside loader lock; Overlay owns finalization | Necessary. Process exit does not guarantee workers finish or emit a final log |
| Signature scan / inline hooks | `src/hooks/signature_scanner.cpp`; `src/hooks/instruction_validator.cpp`; `runtime.cpp:6598`; `external/safetyhook/safetyhook.cpp:425` | Find and intercept authored game-camera sites | Wrong target, foreign ABI errors, unsafe instruction relocation | Internally authored patterns, executable spans, cardinality/shape checks, game-image targets; native callback admission and noexcept wrappers at `runtime.cpp:1447`; no external configurable code address | Necessary interception. No remote-process injection found; compatibility/runtime correctness is not established merely by a unique signature |
| Generated trampolines / MidHook stubs | `safetyhook.cpp:142`, `498`, `965` | Relocate original instructions and preserve callback continuation | Executable-memory lifetime, code publication, native ABI | Private RW allocation, final cache synchronization and RX publication; fixed vendor stub bytes, not downloaded code; retained published allocations | Keep R3; WB02's earlier VEH publication-boundary finding was subsequently corrected |
| Protection transaction / temporary RWX | `safetyhook.cpp:1799–1916` | Exclude execution during a patch and restore original rights | Partial patch, NX page left behind, writable code window | Per-page snapshot, serialized transaction, actual-state checks, bounded restore attempts; temporary RWX special case for ASI/VirtualProtect-related pages avoids disabling the executing patcher | Required patch operation. No permanent RWX trampoline allocation or RWX fallback found |
| VEH / delayed execute admission | `safetyhook.cpp:1438–1613` | Route/retry execute faults caused by temporary NX pages | Wrong exception ownership, stale route, rejection of a legitimate fault | At audit time, retry-ticket and completed-route paths had the WB01/WB02 defects described below; later targeted corrections are recorded in the follow-up report | Retain execute-only admission; WB01/WB02 are closed by subsequent targeted offline evidence |
| Fail-fast | `safetyhook.cpp:1792`; callers `1894`, `1906`, `1914` | Stop when code/protection safety cannot be restored | Terminates the game on a terminal protection/callback failure | One RaiseFailFastException call; bounded restore failure or callback throw reaches it; successful patch path does not | Technically justified fail-safe; AV attribution unknown, no import-removal proposal |
| WH_GETMESSAGE | `composition_runtime.cpp:1055`, `1091`, `1126` | Install subclass on the game HWND's own thread | Broad keyboard capture/injection if global or foreign-thread scoped | Process ID checked against GetCurrentProcessId; explicit windowThread argument; callback handles the installation message then calls next hook; unhook after installation | Necessary current-thread binding handoff; not a global keyboard hook |
| WndProc / WinEvent hooks | `src/overlay/input/game_thread_subclass.hpp:12`; `composition_runtime.cpp:908`, `1368` | Game input routing and host-window lifecycle | Lost input, foreign subclass chain corruption, stale callback | Game-thread subclass; saved actual predecessor; bounded callback admission; process-filtered WinEvents; retained foreign chain rather than forcefully rewriting it | Necessary; no cross-process hook target identified |
| Hotkey reads / raw input | `runtime.cpp:6219`; `composition_runtime.cpp:954` | User shortcuts and mouse routing while menu owns input | Keylogging-like API surface / blocked game input | Configured keys sampled when game is foreground; no general key history or credential collection path; input released on focus/terminal transitions | Necessary functionality; no keylogger/exfiltration path identified |
| Clipboard / IME | `external/imgui/imgui.cpp:15035`; Win32 backend | Standard UI editing/copy/paste/IME support | Reads/writes clipboard on UI request | Imports map to ImGui defaults; no project clipboard-monitor loop or network sender; shell/file helpers and XInput auto-load are disabled in production flags | Expected UI surface, not independently malicious. Removing it would require a deliberate UI-feature decision, not an AV workaround |
| DirectComposition / D3D11 / shader compile | `src/overlay/presenter/renderer_runtime.cpp:1025`, `1360`; `imgui_d3d11_renderer.cpp:159` | Render the optional settings UI to owned composition surfaces | Graphics resource/owner failure; runtime code-generation resemblance | Private hardware D3D11 device/surface; static embedded HLSL with no external include handler; no game swapchain/Present patch; draw/commit/attachment owned by presenter | Necessary Overlay implementation; no external script/code execution found |
| Game-language native calls | `src/overlay/localization/game_language_reader.cpp:61`, `113`, `153` | Read the game's Interface Language on its window thread | Wrong native address/layout or buffer ownership on unsupported game build | Named current game image, bounded PE/layout/prologue validation, fixed authored RVAs, self-process reads, bounded FString, native free, SEH fallback | Necessary for Auto locale; no user-provided function pointer, OS-locale scraping or settings write |
| INI and recovery backup | `src/config/config_repository.cpp:24`, `143`; `config_template.cpp:665`, `1243` | Preserve settings, migrate documentation and recover malformed INI | Truncation/link/path races or unbounded untrusted input | Fixed module-relative filename; 4 MiB read cap; exclusive staging; flush/close before MoveFileEx; fail-if-exists backup names; final-component reparse and writer hard-link checks | Keep. Parent-directory trust and pathname replacement are not protection against a hostile same-directory writer; no elevated cross-user attack path established |
| Runtime / Overlay / startup journals | `runtime.cpp:6444`; `composition_runtime.cpp:296`; `src/diagnostics/startup_journal.hpp` | Local support evidence | Disk growth, sensitive diagnostic values, teardown locks | Fixed filenames next to ASI; runtime/Overlay 16 MiB session budget; startup 48-event capacity; regular single-link writer validation | Expected local writes; no log-upload path. Logs may contain local paths/addresses useful for debugging |
| Process / network / persistence APIs | Production source registry, current PE imports, dynamic call sites | Self-process querying and OS facilities | Remote code execution, downloader, persistence or data transmission if present | No production child-process launch, OpenProcess/WriteProcessMemory/VirtualAllocEx/CreateRemoteThread, network client, registry-autostart/service/scheduler writer found. ReadProcessMemory uses GetCurrentProcess; global EnumWindows only filters HWNDs by current PID | No issue found in inspected paths. This is source/PE evidence, not a network-capture claim |
| CRT imports and dynamic OS helpers | PE KERNEL32 imports; installed MSVC `utility_desktop.cpp`, `winapi_downlevel.cpp` | Static CRT, exception handling and OS compatibility | API list can resemble anti-analysis/process manipulation | /MT verified; source explains IsDebuggerPresent, TerminateProcess, loader and exception-support imports; first-party optional symbol lookups use known ntdll/dcomp exports; no project debugger-based feature switch | Expected runtime support; API names alone do not establish anti-analysis |
| Build inputs / embedded resources | `CMakeLists.txt`; `tools/build/production_sources.json`; build database | Build the single ASI from vendored source and embedded assets | Supply-chain substitution or unexpected executable payload | 61 production commands match 61 registered TUs; SafetyHook/Zydis/ImGui vendored; spdlog header dependency; resources are catalogs/fonts/licenses/version; fixed-source build helpers; no download stage identified | No supply-chain anomaly established. Exact historical source-to-binary reproducibility is not proven by these records |

## Findings confirmed at audit time — subsequently corrected

### WB01 — High: retry suppression survives a successful execution epoch

Evidence: `safetyhook.cpp:1517–1538`, `1606–1610`. `DelayedRetryTicket` contains
only an address. The completed/RX branch writes that address, returns
CONTINUE_EXECUTION, and has no success-completion path that clears it.
The next same-address execute fault returns CONTINUE_SEARCH before inspecting
current trap state, allocation identity or generation. Generation is maintained
in TrapInfo but is not consulted by this ticket check.

Concrete order:

1. A thread faults at address A during transaction G1's NX interval. Dispatch is
   delayed until RX restoration; the VEH records ticket A and retries.
2. The retry executes successfully. No unrelated exception occurs to clear the
   thread-local ticket.
3. A later legitimate transaction G2 changes that page to NX. The same thread
   executes A and faults again.
4. The address-only ticket rejects G2's owned exception before G2 is examined.
   It can reach an unhandled AV instead of SafetyHook recovery.

This can occur across different hooks on the same executable page, not only
recreating one hook. It is a source-established interleaving, not a captured
game crash. The existing two-worker delayed test gives each worker one delayed
fault and then ends it; the synthetic immediate-repeat test also cannot
distinguish a later valid transaction from a failed immediate retry.

Minimal correction plan: resolve authoritative trap/phase/ownership before retry
suppression; scope retry state to the relevant transaction/page generation and
allocation lifetime. A new legitimate generation must not inherit an earlier
generation's suppression. Keep bounded refusal of a true repeated unresolved
fault and strict rejection of read/write/malformed exceptions. Do not use a
timer to infer successful execution.

Required tests: one persistent worker performs successful delayed continuation
in G1, then faults again at the same IP in G2 without any intervening exception;
both must continue correctly. Also cover a genuinely unresolved same-epoch
repeat, another hook on the same page, thread isolation, and OFF/ON diagnostics.

### WB02 — High: VEH continuation publication outlives pre-write failure ownership

Evidence: `find_trap()` at `1438–1448` searches all records without a phase or
generation filter. The active NX branch first validates the page from one
selected transaction, then calls that independent search at `1595–1597` to
select its redirection. Meanwhile `enable():698–738` retains the trampoline
only after `write_attempted`, which is set after *all* forward protection
transitions (`1901`). The pre-write failure branch (`1888–1896`) can restore
the page and leave a Completed record with write_attempted still false;
`destroy():784–795` can then free the trampoline.

Two related causal paths share this missing publication/lifetime boundary:

1. **Already dispatched continuation:** source page changes RX to RW/NX; a
   thread is redirected by the active VEH into the still-RX trampoline. The
   next page-protection call fails before changing the trampoline. Rollback
   succeeds before the patch callback runs. Destruction treats the trampoline
   as unreferenced and can free it while that redirected thread is still
   executing it or has its continuation saved there.
2. **Stale completed route:** the same pre-write failure leaves source A's
   completed A-to-trampoline mapping after the trampoline is freed. A later
   hook B on A's page creates a valid active NX transaction. A fault at A passes
   B's page validation, but the separate find_trap(A) can select completed A's
   stale mapping and redirect into freed or repurposed storage.

In the second path, validating the *faulting source page's* AllocationBase and
Type does not validate the lifetime of the independently selected destination.
AllocationBase/Type are also not unique reservation-incarnation identifiers:
Windows can reuse the same base/type. A stored generation number does not help
unless it is tied to the selected route and the memory owner's publication
contract.

Impact: native use-after-free/invalid execution under a concrete protection
failure and scheduling order; no live occurrence or exploit was established.
The normal success-path retained continuations and WC02's failed-allocation
RAII do not close this pre-write publication path. The current failed-create
test has no thread already redirected through VEH and labels no-byte-write
storage unreferenced; it does not establish that stronger premise.

Minimal correction plan: use only the authoritative active transaction for NX
redirection; completed records may support coherent retry after restoration,
but must not silently become another transaction's redirect route. Mark a
continuation as published before VEH can expose it and preserve its lifetime
across pre-write rollback. Either prove admitted execution has drained before
reclamation or retain exposed code for process lifetime, consistent with the
existing module contract. Do not clear all tombstones merely at RX restoration.

Required tests: use source code outside the patcher's image so the fixture
actually enters NX (the ASI-image special case uses temporary RWX); hold a
worker after active VEH redirection, inject a true refusal on the next forward
protection call, and verify its continuation remains valid through owner
destruction. Separately fault on old source A while B on the same page is
active and verify no completed A route is used. Exercise address reuse,
failed-create cleanup, repeated hooks and delayed post-RX admission together.

## Removable mechanisms and necessary overlap

No complete dangerous mechanism was confirmed removable with zero loss of the
currently required functionality or safety contract. Hooks, VEH, DLL pinning,
game-thread subclass handoff, private executable continuation storage and local
persistence each have a concrete owner and purpose. At audit time, WB01/WB02
called for admission/routing/lifetime corrections, not deletion of those
facilities; the corrections are recorded in the follow-up report above.

Executable allocation plus native patching plus VEH plus thread/input hooks is
a meaningful behavioral overlap with capabilities usable by malware. Here the
inspected destinations are authored callbacks and validated game sites in the
same process. No accompanying downloader, remote control, persistence,
credential collection or transmission path was found. Dormant vendor helpers
are not automatically production behavior: actual build definitions disable
ImGui shell/file helpers and XInput loading, and experimental startup timeline,
language-watcher and other diagnostic programs are outside this profile.

## Binary provenance and artifact identity

| Property | `5124…E654` | `CC40…67CB` | `492A…1298` |
| --- | --- | --- | --- |
| File bytes | 3,552,768 | 3,554,304 | 3,554,304 |
| Imports / DLLs | 211 / 7 | 212 / same 7 | 212 / same 7 |
| New import relative to baseline | — | RaiseFailFastException only | RaiseFailFastException only |
| TLS callback count | 2 | 2 | 2 |
| Section count | 7 | 7 | 7 |
| Writable+executable PE sections | None | None | None |
| Resource entries | 30 | Same paths and payload hashes | Same paths and payload hashes |
| Export, delay-import, certificate directories | Empty | Empty | Empty |
| Trailing file overlay | None | None | None |

The `.text` raw-size delta is +1,536 bytes after R1–R3; names and protection flags
remain `.text`, `.rdata`, `.data`, `.pdata`, `.fptable`, `.rsrc`, `.reloc`.
`.fptable` remains writable data, not executable. No packer/obfuscator structure
was identified. These observations do not establish all dynamic protections;
the source still deliberately has short RWX patch windows described above.

The production commands use MSVC 14.44.35207, x64, /O1, /MT, /EHsc and the
expected production/journal/UI definitions. No /GL or /LTCG appears in the
inspected production compile/link commands. SAFETYHOOK_TEST_VIRTUAL_PROTECT is
absent from the production object commands. CMake's broad default link-library
list is not the set of actual imported DLLs; the PE imports are authoritative
for retained static imports.

The baseline was read from the **preserved** release-candidate directory.
During this audit, repository-root STALKER2CameraTweaks.asi instead hashed to
`9AEB720A7E25B6DA5D4F9FA1B1FE244D1D413F28D34F78509D1F3612C8A75B70`.
It has the same code and resources as `492A…1298`; all differing bytes lie
inside COFF.TimeDateStamp (file offset 280) and the PE debug directory timestamp
(2188708). This is consistent with a relink. No claim is made about who built
it, or its AV verdict; a verdict for another hash does not transfer to it.
None of these ASIs was changed by this audit.

The evidence record identifies the audited current source with selected file
hashes and a manifest hash for the 61 registered translation units. This is
not a complete historical input snapshot or signed reproducible-build proof.
No new vendor component outside the local SafetyHook edits was found in the
diff against HEAD; current ImGui/Zydis/spdlog source remains vendored. This audit
does not claim to be an exhaustive CVE/upstream integrity review of those
libraries, the toolchain or OS-supplied graphics/CRT facilities.

## Microsoft and Cynet attribution

User-supplied screenshots show Microsoft clear for the earlier baseline scan,
then Wacatac.B!ml for CC40 and 492A; Cynet labels all three. Separate recorded
local custom scans reported no threats for all three with engine 1.1.26090.9
and signatures 1.461.74.0, under SxS Passive Mode, DisableRemediation and the
existing NeverSend sample policy. The reported Defender update did not change
the active versions during the recorded recheck. These are different scan
environments, not a matched VirusTotal engine/model experiment.

Microsoft's public naming documentation describes `!` suffixes as internal
indicators. The supplied label does not disclose the sample-specific feature,
rule, behavioral trace or exact classifier. This audit does not infer a
particular Trojan technique or a purely static classifier from `!ml`.

| Hypothesis | Supported fact | Remaining uncertainty / disposition |
| --- | --- | --- |
| R1–R3 changed classification inputs | Added fail-fast import plus changed allocator, emitted code and VEH | Changes occurred together; no feature attribution. Do not single out one API as the cause |
| WB01/WB02 caused Wacatac | The source defects were corrected and offline-tested after the audit | No evidence the classifier executes or recognizes either interleaving; no causal attribution is established |
| Different Microsoft engine/cloud/model/cache settings explain the discrepancy | Local and VT verdicts differ for identical hashes | Exact VT versions/configuration not available; local no-threat result does not adjudicate VT |
| Cynet has the same cause as Microsoft | Both flag hardened samples | Cynet already flagged baseline; same cause not established, numeric score is not a proven probability |
| A trojan payload was introduced by R1–R3 | No supporting payload/network/persistence evidence identified; resources unchanged | Neither behavior overlap nor the verdict alone establishes infection; absence in this bounded review is not mathematical proof of safety |

Relevant primary references:
[Microsoft malware naming](https://learn.microsoft.com/en-us/defender-xdr/malware-naming),
[execute AV fields](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record),
[fail-fast contract](https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-raisefailfastexception),
[custom scan / DisableRemediation semantics](https://learn.microsoft.com/en-us/defender-endpoint/command-line-arguments-microsoft-defender-antivirus).

## Original next-step plan — completed

1. Correct WB01's retry-epoch admission and add the persistent-worker/two-epoch
   regression; retain the existing single-epoch delayed-fault and strict AV
   rejection cases.
2. Correct WB02's authoritative route selection and continuation publication
   lifetime, with true pre-write OS failure and in-flight/reused-address tests.
   Keep R1 bounded restore/fail-fast, R3 RW-to-RX and WC02 RAII intact.
3. After each correction run its CPU-only native cases with diagnostics OFF/ON.
   Then one authorized canonical nongraphics regression and a separately
   identified Release artifact; preserve all current samples. No game/graphics
   work is implied by this audit.

At the time of this audit, no code or tests were changed or executed for these
findings. The later correction report records the implementation and new test
evidence. Historical results cited here remain limited to their tested
scenarios; closure relies on the later WB01/WB02-specific evidence, not on the
earlier 62/62 result alone. No general architecture rewrite, API hiding,
obfuscation, permanent RWX restoration or cosmetic AV-driven edit was made.

## Execution limits

Bounded standard-library PE parsing and static dumpbin disassembly completed
with verified process-tree cleanup; no DLL was loaded for analysis. Two first
dumpbin range attempts failed because the hexadecimal bounds lacked 0x prefixes;
corrected bounded commands succeeded. No analysis process remained. Source
review covers the production lifecycle and sensitive API/ownership boundaries,
the registered source/build inputs and embedded-resource identities; it does
not claim exhaustive verification of every camera equation or every byte of
the statically linked libraries.
