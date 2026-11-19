# U2 FOV provenance chain — 2026-11-15

This note extends the bounded writer-site inventory. It separates the native camera field from the view/output field and records the evidence level at each edge. Analysis is read-only; no production code or FOV policy changed.

## Chain

```text
Native FOV-producing/evaluating code
    → native camera object +0x230
    → primary GetCameraView copy site 0x9CD2EB
    → view/output object +0x30
```

| Link | Current evidence | Status / limits |
|---|---|---|
| Derived-camera FOV evaluator → native `+0x230` | Same-image report identifies derived GetCameraView path at RVA `0x53540F6`, which calls `0x3236604`; evaluator `0x3236740` computes FOV and caller stores it to camera `+0x230`. | Static path confirmed for this derived path. This does not select the runtime camera subtype for every logged transaction. |
| Base camera setter → native `+0x230` | Base vtable table `0x86588A0`, slot `+0x608`, resolves to `0x1F88EDC`, which stores XMM1 to `[RCX+0x230]`. | Setter identity and vtable slot confirmed statically. Runtime call/reachability for the logged receiver is unknown. |
| Derived camera setter → native `+0x230` | Derived vtable table `0x8492160`, slot `+0x608`, resolves to `0x5353CC8`; it stores XMM1 to `[RCX+0x230]` and derives camera parameter `+0xC7C` from the FOV and optical fields. | Setter identity and vtable slot confirmed statically. Runtime call/reachability for the logged receiver is unknown. |
| Native camera `+0x230` → primary writer input | The validated hook at `0x9CD2EB` reads `[RSI+0x230]`. Runtime log records RSI source pointer `0x17a3b01f340` for its observed stream. | Confirmed at the hooked site for the capture. Other copy/read paths are not excluded. |
| Primary writer decision → output `+0x30` | Hook stores its resulting XMM0 to `[RBX+0x30]`; runtime diagnostics correlate BEFORE, DECISION and POST_STORE for 5,573 calls with no readback mismatch/drop in that capture. | Confirmed for the instrumented site and target address observed there. Not proof of rendered FOV or exclusive ownership afterward. |
| View/output `+0x30` → rendered view | No direct render/presentation measurement in this evidence set. | Unknown. |

## Native evaluator input analysis (`0x3236740`)

The pinned-image disassembly at `02-Research/evidence/post-exit-dataflow-2026-11-11/fov-evaluation.txt` resolves the derived update as:

```text
0x53540F6 GetCameraView
  -> 0x3236604
       -> 0x3236740(camera, EDX=0)
       -> store returned XMM0 to [camera+0x230]
```

In this call form, evaluator `0x3236740` reads camera fields `+0xC7C`, `+0xBE4`, and `+0xC0C`; a conditional branch also uses `+0xC78` and `+0xBE8`. Its alternate nonzero-DL branch reads `+0x25C`, but the observed static caller `0x3236604` explicitly clears EDX before calling, so that branch is not selected on this derived GetCameraView path. The evaluator does not directly read `+0x234`, output/view `+0x30`, or the writer aspect field `+0x254`. The current evidence does not assign semantic names to all the `+0xBxx/+0xCxx` inputs, so `+0xC0C` must not be called an aspect ratio without additional proof.

The same-image constructor at `0x32367E4` initializes both `[object+0x230]` and `[object+0x234]` to `90.0`; that is default object initialization, not proof that either field directly represents the player's current UI setting. At runtime, the supplied log labels them `cameraWorldFov(+0x230)` and `cameraFirstPersonFov(+0x234)`, but those labels alone do not establish their full semantics.

The derived setter `0x5353CC8` stores its incoming XMM1 at `+0x230` and computes `+0xC7C` from that FOV argument and optical fields. Thus a setter call can affect a subsequent evaluator result through `+0xC7C`, even though the evaluator has no direct dependency on output `+0x30` or `+0x234`. The mod's current Dialogue boundary callback can conditionally replace XMM1 for selected lifecycle samples; static evidence therefore permits an indirect mod-to-optical-state-to-evaluator path on that setter route. It does not establish that this path ran for the runtime receiver that first showed `120°`, or that it explains that value.

