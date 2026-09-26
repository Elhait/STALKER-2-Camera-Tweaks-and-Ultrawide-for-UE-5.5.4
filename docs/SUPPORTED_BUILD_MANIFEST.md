# Supported Build Manifest

## Build support and evidence matrix

Game: S.T.A.L.K.E.R. 2: Heart of Chornobyl, UE 5.5.4. This matrix is the
repository's authoritative summary of evidence scope; it distinguishes a
tested game/runtime session from static resolver portability and from a
feature-specific reader check. SHA-256 values are evidence identities, not a
universal production allowlist.

| Steam build | Evidence scope | Game executable SHA-256 | Runtime mod artifact identity | Record and boundary |
| --- | --- | --- | --- | --- |
| 2.0.2–2.0.4 | Historical 0.4.0 production resolver-contract portability only; unique matches and relevant instruction contracts were validated on identity-matched images. This does not establish the complete current resolver set or runtime support. | Per-image identities are recorded in the cross-patch resolver task evidence. | Not applicable | [`CROSS_PATCH_PRODUCTION_RESOLVER_VALIDATION_TASK_PLAN.md`](../research/completed/CROSS_PATCH_PRODUCTION_RESOLVER_VALIDATION_TASK_PLAN.md). |
| 2.0.5 | Production runtime evidence for the tested scenarios. | `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293` | `69021D8758069F7EFE098B3C562A41E326A6DB0BF4BA88B789FB38854217DFB2` in the recorded run; not an identity allowlist. | [`RELEASE_PREPARATION_v0.5.2.md`](../research/reports/RELEASE_PREPARATION_v0.5.2.md). This does not establish every scenario or later binary. |
| 2.0.6 | Combined production runtime pass and tested feature scenarios; separately, the Auto Interface Language reader passed its bounded locale sequence. | `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B` | Combined runtime log records `19F2F31C20BB5D47CD12D2D3D773774985A5D771E6F7F8730A6363983161DA72`. | [`GLOBAL_HORPLUS_RUNTIME_PASS.md`](../research/reports/GLOBAL_HORPLUS_RUNTIME_PASS.md) and the v1.0.0 compatibility summary in [`TESTING_AND_RESEARCH.md`](../TESTING_AND_RESEARCH.md). The report's scenarios are the scope of the claim, not blanket coverage of all gameplay. |

The v1.0.0 release-candidate hash `D04A43E28DB5DFFD10D88B6F30BEF8FEC2A560E1CD9949FEA31FAF485DA0E7BC`
is recorded in `TESTING_AND_RESEARCH.md`, but the combined 2.0.6 runtime
record names a different mod hash (`19F2…`). The repository does not establish
that those are the same binary; therefore the runtime pass must not be
attributed to the exact `D04A…` release artifact without matching run evidence.

