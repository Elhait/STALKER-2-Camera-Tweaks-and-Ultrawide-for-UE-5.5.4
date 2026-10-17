# SafetyHook R1–R3 hardening — 2026-10-17

Historical report: later reviews found WC01/WC02 and then WB01/WB02. All four
findings were subsequently addressed and offline-verified; WB01/WB02 evidence
and the resulting candidate are recorded in the
[WB01/WB02 correction report](SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md).
The [behavioral-overlap audit](WACATAC_BEHAVIORAL_OVERLAP_SECURITY_AUDIT_2026-10-17.md)
is an audit-time snapshot, not the current open-findings list. The original
PASS results below apply only to their recorded scenarios and artifact.

Status: implemented and offline-validated on branch `safetyhook-r1-r3-hardening`. Manual in-game qualification remains. No ASI was installed or uploaded. The previous release candidate was not modified.

## Identity and scope

- Preserved baseline: `build-artifacts/release-3.2.1-candidate/STALKER2CameraTweaks.asi` and repository-root `STALKER2CameraTweaks.asi` both remain SHA-256 `5124A197A036BFF5C630CA0E48250C3520745D508CC7E66465071ED75BA8E654`.
- New production Release ASI: `build-artifacts/security-hardening/release/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`, 3,554,304 bytes, SHA-256 `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB`.
- Production edits for this task are confined to `external/safetyhook/safetyhook.cpp` and `.hpp`; test registry/harness and the vendor-local changes record were updated. The branch inherited unrelated staged v3.2.1 work; this task did not reset or rewrite it.

## R1 — page protection and trap lifetime

Root cause: the old `trap_threads_result()` trusted the `VirtualProtect` return value, restored only ranges whose forward call reported success, and removed trap metadata even if rollback/restoration failed. A page could therefore remain non-executable or writable after an error while the exception handler no longer had a coherent trap.

The replacement at `external/safetyhook/safetyhook.cpp:1604-1760` serializes the transaction before trap registration, snapshots each affected committed page's original protection, queries effective rights after transitions, and restores every affected page. It treats image-page WRITECOPY as the observed equivalent of requested writable protection. A transient restore failure gets at most two attempts; trap metadata is retired only after original rights are confirmed. If restoration cannot be confirmed, the process fails fast while the trap is still registered rather than returning into an unknown executable-memory state. A callback exception is contained while the restoration path runs.

Invariant: a normal return leaves all affected pages at their recorded original protections and no live transaction trap. A permanent OS refusal is a terminal safety failure, not a successful rollback. The permanent-failure fail-fast path was reviewed statically, not exercised by the ordinary harness because that would intentionally terminate its process.

## R2 — VEH admission

Root cause: the previous VEH path admitted an access violation by exception code and trap-page lookup without establishing that it was an execute fault owned by SafetyHook. At `external/safetyhook/safetyhook.cpp:1458-1505`, admission now requires execute AV (`ExceptionInformation[0] == 8`), fault address coherent with instruction pointer and exception address, a valid control context, a live trap, and effective temporary NX protection. Read/write, malformed, mismatched, or foreign AVs continue to the next handler. The process-shutdown bypass remains ahead of locking.

Invariant: only an execute fault in a current SafetyHook patch transaction can be redirected or retried. Synthetic deterministic admission tests cover accepted and rejected records; an integrated game/third-party VEH interaction was not exercised.

## R3 — published executable memory

Root cause: the allocator suballocated hooks from shared permanently RWX blocks; sealing one hook would also affect later writes for another hook on the same page. At `external/safetyhook/safetyhook.cpp:129-188`, each allocation now owns a private page-sized reservation committed RW. The final trampoline is flushed and sealed RX at line 484; the MidHook stub's final continuation pointer is written before its final flush and RX seal at lines 946-958. VMT table storage remains RW data. There is no RWX allocator fallback. Temporary RWX can still occur during the serialized patch transaction for ASI/VirtualProtect-related pages, followed by restoration; this is not a persistent trampoline or stub allocation.

Invariant: published stubs and trampolines are RX, not writable; no later allocation writes into an existing RX page. Continuations retained after hook reset retain their RX storage for process lifetime.

## Validation

- R1, R2 and R3 focused native adapter scenarios passed in Diagnostics OFF and ON, including real pre-change `VirtualProtect` refusal, transient partial rollback, read/write/execute VEH admission, two simultaneous hooks, disable/reset/recreation, execution through published code, and `VirtualQuery` checks of RX protection and private one-page small allocations. Relevant pre-existing lifetime/trampoline scenarios also passed.
- Final canonical `test.cmd -NonGraphics`: 62/62 registry invocations PASS; 214 fresh native child processes (one graphics case omitted); all four Zoom scenarios PASS; bounded process-tree cleanup verified. Evidence: `build-artifacts/security-hardening/Regression.json` (exit 0, no timeout, no output truncation, cleanup verified). WARP/graphics fixtures were intentionally omitted.
- A sandboxed run first failed only at the unrelated config harness's `CreateHardLinkW` fixture with Windows error 5. A focused, bounded run outside the sandbox passed that fixture and its full harness. The final canonical run outside the sandbox passed. The diagnostic-only instrumentation used to isolate the API was reverted; the config harness source has no task-specific change.
- Production Release build succeeded with process cleanup verified. CMake warned that one *harness* object path in this long build directory may exceed its safe path budget, but the ASI link completed; no object was placed in the repository root.
- Static PE inspection of the new ASI: AMD64, seven sections, no static writable+executable section, no trailing overlay bytes. One source-resource comparison reports `UNKNOWN` for RT_VERSION because the inspector indexes the localization resource manifest, not VERSIONINFO; this is not a binary mismatch. Static PE flags do not prove dynamic rights; the focused `VirtualQuery` tests cover those transitions in an offline process.
- Both preserved ASI copies were rehashed after the new build and still match the baseline SHA. `git diff --check` reported no whitespace errors.

## Residual limits and manual qualification

No game, WinDbg, WARP, graphics fixture or VirusTotal scan was run for this candidate. Offline tests cannot prove compatibility with the game's live camera callbacks, concurrent game threads, graphics injectors, or a specific AV classifier. Permanent inability to restore page rights is intentionally fail-fast and remains unexercised as a runtime fault-injection scenario.

For one manual game session, copy the new ASI only after separately preserving the installed one; verify startup, visible Overlay, normal camera transitions, hook-sensitive features and a clean game exit. Keep the installed configuration unchanged. If any fault occurs, capture the exact ASI hash, game version, logs, and reproduction sequence before deciding on rollback. Compare VirusTotal only after functional stability is confirmed. Do not treat this report as a claim that Cynet has changed its verdict.