### Assessment of the `90° → 120°` hypothesis

- **Possible in principle:** the game computes native `+0x230` from optical/camera state before the primary writer, so the value need not equal the separate `+0x234` field.
- **Not established as a direct conversion:** evaluator `0x3236740` does not read `+0x234`, and the inspected derived call has no explicit aspect-field input. The proposed `90° → 120°` aspect conversion is therefore not supported by this function's direct dataflow.
- **Origin of `120°` remains unknown:** the runtime log does not record evaluator inputs `+0xBE4`, `+0xC0C`, `+0xC7C`, `+0xC78`, or `+0xBE8`, nor receiver vtable/call identity. It cannot tell which combination produced 120.
- **The `120°` correction-off sample is pre-save menu state, not Gameplay evidence:** the user clarified that the first `120°` sample in the control log occurred in the main menu before loading the save, after changing the FOV value there. It must not be used to infer the saved game's native Gameplay FOV or the origin of `120°` during an in-save session. The later `90.3115°` sample follows an aspect/state change, but the log has no explicit save-load marker to classify it independently.
- **Dialogue setter influence is now strongly supported across captures:** with Dialogue `Reduced` active, the earlier capture recorded native/writer input around `80.15°` and corrected output `118.59°–122.81°`; with Dialogue correction off, the control callback recorded `selectedPolicy=OFF`, target `70°`, and the active Dialogue writer later observed native `+0x230=70°` pass-through. This aligns with the code path where Dialogue replaces a blend sample before the derived setter, which writes `+0x230` and updates evaluator input `+0xC7C`. However, neither capture pairs the exact Dialogue setter `XMM1`/receiver vtable with the primary writer receiver, so causal runtime dispatch through `0x5353CC8` remains unconfirmed.

This supported Dialogue setter route is distinct from direct feedback from output/view `+0x30`. The control capture's provenance, counts and remaining limits are recorded in `post-process-observer-implementation.md`, section “FOV-correction-off control capture.”

No FOV or recovery behavior was changed for that control capture. Although the coordinator labels many records `GameplayEligible`, the first `120°` sample was in the main menu before save load. That capture does **not** establish a correction-off Gameplay FOV interval because Gameplay FOV correction remained disabled throughout. A later two-pass capture now provides a paired sample at `90.3115°` with Gameplay `AspectRecalculation` versus `RuntimeCombine`; see `post-process-observer-implementation.md`, “Two-pass Gameplay-mode comparison — 2026-11-15.” The `120°` switch-time sample remains excluded from that pairing. The correction-off capture still provides a Dialogue observation; evaluator inputs, full native writer coverage, and exact setter-to-primary-writer receiver pairing remain unmeasured.

## Highest-priority unresolved candidate: RVA `0x681E8D6`

Disassembly shows a read-modify-write pattern:

1. Read scale/delta input from XMM1 and multiply by `[RSI+0x224]`.
2. Read current `[RSI+0x230]`.
3. Subtract the scaled amount, clamp against a constant and zero.
4. Store the result back to `[RSI+0x230]`.
5. On a threshold branch, dispatch through vtable slot `+0x3E8`.

This is consistent with a gradual decay/interpolation-like update to a float field. It is **not** enough to identify the object as a camera, call it a camera FOV writer, or connect it to the runtime RSI stream. The function's owning type, callers, and relationship to camera vtables remain unknown. It should be the first candidate for type/call-path classification, followed by the other five unclassified stores.

## Identity and coverage corrections

- In the current diagnostic schema, `receiver` is the source pointer RSI, while `fovFieldAddress` is RBX+0x30. These identify different objects. Native source field address RSI+0x230 should be named and recorded separately if future diagnostics need both identities.
- The cinematic/activation generation and settings revision counters do not establish native receiver lifetime. Zero values are not evidence of a broken lifecycle detector; the required lifecycle identity is simply not measured by those counters.
- `POST_STORE` is a readback at the hooked output field. It does not prove no later writer changed that field, nor that the game rendered that exact value.
- One instrumented copy site is known. Other native setters and possible output mutations remain unclassified; global writer coverage stays unknown.

