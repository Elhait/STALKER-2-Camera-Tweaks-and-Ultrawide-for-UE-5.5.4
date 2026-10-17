# Microsoft Wacatac / Cynet — targeted SafetyHook investigation

Date: 2026-10-17. Scope: read-only production-code and binary review of the R1–R3 change. No game, debugger, graphics fixture, ASI installation, new build or AV submission was performed. This report supersedes the earlier unconditional R2 completion assessment where it discusses delayed exception admission.

## Result

The two local ASIs match the SHA-256 identities supplied by the user. The new binary adds one import, `KERNEL32.dll!RaiseFailFastException`, but adds no DLL dependency, PE section, TLS callback or resource. No malicious capability was identified in the scoped diff. There is a concrete correctness defect in R2's admission of delayed execute exceptions and an inherited allocator exception-safety defect. Neither is established as the cause of Microsoft or Cynet detection.

The user screenshots establish `2/69` for the new hash: Microsoft `Trojan:Win32/Wacatac.B!ml` and Cynet `Malicious (score: 100)`. The previous screenshot established Cynet alone at `1/71`. These are different scans; engine/signature versions and the complete original scan JSON are unavailable. The VT page could not be independently read through the web tool. Thus the new Microsoft result is an observation, not proof that a particular R1–R3 change caused it.

## Artifact comparison

| Property | Preserved baseline | R1–R3 ASI |
| --- | --- | --- |
| SHA-256 | `5124A197A036BFF5C630CA0E48250C3520745D508CC7E66465071ED75BA8E654` | `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB` |
| Local path | `STALKER2CameraTweaks.asi` | `build-artifacts/security-hardening/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi` |
| Size | 3,552,768 bytes | 3,554,304 bytes (+1,536) |
| Architecture | AMD64 PE32+ DLL | Same |
| Sections | `.text`, `.rdata`, `.data`, `.pdata`, `.fptable`, `.rsrc`, `.reloc` | Same names and protection flags; `.text` raw size +1,536 bytes |
| Static W+X sections | None | None |
| Imports | 211 functions / 7 DLLs | 212 functions / same 7 DLLs |
| Import delta | — | Only `RaiseFailFastException` |
| TLS callbacks | RVA `13D3CC`, `13D0C8` | RVA `13D97C`, `13D678` |
| Resources | 29 RCDATA + 1 VERSIONINFO | All 30 payload hashes and resource paths identical |
| Export / delay-import / certificate directories | Empty | Empty |
| Trailing file overlay | 0 bytes | 0 bytes |
| DLL characteristics / load-config flags | `0x160` / guard flags `0x100` | Unchanged |

The imported DLLs remain USER32, bcrypt, d3d11, dcomp, D3DCOMPILER_47, KERNEL32 and IMM32. `.fptable` is writable but non-executable and existed in the baseline. No new third-party runtime DLL appears.

Both TLS routines were disassembled within bounded ranges. They retain the same instruction/control-flow structure and lengths (102 and 165 bytes), with relocated code/data references. Their attach guard/initializer iteration and detach destructor-list traversal match the installed MSVC `vcruntime/tlsdyn.cpp::__dyn_tls_init` and `tlsdtor.cpp::__dyn_tls_dtor` source. They are not new R1–R3 callbacks. Raw callback hashes differ because their relative references changed.

## Source and build consistency

The task diff against the preserved index is confined to SafetyHook implementation/header, its local-changes record, native tests and test registry wiring. The baseline source association relies on prior build records; the index is not a signed source provenance artifact.

The actual production compile command in the release build database uses MSVC 14.44.35207, x64, `/O1 /MT /std:c++latest /EHsc /W4 /utf-8`, `NDEBUG`, `OVERLAY_PRODUCTION`, and the startup journal. The production SafetyHook object does **not** define `SAFETYHOOK_TEST_VIRTUAL_PROTECT`; a separate test object does. The link is `/machine:x64 /INCREMENTAL:NO /MANIFEST:NO`, with no `/GL` or `/LTCG` in the inspected commands. The static CRT explains the embedded CRT support routines.

