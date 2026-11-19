# Lifecycle Authority M1: native binding sheet

Date: 2026-11-17. **Offline review only; no live adapter is connected.**
The sheet binds current observed native/source boundaries to candidate M1 facts.
It records both what can be supplied and what must remain Unknown.

## Identities and evidence surfaces

- Reviewed game image: SHA-256
  `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Current source anchors: `src/plugin/runtime.cpp`,
  `src/dialogue/post_exit_provider_handoff.hpp`,
  `src/plugin/camera_writer_transaction.hpp`, and the native data-flow record
  `research/reports/POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md`.
- R1 remains frozen at model
  `E5F520535514C041E0422164131DC9E09D2160D0DE415D986E6C4C8DDAA529ED` and
  result `F9A8447BD5E80AD31008DD3AE26FDA6D76277C29CF8B2603416F4E9B9E35C32D`.
- `AuthorityEventId` in M1 identifies an accepted event in our relation only.
  It does not identify Unreal allocation, world lifetime, or native activation.

## Facts and guards

| M1 fact | Current source/native evidence | Established relation | Permission limit / expiry |
| --- | --- | --- | --- |
| `callerValidated` for provider arm | `runtime.cpp` provider producer capture (around lines 4195–4271) requires `BlendCallerKind::ProviderProducer`, recovery owner/source checks, settings gate, and a successful provider armer call. Native arming/tick structure is decoded in `POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md` §§2–3. | The producer boundary can supply caller kind, component, receiver, thread, start, target, and receiver continuity value. | Proves this observed producer call passed the current adapter guards. Does not prove global provider coverage or an engine lifetime token. Current handoff object can be superseded/reset by later runtime paths. |
| Setter boundary / argument readable | Game image RVA `0xCCE673` invokes receiver vtable slot `+0x608`; `0xCCE679` compares the blend result with target. The hook is `TraceDialogueBoundary` in `runtime.cpp` around line 5258. | A callback is a native FOV blend setter call with receiver, blend component/context, and an argument at that boundary. | It is not proof that the callback is a true Dialogue, and its blend argument is not necessarily original/provider FOV. Component pointer is `T-0x28`; do not confuse it with the receiver. |
| Arming-to-setter causal join | Static dataflow shows provider `+0x2CF8` retains/reuses the blend component; a component can be reused for another target. Existing runtime stores provider generation/component/receiver/thread and checks trajectory. | The existing candidate path has compatibility relations and trajectory checks. | No independent native arming episode ID has been found. Same component/thread/generation tuple is insufficient. M1 keeps causal join as unavailable unless a specifically evidenced relation is supplied; allocating an Authority ID does not fill it. |
| Writer boundary / prepared input | Primary writer hook at `0x9CD2EB` reads `[RSI+0x230]` and writes `[RBX+0x30]`; `ProcessCameraWriter` at `runtime.cpp` around line 7070. The native copy and optional `+0x2D4` branch are in the U2 writer inventory. | Per-call source/output and prepared input can be observed at the validated hook; an output readback can be named by its observation boundary. | Writer input can be upstream-modified by Dialogue setter or Cinematic preparation. It does not prove original FOV, Gameplay role, or persistent output identity. |
| Provider writer confirmation | Runtime takes a writer sample and evaluates provider trajectory before `CompleteRecovery` around lines 7031–7150; handoff helper currently owns its own mutable matching/consumption state. | The current implementation can confirm a bounded producer→setter→writer trajectory when its guards and source identities match. | This does not distinguish repeated native episodes sharing a tuple without a causal join. M1 must reject/leave unresolved a sample with only tuple equality. |
| Source-owned recovery endpoint | Existing `ProcessHorPlusRecovery` / `CompleteCameraWriterRecovery` path is called from the writer transaction; the native evidence audit documents validated source, source match, flags/readability and endpoint constraints. | A supported EXIT for a known source plus its accepted native recovery endpoint can complete that bounded recovery claim. | It does not establish a general Gameplay role for a new source. Endpoint completion and Dialogue END remain different events. |
| Gameplay role / original input | Primary writer exposes address/shape/values only. Existing logs and Camera State labels such as `GameplayEligible` are mod classifications. | A prior trusted baseline can be retained/invalidated by receiver and settings identity; a source-bound known recovery endpoint is bounded evidence for its own transfer. | **Independent Gameplay bootstrap for an arbitrary new source is not established.** M1 requires explicit evidence and otherwise leaves baseline Unknown; it may not infer from `+0x230`, `+0x234`, flags, or FOV equality. |
| Dialogue terminal | Blend target completion can clear provider `+0x2CF8` at RVA `0x31FD92E`; `runtime.cpp` provider reset/complete path is separate. | Native blend/provider cleanup is observable for that technical activation. | Blend completion or callback silence is not Dialogue END. No general terminal binding is established when setter callbacks cease. |
| Effect intent/result | Current writer transaction exposes planned decision and separate diagnostic post-store/post-helper observations; completion bookkeeping occurs after the writer result. | M1 stores a per-call intent and accepts only a named, correlated result with matching transaction, scope, receiver, generation, boundary, settings epoch, and dependency revision. Post-store and post-helper are retained independently. | M1 never performs a native effect. `Applied` is only an observation label after a supplied matching boundary result; partial/refused/unknown do not roll back unrelated lifecycle state. Current native publication does not yet establish that every adapter can supply all these identity fields; that binding is still an M2 gate. |
| Settings/reset | Settings mutations and runtime reset are serialized through the current Runtime control path (`ApplyCameraSetting`, around line 8465; startup camera settings around 9114–9127). | An accepted relevant settings event advances the model epoch and revokes pending permission/intents. | This is a source-level binding candidate, not proof every native callback has been migrated. Persisted config is not lifecycle ownership. |

## M1 disposition

The isolated core may model explicit evidence fields and fail closed when they
are absent. The current evidence supports bounded source-owned EXIT/recovery
and the observed producer→setter→writer relation, but **does not** support a
general new-source Gameplay proof, universal Dialogue terminal, or unique
native arming episode after component reuse. Those are M2 gates, not facts to
be guessed in an adapter. No new hook is proposed here.
