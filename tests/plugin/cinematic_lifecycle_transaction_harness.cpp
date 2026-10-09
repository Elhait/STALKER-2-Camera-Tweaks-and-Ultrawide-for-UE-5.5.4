#include "../../src/plugin/cinematic_lifecycle_transaction.hpp"
#include "../../src/plugin/dialogue_boundary_transaction.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"
#include "../../src/diagnostics/diagnostic_runtime.hpp"

#include <array>
#include <cmath>
#include <iostream>

namespace
{
    void SelectModeForTest(void* context, config::GameplayMode mode)
    {
        static_cast<std::atomic<config::GameplayMode>*>(context)->store(
            mode, std::memory_order_release);
    }

    struct FreshFieldsFixture
    {
        plugin::CameraWriterFreshAspectFlags fields{};
        std::uintptr_t expectedSource{};
        std::uintptr_t lastReadSource{};
        std::uint32_t calls{};
        bool lastReadIncludedFlags{};
        bool sourceMatched{};
    };

    struct GameplayWriterFixture
    {
        std::atomic<bool> enableApplyPending{};
        std::atomic<bool> disableRestorePending{};
        std::atomic<bool> modeTransitionPending{};
        std::atomic<bool> enableTransitionDeferralLogged{};
        std::atomic<bool> modeTransitionDeferralLogged{};
        std::atomic<config::GameplayMode> modeTransitionTarget{
            config::GameplayMode::HorPlus};
        std::atomic<gameplay::ReplayState> replayState{
            gameplay::ReplayState::WaitingForAutomaticUpdate};
        std::atomic<std::uintptr_t> lastAutoRestoreSource{};
        std::atomic<std::uintptr_t> lastSource{};
        std::atomic<float> lastFov{NAN};
        camera::GameplayAspectRestorationStore aspectRestoration;
        camera::CameraFovObservationStore fovObservation;
        camera::GameplayBaselineStore baseline;

        plugin::CameraWriterGameplayStateView View()
        {
            return {enableApplyPending, disableRestorePending,
                modeTransitionPending, enableTransitionDeferralLogged,
                modeTransitionDeferralLogged, modeTransitionTarget,
                replayState, lastAutoRestoreSource, lastSource, lastFov,
                aspectRestoration, fovObservation, baseline};
        }
    };

    struct DialogueResetFixture
    {
        std::uint32_t calls{};
    };

    void ResetDialogueForTest(void* context)
    {
        ++static_cast<DialogueResetFixture*>(context)->calls;
    }

    plugin::CameraWriterFreshAspectFlags ReadFreshFields(void* context,
        std::uintptr_t source, bool includeFlags)
    {
        auto& fixture = *static_cast<FreshFieldsFixture*>(context);
        ++fixture.calls;
        fixture.lastReadSource = source;
        fixture.lastReadIncludedFlags = includeFlags;
        fixture.sourceMatched = source == fixture.expectedSource;
        if (!fixture.sourceMatched) return {};
        auto fields = fixture.fields;
        if (!includeFlags) fields.flagsReadable = false;
        return fields;
    }

    float AutoAspectForTest() { return 3.55556f; }
    bool WritableForTest(std::uintptr_t, std::size_t) { return true; }
    bool ReadOnlyForTest(std::uintptr_t, std::size_t) { return false; }
    bool ValidAspectForTest(float value)
    {
        return std::isfinite(value) && value > 1.0f;
    }

    plugin::CinematicAspectStoreInput AspectInput(std::uintptr_t target)
    {
        return {target, true, true, false, true, true,
            16.0f / 9.0f, cinematics::Forced21x9Aspect, 32.0f / 9.0f,
            &AutoAspectForTest, &AutoAspectForTest};
    }

    struct Fixture
    {
        std::atomic<config::GameplayMode> settingGameplayMode{
            config::GameplayMode::HorPlus};
        std::atomic<bool> settingGameplayEnabled{true};
        std::atomic<std::uint64_t> settingGameplayEpoch{};
        std::atomic<bool> settingGameplayEnablePending{};
        std::atomic<bool> settingGameplayDisablePending{};
        std::atomic<bool> settingGameplayTransitionPending{};
        std::atomic<bool> settingGameplayDeferralLogged{};
        std::atomic<bool> settingCinematicsEnabled{true};
        std::atomic<std::uint64_t> activeCinematicSettingsEpoch{};
        std::atomic<camera::CoordinatorState> coordinator{camera::CoordinatorState::Gameplay};
        std::atomic<bool> selectionValid{};
        std::atomic<std::uint64_t> generation{};
        std::atomic<bool> activeEnabled{};
        std::atomic<std::uint64_t> settingsEpoch{};
        std::atomic<config::CinematicAspectPolicy> aspectPolicy{};
        std::atomic<config::CinematicFovMode> fovMode{};
        std::atomic<bool> fovApplied{};
        std::atomic<float> transformed{NAN};
        std::atomic<float> exitTarget{NAN};
        std::atomic<float> exitTransformed{NAN};
        std::atomic<float> enterNative{NAN};
        std::atomic<float> preEnterNative{NAN};
        std::atomic<float> preEnterHorPlus{NAN};
        std::atomic<float> preEnterAspect{NAN};
        std::atomic<bool> preEnterValid{};
        std::atomic<std::uintptr_t> gameplayContinuationSource{};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationExitTarget{NAN};
        std::atomic<float> continuationPreviousTarget{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner recoveryOwner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffPreviousFov{NAN};
        std::mutex dialogueMutex;
        dialogue::Phase dialoguePhase{dialogue::Phase::Inactive};
        dialogue::PolicySnapshot activeDialoguePolicy;
        dialogue::CorrectionActivation dialogueActivation;
        float dialogueBaseline{NAN};
        float dialoguePrevious{NAN};
        float dialogueExitStart{NAN};
        dialogue::CandidateTracker dialogueCandidate;
        dialogue::RecoveryRearm dialogueRecovery;
        std::atomic<bool> dialogueEnabled{true};
        std::atomic<config::DialogueZoomPolicy> dialoguePolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<std::uint64_t> dialogueSettingsEpoch{1};
        std::atomic<std::uint64_t> activeDialogueSettingsEpoch{};
        std::atomic<std::uintptr_t> snapshotDialogueSource{};
        std::atomic<float> snapshotDialogueTarget{NAN};
        std::atomic<bool> snapshotDialogueTargetValid{};
        GameplayWriterFixture gameplayWriter;
        FreshFieldsFixture defaultFreshFields{};
        const plugin::CameraWriterEngineOperations defaultFreshReader{
            &defaultFreshFields, &ReadFreshFields};

        plugin::CinematicLifecycleStateView View()
        {
            return {coordinator, selectionValid, generation, activeEnabled, settingsEpoch,
                aspectPolicy, fovMode, fovApplied, transformed, exitTarget,
                exitTransformed, enterNative, preEnterNative, preEnterHorPlus,
                preEnterAspect, preEnterValid, gameplayContinuationSource,
                continuationPending, continuationCarryPending, continuationSource,
                continuationExitTarget,
                continuationPreviousTarget, continuationInterpolation, continuationStart,
                association, recoveryOwner, exclusion, handoffPending, handoffSource,
                handoffPreviousFov};
        }

        plugin::CameraWriterCinematicStateView WriterView()
        {
            return {activeEnabled, selectionValid, generation, fovApplied,
                enterNative, transformed, continuationSource, continuationStart,
                continuationInterpolation, association};
        }

        plugin::CameraWriterGameplayStateView GameplayView()
        {
            return gameplayWriter.View();
        }

        plugin::DialogueBoundaryStateView DialogueView()
        {
            return {dialogueMutex, coordinator, exclusion, dialoguePhase,
                activeDialoguePolicy, dialogueActivation, dialogueBaseline,
                dialoguePrevious, dialogueExitStart, dialogueCandidate,
                dialogueRecovery, dialogueEnabled, dialoguePolicy,
                dialogueSettingsEpoch, activeDialogueSettingsEpoch,
                snapshotDialogueSource, snapshotDialogueTarget,
                snapshotDialogueTargetValid};
        }

        plugin::CameraSettingStateView SettingsView()
        {
            return {settingGameplayMode, settingGameplayEnabled,
                settingGameplayEpoch, settingGameplayEnablePending,
                settingGameplayDisablePending, settingGameplayTransitionPending,
                settingGameplayDeferralLogged, settingCinematicsEnabled,
                settingsEpoch, activeEnabled, activeCinematicSettingsEpoch,
                fovApplied, transformed, exitTarget, continuationPending,
                continuationCarryPending,
                continuationInterpolation, continuationSource, dialogueEnabled,
                dialoguePolicy, dialogueSettingsEpoch, activeDialogueSettingsEpoch,
                true,
                aspectPolicy, fovMode, dialogueActivation, &settingGameplayMode,
                &SelectModeForTest};
        }

        plugin::DialogueBoundaryResult DialogueEvent(
            plugin::CameraCorrectionGate& gate, float fov,
            std::uintptr_t source, float target = 70.0f)
        {
            auto correctionLock = gate.Acquire();
            std::lock_guard dialogueLock(dialogueMutex);
            return plugin::ProcessDialogueBoundary(DialogueView(),
                {fov, source, target, source != 0 && std::isfinite(target),
                    0.01f, 0.02f, 3});
        }

        plugin::CameraWriterResult Writer(plugin::CameraCorrectionGate& gate,
            const plugin::CameraWriterInput& input,
            plugin::CameraWriterEngineOperations freshReader = {},
            plugin::CameraWriterDialogueResetOperation dialogueReset = {},
            plugin::CameraWriterGameplayOperations gameplayOperations = {})
        {
            auto correctionLock = gate.Acquire();
            return WriterTransaction(gate, input, freshReader, dialogueReset,
                gameplayOperations);
        }

        plugin::CameraWriterResult WriterTransaction(
            plugin::CameraCorrectionGate& gate,
            const plugin::CameraWriterInput& input,
            plugin::CameraWriterEngineOperations freshReader = {},
            plugin::CameraWriterDialogueResetOperation dialogueReset = {},
            plugin::CameraWriterGameplayOperations gameplayOperations = {})
        {
            plugin::CameraWriterRecoveryStateView recoveryView{
                coordinator, association, recoveryOwner, exclusion,
                handoffPending, handoffSource, handoffPreviousFov,
                continuationPending, continuationCarryPending, continuationSource,
                continuationExitTarget,
                continuationPreviousTarget, continuationInterpolation,
                continuationStart, gameplayContinuationSource, exitTarget,
                exitTransformed};
            auto gameplayView = GameplayView();
            auto reader = freshReader;
            if (!reader.read) {
                defaultFreshFields.expectedSource = input.sourceIdentity;
                defaultFreshFields.fields.aspect = input.aspect;
                defaultFreshFields.fields.flags = input.flags;
                defaultFreshFields.fields.aspectReadable = input.aspectValid;
                defaultFreshFields.fields.flagsReadable = input.flagsValid;
                reader = defaultFreshReader;
            }
            return plugin::ProcessCameraWriter(gate, recoveryView, input, WriterView(),
                reader, dialogueReset, &gameplayView, gameplayOperations);
        }

        static bool ResetDialogueOnGameplayContextChange(void* context,
            std::uintptr_t, float, std::uintptr_t, float)
        {
            auto& state = *static_cast<Fixture*>(context);
            std::lock_guard lock(state.dialogueMutex);
            if (!plugin::DialogueBoundaryIsActive(state.DialogueView())) return false;
            plugin::ResetDialogueBoundaryState(state.DialogueView());
            return true;
        }

        plugin::AcceptedCinematicEnterResult Enter(bool enabled,
            std::uint64_t enterSettingsEpoch, float authoredFov,
            const camera::GameplayBaseline& baseline, float effectiveAspect,
            config::CinematicFovMode mode =
                config::CinematicFovMode::GameplayHorPlus)
        {
            const auto selection = plugin::PrepareCinematicEnter(View(),
                config::CinematicAspectPolicy::Auto, mode);
            return plugin::ProcessAcceptedCinematicEnter(View(),
                {selection, enabled, enterSettingsEpoch, authoredFov, baseline,
                    effectiveAspect, 16.0f / 9.0f, 0, NAN, NAN, false});
        }
    };

