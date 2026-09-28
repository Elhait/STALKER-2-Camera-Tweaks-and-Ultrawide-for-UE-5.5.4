# Overlay/DXGI startup crash investigation and repair history

Date: 2026-09-27. This report preserves the investigation path behind the v2.0 Overlay startup failures and the subsequent early-DXGI lifecycle repair. It is an engineering history, not a compatibility guarantee or a claim that every reported crash signature had one proven cause.

## Summary

After v2.0, users reported startup failures with different visible signatures: D3D12 `PresentInternal` failures including `E_ABORT`, GPU crash/device-removed reports, and access violations. A controlled diagnostic sequence separated Overlay GPU submission from DXGI interception. Disabling Overlay GPU work while retaining DXGI hooks did not stop the reproduced failure; disabling the full Overlay/DXGI path did, while the camera core remained available in the user's test environment. This localized the reproducible failure to the Overlay/DXGI integration boundary, but did not identify one exact native instruction or driver fault as the universal root cause.

The first safety-oriented DXGI repair stopped the observed startup crash in a candidate run but also left the Overlay unavailable. Runtime instrumentation then showed that the initial factory hooks were installed, yet the relevant factory/swapchain creation and Present path was not observed. Synchronously arming factory observation before camera-core initialization, while withholding renderer/input activation until the core and presentation target were ready, restored the observed Overlay path. The user's candidate runtime logs show successful renderer initialization, input-hook installation, first frame and repeated visibility toggles, with no crash in that run.

The demonstrated lifecycle issue was a startup-order/discovery gap: observation was not active early enough to see the factory path needed to discover the effective swapchain. The exact native mechanism behind every earlier `DEVICE_REMOVED`, `E_ABORT` or access-violation report remains unproven. The successful diagnostic-candidate run is not runtime validation of the separately built production artifact or every graphics-wrapper/driver combination.

## Incident and evidence timeline

### 1. Reports after v2.0

Users reported that v1.0 worked but v2.0 could crash during startup. The evidence arrived in several forms, including a null-address access violation, a D3D12 `PresentInternal(SyncInterval)` failure with `0x80004004` (`E_ABORT`), GPU crash/DRED output and a D3D12 viewport failure with `0x80070005`. These reports are preserved as separate observations; similar timing does not establish a common cause.

Some affected setups included third-party graphics components such as OptiScaler, ReShade/NR, Frame Generation or RenoDX. The presence of such components was context for the experiments, not proof that any one component caused the failures. In one controlled comparison the user kept the external graphics setup in place while changing only the mod's Overlay/DXGI path.

### 2. Debugger and dump attempts

The investigation used user-collected WinDbg output and attempted minidumps while the game was still running after the D3D12 fatal Present report. The observed first-chance access violation during unwind/debugger activity and the later `RHIThread` termination were not treated as evidence that the Overlay caused those debugger events. The dump was incomplete in at least one attempt because `ReadProcessMemory` could not read part of the process memory. Consequently, these debugger artifacts did not provide a symbolized native call chain that uniquely attributed the original GPU failure to a specific Overlay command or hook.

The useful result of this phase was operational: preserving the process long enough to collect thread stacks/logs produced more evidence than treating the first fatal Present line as a complete diagnosis. It did not, by itself, identify the root cause.

### 3. Controlled Overlay/DXGI isolation

The diagnostic experiments were interpreted as follows:

| Experiment | Observed result | What it supports | What it does not prove |
| --- | --- | --- | --- |
| Overlay GPU submission disabled; DXGI hooks retained | The startup failure still reproduced. | Overlay draw submission was not required for that reproduced failure; DXGI interception remained in the causal region. | It does not prove every GPU-side Overlay effect irrelevant, nor identify which DXGI hook/interleaving caused the failure. |
| All Overlay/DXGI hooks disabled; Gameplay/Cinematics/Dialogue core retained | The user reported that the game launched and continued to work; those camera features remained `AVAILABLE`. | Strong A/B evidence that the reproduced failure depended on the Overlay/DXGI integration path in that environment, rather than camera-core initialization. | It does not establish which hook or callback was responsible or prove universal independence on every machine. |
| Early-armed candidate with the full Overlay path restored | No startup crash was reported; the Overlay reached its visible runtime path. | The revised lifecycle worked in the observed environment and addressed the missing-discovery condition. | It does not prove the original native GPU fault had a single mechanism or that all wrappers/drivers are supported. |

The user reported retaining the external graphics stack for the isolation comparison. This makes the A/B useful because the mod's DXGI interception was the controlled variable; exact machine configuration and binary identities are not fully established in this history, so the result remains scoped to that run.

### 4. Safety repair and first non-crashing candidate

The bounded DXGI safety repair addressed concrete ownership and callback contracts found in source: SDK-correct callback ABI, publication of hook originals before callbacks could use them, synchronized table-owned callback records/leases, native pass-through of arguments and results, matching-swapchain resource release before resize/replacement, `ResizeBuffers1` handling, and bounded GPU waits with Overlay-local failure disposition. Additional adversarial review found other concrete backend/lifetime failure paths; those findings and their test boundaries are detailed in [the Overlay adversarial crash-safety audit](OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md).

