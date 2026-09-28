# Frame Generation compatibility: confirmed ResizeBuffers hook cycle

## Scope and disposition

Read-only investigation requested on 2026-09-28. No production source/configuration changes, builds, ASI replacement, game launches, Git mutations, release or packaging actions were performed. The investigation inspected the user's already-running, hung game and preserved its logs and a diagnostic snapshot. The later user update broadened the evidence from MFG-unlock startup termination to a hang after enabling native RTX 40-series DLSS Frame Generation ×2 without the unlock addon. The user subsequently confirmed that the missing MFG addon need not be reinstalled to reproduce the problem.

**Confirmed local mechanism:** the active `ResizeBuffers` forwarding graph contains a cycle through Camera Tweaks, system DXGI, Steam Overlay and RTSS. Camera Tweaks' captured original is an address in system DXGI, not an immutable continuation independent of subsequent foreign hook topology. Steam's active downstream continuation returns to Camera Tweaks' own callback body. Native resize therefore does not complete; repeated re-entry reaches RTSS's spin-wait path.

**Not established:** the exact writer/order that formed this graph; the mechanism of the independent `DLSSG_For_SM86` v0.3.5 startup termination; compatibility or incompatibility of every FG implementation. A presence-based DLL accusation, device-loss diagnosis, or transfer of the previous E_ABORT investigation is not justified.

## Evidence identity and acquisition

- Actual game PID: **23136**, parent launcher PID **21416**. Both were created at 13:50:07 local time (UTC+3). Child image: `Stalker2/Binaries/Win64/Stalker2-Win64-Shipping.exe`.
- Loaded canonical mod: `STALKER2CameraTweaks.asi`, SHA-256 `6C76FF71D530A362B2B14F3FC1BE5EBD9D30D899495206FF885DC7258452347F`. The installed binary and repository-root binary matched. PE timestamp: 2026-09-28 04:04:42.
- Game SHA-256 in the same session's core log: `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- Diagnostics.Enabled=true in that run. Startup journal v=1 records the same PID and completes through first Overlay submission.
- Local `dxgi.dll` is **ReShade 6.8.0.2155**, not the independent reporter's SM86 proxy. Local `d3d12.dll` is **OptiScaler 0.9.5.4**. Ultimate ASI Loader is 9.7.4. UE4SS injection proxy, Achievement Enabler, Steam Overlay, RTSS, `renodx-dlss5.addon64` and `renodx-ue-extended.addon64` are also present. This inventory describes the tested stack; it does not assign blame by module presence.
- The current addon inventory and loaded-module list contain no `renodx-mfgunlock.addon64`. `ReShade.ini` retaining a MFG Unlock settings section is not evidence that the addon was loaded.
- Loaded Streamline interposer is the game's plugin-directory `sl.interposer.dll`, version 2.9.0.0. Native DLSS-G provider/tracker threads are present. GPU reported by ReShade: RTX 4080, driver 616.56. No simplification of that graphics stack was performed.

Raw evidence is retained under the ignored `build-artifacts/fg-compatibility-20260928/` directory:

- `initial-native-fg-135801/`: copies of the three Camera Tweaks logs, ReShade logs/config, OptiScaler config and mod INI, taken before debugger inspection.
- `native-fg-hang-stacks-2.txt`: module inventory and all-thread snapshot, including the repeating GameThread stack.
- `native-fg-recursion-detail.txt`, `native-fg-original-target.txt`, `native-fg-chain-proof.txt`, `native-fg-complete-chain.txt`, `native-fg-steam-bridge.txt`, `native-fg-closed-cycle.txt`, `native-fg-spin-proof.txt`: disassembly and exact pointer evidence.
- `native-fg-hang.dmp`: a successfully written `/mFhut` **minidump snapshot**, not a full-memory dump or an exception-triggered crash dump. Live-read transcripts preserve relay/heap data that this minidump may not include.
- `GameUserSettings-current.ini`, `Stalker2_2-current.log`: settings snapshot and actual child UE log. The generic `Stalker2.log` belongs to the launcher and closes early; it must not be substituted for the child log.

Acquisition caveat: the first successful non-invasive debugger attach had an invalid initial command sequence and exited without the intended explicit detach. One residual diagnostic suspend count was detected. It was removed with a non-suspending attach and one resume per thread, followed by quit-and-detach. Process CPU subsequently advanced while the resize remained unresolved. Intermediate repeated snapshots are therefore inspections of the preserved stopped state, **not independent reproductions or proof of continuous progress**. The hang and the final resize log already preceded debugger attachment. The game was not terminated by the investigator.

## Controlled evidence and chronology

