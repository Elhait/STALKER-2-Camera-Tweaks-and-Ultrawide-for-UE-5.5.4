# SafetyHook WC01 / WC02 correction — 2026-10-17

Follow-up status: the later [behavioral-overlap audit](WACATAC_BEHAVIORAL_OVERLAP_SECURITY_AUDIT_2026-10-17.md)
identified WB01/WB02 after this WC01/WC02 report. Those subsequent findings
were later corrected and offline-verified; see the
[WB01/WB02 correction report](SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md).
This report remains the record for WC01/WC02 and their original evidence. The
preserved `492A…1298` artifact and earlier repository-root ASI identity are
historical; current WB01/WB02 status and candidate identity are in the later
correction report.

## Outcome

WC01 (delayed execute AV after RX restoration) and WC02 (VirtualAlloc leak on
metadata allocation failure) are corrected in branch
`safetyhook-r1-r3-hardening`. Offline validation is complete. No game, WinDbg,
WARP, graphics fixture, VirusTotal upload, or ASI installation was performed
by the assistant. A subsequent user-supplied VirusTotal result is recorded
below. This is not a conclusion about the cause of either detection.

## Corrections and evidence

### WC01 — delayed execute exception

The VEH previously relied on the current page protection at dispatch. That can
reject a SafetyHook execute fault raised while the page was NX but delivered
after the patch transaction restored RX.

Trap records now retain transaction phase, generation, page allocation
identity, original and temporary protections, and transition history. The
transition is marked before `VirtualProtect`; the observed OS state is then
recorded. Completed per-hook trap records remain as compact process-lifetime
tombstones so delayed exception dispatch does not rely only on the current
protection state. Admission still requires an execute AV, coherent fault
address/IP/exception address, valid control context, and matching committed
allocation identity. Read/write, malformed and unrelated records continue
searching. For a completed transaction, the handler retries the unchanged
instruction pointer once; a repeated same-address fault on that thread exits
to `EXCEPTION_CONTINUE_SEARCH`, preventing an infinite retry loop.

`EvidenceSafetyHookDelayedExecute` coordinates two real worker-thread execute
faults. The harness VEH holds both faults while the patch transaction finishes
and restores RX; after release, both faults traverse the production handler
after transaction completion. Both workers return the expected native value,
and a subsequent unhooked call returns the same value. The test has bounded
barriers and process-supervisor cleanup.

This demonstrates the requested delayed-after-RX ordering in the controlled
Windows harness. It does not establish compatibility with every third-party
VEH ordering or every game/injector interaction. Trap tombstones intentionally
remain process-lifetime metadata; memory growth is bounded by distinct hook
source addresses used by the module, not by transaction count for the same
address.

### WC02 — reservation ownership

Allocator vector capacity and its `Memory` owner are prepared before
`VirtualAlloc`. Immediately after a successful allocation, the region address
and size are attached to the RAII owner; moving the owner into pre-reserved
storage cannot allocate. If a later operation throws, `Memory` releases the
reservation.

The native test injects `std::bad_alloc` after ownership has been attached,
then checks with `VirtualQuery` that the actual address is `MEM_FREE`. It also
performs a successful allocation afterward and verifies that it remains
committed and RW before code publication.

## Validation

- Focused native scenarios passed with Diagnostics OFF and ON: WC01 delayed
  execute dispatch, strict VEH admission, WC02 ownership/failure cleanup,
  existing protection-failure/rollback cases, hook lifetime and RX publication.
  `build-artifacts/security-hardening/FocusedBuild.json` records successful
  compilation; the focused execution completed without a remaining test
  process.
- Canonical `test.cmd -NonGraphics`: 62/62 registry invocations passed; 110
  native cases / 218 fresh native child processes passed. Process-tree cleanup
  verified; the graphics scenario was excluded. Evidence:
  `build-artifacts/security-hardening/Regression.json`.
- Production Release build completed successfully in the isolated
  `wc01-wc02` output directory. The first full build attempt was marked timed
  out by the supervisor because the expected `vctip.exe` helper outlived its
  parent; the owned process tree was cleaned. A subsequent bounded incremental
  build completed with exit code 0, no timeout, and verified cleanup. CMake
  emitted long-object-path warnings for unrelated test harness targets in
  this deep output directory; the production ASI linked successfully.
