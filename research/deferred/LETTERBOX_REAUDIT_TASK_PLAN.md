# Experimental Letterbox Re-audit And Redesign

Status: In progress — Batch 1 and Batch 2 are complete; Batch 3 manual testing confirmed safe execution but found an incorrect cutscene FOV/projection result in game version 2.0.3.

## Objective

Re-audit the experimental cutscene letterbox A/B hook design against the established static evidence and prepare a minimal safe implementation path for cutscene aspect/letterbox correction.

The task must determine whether safe hook boundaries and return contracts can be established for both letterbox setter entrypoints. It must not assume that the previous `RET`-adjacent hook placement is valid.

## Established evidence and current state

- The stable gameplay ASI operates without the experimental letterbox ASI.
- Manual testing of the setter-level redesign confirmed that the experimental module no longer crashes during the tested cutscene flow and that the A/B interception is active.
- NVIDIA/DLSS logging showed that the cutscene render target changes from `2560x1440` without the experimental ASI to `5120x1440` with the setter-level redesign at the native `5120x1440` display. This confirms that the A/B interception changes the cinematic render/aspect path rather than merely changing a cosmetic overlay.
- The same test showed incorrect cutscene FOV/projection behavior: the render aspect becomes native `32:9`, while the camera/projection FOV remains inconsistent with that aspect. The post-cutscene gameplay correction then makes the contrast visible.
- The current implementation therefore passes the native cutscene render/aspect objective but does not yet preserve correct FOV/projection behavior. It is not release-ready, and no FOV multiplier may be added as a shortcut.
- The previous `STALKER2ExperimentalLetterboxFix.asi` caused `EXCEPTION_ACCESS_VIOLATION` reading `0xffffffffffffffff` during a cutscene.
- The experimental ASI is disabled and must not be retested in its current form.
- BigChenga reference evidence identified two letterbox setter signatures, A and B, in the historical Steam 2.0.2 executable.
- The current runtime target is game version 2.0.3. Its executable identity, including available PE/hash evidence, must be recorded separately from the historical 2.0.2 provenance.
- Historical 2.0.2 addresses are provenance only; they must not be reused as runtime addresses for 2.0.3.
- Current 2.0.3 test executable: `Stalker2-Win64-Shipping.exe`, size `174,717,488` bytes, image base `0x140000000`, SHA-256 `81961B7281C7CF528CE49C549CE086FCC684BD676F32FAF042BC743D939E3C69`.
- The durable Ghidra project at `E:\Work\Slaker2 mods\Dump\STALKER2-Ghidra` contains analyzed program entries for both `Stalker2-Win64-Shipping.exe (2.0.2)` and `Stalker2-Win64-Shipping.exe (2.0.3)`, so the comparison can be performed there without treating the C: temporary copies as the primary project.
- Read-only scan of the current executable found exactly one A match and one B match. Resolved entrypoints are A `0x146B77992` and B `0x146B779B6`; setter instructions are A `0x146B779AB` and B `0x146B779CF`.
- The current A/B matches decode to the expected `MOV dword ptr [RAX+0x254], 0x3FE38E39` setter instruction.
- Read-only Ghidra comparison of the durable 2.0.2 and 2.0.3 program entries confirmed the same instruction layout in both versions: A is `setter; ret`, while B is `setter; mov al,1; ret`, followed by the next function's `push rsi` prologue.
- The A/B entrypoints shifted by `0xC24` between the analyzed versions, confirming that runtime addresses must remain resolver-derived.
- Ghidra has not assigned Function objects to these raw entrypoints, but the decoded `ret` and following function prologue provide clear local continuation boundaries for this bounded comparison.
- Both targets decode as `MOV dword ptr [RAX+0x254], 0x3FE38E39`.
- A and B have different observed return contracts: A returns with `ret`; B returns with `mov al, 1; ret`.
- The previous implementation attempted to hook at the `RET` immediately following the setter.
- Source inspection confirms the old implementation passes A's post-setter `RET` and B's post-setter `MOV AL,1` address to `safetyhook::create_mid`; this remains an unsafe design candidate and is not approved for retesting.
- SafetyHook's near `E9` path can replace the complete 10-byte setter instruction without touching the A/B suffix. A setter-level mid-hook can write the desired aspect, advance the trampoline instruction pointer past the original setter, and resume at the untouched suffix, preserving A's `ret` and B's `mov al,1; ret`.
- SafetyHook's possible 14-byte absolute fallback would be unsafe at these function tails unless separately proven not to overwrite a suffix or adjacent function entry; it must not be accepted implicitly.
- A common class owner and upstream caller relationship remain unproven.
- Weapon/viewmodel FOV research is deferred pending new external reference evidence and is not part of this task.

