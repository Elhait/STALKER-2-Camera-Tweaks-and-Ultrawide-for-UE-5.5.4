# Native evidence and independent transition model

Status: bounded research checks completed; findings await discussion.
Authority migration is not authorized. Full concrete native-guard/ABI/C++ path
verification is not claimed by the abstract model.
Input contract: Lifecycle Authority design revision 2 and native evidence audit.

Two sequential workstreams in this chat, without delegated agents:
1. Bounded immediate dataflow inspection of existing writer, cinematic and
   Dialogue/provider hook inputs plus archived native disassembly. Seek positive
   new-source Gameplay identity and callback-independent Dialogue termination.
   Do not invent proof from coordinator labels, numbers or silence.
2. Independent research-only transition model, separate from production. Map
   T01–T36 individually, including guards/rejects, atomic commit/early abort,
   effects, scoped claims and source interaction. Generate sequences, report
   shortest discovered invariant counterexamples and reproduce known defect
   classes with deliberate faulty-model variants. Clearly identify abstractions,
   inactive paths and transition details not specified by the design.

Outputs: research report and evidence directory
`lifecycle-transition-model-2026-11-17/` with reproducible checker, coverage,
bounded search results, source identities and native evidence findings.
No production/tests/settings/hooks/ASI changes, builds, test.cmd, game runs,
new native hooks or whole-image/Unreal searches. Normative design stays frozen.

Validation budget: checker in functions.exec V8, no imports/native subprocesses,
no callbacks/network/I/O inside model; max 40,000 visited states, 600,000 edges,
depth 8, 15 seconds per search, finite fault variants. Fail explicitly on budget
limit; state what was explored, not all possible engine sequences. File reads
and extraction lightweight/sequential. No persistent native processes.

Acceptance: answers for both evidence gaps, per-ID transition/branch coverage,
stateful independent safety and conditional progress assertions, minimized
counterexamples where found, explicit gaps/assumptions and unchanged source/ASI.
An abstract checker PASS is not native reachability, production correctness,
universal liveness or approval to migrate. No new engine run requested.

## Executed outcome

See [report](../reports/LIFECYCLE_EVIDENCE_AND_TRANSITION_MODEL_2026-11-17.md).

- Native investigation complete within the bounded archived dataflow: camera
  weak-reference binding, blend completion/delegate cleanup and same-component
  rearming established for the pinned-image graph. Neither independent new-source
  Gameplay role nor callback-silent Dialogue END established. These are explicit
  evidence gaps, not proof that another hook is necessary.
- All T01–T36 mapped to research handlers and coverage dispositions. Focused BFS
  depth6, all-family BFS depth3, ten accepted-state seeds, deterministic streams,
  guard/abort fixtures and a 32-event lifecycle scenario executed. No assertion
  violations in final relation within those domains. Concrete numerical/native
  guards remain abstract assumptions; legacy/experimental profiles excluded.
- Seven faulty-model variants produce shortest-in-alphabet counterexamples.
  A provider-quarantine omission in the research relation was found through
  seeded next-transaction search, recorded red and corrected to the existing
  design contract. This is not a production finding.
- Scope preserved: source/design/ASI unchanged; research records only, no
  production suite/build/native process launched. No authority migration started.

Follow-up disposition: native role/termination gaps require a separately scoped
positive observation path; concrete adapter-to-proof/access-map verification
belongs before claiming implemented Authority guarantees. Neither is silently
closed by abstract PASS or turned into an automatic new-hook task.
