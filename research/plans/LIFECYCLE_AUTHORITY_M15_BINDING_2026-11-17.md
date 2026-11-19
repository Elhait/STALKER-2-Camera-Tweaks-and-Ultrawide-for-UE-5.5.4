# M1.5 Adapter Binding Verification

Authorized by the user's instruction following M1 hardening acceptance.
M1 hardening accepted; M2/M3 remain NO-GO. Offline only, no runtime wiring,
production/core/oracle/ASI/game changes. Native selection research is separate.

Inspect actual callback capture, admission, execution, readback, early-return
and settings boundaries. Verify binding candidates using existing production
transactions and the isolated Authority; explicitly report divergences and
unsupported bindings. Exercise effect closure under normal and interrupted
control flow with Diagnostics OFF/ON. Do not infer live guarantees from fixture
finalizers or set evidence flags from production lifecycle verdicts.

Deliverables: test-only binding counterexamples, bounded supervised results,
source-backed event/finalization mapping, parity coverage limits and M2 verdict.
An evidenced interface incompatibility blocks only dependent parity claims;
do not patch the isolated core speculatively or call the whole binding PASS.

Status: bounded verification produced reproducible incompatibilities. Readiness
failed; dependent full differential parity is incomplete for the documented
interface reasons. Report: ../reports/LIFECYCLE_AUTHORITY_M15_BINDING_2026-11-17.md.