    bool Check(bool value, const char* message)
    {
        if (!value) std::cerr << "FAIL: " << message << '\n';
        return value;
    }

    camera::GameplayBaseline Baseline(std::uintptr_t source = 0xA)
    {
        camera::GameplayBaseline baseline{};
        baseline.nativeFov = {90.6557f, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true};
        baseline.horPlusFov = {127.393f, camera::FovSpace::Transformed,
            camera::FovProvenance::ModTransformResult, true};
        baseline.aspect = 3.55556f;
        baseline.source = {source, true};
        baseline.valid = true;
        return baseline;
    }

    struct DiagnosticScenarioSample
    {
        float outputFov{NAN};
        bool mutate{};
        std::uint32_t action{};
        std::uint64_t generation{};
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        bool selectionValid{};
        bool associationValid{};
        bool associationConflict{};
        std::uintptr_t associatedRsi{};
        bool initialEstablishmentActive{};
        bool initialEstablishmentEnded{};
        bool recoveryOwnerValid{};
        bool recoveryComplete{};
        std::uintptr_t recoveryOwnerRsi{};
        bool exclusionActive{};
        bool continuationPending{};
        bool continuationActive{};
        dialogue::Phase dialoguePhase{dialogue::Phase::Inactive};
        std::uint64_t freshReadCalls{};
        std::uintptr_t freshReadSource{};
        bool freshReadIncludedFlags{};
        bool freshReadSourceMatched{};
    };

    struct DiagnosticScenarioDigest
    {
        std::array<DiagnosticScenarioSample, 12> samples{};
        std::size_t count{};
        std::uint32_t emittedMarkers{};
        std::uint32_t freshReadCalls{};
    };