The initial requested separate startup A/B with the unlock addon was **not newly executed**: the user had already provided a live native-FG failure, and the unlock addon was absent. The captured session provides a successful startup followed by a later FG-associated swapchain replacement/hang. The user's report supplies the native ×2 toggle association; existing logs do not timestamp the UI toggle itself. This is not presented as two investigator-launched, instrumented startup runs.

| Stage | Confirmed observation in PID 23136 | Boundary |
| --- | --- | --- |
| Early arm/bootstrap | Export observation, seed-factory bootstrap and shared-table discovery all ready | No bootstrap failure in this session |
| Core initialization | Gameplay/Cinematics/Dialogue AVAILABLE at 13:50:08.466 | Core initialized successfully |
| Initial real chain A | `CreateSwapChainForHwnd` succeeds at about 13:50:21; native identity `0x14a0f1d4190` | Actual device/queue association accepted |
| Initial native resize | Resize sequences 1, 2, 3 return S_OK; about 6.00, 13.35, 10.86 ms native time | Resize path worked before the later replacement |
| Activation | Two successful Presents; renderer/input active at 13:50:34.468; first Overlay frame at 13:50:35.447 | Early observation and delayed activation succeeded |
| Later chain B | ReShade creates a replacement at 13:55:32.785; Camera Tweaks releases old ownership, native creation returns S_OK, accepts new identity `0x14a2abd9c10` | Not a failed factory creation |
| Last confirmed valid B state | New chain has matching device/queue, process HWND and 2 buffers; pending target accepted | Same device `0x14adbae1d80`, different actual queue `0x14a2a402230`; this change alone is not a defect |
| First non-completing boundary | ReShade enters ResizeBuffers at 13:55:32.855; Camera Tweaks logs resize sequence 4, then no END | Original call does not return; no later B Present/activation is established |
| Debugger evidence | GameThread has repeated Camera Tweaks/RTSS ResizeBuffers frames and a fully closed downstream graph | Concrete hook re-entry mechanism, rather than inference from missing log lines |

The last good shared lifecycle is successful real-chain discovery/association and, on chain A, native resize plus renderer activation. The first structural change visible after the working phase is **replacement creation**; replacement itself succeeds and is not declared faulty. The first confirmed failure is the subsequent **non-returning ResizeBuffers dispatch** on B. Existing telemetry cannot identify the earliest individual hook write between the working topology and this graph.

## Exact forwarding graph

All addresses below belong to PID 23136 only; they are evidence anchors, not production patch locations. `ResizeBuffers` is SDK slot 13 (byte offset 0x68).

1. Chain B object `0x14a2abd9c10` uses system DXGI table `0x7ffaa5b1d688`. Its slot 13 contains Camera Tweaks `0x7ffa5e98b990` (ASI RVA **0x5b990**).
2. The registry record observed on the callback stack is `0x14a100d1690`. Its saved-original vector at `0x14a08d99670` has slot 13 = **`0x7ffaa5a87180`**, `dxgi!CDXGISwapChain::ResizeBuffers`.
3. Camera Tweaks callback disassembly loads vector slot 13 and invokes it via `call rbx` at ASI RVA **0x5baa1**, return address **0x5baa3**. Reconstructed nonvolatile RBX at that frame is `0x7ffaa5a87180`.
4. That system DXGI address currently begins with a JMP to `0x7ffa2867050c`; the relay jumps to **Steam Overlay `0x7ff9a5674250`**.
5. Steam's entry is itself redirected through `0x7ff9656602f8` to **RTSS RVA 0x73690**. RTSS's indirect original at `0x1802404f0` is `0x7ff9656602a0`.
6. That RTSS continuation executes the relocated Steam prologue and resumes at **Steam `0x7ff9a5674255`**. Steam later loads its downstream pointer from **`0x7ff9a5747348`** and tail-jumps to it.
7. The downstream pointer is **`0x7ffa28670ac0`**. It executes relocated Camera Tweaks prologue bytes, then jumps into **Camera Tweaks RVA 0x5b997**: the body of the same ResizeBuffers callback. Its saved native address is still step 2, closing the cycle.

```text
Camera Tweaks ResizeBuffers body
  -> captured system DXGI method address
  -> Steam entry / RTSS detour
  -> RTSS saved Steam continuation
  -> Steam downstream continuation
  -> Camera Tweaks ResizeBuffers body
  -> ...
```

The Camera Tweaks callback entry is also redirected to the same Steam hook entry. The graph is proved by code and pointer reads, not solely by DLL inventory or symbol names. Nearby exported names displayed for vendor modules are not assumed to identify source functions; exact disassembly and slot/argument ABI provide the method identity.

