# SafetyHook R1–R3 hardening

Status: WB01/WB02 CORRECTION COMPLETE; OFFLINE-VERIFIED. This plan was reopened after the independent behavioral-overlap audit and has since been completed. Current source, targeted evidence and the new candidate are recorded in [WB01/WB02 correction report](../reports/SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md). Branch: `safetyhook-r1-r3-hardening`.

Baseline: production ASI SHA-256 `5124A197A036BFF5C630CA0E48250C3520745D508CC7E66465071ED75BA8E654`; preserve both existing copies unchanged. The worktree already contains staged v3.2.1 changes; preserve them.

Preserve the inspected R1–R3 ASI as evidence too: `build-artifacts/security-hardening/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`, SHA-256 `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB`. Future builds must use a separate output directory so neither audited binary is overwritten.

Current review: [Microsoft Wacatac / Cynet targeted investigation](../reports/WACATAC_CYNET_SAFETYHOOK_TARGETED_INVESTIGATION_2026-10-17.md). Its WC01/WC02 findings supersede the previous unconditional completion assessment. Neither finding has an established causal connection to an AV verdict.

The [Wacatac behavioral-overlap production audit](../reports/WACATAC_BEHAVIORAL_OVERLAP_SECURITY_AUDIT_2026-10-17.md)
records the original WB01/WB02 findings as they stood during that read-only
review. The bounded corrections were subsequently implemented and verified;
see the correction report for source paths, targeted test evidence and limits.
No causal AV attribution is established.

## Current status

| Package | Status | Remaining work |
| --- | --- | --- |
| R1 protection recovery | Implemented; WB02 correction preserves the bounded recovery contract | Offline correction evidence is complete; manual game qualification remains separate |
| R2 VEH admission | WB01/WB02 implemented and offline-verified | Targeted epoch-isolation, route-selection and AV-admission scenarios are recorded in the correction report |
| R3 executable publication | RW-to-RX, WC02 RAII, and WB02 continuation retention implemented and offline-verified | No permanent RWX fallback; manual game qualification remains separate |
| R4 validation and artifact | Focused and canonical non-graphics validation passed; isolated candidate recorded | Live/manual game qualification remains outstanding |

Previous evidence remains historical: the earlier `test.cmd -NonGraphics` run passed 62/62 invocations and 214 native child processes; it did not cover WC01/WC02. The current corrected run is recorded in [WC01/WC02 correction report](../reports/SAFETYHOOK_WC01_WC02_CORRECTION_2026-10-17.md).

## Approved WB01/WB02 correction contract

Work remains on `safetyhook-r1-r3-hardening`; preserve all existing staged and
unstaged changes and all prior ASIs. Do not use the workspace root ASI as an
output. Recheck current source before implementing the audit findings.

**Single contract:** a trap generation is authoritative for both its protection
transition and its source-to-continuation route. An execute AV can be redirected
only through that same live transaction's exact source range while its page is
owned by the transaction and actually non-executable. Completed generations may
identify a delayed WC01 retry after restoration, but must never supply a route
for a different active generation. Per-thread repeat suppression is keyed by
the admitted generation and page allocation identity; a later generation or
reused allocation cannot inherit suppression. Before any page transition can
make a route executable through VEH, the trampoline and every owned continuation
it can reach must already have process-lifetime storage. Published continuation
storage is not freed while any thread could still execute it. Existing R1
recovery/fail-fast, R2 execute-only admission, R3 private RW-to-RX publication,
WC01 delayed retry and WC02 RAII remain in force.

Expected files: `external/safetyhook/safetyhook.cpp/.hpp`,
`external/safetyhook/LOCAL_CHANGES.md`,
`tests/plugin/runtime_native_adapter_harness.cpp`,
`tests/plugin/runtime_native_evidence_cases.hpp`, this plan, and a dated
WB01/WB02 correction report/evidence record.

Acceptance:

1. Persistent worker: the same source address faults in two distinct generations
   with no intervening exception; both are admitted. A genuine unresolved repeat
   in one generation continues search and cannot loop.
2. Protection failure before the first patch write, after another thread has
   been redirected into a trampoline: rollback and hook destruction leave the
   trampoline and owned stub committed RX until that worker exits.
3. Completed route A cannot redirect an execute fault belonging to active
   transaction B on the same page. Exercise same-page hooks, address reuse and
   existing delayed-after-RX delivery.
4. Read/write AVs remain unhandled; failed-create, repeated-hook and R1/R3/WC02
   regressions stay intact.
5. Focused diagnostics OFF and ON, `test.cmd -NonGraphics`, Release build,
   artifact identity/PE inspection and SHA-256 refer to the corrected source
   and a new isolated output. No game/debugger/WARP/graphics run or installation.

Do not claim a scenario PASS unless the test establishes the specified
interleaving and checks post-destruction memory state. If the native harness
cannot safely establish the real NX-page ordering, document the gap rather
than substituting a synthetic handler call.

## Scope and order

1. R1: Make protection transitions and trap lifetime explicit in `external/safetyhook/safetyhook.cpp`. A failed transition must not unmap a live execution trap or silently leave an NX page. Cover real failures before the OS changes protection and reported failures after it changes protection.
2. R2: Limit the VEH to valid execute access violations associated with a live trap and coherent context. Preserve simultaneous hook and process shutdown behavior.
3. R3: Build code in private RW pages, synchronize, then publish RX. Avoid writing into published pages, including the late MidHook trampoline pointer. Preserve near allocation, retained continuations and hook reset.
4. R4: Focused native tests per batch, one relevant complete regression pass, Release ASI, PE and VirtualQuery checks, SHA-256, manual game checklist.