## Next bounded static work

1. Resolve the owning type and vtable/caller context for `0x681E8D6` first.
2. Classify remaining scalar `+0x230` candidates and separate constructors, setters, and per-frame mutators.
3. For each confirmed camera path, document the call edge into `GetCameraView` and whether it can feed the exact hooked `0x9CD2EB` receiver path.
4. Keep runtime reachability, receiver lifetime/reuse, output ownership after POST_STORE, and rendered-view correspondence marked unknown until directly observed.

## Sources

- `assessment.md` and `scalar-store-scan.txt` in this evidence directory.
- `research/reports/POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md`, section 4, based on the same executable SHA-256.

## Direct-copy proof at the primary writer

The source signature in `src/hooks/signatures/signature_definitions.hpp` provides the exact instruction sequence around the hook site:

```asm
TEST   byte ptr [RSI+0x262], 0x10
MOVSS  XMM0, dword ptr [RSI+0x230]
JNZ    conditional path
LEA    RCX, [RBX+0x30]
MOVSS  dword ptr [RBX+0x30], XMM0

conditional path:
ADDSS  XMM0, dword ptr [RSI+0x2D4]
JMP    LEA/store path
```

The validated hook is placed on the final `MOVSS`. If bit `0x10` at source `[RSI+0x262]` is clear, the output store directly copies native `[RSI+0x230]`. If that bit is set, the game first adds `[RSI+0x2D4]` to the native field, then stores the sum. Thus the site is a direct copy only on one branch; the other branch performs a native pre-store adjustment. The mod replaces/chooses XMM0 at the final store when its policy applies.

This proves the primary site's two local source branches. It does not establish which branch is active in any runtime sample, that every relevant frame or all Gameplay/Cinematic/Dialogue paths execute this site, that the native field is independent across later updates, or that no other code subsequently changes the output field.

## Three proposed invariants — evidence status

1. **No-mod writer input:** Confirmed statically as branch-dependent: native `+0x230` is copied when source flag `+0x262 & 0x10` is clear; otherwise native `+0x2D4` is added before the same store.
2. **Native source remains game-owned with the mod enabled:** Not established by the existing transaction log. Its input records the native value at each hook invocation, but does not watch the native setter/update points or prove causality/independence across calls.
3. **All required modes use a fresh native value through this writer:** Partially supported by the derived-camera GetCameraView path. Not established for every runtime subtype, mode, frame, or branch. The observed capture has one source pointer and one output pointer, but no per-mode writer-coverage proof.

A safe acceptance check for invariants 2–3 needs observations of native `[camera+0x230]` updates and primary-writer inputs for the same camera identity over time, plus mode/context evidence for Gameplay, Cinematic, and Dialogue. Until then, retain baseline/recovery behavior; the available evidence is not sufficient to remove it.

## Existing runtime capture: mode presence and native-input movement

A read-only summary of the supplied `STALKER2CameraTweaks.log` (game SHA-256 matches the pinned image) gives these DECISION-event counts:

| Coordinator / writer context | Transactions |
|---|---:|
| `Gameplay / GameplayEligible` | 1,136 |
| `Gameplay / DialogueLifecycleObserved` | 1,164 |
| `Gameplay / TransitionAmbiguous` | 338 |
| `Gameplay / ProviderTransitionObserved` + `RecoveryOwned` | 2 |
| `CinematicActive / CinematicOwned` | 2,778 |
| `CinematicExiting / ProviderTransitionObserved` + `RecoveryOwned` | 155 |

So the validated primary writer was observed during Gameplay eligibility, Dialogue lifecycle, and Cinematic ownership/exit contexts in this capture. This is **site-use evidence** for those contexts, not proof that every required update/frame or every camera subtype uses the site.

The native input `inputFov` varied within the same logged RSI source address in these contexts: 3 distinct values for `GameplayEligible` (90.3056–120), 80 distinct values for `DialogueLifecycleObserved` (80.1444–90.2849), and 162 distinct values for `CinematicOwned` (80.1509–126.87). This shows the writer observed changing source values while the mod was active. It does not identify which native setter/evaluator caused each change or prove that no feedback path exists elsewhere.