RTSS's observed re-entry path uses a `lock bts` on its guard and loops through GetTickCount, with a **5000 ms** comparison before proceeding. The guard read was set. Therefore the captured symptom is recursive hook re-entry with repeated bounded RTSS spin-waits, **not proof of a permanent mutex deadlock, observed stack overflow, GPU timeout, or process termination**. Unwound stacks show many repeating frames; this inspection did not observe their eventual exit disposition.

## Current-source causal boundary

- `src/overlay/dxgi_hook_registry.hpp:76`–84 copies the current method-address vector. `Original` at line 42 returns an address from that immutable vector. It does **not** freeze the executable bytes or downstream chain at that address.
- Slot publication at lines 109–114 publishes saved records before patching table entries. That protects callback/original availability, but cannot guarantee that another hook manager will keep its downstream continuation acyclic.
- `src/overlay/discovery_runtime.cpp:1329` installs swapchain callbacks; slot 13 is replaced at line 1353. The observed table and callback disassembly match this mechanism.
- `HookResizeBuffers` at line 1130 retrieves slot 13 and calls it at line **1146**. The observed return RVA 0x5baa3 maps to that invocation by vector index, six forwarded parameters and instruction sequence, without rebuilding/replacing the binary for symbols.
- `BeginResizeObservation` at lines 1033–1037 and `DxgiResizeNesting` suppress duplicate optional setup for legitimate nested resize calls. **They do not change native forwarding on re-entry.** Every repeated callback still invokes the same captured address, so nesting does not break this graph.
- FS-04's actual callback fixtures at `tests/overlay/dxgi_callback_harness.cpp:304`–338 cover finite ResizeBuffers1 -> ResizeBuffers nesting and inner-failure/outer-success disposition. They do not model a captured method whose foreign continuation routes back to the same callback.
- The original call deliberately remains outside optional-work exception handlers. No C++ exception has to occur for this cycle to hang the native path. Disabling renderer/UI work alone would not repair the forwarding graph. A manufactured HRESULT or skipped native call is not equivalent to preserving native behavior.
- Chain B is still pending: two-successful-Present renderer/input activation has not been demonstrated for it. The native resize cycle occurs before any established B renderer GPU work. Chain A did already render; this does not prove all GPU involvement irrelevant, but the demonstrated failure does not require a malformed new-chain submission explanation.

**Broken safety assumption:** an original method pointer captured from a shared COM table remains a downstream native call that cannot re-enter our own interception after foreign managers capture/redirect subsequent dispatch targets. This stack violates that assumption while the record vector itself remains intact. There is no evidence here of cross-device resource use, invalid queue selection, null saved original, or a publication race.

## Independent SM86 report and limits

The independent user's reported A/B isolates adding `DLSSG_For_SM86` v0.3.5 under an alternate dxgi.dll basename. Their included startup journal establishes bootstrap readiness, real factory observation, core readiness and successful first CreateSwapChainForHwnd. The provided excerpt does not unambiguously label whether it belongs to the failing FG-enabled run or the working run with absent Overlay. Its ending at SWAPCHAIN_FIRST_CREATED cannot prove no pending target: subsequent journal markers can still be buffered. No exception/termination stack accompanies that excerpt.

The local environment differs: its dxgi.dll is ReShade, its native FG hang includes Steam/RTSS, and the unlock addon is absent. **The local cycle is a confirmed production compatibility defect in Camera Tweaks' forwarding boundary, but is not automatically the root cause of the independent user's silent startup exit.** No assertion is made that all DLSS/FSR/XeSS FG paths fail, or that RTX architecture unlocking itself caused the cycle.

Primary project descriptions were checked to distinguish components, not used as runtime proof: [DLSSG for SM86 README](https://github.com/sdli1995/dlssg_for_sm86/blob/main/README.en.md), [MFG Ada Unlock / RenoDX](https://github.com/mavismmg/MFGAdaUnlock-RenoDx).

## Next causal gate; no repair in this batch

The local hang has a sufficiently concrete mechanism for a bounded repair discussion: saved-native continuation and foreign same-method re-entry must remain acyclic without changing forwarded arguments/results or bypassing the actual wrapper path. This is **not** authorization to add a bypass, return synthetic failure, change hook architecture, or disable Steam/RTSS as a production workaround.

If startup termination still needs to be tied to this mechanism, capture that exact run from before failure with first-chance exception and termination observation; compare its saved-original/downstream graph at the first non-returning boundary. If formation order remains material to choosing a repair, capture writes to the demonstrated Steam downstream continuation and the relevant native/callback entry points. Existing source evidence and the final graph do not identify the writer's call stack or exact installation order.

Scope results: local native-FG hang boundary and minimal call-cycle mechanism **confirmed**; diagnostic evidence preserved; production/core/FS repairs untouched; independent startup-termination mechanism and independently executed startup A/B **incomplete**. No game/config/module changes were made to produce a PASS workaround.