The build log records rebuilding SafetyHook and linking this target, with exit 0 and verified cleanup. Source/header modification times precede the object and final ASI. The SHA, resource equality and sole new import agree with that build and the source. This is consistent provenance evidence, not a cryptographically reproducible source-to-binary attestation; no rebuild was performed during this investigation.

## Confirmed problems

| ID / severity | Exact source and mechanism | Risk and minimal correction | Coverage / AV relevance |
| --- | --- | --- | --- |
| WC01 — High, R2 delayed-exception admission | `external/safetyhook/safetyhook.cpp:1484-1489` requires the page to be RW/WRITECOPY **when the VEH runs**. `restore_pages():1638-1654` can restore RX concurrently before `retire_trap():1733-1736`. A thread can fault during the owned NX interval, be delayed before the permission query, and then be rejected while the matching trap is still live because the current protection is RX. | An owned execute exception is returned to subsequent handlers and can become an unhandled AV. The bug is the use of current protection as proof of protection at fault time. Retain the execute/context/address checks; make admission aware of the transaction phase so a valid delayed fault after restoration safely retries executable code. Review retirement with pending/entered handlers rather than simply accepting every execute AV or dropping ownership checks. | Confirmed incorrect admission path by a concrete source interleaving; no live crash attributed to it. `TestVehAdmission():1523-1580` keeps its fixture RW throughout and cannot catch it. The RX-publication test toggles hooks sequentially. A deterministic barrier test must pause the real admission path before its query, restore RX while keeping the owned trap active, then verify successful continuation. Also cover dispatch around retirement. No established AV-classifier connection. |
| WC02 — Low, inherited allocation exception safety | `external/safetyhook/safetyhook.cpp:148-157` obtains a VirtualAlloc region before `m_memory.emplace_back(new Memory)`. Failure of `new Memory` leaks the region; failure growing the vector additionally loses the raw metadata pointer before unique_ptr ownership is constructed. | Under allocation failure, an unpublished RW reservation becomes unowned. Allocate the metadata owner before reserving memory, assign the region immediately to that RAII owner, and move it into the vector. A vector failure then releases the owned region. | The same ownership gap existed before R1–R3; private per-hook allocation now retains the pattern. No bad_alloc injection covers it. Validate failures at metadata and vector allocation. No evidence that this conditional leak explains either detection. |

WC01 reopens the narrow R2 admission/retirement contract. It does not justify reverting R3's RX allocations or removing VEH.

## R1–R3 behavior reviewed

- R1 at `safetyhook.cpp:1604-1760` records each affected page, restores rights with bounded attempts, and retains trap metadata until restoration succeeds. Failure after a page transition is accounted for by the subsequent state queries and rollback. Irrecoverable restoration calls fail-fast rather than continuing with unsafe permissions. This new import is used for an explicit terminal safety failure; it does not disable AV, modify another process or dynamically resolve hidden code. Microsoft documents that the API terminates the application and may invoke Windows Error Reporting.
- R2 narrows read/write/malformed/foreign exception admission, but WC01 means completion cannot be claimed for all timing orders. Removal of a trap before a delayed dispatch also needs focused retirement coverage; that broader window existed previously and was not resolved by the new NX query.
- R3 at `safetyhook.cpp:129-188,484-489,934-958` allocates private RW pages and seals emitted code RX. The late MidHook continuation pointer is written before final flush/seal. Published code is not subsequently rewritten by the inspected setup/enable/disable paths. Patch transactions still temporarily change involved trampoline/target pages to writable protections, then restore them; pages belonging to the ASI or VirtualProtect-related range may temporarily be RWX. That special case predates R1–R3. Tests of final RX state do not prove that every intermediate state is RX.
- Process-lifetime continuation retention remains in place. VMT tables remain RW data. There is no new permanent RWX allocation fallback. The changed code has no new networking, persistence, process creation or cross-process memory access path.