An early transaction in this capture has native `inputFov=120`, `previousField=90`, and planned/actual output about `147.796`; the user's chronology identifies that `120°` as a main-menu, pre-save value after changing the FOV setting. It is not evidence for loaded-save Gameplay. `previousField` is the pre-store contents of the output object and may be stale; its inequality with native input does not contradict the game's direct-copy instruction. More generally, `GameplayEligible` describes coordinator context and does not prove a save is loaded; these logs lack an explicit save-load boundary, so their `GameplayEligible` ranges cannot by themselves establish in-save Gameplay values.

These counts are from one runtime session and inherit the logged limitation `globalCoverage=unknown otherWriterSites=not-excluded`.

## Type-aware local graph around derived GetCameraView

The exact caller/callee path in the pinned image is:

```text
Derived camera method 0x53540F6 (vtable slot +0x628)
  RCX = native camera object; R8 = caller-provided output/view buffer
  RSI <- R8; RDI <- RCX
  call 0x3236604(RCX)              // updates camera native FOV at +0x230
  call 0x9CD16C(RCX=camera, R8=output)
      RSI <- RCX; RBX <- R8
      MOVSS XMM0, [RSI+0x230]
      MOVSS [RBX+0x30], XMM0       // direct conditional copy (unless mod replaces XMM0)
      ...
      RCX <- RBX; XMM1 <- [RSI+0x25C]; R8b <- [RSI+0x260]; R9b <- [RSI+0x261]
      call 0x9CE0EE                // post-copy in-place output processing
  virtual call [camera.vtable+0x638] -> 0x535425C(RCX=camera, R8=output)
```

Evidence for register flow: `writer-caller-state-type.txt` and exact disassembly of `0x9CD16C`, `0x9CE0EE`, `0x53540F6`, `0x535425C` from the same pinned executable.

### What this identifies

- `RBX` at the primary store is the output pointer supplied in `R8` by the caller. The same call has `RSI=RCX`, the native camera object. The two roles are separate by dataflow; the supplied runtime log also records distinct pointer values.
- This is a caller-provided camera-view output buffer/record. Its exact reflected/C++ type name and allocation/lifetime owner are not established by the current evidence; do not name a specific Unreal structure yet.
- The derived `GetCameraView` method first updates the native camera's `+0x230`, then passes the camera and output buffer to the primary writer. This orders the native evaluator before the writer in this specific path.
- After the primary copy, the writer calls `0x9CE0EE` with the **same output pointer**. On its conditional path it reads `[output+0x30]`, performs scalar operations/calls, and writes a computed value back to `[output+0x30]`; it also updates output fields `+0x40`, `+0x9D0`, `+0x9D4`, and `+0x9D8`. Therefore the primary site's direct copy is not necessarily the final FOV value in the view buffer at `GetCameraView` return.
- The mod's current POST_STORE observation is immediately after the primary MOVSS at `0x9CD2EB`; it precedes `0x9CE0EE`. It verifies that store, not the final output FOV after the remainder of the function.
- The later virtual call target `0x535425C` receives the same output pointer and writes several other view fields. Its decoded body does not read or write output `+0x30` in the inspected path.

### Feedback boundary in this graph

Within the inspected direct path, `0x9CE0EE` reads and rewrites the output record's own `+0x30`. It is not passed the native camera pointer as an ordinary register argument, and its visible stores target the output record. The later `0x535425C` method receives both camera and output pointers but its inspected body does not access output `+0x30` or write camera `+0x230`.

This narrows the direct feedback risk in this one derived GetCameraView graph: no explicit edge from output `+0x30` back to camera `+0x230` was found in the inspected functions. It does **not** rule out indirect-call side effects, shared/global state, other GetCameraView overrides, or later consumers that store values into persistent camera state. Output feedback to native FOV remains unproven globally.

### Bounded virtual-consumer scan (offline, pinned image)

