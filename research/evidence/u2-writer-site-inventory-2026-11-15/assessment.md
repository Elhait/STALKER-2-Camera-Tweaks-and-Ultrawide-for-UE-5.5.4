# U2 writer-site inventory — 2026-11-15

## Scope

Read-only inventory against `Stalker2-Win64-Shipping.exe` SHA-256 `2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479`, paired with the runtime diagnostic log from the repaired build. No FOV policy or production code was changed.

## Runtime capture

The supplied runtime log reports 5,573 writer transactions and 16,719 correlated events (BEFORE, DECISION, POST_STORE), with zero queue drops, zero contention drops, zero post-store readback mismatches, and no pending records at shutdown. This confirms the diagnostic path at the instrumented writer site for that session; it does not establish coverage of every native writer or correspondence to rendered-view FOV.

The record identity fields need careful interpretation. The event's `receiver` currently holds RSI, the source camera object read at `[RSI+0x230]`. The `fovFieldAddress` is derived from RBX+0x30, the output/view object field written by the hook. These are two different objects and fields. The logged pair must not be described as a receiver plus its FOV field.

`writerSiteId` is currently the constant 1 for the validated primary writer. Startup explicitly reports other writer sites as not excluded. Thus this is a known site ID, not a global site inventory.

The logged `lifecycleGeneration` is cinematic/activation association generation, not a verified native receiver lifetime. Settings epoch fields are configuration revision counters; a value of zero means no revision has occurred in the tracked counter, not that receiver identity or lifetime is established.

## Static writer candidates

A bounded, hash-pinned Capstone scan of decoded scalar `MOVSS` stores to displacement +0x230 found eight instructions:

| RVA | Status | Notes |
|---|---|---|
| `0x1F88EDC` | Confirmed camera setter | Base camera setter identified in the same-image post-exit dataflow report; writes XMM1 to `[RCX+0x230]`. |
| `0x5353CD8` | Confirmed derived-camera setter | Same-image report identifies the derived camera setter; writes `[RCX+0x230]` and updates adjacent camera state. |
| `0x32367E4` | Confirmed initialization, not counted in scalar scan | Existing same-image report identifies a qword initialization of two 90.0 floats at `[RSI+0x230]`. This is object initialization evidence, not proof of a runtime update path. |
| `0x4D2463` | Candidate | Conditional update of `[RSI+0x230]`; owning type and call path not established here. |
| `0x3EEE1C9` | Candidate | Writes `[RSI+0x230]`, then adjacent byte +0x234 and calls a helper; owning type not established. |
| `0x54638DD` | Candidate | Appears in a larger object initialization sequence; type/lifecycle relation not established. |
| `0x5A9D4E7` | Candidate | Writes after bulk field copies and a conversion; destination type not established. |
| `0x5F201CE` | Candidate | Adds to existing `[RCX+0x230]` and stores the result; owning type not established. |
| `0x681E8D6` | Candidate | Computes from existing `[RSI+0x230]` and adjacent state before storing; owning type not established. |

This scan only inventories decoded scalar MOVSS stores with the exact +0x230 displacement in the pinned image. It does not exclude vector, integer, indirect, memcpy, other-displacement, dynamically generated, or unrecognized stores, and it does not establish that every candidate targets the camera object observed at runtime.

The current hook's exact output store is RVA `0x9CD2EB`, writing `[RBX+0x30]`. A raw byte-pattern search found 135 possible +0x30 instruction encodings, but these are not all decoded or type-identified and cannot be counted as writers to the same output object. The existing validated signature supports only the primary copy site.

## Coverage conclusion

Confirmed: one instrumented native-to-view FOV copy site; at least two native camera setter sites and one initialization path are separately identified in same-image analysis; additional scalar +0x230 candidates exist.

Unknown: whether any candidate is reachable for the runtime receiver, whether other store forms/sites exist, whether receiver addresses are reused across lifetimes, and whether the observed stored value equals rendered-view FOV.

Accordingly, global writer coverage remains **unknown / incomplete**. The next static step is to classify the six unclassified scalar candidates by owning type and call/vtable context, then compare the resulting paths with runtime address observations. Runtime-only claims about reachability or receiver reuse require a later capture.

## Evidence files

- `scalar-store-scan.txt` — bounded instruction scan and local instruction context.
- `scan_scalar_fov_stores.py` — hash-pinned scanner.
- `research/reports/POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md` — prior same-image native setter/dataflow evidence.
