#include "../../src/diagnostics/dialogue_source_provenance.hpp"

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
