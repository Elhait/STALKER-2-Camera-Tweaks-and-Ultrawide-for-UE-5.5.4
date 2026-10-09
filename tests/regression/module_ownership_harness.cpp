#include "../../src/camera/horplus.hpp"
#include "../../src/camera/presentation_state.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/cinematics/cinematic_selection.hpp"
#include "../../src/dialogue/dialogue_fov.hpp"
#include "../../src/dialogue/dialogue_state.hpp"
#include "../../src/gameplay/gameplay_state.hpp"
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool value, const char* label)
    {
        if (!value) std::cerr << "FAIL: " << label << '\n';
        return value;
    }
}

int main()
{
    bool pass = true;
    using camera::CoordinatorState;
    using config::DialogueZoomPolicy;
    constexpr std::uintptr_t source = 0x12340;
    constexpr float authored = 90.0f;
    const float target = camera::HorPlus(authored, 32.0f / 9.0f, 16.0f / 9.0f);
    const cinematics::CinematicWriterSample baseline{
        source, source, authored, authored, target,
        std::numeric_limits<float>::quiet_NaN(), true, false, true};

    // This harness is intentionally limited to local production-primitive
    // edge checks. Event orchestration and cross-module ownership coverage
    // lives in the production transaction scenario harnesses.
    const auto gameplayOwner = cinematics::DispatchCinematicWriter(
        CoordinatorState::Gameplay, true, baseline);
    pass &= Check(!gameplayOwner.handled,
        "cinematic_dispatch_does_not_claim_gameplay_ownership");

    cinematics::CinematicWriterAssociation association;
    association.BeginGeneration(11, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus });
    association.ObserveAspectStoreTarget(11, source);
    const auto matchingWriter = association.ObserveWriter(
        CoordinatorState::CinematicActive, source, 0xA000, authored,
        32.0f / 9.0f, true, 0x5, true);
    pass &= Check(matchingWriter.associationValid &&
        matchingWriter.associatedWriterRsi == source,
        "aspect_store_object_equal_to_writer_rsi_associates");

    // Initial Establishment authorization/output combinations are covered by
    // the C1 production transaction scenario; do not compose the same policy
    // from primitives here.
    cinematics::CinematicWriterAssociation mismatchAssociation;
    mismatchAssociation.BeginGeneration(12, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus });
    mismatchAssociation.ObserveAspectStoreTarget(12, source);
    auto wrongSource = mismatchAssociation.ObserveWriter(
        CoordinatorState::CinematicActive, source + 0x20, 0xA100, authored,
        32.0f / 9.0f, true, 0x5, true);
    pass &= Check(!wrongSource.associationValid &&
        wrongSource.associatedWriterRsi == 0,
        "mismatched_writer_rsi_does_not_associate");
    wrongSource = mismatchAssociation.ObserveWriter(
        CoordinatorState::CinematicActive, source + 0x20, 0xA100, authored,
        32.0f / 9.0f, true, 0x5, true);
    pass &= Check(!wrongSource.associationValid,
        "same_fov_with_wrong_rsi_never_associates");

    cinematics::CinematicWriterAssociation wrongOwner;
    wrongOwner.BeginGeneration(121, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus });
    wrongOwner.ObserveAspectStoreTarget(121, source);
    const auto nonCinematicOwner = wrongOwner.ObserveWriter(
        CoordinatorState::Gameplay, source, 0xA110, authored,
        32.0f / 9.0f, true, 0x5, true);
    pass &= Check(!nonCinematicOwner.associationValid &&
        nonCinematicOwner.associatedWriterRsi == 0 &&
        nonCinematicOwner.latestWriter.source == source,
        "matching_writer_is_observed_but_not_associated_without_cinematic_owner");

    association.BeginGeneration(13, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::NativeHorPlus });
    auto newGeneration = association.Read();
    pass &= Check(newGeneration.generation == 13 &&
        newGeneration.generationActive && !newGeneration.associationValid &&
        newGeneration.aspectStoreTargetObject == 0 &&
        newGeneration.associatedWriterRsi == 0 &&
        newGeneration.initialEstablishmentActive &&
        !newGeneration.initialEstablishmentEnded &&
        newGeneration.initialEstablishmentEndReason ==
            cinematics::InitialEstablishmentEndReason::None,
        "accepted_new_generation_resets_association_and_starts_establishment");
    association.ObserveAspectStoreTarget(13, source);
    association.ObserveWriter(CoordinatorState::CinematicActive, source,
        0xA200, authored, 32.0f / 9.0f, true, 0x5, true);
    const bool nextGenerationRetains =
        association.AuthorizeInitialEstablishmentRetention(
            CoordinatorState::CinematicActive, true, true, true, 13, source, authored);
    association.InvalidateGeneration(13);
    const auto exitedGeneration = association.Read();
    pass &= Check(!exitedGeneration.generationActive &&
        !exitedGeneration.associationValid &&
        exitedGeneration.associatedWriterRsi == 0 && nextGenerationRetains &&
        !exitedGeneration.initialEstablishmentActive &&
        exitedGeneration.initialEstablishmentEnded &&
        exitedGeneration.initialEstablishmentEndReason ==
            cinematics::InitialEstablishmentEndReason::CinematicExit,
        "matching_exit_invalidates_association_and_clears_establishment");

    cinematics::CinematicWriterAssociation conflictingTargets;
    conflictingTargets.BeginGeneration(14, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus });
    conflictingTargets.ObserveAspectStoreTarget(14, source);
    conflictingTargets.ObserveWriter(CoordinatorState::CinematicActive,
        source, 0xA300, authored, 32.0f / 9.0f, true, 0x5, true);
    auto conflict = conflictingTargets.ObserveAspectStoreTarget(14, source + 0x40);
    pass &= Check(conflict.associationConflict && !conflict.associationValid &&
        conflict.conflictingAspectStoreTargetObject == source + 0x40,
        "conflicting_aspect_store_target_fails_closed");

    cinematics::CinematicWriterAssociation conflictingOutput;
    conflictingOutput.BeginGeneration(15, authored, target,
        { true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus });
    conflictingOutput.ObserveAspectStoreTarget(15, source);
    conflictingOutput.ObserveWriter(CoordinatorState::CinematicActive,
        source, 0xA400, authored, 32.0f / 9.0f, true, 0x5, true);
    conflict = conflictingOutput.ObserveWriter(CoordinatorState::CinematicActive,
        source, 0xA401, authored, 32.0f / 9.0f, true, 0x5, true);
    pass &= Check(conflict.associationConflict && !conflict.associationValid,
        "same_rsi_with_conflicting_rbx_fails_closed");

    const auto unchangedDispatch = cinematics::DispatchCinematicWriter(
        CoordinatorState::CinematicActive, true, baseline);
    pass &= Check(unchangedDispatch.handled &&
        unchangedDispatch.resolution.action ==
            cinematics::CinematicWriterAction::RetainTransformedTarget &&
        std::fabs(unchangedDispatch.resolution.outputFov - target) < 0.001f,
        "valid_initial_retention_authorization_keeps_established_target");

    auto mismatched = baseline;
    mismatched.source += 0x10;
    mismatched.initialEstablishmentAuthorized = false;
    const auto sourceMismatch = cinematics::DispatchCinematicWriter(
        CoordinatorState::CinematicActive, true, mismatched);
    pass &= Check(sourceMismatch.handled &&
        sourceMismatch.resolution.action ==
            cinematics::CinematicWriterAction::PassThroughUnclassified &&
        sourceMismatch.resolution.outputFov == authored,
        "unconfirmed_cinematic_source_fails_closed_to_native");

    // Keep direct policy primitives local here. Cross-event Dialogue behavior
    // and Diagnostic/Enabled permutations are asserted through production
    // ProcessDialogueBoundary scenarios in the dedicated transaction harness.
    dialogue::CorrectionActivation activation;
    activation.AcceptedStart(false);
    pass &= Check(!activation.CanCorrect(false), "dialogue_off_start_observes_without_correction");
    pass &= Check(!activation.CanCorrect(false),
        "dialogue_off_candidate_primitive_does_not_arm_correction");
    activation.ModuleEnabledChanged(true);
    pass &= Check(!activation.CanCorrect(true),
        "dialogue_off_to_on_during_active_lifecycle_is_deferred");
    activation.AcceptedStart(true);
    pass &= Check(activation.CanCorrect(true),
        "next_accepted_dialogue_start_activates_saved_mode");
    activation.ModuleEnabledChanged(false);
    pass &= Check(!activation.CanCorrect(false) && !activation.CanCorrect(true),
        "dialogue_on_to_off_stops_correction_without_transferring_owner");

    const auto exitOffHorPlus = gameplay::ResolveCinematicExitTransition(
        true, false, config::GameplayMode::HorPlus);
    const auto exitOffAspect = gameplay::ResolveCinematicExitTransition(
        true, false, config::GameplayMode::AspectRecalculation);
    const auto exitOnHorPlus = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto exitOnAspect = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::AspectRecalculation);
    pass &= Check(exitOffHorPlus.nextState == CoordinatorState::CinematicExiting &&
        exitOffAspect.nextState == CoordinatorState::CinematicExiting &&
        exitOnHorPlus.nextState == CoordinatorState::CinematicExiting &&
        exitOnAspect.nextState == CoordinatorState::CinematicExiting &&
        !exitOffHorPlus.armGameplayHandoff && !exitOffAspect.armGameplayHandoff &&
        !exitOnHorPlus.armGameplayHandoff && exitOnAspect.armGameplayHandoff,
        "cinematic_exit_observation_is_shared_gameplay_handoff_is_module_specific");

    gameplay::HorPlusRecoverySample recovery{};
    recovery.source = source;
    recovery.validatedSource = source;
    recovery.inputFov = 90.0f;
    recovery.exitNativeTarget = 90.0f;
    recovery.aspect = 32.0f / 9.0f;
    recovery.flags = 0x04;
    recovery.cameraReadable = true;
    recovery.gameplayEnabled = false;
    const bool nativeValidated = gameplay::IsNativeHorPlusRecoverySample(recovery, 0.01f);
    const auto disabledRecovery = gameplay::ResolveHorPlusRecoveryAction(
        CoordinatorState::CinematicExiting, recovery, nativeValidated, 0.01f);
    pass &= Check(nativeValidated &&
        disabledRecovery == gameplay::HorPlusRecoveryAction::ResumeGameplay,
        "gameplay_off_still_completes_shared_native_recovery");

    std::cout << "module_ownership_primitives=" << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