- Static PE inspection found AMD64, the expected seven sections, no static
  writable+executable section, and imports/resources/TLS matching the prior
  hardened artifact. Static PE metadata does not replace runtime protection
  checks; the focused tests queried actual allocation rights with
  `VirtualQuery`.
- `git diff --check` passed. No other mod subsystem was intentionally changed.

## Candidate and preserved artifacts

New candidate (not installed):

`build-artifacts/security-hardening/wc01-wc02/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

- Size: 3,554,304 bytes
- SHA-256: `492A5884528ED8BA7C6401D15FF178B9EF2AFA77DD1903F8A687A342D7591298`

The two original release-candidate copies still hash to
`5124A197A036BFF5C630CA0E48250C3520745D508CC7E66465071ED75BA8E654`. The
previous hardened artifact remains unchanged at
`build-artifacts/security-hardening/release/production-tOFF-dOFF/bin/` with
SHA-256 `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB`.

## Remaining qualification

The next functional step is one user-controlled live game test of hook
behavior, Overlay visibility, and normal exit using this candidate. Do not
install it automatically. Neither WC01/WC02 correction nor the new hash proves
or disproves either vendor's classification.

## Subsequent VirusTotal observation and fail-fast check

The user supplied two screenshots for the exact corrected candidate hash
`492A5884528ED8BA7C6401D15FF178B9EF2AFA77DD1903F8A687A342D7591298`:

- Detection screenshot: `C:/Users/enton/AppData/Local/Temp/codex-clipboard-8c408d3e-da73-4725-b4d2-a6b2fa53a88e.png`.
- Details screenshot: `C:/Users/enton/AppData/Local/Temp/codex-clipboard-da58dda2-f19c-4801-884a-250db6b22803.png`.

| Sample | User-supplied scan result | Cynet | Microsoft |
| --- | --- | --- | --- |
| Preserved `5124…E654` | 1/71 | Malicious (score: 100) | No detection shown in the earlier supplied scan |
| R1–R3 `CC40…67CB` | 2/69 | Malicious (score: 100) | Trojan:Win32/Wacatac.B!ml |
| WC01/WC02 `492A…1298` | 2/71 | Malicious (score: 100) | Trojan:Win32/Wacatac.B!ml |

These are observations from different user-supplied scans, not a matched
engine/signature-version experiment. The unchanged labels show that WC01/WC02
corrections did not remove the reported detections in this scan. They do not
identify a classifier feature or prove that either defect was irrelevant to
classification. The different denominator alone is not evidence of a changed
verdict for either named engine.

A bounded read-only PE comparison of the three unchanged local files rechecked
their exact SHA-256 values and import tables:

| Property | `5124…E654` | `CC40…67CB` | `492A…1298` |
| --- | --- | --- | --- |
| Imported functions | 211 | 212 | 212 |
| `KERNEL32!RaiseFailFastException` | Absent | Present | Present |
| Import delta from preceding sample | — | Only RaiseFailFastException added | None |
| TLS callbacks | 2 | 2 | 2 |
| Resource entries | 30 | 30 | 30 |
| Static writable+executable sections | None | None | None |

The parser ran under the process-tree supervisor with a 20-second timeout and
12,000-character output budget; it exited 0 without timeout or truncated output,
and cleanup was verified. It parsed bytes without loading the ASI.

Current source contains one direct RaiseFailFastException call in
`unrecoverable_protection_failure()`. Callers reach it when bounded protection
restoration fails, or when the patch callback throws and byte completeness
cannot be established. Successful patch transactions do not call it. The
restoration fail-fast was introduced by R1 before the CC40 artifact; WC01/WC02
did not introduce the import. The source still requires this terminal response
to avoid returning into an uncertain code/protection state.

Its presence remains a measurable classification hypothesis for the Microsoft
change, alongside the simultaneous allocator/VEH/machine-code changes. It is
not an isolated causal explanation. Cynet already detected the preserved
sample without this import, so this import is not a prerequisite for the
observed Cynet detections across these samples.

Disposition: retain the current SafetyHook implementation and both corrections;
complete one manual functional qualification of this exact hash. Keep AV review
separate: submit the unchanged sample and comparison evidence to Microsoft and
Cynet if authorized, retaining their independent responses. No production edit,
new build, scan, installation, or runtime launch was made for this follow-up.
