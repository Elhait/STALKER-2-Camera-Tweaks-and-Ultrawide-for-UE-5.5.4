# Post-EXIT native blend data-flow

Date: 2026-11-11. Status: bounded static investigation complete; runtime producer/episode correlation remains unproven. Production recovery is unchanged.

## Scope and identities

Inspect the actual game EXE without loading/executing it, and compare its bounded native paths with the supplied weapon-handoff log. No game, debugger, Ghidra, compilation, harness, graphics fixture, installation or Git operation was performed.

- Actual EXE: `E:/Steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl/Stalker2/Binaries/Win64/Stalker2-Win64-Shipping.exe`.
- Size: 174,735,920 bytes; preferred image base: `0x140000000`.
- Independently computed SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`, matching the supplied runtime log.
- Runtime ASI identity: `8488120F244D377E9B39BFF8CFF941B6C6C106021B74D8AD75E3D62B5D89FC0C`.
- Preserved log: [weapon handoff](../evidence/cinematic-weapon-handoff-2026-11-11/STALKER2CameraTweaks.log).
- Native decode artifacts/helper: `E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/`.

Method: memory-map the PE read-only, verify SHA, read PE sections and exception-directory ranges, decode bounded instruction regions with bundled Capstone 5.0.6. Each isolated Python decoder had a 25-second supervisor timeout and was reaped before the next invocation. No decoder spawned children. Decoded leaf functions missing a `.pdata` range use a bounded neighborhood; instructions after their RET must not be attributed to that leaf. Contiguous executable-pointer tables are candidates until corroborated by constructor assignments. Call-byte/RIP-reference searches are bounded leads, not exhaustive semantic cross-reference proofs.

## 1. EXIT target: origin and limits

The actual EXIT-containing function starts at RVA `0x35C7CB2`:

1. Rejects when state byte `[RCX+0xB1]` is zero or `[RCX+0xAC]` already equals requested `DL`.
2. Stores the requested state in `+0xAC`.
3. Obtains a selector through a separate object/helper (`0xF7DD6E` reads a byte at object `+0xA0`). The semantic meaning of that selector is not established.
4. In the EXIT branch, obtains RBX through a global provider and its `+0xF8` pointer.
5. At `0x35C7D39`, loads `XMM0 = [RBX + RAX*4 + 0x38]`.
6. At `0x35C7D3F`, calls `0x6AFFF7C`, the same consumer reached by ENTER at `0x35C7DA1`.

The log records index zero, the same operand base `0x19938277D80`, and `90.3115` in both EXIT events. This is target read provenance, not proof of the configuration object's type or of who last wrote its element. The global provider is widely referenced; the bounded reference search did not establish an element writer. No weapon-mode interpretation is assigned to the selector.

## 2. Target reaches an actual FOV blend state

The current image establishes this chain:

```text
EXIT callsite 0x35C7D3F, XMM0 target
  -> 0x6AFFF7C: preserves target, resolves provider +0x100
  -> tail call 0x30DB9B8 with target in XMM1
  -> provider +0x2CF8 supplies/receives blend component S
  -> 0x30DBCD2 initializes target/start/remaining state