    bool SameDiagnosticScenario(
        const DiagnosticScenarioDigest& left,
        const DiagnosticScenarioDigest& right)
    {
        if (left.count != right.count ||
            left.freshReadCalls != right.freshReadCalls)
            return false;
        for (std::size_t i = 0; i < left.count; ++i) {
            const auto& a = left.samples[i];
            const auto& b = right.samples[i];
            const bool sameOutput = a.outputFov == b.outputFov ||
                (std::isnan(a.outputFov) && std::isnan(b.outputFov));
            if (!sameOutput || a.mutate != b.mutate || a.action != b.action ||
                a.generation != b.generation || a.coordinator != b.coordinator ||
                a.selectionValid != b.selectionValid ||
                a.associationValid != b.associationValid ||
                a.associationConflict != b.associationConflict ||
                a.associatedRsi != b.associatedRsi ||
                a.initialEstablishmentActive != b.initialEstablishmentActive ||
                a.initialEstablishmentEnded != b.initialEstablishmentEnded ||
                a.recoveryOwnerValid != b.recoveryOwnerValid ||
                a.recoveryComplete != b.recoveryComplete ||
                a.recoveryOwnerRsi != b.recoveryOwnerRsi ||
                a.exclusionActive != b.exclusionActive ||
                a.continuationPending != b.continuationPending ||
                a.continuationActive != b.continuationActive ||
                a.dialoguePhase != b.dialoguePhase ||
                a.freshReadCalls != b.freshReadCalls ||
                a.freshReadSource != b.freshReadSource ||
                a.freshReadIncludedFlags != b.freshReadIncludedFlags ||
                a.freshReadSourceMatched != b.freshReadSourceMatched)
                return false;
        }
        return true;
    }
}

int main()
{
    Fixture state;
    bool pass = true;
    const cinematics::CinematicSelectionSnapshot selection{
        config::CinematicAspectPolicy::Auto,
        config::CinematicFovMode::GameplayHorPlus, 1, true};
    const auto enter = state.Enter(true, 3, 90.6557f, Baseline(), 3.55556f,
        selection.fovMode);
    pass &= Check(enter.accepted && enter.generation == 1 &&
        state.coordinator.load() == camera::CoordinatorState::CinematicActive &&
        state.selectionValid.load() && state.activeEnabled.load() &&
        std::fabs(state.transformed.load() - 127.393f) < 0.05f,
        "accepted ENTER commits generation, ownership, and target");
    const auto enterSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(enterSnapshot.selectionValid && enterSnapshot.generation == 1 &&
        enterSnapshot.coordinator == camera::CoordinatorState::CinematicActive &&
        enterSnapshot.targetEstablished &&
        std::fabs(enterSnapshot.targetFov - 127.393f) < 0.01f,
        "read-only lifecycle snapshot exposes production state after ENTER");
    const auto duplicateEnter = plugin::PrepareCinematicEnter(state.View(),
        selection.aspectPolicy, selection.fovMode);
    pass &= Check(!duplicateEnter.valid &&
        state.generation.load() == enter.generation,
        "rejected duplicate ENTER allocates no generation");

    const auto aspect = plugin::ObserveCinematicAspectStore(
        state.View(), AspectInput(0xA));
    pass &= Check(aspect.accepted &&
        aspect.association.aspectStoreTargetObject == 0xA &&
        aspect.correctionPolicyActive &&
        aspect.effectivePolicy == config::CinematicAspectPolicy::Auto &&
        std::fabs(aspect.autoAspectForLog - 3.55556f) < 0.001f &&
        std::fabs(aspect.aspectToStore - 3.55556f) < 0.001f,
        "aspect-store event records association and returns the effective aspect policy result");
    const auto aspectSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(aspectSnapshot.association.aspectStoreTargetObject == 0xA &&
        !aspectSnapshot.association.associationValid,
        "snapshot observes aspect-store event without granting writer association");
    float writableAspect = 16.0f / 9.0f;
    const auto successfulStore = cinematics::ApplyAspectStore(
        reinterpret_cast<std::uintptr_t>(&writableAspect), aspect.aspectToStore,
        16.0f / 9.0f, 0, &WritableForTest, &ValidAspectForTest);
    float refusedAspect = 16.0f / 9.0f;
    const auto refusedStore = cinematics::ApplyAspectStore(
        reinterpret_cast<std::uintptr_t>(&refusedAspect), aspect.aspectToStore,
        16.0f / 9.0f, 0, &ReadOnlyForTest, &ValidAspectForTest);
    pass &= Check(successfulStore.writable &&
        std::fabs(writableAspect - aspect.aspectToStore) < 0.001f &&
        !refusedStore.writable && std::fabs(refusedAspect - 16.0f / 9.0f) < 0.001f,
        "existing aspect-store operation reports successful and refused native write outcomes");
    auto disabledAspectInput = AspectInput(0xA);
    disabledAspectInput.runtimeEnabled = false;
    const auto disabledAspect = plugin::ObserveCinematicAspectStore(
        state.View(), disabledAspectInput);
    pass &= Check(disabledAspect.accepted && !disabledAspect.correctionPolicyActive &&
        disabledAspect.effectivePolicy == config::CinematicAspectPolicy::Native &&
        std::fabs(disabledAspect.aspectToStore - 16.0f / 9.0f) < 0.001f,
        "Cinematics OFF preserves lifecycle observation while selecting native aspect only");

    plugin::CameraCorrectionGate gate;
    std::atomic<std::uint8_t> flags{0x04};
    std::atomic<float> nativeAspect{3.55556f};
    plugin::CameraWriterRecoveryStateView writerState{
        state.coordinator, state.association, state.recoveryOwner, state.exclusion,
        state.handoffPending, state.handoffSource, state.handoffPreviousFov,
        state.continuationPending, state.continuationCarryPending,
        state.continuationSource,
        state.continuationExitTarget, state.continuationPreviousTarget,
        state.continuationInterpolation, state.continuationStart,
        state.gameplayContinuationSource, state.exitTarget, state.exitTransformed};
    auto writer = state.Writer(gate,
        {0xA, 0xAA, 90.6557f, nativeAspect.load(), true, flags.load(), true});
    pass &= Check(writer.processed && state.association.Read().associationValid &&
        state.association.Read().associatedWriterRsi == 0xA &&
        state.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "writer event associates only the aspect-store RSI without changing cinematic ownership");
    pass &= Check(writer.cinematicHandled && writer.mutateFov &&
        std::fabs(writer.outputFov - 127.393f) < 0.01f &&
        writer.initialEstablishmentAuthorized &&
        writer.associationAfterDecision.initialEstablishmentActive &&
        writer.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        writer.continuation == plugin::CameraWriterContinuation::ReturnCinematic,
        "single production writer transaction authorizes Initial Establishment and returns target output");
    const auto writerSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(writerSnapshot.association.associationValid &&
        writerSnapshot.association.associatedWriterRsi == 0xA &&
        writerSnapshot.coordinator == camera::CoordinatorState::CinematicActive,
        "snapshot exposes association and owner after production writer event");
    pass &= Check(enter.baselineSelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline &&
        enter.baselineSelection.usedGameplayBaselineValue &&
        std::fabs(enter.baselineSelection.targetBaselineFov - 90.6557f) < 0.001f,
        "ENTER event selects the numeric gameplay baseline without deriving source identity");

    const auto repeatedBaseline = state.Writer(gate,
        {0xA, 0xAA, 91.1f, nativeAspect.load(), true, flags.load(), true});
    const auto repeatedSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(repeatedBaseline.initialEstablishmentAuthorized &&
        repeatedBaseline.mutateFov &&
        std::fabs(repeatedBaseline.outputFov - enter.outputFov) < 0.001f &&
        repeatedSnapshot.association.initialEstablishmentActive,
        "repeated baseline-equivalent sample remains retained by the production writer event");

    const auto baselineDeparture = state.Writer(gate,
        {0xA, 0xAA, 92.0f, nativeAspect.load(), true, flags.load(), true});
    const auto departureSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(!baselineDeparture.initialEstablishmentAuthorized &&
        !baselineDeparture.mutateFov &&
        departureSnapshot.association.initialEstablishmentEnded &&
        departureSnapshot.association.initialEstablishmentEndReason ==
            cinematics::InitialEstablishmentEndReason::AuthoredBaselineDeparture,
        "first sample outside the semantic baseline region ends Initial Establishment");

    const auto returnedBaseline = state.Writer(gate,
        {0xA, 0xAA, 90.6557f, nativeAspect.load(), true, flags.load(), true});
    const auto returnedSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(!returnedBaseline.initialEstablishmentAuthorized &&
        !returnedBaseline.mutateFov && returnedBaseline.outputFov == 90.6557f &&
        returnedSnapshot.association.initialEstablishmentEnded &&
        !returnedSnapshot.association.initialEstablishmentActive,
        "return to baseline after departure does not re-arm Initial Establishment");

    const auto transition = gameplay::ResolveCinematicExitTransition(true, true,
        config::GameplayMode::HorPlus);
    const auto exit = plugin::ProcessAcceptedCinematicExit(state.View(),
        {state.coordinator.load(), transition.nextState, enter.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA, 127.393f});
    pass &= Check(exit.accepted && exit.recoveryOwnerCaptured &&
        state.recoveryOwner.Read().HasRecoveryAuthority() &&
        state.recoveryOwner.Read().writerRsi == 0xA &&
        state.coordinator.load() == camera::CoordinatorState::CinematicExiting &&
        !state.selectionValid.load(),
        "accepted EXIT snapshots owner A and enters recovery lifecycle");
    const auto exitSnapshot = plugin::ReadCinematicLifecycleSnapshot(state.View());
    pass &= Check(exitSnapshot.coordinator == camera::CoordinatorState::CinematicExiting &&
        exitSnapshot.recoveryOwner.HasRecoveryAuthority() &&
        exitSnapshot.recoveryOwner.writerRsi == 0xA && !exitSnapshot.selectionValid,
        "snapshot exposes recovery state after accepted EXIT");

    const auto duplicateExit = plugin::ProcessAcceptedCinematicExit(state.View(),
        {state.coordinator.load(), transition.nextState, enter.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA, 127.393f});
    pass &= Check(!duplicateExit.accepted &&
        state.recoveryOwner.Read().writerRsi == 0xA &&
        state.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "duplicate EXIT has no lifecycle mutation authority");

    Fixture recoveryState;
    plugin::CameraCorrectionGate recoveryGate;
    const auto recoveryEnter = recoveryState.Enter(true, 11, 90.6557f,
        Baseline(), 3.55556f);
    const auto recoveryAspect = plugin::ObserveCinematicAspectStore(
        recoveryState.View(), AspectInput(0xA));
    const auto recoveryInitialWriter = recoveryState.Writer(recoveryGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto recoveryExitTransition = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto recoveryExit = plugin::ProcessAcceptedCinematicExit(
        recoveryState.View(), {recoveryState.coordinator.load(),
            recoveryExitTransition.nextState, recoveryEnter.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA,
            recoveryEnter.outputFov});
    pass &= Check(recoveryEnter.accepted && recoveryAspect.accepted &&
        recoveryInitialWriter.mutateFov && recoveryExit.accepted &&
        recoveryExit.recoveryOwnerCaptured &&
        recoveryState.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "production lifecycle events establish a source-owned HorPlus recovery");

    FreshFieldsFixture freshFields{
        {3.55556f, 0x04, false, false}, 0xA, 0, false};
    const plugin::CameraWriterEngineOperations freshReader{
        &freshFields, &ReadFreshFields};
    plugin::CameraWriterInput recoveryWriterInput{
        0xA, 0xAA, 110.0f, 3.55556f, true, 0x04, true,
        0.02f, true, config::GameplayMode::HorPlus, 16.0f / 9.0f};
    recoveryWriterInput.currentViewportAspect = 2.38889f;
    recoveryWriterInput.currentViewportAspectValid = true;
    const auto unreadableRecovery = recoveryState.Writer(recoveryGate,
        recoveryWriterInput, freshReader);
    pass &= Check(freshFields.calls == 1 && freshFields.sourceMatched &&
        unreadableRecovery.horPlusRecoveryProcessed &&
        unreadableRecovery.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::HoldNativePassThrough &&
        unreadableRecovery.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        !unreadableRecovery.mutateFov && unreadableRecovery.outputFov == 110.0f &&
        recoveryState.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "fresh field read failure preserves native recovery pass-through");

    freshFields.fields.aspectReadable = true;
    freshFields.fields.flagsReadable = true;
    recoveryWriterInput.currentViewportAspect = std::numeric_limits<float>::quiet_NaN();
    recoveryWriterInput.currentViewportAspectValid = false;
    const auto invalidPolicyRecovery = recoveryState.Writer(recoveryGate,
        recoveryWriterInput, freshReader);
    pass &= Check(invalidPolicyRecovery.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation &&
        invalidPolicyRecovery.mutateFov &&
        std::fabs(invalidPolicyRecovery.outputFov - 127.393f) < 0.02f &&
        recoveryState.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "recovery policy remains native-writer-authorized when viewport telemetry is unavailable");

    recoveryWriterInput.currentViewportAspect = 2.38889f;
    recoveryWriterInput.currentViewportAspectValid = true;
    const auto interpolationRecovery = recoveryState.Writer(recoveryGate,
        recoveryWriterInput, freshReader);
    pass &= Check(freshFields.calls == 3 &&
        interpolationRecovery.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation &&
        interpolationRecovery.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        interpolationRecovery.mutateFov &&
        std::fabs(interpolationRecovery.outputFov - 127.393f) < 0.02f &&
        recoveryState.coordinator.load() == camera::CoordinatorState::CinematicExiting &&
        recoveryState.recoveryOwner.Read().HasRecoveryAuthority(),
        "recovery validates and targets the native-writer Gameplay HorPlus authority");

    recoveryWriterInput.incomingFov = 90.6557f;
    const auto endpointRecovery = recoveryState.Writer(recoveryGate,
        recoveryWriterInput, freshReader);
    const auto endpointSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        recoveryState.View());
    pass &= Check(endpointRecovery.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::ResumeGameplay &&
        endpointRecovery.horPlusRecovery.completed &&
        endpointRecovery.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        endpointRecovery.gameplay.handled && endpointRecovery.mutateFov &&
        std::fabs(endpointRecovery.outputFov - 127.393f) < 0.02f &&
        endpointSnapshot.coordinator == camera::CoordinatorState::Gameplay &&
        !endpointSnapshot.recoveryOwner.HasRecoveryAuthority(),
        "native endpoint completes recovery and same writer transaction uses native Gameplay policy aspect");

    Fixture runtimeCombineRecoveryState;
    plugin::CameraCorrectionGate runtimeCombineRecoveryGate;
    const auto runtimeCombineEnter = runtimeCombineRecoveryState.Enter(true, 12,
        90.0f, Baseline(), 3.55556f);
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        runtimeCombineRecoveryState.View(), AspectInput(0xA)));
    FreshFieldsFixture runtimeCombineFields{
        {3.55556f, 0x04, true, true}, 0xA, 0, false};
    const plugin::CameraWriterEngineOperations runtimeCombineReader{
        &runtimeCombineFields, &ReadFreshFields};
    plugin::CameraWriterInput runtimeCombineCinematicInput{
        0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true,
        0.02f, true, config::GameplayMode::RuntimeCombine, 16.0f / 9.0f};
    runtimeCombineCinematicInput.currentViewportAspect = 3.55556f;
    runtimeCombineCinematicInput.currentViewportAspectValid = true;
    const auto runtimeCombineCinematicWriter = runtimeCombineRecoveryState.Writer(
        runtimeCombineRecoveryGate, runtimeCombineCinematicInput,
        runtimeCombineReader);
    const auto runtimeCombineExitTransition = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::RuntimeCombine);
    const auto runtimeCombineExit = plugin::ProcessAcceptedCinematicExit(
        runtimeCombineRecoveryState.View(), {
            runtimeCombineRecoveryState.coordinator.load(),
            runtimeCombineExitTransition.nextState, runtimeCombineEnter.generation,
            90.6557f, true, config::GameplayMode::RuntimeCombine, 0xA,
            runtimeCombineCinematicWriter.outputFov});
    pass &= Check(runtimeCombineEnter.accepted &&
        runtimeCombineCinematicWriter.cinematicHandled &&
        runtimeCombineCinematicWriter.mutateFov &&
        std::fabs(runtimeCombineCinematicWriter.outputFov - 127.393f) < 0.02f &&
        runtimeCombineExit.accepted && runtimeCombineExit.recoveryOwnerCaptured &&
        runtimeCombineRecoveryState.coordinator.load() ==
            camera::CoordinatorState::CinematicExiting,
        "RuntimeCombine setup reaches CinematicExiting through production ENTER/writer/EXIT transactions");

    auto runtimeCombineRecoveryWriter = [&](float fov) {
        runtimeCombineCinematicInput.incomingFov = fov;
        return runtimeCombineRecoveryState.Writer(runtimeCombineRecoveryGate,
            runtimeCombineCinematicInput, runtimeCombineReader);
    };
    const auto runtimeCombineExitFirst = runtimeCombineRecoveryWriter(127.393f);
    pass &= Check(runtimeCombineExitFirst.horPlusRecoveryProcessed &&
        runtimeCombineExitFirst.mutateFov &&
        std::fabs(runtimeCombineExitFirst.outputFov - 127.393f) < 0.02f &&
        runtimeCombineRecoveryState.coordinator.load() ==
            camera::CoordinatorState::CinematicExiting &&
        runtimeCombineRecoveryState.recoveryOwner.Read().HasRecoveryAuthority(),
        "RuntimeCombine keeps the confirmed Cinematic/Gameplay target on the first EXIT writer sample");
    const auto runtimeCombineExitMid = runtimeCombineRecoveryWriter(110.0f);
    pass &= Check(runtimeCombineExitMid.horPlusRecoveryProcessed &&
        runtimeCombineExitMid.mutateFov &&
        std::fabs(runtimeCombineExitMid.outputFov - 127.393f) < 0.02f &&
        runtimeCombineRecoveryState.coordinator.load() ==
            camera::CoordinatorState::CinematicExiting &&
        runtimeCombineRecoveryState.recoveryOwner.Read().HasRecoveryAuthority(),
        "RuntimeCombine masks native EXIT interpolation until Gameplay handoff is validated");
    const auto runtimeCombineExitEndpoint = runtimeCombineRecoveryWriter(90.6557f);
    const auto runtimeCombineEndpointSnapshot =
        plugin::ReadCinematicLifecycleSnapshot(runtimeCombineRecoveryState.View());
    pass &= Check(runtimeCombineExitEndpoint.horPlusRecoveryProcessed &&
        runtimeCombineExitEndpoint.horPlusRecovery.completed &&
        runtimeCombineExitEndpoint.gameplay.handled &&
        runtimeCombineExitEndpoint.gameplay.runtimeCombineConfirmed &&
        runtimeCombineExitEndpoint.mutateFov &&
        std::fabs(runtimeCombineExitEndpoint.outputFov - 127.393f) < 0.02f &&
        runtimeCombineEndpointSnapshot.coordinator == camera::CoordinatorState::Gameplay &&
        !runtimeCombineEndpointSnapshot.recoveryOwner.HasRecoveryAuthority(),
        "RuntimeCombine endpoint completes recovery and the same callback resumes confirmed Gameplay output");

    Fixture runtimeCombineUnconfirmedState;
    plugin::CameraCorrectionGate runtimeCombineUnconfirmedGate;
    const auto runtimeCombineUnconfirmedEnter = runtimeCombineUnconfirmedState.Enter(
        true, 13, 90.0f, Baseline(), 3.55556f);
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        runtimeCombineUnconfirmedState.View(), AspectInput(0xA)));
    FreshFieldsFixture runtimeCombineUnconfirmedFields{
        {2.38889f, 0x04, true, true}, 0xA, 0, false};
    const plugin::CameraWriterEngineOperations runtimeCombineUnconfirmedReader{
        &runtimeCombineUnconfirmedFields, &ReadFreshFields};
    runtimeCombineCinematicInput.incomingFov = 90.6557f;
    const auto runtimeCombineUnconfirmedCinematicWriter =
        runtimeCombineUnconfirmedState.Writer(runtimeCombineUnconfirmedGate,
            runtimeCombineCinematicInput, runtimeCombineUnconfirmedReader);
    const auto runtimeCombineUnconfirmedExitTransition =
        gameplay::ResolveCinematicExitTransition(true, true,
            config::GameplayMode::RuntimeCombine);
    const auto runtimeCombineUnconfirmedExit = plugin::ProcessAcceptedCinematicExit(
        runtimeCombineUnconfirmedState.View(), {
            runtimeCombineUnconfirmedState.coordinator.load(),
            runtimeCombineUnconfirmedExitTransition.nextState,
            runtimeCombineUnconfirmedEnter.generation, 90.6557f, true,
            config::GameplayMode::RuntimeCombine, 0xA,
            runtimeCombineUnconfirmedCinematicWriter.outputFov});
    runtimeCombineCinematicInput.incomingFov = 110.0f;
    runtimeCombineCinematicInput.currentViewportAspectValid = true;
    const auto runtimeCombineUnconfirmedRecovery =
        runtimeCombineUnconfirmedState.Writer(runtimeCombineUnconfirmedGate,
            runtimeCombineCinematicInput, runtimeCombineUnconfirmedReader);
    const auto runtimeCombineUnconfirmedSnapshot =
        plugin::ReadCinematicLifecycleSnapshot(runtimeCombineUnconfirmedState.View());
    pass &= Check(runtimeCombineUnconfirmedExit.accepted &&
        runtimeCombineUnconfirmedRecovery.gameplay.runtimeCombineTelemetryValid &&
        runtimeCombineUnconfirmedRecovery.gameplay.runtimeCombineMismatch &&
        runtimeCombineUnconfirmedRecovery.gameplay.runtimeCombineWriteAttempted &&
        !runtimeCombineUnconfirmedRecovery.gameplay.runtimeCombineWriteSucceeded &&
        !runtimeCombineUnconfirmedRecovery.gameplay.runtimeCombineConfirmed &&
        runtimeCombineUnconfirmedRecovery.mutateFov &&
        std::fabs(runtimeCombineUnconfirmedRecovery.outputFov - 127.393f) < 0.02f &&
        runtimeCombineUnconfirmedSnapshot.coordinator ==
            camera::CoordinatorState::CinematicExiting &&
        runtimeCombineUnconfirmedSnapshot.recoveryOwner.HasRecoveryAuthority() &&
        runtimeCombineUnconfirmedFields.fields.aspect == 2.38889f &&
        runtimeCombineUnconfirmedFields.fields.flags == 0x04,
        "unconfirmed RuntimeCombine recovery holds only the prior confirmed Cinematic output and keeps native fields unchanged");

    Fixture noOwnerState;
    plugin::CameraCorrectionGate noOwnerGate;
    const auto noOwnerEnter = noOwnerState.Enter(true, 12, 90.6557f,
        Baseline(), 3.55556f);
    const auto noOwnerExitTransition = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto noOwnerExit = plugin::ProcessAcceptedCinematicExit(
        noOwnerState.View(), {noOwnerState.coordinator.load(),
            noOwnerExitTransition.nextState, noOwnerEnter.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA, 127.393f});
    DialogueResetFixture dialogueReset{};
    const auto noOwnerWriter = noOwnerState.Writer(noOwnerGate,
        {0xB, 0xBB, 92.0f, 3.55556f, true, 0x04, true}, {},
        {&dialogueReset, &ResetDialogueForTest});
    const auto noOwnerSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        noOwnerState.View());
    pass &= Check(noOwnerExit.accepted && !noOwnerExit.recoveryOwnerCaptured &&
        noOwnerWriter.noOwnerRecoveryCleanup &&
        noOwnerWriter.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        noOwnerWriter.outputFov == 92.0f && !noOwnerWriter.mutateFov &&
        noOwnerSnapshot.coordinator == camera::CoordinatorState::Gameplay &&
        !noOwnerSnapshot.recoveryOwner.valid && !noOwnerSnapshot.exclusionActive &&
        !noOwnerSnapshot.handoffPending && dialogueReset.calls == 1,
        "no-owner recovery cleanup clears auxiliary state and resets Dialogue before Gameplay");

    Fixture foreignState;
    plugin::CameraCorrectionGate foreignGate;
    const auto foreignEnter = foreignState.Enter(true, 13, 90.6557f,
        Baseline(), 3.55556f);
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        foreignState.View(), AspectInput(0xA)));
    const auto associatedWriter = foreignState.Writer(foreignGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto cinematicForeignWriter = foreignState.Writer(foreignGate,
        {0xB, 0xBB, 91.0f, 3.55556f, true, 0x04, true});
    const auto foreignExitTransition = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto foreignExit = plugin::ProcessAcceptedCinematicExit(
        foreignState.View(), {foreignState.coordinator.load(),
            foreignExitTransition.nextState, foreignEnter.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA,
            associatedWriter.outputFov});
    DialogueResetFixture foreignDialogueReset{};
    const auto foreignRecoveryWriter = foreignState.Writer(foreignGate,
        {0xB, 0xBB, 91.0f, 3.55556f, true, 0x04, true}, {},
        {&foreignDialogueReset, &ResetDialogueForTest});
    const auto foreignSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        foreignState.View());
    pass &= Check(associatedWriter.cinematicHandled &&
        !cinematicForeignWriter.mutateFov && foreignExit.accepted &&
        foreignExit.recoveryOwnerCaptured &&
        foreignRecoveryWriter.foreignSourceRejected &&
        foreignRecoveryWriter.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        foreignRecoveryWriter.outputFov == 91.0f && !foreignRecoveryWriter.mutateFov &&
        foreignSnapshot.coordinator == camera::CoordinatorState::CinematicExiting &&
        foreignSnapshot.recoveryOwner.writerRsi == 0xA &&
        foreignSnapshot.recoveryOwner.HasRecoveryAuthority() &&
        foreignDialogueReset.calls == 0,
        "foreign writer returns native without clearing or taking owner A recovery");

    const auto retiredWriter = state.Writer(gate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(retiredWriter.recoveryRetired &&
        retiredWriter.disposition == plugin::CameraWriterDisposition::FinalOutput &&
        retiredWriter.continuation == plugin::CameraWriterContinuation::ContinueRouting &&
        !retiredWriter.cinematicHandled && retiredWriter.gameplay.handled &&
        retiredWriter.mutateFov &&
        std::fabs(retiredWriter.outputFov - 127.393f) < 0.01f &&
        state.coordinator.load() == camera::CoordinatorState::Gameplay &&
        !state.recoveryOwner.Read().valid,
        "A EXIT recovery retires on B and same transaction returns Gameplay output");

    Fixture continuationState;
    plugin::CameraCorrectionGate continuationGate;
    auto continuationSelection = plugin::PrepareCinematicEnter(
        continuationState.View(), selection.aspectPolicy, selection.fovMode);
    const auto firstContinuationEnter = plugin::ProcessAcceptedCinematicEnter(
        continuationState.View(),
        {continuationSelection, true, 8, 90.6557f, Baseline(), 3.55556f,
            16.0f / 9.0f, 0, NAN, NAN, false});
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        continuationState.View(), AspectInput(0xA)));
    plugin::CameraWriterRecoveryStateView continuationWriterState{
        continuationState.coordinator, continuationState.association,
        continuationState.recoveryOwner, continuationState.exclusion,
        continuationState.handoffPending, continuationState.handoffSource,
        continuationState.handoffPreviousFov, continuationState.continuationPending,
        continuationState.continuationCarryPending,
        continuationState.continuationSource, continuationState.continuationExitTarget,
        continuationState.continuationPreviousTarget,
        continuationState.continuationInterpolation, continuationState.continuationStart,
        continuationState.gameplayContinuationSource, continuationState.exitTarget,
        continuationState.exitTransformed};
    const auto firstGenerationWriter = continuationState.Writer(continuationGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto exitTransition = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto continuationExit = plugin::ProcessAcceptedCinematicExit(
        continuationState.View(),
        {continuationState.coordinator.load(), exitTransition.nextState,
            firstContinuationEnter.generation, 90.6557f, true,
            config::GameplayMode::HorPlus, 0xA, 127.393f});
    continuationSelection = plugin::PrepareCinematicEnter(
        continuationState.View(), selection.aspectPolicy, selection.fovMode);
    const auto continuedEnter = plugin::ProcessAcceptedCinematicEnter(
        continuationState.View(),
        {continuationSelection, true, 8, 90.6557f, Baseline(), 3.55556f,
            16.0f / 9.0f, 0xA, 90.6557f, 127.393f, true});
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        continuationState.View(), AspectInput(0xA)));
    const auto continuationWriter = continuationState.Writer(continuationGate,
        {0xA, 0xAA, 100.0f, 3.55556f, true, 0x04, true});
    pass &= Check(firstContinuationEnter.accepted && firstGenerationWriter.mutateFov &&
        continuationExit.accepted && continuationExit.continuityCanBeEvaluated &&
        continuedEnter.accepted && continuedEnter.continuedOwnership &&
        continuationState.continuationInterpolation.load() &&
        continuationWriter.cinematicHandled && continuationWriter.mutateFov &&
        continuationWriter.cinematicAction ==
            cinematics::CinematicWriterAction::RetainContinuationTarget &&
        std::fabs(continuationWriter.outputFov - 127.393f) < 0.01f,
        "single production writer transaction retains continuation target after lifecycle reentry");

    Fixture completedState;
    plugin::CameraCorrectionGate completedGate;
    const auto completedEnter = plugin::ProcessAcceptedCinematicEnter(
        completedState.View(),
        {plugin::PrepareCinematicEnter(completedState.View(),
                config::CinematicAspectPolicy::Auto,
                config::CinematicFovMode::GameplayHorPlus),
            true, 4, 90.6557f, Baseline(), 3.55556f,
            16.0f / 9.0f, 0, NAN, NAN, false});
    const auto completedAspect = plugin::ObserveCinematicAspectStore(
        completedState.View(), AspectInput(0xA));
    const auto completedAEntry = completedState.Writer(completedGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto completedExitPolicy = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto completedExit = plugin::ProcessAcceptedCinematicExit(
        completedState.View(),
        {completedState.coordinator.load(), completedExitPolicy.nextState,
            completedEnter.generation, 90.6557f, true,
            config::GameplayMode::HorPlus, 0xA, 127.393f});
    const auto recoveryA = completedState.Writer(completedGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(completedAspect.accepted && completedAEntry.processed &&
        completedExit.recoveryOwnerCaptured && recoveryA.processed &&
        recoveryA.exclusionObservation ==
            dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed &&
        recoveryA.horPlusRecovery.completed &&
        recoveryA.horPlusRecovery.completion.exclusionActive &&
        completedState.recoveryOwner.Read().recoveryComplete &&
        completedState.exclusion.IsActive(),
        "one production writer event completes A recovery while exclusion remains active");
    const auto dialogueDuringExclusion = completedState.DialogueEvent(
        completedGate, 90.0f, 0xD);
    pass &= Check(dialogueDuringExclusion.action ==
            plugin::DialogueBoundaryAction::ExclusionReset &&
        dialogueDuringExclusion.phaseAfter == dialogue::Phase::Inactive &&
        !dialogueDuringExclusion.mutateXmm1 && completedState.exclusion.IsActive(),
        "post-cinematic exclusion rejects Dialogue correction without consuming exclusion");
    const auto completedB = completedState.Writer(completedGate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(completedB.exclusionCleared &&
        completedB.exclusionObservation ==
            dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled &&
        !completedState.exclusion.IsActive() &&
        !completedState.recoveryOwner.Read().valid &&
        completedB.continuation == plugin::CameraWriterContinuation::ContinueRouting &&
        !completedB.cinematicHandled && completedB.gameplay.handled &&
        completedB.mutateFov && std::fabs(completedB.outputFov - 127.393f) < 0.01f,
        "completed-owner exclusion cancels on B and same transaction returns Gameplay output");

    Fixture cinematicDialogueState;
    plugin::CameraCorrectionGate cinematicDialogueGate;
    const auto cinematicDialogueEnter = cinematicDialogueState.Enter(true, 1,
        90.6557f, Baseline(), 3.55556f);
    const auto dialogueCandidateDuringCinematic =
        cinematicDialogueState.DialogueEvent(cinematicDialogueGate, 90.0f, 0xD);
    pass &= Check(cinematicDialogueEnter.accepted &&
        cinematicDialogueState.coordinator.load() ==
            camera::CoordinatorState::CinematicActive &&
        dialogueCandidateDuringCinematic.action ==
            plugin::DialogueBoundaryAction::NonGameplayReset &&
        cinematicDialogueState.dialoguePhase == dialogue::Phase::Inactive &&
        !dialogueCandidateDuringCinematic.mutateXmm1,
        "Dialogue candidate observed during CinematicActive cannot acquire correction ownership");

    Fixture dialogueWriterChangeState;
    plugin::CameraCorrectionGate dialogueWriterChangeGate;
    const auto writerA = dialogueWriterChangeState.Writer(dialogueWriterChangeGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto dialogueCandidate = dialogueWriterChangeState.DialogueEvent(
        dialogueWriterChangeGate, 90.0f, 0xD);
    const auto dialogueAccepted = dialogueWriterChangeState.DialogueEvent(
        dialogueWriterChangeGate, 89.8f, 0xD);
    const auto writerB = dialogueWriterChangeState.Writer(dialogueWriterChangeGate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true}, {}, {},
        {&dialogueWriterChangeState,
            &Fixture::ResetDialogueOnGameplayContextChange});
    pass &= Check(writerA.gameplay.handled &&
        dialogueCandidate.phaseAfter == dialogue::Phase::Candidate &&
        dialogueAccepted.phaseAfter == dialogue::Phase::Active &&
        writerB.gameplay.dialogueContextInvalidated &&
        dialogueWriterChangeState.dialoguePhase == dialogue::Phase::Inactive &&
        writerB.gameplay.applied && writerB.mutateFov &&
        std::fabs(writerB.outputFov - 127.393f) < 0.01f,
        "Gameplay writer context change resets active Dialogue before same B writer output");

    Fixture cinematicOffFirst;
    plugin::CameraCorrectionGate cinematicOffFirstGate;
    static_cast<void>(plugin::ApplyCameraSetting(cinematicOffFirstGate,
        cinematicOffFirst.SettingsView(),
        plugin::RuntimeSettingMutation::CinematicsEnabled(false), true, [] {}));
    plugin::AcceptedCinematicEnterResult offFirstEnter{};
    {
        auto gateLock = cinematicOffFirstGate.Acquire();
        offFirstEnter = cinematicOffFirst.Enter(
            cinematicOffFirst.settingCinematicsEnabled.load(), 0,
            90.6557f, Baseline(), 3.55556f);
    }
    auto offFirstAspectInput = AspectInput(0xC);
    offFirstAspectInput.runtimeEnabled = false;
    const auto offFirstAspect = plugin::ObserveCinematicAspectStore(
        cinematicOffFirst.View(), offFirstAspectInput);
    const auto offFirstWriter = cinematicOffFirst.Writer(cinematicOffFirstGate,
        {0xC, 0xCC, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(offFirstEnter.accepted && !offFirstEnter.transformed &&
        offFirstAspect.accepted && offFirstWriter.processed &&
        !offFirstWriter.mutateFov && offFirstWriter.outputFov == 90.6557f &&
        cinematicOffFirst.coordinator.load() ==
            camera::CoordinatorState::CinematicActive &&
        !cinematicOffFirst.activeEnabled.load(),
        "Cinematics OFF-first keeps lifecycle observation but writer output native");

    Fixture cinematicWriterFirst;
    plugin::CameraCorrectionGate cinematicWriterFirstGate;
    const auto writerFirstEnter = cinematicWriterFirst.Enter(true, 1,
        90.6557f, Baseline(), 3.55556f);
    static_cast<void>(plugin::ObserveCinematicAspectStore(
        cinematicWriterFirst.View(), AspectInput(0xE)));
    std::mutex cinematicRendezvousMutex;
    std::condition_variable cinematicRendezvous;
    bool cinematicWriterPaused = false;
    bool cinematicReleaseWriter = false;
    bool cinematicOffAttempted = false;
    std::atomic<bool> cinematicOffReturned{};
    plugin::CameraWriterResult cinematicInFlight{};
    float committedXmm0 = 90.6557f;
    std::thread cinematicWriter([&] {
        auto gateLock = cinematicWriterFirstGate.Acquire();
        {
            std::unique_lock lock(cinematicRendezvousMutex);
            cinematicWriterPaused = true;
            cinematicRendezvous.notify_all();
            cinematicRendezvous.wait(lock, [&] { return cinematicReleaseWriter; });
        }
        cinematicInFlight = cinematicWriterFirst.WriterTransaction(
            cinematicWriterFirstGate,
            {0xE, 0xEE, 90.6557f, 3.55556f, true, 0x04, true});
        if (cinematicInFlight.mutateFov)
            committedXmm0 = cinematicInFlight.outputFov;
    });
    {
        std::unique_lock lock(cinematicRendezvousMutex);
        cinematicRendezvous.wait(lock, [&] { return cinematicWriterPaused; });
    }
    std::thread cinematicDisable([&] {
        {
            std::lock_guard lock(cinematicRendezvousMutex);
            cinematicOffAttempted = true;
            cinematicRendezvous.notify_all();
        }
        const auto result = plugin::ApplyCameraSetting(cinematicWriterFirstGate,
            cinematicWriterFirst.SettingsView(),
            plugin::RuntimeSettingMutation::CinematicsEnabled(false), true, [] {});
        cinematicOffReturned.store(result.mutation.accepted,
            std::memory_order_release);
    });
    {
        std::unique_lock lock(cinematicRendezvousMutex);
        cinematicRendezvous.wait(lock, [&] { return cinematicOffAttempted; });
        pass &= Check(!cinematicOffReturned.load(std::memory_order_acquire),
            "Cinematics OFF cannot return while the writer owns the correction gate");
        cinematicReleaseWriter = true;
        cinematicRendezvous.notify_all();
    }
    cinematicWriter.join();
    cinematicDisable.join();
    pass &= Check(writerFirstEnter.accepted && cinematicInFlight.processed &&
        cinematicInFlight.initialEstablishmentAuthorized &&
        cinematicInFlight.mutateFov && std::fabs(committedXmm0 - 127.393f) < 0.01f &&
        cinematicOffReturned.load() && !cinematicWriterFirst.activeEnabled.load() &&
        std::isnan(cinematicWriterFirst.transformed.load()),
        "Cinematics writer-first commits its real output before OFF clears active correction");

    // C5 P01-P04: meaningful module Enabled/mode combinations go through
    // production settings and lifecycle/writer/Dialogue transactions.
    Fixture p01;
    plugin::CameraCorrectionGate p01Gate;
    const auto p01GameplayOff = plugin::ApplyCameraSetting(p01Gate,
        p01.SettingsView(), plugin::RuntimeSettingMutation::GameplayEnabled(false),
        true, [] {});
    pass &= Check(p01GameplayOff.mutation.changed &&
        !p01.settingGameplayEnabled.load(),
        "P01 Gameplay OFF setting event is applied before cinematic events");
    const auto p01Enter = p01.Enter(true, 1, 90.6557f, Baseline(), 3.55556f);
    pass &= Check(p01Enter.accepted &&
        p01.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "P01 ENTER remains observable with Gameplay OFF");
    const auto p01Aspect = plugin::ObserveCinematicAspectStore(p01.View(),
        AspectInput(0xA));
    pass &= Check(p01Aspect.accepted &&
        plugin::ReadCinematicLifecycleSnapshot(p01.View()).association
            .aspectStoreTargetObject == 0xA,
        "P01 aspect-store observation associates lifecycle target with Gameplay OFF");
    const auto p01Writer = p01.Writer(p01Gate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(p01Writer.cinematicHandled && p01Writer.mutateFov &&
        std::fabs(p01Writer.outputFov - p01Enter.outputFov) < 0.01f,
        "P01 Gameplay OFF with Cinematics ON preserves cinematic correction");

    Fixture p02;
    plugin::CameraCorrectionGate p02Gate;
    const auto p02Mode = plugin::ApplyCameraSetting(p02Gate, p02.SettingsView(),
        plugin::RuntimeSettingMutation::Gameplay(
            config::GameplayMode::AspectRecalculation), true, [] {});
    pass &= Check(p02Mode.mutation.accepted &&
        p02.settingGameplayMode.load() == config::GameplayMode::AspectRecalculation,
        "P02 Gameplay mode setting event selects AspectRecalculation");
    const auto p02CinematicsOff = plugin::ApplyCameraSetting(p02Gate,
        p02.SettingsView(), plugin::RuntimeSettingMutation::CinematicsEnabled(false),
        true, [] {});
    pass &= Check(p02CinematicsOff.mutation.changed &&
        !p02.settingCinematicsEnabled.load(),
        "P02 Cinematics OFF setting event is applied");
    const auto p02Enter = p02.Enter(p02.settingCinematicsEnabled.load(), 0,
        90.6557f, Baseline(), 3.55556f);
    pass &= Check(p02Enter.accepted && !p02Enter.transformed &&
        p02.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "P02 ENTER is observed but not transformed while Cinematics is OFF");
    auto p02AspectInput = AspectInput(0xB);
    p02AspectInput.runtimeEnabled = false;
    const auto p02Aspect = plugin::ObserveCinematicAspectStore(p02.View(),
        p02AspectInput);
    pass &= Check(p02Aspect.accepted,
        "P02 aspect-store event remains observable while Cinematics is OFF");
    const auto p02Writer = p02.Writer(p02Gate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(p02Writer.processed && !p02Writer.mutateFov &&
        p02Writer.outputFov == 90.6557f,
        "P02 Gameplay AspectRecalculation with Cinematics OFF observes lifecycle without correction");

    Fixture p03;
    plugin::CameraCorrectionGate p03Gate;
    const auto p03DialoguePolicy = plugin::ApplyCameraSetting(p03Gate,
        p03.SettingsView(), plugin::RuntimeSettingMutation::Dialogue(
            config::DialogueZoomPolicy::Reduced), true, [] {});
    pass &= Check(p03DialoguePolicy.mutation.accepted &&
        p03.dialoguePolicy.load() == config::DialogueZoomPolicy::Reduced,
        "P03 Dialogue policy setting is published before disabling Dialogue");
    const auto p03DialogueOff = plugin::ApplyCameraSetting(p03Gate,
        p03.SettingsView(), plugin::RuntimeSettingMutation::DialogueEnabled(false),
        true, [] {});
    pass &= Check(p03DialogueOff.mutation.changed &&
        !p03.dialogueEnabled.load(),
        "P03 Dialogue OFF setting event is applied");
    const auto p03Writer = p03.Writer(p03Gate,
        {0xC, 0xCC, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(p03Writer.gameplay.applied && p03Writer.mutateFov,
        "P03 Gameplay HorPlus writer still corrects with Dialogue OFF");
    const auto p03Candidate = p03.DialogueEvent(p03Gate, 90.0f, 0xD);
    pass &= Check(p03Candidate.action ==
        plugin::DialogueBoundaryAction::CandidateCaptured &&
        p03.dialoguePhase == dialogue::Phase::Candidate,
        "P03 Dialogue candidate remains observable while correction is OFF");
    const auto p03Active = p03.DialogueEvent(p03Gate, 89.8f, 0xD);
    pass &= Check(p03Active.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        p03.dialoguePhase == dialogue::Phase::Active && !p03Active.mutateXmm1,
        "P03 Gameplay ON with Dialogue OFF preserves Gameplay output and Dialogue observation only");

    Fixture p04;
    plugin::CameraCorrectionGate p04Gate;
    const auto p04GameplayOff = plugin::ApplyCameraSetting(p04Gate,
        p04.SettingsView(), plugin::RuntimeSettingMutation::GameplayEnabled(false),
        true, [] {});
    const auto p04Mode = plugin::ApplyCameraSetting(p04Gate, p04.SettingsView(),
        plugin::RuntimeSettingMutation::Gameplay(
            config::GameplayMode::AspectRecalculation), true, [] {});
    const auto p04CinematicsOff = plugin::ApplyCameraSetting(p04Gate,
        p04.SettingsView(), plugin::RuntimeSettingMutation::CinematicsEnabled(false),
        true, [] {});
    const auto p04DialogueOff = plugin::ApplyCameraSetting(p04Gate,
        p04.SettingsView(), plugin::RuntimeSettingMutation::DialogueEnabled(false),
        true, [] {});
    pass &= Check(p04GameplayOff.mutation.changed && p04Mode.mutation.accepted &&
        p04CinematicsOff.mutation.changed && p04DialogueOff.mutation.changed &&
        !p04.settingGameplayEnabled.load() && !p04.settingCinematicsEnabled.load() &&
        !p04.dialogueEnabled.load() && p04.settingGameplayMode.load() ==
            config::GameplayMode::AspectRecalculation,
        "P04 all module settings and mode are OFF before lifecycle observations");
    const auto p04Enter = p04.Enter(p04.settingCinematicsEnabled.load(), 0,
        90.6557f, Baseline(), 3.55556f);
    pass &= Check(p04Enter.accepted && !p04Enter.transformed &&
        p04.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "P04 ENTER lifecycle remains observable with all correction modules OFF");
    auto p04AspectInput = AspectInput(0xE);
    p04AspectInput.runtimeEnabled = false;
    const auto p04Aspect = plugin::ObserveCinematicAspectStore(p04.View(),
        p04AspectInput);
    pass &= Check(p04Aspect.accepted,
        "P04 aspect-store observation remains available with all modules OFF");
    const auto p04Writer = p04.Writer(p04Gate,
        {0xE, 0xEE, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(p04Writer.processed && !p04Writer.mutateFov &&
        p04Writer.outputFov == 90.6557f &&
        p04.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "P04 Writer event passes through without losing cinematic observation");
    const auto p04Dialogue = p04.DialogueEvent(p04Gate, 90.0f, 0xD);
    pass &= Check(p04Dialogue.action == plugin::DialogueBoundaryAction::NonGameplayReset &&
        !p04Dialogue.mutateXmm1,
        "P04 all correction modules OFF retains lifecycle observation and native output");

    // Rapid legitimate EXIT→ENTER and the accepted no-owner fail-closed path
    // are event sequences, not direct state setup. They deliberately do not
    // invent a discriminator for a hypothetical stale EXIT.
    Fixture rapidOwned;
    plugin::CameraCorrectionGate rapidOwnedGate;
    const auto rapidOwnedEnterA = rapidOwned.Enter(true, 1, 90.6557f,
        Baseline(), 3.55556f);
    pass &= Check(rapidOwnedEnterA.accepted &&
        rapidOwned.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "X04 first accepted ENTER establishes active generation A");
    const auto rapidOwnedAspectA = plugin::ObserveCinematicAspectStore(
        rapidOwned.View(), AspectInput(0xA));
    pass &= Check(rapidOwnedAspectA.accepted,
        "X04 aspect-store event prepares generation A association");
    const auto rapidOwnedWriterA = rapidOwned.Writer(rapidOwnedGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(rapidOwnedWriterA.mutateFov &&
        plugin::ReadCinematicLifecycleSnapshot(rapidOwned.View()).association
            .associatedWriterRsi == 0xA,
        "X04 writer A establishes associated cinematic output");
    const auto rapidOwnedExitPolicy = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto rapidOwnedExit = plugin::ProcessAcceptedCinematicExit(
        rapidOwned.View(), {rapidOwned.coordinator.load(),
            rapidOwnedExitPolicy.nextState, rapidOwnedEnterA.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA,
            rapidOwnedEnterA.outputFov});
    pass &= Check(rapidOwnedExit.accepted && rapidOwnedExit.recoveryOwnerCaptured &&
        rapidOwned.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "X04 accepted EXIT snapshots owner A before rapid next ENTER");
    const auto rapidOwnedEnterB = rapidOwned.Enter(true, 2, 90.6557f,
        Baseline(0xB), 3.55556f);
    pass &= Check(rapidOwnedEnterB.accepted &&
        rapidOwnedEnterB.generation > rapidOwnedEnterA.generation &&
        rapidOwned.coordinator.load() == camera::CoordinatorState::CinematicActive &&
        rapidOwned.selectionValid.load() &&
        !rapidOwned.recoveryOwner.Read().HasRecoveryAuthority(),
        "X04 new ENTER B allocates a fresh active generation and clears owner A");
    const auto rapidOwnedAspectB = plugin::ObserveCinematicAspectStore(
        rapidOwned.View(), AspectInput(0xB));
    pass &= Check(rapidOwnedAspectB.accepted &&
        rapidOwned.association.Read().aspectStoreTargetObject == 0xB,
        "X04 aspect-store observation belongs to the newly accepted generation B");
    const auto rapidOwnedWriterB = rapidOwned.Writer(rapidOwnedGate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true});
    pass &= Check(rapidOwnedWriterB.cinematicHandled &&
        rapidOwnedWriterB.mutateFov,
        "X04 immediate accepted EXIT→ENTER clears prior recovery authority and starts the next generation");

    Fixture rapidNoOwner;
    plugin::CameraCorrectionGate rapidNoOwnerGate;
    const auto rapidNoOwnerEnterA = rapidNoOwner.Enter(true, 1, 90.6557f,
        Baseline(), 3.55556f);
    pass &= Check(rapidNoOwnerEnterA.accepted &&
        rapidNoOwner.coordinator.load() == camera::CoordinatorState::CinematicActive,
        "X05 no-owner path begins from accepted generation A");
    const auto rapidNoOwnerExitPolicy = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    const auto rapidNoOwnerExit = plugin::ProcessAcceptedCinematicExit(
        rapidNoOwner.View(), {rapidNoOwner.coordinator.load(),
            rapidNoOwnerExitPolicy.nextState, rapidNoOwnerEnterA.generation,
            90.6557f, true, config::GameplayMode::HorPlus, 0xA, 127.393f});
    pass &= Check(rapidNoOwnerExit.accepted &&
        !rapidNoOwnerExit.recoveryOwnerCaptured &&
        !rapidNoOwner.selectionValid.load() &&
        !rapidNoOwner.recoveryOwner.Read().HasRecoveryAuthority(),
        "X05 accepted EXIT without association remains fail-closed before next ENTER");
    const auto rapidNoOwnerEnterB = rapidNoOwner.Enter(true, 2, 90.6557f,
        Baseline(0xB), 3.55556f);
    const auto rapidNoOwnerSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        rapidNoOwner.View());
    pass &= Check(rapidNoOwnerEnterB.accepted &&
        rapidNoOwnerEnterB.generation > rapidNoOwnerEnterA.generation &&
        rapidNoOwnerSnapshot.coordinator == camera::CoordinatorState::CinematicActive &&
        rapidNoOwnerSnapshot.selectionValid &&
        !rapidNoOwnerSnapshot.recoveryOwner.HasRecoveryAuthority() &&
        !rapidNoOwnerSnapshot.exclusionActive,
        "X05 accepted no-owner EXIT→ENTER remains fail-closed and starts a fresh generation");

    // RUNTIME-CAPTURED GENERATION CHAIN (2026-10-04): gen3 really inherits
    // gen2's validated continuation, but has no writer invocation before its
    // no-owner EXIT. Characterize the current state erasure and resulting
    // gen4 121.060 pass-through; do not encode a desired policy here.
    Fixture generationChain;
    plugin::CameraCorrectionGate generationChainGate;
    const auto enterWithReentry = [&](std::uintptr_t previousSource,
        float previousNativeTarget, float previousTransformedTarget,
        bool awaitingValidatedRecovery) {
        const auto candidate = plugin::PrepareCinematicEnter(
            generationChain.View(), config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus);
        return plugin::ProcessAcceptedCinematicEnter(generationChain.View(),
            {candidate, true, 1, 90.0f, Baseline(), 3.55556f,
                16.0f / 9.0f, previousSource, previousNativeTarget,
                previousTransformedTarget, awaitingValidatedRecovery});
    };
    const auto exitGeneration = [&](std::uint64_t generation,
        std::uintptr_t previousSource, float nativeExitFov,
        float transformedTarget, bool recoveryObserverAvailable) {
        const auto transition = gameplay::ResolveCinematicExitTransition(
            recoveryObserverAvailable, true, config::GameplayMode::HorPlus);
        return plugin::ProcessAcceptedCinematicExit(generationChain.View(),
            {generationChain.coordinator.load(std::memory_order_acquire),
                transition.nextState, generation, nativeExitFov, true,
                config::GameplayMode::HorPlus, previousSource, transformedTarget});
    };

    const auto chainEnter1 = enterWithReentry(0, NAN, NAN, false);
    const auto chainAspect1 = plugin::ObserveCinematicAspectStore(
        generationChain.View(), AspectInput(0xA));
    const auto chainWriter1 = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x05, true});
    pass &= Check(chainEnter1.accepted && chainEnter1.generation == 1 &&
        chainAspect1.accepted && chainWriter1.cinematicHandled &&
        chainWriter1.cinematicAction ==
            cinematics::CinematicWriterAction::RetainTransformedTarget &&
        chainWriter1.mutateFov && std::fabs(chainWriter1.outputFov - 127.393f) < 0.01f,
        "GC01 generation 1 establishes the 127.393 target through production events");
    const auto chainExit1 = exitGeneration(chainEnter1.generation, 0xA,
        90.6557f, 127.393f, true);
    pass &= Check(chainExit1.accepted && chainExit1.recoveryOwnerCaptured &&
        generationChain.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "GC02 generation 1 EXIT captures its associated owner before reentry");

    const auto chainEnter2 = enterWithReentry(0xA, 90.6557f, 127.393f, true);
    const auto chainAspect2 = plugin::ObserveCinematicAspectStore(
        generationChain.View(), AspectInput(0xA));
    const auto chainWriter2Baseline = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x05, true});
    const auto chainWriter2NearBaseline = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 90.6712f, 3.55556f, true, 0x05, true});
    pass &= Check(chainEnter2.accepted && chainEnter2.continuedOwnership &&
        chainEnter2.generation == 2 && chainAspect2.accepted &&
        chainWriter2Baseline.cinematicHandled && chainWriter2Baseline.mutateFov &&
        std::fabs(chainWriter2Baseline.outputFov - 127.393f) < 0.01f &&
        chainWriter2NearBaseline.cinematicAction ==
            cinematics::CinematicWriterAction::RetainTransformedTarget &&
        std::fabs(chainWriter2NearBaseline.outputFov - 127.393f) < 0.01f,
        "GC03 generation 2 reentry associates A and retains baseline-equivalent 90.6557/90.6712 samples");
    const auto chainWriter2Departure = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 91.109f, 3.55556f, true, 0x05, true});
    const auto chainWriter2MidRamp = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 120.42f, 3.55556f, true, 0x05, true});
    const auto chain2Snapshot = plugin::ReadCinematicLifecycleSnapshot(
        generationChain.View());
    pass &= Check(chainWriter2Departure.cinematicAction ==
            cinematics::CinematicWriterAction::RetainContinuationTarget &&
        chainWriter2Departure.mutateFov &&
        std::fabs(chainWriter2Departure.outputFov - 127.393f) < 0.01f &&
        chainWriter2MidRamp.cinematicAction ==
            cinematics::CinematicWriterAction::RetainContinuationTarget &&
        chainWriter2MidRamp.mutateFov &&
        std::fabs(chainWriter2MidRamp.outputFov - 127.393f) < 0.01f &&
        chain2Snapshot.continuationActive &&
        chain2Snapshot.association.initialEstablishmentEnded,
        "GC04 generation 2 ends Initial Establishment at 91.109 then continuation protects raw 120.42");
    const auto chainExit2 = exitGeneration(chainEnter2.generation, 0xA,
        90.6557f, 127.393f, true);
    pass &= Check(chainExit2.accepted && chainExit2.recoveryOwnerCaptured &&
        generationChain.coordinator.load() == camera::CoordinatorState::CinematicExiting,
        "GC05 generation 2 EXIT captures owner A after the protected native ramp");

    const auto chainEnter3 = enterWithReentry(0xA, 90.6557f, 127.393f, true);
    const auto chainAspect3 = plugin::ObserveCinematicAspectStore(
        generationChain.View(), AspectInput(0xA));
    const auto chain3BeforeExit = plugin::ReadCinematicLifecycleSnapshot(
        generationChain.View());
    pass &= Check(chainEnter3.accepted && chainEnter3.continuedOwnership &&
        chainEnter3.generation == 3 &&
        chainAspect3.accepted &&
        chain3BeforeExit.coordinator == camera::CoordinatorState::CinematicActive &&
        chain3BeforeExit.selectionValid &&
        chain3BeforeExit.continuationActive &&
        chain3BeforeExit.continuationSource == 0xA &&
        std::fabs(chain3BeforeExit.continuationStartFov - 90.6557f) < 0.001f &&
        !chain3BeforeExit.association.writerObserved &&
        !chain3BeforeExit.association.associationValid &&
        chain3BeforeExit.association.aspectStoreTargetObject == 0xA &&
        chain3BeforeExit.association.associatedWriterRsi == 0,
        "GC06 generation 3 inherits validated continuation A and observes AspectStore(A)");
    const auto chainExit3 = exitGeneration(chainEnter3.generation, 0xA,
        90.6557f, 127.393f, true);
    const auto chain3AfterExit = plugin::ReadCinematicLifecycleSnapshot(
        generationChain.View());
    pass &= Check(chainExit3.accepted && !chainExit3.recoveryOwnerCaptured &&
        !chain3AfterExit.selectionValid && !chain3AfterExit.recoveryOwner.valid &&
        !chain3AfterExit.continuationPending &&
        chain3AfterExit.continuationCarryPending &&
        chain3AfterExit.continuationActive &&
        chain3AfterExit.continuationSource == 0xA &&
        std::fabs(chain3AfterExit.continuationStartFov - 90.6557f) < 0.001f &&
        std::fabs(chain3AfterExit.continuationExitNativeTarget - 90.6557f) < 0.001f &&
        std::fabs(chain3AfterExit.continuationPreviousTarget - 127.393f) < 0.01f &&
        chain3AfterExit.coordinator == camera::CoordinatorState::CinematicExiting,
        "GC07 writer-empty generation 3 preserves predecessor A/90.6557/127.393 without recovery owner");

    const auto chainEnter4 = enterWithReentry(
        chain3AfterExit.continuationSource,
        chain3AfterExit.continuationExitNativeTarget,
        chain3AfterExit.continuationPreviousTarget, false);
    const auto chainAspect4 = plugin::ObserveCinematicAspectStore(
        generationChain.View(), AspectInput(0xA));
    const auto chain4BeforeWriter = plugin::ReadCinematicLifecycleSnapshot(
        generationChain.View());
    pass &= Check(chainEnter4.accepted && chainEnter4.continuedOwnership &&
        chainEnter4.generation == 4 &&
        std::fabs(chainEnter4.outputFov - 127.393f) < 0.01f &&
        !plugin::ReadCinematicLifecycleSnapshot(generationChain.View()).continuationCarryPending &&
        chainAspect4.accepted &&
        chain4BeforeWriter.coordinator == camera::CoordinatorState::CinematicActive &&
        chain4BeforeWriter.association.aspectStoreTargetObject == 0xA &&
        !chain4BeforeWriter.association.associationValid,
        "GC08 generation 4 re-establishes the same target and awaits its associated writer");
    const auto chainWriter4First = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 121.060f, 3.55556f, true, 0x05, true});
    const auto chain4AfterWriter = plugin::ReadCinematicLifecycleSnapshot(
        generationChain.View());
    pass &= Check(chainWriter4First.processed && chainWriter4First.cinematicHandled &&
        chainWriter4First.associationAfterDecision.associationValid &&
        chainWriter4First.associationAfterDecision.associatedWriterRsi == 0xA &&
        chainWriter4First.associationAfterDecision.initialEstablishmentEnded &&
        chainWriter4First.associationAfterDecision.initialEstablishmentEndReason ==
            cinematics::InitialEstablishmentEndReason::AuthoredBaselineDeparture &&
        chainWriter4First.associationAfterDecision.writerObserved &&
        chainWriter4First.continuationSourceMatch &&
        chainWriter4First.cinematicAction ==
            cinematics::CinematicWriterAction::RetainContinuationTarget &&
        chainWriter4First.mutateFov &&
        std::fabs(chainWriter4First.outputFov - 127.393f) < 0.01f &&
        chain4AfterWriter.generation == 4 &&
        chain4AfterWriter.coordinator == camera::CoordinatorState::CinematicActive,
        "GC09 first generation 4 writer ends establishment on departure but predecessor continuation retains 121.060 to target");
    const auto chainWriter4Ramp = generationChain.Writer(generationChainGate,
        {0xA, 0xAA, 121.624f, 3.55556f, true, 0x05, true});
    pass &= Check(chainWriter4Ramp.mutateFov &&
        chainWriter4Ramp.cinematicAction ==
            cinematics::CinematicWriterAction::RetainContinuationTarget &&
        std::fabs(chainWriter4Ramp.outputFov - 127.393f) < 0.01f,
        "GC10 subsequent generation 4 ramp sample remains protected by the preserved continuation");

    // Negative control: when the intermediate continued generation does have
    // an associated writer event, EXIT follows the ordinary owner-capture path;
    // it is not the writer-empty/no-owner case characterized above.
    Fixture generationChainWriterControl;
    plugin::CameraCorrectionGate generationChainWriterControlGate;
    const auto controlEnter = [&](std::uintptr_t previousSource,
        float previousNativeTarget, float previousTransformedTarget,
        bool awaitingValidatedRecovery) {
        const auto candidate = plugin::PrepareCinematicEnter(
            generationChainWriterControl.View(), config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus);
        return plugin::ProcessAcceptedCinematicEnter(
            generationChainWriterControl.View(),
            {candidate, true, 1, 90.0f, Baseline(), 3.55556f,
                16.0f / 9.0f, previousSource, previousNativeTarget,
                previousTransformedTarget, awaitingValidatedRecovery});
    };
    const auto controlExit = [&](std::uint64_t generation) {
        const auto transition = gameplay::ResolveCinematicExitTransition(
            true, true, config::GameplayMode::HorPlus);
        return plugin::ProcessAcceptedCinematicExit(
            generationChainWriterControl.View(),
            {generationChainWriterControl.coordinator.load(std::memory_order_acquire),
                transition.nextState, generation, 90.6557f, true,
                config::GameplayMode::HorPlus, 0xA, 127.393f});
    };
    const auto controlEnter1 = controlEnter(0, NAN, NAN, false);
    plugin::ObserveCinematicAspectStore(generationChainWriterControl.View(), AspectInput(0xA));
    const auto controlWriter1 = generationChainWriterControl.Writer(
        generationChainWriterControlGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x05, true});
    const auto controlExit1 = controlExit(controlEnter1.generation);
    const auto controlEnter2 = controlEnter(0xA, 90.6557f, 127.393f, true);
    plugin::ObserveCinematicAspectStore(generationChainWriterControl.View(), AspectInput(0xA));
    generationChainWriterControl.Writer(generationChainWriterControlGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x05, true});
    const auto controlExit2 = controlExit(controlEnter2.generation);
    const auto controlEnter3 = controlEnter(0xA, 90.6557f, 127.393f, true);
    plugin::ObserveCinematicAspectStore(generationChainWriterControl.View(), AspectInput(0xA));
    const auto controlWriter3 = generationChainWriterControl.Writer(
        generationChainWriterControlGate,
        {0xA, 0xAA, 90.6712f, 3.55556f, true, 0x05, true});
    const auto control3AfterWriter = plugin::ReadCinematicLifecycleSnapshot(
        generationChainWriterControl.View());
    const auto controlExit3 = controlExit(controlEnter3.generation);
    pass &= Check(controlEnter1.accepted && controlWriter1.mutateFov &&
        controlExit1.recoveryOwnerCaptured && controlEnter2.continuedOwnership &&
        controlExit2.recoveryOwnerCaptured && controlEnter3.continuedOwnership &&
        controlWriter3.associationAfterDecision.associationValid &&
        controlWriter3.associationAfterDecision.writerObserved &&
        controlWriter3.mutateFov && control3AfterWriter.association.associationValid &&
        control3AfterWriter.association.writerObserved &&
        controlExit3.accepted && controlExit3.recoveryOwnerCaptured &&
        !controlExit3.preservedInheritedContinuation &&
        generationChainWriterControl.recoveryOwner.Read().HasRecoveryAuthority(),
        "GC11 writer-positive continued generation uses its associated-writer owner path");

    // Z01 replays one identical event-only production transaction sequence
    // with Diagnostic Mode disabled/enabled. Diagnostics are emitted through
    // their normal gate, while the read-only semantic digest includes output,
    // lifecycle, ownership, exclusion and fake engine-read outcomes after
    // every setting/lifecycle/writer/Dialogue event.
    const auto runDiagnosticScenario = [](bool diagnosticEnabled) {
        diagnostics::SetEnabled(diagnosticEnabled);
        DiagnosticScenarioDigest digest{};
        Fixture scenario;
        plugin::CameraCorrectionGate scenarioGate;
        FreshFieldsFixture fresh{
            {3.55556f, 0x04, true, true}, 0xA, 0, false};
        const plugin::CameraWriterEngineOperations freshReader{
            &fresh, &ReadFreshFields};
        const auto emitMarker = [&] {
            diagnostics::EmitIfEnabled([&] { ++digest.emittedMarkers; });
        };
        const auto record = [&](float output, bool mutate, std::uint32_t action) {
            const auto snapshot = plugin::ReadCinematicLifecycleSnapshot(
                scenario.View());
            auto& sample = digest.samples[digest.count++];
            sample.outputFov = output;
            sample.mutate = mutate;
            sample.action = action;
            sample.generation = snapshot.generation;
            sample.coordinator = snapshot.coordinator;
            sample.selectionValid = snapshot.selectionValid;
            sample.associationValid = snapshot.association.associationValid;
            sample.associationConflict = snapshot.association.associationConflict;
            sample.associatedRsi = snapshot.association.associatedWriterRsi;
            sample.initialEstablishmentActive =
                snapshot.association.initialEstablishmentActive;
            sample.initialEstablishmentEnded =
                snapshot.association.initialEstablishmentEnded;
            sample.recoveryOwnerValid = snapshot.recoveryOwner.HasRecoveryAuthority();
            sample.recoveryComplete = snapshot.recoveryOwner.recoveryComplete;
            sample.recoveryOwnerRsi = snapshot.recoveryOwner.writerRsi;
            sample.exclusionActive = snapshot.exclusionActive;
            sample.continuationPending = snapshot.continuationPending;
            sample.continuationActive = snapshot.continuationActive;
            sample.dialoguePhase = scenario.dialoguePhase;
            sample.freshReadCalls = fresh.calls;
            sample.freshReadSource = fresh.lastReadSource;
            sample.freshReadIncludedFlags = fresh.lastReadIncludedFlags;
            sample.freshReadSourceMatched = fresh.sourceMatched;
        };

        const auto setting = plugin::ApplyCameraSetting(scenarioGate,
            scenario.SettingsView(), plugin::RuntimeSettingMutation::Dialogue(
                config::DialogueZoomPolicy::Adaptive), true, [] {});
        emitMarker();
        record(NAN, setting.mutation.changed, setting.mutation.accepted ? 1u : 0u);

        const auto enterResult = scenario.Enter(true, 1, 90.6557f,
            Baseline(), 3.55556f);
        emitMarker();
        record(enterResult.outputFov, enterResult.transformed,
            enterResult.accepted ? 1u : 0u);

        const auto aspectResult = plugin::ObserveCinematicAspectStore(
            scenario.View(), AspectInput(0xA));
        emitMarker();
        record(aspectResult.aspectToStore, aspectResult.accepted,
            aspectResult.accepted ? 1u : 0u);

        fresh.expectedSource = 0xA;
        const auto associatedWriter = scenario.Writer(scenarioGate,
            {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true}, freshReader);
        emitMarker();
        record(associatedWriter.outputFov, associatedWriter.mutateFov,
            static_cast<std::uint32_t>(associatedWriter.cinematicAction));

        const auto dialogueObservedInCinematic = scenario.DialogueEvent(
            scenarioGate, 90.0f, 0xD);
        emitMarker();
        record(dialogueObservedInCinematic.outputFov,
            dialogueObservedInCinematic.mutateXmm1,
            static_cast<std::uint32_t>(dialogueObservedInCinematic.action));

        const auto exitTransition = gameplay::ResolveCinematicExitTransition(
            true, true, config::GameplayMode::HorPlus);
        const auto exitResult = plugin::ProcessAcceptedCinematicExit(
            scenario.View(), {scenario.coordinator.load(), exitTransition.nextState,
                enterResult.generation, 90.6557f, true,
                config::GameplayMode::HorPlus, 0xA, enterResult.outputFov});
        emitMarker();
        record(NAN, exitResult.recoveryOwnerCaptured,
            exitResult.accepted ? 1u : 0u);

        fresh.expectedSource = 0xA;
        const auto recoveryResult = scenario.Writer(scenarioGate,
            {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true}, freshReader);
        emitMarker();
        record(recoveryResult.outputFov, recoveryResult.mutateFov,
            static_cast<std::uint32_t>(recoveryResult.horPlusRecovery.action));

        const auto dialogueSuppressedByExclusion = scenario.DialogueEvent(
            scenarioGate, 90.0f, 0xD);
        emitMarker();
        record(dialogueSuppressedByExclusion.outputFov,
            dialogueSuppressedByExclusion.mutateXmm1,
            static_cast<std::uint32_t>(dialogueSuppressedByExclusion.action));

        fresh.expectedSource = 0xB;
        const auto replacementWriter = scenario.Writer(scenarioGate,
            {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true}, freshReader);
        emitMarker();
        record(replacementWriter.outputFov, replacementWriter.mutateFov,
            replacementWriter.gameplay.handled ? 1u : 0u);

        const auto dialogueCandidateAfterCinematic = scenario.DialogueEvent(
            scenarioGate, 90.0f, 0xD);
        emitMarker();
        record(dialogueCandidateAfterCinematic.outputFov,
            dialogueCandidateAfterCinematic.mutateXmm1,
            static_cast<std::uint32_t>(dialogueCandidateAfterCinematic.action));

        const auto dialogueActiveAfterCinematic = scenario.DialogueEvent(
            scenarioGate, 89.8f, 0xD);
        emitMarker();
        record(dialogueActiveAfterCinematic.outputFov,
            dialogueActiveAfterCinematic.mutateXmm1,
            static_cast<std::uint32_t>(dialogueActiveAfterCinematic.action));
        digest.freshReadCalls = fresh.calls;
        return digest;
    };
    const auto diagnosticOffScenario = runDiagnosticScenario(false);
    const auto diagnosticOnScenario = runDiagnosticScenario(true);
    diagnostics::SetEnabled(false);
    pass &= Check(diagnosticOffScenario.count == 11 &&
        diagnosticOnScenario.count == 11 &&
        diagnosticOffScenario.emittedMarkers == 0 &&
        diagnosticOnScenario.emittedMarkers == 11 &&
        SameDiagnosticScenario(diagnosticOffScenario, diagnosticOnScenario),
        "Z01 Diagnostic OFF/ON preserves every production event output/state/effect while only gated markers differ");

    Fixture fallbackState;
    auto invalidBaseline = Baseline();
    invalidBaseline.valid = false;
    const auto fallbackEnter = fallbackState.Enter(true, 2, 90.6557f,
        invalidBaseline, 3.55556f);
    pass &= Check(fallbackEnter.accepted &&
        fallbackEnter.baselineSelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::AuthoredEnter &&
        !fallbackEnter.baselineSelection.usedGameplayBaselineValue &&
        std::fabs(fallbackEnter.baselineSelection.targetBaselineFov - 90.6557f) < 0.001f,
        "unusable gameplay baseline selects authored ENTER numeric fallback");

    Fixture wrongRsiState;
    plugin::CameraCorrectionGate wrongRsiGate;
    const auto wrongRsiEnter = wrongRsiState.Enter(true, 2, 90.6557f,
        Baseline(), 3.55556f);
    const auto wrongRsiAspect = plugin::ObserveCinematicAspectStore(
        wrongRsiState.View(), AspectInput(0xA));
    const auto wrongRsiWriter = wrongRsiState.Writer(wrongRsiGate,
        {0xB, 0xBB, 90.6557f, 3.55556f, true, 0x04, true});
    const auto wrongRsiSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        wrongRsiState.View());
    pass &= Check(wrongRsiEnter.accepted && wrongRsiAspect.accepted &&
        !wrongRsiSnapshot.association.associationValid &&
        !wrongRsiWriter.initialEstablishmentAuthorized && !wrongRsiWriter.mutateFov &&
        wrongRsiWriter.outputFov == 90.6557f,
        "same authored FOV on the wrong RSI cannot establish or authorize association");

    Fixture conflictState;
    plugin::CameraCorrectionGate conflictGate;
    const auto conflictEnter = conflictState.Enter(true, 2, 90.6557f,
        Baseline(), 3.55556f);
    const auto firstConflictTarget = plugin::ObserveCinematicAspectStore(
        conflictState.View(), AspectInput(0xA));
    auto conflictingTargetInput = AspectInput(0xC);
    const auto secondConflictTarget = plugin::ObserveCinematicAspectStore(
        conflictState.View(), conflictingTargetInput);
    const auto conflictWriter = conflictState.Writer(conflictGate,
        {0xA, 0xAA, 90.6557f, 3.55556f, true, 0x04, true});
    const auto conflictSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        conflictState.View());
    pass &= Check(conflictEnter.accepted && firstConflictTarget.accepted &&
        secondConflictTarget.association.associationConflict &&
        !conflictSnapshot.association.associationValid &&
        !conflictWriter.initialEstablishmentAuthorized && !conflictWriter.mutateFov,
        "conflicting aspect-store targets fail closed through the production writer event");

    Fixture disabledState;
    plugin::CameraCorrectionGate disabledGate;
    const auto disabledEnter = disabledState.Enter(false, 0, 90.6557f,
        Baseline(), 16.0f / 9.0f);
    auto disabledStoreInput = AspectInput(0xA);
    disabledStoreInput.runtimeEnabled = false;
    disabledStoreInput.activeCorrectionEnabled = false;
    const auto disabledStore = plugin::ObserveCinematicAspectStore(
        disabledState.View(), disabledStoreInput);
    const auto disabledWriter = disabledState.Writer(disabledGate,
        {0xA, 0xAA, 90.6557f, 16.0f / 9.0f, true, 0x04, true});
    const auto disabledSnapshot = plugin::ReadCinematicLifecycleSnapshot(
        disabledState.View());
    pass &= Check(disabledEnter.accepted && !disabledEnter.transformed &&
        disabledStore.accepted && !disabledStore.correctionPolicyActive &&
        !disabledWriter.mutateFov && disabledWriter.outputFov == 90.6557f &&
        disabledSnapshot.coordinator == camera::CoordinatorState::CinematicActive &&
        disabledSnapshot.association.associationValid,
        "Cinematics OFF keeps lifecycle observation/association while writer correction stays inert");

    std::cout << (pass ? "PASS" : "FAIL")
              << ": cinematic lifecycle transaction\n";
    return pass ? 0 : 1;
}
