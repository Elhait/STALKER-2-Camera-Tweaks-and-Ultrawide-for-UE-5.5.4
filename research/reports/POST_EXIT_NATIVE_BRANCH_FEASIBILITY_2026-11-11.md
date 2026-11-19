# Post-EXIT native branch — feasibility of activation/progress evidence

Date: 2026-11-11. Scope: approved bounded read-only native branch review and research-contract refinement. No production/diagnostic implementation change, ASI build, native harness, game, debugger, graphics test, installation or Git operation.

## Identity and artifacts

Actual EXE SHA-256 remains `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`. The offline decoder independently recomputes it before decoding. RVAs below identify this image's evidence and are not proposed runtime gates.

Native decode/constant/run records: `E:/Work/Slaker2 mods/02-Research/evidence/post-exit-dataflow-2026-11-11/branch-review/`. Decoder source is the existing bounded `inspect_native.py` in its parent directory, extended only with constant/dispatch and PE-import inspection. Processes have a 25-second supervisor bound, reaped completion, and no child-process launches.

Existing sources: [native data-flow](POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md), [runtime correlation](POST_EXIT_BLEND_LIVE_CORRELATION_2026-11-11.md), [research contract](../plans/POST_EXIT_RECOVERY_CONTRACT_PROPOSAL_2026-11-11.md). The old `106.688 -> 90` case remains a separate mathematical fixture, not a second current-image activation trace.

## 1. One object can be rearmed

The provider-level entry `0x30DB9B8` uses provider `+0x2CF8` as a current component pointer. If that object is usable and its saved target `+0x54` equals the requested target, the wrapper returns. If the target differs, it reaches the same initializer `0x30DBCD2` on the existing object. It can also allocate a component when no usable pointer exists.

Initializer writes:

```text
0x30DBCDB  S+0x54 = incoming target XMM1
0x30DBD12  S+0x50 = resolved provider duration
0x30DBD68  S+0x58 = current receiver camera+0x230
```

Thus an object's allocation identity or weak-object serial is not a rearm/activation epoch. The inspected initialization/tick fields contain no established monotonic activation token. This is not a claim that no token exists elsewhere in the game.

An observation-side counter at a proven successful initialization event could represent an epoch. A counter incremented merely when FOV values, timestamps or pointers change would not prove native activation.

## 2. Tick reads live interpolation settings

At tick `0xCCE4FA`, T=S+0x28:

```text
r = T+0x28, decremented by delta time
S_fov = T+0x30
T_fov = T+0x2C
D = current provider+0x2CE8
k = current provider byte+0x2CEC
p = current provider+0x2CF0, in branches that use it
```

For positive duration, XMM7 receives `a = clamp((D-r)/D, 0, 1)`. The same native tick reads D/k/p again; they are not immutable captured activation metadata. A fixed mapping method must not freeze native settings against actual native changes.

The power helper's IAT slot `0x9C38F08` resolves to `powf` from `api-ms-win-crt-math-l1-1-0.dll`, verified within that DLL's terminated import thunk array. Constants decoded from this EXE are 1, 0.5, -1 and 3, rather than assumed from a familiar easing name.

## 3. Supported normal forms are branch-specific

For the following inspected positive-duration branches, instruction-level algebra has the form `f = S_fov + (T_fov-S_fov)*w`:

| Selector k | Weight w | Evidence |
|---|---|---|
| 0 | a | Dispatch target `0xCCE706`; linear multiply/add |
| 2 | `powf(a,p)` | Dispatch `0xCCE75B`, shared output path `0xCCE7DE` |
| 3 | `powf(a,1/p)` | Dispatch `0xCCE7CE`, shared output path |
| 4 | `0.5*powf(2a,p)` below a=0.5; otherwise `1-0.5*powf(2(1-a),p)` | `0xCCE58D..0xCCE60B`; XMM8 holds the effective weight in this branch |

Selector 5 selects the target directly. It should not be treated as an ordinary interpolation needing a recovered weight.

Selector 1 dispatches to `0xCCE765`. Its decoded expression includes separate start/target polynomial terms and an additional term; it is not promoted here to the same normalized endpoint-preserving form. Its runtime use is not observed and no game defect is claimed. Non-positive duration, invalid parameters, unknown selectors and unsupported branches cannot silently inherit the above contracts.

At the existing common Dialogue hook:

- XMM6 is the raw generated sample;
- XMM1 is copied from XMM6 immediately before the setter;
- XMM7 is normalized time a, not generally eased weight w;
- XMM8 is w for selector 4, but is not a generic all-branch weight register.

Therefore reading XMM7 unconditionally would change easing. Reading XMM8 unconditionally would treat an unrelated register as proof in other branches.

For a proven branch of the normalized form and `S_fov != T_fov`, `w=(f-S_fov)/(T_fov-S_fov)` can recover the effective native weight without recreating its clock. That is not an ownership classifier and cannot authorize the sample on its own. It also needs a numerical error analysis near degenerate endpoints; arbitrary clamping is not a substitute. Branch k must be known or the validated boundary must otherwise establish the required formula for that invocation.

