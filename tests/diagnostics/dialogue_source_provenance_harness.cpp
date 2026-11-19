#include "../../src/diagnostics/dialogue_source_provenance.hpp"
#include "../../src/diagnostics/dialogue_boundary_observation.hpp"

#include <iostream>
#include <string_view>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }
}

int main()
{
    diagnostics::DialogueSourceProvenanceTrace trace;
    bool pass = true;
    const diagnostics::DialogueWriterSnapshot writerA{ true, 12, 3, 0xA, 0xA0 };

    diagnostics::DialogueBoundaryObservationGate observationGate;
    observationGate.Arm(7);
    diagnostics::DialogueBoundaryObservation boundary{
        1, 7, 0xD, 70.0f, 2, 0, 2, true,
        {true, 44, 5, 0xA, 0xA0, 70.0f, 3.55556f, 0x4, true, true}};
    pass &= Check(!observationGate.Observe(false, boundary) &&
        observationGate.Emitted() == 0,
        "disabled_diagnostics_do_not_emit_or_consume_dialogue_capture");
    const auto firstBoundary = observationGate.Observe(true, boundary);
    pass &= Check(firstBoundary && firstBoundary->callbackFov == 70.0f &&
        firstBoundary->latestWriter.rawFov == 70.0f &&
        firstBoundary->cinematicGeneration == 7,
        "boundary_record_keeps_callback_value_writer_snapshot_and_generation");
    pass &= Check(diagnostics::CompareRawAddresses(0xD, 0xA, true) ==
            diagnostics::RawAddressRelation::Different &&
        diagnostics::CompareRawAddresses(0xA, 0xA, true) ==
            diagnostics::RawAddressRelation::Equal &&
        diagnostics::CompareRawAddresses(0xD, 0, false) ==
            diagnostics::RawAddressRelation::Unavailable,
        "raw_address_relation_is_explicit_and_never_claims_engine_identity");
    boundary.callbackSequence = 2;
    pass &= Check(!observationGate.Observe(true, boundary),
        "unchanged_callback_samples_are_suppressed");
    boundary.callbackSequence = 3;
    boundary.callbackFov = 71.0f;
    const auto changedBoundary = observationGate.Observe(true, boundary);
    pass &= Check(changedBoundary && changedBoundary->callbackFov == 71.0f,
        "material_callback_fov_change_is_emitted");
    observationGate.Disarm();
    boundary.callbackSequence = 4;
    boundary.callbackFov = 72.0f;
    pass &= Check(!observationGate.Observe(true, boundary),
        "new_cinematic_enter_disarms_previous_dialogue_window");

    diagnostics::DialogueBoundaryObservationGate boundedGate;
    boundedGate.Arm(9);
    for (std::uint64_t i = 1; i <= diagnostics::DialogueBoundaryObservationGate::RecordLimit + 8; ++i) {
        auto sample = boundary;
        sample.cinematicGeneration = 9;
        sample.callbackSequence = i;
        sample.callbackFov = static_cast<float>(i);
        static_cast<void>(boundedGate.Observe(true, sample));
    }
    pass &= Check(boundedGate.Emitted() ==
            diagnostics::DialogueBoundaryObservationGate::RecordLimit,
        "dialogue_observation_records_have_a_hard_bound");

    auto cinematicCandidate = trace.Callback(true,
        diagnostics::DialogueSourceStage::Candidate, 0xD,
        "CinematicActive", writerA);
    pass &= Check(cinematicCandidate && cinematicCandidate->generation == 0 &&
        cinematicCandidate->owner == std::string_view("CinematicActive") &&
        !cinematicCandidate->lifecycleSourceKnown,
        "candidate_observation_records_cinematic_owner_without_accepting_dialogue");
    pass &= Check(!trace.Accepted(false, 0xA, "Gameplay", writerA),
        "diagnostic_off_does_not_capture_accepted_source");
    auto accepted = trace.Accepted(true, 0xA, "Gameplay", writerA);
    pass &= Check(accepted && accepted->marker == std::string_view("DIALOGUE_SOURCE_ACCEPTED") &&
        accepted->generation == 1 && accepted->lifecycleSource == 0xA &&
        accepted->latestWriter.identityEpoch == 3 &&
        accepted->latestWriter.rsi == 0xA && accepted->latestWriter.rbx == 0xA0,
        "accepted_event_keeps_dialogue_and_shared_writer_observations_separate");

    auto activeA = trace.Callback(true, diagnostics::DialogueSourceStage::Active,
        0xA, "Gameplay", writerA);
    pass &= Check(activeA && activeA->marker == std::string_view("DIALOGUE_SOURCE_CALLBACK") &&
        !activeA->pointerMismatch,
        "active_callback_same_observed_pointer_is_recorded_without_identity_claim");
    pass &= Check(!trace.Callback(false, diagnostics::DialogueSourceStage::Active,
        0xB, "Gameplay", { true, 13, 4, 0xB, 0xB0 }),
        "diagnostic_off_callback_does_not_update_trace");

    const diagnostics::DialogueWriterSnapshot writerB{ true, 14, 4, 0xB, 0xB0 };
    auto activeB = trace.Callback(true, diagnostics::DialogueSourceStage::Active,
        0xB, "Gameplay", writerB);
    pass &= Check(activeB && activeB->marker == std::string_view("DIALOGUE_SOURCE_MISMATCH") &&
        activeB->pointerMismatchKnown && activeB->pointerMismatch &&
        activeB->lifecycleSource == 0xA && activeB->callbackSource == 0xB &&
        activeB->latestWriter.sequence == 14 && activeB->latestWriter.identityEpoch == 4 &&
        activeB->latestWriter.rsi == 0xB && activeB->latestWriter.rbx == 0xB0,
        "active_foreign_pointer_is_diagnostic_only_and_includes_latest_writer_snapshot");
    auto exitingB = trace.Callback(true, diagnostics::DialogueSourceStage::Exiting,
        0xB, "Gameplay", writerB);
    pass &= Check(exitingB && exitingB->marker == std::string_view("DIALOGUE_SOURCE_MISMATCH") &&
        exitingB->stage == diagnostics::DialogueSourceStage::Exiting,
        "exiting_callback_reports_observed_pointer_mismatch");

    auto recovery = trace.RecoveryStarted(true, 0xB, "Gameplay", writerB);
    pass &= Check(recovery && recovery->marker == std::string_view("DIALOGUE_RECOVERY_SOURCE") &&
        recovery->lifecycleSource == 0xA && recovery->callbackSource == 0xB &&
        recovery->pointerMismatch,
        "recovery_source_is_recorded_separately_from_accepted_source");
    auto returnedA = trace.Callback(true, diagnostics::DialogueSourceStage::Exiting,
        0xA, "Gameplay", writerA);
    pass &= Check(returnedA && returnedA->marker == std::string_view("DIALOGUE_SOURCE_CALLBACK") &&
        !returnedA->pointerMismatch,
        "return_to_observed_source_is_reported_without_rebinding");
    auto ended = trace.Ended(true, "test-end", "Gameplay", writerA);
    pass &= Check(ended && ended->marker == std::string_view("DIALOGUE_SOURCE_END") &&
        ended->generation == 1 && ended->lifecycleSource == 0xA,
        "lifecycle_end_closes_diagnostic_generation");
    pass &= Check(!trace.Ended(true, "duplicate-end", "Gameplay", writerA),
        "duplicate_end_is_suppressed");

    auto next = trace.Accepted(true, 0xC, "CinematicActive", writerB);
    pass &= Check(next && next->generation == 2 && next->lifecycleSource == 0xC,
        "new_accepted_dialogue_gets_new_diagnostic_generation");
    std::cout << "dialogue_source_provenance=" << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