## Approved scope

- Re-audit the exact executable/build identity and the existing A/B static evidence.
- Resolve the A/B signatures dynamically in the current 2.0.3 executable and record match counts, decoded instructions and resolved addresses.
- Inspect the current experimental source and build script for hook-boundary, trampoline, ABI, register, flag and state-lifetime risks.
- Establish safe hook boundaries for A and B without changing the stable gameplay path.
- Preserve each original setter and return contract exactly once.
- Reuse existing resolver/decode/safety infrastructure only where doing so causes no gameplay behavior change or broad refactor.
- Consider the least invasive intervention that replaces the validated 16:9 write/value production while leaving game control flow unchanged, if static evidence supports it. Do not assume a post-setter detour is required.
- Define shared letterbox aspect state initialization, update ownership and safe access rules.
- Prepare or implement the smallest experimental letterbox redesign justified by the re-audit.
- Validate safe refusal and all-or-nothing installation behavior before any manual game test.

## Explicit non-goals

- No changes to stable gameplay aspect correction.
- No weapon/viewmodel FOV, dialogue FOV or hard-coded FOV multiplier.
- No timing workaround or forced replay of the gameplay aspect fix.
- No release integration or compatibility claim beyond the analyzed 2.0.3 executable/build identity.
- No broad renderer or weapon/viewmodel research.
- No changes to Ghidra originals, unrelated research branches or publishing assets.
- No deletion of temporary Ghidra folders until archival completeness is verified and the exact cleanup scope is confirmed.
- No final task closure, Git commit or release packaging.
- No final `TASKLOG.md` entry or final Git review until the redesigned module passes the required manual test.

## Expected files and areas

- `src/cutscene_letterbox_fix.cpp`
- `build-artifacts/test-scripts/build-letterbox.cmd` or the current approved letterbox build entrypoint
- `build-artifacts/test-asi/` for disposable experimental output
- Existing resolver/decode helpers only if behaviorally safe to reuse
- This plan file, kept updated as the phase changes

## Batches

### Batch 1 — Static re-audit

- Confirm both identities: historical Steam 2.0.2 provenance and the current game 2.0.3 test executable, including available PE/hash evidence.
- Compare the two analyzed program entries inside the durable Ghidra project; do not overwrite or merge their version-specific analysis state.
- Resolve and re-decode both targets in version 2.0.3; document match counts, exact instruction boundaries, overwritten bytes and return ABI.
- Determine whether the previous hook placement intersects a function boundary, invalid context, unsafe continuation or incorrect trampoline assumptions.
- Review context safety: use only the validated target object, avoid unproven dereferences, and preserve the machine state required by the original instruction stream and proven ABI/continuation contract. Do not assume additional clobbers are safe.
- Compare a minimal validated-write/value intervention with a conventional detour only when static evidence supports the former; select the safer option rather than preserving the old design by default.
- Prefer a setter-level mid-hook: write `[RAX+0x254]` from the validated context, skip the original immediate-write instruction in the trampoline, and resume at the untouched A/B suffix.

Validation: source inspection, current 2.0.3 executable identity, reproducible unique-match scan and decoded setter evidence. No game launch.

Batch 1 result: the previous post-setter placement is rejected. The validated minimal intervention point is the setter instruction itself. SafetyHook patch-path confirmation is an implementation gate in Batch 2, not a remaining Batch 1 item.

Comparison result: Steam 2.0.2 and game 2.0.3 preserve the A/B setter and return layout while shifting the family by `0xC24`. The setter-level design remains viable; Batch 2 must prove the selected SafetyHook patch width/path and refuse unsafe fallback behavior.

### Batch 2 — Safe redesign