```

The component creation path `0x30DBB71 -> 0x30DBBFE` references the UTF-16 class string `FOVBlendComponent` at RVA `0x9571C8E`. Its constructor writes the primary vtable at `0x851B290` and secondary tick-context table at `0x851B560`. The latter points to `0xCCE4FA`. This connects target arming and tick by object construction, rather than only by similar offsets.

At `0x30DBCD2`, with RCX = component S:

| Field | Established operation | Interpretation limit |
|---|---|---|
| `S+0x54` | Stores target from XMM1 | Target role established; no native/transformed-space tag |
| `S+0x50` | Loads provider field `+0x2CE8` | Tick uses this as remaining time; provider value serves duration |
| `S+0x58` | Reads resolved camera `+0x230` | Initial source-camera FOV, potentially already influenced by prior intervention |
| `S+0x40` | Records camera weak-reference representation | Must resolve/validate receiver, not compare wrapper address to camera address |
| `S+0x48` | Records settings/provider weak reference | Supplies duration/easing; exact reflected owner type not established |
| `S+0x28` | Secondary tick context T | `T = S + 0x28` |

Historical reports called provider `+0x2CE8` a PCM-side FOV field. In this current-image tick it is used as a duration divisor, while the captured copy is decremented by delta time. The historical label is not imported into this image's contract.

## 3. Tick is an interpolation producer, and contains our Dialogue hook

At tick RVA `0xCCE4FA`, entry RCX becomes RSI = T. The code resolves a provider using `T-0x28`, then:

- decrements `T+0x28` (S+0x50) by incoming delta time;
- computes/clamps progress from remaining time and provider `+0x2CE8`;
- selects interpolation behavior using provider byte `+0x2CEC`, with parameter `+0x2CF0` in relevant branches;
- computes a blend from `T+0x30` (S+0x58, start) to `T+0x2C` (S+0x54, target); the function contains linear and eased paths, plus special branches;
- resolves the camera receiver from the weak reference at T+0x18;
- at `0xCCE670`, copies XMM6 to XMM1;
- at `0xCCE673`, invokes `[receiver.vtable+0x608]`;
- compares the calculated XMM6 with the stored target; equality can reach a completion delegate through T+0x08 and virtual slot `+0x60`.

This is the exact current production Dialogue hook RVA recorded in the log. It is a generic FOV blend output boundary, not evidence that the mod's Dialogue policy accepted or transformed the sample.

The target-arming callback clears provider `+0x2CF8` at `0x31FD92E`. This establishes a native cleanup path. Reaching it for the recorded transition, and the complete behavior of every callback/special branch, are not runtime-proven here. Do not replace native completion with a guessed elapsed-time timeout.

## 4. Data-flow to +0x230 and the validated writer

Two concrete camera setter paths are available in current-image vtables:

- Base table `0x86588A0`, constructor assignment at `0x32367F2`: slot `+0x608` resolves to `0x1F88EDC`, which stores XMM1 directly to camera `+0x230` and returns.
- Derived table `0x8492160`, constructor assignment reference at `0x32362D6`: slot `+0x608` resolves to `0x5353CC8`, which stores XMM1 to `+0x230` and computes camera parameter `+0xC7C` from it and optical fields.
- The derived GetCameraView path `0x53540F6` first calls `0x3236604`. That function evaluates FOV through `0x3236740`, stores it into `+0x230`, and then the caller invokes the validated writer-containing function `0x9CD16C`.
- The actual writer hook at `0x9CD2EB` consumes `[RSI+0x230]` into output `[RBX+0x30]`; this boundary itself is not the interpolation owner.

This establishes concrete static paths from blend output to camera FOV and onward to a validated writer. The recorded runtime receiver vtable is absent, so it does not select which of these setter paths produced the supplied writer stream. Other subclasses/overrides and writers are not exhaustively excluded.

## 5. What the supplied log cannot correlate

`TraceDialogueBoundary` records RSI as its callback source. In the validated tick this is T, the blend context. The writer's RSI is the camera source. Their differing pointer values do not demonstrate different cameras or competing owners.

At the problematic EXIT the log has both:

- camera writer RSI `0x19C1C385A40`, receiving `70 -> 90.3115`;
- Dialogue-boundary RSI `0x19AF967F368`, just after EXIT, labelled `CandidateObservation`.

The ordinary record omits the boundary's RCX camera receiver, receiver vtable, stored start/target/remaining values, and effective interpolation settings. Consequently it cannot bind that blend instance to this writer camera, prove it was armed by this EXIT, or establish that the native blend formula emitted the exact recorded stream. Absence of accepted Dialogue intervention remains evidence against blaming the mod's Dialogue correction, but does not exclude the generic native blend path.

## 6. FOV space and the two trajectories

The native blend does not attach a native/transformed tag to its floats. It captures the camera's current FOV as start. Therefore the same mechanism can in principle interpolate from a native start (70) or an already modified start (106.688/126.87). This is a data-flow possibility, not a demonstrated attribution of both runtime scenarios to this component.

The current weapon-handoff log establishes native baseline 70 before ENTER, cinematic output 108.941, EXIT target 90.3115, first post-EXIT writer input 70, then gradual arrival at the target. The source correction was applied at the writer register, while its input field remained 70 in the recorded cinematic samples. This supports a captured-native-start hypothesis.

The existing recovery test's `106.688 -> 90` is a fixture for the accepted old recovery contract. An independent historical 2.0.5 runtime report records `126.87 -> 90`. Neither establishes current-image blend-instance/receiver/progress identity for the old path. It would be inaccurate to claim that native call paths have already been compared for two current-game runtime captures.

## 7. Acceptance questions

| Question | Current answer |
|---|---|
| Which transition is active? | A concrete native FOVBlendComponent path is identified statically. Its instance and EXIT episode are not joined to the recorded camera at runtime. |
| Which FOV space is the sample in? | No explicit native tag found. Need start/target provenance plus the actual blend state; float range or same source/generation alone is insufficient. |
| When does the native transition complete? | Tick compares generated value with target and can dispatch cleanup; exact completion for the supplied episode is unobserved. Mod recovery reaching endpoint is not proof of native cleanup. |

## 8. Minimal next phase, without changing recovery

1. Reuse the existing validated Dialogue boundary for bounded observation. Capture T/S identity, RCX receiver and vtable, raw XMM6/XMM1, readable start/target/remaining, and validated duration/easing-provider context. Keep native raw values before any Dialogue intervention separate from final modified arguments.
2. Associate with the current writer camera by actual receiver identity, and with an EXIT by concrete target-arming/instance evidence. Same target or temporal proximity alone is not the admission contract. Establish how to capture arming identity using the current validated call chain before preparing an artifact; do not promote the RVAs above to production gates.
3. Record instance/target changes and bounded first/last samples and extrema; a short controlled intermediate sample budget must allow formula/progress comparison without per-frame file output.
4. Exercise the weapon-handoff path and a known working post-EXIT path. Test the same native state/formula hypothesis first; no requirement for two distinct camera modes.
5. Only after provenance is established, design the correction. A possible model maps confirmed native blend progress between the last rendered cinematic output and the transformed native destination, instead of HorPlus-transforming an untagged intermediate value. This remains a proposal; endpoint ownership, cancellation, interruptions and ambiguous observations must be specified first.

No diagnostic artifact or production repair was built. No new runtime run is requested until the observation/arming join is ready. Existing ASI and evidence are preserved.