The initial repaired candidate did not crash in the user's run, but the Overlay did not appear and there was no startup hint. This was not considered a successful product outcome. The investigation was split into two separate questions: native stability and successful discovery/activation of the intended presentation target.

### 5. Instrumentation found the discovery gap

Factory caller/table instrumentation showed that the observed factory vtable slots contained this module's hook addresses. That evidence ruled out a later slot overwrite for those particular observed factory objects. However, the candidate log contained no relevant `FACTORY_METHOD_INVOKED`, `SWAPCHAIN_DISCOVERED`, Present observation, input-hook installation or startup hint. A caller module basename of `dxgi.dll` was observed; the basename alone does not identify the exact module file or establish wrapper ownership.

The concrete remaining defect was therefore not “the hook was overwritten” on the observed table. The observation path had not seen the factory/swapchain creation that would lead to the active Present path. The bounded hypothesis was that factory observation was armed too late relative to the application's initial DXGI factory creation. The repair moved factory observation earlier, not renderer/GPU work: renderer and input stayed gated on camera-core readiness and validated presentation evidence.

### 6. Successful early-arm candidate sequence

The user-provided `STALKER2CameraTweaksOverlay.log` contains this order and evidence:

1. `DXGI_HOOKS_INSTALLED` records successful installation of the three factory-export hooks.
2. `OVERLAY_CAMERA_CORE_READY` records the completion of camera-core initialization; this follows factory-hook arming.
3. Factory creation and `CreateSwapChainForHwnd` are observed. The logged caller for the relevant swapchain call is `d3d12.dll`.
4. The candidate swapchain is associated with the process window (`4320×1440`, six buffers) and a `DIRECT` queue; the code's device/queue/window checks accept it. The target remains pending rather than immediately starting GPU work.
5. A successful native Present and the required repeated successful-Present evidence validate the target. Activation is recorded as `OVERLAY_RENDER_TARGET_ACTIVATED reason=two_successful_presents`.
6. The log then records `OVERLAY_RENDER_INIT_OK`, `OVERLAY_INPUT_HOOK_OK key=Delete`, `OVERLAY_STARTUP_HINT_QUEUED key=Delete` and `OVERLAY_FIRST_FRAME`.
7. Later `OVERLAY_VISIBILITY=VISIBLE/HIDDEN` transitions confirm that the runtime input/visibility path operated, not merely that initialization returned success.

The paired core log records `Gameplay=AVAILABLE`, `Cinematics=AVAILABLE`, `Dialogue=AVAILABLE` and `Hotkeys=AVAILABLE`. These records establish the observed candidate session; they do not identify the ASI by a verified release hash in this report.

## Production lifecycle after the investigation

The production startup path uses the same combined lifecycle: optional factory observation is synchronously armed before camera-core initialization; the renderer/input path waits for the explicit core-ready signal and a validated stable target. A DXGI/Overlay startup failure remains optional and cannot prevent the camera core from initializing. The production build compiles this shared path; caller-module and vtable-slot trace instrumentation is diagnostic-profile-only, while repeated discovery/association detail is controlled by `Diagnostics.Enabled`.

The production artifact was built separately under `build-artifacts/production-lifecycle/` and the offline suite passed 51/51 harnesses. The game was not launched with that separately built production artifact during that validation pass. Thus the diagnostic-candidate runtime success and production offline build are distinct evidence; runtime confirmation of the production artifact remains a separate step.

## Root-cause disposition

- **Confirmed:** the first non-crashing candidate could not activate the Overlay because the relevant factory/swapchain creation and Present path was not observed; early factory observation captured the path and the candidate subsequently initialized/rendered the Overlay.
- **Strongly supported for the reproduced startup failure:** the problematic integration boundary was the mod's Overlay/DXGI interception path. Disabling only Overlay GPU submission did not suffice; disabling all Overlay/DXGI hooks did.
- **Not established:** a single concrete native GPU/driver cause for all reported `E_ABORT`, device-removal/hang and access-violation failures; a particular third-party injector as the universal cause; universal compatibility of the revised lifecycle; runtime behavior of the production artifact on every wrapper/driver stack.

No delay, timer or arbitrary startup sleep was used as the fix. Readiness is evidence-based: camera core ready, validated swapchain/device/queue/window association, and successful presentation observations before renderer/input activation.

## Engineering takeaways

- Treat “game no longer crashes” and “optional UI is actually active” as separate acceptance conditions.
- Change one controlled variable per A/B experiment; preserve the camera core and external graphics stack when testing the Overlay boundary.
- A missing event in a log is meaningful only within the validated observation surface. No factory callback does not by itself prove a specific wrapper replaced a vtable.
- Keep discovery early and passive, then gate GPU work on explicit readiness evidence. Early hook installation must not imply early renderer activation.
- Keep exact root-cause claims narrower than the evidence: a subsystem-level A/B localization is valuable even when a device-removal instruction-level cause remains unknown.
- Keep diagnostic-candidate runtime, deterministic offline tests and production-artifact runtime as separate evidence classes.

## Related records

- [DXGI startup safety repair plan](../plans/DXGI_STARTUP_SAFETY_REPAIR.md)
- [Adversarial Overlay crash-safety audit](OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md)
- [Production architecture](../../docs/ARCHITECTURE.md)
- [Production safety invariants](../../docs/SAFETY_INVARIANTS.md)
