# Research Provenance and Ghidra Blast-Radius Audit — Task Plan

Status: Batch 2 complete; Batch 3 queue ready

## Objective

Determine which technical conclusions across the project may depend on the
stale Ghidra image, preserve their original provenance, and produce one master
blast-radius report with a prioritized revalidation queue.

## Established evidence and current state

- The canonical Ghidra project is `Dump/STALKER2-Ghidra`.
- Its `Stalker2-Win64-Shipping.exe (2.0.4)` entry matches the current runtime
  image identity: SHA-256
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`, image
  base `0x140000000`, and `.text` size `130818560`.
- An earlier static ADS audit used a stale image with `.text` size
  `130803712`; its static conclusions are not valid current-2.0.4 evidence.
- Runtime observations captured from the actual 2.0.4 executable remain
  evidence unless their tracer target or interpretation depended exclusively
  on the stale static image.

## Approved scope

Audit the full technical-claim chain in:

- `01-Projects/.../research/completed`
- `01-Projects/.../research/rejected`
- `01-Projects/.../research/deferred`
- `01-Projects/.../backlog`
- `02-Research/Ghidra/reports`
- `02-Research/Ghidra/workspace`
- `TESTING_AND_RESEARCH.md`
- `backlog/TASKLOG.md`

Classify each relevant conclusion as `RUNTIME`, `STATIC`, `MIXED` or
`CROSS-VERSION STATIC`; establish actual Ghidra image identity where possible;
and preserve original conclusion, provenance, classification, reason and
required action. Track runtime artifact validity separately from semantic
conclusion validity, and assign decision impact (`A` may affect
production/release safety, `B` may have closed or deferred a research branch,
`C` affects the technical explanation/model, `D` is documentation/navigation
only). Use `runtime artifact: VALID` with `semantic conclusion: UNKNOWN` when
the artifact executed on 2.0.4 but its target or interpretation may have come
from stale static mapping. Record `branch consequence` separately, such as
`none`, `reopen branch`, `repeat mapping`, `recheck hook ownership`, `release
contract revalidation` or `documentation correction`.

## Explicit non-goals

- Do not modify production C++/ASI/INI logic.
- Do not create runtime tracers or launch the game.
- Do not silently rewrite or delete historical research.
- Do not infer image identity from script, report, task-plan or `204` suffixes.
- Do not revalidate every technical claim before the blast-radius map exists.

## Batches

### Batch 1 — Inventory and provenance extraction — completed

Inventory reports, plans, research files, scripts and summaries. Extract image
names, SHA values, `.text` sizes, RVAs, runtime-log references and technical
claims, with special attention to negative and cross-version language.

Validation: inventory found 10 rejected, 58 completed, 3 deferred and 6 active
task plans, plus 32 Ghidra report/text artifacts. Initial high-risk claims were
listed in the master blast-radius report without using filename suffixes as
identity proof.

### Batch 2 — Evidence classification — completed

Build the master blast-radius report. Use these rules:

- actual runtime evidence from 2.0.4 is `VALID` unless its target depended only
  on stale static analysis;
- stale static positive results are `REVALIDATE`;
- stale static negative results are `INVALIDATED / UNKNOWN`;
- stale or mixed cross-version comparisons are `REPEAT`;
- unidentifiable provenance is `UNKNOWN`.

Validation: every classified item records original conclusion, actual image or
unknown provenance, runtime artifact status where applicable, semantic
conclusion status, decision impact, branch consequence, reason and action.

Batch result: the master report records the package-wide triage and separates
document-level inventory from technical-conclusion status. It records 77 task
plans, 87 report/log/text artifacts, three stale-fingerprint artifacts outside
the audit itself, 43 report/log artifacts with current SHA but no full identity
contract, and 39 without a current or stale identity marker. The explicit
2.0.3 branch-closing set is listed for matching-image repetition. No historical
research file was silently rewritten, no runtime tracer was created and no
production path was changed.

### Batch 3 — Revalidation queue — ready, not executed in this task

Prioritize by impact and uncertainty: `A + UNKNOWN/REVALIDATE`,
`B + INVALIDATED/UNKNOWN`, `A + semantic mismatch`, `B + stale negative`, then
`C`, then `D`. Select only the claims that need matching-image reanalysis.

Validation: queue is derived from the master report and does not silently
promote or close any research branch. Priority is `A + UNKNOWN/REVALIDATE`,
`B + INVALIDATED/UNKNOWN`, `A + semantic mismatch`, `B + stale negative`,
then `C` and `D`.

### Batch 4 — Final review

Review changed paths, report consistency and stale-image labeling. Update the
task log only after the scope and classification are compared with this plan.

Validation: no production/runtime changes; historical provenance preserved.

## Risks and safe-failure behavior

- The audit may reopen previously rejected or deferred research; this is an
  intended outcome, not permission to implement it.
- If image identity cannot be established, retain the item as `UNKNOWN` and do
  not treat it as either confirmed or rejected.
- If evidence types conflict, preserve both and escalate the item to
  `REVALIDATE` rather than resolving it by assumption.

## Stop conditions and phase gates

- Stop static interpretation when the matching image identity is not proven.
- Stop before runtime tracing or production changes; those require a separate
  approved task.
- Stop after the master blast-radius report and revalidation queue are complete;
  do not expand into reanalysis in this task.

## Expected final Git review

Review the master report, this plan, any explicitly approved navigation/index
updates and the task-log entry. Confirm that no production ASI, runtime tracer,
or unrelated historical file was changed.
