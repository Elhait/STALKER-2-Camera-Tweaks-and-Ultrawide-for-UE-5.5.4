#include "../../src/plugin/dialogue_boundary_transaction.hpp"
#include "../../src/plugin/camera_writer_transaction.hpp"
#include "../../src/dialogue/post_exit_provider_authorization.hpp"
#include "../../src/dialogue/post_exit_provider_handoff.hpp"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
    bool Check(bool value, const char* label)
    {
        if (!value) std::cerr << "FAIL: " << label << '\n';
        return value;
    }

    struct Fixture
    {
        std::mutex mutex;
        std::atomic<camera::CoordinatorState> coordinator{camera::CoordinatorState::Gameplay};
        dialogue::PostCinematicRecoveryExclusion exclusion;
        dialogue::Phase phase{dialogue::Phase::Inactive};
        bool postExitProviderOwned{};
        std::uintptr_t postExitProviderCallbackSource{};
        std::uintptr_t postExitProviderReceiver{};
        std::uint64_t postExitProviderGeneration{};
        std::uint64_t postExitProviderSettingsEpoch{};
        std::uint32_t postExitProviderThreadId{};
        dialogue::PolicySnapshot activePolicy;
        dialogue::CorrectionActivation activation;
        float baseline{NAN};
        float previous{NAN};
        float exitStart{NAN};
        dialogue::CandidateTracker candidate;
        dialogue::RecoveryRearm recovery;
        std::atomic<bool> enabled{true};
        std::atomic<config::DialogueZoomPolicy> selectedPolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<std::uint64_t> settingsEpoch{1};
        std::atomic<std::uint64_t> activeEpoch{};
        bool dialogueNonNativeCapabilityAvailable{true};
        std::atomic<std::uintptr_t> snapshotSource{};
        std::atomic<float> snapshotTarget{NAN};
        std::atomic<bool> snapshotTargetValid{};

        plugin::DialogueBoundaryStateView View()
        {
            return {mutex, coordinator, exclusion, phase, postExitProviderOwned,
                postExitProviderCallbackSource, postExitProviderReceiver,
                postExitProviderGeneration, postExitProviderSettingsEpoch,
                postExitProviderThreadId,
                activePolicy, activation,
                baseline, previous, exitStart, candidate, recovery, enabled,
                selectedPolicy, settingsEpoch, activeEpoch, snapshotSource,
                snapshotTarget, snapshotTargetValid};
        }

        plugin::DialogueBoundaryResult Event(float fov, std::uintptr_t source,
            float target = 70.0f, bool targetValid = true, std::uintptr_t receiver = 0)
        {
            auto gate = correctionGate.Acquire();
            std::lock_guard lock(mutex);
            plugin::DialogueBoundaryInput input{fov, source, target, targetValid,
                0.01f, 0.02f, 3, false, receiver};
            input.threadId = 7;
            return plugin::ProcessDialogueBoundary(View(), input);
        }

        plugin::CameraCorrectionGate correctionGate;
    };

    void SelectGameplayMode(void*, config::GameplayMode) {}

    struct SettingsFixture
    {
        std::atomic<config::GameplayMode> gameplayMode{config::GameplayMode::HorPlus};
        std::atomic<bool> gameplayEnabled{true};
        std::atomic<std::uint64_t> gameplayEpoch{};
        std::atomic<bool> gameplayEnablePending{};
        std::atomic<bool> gameplayDisablePending{};
        std::atomic<bool> gameplayTransitionPending{};
        std::atomic<bool> gameplayDeferralLogged{};
        std::atomic<bool> cinematicsEnabled{true};
        std::atomic<std::uint64_t> cinematicsEpoch{};
        std::atomic<bool> activeCinematicsEnabled{};
        std::atomic<std::uint64_t> activeCinematicsEpoch{};
        std::atomic<bool> cinematicFovApplied{};
        std::atomic<float> cinematicTarget{NAN};
        std::atomic<float> exitTarget{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<bool> continuationActive{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<config::CinematicAspectPolicy> aspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> fovMode{
            config::CinematicFovMode::NativeHorPlus};

        plugin::CameraSettingStateView View(Fixture& dialogue)
        {
            return {gameplayMode, gameplayEnabled, gameplayEpoch,
                gameplayEnablePending, gameplayDisablePending,
                gameplayTransitionPending, gameplayDeferralLogged,
                cinematicsEnabled, cinematicsEpoch, activeCinematicsEnabled,
                activeCinematicsEpoch, cinematicFovApplied, cinematicTarget,
                exitTarget, continuationPending, continuationCarryPending,
                continuationActive,
                continuationSource, dialogue.enabled, dialogue.selectedPolicy,
                dialogue.settingsEpoch, dialogue.activeEpoch,
                dialogue.dialogueNonNativeCapabilityAvailable, aspectPolicy,
                fovMode, dialogue.activation, nullptr, &SelectGameplayMode};
        }
    };
}

int main()
{
    bool pass = true;
    // Writer-bound termination is justified by a current production reference,
    // never callback silence, elapsed time, or FOV equality on another source.
    for (unsigned control = 0; control != 21; ++control) {
        Fixture stationary;
        plugin::DialogueBoundaryInput callback{130.311f, 0x77,
            control == 10 ? 70.0f : 130.311f, true, 0.01f, 0.02f, 3,
            false, control == 16 ? 0u : 0xAu, 0, 0xA};
        plugin::ProcessDialogueBoundary(stationary.View(), callback);
        camera::GameplayBaseline reference{};
        reference.nativeFov = {130.308f, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true};
        reference.aspect = 32.0f / 9.0f;
        reference.source = {0xA, true};
        reference.observationSequence = 1;
        reference.valid = reference.current = true;
        if (control == 2) reference.current = false;
        if (control == 3) reference.source.valid = false;
        if (control == 4) reference.observationSequence = 0;
        if (control == 5) ++stationary.settingsEpoch;
        if (control == 7) stationary.exclusion.Arm(130.311f);
        if (control == 9) stationary.phase = dialogue::Phase::Active;
        if (control == 14) stationary.postExitProviderOwned = true;
        if (control == 15) stationary.activePolicy.Capture(config::DialogueZoomPolicy::Reduced);
        if (control == 17) stationary.coordinator = camera::CoordinatorState::CinematicActive;
        const auto previousPhase = stationary.phase;
        auto gate = stationary.correctionGate.Acquire();
        std::lock_guard lock(stationary.mutex);
        const bool retired = plugin::RetireStationaryDialogueCandidateOnGameplayWriter(
            stationary.View(), reference, control == 1 ? 0xB : 0xA,
            control == 11 ? 122.0f : control == 13 ? NAN : 130.311f,
            control == 12 ? 16.0f/9.0f : 32.0f/9.0f, control != 18,
            control == 6 ? 1 : 0, control == 8,
            control == 19 ? -1.0f : control == 20 ? NAN : 0.02f);
        if (retired != (control == 0)) std::cerr << "stationary control=" << control << '\n';
        pass &= Check(retired == (control == 0) &&
            stationary.phase == (retired ? dialogue::Phase::Inactive : previousPhase),
            "stationary Candidate termination requires current identity/provenance and preserves real transitions");
    }
    const cinematics::CinematicRecoveryOwnerSnapshot completedOwner{
        true, 7, 0xA, 0, 0, true};
    const cinematics::CinematicRecoveryOwnerSnapshot activeOwner{
        true, 7, 0xA, 0, 0, false};
    pass &= Check(dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, completedOwner, 7, 0xA) &&
        !completedOwner.HasRecoveryAuthority(),
        "completed recovery metadata identifies same-generation provider without recovery authority");
    pass &= Check(dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::CinematicExiting, true, activeOwner, 7, 0xA),
        "active recovery owner authorizes provider handoff during Cinematic EXIT");
    pass &= Check(!dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, false, completedOwner, 7, 0xA) &&
        !dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, completedOwner, 8, 0xA) &&
        !dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, completedOwner, 7, 0xB) &&
        !dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::CinematicActive, true, completedOwner, 7, 0xA) &&
        !dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::CinematicExiting, true, completedOwner, 7, 0xA) &&
        !dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, activeOwner, 7, 0xA),
        "provider owner authorization fails closed for stale identity or lifecycle state");
    dialogue::PostExitProviderHandoff recoveredProvider;
    const auto recoveredProviderOwner = completedOwner;
    const auto recoveredActivation = dialogue::PostExitProviderActivation{
        7, 3, 0x100, 0xA, 9, 130.311f, 70.0f, 130.311f,
        config::DialogueZoomPolicy::Reduced};
    const bool recoveredArmerAuthorized =
        dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, recoveredProviderOwner,
            7, 0xA);
    const bool recoveredArmed = recoveredProvider.ObserveArmer(
        recoveredArmerAuthorized, recoveredActivation);
    const bool recoveredSetterAuthorized =
        dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, recoveredProviderOwner,
            7, 0xA);
    const bool recoveredSetterMatched = recoveredProvider.ObserveSetterSample(
        recoveredSetterAuthorized, 0x100, 0xA, 9, 7, 3,
        130.311f, 70.0f, 70.0f, 70.0f);
    const bool recoveredWriterAuthorized =
        dialogue::IsPostExitProviderOwnerAuthorized(
            camera::CoordinatorState::Gameplay, true, recoveredProviderOwner,
            7, 0xA);
    const auto recoveredWriterSample = recoveredProvider.TakeWriterSample(
        recoveredWriterAuthorized, 0xA, 9, 7, 3, 70.0f);
    pass &= Check(recoveredArmed && recoveredSetterMatched &&
        recoveredWriterSample.valid && recoveredWriterSample.activation.generation == 7 &&
        recoveredWriterSample.activation.receiver == 0xA,
        "completed-recovery provider handoff matches armer, setter and writer identity");
    dialogue::PostExitProviderHandoff interleavedProvider;
    pass &= Check(interleavedProvider.ObserveArmer(true, recoveredActivation),
        "interleaved provider activation armed");
    dialogue::PostExitProviderReject interleavedSetterReject{};
    const bool unrelatedSetterMatched = interleavedProvider.ObserveSetterSample(
        true, 0x101, 0xA, 9, 7, 3, 130.311f, 130.311f,
        130.311f, 130.311f, 0.001f, &interleavedSetterReject);
    const auto unrelatedSetterReject = interleavedSetterReject;
    const bool providerSetterMatchedAfterUnrelated = interleavedProvider.ObserveSetterSample(
        true, 0x100, 0xA, 9, 7, 3, 130.311f, 70.0f,
        70.0f, 70.0f, 0.001f, &interleavedSetterReject);
    const auto providerWriterAfterUnrelated = interleavedProvider.TakeWriterSample(
        true, 0xA, 9, 7, 3, 70.0f);
    pass &= Check(!unrelatedSetterMatched && interleavedProvider.Active() &&
        unrelatedSetterReject == dialogue::PostExitProviderReject::ComponentMismatch &&
        providerSetterMatchedAfterUnrelated && providerWriterAfterUnrelated.valid,
        "unrelated EXIT setter cannot retire an armed provider handoff");

    Fixture receiverBinding;
    receiverBinding.Event(70.1f, 0x77, 70.0f, true, 0xA);
    const auto bindingActive = receiverBinding.Event(70.0f, 0x77, 70.0f, true, 0xA);
    const auto boundView = receiverBinding.View();
    pass &= Check(bindingActive.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        plugin::ClassifyDialogueWriterContext(boundView, 0xA, 1) ==
            camera::WriterContext::DialogueLifecycleObserved &&
        plugin::ClassifyDialogueWriterContext(boundView, 0xB, 1) ==
            camera::WriterContext::TransitionAmbiguous,
        "Dialogue ownership is receiver-scoped and foreign sources remain ambiguous");
    pass &= Check(receiverBinding.candidate.Receiver(2) == 0,
        "receiver is unavailable after settings epoch changes");
    pass &= Check(!receiverBinding.candidate.HasReceiverBinding(0xA, 1, 1) &&
        plugin::ClassifyDialogueWriterContext(boundView, 0xA, 1, 1) ==
            camera::WriterContext::TransitionAmbiguous &&
        plugin::ClassifyDialogueWriterContext(boundView, 0xB, 1, 1) ==
            camera::WriterContext::TransitionAmbiguous,
        "generation change invalidates ownership and keeps active Dialogue ambiguous");
    receiverBinding.Event(69.9f, 0x77, 70.0f, true, 0xB);
    pass &= Check(receiverBinding.candidate.Receiver(1) == 0,
        "receiver change invalidates binding");
    plugin::ResetDialogueBoundaryState(receiverBinding.View());
    pass &= Check(plugin::ClassifyDialogueWriterContext(receiverBinding.View(),
            0xA, 1, 0, true) == camera::WriterContext::TransitionAmbiguous &&
        plugin::ClassifyDialogueWriterContext(receiverBinding.View(), 0xA, 1) ==
            camera::WriterContext::GameplayEligible,
        "completed provider endpoint defers baseline without sticky ambiguity");
    pass &= Check(receiverBinding.candidate.Receiver(1) == 0,
        "dialogue reset retires receiver binding");
    receiverBinding.Event(70.1f, 0x77, 70.0f, true, 0xA);
    receiverBinding.Event(70.0f, 0x77, 70.0f, true, 0xA);
    const auto receiverRearm = receiverBinding.Event(70.1f, 0x77, 70.0f, true, 0xA);
    pass &= Check(receiverRearm.phaseAfter == dialogue::Phase::RearmPending &&
        plugin::ClassifyDialogueWriterContext(receiverBinding.View(), 0xA, 1) ==
            camera::WriterContext::TransitionAmbiguous,
        "rearm remains source-scoped and ambiguous");
    plugin::ResetDialogueBoundaryState(receiverBinding.View());
    receiverBinding.Event(70.1f, 0x77, 70.0f, true, 0xB);
    pass &= Check(receiverBinding.candidate.Receiver(1) == 0xB &&
        !receiverBinding.candidate.AffectsReceiver(0xA, 1),
        "new candidate after reset does not inherit prior receiver");
    plugin::ResetDialogueBoundaryState(receiverBinding.View());
    const auto unverified = plugin::ProcessDialogueBoundary(receiverBinding.View(),
        {70.1f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, 0, 0, 0xA});
    pass &= Check(unverified.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        plugin::ClassifyDialogueWriterContext(receiverBinding.View(), 0xA, 1) ==
            camera::WriterContext::TransitionAmbiguous &&
        plugin::ClassifyDialogueWriterContext(receiverBinding.View(), 0xB, 1) ==
            camera::WriterContext::TransitionAmbiguous,
        "unverified Dialogue receiver keeps writer context ambiguous");
    plugin::ResetDialogueBoundaryState(receiverBinding.View());
    static_cast<void>(plugin::ProcessDialogueBoundary(receiverBinding.View(),
        {70.1f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, 0, 0, 0}));
    const auto laterValidated = plugin::ProcessDialogueBoundary(receiverBinding.View(),
        {70.0f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, 0xA});
    pass &= Check(laterValidated.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        plugin::ClassifyDialogueWriterContext(receiverBinding.View(), 0xA, 1) ==
            camera::WriterContext::DialogueLifecycleObserved,
        "missing receiver does not prevent later validated binding");
    // A completed provider transition is admitted once as Dialogue using the
    // last trusted Gameplay baseline; its native ~70-degree endpoint is never
    // recaptured as the Dialogue baseline or admitted to Gameplay.
    dialogue::PostExitProviderHandoff provider;
    const dialogue::PostExitProviderActivation handled{1, 1, 0x1000, 0x2000,
        7, 126.16f, 70.0f, 126.16f, config::DialogueZoomPolicy::Reduced};
    pass &= Check(provider.ObserveArmer(true, handled), "provider armed");
    provider.CompleteRecovery();
    {
        dialogue::PostExitProviderHandoff correctedProvider;
        correctedProvider.ObserveArmer(true, handled);
        correctedProvider.CompleteRecovery();
        correctedProvider.ConsumeDialogueAdmission(0x1000, 0x2000, 7, 1, 1,
            126.16f, 70.0f);
        correctedProvider.RecordCorrectedDialogueSetterSample(122.092f);
        const bool first = correctedProvider.MatchesCorrectedDialogueSetterSample(
            0x2000, 7, 1, 1, 122.092f);
        const bool repeated = correctedProvider.MatchesCorrectedDialogueSetterSample(
            0x2000, 7, 1, 1, 122.092f);
        pass &= Check(first && repeated,
            "repeated_writer_reads_retain_confirmed_setter_corrected_provenance");
        pass &= Check(!correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2001, 7, 1, 1, 122.092f) &&
            !correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2000, 8, 1, 1, 122.092f) &&
            !correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2000, 7, 2, 1, 122.092f) &&
            !correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2000, 7, 1, 2, 122.092f),
            "corrected_setter_record_does_not_authorize_foreign_writer_identity");
        pass &= Check(!correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2000, 7, 1, 1, 70.0f) &&
            !correctedProvider.MatchesCorrectedDialogueSetterSample(
                0x2000, 7, 1, 1, 122.092f),
            "raw_field_update_retires_corrected_record_without_numeric_resurrection");
    }
    Fixture residual;
    residual.exclusion.Arm(126.16f);
    residual.selectedPolicy.store(config::DialogueZoomPolicy::Reduced);
    bool firstProviderAdmission = true;
    for (const float sample : {70.1051f, 70.0283f, 70.0005f, 70.0f}) {
        const bool admitted = provider.ConsumeDialogueAdmission(0x1000, 0x2000,
            7, 1, 1, 126.16f, 70.0f);
        plugin::DialogueBoundaryInput providerInput{sample, 0x1028, 70.0f, true,
            0.01f, 0.02f, 3, admitted, 0x2000, 1, 0x2000, 90.3115f,
            firstProviderAdmission, config::DialogueZoomPolicy::Reduced, admitted};
        providerInput.threadId = 7;
        const auto result = plugin::ProcessDialogueBoundary(residual.View(),
            providerInput);
        firstProviderAdmission = false;
        const auto reducedTarget = dialogue::ReducedTarget(90.3115f);
        const auto expectedOutput = dialogue::TransformProjectionSample(
            sample, 90.3115f, reducedTarget);
        pass &= Check(!provider.Active() && result.mutateXmm1 &&
            std::fabs(result.outputFov - expectedOutput) < 0.01f &&
            residual.baseline == 90.3115f && residual.phase == dialogue::Phase::Active &&
            (admitted == (result.action ==
                plugin::DialogueBoundaryAction::ProviderDialogueStarted)),
            "confirmed provider endpoint admits Dialogue policy during Gameplay exclusion");
        pass &= Check(provider.BlocksGameplayBaselineAdmission(0x2000, 7, 1, 1,
                sample, 0.02f) &&
            plugin::ClassifyDialogueWriterContext(residual.View(), 0x2000, 1, 1) ==
                camera::WriterContext::DialogueLifecycleObserved,
            "provider-derived Dialogue remains excluded from Gameplay baseline admission");
    }

    // Once accepted, a policy-settings epoch change must not erase provider
    // ownership or replace its captured policy. It remains source/thread/
    // generation scoped, and ordinary Dialogue still uses the strict binding.
    dialogue::PostExitProviderHandoff retainedProvider;
    const dialogue::PostExitProviderActivation retainedActivation{9, 1,
        0x1100, 0x2200, 17, 126.16f, 70.0f, 126.16f,
        config::DialogueZoomPolicy::Reduced};
    pass &= Check(retainedProvider.ObserveArmer(true, retainedActivation),
        "retained provider arming accepted");
    retainedProvider.CompleteRecovery();
    Fixture retainedLifecycle;
    retainedLifecycle.exclusion.Arm(126.16f);
    plugin::DialogueBoundaryInput retainedStart{70.1f, 0x1130, 70.0f, true,
        0.01f, 0.02f, 3, true, 0x2200, 9, 0x2200, 90.3115f, true,
        config::DialogueZoomPolicy::Reduced, true};
    retainedStart.threadId = 17;
    const auto retainedStartResult = plugin::ProcessDialogueBoundary(
        retainedLifecycle.View(), retainedStart);
    retainedLifecycle.settingsEpoch.store(5, std::memory_order_release);
    retainedLifecycle.selectedPolicy.store(config::DialogueZoomPolicy::Adaptive,
        std::memory_order_release);
    plugin::DialogueBoundaryInput policyChanged{70.09f, 0x1130, 70.0f, true,
        0.01f, 0.02f, 3, false, 0x2200, 9, 0x2200};
    policyChanged.threadId = 17;
    const auto policyChangedResult = plugin::ProcessDialogueBoundary(
        retainedLifecycle.View(), policyChanged);
    const bool retainedIdentity = plugin::IsConfirmedProviderDialogueLifecycle(
        retainedLifecycle.View(), retainedActivation, 0x2200, 17, 9);
    pass &= Check(retainedStartResult.action ==
            plugin::DialogueBoundaryAction::ProviderDialogueStarted &&
        policyChangedResult.phaseAfter == dialogue::Phase::Active &&
        retainedLifecycle.activePolicy.IsValid() &&
        retainedLifecycle.activePolicy.Value() == config::DialogueZoomPolicy::Reduced &&
        retainedIdentity && plugin::ClassifyDialogueWriterContext(
            retainedLifecycle.View(), 0x2200, 5, 9, false, retainedIdentity) ==
                camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved,
        "confirmed provider identity and captured policy survive Dialogue settings epoch change");
    pass &= Check(!plugin::IsConfirmedProviderDialogueLifecycle(
            retainedLifecycle.View(), retainedActivation, 0x2201, 17, 9) &&
        !plugin::IsConfirmedProviderDialogueLifecycle(
            retainedLifecycle.View(), retainedActivation, 0x2200, 18, 9) &&
        !plugin::IsConfirmedProviderDialogueLifecycle(
            retainedLifecycle.View(), retainedActivation, 0x2200, 17, 10),
        "confirmed provider ownership rejects receiver, thread, and generation mismatches");
    auto wrongActivationEpoch = retainedActivation;
    wrongActivationEpoch.dialogueSettingsEpoch++;
    pass &= Check(!plugin::IsConfirmedProviderDialogueLifecycle(
            retainedLifecycle.View(), wrongActivationEpoch, 0x2200, 17, 9),
        "provider activation epoch must match the accepted lifecycle epoch");
    plugin::DialogueBoundaryInput providerExitInput{90.3115f, 0x1028, 90.3115f,
        true, 0.01f, 0.02f, 3, false, 0x2000, 1, 0x2000};
    providerExitInput.threadId = 7;
    const auto providerExit = plugin::ProcessDialogueBoundary(residual.View(),
        providerExitInput);
    const auto providerRecovery = plugin::ProcessDialogueBoundary(residual.View(),
        providerExitInput);
    pass &= Check(providerExit.action == plugin::DialogueBoundaryAction::RecoveryStarted &&
        providerRecovery.action == plugin::DialogueBoundaryAction::RecoveryCompleted &&
        residual.phase == dialogue::Phase::Inactive &&
        plugin::ClassifyDialogueWriterContext(residual.View(), 0x2000, 1, 1) ==
            camera::WriterContext::GameplayEligible,
        "provider-derived Dialogue naturally exits and Gameplay baseline admission rearms");
    for (const auto policy : {config::DialogueZoomPolicy::Native,
            config::DialogueZoomPolicy::Adaptive, config::DialogueZoomPolicy::Reduced,
            config::DialogueZoomPolicy::Disabled}) {
        Fixture policyFixture;
        policyFixture.exclusion.Arm(126.16f);
        const auto changedSelection = policy == config::DialogueZoomPolicy::Reduced
            ? config::DialogueZoomPolicy::Adaptive
            : config::DialogueZoomPolicy::Reduced;
        policyFixture.selectedPolicy.store(changedSelection);
        dialogue::PostExitProviderHandoff policyProvider;
        auto policyActivation = handled;
        policyActivation.dialoguePolicy = policy;
        policyProvider.ObserveArmer(true, policyActivation);
        policyProvider.CompleteRecovery();
        const bool admission = policyProvider.ConsumeDialogueAdmission(0x1000,
            0x2000, 7, 1, 1, 126.16f, 70.0f);
        plugin::DialogueBoundaryInput policyInput{70.0f, 0x1028, 70.0f, true,
            0.01f, 0.02f, 3, admission, 0x2000, 1, 0x2000, 90.3115f,
            true, policy, admission};
        policyInput.threadId = 7;
        const auto result = plugin::ProcessDialogueBoundary(policyFixture.View(),
            policyInput);
        const float target = dialogue::ResolveDialogueDestinationTarget(policy,
            90.3115f, 70.0f);
        const bool expectedMutation = policy != config::DialogueZoomPolicy::Native;
        pass &= Check(result.action == plugin::DialogueBoundaryAction::ProviderDialogueStarted &&
            result.activePolicy == policy &&
            result.mutateXmm1 == expectedMutation &&
            std::fabs(result.outputFov - (expectedMutation ? target : 70.0f)) < 0.01f,
            "provider endpoint admission honors each effective Dialogue policy");
    }
    Fixture foreignProviderContinuation;
    foreignProviderContinuation.exclusion.Arm(126.16f);
    plugin::DialogueBoundaryInput foreignProviderStart{70.0f, 0x1028, 70.0f,
        true, 0.01f, 0.02f, 3, true, 0x2000, 1, 0x2000, 90.3115f,
        true, config::DialogueZoomPolicy::Reduced, true};
    foreignProviderStart.threadId = 7;
    const auto providerActivation = plugin::ProcessDialogueBoundary(
        foreignProviderContinuation.View(), foreignProviderStart);
    plugin::DialogueBoundaryInput unrelatedProviderInput{130.0f, 0x2028,
        70.0f, true, 0.01f, 0.02f, 3, false, 0x2000, 1, 0x2000};
    unrelatedProviderInput.threadId = 7;
    const auto unrelatedProviderCallback = plugin::ProcessDialogueBoundary(
        foreignProviderContinuation.View(), unrelatedProviderInput);
    const bool providerOwnershipRetained =
        foreignProviderContinuation.postExitProviderOwned &&
        foreignProviderContinuation.phase == dialogue::Phase::Active;
    plugin::DialogueBoundaryInput providerContinuationInput{69.9f, 0x1028,
        70.0f, true, 0.01f, 0.02f, 3, false, 0x2000, 1, 0x2000};
    providerContinuationInput.threadId = 7;
    const auto providerContinuation = plugin::ProcessDialogueBoundary(
        foreignProviderContinuation.View(), providerContinuationInput);
    plugin::DialogueBoundaryInput foreignProviderInput{69.9f, 0x1028,
        70.0f, true, 0.01f, 0.02f, 3, false, 0x3000, 1, 0x3000};
    foreignProviderInput.threadId = 7;
    const auto foreignProviderEvent = plugin::ProcessDialogueBoundary(
        foreignProviderContinuation.View(), foreignProviderInput);
    pass &= Check(providerActivation.action ==
            plugin::DialogueBoundaryAction::ProviderDialogueStarted &&
        unrelatedProviderCallback.action ==
            plugin::DialogueBoundaryAction::ProviderCallbackIgnored &&
        providerOwnershipRetained && providerContinuation.mutateXmm1 &&
        foreignProviderEvent.action == plugin::DialogueBoundaryAction::ExclusionReset &&
        foreignProviderContinuation.phase == dialogue::Phase::Inactive &&
        !foreignProviderContinuation.postExitProviderOwned,
        "provider Dialogue keeps exact callback ownership and resets on receiver replacement");
    pass &= Check(!provider.BlocksRepeatedAdmission(0x3000, 0x2000, 7, 1, 1,
        126.16f, 70.0f), "independent component is not blocked");
    pass &= Check(provider.BlocksGameplayBaselineAdmission(0x2000, 7, 1, 1,
            70.0856f, 0.02f) &&
        !provider.BlocksGameplayBaselineAdmission(0x2001, 7, 1, 1,
            70.0856f, 0.02f) &&
        !provider.BlocksGameplayBaselineAdmission(0x2000, 8, 1, 1,
            70.0856f, 0.02f) &&
        !provider.BlocksGameplayBaselineAdmission(0x2000, 7, 2, 1,
            70.0856f, 0.02f) &&
        !provider.BlocksGameplayBaselineAdmission(0x2000, 7, 1, 2,
            70.0856f, 0.02f) &&
        !provider.BlocksGameplayBaselineAdmission(0x2000, 7, 1, 1,
            70.3f, 0.02f),
        "post-exit endpoint quarantine is limited to activation identity and endpoint neighborhood");
    pass &= Check(!provider.BlocksRepeatedAdmission(0x1000, 0x3000, 7, 1, 1,
        126.16f, 70.0f), "foreign receiver is not blocked");
    pass &= Check(!provider.BlocksRepeatedAdmission(0x1000, 0x2000, 8, 1, 1,
        126.16f, 70.0f), "foreign thread is not blocked");
    pass &= Check(!provider.BlocksRepeatedAdmission(0x1000, 0x2000, 7, 1, 2,
        126.16f, 70.0f) && !provider.RecoveryCompleted(),
        "settings epoch invalidates completed activation");
    provider.ObserveArmer(true, handled);
    provider.CompleteRecovery();
    provider.ObserveArmer(false, handled);
    pass &= Check(!provider.RecoveryCompleted(),
        "new native initialization clears suppression even without recovery admission");
    provider.ObserveArmer(true, handled);
    provider.CompleteRecovery();
    pass &= Check(!provider.BlocksRepeatedAdmission(0x1000, 0x2000, 7, 2, 1,
        126.16f, 70.0f) && !provider.RecoveryCompleted(),
        "new cinematic generation retires suppression");
    provider.ObserveArmer(true, handled);
    provider.CompleteRecovery();
    provider.Reset();
    pass &= Check(!provider.RecoveryCompleted(), "lifecycle reset releases admission");
    Fixture state;
    const auto candidate = state.Event(100.0f, 0xA);
    const auto active = state.Event(99.8f, 0xA);
    const bool activationArmedAtStart = state.activation.CanCorrect(true);
    const auto activeB = state.Event(89.7f, 0xB);
    const auto exitingB = state.Event(100.2f, 0xB);
    const auto rearmedB = state.Event(70.0f, 0xB);
    if (active.action != plugin::DialogueBoundaryAction::AcceptedStart ||
        active.phaseAfter != dialogue::Phase::Active || !active.mutateXmm1)
        std::cerr << "dialogue_active action=" << static_cast<int>(active.action)
            << " phase=" << static_cast<int>(active.phaseAfter)
            << " mutate=" << active.mutateXmm1 << " output=" << active.outputFov
            << " policy=" << static_cast<int>(active.activePolicy)
        << " armed=" << activationArmedAtStart << '\n';
    pass &= Check(candidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        candidate.phaseAfter == dialogue::Phase::Candidate && !candidate.mutateXmm1,
        "candidate event captures lifecycle without changing XMM1");
    pass &= Check(active.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        active.phaseAfter == dialogue::Phase::Active && active.mutateXmm1 &&
        activationArmedAtStart,
        "confirmed descent accepts Dialogue and returns production correction output");
    pass &= Check(activeB.phaseAfter == dialogue::Phase::Active && activeB.mutateXmm1 &&
        std::isfinite(activeB.outputFov) && activeB.outputFov != 89.7f,
        "active callback may move from accepted A to observed B without fixed-source gate");
    pass &= Check(exitingB.action == plugin::DialogueBoundaryAction::RecoveryStarted &&
        exitingB.phaseAtCallback == dialogue::Phase::Exiting &&
        exitingB.phaseAfter == dialogue::Phase::RearmPending &&
        exitingB.recoverySource == 0xB && !exitingB.mutateXmm1,
        "native A-to-B transition preserves Exiting then rearms on observed B");
    pass &= Check(rearmedB.action == plugin::DialogueBoundaryAction::RecoveryCompleted &&
        rearmedB.phaseAfter == dialogue::Phase::Inactive &&
        !rearmedB.mutateXmm1,
        "RecoveryRearm completion clears lifecycle without changing native XMM1");

    Fixture offStarted;
    offStarted.enabled.store(false);
    const auto offCandidate = offStarted.Event(90.0f, 0xA);
    const auto offActive = offStarted.Event(89.8f, 0xA);
    offStarted.enabled.store(true);
    const auto onMidLifecycle = offStarted.Event(89.0f, 0xA);
    pass &= Check(offCandidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        offActive.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        !offActive.mutateXmm1 && onMidLifecycle.phaseAfter == dialogue::Phase::Active &&
        !onMidLifecycle.mutateXmm1,
        "OFF observation remains active and ON does not retroactively arm lifecycle");

    Fixture candidateMismatch;
    static_cast<void>(candidateMismatch.Event(90.0f, 0xA));
    const auto rejectedCandidate = candidateMismatch.Event(89.8f, 0xB);
    if (rejectedCandidate.action != plugin::DialogueBoundaryAction::CandidateCancelled)
        std::cerr << "dialogue_mismatch action=" << static_cast<int>(rejectedCandidate.action)
            << " phase=" << static_cast<int>(rejectedCandidate.phaseAfter) << '\n';
    pass &= Check(rejectedCandidate.action == plugin::DialogueBoundaryAction::CandidateCancelled &&
        candidateMismatch.phase == dialogue::Phase::Inactive &&
        !rejectedCandidate.mutateXmm1,
        "candidate context mismatch cancels only pending Candidate state");

    Fixture invalidTarget;
    const auto rejectedTarget = invalidTarget.Event(90.0f, 0xA, NAN, false);
    pass &= Check(rejectedTarget.action == plugin::DialogueBoundaryAction::CandidateRejected &&
        invalidTarget.phase == dialogue::Phase::Inactive &&
        !invalidTarget.snapshotTargetValid.load(),
        "unreadable native target fails closed without candidate ownership");

    Fixture exitingOwner;
    exitingOwner.coordinator.store(camera::CoordinatorState::CinematicExiting);
    const auto rejectedDuringExit = exitingOwner.Event(70.0f, 0xD);
    pass &= Check(rejectedDuringExit.action ==
            plugin::DialogueBoundaryAction::NonGameplayReset &&
        rejectedDuringExit.earlyReturn && !rejectedDuringExit.mutateXmm1 &&
        exitingOwner.phase == dialogue::Phase::Inactive,
        "dialogue_callback_during_cinematic_exiting_reports_non_gameplay_guard");

    Fixture excludedRecovery;
    excludedRecovery.exclusion.Arm(90.0f);
    const auto rejectedByExclusion = excludedRecovery.Event(70.0f, 0xD);
    pass &= Check(rejectedByExclusion.action ==
            plugin::DialogueBoundaryAction::ExclusionReset &&
        rejectedByExclusion.earlyReturn && !rejectedByExclusion.mutateXmm1 &&
        excludedRecovery.phase == dialogue::Phase::Inactive,
        "dialogue_callback_during_post_cinematic_exclusion_reports_exact_guard");

    // Dialogue OFF-first: use the production setting transaction, then the
    // same event decision path. Lifecycle observation is retained; XMM1 is not.
    Fixture offFirst;
    SettingsFixture settings;
    auto off = plugin::ApplyCameraSetting(offFirst.correctionGate, settings.View(offFirst),
        plugin::RuntimeSettingMutation::DialogueEnabled(false), true, [] {});
    const auto offFirstCandidate = offFirst.Event(90.0f, 0xA);
    const auto offFirstActive = offFirst.Event(89.8f, 0xA);
    pass &= Check(off.mutation.accepted && !offFirst.enabled.load() &&
        offFirstCandidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        offFirstActive.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        !offFirstActive.mutateXmm1 && offFirst.phase == dialogue::Phase::Active,
        "Dialogue OFF-first is observation-only while real lifecycle phase advances");

    // Deterministic writer-first ordering: pause after the actual correction
    // transaction owns the production gate; OFF cannot return until XMM1 and
    // lifecycle output have committed.
    Fixture writerFirst;
    static_cast<void>(writerFirst.Event(100.0f, 0xA));
    static_cast<void>(writerFirst.Event(99.8f, 0xA));
    SettingsFixture writerFirstSettings;
    std::mutex rendezvousMutex;
    std::condition_variable rendezvous;
    bool writerPaused = false;
    bool releaseWriter = false;
    bool offAttempted = false;
    std::atomic<bool> offReturned{};
    plugin::DialogueBoundaryResult inFlight{};
    float committedXmm1 = 89.0f;
    std::thread writer([&] {
        auto gate = writerFirst.correctionGate.Acquire();
        std::lock_guard dialogueLock(writerFirst.mutex);
        {
            std::unique_lock lock(rendezvousMutex);
            writerPaused = true;
            rendezvous.notify_all();
            rendezvous.wait(lock, [&] { return releaseWriter; });
        }
        inFlight = plugin::ProcessDialogueBoundary(writerFirst.View(),
            {89.0f, 0xA, 70.0f, true, 0.01f, 0.02f, 3});
        if (inFlight.mutateXmm1) committedXmm1 = inFlight.outputFov;
    });
    {
        std::unique_lock lock(rendezvousMutex);
        rendezvous.wait(lock, [&] { return writerPaused; });
    }
    std::thread disable([&] {
        {
            std::lock_guard lock(rendezvousMutex);
            offAttempted = true;
            rendezvous.notify_all();
        }
        const auto result = plugin::ApplyCameraSetting(writerFirst.correctionGate,
            writerFirstSettings.View(writerFirst),
            plugin::RuntimeSettingMutation::DialogueEnabled(false), true, [] {});
        offReturned.store(result.mutation.accepted, std::memory_order_release);
    });
    {
        std::unique_lock lock(rendezvousMutex);
        rendezvous.wait(lock, [&] { return offAttempted; });
        pass &= Check(!offReturned.load(std::memory_order_acquire),
            "OFF cannot return while Dialogue production transaction holds correction gate");
        releaseWriter = true;
        rendezvous.notify_all();
    }
    writer.join();
    disable.join();
    if (!inFlight.mutateXmm1 || committedXmm1 == 89.0f ||
        !offReturned.load() || writerFirst.activation.CanCorrect(true))
        std::cerr << "dialogue_writer_first mutate=" << inFlight.mutateXmm1
            << " committed=" << committedXmm1 << " off=" << offReturned.load()
            << " armed=" << writerFirst.activation.CanCorrect(true)
            << " phase=" << static_cast<int>(writerFirst.phase) << '\n';
    pass &= Check(inFlight.mutateXmm1 && committedXmm1 != 89.0f &&
        offReturned.load() && !writerFirst.activation.CanCorrect(true),
        "writer-first correction commits before OFF, and OFF disarms before returning");

    // Cinematics setting publication shares the same correction gate and
    // retains the already-established deferred-activation semantics.
    Fixture cinematicSettingState;
    SettingsFixture cinematicSettings;
    const auto cinematicOff = plugin::ApplyCameraSetting(
        cinematicSettingState.correctionGate, cinematicSettings.View(cinematicSettingState),
        plugin::RuntimeSettingMutation::CinematicsEnabled(false), true, [] {});
    const auto cinematicOn = plugin::ApplyCameraSetting(
        cinematicSettingState.correctionGate, cinematicSettings.View(cinematicSettingState),
        plugin::RuntimeSettingMutation::CinematicsEnabled(true), true, [] {});
    pass &= Check(cinematicOff.mutation.changed && cinematicOn.mutation.changed &&
        !cinematicSettings.activeCinematicsEnabled.load() &&
        !cinematicSettings.cinematicFovApplied.load(),
        "Cinematics OFF/ON does not retroactively activate the old generation");

    std::cout << (pass ? "PASS" : "FAIL") << ": Dialogue boundary transaction\n";
    return pass ? 0 : 1;
}
