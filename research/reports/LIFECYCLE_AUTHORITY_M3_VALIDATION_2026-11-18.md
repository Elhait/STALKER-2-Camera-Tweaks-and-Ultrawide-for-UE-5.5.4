# M3 — Lifecycle Transition Exhaustive Validation

## Domain and oracle

The new harness drives the production Authority via its Writer, Dialogue and Cinematic ports, the production settings transaction, and the committed lifecycle, Camera Evidence and overlay projections. The reference graph and the numeric oracles are in [lifecycle_authority_transition_model.hpp](../../tests/plugin/lifecycle_authority_transition_model.hpp). The model uses named observable milestones and independent equations for HorPlus, Reduced, the Dialogue projection interpolation, and the linear Cinematic blend. Historical R1 remains frozen.

The short generator enumerates every word of length 1–4 over the three primary modes from a confirmed Gameplay start. This covers all 9 pairs, 27 triples and 81 quadruples of mode visits in each diagnostic configuration, including self-transitions. Each mode visit expands into source-bound native callbacks. A repeated Cinematic mode visit starts a new activation through EXIT and ENTER. An additional case interrupts a provider claim with ENTER before Gameplay recovery. Dialogue self-visits include steady setter/writer frames.

| Protocol | Observed inputs and authority rules |
| --- | --- |
| Bootstrap and Gameplay | Initialize with settings. Writer validates source, output object, thread, aspect and flags. Source replacement can correct one scalar while withholding baseline admission. Subsequent eligible observations publish the new reference. |
| Cinematic selection | Valid ENTER captures generation/config and authored FOV before association. Duplicate ENTER cannot replace it. Aspect associates a policy source and executes one native store. The blend armer may precede or follow Aspect and must match the ENTER thread. Blend Writer frames use that association. EXIT ends selection and starts recovery when its target is valid. |
| Physical recovery | Cache the corrected Cinematic endpoint. Validate the same source and the endpoint separately from the departure-and-return proof that releases baseline exclusion. Cover an interpolated frame before and after the first native endpoint. |
| Provider handoff | The validated armer is scoped to the exiting camera. After a matching setter, Writer joins the sample and moves to the provider endpoint. The recognized setter consumes Dialogue admission. Wrong-thread Writer and an unrelated same-receiver setter are tested independently; a subsequent Cinematic generation invalidates an unconsumed claim. |
| Ordinary Dialogue | Starting setter opens Candidate, stationary samples stay there, a descent confirms Active, and ascent moves through Exiting and RearmPending. A converged native target ends the claim. A subsequent eligible Writer must refresh Gameplay immediately. |
| Effects and projections | Compare every modeled Writer/Setter frame. Check separate post-store and post-helper boundaries, bad correlation, repeats and close, omitted observations, and historical readback after an epoch change. Check committed overlay selection, epoch/generation projections, and store bounds after callbacks. |
| Settings, shutdown, reentry | Cross Gameplay, Cinematic and Dialogue disable gates with all 14 reachable nonterminal milestones. Verify epoch revocation, interruption and late readback behavior. Shutdown clears all claims and rejects a late Writer. Test the native gate's reentrancy predicate while a callback holds it. |
| Pressure | Reuse an address after traversing more than 64 distinct receivers, and omit readbacks for more than 128 Writer frames while corrections remain available. |

The strength-2 array covers pairs of endpoint ordering, 21:9/32:9, 90° and 130.311° Gameplay FOV, receiver identities, HorPlus/RuntimeCombine, and Cinematic armer ordering. It is run in both diagnostic modes, as are the eight deterministic xorshift seeds with 72 mode visits per seed. The seeds are `0x7372`, `0x5E771E`, `0x13579BDF`, `0xA593`, `0x1234ABCD`, `0xC001CAFE`, `0x600DCAFE`, and `0x7781`. Failures emit the case ID or seed and the prefix through the first bad event. Permanent minimized cases execute before the generated paths.

## Reproduced defects and repairs

| Defect | Minimized native-port sequence | Repair |
| --- | --- | --- |
| Post-exit interpolation can be transformed twice | Gameplay baseline → ENTER/Aspect/Writer → EXIT → Writer at native endpoint → same-source Writer inside the protected segment | Keep approved interpolation in Recovery during the pending exclusion. At 32:9 with a 130.311° Gameplay baseline, raw 142.121° now yields the held 153.931° instead of 160.529°. The interpolation does not populate Gameplay baseline. |
| A wrong-thread Writer can invalidate a valid provider join | Gameplay baseline → ENTER/Aspect/Writer → EXIT → provider armer → matching setter → Writer on another thread for the same source | Reject the unrelated Writer before it can consume the native provider's pending setter evidence, update recovery, or prepare an argument. A matching Writer can still finish the handoff. |
| A different setter can terminate an admitted provider Dialogue | Complete a native provider handoff and admit Dialogue → an unrelated setter ascends on the same receiver | Reject a setter outside the admitted callback source/thread/generation before it mutates the provider Dialogue. A matched setter can still finish Dialogue, and the next eligible Gameplay Writer refreshes the reference. |
| The first Cinematic after ordinary Dialogue loses its corrected start | Gameplay → ordinary Dialogue with a committed Writer result → first Cinematic ENTER → blend armer and Aspect → first blend frame | Use the committed Dialogue Writer generation and transaction. Generation 0 is the valid period before any Cinematic ENTER. For the tested 32:9 case, the first frame stays at 149.071° instead of dropping to the 122.087° prepared input. |

