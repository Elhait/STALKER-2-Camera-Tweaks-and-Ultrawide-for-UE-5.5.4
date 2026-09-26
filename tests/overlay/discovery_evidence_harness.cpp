#include "../../src/overlay/discovery_evidence.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }
}

int main()
{
    bool pass = true;
    overlay::FactoryEvidenceStore factories;
    pass &= Check(!factories.ObserveFactory(0), "null_factory_fails_closed");
    pass &= Check(factories.ObserveFactory(0x5000), "factory_added");
    pass &= Check(!factories.ObserveFactory(0x5000) &&
        factories.FactoryCount() == 1, "duplicate_factory_ignored");
    pass &= Check(factories.RecordMethod(0x5000,
        overlay::FactoryMethod::CreateSwapChain), "method_recorded");
    pass &= Check(!factories.RecordMethod(0x5000,
        overlay::FactoryMethod::CreateSwapChain), "duplicate_method_ignored");
    pass &= Check(factories.RecordMethod(0x5000,
        overlay::FactoryMethod::CreateSwapChainForHwnd) &&
        factories.RecordMethod(0x5000,
            overlay::FactoryMethod::CreateSwapChainForCoreWindow) &&
        factories.RecordMethod(0x5000,
            overlay::FactoryMethod::CreateSwapChainForComposition) &&
        factories.Methods(0x5000) == 15u, "method_coverage_bookkeeping");
    pass &= Check(!factories.RecordMethod(0x6000,
        overlay::FactoryMethod::CreateSwapChain), "unknown_factory_method_rejected");

    overlay::QueueEvidenceStore lifecycleEvidence;
    lifecycleEvidence.BeginSwapchain(0x1000);
    pass &= Check(lifecycleEvidence.ObserveCandidate(0x1000, 0x2000, 0x3000, 0),
        "resize_candidate_added");
    lifecycleEvidence.ObservePresent(0x1000);
    pass &= Check(lifecycleEvidence.State(0x1000) ==
        overlay::AssociationState::Supported, "resize_initial_supported");
    lifecycleEvidence.BeginResize(0x1000);
    pass &= Check(lifecycleEvidence.State(0x1000) ==
        overlay::AssociationState::ResizeRevalidationRequired,
        "resize_requires_revalidation");
    lifecycleEvidence.CompleteResize(0x1000, true);
    pass &= Check(lifecycleEvidence.State(0x1000) ==
        overlay::AssociationState::ResizeRevalidationRequired,
        "resize_retains_structural_association");
    lifecycleEvidence.ObservePresent(0x1000);
    pass &= Check(lifecycleEvidence.HasSufficientAssociation(0x1000),
        "resize_revalidated_on_present");

    lifecycleEvidence.BeginResize(0x1000);
    lifecycleEvidence.CompleteResize(0x1000, true);
    lifecycleEvidence.ObservePresent(0x1000);
    lifecycleEvidence.BeginResize(0x1000);
    lifecycleEvidence.CompleteResize(0x1000, true);
    lifecycleEvidence.ObservePresent(0x1000);
    pass &= Check(lifecycleEvidence.HasSufficientAssociation(0x1000),
        "multiple_resizes_revalidated");

    overlay::QueueEvidenceStore recreationEvidence;
    recreationEvidence.BeginSwapchain(0x1000);
    recreationEvidence.ObserveCandidate(0x1000, 0x2000, 0x3000, 0);
    recreationEvidence.ObservePresent(0x1000);
    recreationEvidence.BeginSwapchain(0x2000);
    pass &= Check(!recreationEvidence.HasSufficientAssociation(0x2000),
        "new_swapchain_does_not_inherit_association");
    pass &= Check(recreationEvidence.ObserveCandidate(0x1000, 0x2100, 0x3000, 0) &&
        recreationEvidence.State(0x1000) == overlay::AssociationState::Ambiguous,
        "device_change_invalidates_old_association");

    overlay::QueueEvidenceStore failureEvidence;
    failureEvidence.BeginSwapchain(0x3000);
    failureEvidence.ObserveCandidate(0x3000, 0x4000, 0x5000, 0);
    failureEvidence.ObservePresent(0x3000);
    failureEvidence.BeginResize(0x3000);
    failureEvidence.CompleteResize(0x3000, false);
    pass &= Check(!failureEvidence.HasSufficientAssociation(0x3000),
        "resize_failure_invalidates_association");

    overlay::QueueEvidenceStore ambiguousEvidence;
    ambiguousEvidence.BeginSwapchain(0x4000);
    ambiguousEvidence.ObserveCandidate(0x4000, 0x5000, 0x6000, 0);
    ambiguousEvidence.ObserveCandidate(0x4000, 0x5000, 0x7000, 0);
    ambiguousEvidence.ObservePresent(0x4000);
    ambiguousEvidence.BeginResize(0x4000);
    ambiguousEvidence.CompleteResize(0x4000, true);
    ambiguousEvidence.ObservePresent(0x4000);
    pass &= Check(ambiguousEvidence.State(0x4000) ==
        overlay::AssociationState::Ambiguous,
        "resize_does_not_resolve_multiple_candidates");

    overlay::ObservationWindow window;
    window.Arm(2);
    pass &= Check(window.Open() && window.Consume() && window.Consume() &&
        !window.Consume() && !window.Open(), "bounded_window");
    window.Arm(1);
    pass &= Check(window.Consume() && !window.Consume(), "window_rearm");

    overlay::QueueEvidenceStore evidence;
    evidence.BeginSwapchain(0x1000);
    pass &= Check(evidence.ObserveCandidate(0x1000, 0x2000, 0x3000, 0),
        "candidate_added");
    pass &= Check(!evidence.ObserveCandidate(0x1000, 0x2000, 0x3000, 0) &&
        evidence.CandidateCount(0x1000) == 1,
        "duplicate_candidate_ignored");
    pass &= Check(!evidence.HasSufficientAssociation(0x1000),
        "present_required");
    evidence.ObservePresent(0x1000);
    pass &= Check(evidence.HasSufficientAssociation(0x1000),
        "single_candidate_associated");

    pass &= Check(evidence.ObserveCandidate(0x1000, 0x2000, 0x4000, 0) &&
        evidence.CandidateCount(0x1000) == 2 &&
        !evidence.HasSufficientAssociation(0x1000),
        "multiple_candidates_ambiguous");
    evidence.BeginSwapchain(0x1000);
    pass &= Check(evidence.CandidateCount(0x1000) == 0 &&
        !evidence.HasSufficientAssociation(0x1000),
        "recreation_invalidates_evidence");
    pass &= Check(!evidence.ObserveCandidate(0, 0x2000, 0x3000, 0),
        "null_candidate_fails_closed");

    std::cout << "Overlay discovery evidence harness: "
        << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