The standalone Auto Interface Language reader has feature-specific validation
on Steam 2.0.6 executable SHA-256
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`. It
validates the executable's AMD64 `.text` layout and reader/free-function
prologues before use; failure falls back to English. This does not broaden
resolver or runtime evidence to another game build.

The workspace's current release-assets ASI is a separately built file; its
present SHA-256 is not represented as runtime-tested by the records above.

An unknown executable hash is not accepted as proof of compatibility.
Production compatibility gating requires a unique resolver match followed by
structural byte validation and, where applicable, decoded
instruction/operand validation. A changed image must still go through the
update workflow and runtime regression before support is claimed.

## Production resolver inventory

| Feature/role | Signature and implementation location | Uniqueness and validation | Relevant contract |
| --- | --- | --- | --- |
| Gameplay camera writer | `signatures::CameraWriter`; `gameplay::ResolveCameraWriter` in `src/gameplay/gameplay_camera.cpp` | Exactly one complete `.text` signature; Zydis decode validates `MOVSS [RBX+0x30], XMM0` | Callback contract: `RSI` is the source object and `RBX` is the output/target object; source fields use aspect `0x254` and flags `0x259`; writer input is `XMM0` |
| Cinematic aspect store | `signatures::CinematicAspectSetter`; `cinematics::ResolveAspectStore` called by `runtime.cpp` | `FindAll` must return one; store prefix and expected immediate are checked with byte comparisons; Zydis validates `MOV [RAX+0x254], imm` | Target aspect field `0x254`; expected original immediate `0x3FE38E39` |
| Cinematic ENTER FOV boundary | `signatures::CinematicEnter`; `ResolveCinematicFovCallsites` in `src/plugin/runtime.cpp` | Exactly one match; Zydis validates `MOVSS XMM0,[RIP+disp]`, a relative call and `EnterVcallPair` bytes | ENTER callsite and shared consumer target are validated |
| Cinematic EXIT FOV boundary (legacy) | `signatures::CinematicExit`; `ResolveCinematicFovCallsites` in `src/plugin/runtime.cpp` | Used first; exactly one match; Zydis validates `MOVSS XMM0,[RDI+0x38]`, a relative call and `ExitVcallPair` bytes | EXIT FOV sample at `[RDI+0x38]` |
| Cinematic EXIT FOV boundary (indexed fallback) | `signatures::CinematicExitIndexed`; `ResolveCinematicFovCallsites` in `src/plugin/runtime.cpp` | Used only when the legacy EXIT signature has no matches; exactly one match; Zydis validates `MOVSS XMM0,[RBX+RAX*4+0x38]`, a relative call and `ExitVcallPair` bytes | Indexed EXIT FOV sample at `[RBX+RAX*4+0x38]` |
| Dialogue boundary | `signatures::DialogueBoundary`; `InstallDialogueBoundary` in `src/plugin/runtime.cpp` | Exactly one match; validation is performed at the actual match `+9` hook location with a concrete six-byte `memcmp`. No Zydis decode is claimed. | Callback reads input FOV/value from `XMM6` and writes the transformed result to `XMM1`; boundary bytes are `FF 90 08 06 00 00` |

ENTER and EXIT call targets must resolve to the same executable target and be
executable. The indexed EXIT form is a fallback topology, not an additional
simultaneous hook. Relative displacements are intentionally wildcarded; fixed
addresses and raw RVAs are not portable evidence.

Signatures are owned by `src/hooks/signatures/`. The cinematic FOV and dialogue
resolver/control-flow integration remains in `src/plugin/runtime.cpp`, while
the reusable cinematic aspect resolver is in `src/cinematics/`.

## Callback and concurrency assumptions

- Gameplay camera callbacks receive the validated camera writer context:
  `RSI` is the source object, `RBX` is the output/target object, and the
  writer input is the `XMM0` value used by the validated `MOVSS` instruction.
  The relevant source/target state uses aspect `0x254` and flags `0x259`.
- Cinematic aspect application validates the target object and writable range
  before writing the owned aspect field.
- Dialogue callbacks read the incoming value from `XMM6` and write a valid
  transformed result to `XMM1`; the hook-site validation remains unique-match
  plus concrete byte validation rather than a decoded Zydis contract.
- Ordinary callback telemetry is thread-confined to the runtime/game owner
  thread. Atomic fields carry cross-thread observations where required.
- Worker lifecycle start/stop ownership belongs to one externally serialized
  Runtime control path; it need not be one physical thread. Workers wait on
  the Runtime-owned stop event and do not own joins.
- `DLL_PROCESS_DETACH` only signals stop; normal dynamic unload is unsupported.

## Update and regression evidence

For a new game build, re-establish executable identity, resolver uniqueness,
structural byte and applicable decoded-instruction validation, callback/register
contracts and field offsets, then complete the required runtime regression
before claiming support. Store static evidence in `research/reports/` and
runtime logs/evidence under the corresponding research record. Runtime support
is not extended to older or newer game builds without explicit executable
identity and regression evidence.