The latest available game log supplies a real post-exit provider example (generation 5, native 130.311° → 70°). The generated regressions use admissible native-shaped facts independent of a save or scene. The numeric and ownership failures above were reproduced offline. The log does not establish that these are the only possible causes of a visual interruption.


## Original failing prefixes

These failures came from the short enumeration, covering array or checkpoint traversal. Their case IDs are deterministic. No random seed was involved in these four failures.

~~~text
GGCD/OFF — setter outside the active provider
initialize gameplay-writer gameplay-writer gameplay-writer cinematic-enter duplicate-enter cinematic-aspect cinematic-writer cinematic-exit duplicate-exit provider-arm provider-setter provider-first-writer provider-native-endpoint dialogue-target-setter dialogue-target-writer unrelated-provider-setter

CD/OFF — wrong-thread Writer consumed the provider join
initialize gameplay-writer cinematic-enter duplicate-enter cinematic-aspect cinematic-writer cinematic-exit duplicate-exit provider-arm provider-setter foreign-thread-provider-writer

pairwise/1/OFF — recovery interpolation received Gameplay HorPlus
initialize gameplay-writer cinematic-enter duplicate-enter cinematic-aspect cinematic-writer cinematic-exit duplicate-exit provider-arm provider-setter provider-first-writer provider-native-endpoint dialogue-target-setter dialogue-target-writer unrelated-provider-setter dialogue-exit-start dialogue-exit-endpoint dialogue-return-setter gameplay-writer dialogue-start dialogue-descent-1 dialogue-descent-2 dialogue-descent-3 dialogue-target-setter dialogue-target-writer cinematic-enter duplicate-enter cinematic-aspect cinematic-writer cinematic-exit duplicate-exit recovery-early-endpoint recovery-departure

interrupt/0/5/OFF — first Cinematic after ordinary Dialogue used a raw start
initialize gameplay-writer dialogue-start dialogue-stationary-candidate dialogue-descent-frame dialogue-descent-frame dialogue-descent-frame dialogue-target-setter dialogue-target-writer cinematic-enter duplicate-enter foreign-cinematic-armer cinematic-armer cinematic-aspect cinematic-blend-frame
~~~


## Offline validation

The standalone M3 matrix reached 15 milestones, 25/25 declared protocol edges, 9/9 primary-mode edges, 378 sequences, 22,864 event checkpoints, and 8,502 modeled FOV frames. It used eight seeds in each diagnostic mode and 84 settings/shutdown variants across the 14 nonterminal milestones. 129 correction frames were executed with observation capacity unavailable.


| Verification | Result |
| --- | --- |
| Focused CTest | 7/7 PASS, including native adapter matrix, native trampoline and camera callbacks in Diagnostics OFF/ON, M1/parity/M2 and M3 |
| Canonical non-graphics | 64/64 PASS, zero failures, one hardlink subcase skipped, WARP omitted by profile |
| Release preprocessing audit | Zero audited legacy lifecycle mutation symbols and zero missing Authority boundaries |
| Release candidate | [STALKER2CameraTweaks.asi](../../build-artifacts/lifecycle-authority-m3-2026-11-18/STALKER2CameraTweaks.asi), SHA-256 `2437DD59F530D873DD72F9A719D494483813593436AD423B7301E1F96E09AAB1` |

Process-tree cleanup was verified for the build, CTest, native adapter runs, and canonical suite. The ABI check exercised a real SafetyHook trampoline in the native fixture. Runtime hook addresses and instruction layout were outside the M3 code changes.


The domain is bounded. Same-receiver callbacks indistinguishable on all exposed native facts cannot be uniquely assigned to an engine episode by this harness. The model uses the positive evidence already available to the shipping callbacks. Visual smoothness, the engine's callback distribution, and observation of engine events outside the modeled boundaries are separate live checks. No ASI was copied into the game.


## Live follow-up — provider departure

The 18 November runtime log confirms steady Reduced output of 149.071° in generation 15 through 14:10:45.808. At 14:10:46.208 the same receiver and output object supplied a Writer input of 122.090° and read back 152.900°. The first bad transaction is 88303. A small change from the steady prepared input of 122.087° exposed the forward Dialogue mapping on the way out. The Writer trace does not establish the exact setter callback order at that moment.

The new native-port regression keeps the stable native 70° control, admits and settles the provider, then feeds the departure Writer values from the trace without renewing the setter witness. It reproduced 152.900° where one HorPlus pass yields 149.073°. Authority now uses the completed provider's correlated native endpoint to authorize forward mapping. Other departure samples still receive Gameplay's HorPlus stage, and they cannot replace the retained Gameplay baseline.

The revised matrix passes 380 sequences, 22,958 event checkpoints and 8,530 modeled frames in Diagnostics OFF/ON. This verifies the recorded numerical failure at the Writer boundary. Live verification of the revised artifact remains separate.