- Before installation, prove that the selected SafetyHook path uses a safe near `E9` patch that replaces only the complete 10-byte setter. If the library can fall back to an absolute patch that overwrites the A/B suffix, refuse installation or add an explicit safe guard before proceeding.
- Select hook placements only after both boundaries are established; do not hook directly on an unproven `RET` boundary.
- Resolve and decode both targets before installing either hook.
- Install A and B all-or-nothing; if the second installation fails, roll back the first.
- Preserve A's `ret` and B's `mov al, 1; ret` behavior.
- Keep letterbox aspect policy isolated from gameplay and FOV logic.
- Define shared aspect-state initialization, update owner, lifetime and synchronization assumptions before using it from hook contexts.

Validation: build the experimental ASI and run static rejection tests for zero matches, multiple matches, wrong decode, unsafe boundary and partial-install rollback. A successful build is not runtime evidence.

Batch 2 result: the redesigned experimental ASI builds successfully at `build-artifacts/test-asi/STALKER2ExperimentalLetterboxFix.asi`. The implementation creates both hooks disabled, requires a 10-byte original patch width before enabling either hook, writes the display aspect from the validated `RAX` context, skips the original setter only after a successful guarded write, and rolls back both hooks on failure. Manual runtime behavior remains unvalidated.

### Batch 3 — Manual runtime test

- Keep the stable gameplay ASI isolated and keep the previous crashing experimental binary disabled.
- Test the redesigned experimental ASI at the established `5120x1440` native resolution and `32:9` display aspect.
- Reproduce a new game/cutscene flow and verify cutscene aspect/letterbox behavior.
- Confirm no crash, no gameplay aspect regression, no FOV change and safe refusal on invalid installation conditions.
- Record the exact executable identity, module logs and reproduction result.

Validation: user-run manual runtime evidence only. Runtime success must not be inferred from build or load logs.

Batch 3 result: safety, load behavior and native cutscene render/aspect behavior passed in the tested flow, confirmed by NVIDIA/DLSS resolution logging. Cutscene FOV/projection behavior remains incorrect, so the experimental ASI is not release-ready and requires a new bounded analysis of the camera/projection state coupled to the A/B aspect transition.

### Batch 4 — Workspace archival

- Move reusable Ghidra scripts from the temporary C: workspace into the private workspace path `02-Research/Ghidra/ghidra-scripts`.
- Keep the current 2.0.3 Ghidra project in `E:\Work\Slaker2 mods\Dump\STALKER2-Ghidra`; do not create a second project location or overwrite the preserved analysis state.
- Move only required durable reports/evidence into the workspace if they are not already stored with the current Ghidra project.
- Record the final artifact paths in the task evidence and keep temporary runtime state separate from durable research artifacts.
- Inspect the temporary project copies and lock files after archival; remove them only as a separately confirmed cleanup scope and only after verifying that no required artifact remains exclusively on C:.

Validation: path/completeness check and read-only comparison of archived artifacts. No deletion is implied by this batch.

## Risks and rollback / safe failure

- Any uncertain hook boundary, ambiguous signature, decode mismatch, unknown executable identity or invalid context must cause refusal to install.
- No hook may be installed until both A and B have resolved and passed decode/boundary validation.
- If installation fails after the first hook, remove every installed experimental hook before reporting failure.
- If the redesigned module crashes, corrupts return behavior or changes gameplay/FOV behavior, disable it immediately and stop the phase.
- The stable gameplay ASI must remain usable without the experimental module.

## Stop conditions and phase gates

- Stop if the A/B targets cannot be tied to the analyzed executable identity.
- Stop if a safe hook boundary or exact return contract cannot be proven for either target.
- Stop if the redesign requires changes to stable gameplay ownership or a broad resolver refactor.
- Stop on any runtime crash or behavior outside cutscene aspect/letterbox scope.
- Do not reopen deferred weapon FOV research from this task.
- Advance to manual testing only after both targets validate and the experimental build is produced.
- Treat the task as closed only after manual testing, plan comparison and the required read-only Git review.

## Expected final review

After manual testing, inspect Git status and the relevant diff, compare all changed paths with this plan, and classify completed, remaining, deferred, blocked and not-runtime-validated items. Update `backlog/TASKLOG.md` only after that review and the manual test result are complete.