To narrow the next link without repeating the function-level audit or scanning the executable for every `+0x30` displacement, `scan_getcameraview_virtual_callers.py` searched the pinned image for direct `CALL [register+0x628]` encodings, validated candidate instruction boundaries against `.pdata` function ranges, and inspected each containing function for a locally traceable output pointer in `R8` followed by direct `[output+0x30]` accesses. The scan is deliberately bounded to this vtable slot and local function bodies.

Result for SHA-256 `2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479`: 38 candidates, 38 verified call instructions, 0 unbounded candidates; 0 direct output `+0x30` accesses found by the script after these calls, and 0 such accesses followed locally by a virtual setter-slot `+0x608` call. The script's matcher currently covers the encoded call forms it explicitly searches; this is not a complete disassembly/dataflow proof for every register extension, indirect dispatch target, alias, spill, or interprocedural path. The 38 sites are generic slot callers: static matching does not prove each runtime-dispatches to derived method `0x53540F6`.

Interpretation: this adds bounded negative evidence against a simple same-function pattern of “GetCameraView returns, caller directly reads its output FOV, then calls the native FOV setter.” It does not establish that output FOV cannot feed back by another route. In particular, this scan does not resolve indirect callees, copied or spilled output values, global/shared state, later asynchronous consumers, or other camera methods. The separately documented Dialogue setter path can write native camera `+0x230`; its existence alone does not show that it sources that value from output `+0x30`.

### Mode coverage boundary

This derived method has a concrete native-update-then-output sequence, but it does not by itself prove which runtime camera types or Gameplay/Cinematic/Dialogue cases dispatch through it. The runtime state labels and primary writer counts demonstrate site use in one session; runtime receiver vtable/type and per-mode call-path identity were not captured. The base setter at `0x1F88EDC`, other `+0x230` candidates, and output buffers from other camera overrides remain outside this local graph.

## `0x9CE0EE` condition and FOV math

The helper's relevant dataflow is statically recoverable in the pinned image:

1. It takes camera-derived modifier `m = [camera+0x25C]` in XMM1, output pointer in RCX, and two flags in R8b/R9b.
2. It clears the sign bit of `m` and compares `abs(m)` with `1.0e-8`. If `abs(m) <= 1.0e-8`, it returns without touching output FOV. An unordered comparison (for example, NaN) sets CF, so the following `JB` takes the active path; the later `MAXSS` then determines the fallback modifier.
3. On the active path, it reads smoothing state `s = [output+0x9D8]` and forms a lower bound `m' = max(m, -s/(s+1))`.
4. It sets `scale = 1 + m'` and updates the smoothing state to `s' = s*scale + m'` at `[output+0x9D8]`.
5. It transforms current output FOV `F = [output+0x30]` using the equivalent expression:

   `F' = 2 * atanf(tanf(F * 0.5 * 0.0174532923847) * scale) * 57.295776`

   Constants resolve to `0.5`, degrees-to-radians `0.0174532923847`, and radians-to-degrees `57.295776`. The imported functions resolve to CRT `tanf` and `atanf`.
6. It stores `F'` back to `[output+0x30]`. It also scales output field `+0x40` and updates `+0x9D0` / `+0x9D4`; the first value `+0x9D8` is used as smoothing state.

Thus it performs a tangent-space FOV scale, not a simple additive degree offset. When active, the input is whatever value is currently in the output buffer at that point, including the mod's corrected value if the primary hook replaced XMM0. This proves the helper **can** modify a corrected value on its active branch; it does not prove the branch was active in any supplied runtime sample or quantify its effect there.

### Runtime status

The existing log's post-store callback runs before this helper and contains neither the camera `+0x25C` modifier nor output `+0x9D8` smoothing state nor a readback after `0x9CE0EE`. It therefore cannot tell whether the helper activated in Gameplay, Cinematic, or Dialogue, or what final FOV it left in the output buffer. Establishing that needs a later observation after the helper (or a narrowly validated helper-entry/exit observation) plus mode context. The current evidence does not justify moving the production correction hook.

### Scope limit

This describes the identified helper call in the derived GetCameraView graph. It does not classify the semantic owner of camera `+0x25C` or output `+0x9D8`, establish output-buffer lifetime, or exclude other post-copy processing in other camera overrides.

