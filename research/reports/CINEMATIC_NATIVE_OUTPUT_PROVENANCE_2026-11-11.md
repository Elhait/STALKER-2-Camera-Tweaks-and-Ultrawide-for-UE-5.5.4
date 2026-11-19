# Native cinematic output provenance and EXIT freshness

Date: 2026-11-11. Read-only source/runtime-evidence investigation. No production changes, compilation, game/debugger launch or graphics tests.

## Evidence identity and scope

Supplied log: `STALKER2CameraTweaks.log`, 2,288 lines, 1,352,350 bytes at inspection.

- ASI SHA-256: `896C4923ECB3C8D4321FB84D73AEE94C5FAD6582594427FD379E84B41AFA7590`.
- Game SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Runtime records successful camera-writer signature validation at RVA `0x9CD2EB` and observer installation.
- Current source: `src/gameplay/gameplay_camera.cpp`, `src/hooks/signatures/signature_definitions.hpp`, cinematic association/recovery code and the production writer/lifecycle transactions.
- Historical disassembly provenance: [Track B](CINEMATIC_AXIS_CONSTRAINT_RUNTIME_REFLECTION_TRACK_B.md). Its Steam 2.0.4 addresses/types are not promoted to current-game facts. The current-game identity and resolver acceptance above come from the supplied log. No independent current EXE disassembly was performed here.

## What the native boundary proves

The resolver requires one matching complete camera-view-copy signature in `.text`, then decodes the selected store as `MOVSS [RBX+0x30], XMM0`. The fixed bytes around it include:

| Operation in matched path | Bounded meaning |
| --- | --- |
| Read `[RSI+0x230]` into XMM0 | Input FOV of the current native source |
| Store XMM0 into `[RBX+0x30]` | Destination of this invocation's native FOV store |
| Copy `[RSI+0x254]` into `[RBX+0x5C]` | Same invocation copies source aspect to destination |
| Read source flag bytes and update destination fields | Further context distinguishing the copy path from a generic float store |

This establishes a source-to-destination operation for the invocation. Together with accepted cinematic generation and exact source/aspect-target association, it is sufficient to scope the mod's correction to the destination that this native invocation selected. Stable RBX across invocations is not necessary for this bounded operation.

It does **not** establish that this is the only FOV writer in the entire game, that RBX is a UObject, that the destination is the final onscreen camera, or that it remains allocated/active after the callback. No global uniqueness, reflected class identity or final presentation claim is required for this correction.

## Discriminator and observation limit

No additional output-ownership discriminator is established by the inspected source, signature, historical report or supplied log. This is not proof that none exists elsewhere in the engine.

In particular, two destinations reached through the same validated native invocation boundary with the same accepted source/generation cannot be labelled legitimate versus semantically foreign using the current observations. A writable pointer or matching FOV would not solve that distinction either.

The implementable contract is invocation-relative: an output selected by this native copy call from the confirmed source is eligible for the existing cinematic policy. It is not a persistent output-owner allowlist. A test must not invent an unobservable semantic foreign-output label or pass a fabricated `outputTrusted` flag that production cannot establish.

Rejectable cases remain concrete: missing/zero output identity, unassociated source, inactive/stale generation, conflicting aspect target, disabled intervention or an existing failed FOV/policy precondition. Native pass-through is preserved for rejected invocations. Whether to add structural address-span validation is a separate safety choice; it must not be advertised as semantic ownership proof or silently add diagnostic-only reads to production authority.

## EXIT and actual recovery consumers

`ProcessAcceptedCinematicExit` reads association and calls RecoveryOwner::Capture before invalidating the generation. EXIT's RBX is not a writer-output observation: the indexed EXIT context uses RBX/RAX for its own native operand. In generation 2 the EXIT RBX is `0x1e9e43b6400`, whereas the admitted writer destination is `0x1e9d5a90360`. Substituting EXIT RBX would corrupt the meaning of the evidence.

Recovery authorization is already source-bound:

- `CinematicRecoveryOwner::MatchesSource` compares RSI only.
- `RetireRecoveryForWriterRsiChange` explicitly treats RBX/identityEpoch as metadata.
- Writer recovery checks `owner.writerRsi` against the current source, then uses current-call source fields/input and recovery FOV rules.
- The recovery snapshot's observed RBX is copied/logged, not dereferenced as a retained destination or used as proof of output lifetime.

The log's last emitted AFTER record precedes EXIT by about 11.923 seconds in generation 1 and 9.691 seconds in generation 2. These are emission gaps, **not** proof that no callbacks occurred. A millisecond freshness timeout or requirement for a new output observation at EXIT would therefore be unsupported by these records.

## Proposed contract for the implementation phase

This is an evidence-backed engineering decision, not an already implemented or runtime-validated fix.

1. **Source ownership:** accepted generation/settings plus the confirmed cinematic aspect-store source. A conflicting aspect target invalidates that ownership. An unrelated source invocation is rejected without adopting it.
2. **Invocation output:** RBX is the destination chosen by the validated native copy path for this call. A -> B -> A or A -> B alone is an output transition, not source conflict. Existing FOV/establishment/continuation preconditions still apply per call.
3. **Last admitted observation:** store generation, source, output and observation sequence for the latest admitted call. Keep this distinct from latest observed writer and from whether the FOV policy actually changed the register; valid interpolation need not destroy source provenance.
4. **Rejection:** do not replace last admitted evidence, revive establishment, acquire recovery authority or poison the source association merely because a call was rejected. A real source/aspect ownership conflict remains terminal under the existing rules.
5. **EXIT freshness:** snapshot the last admitted output for the same still-valid generation/source as historical metadata. It means "last admitted destination", not "currently active output". No timeout, borrowed EXIT register, output dereference or output-lifetime guarantee. Without matching admitted evidence, fail recovery capture closed. After a rejected B, previously admitted A may remain historical metadata while independently valid source ownership permits capture; B receives no authority.
6. **Recovery:** authority remains generation/source-bound. Each subsequent recovery invocation uses its own current native writer context; cached A/B must not become a memory-write target or a new RBX equality gate. Preserve existing source retirement, interpolation/endpoint and recovery completion behavior.

## Test implications

The old same-RSI/different-RBX test should distinguish output transitions from true source/aspect ownership conflicts. Cover A -> B -> A, B persisting through EXIT, unrelated sources between admitted calls, null/invalid admission, stale generation, EXIT after rejected B, and Gameplay recovery while its current output differs from the last cinematic output. Assert actual register output and owner generation/source plus historical output metadata.

A "foreign B" with exactly identical observable native provenance is outside the discriminator's power: the test should document that the native call selects B, not pretend production can reject it. Truly foreign source or missing invocation admission must remain rejected. Diagnostics OFF/ON must give identical decisions.

## Remaining limits

No caller enumeration, reflected output-type reconstruction or unique-final-camera proof. No production diff or regression execution yet. The original weapon-switch visual scenario remains a manual acceptance check after the coupled association/continuation/recovery implementation. The bounded contract above removes the previous need to search indefinitely for a stronger, currently unobservable output identity.
