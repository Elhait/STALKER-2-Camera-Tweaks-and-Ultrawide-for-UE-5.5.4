# SafetyHook WB01/WB02 Correction — 2026-10-17

## Outcome

WB01 (retry ticket generation isolation) and WB02 (continuation lifetime after a pre-write protection failure) are implemented and have focused Windows-native evidence. The exact protected interleavings were tested with real execute faults and `VirtualQuery` checks; the canonical non-graphics suite and a fresh production Release build also passed.

No game, debugger, WARP, graphics fixture, VirusTotal scan, or installation was performed. This is offline engineering evidence, not live game qualification or an antivirus verdict.

## Root causes and corrections

### WB01 — delayed retry suppression crossed transaction generations

The thread-local “one unresolved retry” record could outlive the transaction that created it. Matching only the same fault address meant a later transaction at a reused source address could inherit the previous transaction’s suppression and return `EXCEPTION_CONTINUE_SEARCH` before its own retry was attempted.

The ticket now identifies the source address, page address, allocation base, memory type, and trap generation. Suppression applies only to a second unresolved execute fault matching that complete transaction identity. A new generation at the same address gets its own retry. VEH routing also selects an active transaction before considering completed same-page records, and an active transaction only redirects when the fault address falls inside that transaction’s exact source range.

Evidence:

- `evidence-safetyhook-retry-generations` holds one worker across enable/disable/re-enable at the same source address. Both transaction generations produce a real execute fault and complete their native call; the worker’s first delayed ticket does not suppress the second generation.
- `evidence-safetyhook-veh-admission` additionally verifies completed route A cannot hijack active route B on the same page, then frees/reallocates the source at the same VA and verifies the newer allocation/generation wins.
- Both scenarios passed with Diagnostics OFF and ON. Existing read/write and malformed-VEH admission checks remain in the same focused scenario.

### WB02 — route exposure was not represented in owner lifetime

The hook owner previously used “patch bytes were written” as the effective signal for retaining executable continuations. That is insufficient: VEH can redirect a worker to the trampoline after the source page becomes NX, before the transaction reaches its first write. A subsequent pre-change `VirtualProtect` failure and owner destruction could therefore free code that a redirected worker is still executing.

The transaction now reports whether a route was exposed. Trampoline and MidHook stub ownership is retained before entering the protection transition; if the operation fails without a write or route, that temporary retention is released. If VEH exposed a route, retention survives rollback, move/reset/destruction, and remains process-lifetime. Route exposure is marked under the trap lock before the context IP is redirected.

Evidence:

- `evidence-safetyhook-prewrite-route-lifetime` uses real RX source code whose relocated trampoline calls a barrier routine. The worker is demonstrably inside the trampoline while the second `VirtualProtect` fails before changing the trampoline page and before any patch write.
- Rollback is checked against the original code bytes and RX protection. The hook owner is destroyed while the worker remains blocked in the trampoline; both trampoline and stub are checked as committed RX before and after destruction. The worker is then released and completes the original return value.
- The test verifies two retained executable allocations and passes with Diagnostics OFF and ON. The fixture does not call or wait on the source function while its worker is held.

## Validation

- Focused: VEH admission/isolation, retry generations, and pre-write route lifetime all passed OFF and ON (6/6 process runs); cleanup was verified after each.
- Canonical `test.cmd -NonGraphics`: PASS, 62/62 registry invocations. The registry runner reports 112 native cases; 218 fresh native child processes ran because the two graphics-mode children were intentionally omitted. Build, runner exit-code contract, and process-tree cleanup passed. The first sandboxed run was blocked by its hard-link fixture (`ERROR_ACCESS_DENIED`); the isolated harness passed with the needed local permission, then the full canonical suite passed in the same bounded, supervised context.
- Production Release build: PASS, MSVC 19.44, `production-tOFF-dOFF`, separate object and output roots. The ASI linked and was atomically published to the new isolated candidate. CMake emitted long-object-path warnings for some non-production test targets in this deep build directory; production objects and the ASI linked successfully.
- Static PE inspection: AMD64 PE32+, 7 sections, 212 imported functions across 7 DLLs, 2 TLS callbacks, 30 resources (all source-mapped embedded resources matched; the additional VERSIONINFO resource is expected), and zero static writable+executable sections. Static section flags do not replace runtime `VirtualQuery`; focused tests checked actual trampoline/stub rights as committed RX.
- `git diff --check`: PASS.
- Prior artifact identity rechecked unchanged: `5124…E654` (3,552,768 bytes), `CC40…67CB` (3,554,304 bytes), `492A…1298` (3,554,304 bytes), and repository-root `9AEB…5B70` (3,554,304 bytes).

## Candidate

New, not installed:

`build-artifacts/security-hardening/wb01-wb02/STALKER2CameraTweaks.asi`

- Size: 3,555,328 bytes
- SHA-256: `DC2877E864C1C9B73CC2CB285C97EB2285837441C33CB3866D6133C5D21AD7DE`
- Profile: production Release, startup timeline OFF, dialogue discovery OFF.

The artifact is separate from all four prior ASIs; those paths and hashes were read-verified after the build.

## Remaining limits

The candidate has not been tested in the game. Manual functional qualification (hook behavior, Overlay, and normal exit) remains a separate step and requires the user-controlled launch already agreed for runtime validation. No causal claim is made about Cynet or Microsoft detections; no scan was run for this candidate.