## Remaining implementation batches

### Batch A — WC01: original delayed-after-RX case verified; broader contract REOPENED

Evidence: `external/safetyhook/safetyhook.cpp:1484-1489` checks current protection, while `restore_pages()` can already have restored RX before the handler processes an exception raised during the owned NX interval. The matching trap can still be live when the handler incorrectly returns `EXCEPTION_CONTINUE_SEARCH`. Dispatch around subsequent trap removal also lacks focused coverage.

1. Transaction metadata now records Armed/Committed/Completed phase, page allocation identity, original/temporary protection, transition-started/changed state, and generation. Completed per-hook records are retained as tombstones rather than removed at RX restoration.
2. The VEH still requires execute AV, matching fault address/IP/exception address, valid control context and matching committed allocation. It accepts active owned NX faults; after RX restoration it retries the unchanged IP once for the committed/completed transaction, with a thread-local one-shot repeat escape to CONTINUE_SEARCH.
3. The native integration test holds two real execute AVs at a harness VEH barrier until after RX restoration and transaction completion. Both then pass through the production handler, execute their native continuation/result, and exit. This exercises delayed dispatch after the transaction, not only a synthetic admission result.
4. Existing strict rejection tests for read/write, malformed, mismatched context and unrelated records remain; full tests and regression evidence are listed in the correction report.

Offline acceptance: delayed owned faults are retried after RX restoration; read/write and malformed records continue search; metadata remains available through delayed dispatch; the retry cannot loop indefinitely. Runtime interaction with unrelated third-party VEH handlers is not proven by the harness and remains a qualification limit.

### Batch B — WC02: allocation ownership on exceptions — COMPLETE OFFLINE

Evidence: `external/safetyhook/safetyhook.cpp:148-157` obtains a region before `new Memory` and vector insertion establish ownership. Allocation failure can leak the region and, on vector growth failure, the raw metadata object. This is inherited behavior, not a newly proven AV cause.

1. Reserve collection capacity and create the metadata owner before VirtualAlloc. Immediately attach a successful region to that owner, then move it into the already-reserved vector.
2. Inject `std::bad_alloc` after the region is owned; confirm `VirtualQuery` reports `MEM_FREE` after unwinding. A subsequent normal allocation succeeds and is verified committed RW.
3. RX publication, private-page allocation and retained continuation checks passed; no persistent RWX fallback was introduced.

Offline acceptance: every throwing operation after successful VirtualAlloc is covered by RAII ownership; the injected failure releases the real reservation; normal published continuations retain their required lifetime.

### Batch C — final verification and handoff

1. WB01/WB02 focused scenarios passed OFF/ON; canonical `test.cmd -NonGraphics` passed 62/62 registry invocations (112 native scenarios; 218 native children because graphics children were omitted). Process-tree cleanup was verified.
2. Production Release ASI was built in `build-artifacts/security-hardening/wb01-wb02/`, statically inspected, and hashed. All four prior artifact hashes were rechecked unchanged.
3. WB01/WB02 are closed by the stated offline acceptance evidence. Live game qualification remains open and is not claimed by these tests. Do not install automatically.
4. Microsoft and Cynet assessment remains independent and unresolved; no scan or AV conclusion is part of this correction.

Closure status: WB01 generation-scoped delayed retry and WB02 route-exposed continuation ownership are implemented and offline-verified. R1–R3, WC01 and WC02 remain intact. No evidence establishes a cause for Microsoft or Cynet classifications, and no live-game qualification was performed in this pass.

Post-correction AV observation: user screenshots identify `492A5884528ED8BA7C6401D15FF178B9EF2AFA77DD1903F8A687A342D7591298` at 2/71, with unchanged Cynet and Microsoft labels. A read-only comparison confirmed RaiseFailFastException was already present in the CC40 artifact and that WC01/WC02 added no imports. This does not establish classifier causality or reopen the correction scope. Details are in the correction report's subsequent-observation section. Next functional step remains manual game qualification of the exact corrected hash; no further SafetyHook edits are planned without new defect evidence.

AV-priority follow-up: the user deferred game qualification to investigate the
new Microsoft verdict first. Proposed package-subtraction builds would restore
previously identified risks or require a new R1/R2 recovery design, so none were
built. Three unchanged ASIs were instead custom-scanned with the same installed
Defender engine/signatures, with remediation disabled: all reported no threats.
See [same-engine comparison](../reports/MICROSOFT_WACATAC_SAME_ENGINE_COMPARISON_2026-10-17.md).
This is a local result, not a resolution of the VirusTotal or Cynet verdicts.
Current priority is the Microsoft environment/verdict discrepancy; manual game
qualification remains outstanding and deferred at the user's direction.

Expected files: bundled SafetyHook implementation/header, local vendor changes record, native adapter/evidence harness, this plan and a concise completion report. Touch build scripts only for isolation or targeted test wiring.

Non-goals: Gameplay/Cinematics/Dialogue/Overlay behavior, DLL pinning, game execution, debugger/graphics fixtures, installed ASI, VirusTotal upload or classifier evasion.

Risks: page permissions during in-flight execution; restoration failure under real OS error; near-address allocation; trap lock ordering and process termination. Validate each changed state with observed `VirtualQuery` data and bounded failure injection. If restoration cannot be made safe after a true OS failure, report that explicit residual limitation rather than claiming a fully safe recovery.
