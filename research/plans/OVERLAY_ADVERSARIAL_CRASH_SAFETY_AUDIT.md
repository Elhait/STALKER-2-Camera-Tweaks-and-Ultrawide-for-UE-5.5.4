# Overlay adversarial crash-safety audit — 2026-09-27

Authority: user attachment d864b7b7-68a9-42ff-8cc3-6a5a2b18034d. Audit current repaired production paths; repair only concrete defects, not speculative hardening. Existing uncommitted startup repairs and user artwork are preserved.

## Invariant and scope

Optional Overlay failures disable UI without invalidating core camera initialization or native DXGI pass-through. GPU submission/lifetime errors must be prevented, not disguised by exception handling. Device loss already caused in native rendering cannot be contained by C++ catches.

Audit callbacks/ABI, table ownership, native lifetime and concurrency, renderer/backends, input subclassing, initialization/logging, wrappers and failure paths. No game launch, Git mutation, packaging, camera/config/UX/localization changes.

## Batches

1. Establish new findings from source and SDK/API contracts; distinguish confirmed defects, plausible paths, hardening and checked non-issues.
2. Repair confirmed backend font-upload/partial-init, renderer GPU lifetime and frame-buffer ownership defects with focused offline fixtures. Preserve presentation.
3. Repair confirmed terminal-input/native boundary synchronization defects; review all remaining audit areas and document speculative limits separately.
4. Run focused harnesses, full test entry point and production build into ignored audit output. Review changed paths/diff; produce report and runtime A/B disposition. Do not claim field crashes fixed.

## Expected files and evidence

Renderer/discovery, narrowly adapted vendored ImGui D3D12 backend, focused overlay tests and runner, safety/architecture contracts; report in research/reports/OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md. Vendor changes remain separately identified. No general UI or graphics framework. Final review also confirmed noexcept settings API callback forwarding can terminate before Overlay isolation: include runtime_settings.hpp/readback wrapper and existing API harness, without changing mutation/config/camera semantics.

Confirmed source evidence: font upload uses unchecked/assert-only HRESULTs and INFINITE wait; backend teardown dereferences unpublished frame resource array on partial allocation; renderer releases GPU-visible references after failed completion wait; backend upload-buffer ring is based on draw-call ordinal, but renderer fences are indexed by actual backbuffer; terminal input path still accepts Toggle after renderer disable.

Acceptance: failures do not submit invalid/null resources, no unbounded project/backend fence wait, incomplete GPU work does not lose last owning resource refs, upload-buffer reuse follows the waited frame slot, disabled UI cannot reacquire input, native results/arguments preserved. Deterministic coverage is not real GPU/wrapper/runtime validation. Pending GPU refs may require terminal process-lifetime retention; this tradeoff must be explicit.

Status: audit/authorized confirmed repairs and final validation complete (51/51 full harnesses, focused failures, 18/182 catalogs, offline fonts, production build, diff review). Report contains AS-01–AS-10, conditional risks and runtime A/B disposition. No game/field reproduction claimed; awaiting user closure, no task-log entry or release action.