## 4. The recorded curve supports one hypothesis, not a provider-state observation

All eight sampled points of the problematic context are numerically consistent with D=1, k=4, p=2. Using logged remaining time and the above formula gives maximum absolute output residual about `0.0000404` degrees, within the limited precision of the printed fields.

See `recorded-easing-consistency.json` in the branch-review artifacts. These parameter values were not logged. This is a bounded consistency result, not proof of the actual provider selector/exponent or its uniqueness. It must not become a production constant or game-build-specific assumption.

## 5. Why provider/activation identity is not already available at writer

The tick resolves its settings provider via `0xCCE802 -> 0xCCE84C -> 0xCCE88A`. The component's actual virtual slot +0x198 points to `0x2521C7A`, which traverses an outer/context chain and may dispatch another virtual implementation. It is not the camera component getter at `0x235EE2`.

The subsequent resolver calls `0x240F5C0` with EDX=0, then reads a pointer at returned object +0x368. Exact reflected type naming is not necessary to establish this data-flow and is not promoted here.

The weak provider reference at S+0x48 records the initializer's provider; tick resolves a live provider again. It cannot be assumed that a historical weak reference always names the current duration/easing source. Calling the native resolver again from an observer would execute game code and is not proposed as a read-only memory observation.

The current writer callback has camera/source/output/input, but not this native component's activation or settings state. The latest same-camera/thread diagnostic pair is observational metadata, not an exclusive native write lineage. The correction gate serializes mod callbacks, not all native code between the pre-setter callback and writer. Later writer input must not inherit an earlier blend's method merely because its number matches.

These facts are independent of Diagnostics being enabled: the native state exists without it. However, a production-safe capability extracting and transferring that evidence is not implemented or validated by the existing diagnostic records.

## 6. A concrete successful-initialization anchor

After `0x30DBD68` stores the actual camera start into S+0x58, RVA `0x30DBD6D` has RSI=S and RDI=the resolved camera. This is a concrete candidate observation point for a successful start capture, before the activation/query path. It can distinguish new successful initialization on a reused S address from continued ticks.

That point is not one of the currently installed hooks. It is identified here, not installed, not approved for production and not promoted to an RVA-based compatibility mechanism. Any future observer would need structural resolution through the validated native chain and exact operand/bounds checks.

The normal frame has `push RSI; push RDI; sub RSP,0x28`, suggesting a return slot at native RSP+0x38. The known ENTER/EXIT forwarding wrappers balance their frames and tail-call the initializer. This offers a bounded caller-lineage candidate tied to the validated call return sites. SafetyHook documents a hook-time RSP and trampoline RIP; it does not make an arbitrary RSP read a verified caller. This exact frame/context interpretation still needs a CPU-only adapter proof before any game use. Do not repeat the old unvalidated `[context.rsp]` caller assumption or infer a native PC from context.rip.

Failure paths can update the target before successful camera-start capture. A successful-start observer alone does not prove safe handling of failed rearm or native cleanup. Those failure/invalidation rules must be established before treating an observed epoch as production authority.

With the earlier restriction to existing hook points, direct initialization lineage remains unobserved. Repeating the same general log cannot fill that gap. An additional bounded observation point would be a separately approved scope change, not something silently added to the current artifact.

## 7. Result of the A/B feasibility fork

| Requirement | Result |
|---|---|
| Native eased progress exists and can be used without a new timer | Confirmed statically for the listed branches, subject to live state/provenance and numerical checks |
| One global generic progress register | Rejected: XMM7 is pre-easing and XMM8 is branch-specific |
| Component pointer is activation identity | Rejected by wrapper rearm path |
| Current writer has proven activation→progress→endpoints lineage | Not established by existing hooks/recorded data |
| Universal Native-space tag allowing A without this lineage | Not established; native code reading camera+0x230 does not prove absence of prior mod-derived values |
| B is a feasible bounded research candidate | Yes, for established branches and an actual admission/lineage contract; not yet production-ready |
| A is proved safe for every post-EXIT sample with fewer changes | No such proof found in the bounded review |

A and B need not be global mutually exclusive modes. The research proposal now pins the admitted mapping method per proven scope, defines explicit method-change boundaries, and requires counterfactual same-evidence tests across recovery/Gameplay. Actual native cuts and loss of evidence are separate dispositions. Neither desired output curve nor JSON behavior oracle is selected here.

## Next decision, kept narrow

Retain the current ASI and recovery unchanged. The next useful step is to decide whether to authorize a bounded successful-initialization observation/validation phase at the identified anchor, with explicit failure and caller-frame checks. It is not another general camera-log run or a production rewrite. If the no-new-hook-point restriction remains, report the activation gap and do not pretend that numerical correlation closes it.

No full native ownership manager, polling loop, hash gate, speculative callback or camera write is proposed. The old transformed-start fixture and the recorded native-start/multiple-context input remain mandatory, distinct regression subjects.