## Evidence-backed classification hypotheses

| ID | Property | What is supported / how to verify without weakening protection |
| --- | --- | --- |
| WH01 — Microsoft | New fail-fast call/import, changed VEH code, and altered executable-allocation code change static binary features. | These differences are measurable; their contribution to `Wacatac.B!ml` is unknown. Submit the exact unchanged sample and this delta to Microsoft for review, retaining scan/signature versions and submission result. Do not remove fail-fast or RX sealing to test a classifier threshold. |
| WH02 — Microsoft | A changed scan date/model/cloud metadata can alter a verdict independently of a code defect. | Microsoft documents cloud models using metadata and other signals. The two screenshots do not provide matched engine/signature versions. Vendor analysis or a matched-version comparison of the two unchanged hashes can distinguish this better than repeated code edits. |
| WH03 — Cynet | Both candidates retain native code generation, page-protection changes and VEH redirection. | These are expected hook capabilities and potential classifier inputs. Cynet documents several detection layers; its public material does not identify which one produced this VT result. Request sample-specific rationale for both hashes. The unchanged Cynet label across candidates cannot identify one new R1–R3 cause. |

## Unconfirmed assumptions

| Assumption | Why not established |
| --- | --- |
| Either alert is a false positive, or the numeric score is a 100% malware probability | No sample-specific vendor adjudication or definition of this Cynet score was supplied. Engine count alone establishes neither safety nor maliciousness. |
| Microsoft and Cynet share one cause | Their observed changes differ; no common feature attribution is available. |
| `RaiseFailFastException` alone caused Wacatac | It is the sole new import, but allocation, VEH and emitted machine code changed together. Correlation is not an isolated cause. |
| R1–R3 introduced malicious TLS code or a packed payload | Binary inspection contradicts the alleged structural changes: same TLS control flow, sections and resource payloads, with no trailing overlay. This does not prove the absence of every possible malicious implementation outside the scoped diff. |
| Current source is cryptographically proven identical to the inspected release's inputs | Build records are consistent, but there is no saved signed source/object hash manifest or reproducible rebuild from the exact input snapshot. |

## Priority plan

1. Correct WC01 with a transaction-phase/retirement contract and deterministic CPU-only tests for delayed execute exceptions. Preserve execute-only admission and R1 rollback guarantees; do not broaden the handler to unrelated AVs.
2. Repair WC02's metadata/reservation RAII gap and inject both allocation failures in a focused offline test. Verify the reservation is actually released.
3. Review only these fixes, run focused native scenarios OFF/ON and the relevant nongraphics regression, then build one Release ASI with a saved source/build/artifact identity record. Preserve both audited binaries for comparison.
4. Perform one separately authorized manual game qualification. Submit the unchanged qualified sample separately to Microsoft and Cynet with this report and hash identities; retain their independent verdicts. No claim of AV resolution precedes a scan or vendor result.

## Sources and execution limits

Read-only PE parser and bounded dumpbin runs exited and reported process cleanup. No production file was modified; this report is the only added artifact of this investigation. Existing offline PASS records were reviewed, not rerun as evidence for a new fix.

- [Microsoft RaiseFailFastException contract](https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-raisefailfastexception)
- [Microsoft vectored exception handling](https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling)
- [Microsoft exception record access-type fields](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record)
- [Microsoft cloud protection and sample signals](https://learn.microsoft.com/en-us/defender-endpoint/cloud-protection-microsoft-antivirus-sample-submission)
- [Microsoft sample submission](https://www.microsoft.com/en-us/wdsi/filesubmission)
- [Cynet detection layers](https://www.cynet.com/security-foundations/attack-techniques/4-malware-detection-techniques-and-their-use-in-epp-and-edr/)
