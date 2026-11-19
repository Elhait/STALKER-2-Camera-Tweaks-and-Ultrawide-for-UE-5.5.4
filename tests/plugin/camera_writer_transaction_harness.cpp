#include "../../src/plugin/camera_writer_transaction.hpp"
#include "../../src/plugin/lifecycle_authority_core.hpp"
#include "../../src/plugin/lifecycle_authority_policy.hpp"
#include "../../src/plugin/lifecycle_authority_writer_port.hpp"
#include "../../src/plugin/lifecycle_authority_cinematic_port.hpp"
#include "../../src/plugin/lifecycle_authority_dialogue_port.hpp"
#include "../../src/plugin/dialogue_boundary_transaction.hpp"
#include "../../src/diagnostics/diagnostic_runtime.hpp"
#include "../../src/overlay/presenter/presenter_runtime_policy.hpp"
#include "../../src/plugin/cinematic_lifecycle_transaction.hpp"
#include "../../src/camera/camera_state_publication.hpp"
#include "../../src/camera/horplus.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"
#include "../../src/gameplay/gameplay_state.hpp"
#include "../../src/overlay/presentation/camera_integration.hpp"

#include <array>
#include <future>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <string_view>

namespace
{

    bool m15Verification{};
    bool m15FirstWriterChecked{};
    bool m15BindingChecksPass{true};

    void CheckM15FirstProductionWriter(const plugin::CameraWriterInput& input,
        const plugin::CameraWriterResult& production)
    {
        if (!m15Verification || m15FirstWriterChecked) return;
        m15FirstWriterChecked = true;
        namespace la = plugin::lifecycle_authority;
        la::Authority authority;
        la::EventFacts init{};
        init.kind = la::EventKind::Initialize;
        init.evidence.callbackBoundaryValidated = true;
        init.initialGlobalEnabled = init.initialGameplayEnabled = true;
        authority.Accept(init);
        // Explicit fixture premise: this first source is an original Gameplay
        // sample. Do not obtain that premise from the result's owner/context.
        la::EventFacts sample{};
        sample.kind = la::EventKind::GameplaySample;
        sample.scope = 1;
        sample.receiver = input.sourceIdentity;
        sample.evidence.callbackBoundaryValidated = true;
        sample.evidence.sourceRelationValidated = true;
        sample.requestPolicyAdmission = true;
        sample.fov = input.incomingFov;
        sample.fovProvenance = la::FovProvenance::OriginalNative;
        sample.fovBoundary = la::FovBoundary::GameplayWriterInput;
        const auto admitted = authority.Accept(sample);
        auto effect = sample;
        effect.kind = la::EventKind::PrepareEffect;
        effect.transactionId = input.writerTransactionId;
        effect.fov = production.outputFov;
        effect.fovProvenance = la::FovProvenance::ModCorrectedOutput;
        effect.fovBoundary = la::FovBoundary::WriterDecision;
        const auto currentBinding = authority.Accept(effect);
        m15BindingChecksPass &= admitted.accepted && production.mutateFov &&
            input.writerTransactionId == 0 && !currentBinding.accepted;
        std::cout << "M15_DIVERGENCE diagnostics_off production_corrects="
            << production.mutateFov << " transaction_id=" << input.writerTransactionId
            << " authority_prepare=" << currentBinding.accepted << '\n';
        // A proposed adapter-owned ID resolves just this mismatch; it is not
        // derived from timestamp, diagnostics or the Authority decision.
        effect.transactionId = 1;
        const auto proposedBinding = authority.Accept(effect);
        m15BindingChecksPass &= proposedBinding.accepted;
        std::cout << "M15_PROPOSAL adapter_owned_id_prepare=" << proposedBinding.accepted << '\n';
        // Existing API makes admission and preparation separate visible commits.
        m15BindingChecksPass &= proposedBinding.revision > admitted.revision;
        std::cout << "M15_DIVERGENCE single_writer_admission_revision=" << admitted.revision
            << " prepare_revision=" << proposedBinding.revision << '\n';
    }

    bool CheckM15ObservationWindows()
    {
        namespace la = plugin::lifecycle_authority;
        bool pass = true;
        for (bool diagnosticsOn : {false, true}) {
            diagnostics::SetEnabled(diagnosticsOn);
            la::Authority authority;
            la::EventFacts init{};
            init.kind = la::EventKind::Initialize;
            init.evidence.callbackBoundaryValidated = true;
            init.initialGlobalEnabled = init.initialGameplayEnabled = true;
            authority.Accept(init);
            la::EventFacts sample{};
            sample.kind = la::EventKind::GameplaySample;
            sample.scope = 1;
            sample.receiver = 101;
            sample.evidence.callbackBoundaryValidated = true;
            sample.evidence.sourceRelationValidated = true;
            sample.requestPolicyAdmission = true;
            sample.fov = 130.311;
            sample.fovBoundary = la::FovBoundary::GameplayWriterInput;
            sample.fovProvenance = la::FovProvenance::OriginalNative;
            authority.Accept(sample);
            unsigned closed{};
            for (unsigned i = 1; i <= 512; ++i) {
                auto prepare = sample;
                prepare.kind = la::EventKind::PrepareEffect;
                prepare.transactionId = i;
                prepare.fov = 153.931;
                prepare.fovBoundary = la::FovBoundary::WriterDecision;
                prepare.fovProvenance = la::FovProvenance::ModCorrectedOutput;
                const auto intent = authority.Accept(prepare);
                if (!intent.accepted) { pass = false; break; }
                // Models a future owner of an entire observation window, not
                // the lifetime of the PRE writer callback. No native execution.
                struct Window {
                    la::Authority& authority;
                    la::EventFacts close;
                    unsigned& closed;
                    ~Window() { if (authority.Accept(close).accepted) ++closed; }
                };
                auto close = sample;
                close.kind = la::EventKind::EffectClosed;
                close.effectId = intent.effectId;
                close.transactionId = i;
                close.dependencyRevision = intent.revision;
                const auto exercise = [&]() {
                    Window window{authority, close, closed};
                    if (i % 4 == 0) return; // early return / refused execution
                    if (i % 4 == 1) throw i; // aborted adapter execution
                    if (diagnosticsOn && i % 4 == 2) {
                        auto observed = close;
                        observed.kind = la::EventKind::EffectObserved;
                        observed.evidence.effectBoundaryCorrelated = true;
                        observed.fovBoundary = la::FovBoundary::PostStore;
                        observed.fovProvenance = la::FovProvenance::EffectiveReadback;
                        observed.effectResult = la::EffectResult::Partial;
                        if (!authority.Accept(observed).accepted) pass = false;
                    }
                };
                try { exercise(); } catch (unsigned) {}
            }
            pass &= closed == 512 && authority.Read().effects.empty();
            std::cout << "M15_WINDOW_FIXTURE diagnostics=" << diagnosticsOn
                << " closed=" << closed << " pending=" << authority.Read().effects.size()
                << " live_binding=unverified\n";
            unsigned pendingAccepted{};
            for (unsigned i = 0; i <= la::Authority::MaxEffects; ++i) {
                auto prepare = sample;
                prepare.kind = la::EventKind::PrepareEffect;
                prepare.transactionId = 2000 + i;
                prepare.fov = 153.931;
                prepare.fovBoundary = la::FovBoundary::WriterDecision;
                prepare.fovProvenance = la::FovProvenance::ModCorrectedOutput;
                if (authority.Accept(prepare).accepted) ++pendingAccepted;
            }
            pass &= pendingAccepted == la::Authority::MaxEffects;
            std::cout << "M15_DIVERGENCE no_terminal diagnostics=" << diagnosticsOn
                << " accepted=" << pendingAccepted << " next_correction=refused\n";

        }
        diagnostics::SetEnabled(false);
        return pass;
    }

    struct CameraStateFreshnessSink
    {
        std::mutex* mutex{};
        camera::CameraStateSnapshot* snapshot{};
        bool* valid{};

        static void OnSettingChanged(void* context,
            plugin::CameraSettingFreshnessChange change)
        {
            auto& sink = *static_cast<CameraStateFreshnessSink*>(context);
            std::lock_guard lock(*sink.mutex);
            plugin::MarkCameraSettingFactsHistorical(*sink.snapshot,
                *sink.valid, change);
        }
    };

    void SelectMode(void* context, config::GameplayMode mode)
    {
        static_cast<std::atomic<config::GameplayMode>*>(context)->store(
            mode, std::memory_order_release);
    }

    bool Check(bool value, const char* message)
    {
        if (!value) std::cerr << "FAIL: " << message << '\n';
        return value;
    }

    struct SettingsFixture
    {
        std::atomic<config::GameplayMode> gameplayMode{config::GameplayMode::HorPlus};
        std::atomic<bool> gameplayEnabled{true};
        std::atomic<std::uint64_t> gameplayEpoch{0};
        std::atomic<bool> gameplayEnablePending{};
        std::atomic<bool> gameplayDisablePending{};
        std::atomic<bool> gameplayTransitionPending{};
        std::atomic<bool> gameplayTransitionDeferralLogged{};
        std::atomic<bool> cinematicsEnabled{true};
        std::atomic<std::uint64_t> cinematicsEpoch{0};
        std::atomic<bool> activeCinematicsEnabled{};
        std::atomic<std::uint64_t> activeCinematicsEpoch{};
        std::atomic<bool> cinematicFovApplied{};
        std::atomic<float> cinematicTarget{NAN};
        std::atomic<float> exitTarget{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<bool> dialogueEnabled{true};
        std::atomic<config::DialogueZoomPolicy> dialoguePolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<std::uint64_t> dialogueEpoch{0};
        std::atomic<std::uint64_t> activeDialogueEpoch{0};
        bool dialogueNonNativeCapabilityAvailable{true};
        std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> cinematicFovMode{
            config::CinematicFovMode::NativeHorPlus};
        dialogue::CorrectionActivation dialogueActivation;
        void* cameraStateFreshnessContext{};
        void (*cameraSettingChanged)(void*, plugin::CameraSettingFreshnessChange){};

        plugin::CameraSettingStateView View()
        {
            return {gameplayMode, gameplayEnabled, gameplayEpoch, gameplayEnablePending,
                gameplayDisablePending, gameplayTransitionPending,
                gameplayTransitionDeferralLogged, cinematicsEnabled,
                cinematicsEpoch, activeCinematicsEnabled, activeCinematicsEpoch,
                cinematicFovApplied, cinematicTarget, exitTarget,
                continuationPending, continuationCarryPending, continuationInterpolation,
                continuationSource, dialogueEnabled, dialoguePolicy, dialogueEpoch,
                activeDialogueEpoch, dialogueNonNativeCapabilityAvailable,
                cinematicAspectPolicy, cinematicFovMode,
                dialogueActivation, &gameplayMode, &SelectMode,
                cameraStateFreshnessContext, cameraSettingChanged};
        }
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

    struct ModeTransitionBarrier
    {
        std::mutex mutex;
        std::condition_variable condition;
        unsigned calls{};
        bool pause{};
        bool entered{};
        bool release{};
    };

    struct FreshFieldsFixture
    {
        std::uintptr_t expectedSource{};
        float aspect{3.55556f};
        std::uint8_t flags{0x04};
        bool readable{true};
        unsigned calls{};
        ModeTransitionBarrier* barrier{};
        bool useAspectSequence{};
        std::array<float, 4> aspectSequence{};
        bool aspectFlagsWriteSucceeds{true};
        unsigned aspectFlagsWriteCalls{};
    };

    plugin::CameraWriterFreshAspectFlags ReadFreshFields(
        void* context, std::uintptr_t source, bool includeFlags)
    {
        auto& fixture = *static_cast<FreshFieldsFixture*>(context);
        const auto callIndex = fixture.calls++;
        if (fixture.barrier && fixture.barrier->pause) {
            std::unique_lock lock(fixture.barrier->mutex);
            ++fixture.barrier->calls;
            if (!fixture.barrier->entered) {
                fixture.barrier->entered = true;
                fixture.barrier->condition.notify_all();
                fixture.barrier->condition.wait(lock, [&] {
                    return fixture.barrier->release;
                });
            }
        }
        plugin::CameraWriterFreshAspectFlags fields{};
        if (source != fixture.expectedSource || !fixture.readable) return fields;
        fields.aspect = fixture.useAspectSequence && callIndex < fixture.aspectSequence.size()
            ? fixture.aspectSequence[callIndex] : fixture.aspect;
        fields.flags = fixture.flags;
        fields.aspectReadable = true;
        fields.flagsReadable = includeFlags;
        return fields;
    }

    bool WriteFreshAspectAndFlags(void* context, std::uintptr_t, float, std::uint8_t)
    {
        auto& fixture = *static_cast<FreshFieldsFixture*>(context);
        ++fixture.aspectFlagsWriteCalls;
        return fixture.aspectFlagsWriteSucceeds;
    }

    struct EngineOperationFixture
    {
        std::array<unsigned, 3> operations{};
        unsigned count{};
        bool aspectOnlyResult{true};
        bool aspectFlagsResult{};
    };

    plugin::CameraWriterFreshAspectFlags RecordEngineRead(
        void* context, std::uintptr_t, bool includeFlags)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = includeFlags ? 1u : 2u;
        return {3.5f, 0x04, true, includeFlags};
    }

    bool RecordAspectOnlyWrite(void* context, std::uintptr_t, float)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = 3u;
        return fixture.aspectOnlyResult;
    }

    bool RecordAspectFlagsWrite(void* context, std::uintptr_t, float, std::uint8_t)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = 4u;
        return fixture.aspectFlagsResult;
    }

    struct RuntimeCombineMemory
    {
        std::uintptr_t source{0xC01};
        float aspect{16.0f / 9.0f};
        std::uint8_t flags{0x04};
        bool readable{true};
        bool writeSucceeds{true};
        bool writeActuallyApplies{true};
        unsigned reads{};
        unsigned aspectWrites{};
        unsigned aspectFlagsWrites{};
    };

    plugin::CameraWriterFreshAspectFlags ReadRuntimeCombineMemory(
        void* context, std::uintptr_t source, bool includeFlags)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.reads;
        if (source != memory.source || !memory.readable) return {};
        return {memory.aspect, memory.flags, true, includeFlags};
    }

    bool WriteRuntimeCombineAspect(void* context, std::uintptr_t source, float aspect)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.aspectWrites;
        if (source != memory.source || !memory.writeSucceeds) return false;
        if (memory.writeActuallyApplies) memory.aspect = aspect;
        return true;
    }

    bool WriteRuntimeCombineAspectAndFlags(void* context, std::uintptr_t source,
        float aspect, std::uint8_t flags)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.aspectFlagsWrites;
        if (source != memory.source || !memory.writeSucceeds) return false;
        memory.aspect = aspect;
        memory.flags = flags;
        return true;
    }

    bool CheckRuntimeCombineProductionTransaction()
    {
        using namespace camera;
        constexpr float native = 16.0f / 9.0f;
        constexpr float arbitraryTarget = 2.75f;
        RuntimeCombineMemory memory{};
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        plugin::CameraWriterRecoveryStateView recoveryState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget, continuationPrevious, continuationInterpolation,
            continuationStart, baselineSource, recoveryTarget, recoveryCinematicTarget};
        std::atomic<bool> cinematicCorrection{};
        std::atomic<bool> cinematicSelection{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<std::uint64_t> cinematicSettingsEpoch{};
        cinematics::CinematicEnterActivation enterActivation;
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> authoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraWriterCinematicStateView cinematicState{cinematicCorrection,
            cinematicSelection, cinematicGeneration, cinematicSettingsEpoch,
            cinematicSettingsEpoch,
            cinematicTargetEstablished,
            authoredFov, cinematicTargetFov, cinematicContinuationSource,
            cinematicContinuationStart, cinematicContinuationActive, association,
            enterActivation};
        plugin::CameraCorrectionGate gate;
        GameplayWriterFixture gameplayState;
        const plugin::CameraWriterEngineOperations engineOperations{&memory,
            &ReadRuntimeCombineMemory, &WriteRuntimeCombineAspect,
            &WriteRuntimeCombineAspectAndFlags};

        std::mutex dialogueMutex;
        dialogue::Phase dialoguePhase{dialogue::Phase::Inactive};
        bool dialoguePostExitProviderOwned{};
        std::uintptr_t dialoguePostExitProviderCallbackSource{};
        std::uintptr_t dialoguePostExitProviderReceiver{};
        std::uint64_t dialoguePostExitProviderGeneration{};
        std::uint64_t dialoguePostExitProviderSettingsEpoch{};
        std::uint32_t dialoguePostExitProviderThreadId{};
        dialogue::PolicySnapshot dialoguePolicy;
        dialogue::CorrectionActivation dialogueActivation;
        float dialogueBaseline{NAN}, dialoguePrevious{NAN}, dialogueExitStart{NAN};
        dialogue::CandidateTracker dialogueCandidate;
        dialogue::RecoveryRearm dialogueRecovery;
        std::atomic<bool> dialogueEnabled{true};
        std::atomic<config::DialogueZoomPolicy> selectedDialogue{config::DialogueZoomPolicy::Reduced};
        std::atomic<std::uint64_t> dialogueEpoch{1}, activeDialogueEpoch{};
        std::atomic<std::uintptr_t> dialogueSource{};
        std::atomic<float> dialogueTarget{NAN};
        std::atomic<bool> dialogueTargetValid{};
        plugin::DialogueBoundaryStateView dialogueView{dialogueMutex, coordinator, exclusion,
            dialoguePhase, dialoguePostExitProviderOwned,
            dialoguePostExitProviderCallbackSource, dialoguePostExitProviderReceiver,
            dialoguePostExitProviderGeneration, dialoguePostExitProviderSettingsEpoch,
            dialoguePostExitProviderThreadId,
            dialoguePolicy,
            dialogueActivation, dialogueBaseline, dialoguePrevious,
            dialogueExitStart, dialogueCandidate, dialogueRecovery, dialogueEnabled,
            selectedDialogue, dialogueEpoch, activeDialogueEpoch, dialogueSource,
            dialogueTarget, dialogueTargetValid};
        const auto run = [&](float target, bool targetValid, bool enabled = true,
            float inputFov = 90.6557f,
            camera::WriterContext context = camera::WriterContext::GameplayEligible,
            bool providerDialogue = false, float dialogueBaseline = NAN,
            float dialogueNativeTarget = NAN, float dialogueTargetFov = NAN,
            bool correctedDialogueSeed = false, std::uint32_t threadId = 0) {
            plugin::CameraWriterInput input{memory.source, 0xC02, inputFov,
                memory.aspect, true, memory.flags, true, 0.02f, enabled,
                config::GameplayMode::RuntimeCombine, native};
            input.writerContext = context;
            input.threadId = threadId;
            input.providerDialogueContinuation = providerDialogue;
            input.providerDialogueBaseline = dialogueBaseline;
            input.providerDialogueNativeTarget = dialogueNativeTarget;
            input.providerDialogueTarget = dialogueTargetFov;
            input.providerDialogueInputAlreadyCorrected = correctedDialogueSeed;
            input.dialogueCorrectionEnabled = providerDialogue;
            if (providerDialogue)
                input.dialoguePolicy = config::DialogueZoomPolicy::Reduced;
            input.currentViewportAspect = target;
            input.currentViewportAspectValid = targetValid;
            auto gameplayView = gameplayState.View();
            auto guard = gate.Acquire();
            const auto productionResult = plugin::ProcessCameraWriter(gate, recoveryState, input,
                cinematicState, engineOperations, {}, &gameplayView,
                {&dialogueView, +[](void* value, std::uintptr_t previousSource, float,
                    std::uintptr_t source, float) {
                    auto& view = *static_cast<plugin::DialogueBoundaryStateView*>(value);
                    if (!plugin::DialogueBoundaryIsActive(view)) return false;
                    if (previousSource && previousSource != source) return false;
                    if (!view.candidate.AffectsReceiver(source,
                            view.settingsEpoch.load(std::memory_order_acquire))) return false;
                    plugin::ResetDialogueBoundaryState(view);
                    return true;
                }});
            CheckM15FirstProductionWriter(input, productionResult);
            return productionResult;
        };

        bool pass = true;
        const auto corrected = run(arbitraryTarget, true);
        pass &= Check(corrected.gameplay.runtimeCombineTelemetryValid &&
            corrected.gameplay.runtimeCombineMismatch &&
            corrected.gameplay.runtimeCombineWriteAttempted &&
            corrected.gameplay.runtimeCombineWriteSucceeded &&
            corrected.gameplay.runtimeCombineConfirmed &&
            std::fabs(corrected.gameplay.runtimeCombineAspectBefore - native) < 0.001f &&
            std::fabs(corrected.gameplay.runtimeCombineTargetAspect - arbitraryTarget) < 0.001f &&
            std::fabs(corrected.gameplay.runtimeCombineAspectAfter - arbitraryTarget) < 0.001f &&
            memory.aspectWrites == 1 && memory.aspectFlagsWrites == 0 &&
            memory.flags == 0x04 && corrected.gameplay.runtimeCombineFlagsBefore ==
                corrected.gameplay.runtimeCombineFlagsAfter && corrected.mutateFov &&
            corrected.gameplay.runtimeCombineFovApplied &&
            std::fabs(corrected.gameplay.policyAspect - arbitraryTarget) < 0.001f &&
            corrected.gameplay.observation.policyAspect.provenance ==
                FovProvenance::ClientViewportAspect &&
            std::fabs(corrected.outputFov - gameplay::EvaluateHorPlus(
                90.6557f, arbitraryTarget, 0x04, native).outputFov) < 0.001f,
            "RuntimeCombine synchronizes an arbitrary target aspect before HorPlus and preserves native flags");

        CameraStateSnapshot publishedSnapshot{};
        bool publishedValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> eventSequence{};
        CameraStatePublicationView publicationState{publishedSnapshot, publishedValid,
            neutralCandidate, neutralSamples, neutralSource, zoomHold, eventSequence};
        CameraStatePublicationInput publication{};
        publication.zoomObserverAvailable = true;
        publication.zoomObservationValid = true;
        publication.observationPublished = corrected.gameplay.observationPublished;
        publication.observation = corrected.gameplay.observation;
        publication.source = corrected.gameplay.source;
        publication.applied = corrected.gameplay.applied;
        publication.presentation = PresentationState::Gameplay;
        publication.gameplayMode = static_cast<std::uint8_t>(
            config::GameplayMode::RuntimeCombine);
        publication.runtimeCombineValid = corrected.gameplay.runtimeCombineTelemetryValid;
        publication.runtimeCombineAspectBefore = corrected.gameplay.runtimeCombineAspectBefore;
        publication.runtimeCombineTargetAspect = corrected.gameplay.runtimeCombineTargetAspect;
        publication.runtimeCombineAspectAfter = corrected.gameplay.runtimeCombineAspectAfter;
        publication.runtimeCombineFovInput = corrected.gameplay.runtimeCombineFovInput;
        publication.runtimeCombineFovOutput = corrected.gameplay.runtimeCombineFovOutput;
        publication.runtimeCombineFlagsBefore = corrected.gameplay.runtimeCombineFlagsBefore;
        publication.runtimeCombineFlagsAfter = corrected.gameplay.runtimeCombineFlagsAfter;
        publication.runtimeCombineMismatch = corrected.gameplay.runtimeCombineMismatch;
        publication.runtimeCombineWriteAttempted = corrected.gameplay.runtimeCombineWriteAttempted;
        publication.runtimeCombineWriteSucceeded = corrected.gameplay.runtimeCombineWriteSucceeded;
        publication.runtimeCombineReadbackValid = corrected.gameplay.runtimeCombineReadbackValid;
        publication.runtimeCombineConfirmed = corrected.gameplay.runtimeCombineConfirmed;
        publication.runtimeCombineFovApplied = corrected.gameplay.runtimeCombineFovApplied;
        const auto publicationResult = PublishCameraStateSnapshotLocked(
            publicationState, publication);
        pass &= Check(publicationResult.published &&
            publishedSnapshot.evidence.runtimeCombineValid &&
            publishedSnapshot.evidence.runtimeCombineWriteAttempted &&
            publishedSnapshot.evidence.runtimeCombineWriteSucceeded &&
            publishedSnapshot.evidence.runtimeCombineReadbackValid &&
            publishedSnapshot.evidence.runtimeCombineConfirmed &&
            publishedSnapshot.evidence.runtimeCombineFlagsBefore == 0x04 &&
            publishedSnapshot.evidence.runtimeCombineFlagsAfter == 0x04 &&
            std::fabs(publishedSnapshot.evidence.runtimeCombineFovInput - 90.6557f) < 0.001f &&
            std::fabs(publishedSnapshot.evidence.runtimeCombineFovOutput -
                corrected.outputFov) < 0.001f,
            "RuntimeCombine transaction telemetry reaches the published Camera State snapshot");

        const auto alreadyMatched = run(arbitraryTarget, true);
        pass &= Check(alreadyMatched.gameplay.runtimeCombineConfirmed &&
            !alreadyMatched.gameplay.runtimeCombineMismatch &&
            !alreadyMatched.gameplay.runtimeCombineWriteAttempted &&
            memory.aspectWrites == 1 && alreadyMatched.mutateFov,
            "RuntimeCombine skips aspect mutation when the receiver already matches target");

        memory.aspect = native;
        memory.writeSucceeds = false;
        const auto writeFailed = run(arbitraryTarget, true);
        pass &= Check(writeFailed.gameplay.runtimeCombineMismatch &&
            writeFailed.gameplay.runtimeCombineWriteAttempted &&
            !writeFailed.gameplay.runtimeCombineWriteSucceeded &&
            !writeFailed.gameplay.runtimeCombineConfirmed &&
            !writeFailed.gameplay.runtimeCombineFovApplied && !writeFailed.mutateFov &&
            std::string_view(writeFailed.gameplay.reason) == "RUNTIME_ASPECT_UNCONFIRMED" &&
            std::fabs(writeFailed.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed and does not HorPlus-transform after an unconfirmed write");

        memory.writeSucceeds = true;
        memory.writeActuallyApplies = false;
        memory.aspect = native;
        const auto unconfirmedReadback = run(arbitraryTarget, true);
        pass &= Check(unconfirmedReadback.gameplay.runtimeCombineWriteSucceeded &&
            unconfirmedReadback.gameplay.runtimeCombineReadbackValid &&
            !unconfirmedReadback.gameplay.runtimeCombineConfirmed &&
            !unconfirmedReadback.gameplay.runtimeCombineFovApplied &&
            !unconfirmedReadback.mutateFov &&
            std::fabs(unconfirmedReadback.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed when a reported write does not confirm by read-back");

        memory.writeActuallyApplies = true;
        const auto invalidTarget = run(NAN, false);
        pass &= Check(!invalidTarget.gameplay.runtimeCombineConfirmed &&
            !invalidTarget.gameplay.runtimeCombineWriteAttempted &&
            !invalidTarget.gameplay.runtimeCombineFovApplied && !invalidTarget.mutateFov &&
            std::fabs(invalidTarget.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed when runtime target aspect is unavailable");

        const auto invalidMarkedTarget = run(-1.0f, true);
        pass &= Check(!invalidMarkedTarget.gameplay.runtimeCombineConfirmed &&
            !invalidMarkedTarget.gameplay.runtimeCombineWriteAttempted &&
            !invalidMarkedTarget.gameplay.runtimeCombineFovApplied &&
            !invalidMarkedTarget.mutateFov,
            "RuntimeCombine independently validates a target marked available");

        const auto disabled = run(arbitraryTarget, true, false);
        pass &= Check(memory.aspectWrites == 3 && !disabled.gameplay.applied &&
            !disabled.mutateFov && std::fabs(disabled.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine OFF does not perform correction or mutate the aspect");

        memory.writeSucceeds = true;
        memory.aspect = native;
        const auto aspect169 = run(native, true);
        const auto aspect219 = run(2.3888888f, true);
        const auto aspect329 = run(32.0f / 9.0f, true);
        pass &= Check(!aspect169.gameplay.runtimeCombineMismatch &&
            !aspect169.gameplay.runtimeCombineWriteAttempted &&
            !aspect169.mutateFov &&
            std::fabs(aspect169.outputFov - 90.6557f) < 0.001f &&
            aspect219.gameplay.runtimeCombineConfirmed && aspect219.mutateFov &&
            std::fabs(aspect219.outputFov - 107.3f) < 0.15f &&
            aspect329.gameplay.runtimeCombineConfirmed && aspect329.mutateFov &&
            std::fabs(aspect329.outputFov - 127.393f) < 0.02f &&
            memory.aspectWrites == 5 && memory.aspectFlagsWrites == 0,
            "RuntimeCombine production transaction covers 16:9, 21:9, and 32:9 domains");

        const auto seed = run(32.0f / 9.0f, true, true, 70.0005f);
        auto candidate = plugin::ProcessDialogueBoundary(dialogueView,
            {70.030342f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        auto accepted = plugin::ProcessDialogueBoundary(dialogueView,
            {70.0005f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        const auto otherContext = plugin::ClassifyDialogueWriterContext(dialogueView,
            0xCAFE, dialogueEpoch.load());
        const auto originalSource = memory.source;
        memory.source = 0xCAFE;
        const auto otherWriter = run(32.0f / 9.0f, true, true, accepted.outputFov, otherContext);
        pass &= Check(otherWriter.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
                otherWriter.writerContext == camera::WriterContext::TransitionAmbiguous &&
                otherWriter.mutateFov,
            "unproven receiver mismatch preserves correction and defers baseline admission");
        memory.source = originalSource;
        const auto interleavedContext = plugin::ClassifyDialogueWriterContext(dialogueView,
            memory.source, dialogueEpoch.load());
        const auto interleaved = run(32.0f / 9.0f, true, true, accepted.outputFov,
            interleavedContext);
        pass &= Check(dialoguePhase == dialogue::Phase::Active &&
            !interleaved.gameplay.dialogueContextInvalidated &&
            interleaved.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained,
            "interleaved other receiver does not erase dialogue context");
        plugin::ResetDialogueBoundaryState(dialogueView);
        const auto sameSeed = run(32.0f / 9.0f, true, true, 70.0005f);
        candidate = plugin::ProcessDialogueBoundary(dialogueView,
            {70.030342f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        accepted = plugin::ProcessDialogueBoundary(dialogueView,
            {70.0005f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        const auto sameContext = plugin::ClassifyDialogueWriterContext(dialogueView,
            memory.source, dialogueEpoch.load());
        const auto conflicted = run(32.0f / 9.0f, true, true, accepted.outputFov, sameContext);
        if (!(candidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
            accepted.action == plugin::DialogueBoundaryAction::AcceptedStart && accepted.mutateXmm1 &&
            std::fabs(accepted.outputFov - 60.9227f) < 0.01f &&
            conflicted.gameplay.dialogueContextInvalidated &&
            conflicted.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
            conflicted.gameplay.baselineProjection.baseline.nativeFov.value == sameSeed.inputFov &&
            !conflicted.gameplay.baselineProjection.baseline.current && conflicted.mutateFov))
            std::cerr << "context integration: candidate=" << int(candidate.action)
                << " accepted=" << int(accepted.action) << " mutation=" << accepted.mutateXmm1
                << " setter=" << accepted.outputFov << " context=" << int(sameContext)
                << " reset=" << conflicted.gameplay.dialogueContextInvalidated
                << " disposition=" << int(conflicted.gameplay.baselineProjection.disposition)
                << " baseline=" << conflicted.gameplay.baselineProjection.baseline.nativeFov.value
                << " seed=" << sameSeed.inputFov
                << " current=" << conflicted.gameplay.baselineProjection.baseline.current
                << " corrected=" << conflicted.mutateFov << '\n';
        pass &= Check(candidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
            accepted.action == plugin::DialogueBoundaryAction::AcceptedStart &&
            accepted.mutateXmm1 && std::fabs(accepted.outputFov - 60.9227f) < 0.01f &&
            conflicted.gameplay.dialogueContextInvalidated &&
            conflicted.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
            conflicted.gameplay.baselineProjection.baseline.nativeFov.value == sameSeed.inputFov &&
            !conflicted.gameplay.baselineProjection.baseline.current && conflicted.mutateFov &&
            otherContext == camera::WriterContext::TransitionAmbiguous,
            "dialogue context survives reset; unproven source relation blocks baseline only");
        const auto independent = run(32.0f / 9.0f, true, true, accepted.outputFov);
        const auto ten = run(32.0f / 9.0f, true, true, 10.0f);
        const auto twenty = run(32.0f / 9.0f, true, true, 20.0f);
        pass &= Check(independent.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Updated &&
            independent.gameplay.baselineProjection.baseline.current &&
            ten.mutateFov && twenty.mutateFov &&
            twenty.gameplay.baselineProjection.baseline.nativeFov.value == 20.0f,
            "independent same FOV and dynamic 10/20 gameplay remain eligible");
        const auto postExitEndpoint = run(32.0f / 9.0f, true, true, 70.0856f,
            camera::WriterContext::TransitionAmbiguous);
        pass &= Check(postExitEndpoint.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
            postExitEndpoint.gameplay.baselineProjection.baseline.nativeFov.value == 20.0f &&
            postExitEndpoint.mutateFov && postExitEndpoint.outputFov > 70.0856f,
            "post-exit endpoint is excluded from baseline while Gameplay HorPlus continues");
        const auto afterPostExitEndpoint = run(32.0f / 9.0f, true, true, 90.3019f);
        pass &= Check(afterPostExitEndpoint.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Updated &&
            afterPostExitEndpoint.gameplay.baselineProjection.baseline.nativeFov.value == 90.3019f,
            "Gameplay baseline admission resumes beyond the matched provider endpoint");
        exclusion.Arm(90.0f);
        const auto exclusionEndpoint = run(32.0f / 9.0f, true, true, 90.0f);
        const auto exclusionBlend = run(32.0f / 9.0f, true, true, 85.0f);
        const auto excludedGameplay = run(32.0f / 9.0f, true, true, 70.0f);
        pass &= Check(exclusionEndpoint.exclusionObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed &&
            exclusionBlend.exclusionObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed &&
            excludedGameplay.exclusionObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed &&
            excludedGameplay.writerContext ==
                camera::WriterContext::PostCinematicRecoveryExcluded &&
            excludedGameplay.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
            camera::IsUsableGameplayBaseline(
                excludedGameplay.gameplay.baselineProjection.baseline) &&
            excludedGameplay.gameplay.baselineProjection.baseline.nativeFov.value == 90.3019f &&
            excludedGameplay.mutateFov && excludedGameplay.outputFov > 70.0f,
            "post-cinematic exclusion retains same-receiver baseline without stopping Gameplay HorPlus");
        const auto exclusionRecovered = run(32.0f / 9.0f, true, true, 90.0f);
        pass &= Check(exclusionRecovered.exclusionObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Recovered &&
            exclusionRecovered.writerContext == camera::WriterContext::GameplayEligible &&
            exclusionRecovered.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Updated &&
            exclusionRecovered.gameplay.baselineProjection.baseline.nativeFov.value == 90.0f,
            "post-cinematic endpoint recovery restores Gameplay baseline admission");
        const auto sameReceiverTransition = run(32.0f / 9.0f, true, true, 70.0f,
            camera::WriterContext::PostCinematicRecoveryExcluded);
        const auto baselineForNextCinematic = cinematics::SelectCinematicBaseline(
            true, 90.0f, sameReceiverTransition.gameplay.baselineProjection.baseline);
        pass &= Check(sameReceiverTransition.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained &&
            camera::IsUsableGameplayBaseline(
                sameReceiverTransition.gameplay.baselineProjection.baseline) &&
            sameReceiverTransition.gameplay.baselineProjection.baseline.nativeFov.value == 90.0f &&
            sameReceiverTransition.mutateFov && sameReceiverTransition.outputFov > 70.0f &&
            baselineForNextCinematic.usedGameplayBaselineValue &&
            baselineForNextCinematic.gameplayRetentionWitnessAvailable &&
            baselineForNextCinematic.targetBaselineFov == 90.0f,
            "same-receiver transition keeps last trusted Gameplay baseline for following Cinematic");
        const auto recoverySeed = run(32.0f / 9.0f, true, true, 70.0005f);
        static_cast<void>(recoverySeed);
        static_cast<void>(plugin::ProcessDialogueBoundary(dialogueView,
            {70.030342f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source}));
        static_cast<void>(plugin::ProcessDialogueBoundary(dialogueView,
            {70.0005f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source}));
        static_cast<void>(plugin::ProcessDialogueBoundary(dialogueView,
            {69.9f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source}));
        const auto recoveryStart = plugin::ProcessDialogueBoundary(dialogueView,
            {70.0005f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        const auto recoveryComplete = plugin::ProcessDialogueBoundary(dialogueView,
            {70.0f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
        const auto postRecoveryContext = plugin::ClassifyDialogueWriterContext(
            dialogueView, memory.source, dialogueEpoch.load());
        const auto postRecoveryWriter = run(32.0f / 9.0f, true, true, 70.0005f,
            postRecoveryContext);
        if (!(recoveryStart.action == plugin::DialogueBoundaryAction::RecoveryStarted &&
            recoveryComplete.action == plugin::DialogueBoundaryAction::RecoveryCompleted &&
            postRecoveryContext == camera::WriterContext::GameplayEligible &&
            postRecoveryWriter.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Updated))
            std::cerr << "recovery lifecycle: start=" << int(recoveryStart.action)
                << "/" << int(recoveryStart.phaseAfter) << " complete="
                << int(recoveryComplete.action) << "/" << int(recoveryComplete.phaseAfter)
                << " context=" << int(postRecoveryContext) << " baseline="
                << int(postRecoveryWriter.gameplay.baselineProjection.disposition) << '\n';
        pass &= Check(recoveryStart.action == plugin::DialogueBoundaryAction::RecoveryStarted &&
            recoveryStart.phaseAfter == dialogue::Phase::RearmPending &&
            recoveryComplete.action == plugin::DialogueBoundaryAction::RecoveryCompleted &&
            recoveryComplete.phaseAfter == dialogue::Phase::Inactive &&
            postRecoveryContext == camera::WriterContext::GameplayEligible &&
            postRecoveryWriter.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Updated,
            "natural Dialogue recovery clears ambiguity and restores Gameplay baseline admission");

        for (const bool diagnosticMode : {false, true}) {
            diagnostics::SetEnabled(diagnosticMode);
            static_cast<void>(run(32.0f / 9.0f, true, true, 70.0005f));
            static_cast<void>(plugin::ProcessDialogueBoundary(dialogueView,
                {70.030342f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source}));
            const auto sample = plugin::ProcessDialogueBoundary(dialogueView,
                {70.0005f, 0x77, 70.0f, true, 0.01f, 0.02f, 3, false, memory.source});
            const auto context = plugin::ClassifyDialogueWriterContext(dialogueView,
                memory.source, dialogueEpoch.load());
            const auto result = run(32.0f / 9.0f, true, true, sample.outputFov, context);
            pass &= Check(result.mutateFov && result.gameplay.dialogueContextInvalidated &&
                result.gameplay.baselineProjection.disposition ==
                    camera::GameplayBaselineProjectionDisposition::Retained &&
                !camera::IsUsableGameplayBaseline(result.gameplay.baselineProjection.baseline),
                "context admission is independent of Diagnostics and rejects stale baseline");
        }
        diagnostics::SetEnabled(false);

        dialoguePolicy.Capture(config::DialogueZoomPolicy::Reduced);
        dialoguePhase = dialogue::Phase::Active;
        dialogueBaseline = 90.3115f;
        dialogueCandidate.Begin(dialogueBaseline, 0x77, 70.0f);
        dialogueCandidate.BindReceiver(memory.source, dialogueEpoch.load(), 0x77, 1, true);
        dialogueTarget.store(70.0f, std::memory_order_release);
        dialogueTargetValid.store(true, std::memory_order_release);
        const float reducedTarget = dialogue::ReducedTarget(dialogueBaseline);
        const auto providerSeed = run(32.0f / 9.0f, true, true, reducedTarget,
            camera::WriterContext::DialogueLifecycleObserved, true,
            dialogueBaseline, 70.0f, reducedTarget, true);
        const auto providerEndpoint = run(32.0f / 9.0f, true, true, 70.0357f,
            camera::WriterContext::DialogueLifecycleObserved, true,
            dialogueBaseline, 70.0f, reducedTarget, false);
        const float expectedDialogueOutput = gameplay::EvaluateHorPlus(
            reducedTarget, 32.0f / 9.0f, 0x04, native).outputFov;
        pass &= Check(!providerSeed.gameplay.dialogueContextInvalidated &&
            !providerEndpoint.gameplay.dialogueContextInvalidated &&
            dialoguePhase == dialogue::Phase::Active && providerSeed.mutateFov &&
            providerEndpoint.mutateFov &&
            std::fabs(providerSeed.outputFov - expectedDialogueOutput) < 0.03f &&
            std::fabs(providerEndpoint.outputFov - expectedDialogueOutput) < 0.03f &&
            providerEndpoint.gameplay.baselineProjection.disposition ==
                camera::GameplayBaselineProjectionDisposition::Retained,
            "matched provider Dialogue handoff keeps the reduced endpoint continuous through the next native writer sample");

        // Runtime regression: seed receiver A at 90°, then observe receiver B
        // at 130° while lifecycle classification is ambiguous. The production
        // writer must continue applying Gameplay HorPlus to B but may not
        // expose A's baseline as current for Cinematic/provider decisions.
        constexpr std::uintptr_t previousReceiver = 0xC01;
        constexpr std::uintptr_t reloadedReceiver = 0xC03;
        constexpr float reloadedNativeFov = 130.3115f;
        coordinator.store(camera::CoordinatorState::Gameplay,
            std::memory_order_release);
        exclusion.Reset();
        owner.Clear();
        plugin::ResetDialogueBoundaryState(dialogueView);
        memory.readable = true;
        memory.writeSucceeds = true;
        memory.flags = 0x04;
        static_cast<void>(gameplayState.baseline.Invalidate());
        memory.source = previousReceiver;
        memory.aspect = 32.0f / 9.0f;
        const auto baselineSeed = run(32.0f / 9.0f, true, true, 90.3115f);
        memory.source = reloadedReceiver;
        const auto replacementWriter = run(32.0f / 9.0f, true, true,
            reloadedNativeFov, camera::WriterContext::TransitionAmbiguous);
        if (!(baselineSeed.gameplay.baselineProjection.baseline.source.value ==
                previousReceiver && replacementWriter.mutateFov &&
            std::fabs(replacementWriter.outputFov - 153.931f) < 0.02f &&
            replacementWriter.gameplay.baselineProjection.baseline.source.value ==
                previousReceiver &&
            !replacementWriter.gameplay.baselineProjection.baseline.current &&
            !camera::IsUsableGameplayBaseline(
                replacementWriter.gameplay.baselineProjection.baseline)))
            std::cerr << "receiver replacement: seeded="
                << baselineSeed.gameplay.baselineProjection.baseline.source.value
                << " old=" << previousReceiver << " disposition="
                << int(replacementWriter.gameplay.baselineProjection.disposition)
                << " source="
                << replacementWriter.gameplay.baselineProjection.baseline.source.value
                << " current="
                << replacementWriter.gameplay.baselineProjection.baseline.current
                << " usable=" << camera::IsUsableGameplayBaseline(
                    replacementWriter.gameplay.baselineProjection.baseline)
                << " output=" << replacementWriter.outputFov
                << " mutate=" << replacementWriter.mutateFov << '\n';
        pass &= Check(baselineSeed.gameplay.baselineProjection.baseline.source.value ==
                previousReceiver && replacementWriter.mutateFov &&
            std::fabs(replacementWriter.outputFov - 153.931f) < 0.02f &&
            replacementWriter.gameplay.baselineProjection.baseline.source.value ==
                previousReceiver &&
            !replacementWriter.gameplay.baselineProjection.baseline.current &&
            !camera::IsUsableGameplayBaseline(
                replacementWriter.gameplay.baselineProjection.baseline),
            "receiver_reload_90_to_130_keeps_gameplay_horplus_and_retires_stale_baseline");

        // Full positive lifecycle: a confirmed same-receiver provider Dialogue
        // must not overwrite or retire the previously admitted Gameplay
        // reference needed by the next GameplayHorPlus Cinematic ENTER.
        {
            constexpr std::uintptr_t lifecycleReceiver = 0xC04;
            constexpr float gameplayNativeFov = 130.3115f;
            constexpr float authoredCinematicFov = 90.0f;
            constexpr float runtimeAspect = 32.0f / 9.0f;
            coordinator.store(camera::CoordinatorState::Gameplay,
                std::memory_order_release);
            exclusion.Reset();
            owner.Clear();
            plugin::ResetDialogueBoundaryState(dialogueView);
            static_cast<void>(gameplayState.baseline.Invalidate());
            memory.source = lifecycleReceiver;
            memory.aspect = runtimeAspect;
            memory.flags = 0x04;

            const auto gameplayAdmission = run(runtimeAspect, true, true,
                gameplayNativeFov);
            const auto cinematic1Reference = cinematics::SelectCinematicBaseline(
                true, authoredCinematicFov,
                gameplayAdmission.gameplay.baselineProjection.baseline);
            float cinematic1Target = authoredCinematicFov;
            const bool cinematic1Transformed = cinematics::TryTransformCinematicFov(
                authoredCinematicFov, authoredCinematicFov,
                cinematic1Reference.targetBaselineFov, runtimeAspect, native,
                cinematic1Target);
            pass &= Check(gameplayAdmission.gameplay.baselineProjection.disposition ==
                    camera::GameplayBaselineProjectionDisposition::Updated &&
                camera::IsUsableGameplayBaseline(
                    gameplayAdmission.gameplay.baselineProjection.baseline) &&
                cinematic1Reference.valueOrigin ==
                    cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline &&
                cinematic1Transformed && std::fabs(cinematic1Target - 153.931f) < 0.03f,
                "cycle_gameplay_and_cinematic1_use_admitted_130_degree_gameplay_reference");

            dialoguePolicy.Capture(config::DialogueZoomPolicy::Reduced);
            plugin::DialogueBoundaryInput providerDialogueInput{};
            providerDialogueInput.incomingFov = 70.0f;
            providerDialogueInput.callbackSource = 0x77;
            providerDialogueInput.nativeTarget = 70.0f;
            providerDialogueInput.nativeTargetReadable = true;
            providerDialogueInput.handledProviderActivation = true;
            providerDialogueInput.validatedReceiver = lifecycleReceiver;
            providerDialogueInput.generation = 42;
            providerDialogueInput.observedReceiver = lifecycleReceiver;
            providerDialogueInput.providerGameplayBaseline = gameplayNativeFov;
            providerDialogueInput.providerGameplayBaselineValid = true;
            providerDialogueInput.providerDialoguePolicy =
                config::DialogueZoomPolicy::Reduced;
            providerDialogueInput.providerDialoguePolicyValid = true;
            providerDialogueInput.threadId = 77;
            const auto providerDialogueAdmission = plugin::ProcessDialogueBoundary(
                dialogueView, providerDialogueInput);
            const dialogue::PostExitProviderActivation providerActivation{42,
                dialogueEpoch.load(), 0xC05, lifecycleReceiver, 77, 126.16f,
                70.0f, 126.16f, config::DialogueZoomPolicy::Reduced};
            const bool confirmedProviderOwnership =
                plugin::IsConfirmedProviderDialogueLifecycle(dialogueView,
                    providerActivation, lifecycleReceiver, 77, 42);
            const auto confirmedDialogueContext = plugin::ClassifyDialogueWriterContext(
                dialogueView, lifecycleReceiver, dialogueEpoch.load(), 42, false,
                confirmedProviderOwnership);
            const float cycleReducedTarget = dialogue::ReducedTarget(gameplayNativeFov);
            const auto dialogueWriter = run(runtimeAspect, true, true,
                cycleReducedTarget, confirmedDialogueContext, true,
                gameplayNativeFov, 70.0f, cycleReducedTarget, true);
            const float expectedCycleDialogueOutput = gameplay::EvaluateHorPlus(
                cycleReducedTarget, runtimeAspect, 0x04, native).outputFov;
            pass &= Check(providerDialogueAdmission.action ==
                    plugin::DialogueBoundaryAction::ProviderDialogueStarted &&
                providerDialogueAdmission.activePolicy ==
                    config::DialogueZoomPolicy::Reduced &&
                providerDialogueAdmission.mutateXmm1 &&
                confirmedDialogueContext ==
                    camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved &&
                dialogueWriter.mutateFov &&
                std::fabs(dialogueWriter.outputFov - expectedCycleDialogueOutput) < 0.03f &&
                std::fabs(dialogueWriter.outputFov - 149.071f) < 0.08f &&
                dialogueWriter.gameplay.baselineProjection.disposition ==
                    camera::GameplayBaselineProjectionDisposition::Retained &&
                dialogueWriter.gameplay.baselineProjection.baseline.nativeFov.value ==
                    gameplayNativeFov,
                "cycle_confirmed_reduced_dialogue_uses_reduced_target_without_replacing_gameplay_reference");

            const auto cinematic2Baseline =
                dialogueWriter.gameplay.baselineProjection.baseline;
            dialogue::PostExitProviderHandoff repeatedSetterHandoff;
            repeatedSetterHandoff.ObserveArmer(true, providerActivation);
            repeatedSetterHandoff.CompleteRecovery();
            repeatedSetterHandoff.ConsumeDialogueAdmission(providerActivation.component,
                lifecycleReceiver, 77, 42, dialogueEpoch.load(),
                providerActivation.nativeStart, 70.0f);
            plugin::CameraWriterResult lastDialogueWriter{};
            providerDialogueInput.handledProviderActivation = false;
            for (const float raw : {70.0f, 70.0f, 69.9999f, 69.9999f}) {
                providerDialogueInput.incomingFov = raw;
                const auto setter = plugin::ProcessDialogueBoundary(dialogueView,
                    providerDialogueInput);
                plugin::RecordProviderDialogueSetterResult(dialogueView,
                    providerDialogueInput, setter, repeatedSetterHandoff);
                // The same native field may be read several times between
                // setter updates. None may apply Dialogue mapping twice.
                for (unsigned repeat = 0; repeat < 3; ++repeat) {
                    const bool correctedInput =
                        repeatedSetterHandoff.MatchesCorrectedDialogueSetterSample(
                            lifecycleReceiver, 77, 42, dialogueEpoch.load(), setter.outputFov);
                    lastDialogueWriter = run(runtimeAspect, true, true,
                        setter.outputFov, confirmedDialogueContext, true,
                        gameplayNativeFov, 70.0f, cycleReducedTarget, correctedInput, 77);
                    pass &= Check(correctedInput && lastDialogueWriter.mutateFov &&
                        std::fabs(lastDialogueWriter.outputFov - 149.071f) < 0.08f,
                        "repeated_provider_setter_writer_transactions_do_not_double_transform_reduced");
                }
            }
            const auto correctedStart = cinematics::MakeDialogueWriterResultCandidate(
                lastDialogueWriter.gameplay.observation, lifecycleReceiver,
                dialogueEpoch.load(), 42, true);
            const float nativeStart = lastDialogueWriter.gameplay.inputFov;
            coordinator.store(camera::CoordinatorState::CinematicActive);
            cinematicCorrection.store(true);
            cinematicSelection.store(true);
            cinematicGeneration.store(43);
            cinematicSettingsEpoch.store(0);
            cinematicTargetEstablished.store(true);
            authoredFov.store(90.0f);
            cinematicTargetFov.store(153.931f);
            association.BeginGeneration(43, 90.0f, 153.931f,
                {true, config::CinematicAspectPolicy::Auto,
                    config::CinematicFovMode::GameplayHorPlus}, gameplayNativeFov,
                lifecycleReceiver);
            association.ObserveAspectStoreTarget(43, lifecycleReceiver);
            enterActivation.Arm(43, 0, lifecycleReceiver, gameplayNativeFov,
                153.931f, true, 153.931f, correctedStart);
            const bool captured = enterActivation.Capture(true, 43, 0, 0xC06,
                lifecycleReceiver, 77, nativeStart, 153.931f, dialogueEpoch.load());
            plugin::ResetDialogueBoundaryState(dialogueView);
            for (const float weight : {0.0f, 0.03f, 0.17f, 0.49f, 0.83f, 1.0f}) {
                const auto cinematicWriter = run(runtimeAspect, true, true,
                    nativeStart + weight * (153.931f - nativeStart));
                const float expected = lastDialogueWriter.outputFov + weight *
                    (153.931f - lastDialogueWriter.outputFov);
                pass &= Check(captured && correctedStart.valid &&
                    cinematicWriter.enterActivationMappingApplied &&
                    cinematicWriter.cinematicOutputAdmitted && cinematicWriter.mutateFov &&
                    std::fabs(cinematicWriter.outputFov - expected) < 0.02f &&
                    dialoguePhase == dialogue::Phase::Inactive,
                    "confirmed_dialogue_to_cinematic_all_writer_samples_keep_corrected_trajectory");
            }
            const auto cinematic2Reference = cinematics::SelectCinematicBaseline(
                true, authoredCinematicFov, cinematic2Baseline);
            float cinematic2Target = authoredCinematicFov;
            const bool cinematic2Transformed = cinematics::TryTransformCinematicFov(
                authoredCinematicFov, authoredCinematicFov,
                cinematic2Reference.targetBaselineFov, runtimeAspect, native,
                cinematic2Target);
            pass &= Check(cinematic2Baseline.valid &&
                cinematic2Baseline.source.value == lifecycleReceiver &&
                cinematic2Baseline.nativeFov.valid &&
                std::fabs(cinematic2Baseline.nativeFov.value - gameplayNativeFov) < 0.001f,
                "cycle_dialogue_samples_retain_the_original_gameplay_baseline_data");
            pass &= Check(cinematic2Baseline.current &&
                camera::IsUsableGameplayBaseline(cinematic2Baseline),
                "cycle_confirmed_dialogue_does_not_retire_same_receiver_gameplay_baseline");
            pass &= Check(cinematic2Reference.usedGameplayBaselineValue &&
                cinematic2Reference.valueOrigin ==
                    cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline &&
                cinematic2Reference.gameplayRetentionWitnessAvailable &&
                std::fabs(cinematic2Reference.targetBaselineFov - gameplayNativeFov) < 0.001f,
                "cycle_cinematic2_selects_gameplay_native_reference_with_retention_witness");
            pass &= Check(cinematic2Transformed &&
                std::fabs(cinematic2Target - 153.931f) < 0.03f,
                "cycle_cinematic2_targets_the_gameplayhorplus_fov");
        }
        return pass;
    }

    bool CheckSeededWriterSequenceAndDiagnosticParity()
    {
        struct Outcome
        {
            float output{};
            std::uintptr_t baselineSource{};
            float baselineNativeFov{NAN};
            camera::WriterContext context{};
            bool mutate{};
            bool baselineCurrent{};
            bool baselineUsable{};
        };

        constexpr std::uint32_t seed = 0x4F9A31C7u;
        constexpr std::size_t stepCount = 48;
        constexpr float nativeAspect = 16.0f / 9.0f;
        constexpr std::uintptr_t receivers[] = {0xA11, 0xB22, 0xC33};
        constexpr float fovs[] = {70.0f, 90.3115f, 100.0f, 130.3115f};
        constexpr float aspects[] = {16.0f / 9.0f, 21.0f / 9.0f,
            32.0f / 9.0f};
        constexpr config::GameplayMode modes[] = {
            config::GameplayMode::HorPlus,
            config::GameplayMode::RuntimeCombine};
        std::array<Outcome, stepCount> results[2]{};
        bool pass = true;

        for (std::size_t diagnosticRun = 0; diagnosticRun < 2; ++diagnosticRun) {
            const bool diagnosticsEnabled = diagnosticRun != 0;
            diagnostics::SetEnabled(diagnosticsEnabled);

            std::atomic<camera::CoordinatorState> coordinator{
                camera::CoordinatorState::Gameplay};
            cinematics::CinematicWriterAssociation association;
            cinematics::CinematicRecoveryOwner owner;
            dialogue::PostCinematicRecoveryExclusion exclusion;
            std::atomic<bool> handoffPending{};
            std::atomic<std::uintptr_t> handoffSource{};
            std::atomic<float> handoffFov{NAN};
            std::atomic<bool> continuationPending{};
            std::atomic<bool> continuationCarryPending{};
            std::atomic<std::uintptr_t> continuationSource{};
            std::atomic<float> continuationTarget{NAN};
            std::atomic<float> continuationPrevious{NAN};
            std::atomic<bool> continuationInterpolation{};
            std::atomic<float> continuationStart{NAN};
            std::atomic<std::uintptr_t> baselineSource{};
            std::atomic<float> recoveryTarget{NAN};
            std::atomic<float> recoveryCinematicTarget{NAN};
            plugin::CameraWriterRecoveryStateView recoveryState{coordinator,
                association, owner, exclusion, handoffPending, handoffSource,
                handoffFov, continuationPending, continuationCarryPending,
                continuationSource, continuationTarget, continuationPrevious,
                continuationInterpolation, continuationStart, baselineSource,
                recoveryTarget, recoveryCinematicTarget};
            std::atomic<bool> cinematicEnabled{};
            std::atomic<bool> cinematicSelection{};
            std::atomic<std::uint64_t> cinematicGeneration{};
            std::atomic<std::uint64_t> cinematicSettingsEpoch{};
            cinematics::CinematicEnterActivation enterActivation;
            std::atomic<bool> cinematicTargetEstablished{};
            std::atomic<float> authoredFov{NAN};
            std::atomic<float> cinematicTargetFov{NAN};
            std::atomic<std::uintptr_t> cinematicContinuationSource{};
            std::atomic<float> cinematicContinuationStart{NAN};
            std::atomic<bool> cinematicContinuationActive{};
            plugin::CameraWriterCinematicStateView cinematicState{cinematicEnabled,
                cinematicSelection, cinematicGeneration, cinematicSettingsEpoch,
                cinematicSettingsEpoch, cinematicTargetEstablished, authoredFov,
                cinematicTargetFov, cinematicContinuationSource,
                cinematicContinuationStart, cinematicContinuationActive,
                association, enterActivation};
            plugin::CameraCorrectionGate gate;
            GameplayWriterFixture gameplayState;
            FreshFieldsFixture memory{};
            memory.flags = 0x04;
            const plugin::CameraWriterEngineOperations engineOperations{&memory,
                &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
            std::uint32_t randomState = seed;
            const auto nextRandom = [&]() {
                randomState ^= randomState << 13;
                randomState ^= randomState >> 17;
                randomState ^= randomState << 5;
                return randomState;
            };

            for (std::size_t step = 0; step < stepCount; ++step) {
                const auto bits = nextRandom();
                const auto source = receivers[(bits >> 1) % std::size(receivers)];
                const float inputFov = fovs[(bits >> 5) % std::size(fovs)];
                const float aspect = aspects[(bits >> 9) % std::size(aspects)];
                const auto mode = modes[(bits >> 13) % std::size(modes)];
                const auto context = (bits & 0x10000u) != 0
                    ? camera::WriterContext::GameplayEligible
                    : camera::WriterContext::TransitionAmbiguous;
                const bool enabled = (bits & 0x20000u) == 0;
                memory.expectedSource = source;
                memory.aspect = aspect;
                memory.readable = true;

                plugin::CameraWriterInput input{source, 0xF00D, inputFov,
                    aspect, true, 0x04, true, 0.02f, enabled, mode, nativeAspect};
                input.currentViewportAspect = aspect;
                input.currentViewportAspectValid = true;
                input.writerContext = context;
                input.writerTransactionId = step + 1;
                plugin::CameraWriterResult result{};
                {
                    auto gameplayView = gameplayState.View();
                    auto writerGate = gate.Acquire();
                    result = plugin::ProcessCameraWriter(gate, recoveryState,
                        input, cinematicState, engineOperations, {}, &gameplayView);
                }

                double expectedOutput = inputFov;
                if (enabled) {
                    expectedOutput = inputFov;
                    if (mode == config::GameplayMode::HorPlus ||
                        mode == config::GameplayMode::RuntimeCombine) {
                        expectedOutput = 2.0 * std::atan(std::tan(
                            static_cast<double>(inputFov) * 3.14159265358979323846 / 360.0) *
                            (static_cast<double>(aspect) / nativeAspect)) *
                            180.0 / 3.14159265358979323846;
                    }
                }
                const auto baseline = gameplayState.baseline.Read();
                const bool stepPass = result.processed &&
                    std::fabs(result.outputFov - expectedOutput) < 0.02 &&
                    (!enabled || !camera::IsUsableGameplayBaseline(baseline) ||
                        baseline.source.value == source) &&
                    (context != camera::WriterContext::GameplayEligible || !enabled ||
                        (baseline.current && baseline.source.valid &&
                            baseline.source.value == source));
                if (!stepPass) {
                    std::cerr << "FAIL: seeded_writer_sequence seed=0x" << std::hex
                        << seed << std::dec << " step=" << step << " diagnostic="
                        << (diagnosticsEnabled ? "on" : "off")
                        << " source=0x" << std::hex << source << std::dec
                        << " input=" << inputFov << " aspect=" << aspect
                        << " mode=" << static_cast<unsigned>(mode)
                        << " context=" << static_cast<unsigned>(context)
                        << " enabled=" << enabled << " output=" << result.outputFov
                        << " expected=" << expectedOutput << " processed=" << result.processed
                        << " baseline_valid=" << baseline.valid
                        << " baseline_current=" << baseline.current
                        << " baseline_source=0x" << std::hex << baseline.source.value
                        << std::dec << " baseline_native=" << baseline.nativeFov.value
                        << '\n';
                }
                pass &= Check(stepPass,
                    "seeded writer sequence preserves FOV and receiver baseline contract");
                results[diagnosticRun][step] = {result.outputFov,
                    baseline.source.value, baseline.nativeFov.value, context,
                    result.mutateFov, baseline.current,
                    camera::IsUsableGameplayBaseline(baseline)};
            }
        }
        diagnostics::SetEnabled(false);

        for (std::size_t step = 0; step < stepCount; ++step) {
            const auto& off = results[0][step];
            const auto& on = results[1][step];
            pass &= Check(std::fabs(off.output - on.output) < 0.0001f &&
                off.baselineSource == on.baselineSource &&
                (std::isnan(off.baselineNativeFov) && std::isnan(on.baselineNativeFov)
                    ? true : std::fabs(off.baselineNativeFov - on.baselineNativeFov) < 0.0001f) &&
                off.context == on.context && off.mutate == on.mutate &&
                off.baselineCurrent == on.baselineCurrent &&
                off.baselineUsable == on.baselineUsable,
                "diagnostics ON/OFF transaction outcome parity");
        }
        return pass;
    }

    bool CheckWriterOffOrder(bool writerFirst)
    {
        using namespace camera;
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<std::uint64_t> cinematicSettingsEpoch{};
        cinematics::CinematicEnterActivation enterActivation;
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraCorrectionGate gate;
        plugin::CameraWriterRecoveryStateView writerState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget,
            continuationPrevious, continuationInterpolation, continuationStart,
            baselineSource, recoveryTarget, recoveryCinematicTarget};
        SettingsFixture settings;
        GameplayWriterFixture gameplayState;
        ModeTransitionBarrier modeOperation{};
        modeOperation.pause = writerFirst;
        FreshFieldsFixture freshFields{0xCA};
        freshFields.barrier = &modeOperation;
        const plugin::CameraWriterEngineOperations freshReader{
            &freshFields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
        gameplayState.modeTransitionPending.store(writerFirst,
            std::memory_order_release);
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicSettingsEpoch, cinematicSettingsEpoch,
            cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
            cinematicContinuationSource, cinematicContinuationStart,
            cinematicContinuationActive, association, enterActivation};
        float finalOutput = 90.6557f;
        bool correctionApplied = false;
        const plugin::CameraWriterGameplayOperations gameplayOperations{};

        auto executeWriter = [&](config::GameplayMode mode =
                config::GameplayMode::HorPlus) {
            const plugin::CameraWriterInput input{
                0xCA, 0xCB, 90.6557f, 3.55556f, true, 0x04, true,
                0.02f, settings.gameplayEnabled.load(std::memory_order_acquire),
                mode, 16.0f / 9.0f};
            auto policyInput = input;
            policyInput.currentViewportAspect = 3.55556f;
            policyInput.currentViewportAspectValid = true;
            auto gameplayView = gameplayState.View();
            const auto observed = plugin::ProcessCameraWriter(gate, writerState,
                policyInput, cinematicState, freshReader, {}, &gameplayView,
                gameplayOperations);
            finalOutput = observed.outputFov;
            correctionApplied = observed.mutateFov;
            return observed;
        };

        if (writerFirst) {
            std::mutex readyMutex;
            std::condition_variable readyCv;
            bool setterStarted = false;
            plugin::CameraSettingTransactionResult setterResult{};
            std::atomic<bool> offReturned{};
            std::thread correction([&] {
                auto writerGuard = gate.Acquire();
                static_cast<void>(executeWriter());
            });
            {
                std::unique_lock lock(modeOperation.mutex);
                modeOperation.condition.wait(lock, [&] { return modeOperation.entered; });
            }
            std::thread setter([&] {
                {
                    std::lock_guard lock(readyMutex);
                    setterStarted = true;
                }
                readyCv.notify_one();
                setterResult = plugin::ApplyCameraSetting(gate,
                    settings.View(),
                    plugin::RuntimeSettingMutation::GameplayEnabled(false),
                    true, [] {});
                offReturned.store(true, std::memory_order_release);
            });
            bool offStayedBlocked = false;
            {
                std::unique_lock lock(readyMutex);
                readyCv.wait(lock, [&] { return setterStarted; });
                offStayedBlocked = !offReturned.load(std::memory_order_acquire);
            }
            {
                std::lock_guard lock(modeOperation.mutex);
                modeOperation.release = true;
            }
            modeOperation.condition.notify_all();
            correction.join();
            setter.join();
            return setterResult.mutation.accepted &&
                offStayedBlocked && offReturned.load(std::memory_order_acquire) &&
                !settings.gameplayEnabled.load(std::memory_order_acquire) &&
                correctionApplied && std::fabs(finalOutput - 127.393f) < 0.01f &&
                modeOperation.calls == 2;
        }

        const auto off = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
        {
            auto writerGuard = gate.Acquire();
            static_cast<void>(executeWriter());
        }
        bool offAllModesInert = true;
        const std::array<config::GameplayMode, 2> modes{
            config::GameplayMode::AspectRecalculation,
            config::GameplayMode::HorPlus};
        for (const auto mode : modes) {
            auto writerGuard = gate.Acquire();
            const auto result = executeWriter(mode);
            offAllModesInert &= result.gameplay.handled &&
                result.gameplay.reason == std::string_view("GAMEPLAY_DISABLED") &&
                !result.mutateFov && result.outputFov == 90.6557f;
        }
        return off.mutation.accepted &&
            !settings.gameplayEnabled.load(std::memory_order_acquire) &&
            !correctionApplied && finalOutput == 90.6557f && offAllModesInert &&
            modeOperation.calls == (writerFirst ? 1u : 0u);
    }

    bool CheckGameplayPolicyUsesValidatedViewportAspect()
    {
        using namespace camera;
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<std::uint64_t> cinematicSettingsEpoch{};
        cinematics::CinematicEnterActivation enterActivation;
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraWriterRecoveryStateView writerState{coordinator,
            association, owner, exclusion, handoffPending, handoffSource,
            handoffFov, continuationPending, continuationCarryPending,
            continuationSource, continuationTarget, continuationPrevious,
            continuationInterpolation, continuationStart, baselineSource,
            recoveryTarget, recoveryCinematicTarget};
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicSettingsEpoch, cinematicSettingsEpoch,
            cinematicTargetEstablished, cinematicAuthoredFov,
            cinematicTargetFov, cinematicContinuationSource,
            cinematicContinuationStart, cinematicContinuationActive,
            association, enterActivation};
        plugin::CameraCorrectionGate gate;
        GameplayWriterFixture gameplayState;
        FreshFieldsFixture fields{0xCA};
        fields.aspect = 3.55556f;
        const plugin::CameraWriterEngineOperations engineOperations{
            &fields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};

        CameraStateSnapshot snapshot{};
        bool snapshotValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> eventSequence{};
        CameraStatePublicationView publicationState{snapshot, snapshotValid,
            neutralCandidate, neutralSamples, neutralSource, zoomHold,
            eventSequence};

        const auto publishWriter = [&](float inputFov, float nativeWriterAspect,
            float viewportAspect,
            bool viewportValid = true) {
            fields.aspect = nativeWriterAspect;
            plugin::CameraWriterInput input{0xCA, 0xCB, inputFov, nativeWriterAspect, true,
                0x04, true, 0.02f, true, config::GameplayMode::HorPlus,
                16.0f / 9.0f};
            input.currentViewportAspect = viewportAspect;
            input.currentViewportAspectValid = viewportValid;
            auto gameplayView = gameplayState.View();
            auto guard = gate.Acquire();
            const auto result = plugin::ProcessCameraWriter(gate, writerState,
                input, cinematicState, engineOperations, {}, &gameplayView);
            CameraStatePublicationInput publication{};
        publication.zoomObserverAvailable = true;
        publication.zoomObservationValid = true;
            publication.observationPublished = result.gameplay.observationPublished;
            publication.observation = result.gameplay.observation;
            publication.source = result.gameplay.source;
            publication.applied = result.gameplay.applied;
            publication.presentation = PresentationState::Gameplay;
            publication.dialogue = DialogueState::Inactive;
            publication.gameplayMode = static_cast<std::uint8_t>(
                config::GameplayMode::HorPlus);
            publication.nativeAspect = 16.0f / 9.0f;
            const auto published = PublishCameraStateSnapshotLocked(
                publicationState, publication);
            return std::pair{result, published};
        };

        bool pass = true;
        const auto at169 = publishWriter(90.6557f, 16.0f / 9.0f, 32.0f / 9.0f);
        const auto at219 = publishWriter(90.6557f, 2.3888888f, 16.0f / 9.0f);
        const auto at329 = publishWriter(90.6557f, 32.0f / 9.0f, 16.0f / 9.0f);
        pass &= Check(!at169.first.gameplay.applied &&
            !at169.first.mutateFov &&
            std::fabs(at169.first.outputFov - 90.6557f) < 0.001f &&
            at219.first.gameplay.applied && at219.first.mutateFov &&
            std::fabs(at219.first.outputFov - 107.3f) < 0.15f &&
            at329.first.gameplay.applied && at329.first.mutateFov &&
            std::fabs(at329.first.outputFov - 127.393f) < 0.02f,
            "production writer transaction uses native writer aspect for 16:9/21:9/32:9 despite viewport divergence");
        pass &= Check(at329.second.published && snapshot.evidence.aspectValid &&
            std::fabs(fields.aspect - 32.0f / 9.0f) < 0.0001f &&
            std::fabs(snapshot.evidence.aspect - 3.55556f) < 0.0001f &&
            snapshot.evidence.gameplayPolicyAspectValid &&
            std::fabs(snapshot.evidence.gameplayPolicyAspect - 32.0f / 9.0f) < 0.0001f &&
            snapshot.evidence.gameplayPolicyAspectProvenance ==
                EvidenceProvenance::NativeNumericObservation &&
            snapshot.evidence.currentViewportAspectValid &&
            std::fabs(snapshot.evidence.currentViewportAspect - 16.0f / 9.0f) < 0.0001f &&
            snapshot.evidence.currentViewportAspectProvenance ==
                EvidenceProvenance::ClientViewportObservation &&
            snapshot.freshness.transformedResult == SnapshotFreshness::Current,
            "snapshot keeps native policy authority and independent viewport observation/provenance");

        const auto adsWriter = publishWriter(85.0f, 32.0f / 9.0f,
            16.0f / 9.0f).first;
        pass &= Check(adsWriter.gameplay.applied &&
            std::fabs(adsWriter.outputFov - at329.first.outputFov) > 1.0f &&
            std::fabs(adsWriter.gameplay.observation.aspect.value - 3.55556f) < 0.0001f &&
            std::fabs(adsWriter.gameplay.observation.policyAspect.value - 32.0f / 9.0f) < 0.0001f &&
            adsWriter.gameplay.observation.policyAspect.provenance ==
                camera::FovProvenance::NativeWriterAspect &&
            std::fabs(adsWriter.gameplay.observation.currentViewportAspect.value - 16.0f / 9.0f) < 0.0001f,
            "ADS changes writer FOV while HorPlus authority and viewport remain separately observed");

        const auto invalidViewport = publishWriter(90.6557f,
            32.0f / 9.0f, NAN, false).first;
        pass &= Check(invalidViewport.gameplay.applied &&
            invalidViewport.mutateFov &&
            std::fabs(invalidViewport.outputFov - 127.393f) < 0.02f &&
            !invalidViewport.gameplay.observation.currentViewportAspect.valid,
            "unavailable viewport telemetry does not replace or disable native writer policy authority");

        plugin::CameraWriterInput disabledInput{0xCA, 0xCB, 90.6557f, 3.55556f,
            true, 0x04, true, 0.02f, false, config::GameplayMode::HorPlus,
            16.0f / 9.0f};
        disabledInput.currentViewportAspect = 32.0f / 9.0f;
        disabledInput.currentViewportAspectValid = true;
        auto disabledGameplayView = gameplayState.View();
        plugin::CameraWriterResult disabledWriter{};
        {
            auto guard = gate.Acquire();
            disabledWriter = plugin::ProcessCameraWriter(gate, writerState,
                disabledInput, cinematicState, engineOperations, {},
                &disabledGameplayView);
        }
        pass &= Check(!disabledWriter.gameplay.applied &&
            !disabledWriter.mutateFov && disabledWriter.outputFov == 90.6557f,
            "Gameplay OFF remains pass-through regardless of viewport observation");
        return pass;
    }

    bool CheckCameraStatePublicationAfterWriterChange()
    {
        constexpr std::uintptr_t sourceA = 0xA1;
        constexpr std::uintptr_t sourceB = 0xB1;
        std::atomic<camera::CoordinatorState> coordinator{
            camera::CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<std::uint64_t> cinematicSettingsEpoch{};
        cinematics::CinematicEnterActivation enterActivation;
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraCorrectionGate gate;
        plugin::CameraWriterRecoveryStateView writerState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget, continuationPrevious, continuationInterpolation,
            continuationStart, baselineSource, recoveryTarget,
            recoveryCinematicTarget};
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicSettingsEpoch, cinematicSettingsEpoch,
            cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
            cinematicContinuationSource, cinematicContinuationStart,
            cinematicContinuationActive, association, enterActivation};
        SettingsFixture settings;
        GameplayWriterFixture gameplayState;
        FreshFieldsFixture fields{sourceA};
        const plugin::CameraWriterEngineOperations engineOperations{
            &fields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};

        std::mutex snapshotMutex;
        camera::CameraStateSnapshot snapshot{};
        bool snapshotValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralCandidateSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> snapshotSequence{};
        overlay::PresenterWakeGate cameraOverlayWake;
        bool cameraOverlayVisible = true;
        bool cameraOverlayRefreshScheduled{};
        camera::CameraStatePublicationView publicationState{
            snapshot, snapshotValid, neutralCandidate, neutralSamples,
            neutralCandidateSource, zoomHold, snapshotSequence};
        const auto readSnapshot = [&] {
            std::lock_guard lock(snapshotMutex);
            return camera::ReadCameraStateSnapshotLocked(snapshot, snapshotValid);
        };
        CameraStateFreshnessSink freshnessSink{
            &snapshotMutex, &snapshot, &snapshotValid};
        settings.cameraStateFreshnessContext = &freshnessSink;
        settings.cameraSettingChanged = &CameraStateFreshnessSink::OnSettingChanged;
        const auto writer = [&](std::uintptr_t source, std::uintptr_t output,
            float inputFov) {
            fields.expectedSource = source;
            const plugin::CameraWriterInput input{
                source, output, inputFov, 3.55556f, true, 0x04, true,
                0.02f, settings.gameplayEnabled.load(std::memory_order_acquire),
                settings.gameplayMode.load(std::memory_order_acquire), 16.0f / 9.0f};
            auto policyInput = input;
            policyInput.currentViewportAspect = 3.55556f;
            policyInput.currentViewportAspectValid = true;
            auto gameplayView = gameplayState.View();
            auto writerGate = gate.Acquire();
            const auto result = plugin::ProcessCameraWriter(gate, writerState,
                policyInput, cinematicState, engineOperations, {}, &gameplayView);
            if (result.pairTransition.changed &&
                result.pairTransition.previous.source !=
                    result.pairTransition.current.source) {
                std::lock_guard lock(snapshotMutex);
                camera::MarkCameraWriterSourceChanged(snapshot, snapshotValid);
            }
            camera::CameraStatePublicationInput publication{};
        publication.zoomObserverAvailable = true;
        publication.zoomObservationValid = true;
            publication.observationPublished = result.gameplay.observationPublished;
            publication.observation = result.gameplay.observation;
            publication.source = result.gameplay.source;
            publication.applied = result.gameplay.applied;
            publication.presentation = camera::PresentationState::Gameplay;
            publication.dialogue = camera::DialogueState::Inactive;
            publication.modeTransitionPending = gameplayState.modeTransitionPending.load();
            publication.gameplayMode = static_cast<std::uint8_t>(input.gameplayMode);
            publication.nativeAspect = input.nativeAspect;
            publication.originalInputAvailable = true;
            publication.gameplayReferenceProvided = true;
            publication.gameplayReference = gameplayState.baseline.Read();
            {
                std::lock_guard lock(snapshotMutex);
                const auto published = camera::PublishCameraStateSnapshotLocked(
                    publicationState, publication);
                cameraOverlayRefreshScheduled = published.published &&
                    overlay::TryScheduleVisibleCameraStateRedraw(
                        cameraOverlayVisible, cameraOverlayWake);
            }
            return result;
        };

        bool pass = true;
        for (unsigned sample = 1; sample <= 3; ++sample) {
            const auto result = writer(sourceA, 0xA2, 90.6557f);
            const auto read = readSnapshot();
            pass &= Check(result.gameplay.observationPublished && result.mutateFov &&
                std::fabs(result.outputFov - 127.393f) < 0.01f &&
                read.valid && read.snapshot.source.gameplayWriter.value == sourceA &&
                read.snapshot.generation.eventSequence == sample &&
                 read.snapshot.evidence.nativeFov.valid &&
                 std::fabs(read.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
                read.snapshot.evidence.neutralGameplayFov.valid == (sample == 3),
                "F9 A writer publication accumulates retained FOV only on the third sample");
            pass &= Check(cameraOverlayRefreshScheduled &&
                cameraOverlayWake.pending(),
                "visible Camera State schedules from each published A writer sample");
            cameraOverlayWake.BeginHandling();
            pass &= Check(read.snapshot.freshness.writerObservation ==
                    camera::SnapshotFreshness::Current &&
                read.snapshot.freshness.transformedResult ==
                    camera::SnapshotFreshness::Current &&
                (sample == 3
                    ? read.snapshot.evidence.neutralGameplayFov.freshness ==
                        camera::SnapshotFreshness::Current
                    : read.snapshot.evidence.neutralGameplayFov.freshness ==
                        camera::SnapshotFreshness::Unavailable),
                "F9 initial A samples publish only facts with sufficient provenance");
        }
        const auto beforeOff = readSnapshot();
        plugin::OverlaySemanticSnapshot beforeOffSemantic{};
        beforeOffSemantic.cameraState = beforeOff.snapshot;
        beforeOffSemantic.cameraStateValid = beforeOff.valid;
        const auto beforeOffIntegration =
            overlay::ProjectCameraIntegration(beforeOffSemantic);
        pass &= Check(beforeOffIntegration.nativeFovValid &&
            beforeOffIntegration.horPlusTargetFovValid &&
            beforeOffIntegration.currentOutputFovValid &&
            beforeOffIntegration.gameplayFovValid,
            "F9 confirmed A writer facts are current before a freshness boundary");
        const auto off = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
        const auto afterOff = readSnapshot();
        pass &= Check(off.mutation.accepted && !settings.gameplayEnabled.load() &&
            afterOff.valid && afterOff.snapshot.generation.eventSequence ==
                beforeOff.snapshot.generation.eventSequence &&
             afterOff.snapshot.source.gameplayWriter.value == sourceA &&
             afterOff.snapshot.evidence.nativeFov.valid &&
             std::fabs(afterOff.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
             afterOff.snapshot.evidence.gameplayFov.valid &&
             std::fabs(afterOff.snapshot.evidence.gameplayFov.value - 90.6557f) < 0.001f &&
             afterOff.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current &&
             afterOff.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Historical &&
             afterOff.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "F9 Gameplay OFF preserves the previously published A camera state");

        const auto writerB = writer(sourceB, 0xB2, 94.0f);
        const auto afterB = readSnapshot();
        const auto latest = association.Read().latestWriter;
        plugin::OverlaySemanticSnapshot semantic{};
        semantic.gameplayEnabled = {false, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        semantic.cameraState = afterB.snapshot;
        semantic.cameraStateValid = afterB.valid;
        const auto integration = overlay::ProjectCameraIntegration(semantic);
        pass &= Check(writerB.processed && !writerB.mutateFov &&
            writerB.outputFov == 94.0f &&
            latest.source == sourceB && latest.output == 0xB2 &&
            !writerB.gameplay.observationPublished &&
            afterB.valid && afterB.snapshot.source.gameplayWriter.value == sourceA &&
            afterB.snapshot.generation.eventSequence ==
                beforeOff.snapshot.generation.eventSequence &&
            afterB.snapshot.evidence.nativeFov.valid &&
            std::fabs(afterB.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
            afterB.snapshot.evidence.gameplayFov.valid &&
            std::fabs(afterB.snapshot.evidence.gameplayFov.value - 90.6557f) < 0.001f &&
             afterB.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
             afterB.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Historical &&
             integration.nativeFovValid && integration.gameplayFovValid &&
             integration.horPlusTargetFovValid && integration.currentOutputFovValid &&
             integration.nativeFovFreshness == camera::SnapshotFreshness::Historical &&
             integration.gameplayFovFreshness == camera::SnapshotFreshness::Historical,
            "F9 AГ—3 в†’ OFF в†’ B projects retained and last-known role facts with freshness");
        pass &= Check(!cameraOverlayRefreshScheduled &&
            !cameraOverlayWake.pending(),
            "a writer callback without snapshot publication does not fabricate a refresh event");

        const auto on = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(true), true, [] {});
        const auto firstBPublication = writer(sourceB, 0xB2, 94.0f);
        const auto afterFirstB = readSnapshot();
        pass &= Check(on.mutation.accepted && firstBPublication.gameplay.observationPublished &&
            afterFirstB.snapshot.source.gameplayWriter.value == sourceB &&
            afterFirstB.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current &&
            afterFirstB.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Current &&
            afterFirstB.snapshot.evidence.neutralGameplayFov.source == sourceA &&
            afterFirstB.snapshot.evidence.neutralGameplayFov.freshness ==
                camera::SnapshotFreshness::Historical,
            "F9 B publication refreshes writer facts without rebinding retained A FOV");
        pass &= Check(cameraOverlayRefreshScheduled &&
            cameraOverlayWake.pending() &&
            readSnapshot().snapshot.source.gameplayWriter.value == sourceB,
            "post-transition B publication wakes visible Camera State with B facts");
        cameraOverlayWake.BeginHandling();
        const auto secondBPublication = writer(sourceB, 0xB2, 94.0f);
        const bool secondBRefreshScheduled = cameraOverlayRefreshScheduled;
        cameraOverlayWake.BeginHandling();
        const auto thirdBPublication = writer(sourceB, 0xB2, 94.0f);
        const auto afterStableB = readSnapshot();
        pass &= Check(secondBPublication.gameplay.observationPublished &&
            thirdBPublication.gameplay.observationPublished &&
            afterStableB.snapshot.evidence.neutralGameplayFov.source == sourceB &&
            afterStableB.snapshot.evidence.neutralGameplayFov.freshness ==
                camera::SnapshotFreshness::Current &&
            std::fabs(afterStableB.snapshot.evidence.neutralGameplayFov.value - 94.0f) < 0.001f,
            "F9 retained FOV becomes current only after source B's own neutral confirmation");
        pass &= Check(secondBRefreshScheduled && cameraOverlayRefreshScheduled &&
            cameraOverlayWake.pending(),
            "successive B publications keep the visible semantic panel current");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraPresentationTransition(snapshot, snapshotValid);
        }
        const auto afterPresentation = readSnapshot();
        pass &= Check(afterPresentation.snapshot.freshness.presentation ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "presentation boundary retires prior writer facts but retains Gameplay baseline");

        {
            std::lock_guard lock(snapshotMutex);
            camera::CameraFovTransactionPublication cinematicTransaction{};
            cinematicTransaction.nativeInput = 26.14f;
            cinematicTransaction.nativeInputValid = true;
            cinematicTransaction.horPlusTarget = 34.65f;
            cinematicTransaction.horPlusTargetValid = true;
            cinematicTransaction.output = 34.65f;
            cinematicTransaction.outputValid = true;
            cinematicTransaction.transactionOwner =
                camera::PresentationState::CinematicActive;
            cinematicTransaction.source = sourceB;
            cinematicTransaction.sequence = snapshot.generation.eventSequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                cinematicTransaction);
        }
        auto afterCinematicTransaction = readSnapshot();
        pass &= Check(afterCinematicTransaction.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            afterCinematicTransaction.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterCinematicTransaction.snapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterCinematicTransaction.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current,
            "Cinematic transaction refreshes live FOV roles without changing Gameplay baseline lifetime");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraDialogueTransition(snapshot, snapshotValid);
        }
        const auto afterDialogue = readSnapshot();
        pass &= Check(afterDialogue.snapshot.freshness.dialogue ==
                camera::SnapshotFreshness::Historical &&
            afterDialogue.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
            afterDialogue.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterDialogue.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current,
            "Dialogue lifecycle transition does not invalidate independent live FOV transaction facts");

        {
            std::lock_guard lock(snapshotMutex);
            camera::CameraFovTransactionPublication continuation{};
            continuation.nativeInput = 26.14f;
            continuation.nativeInputValid = true;
            continuation.horPlusTarget = 34.65f;
            continuation.horPlusTargetValid = true;
            continuation.output = 34.65f;
            continuation.outputValid = true;
            continuation.transactionOwner =
                camera::PresentationState::CinematicActive;
            continuation.source = sourceB;
            continuation.sequence = snapshot.generation.eventSequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                continuation);

            camera::CameraFovTransactionPublication recovery{};
            recovery.nativeInput = 30.0f;
            recovery.nativeInputValid = true;
            recovery.horPlusTarget = 34.65f;
            recovery.horPlusTargetValid = true;
            recovery.output = 31.2f;
            recovery.outputValid = true;
            recovery.transactionOwner =
                camera::PresentationState::CinematicExiting;
            recovery.source = sourceB;
            recovery.sequence = continuation.sequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                recovery);
        }
        const auto duringRecovery = readSnapshot();
        pass &= Check(duringRecovery.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            duringRecovery.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            duringRecovery.snapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Current &&
            duringRecovery.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current &&
            std::fabs(duringRecovery.snapshot.evidence.horPlusTargetFov.value -
                duringRecovery.snapshot.evidence.currentOutputFov.value) > 1.0f,
            "Cinematic continuation/recovery transactions keep target and actual output distinct");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraPresentationTransition(snapshot, snapshotValid);
        }
        const auto afterRecoveryBoundary = readSnapshot();
        pass &= Check(afterRecoveryBoundary.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterRecoveryBoundary.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterRecoveryBoundary.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "EXIT/recovery boundary retires Cinematic live facts without dropping retained Gameplay FOV");
        static_cast<void>(writer(sourceB, 0xB2, 94.0f));
        const auto afterGameplayHandoff = readSnapshot();
        pass &= Check(afterGameplayHandoff.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterGameplayHandoff.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterGameplayHandoff.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "first full Gameplay publication after recovery re-establishes live writer facts");

        camera::CameraStatePublicationInput zoomObservation{};
        zoomObservation.zoomObserverAvailable = true;
        zoomObservation.observationPublished =
            thirdBPublication.gameplay.observationPublished;
        zoomObservation.observation = thirdBPublication.gameplay.observation;
        zoomObservation.source = sourceB;
        zoomObservation.applied = thirdBPublication.gameplay.applied;
        zoomObservation.presentation = camera::PresentationState::Gameplay;
        zoomObservation.dialogue = camera::DialogueState::Inactive;
        zoomObservation.zoomActive = true;
        zoomObservation.zoomObservationValid = true;
        zoomObservation.zoomDirection = camera::ZoomDirection::In;
        zoomObservation.zoomSequence = 1;
        zoomObservation.zoomSource = sourceB;
        zoomObservation.zoomPrimary = 0.25f;
        zoomObservation.zoomSecondary = 0.75f;
        zoomObservation.gameplayMode = static_cast<std::uint8_t>(
            config::GameplayMode::HorPlus);
        zoomObservation.nativeAspect = 16.0f / 9.0f;
        {
            std::lock_guard lock(snapshotMutex);
            static_cast<void>(camera::PublishCameraStateSnapshotLocked(
                publicationState, zoomObservation));
        }
        const auto beforeZoomTransition = readSnapshot();
        pass &= Check(beforeZoomTransition.snapshot.freshness.zoom ==
                camera::SnapshotFreshness::Current,
            "F9 zoom event publication establishes current zoom provenance");
        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraZoomTransition(snapshot, snapshotValid);
        }
        const auto afterZoom = readSnapshot();
        pass &= Check(afterZoom.snapshot.freshness.zoom ==
                camera::SnapshotFreshness::Historical &&
            afterZoom.snapshot.evidence.neutralGameplayFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterZoom.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "F9 zoom transition before next writer marks zoom and retained FOV historical");

        camera::CameraStateSnapshot roleSnapshot{};
        roleSnapshot.presentation.state = camera::PresentationState::Gameplay;
        roleSnapshot.presentation.provenance =
            camera::EvidenceProvenance::ModDerivedState;
        roleSnapshot.evidence.gameplayFov = {90.0f, true,
            camera::EvidenceProvenance::NativeNumericObservation,
            camera::SnapshotFreshness::Current, sourceA, true, 10};
        roleSnapshot.evidence.gameplayReferenceUsable = true;
        bool roleSnapshotValid = false;
        camera::CameraFovTransactionPublication roles{};
        roles.nativeInput = 90.0f;
        roles.nativeInputValid = true;
        roles.horPlusTarget = 107.0f;
        roles.horPlusTargetValid = true;
        roles.output = 107.0f;
        roles.outputValid = true;
        roles.transactionOwner = camera::PresentationState::Gameplay;
        roles.source = sourceA;
        roles.sequence = 11;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        roles.transactionOwner = camera::PresentationState::CinematicActive;
        roles.nativeInput = 107.0f;
        roles.output = 107.0f;
        roles.sequence = 12;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(roleSnapshotValid && roleSnapshot.evidence.nativeFov.valid &&
            roleSnapshot.evidence.nativeFov.value == 107.0f &&
            roleSnapshot.evidence.horPlusTargetFov.valid &&
            roleSnapshot.evidence.currentOutputFov.value == 107.0f &&
            roleSnapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            roleSnapshot.presentation.state == camera::PresentationState::Gameplay,
            "cinematic_transaction_retains_gameplay_baseline_without_relabeling_last_full_publication_state");

        roles.nativeInput = 26.14f;
        roles.horPlusTargetValid = false;
        roles.output = 34.65f;
        roles.sequence = 13;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(roleSnapshot.evidence.nativeFov.value == 26.14f &&
            roleSnapshot.evidence.currentOutputFov.value == 34.65f &&
            roleSnapshot.evidence.horPlusTargetFov.value == 107.0f &&
            roleSnapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "Dialogue-like output remains distinct from retained HorPlus target");

        roles.nativeInput = std::numeric_limits<float>::quiet_NaN();
        roles.nativeInputValid = false;
        roles.output = std::numeric_limits<float>::quiet_NaN();
        roles.outputValid = false;
        roles.sequence = 14;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(roleSnapshot.evidence.nativeFov.freshness == camera::SnapshotFreshness::Historical &&
            !roleSnapshot.evidence.currentOutputFov.valid &&
            roleSnapshot.evidence.gameplayFov.valid &&
            roleSnapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "invalid transaction values fail closed without erasing retained Gameplay baseline");

        const auto beforeMode = readSnapshot();
        const auto modeChange = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::Gameplay(
                config::GameplayMode::AspectRecalculation), true, [] {});
        const auto afterMode = readSnapshot();
        pass &= Check(modeChange.mutation.accepted && afterMode.valid &&
            afterMode.snapshot.generation.eventSequence ==
                beforeMode.snapshot.generation.eventSequence &&
            afterMode.snapshot.freshness.gameplayMode ==
                camera::SnapshotFreshness::Historical &&
            afterMode.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Historical &&
            afterMode.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "F9 mode change without writer publication stales only dependent groups");
        return pass;
    }
#include "lifecycle_authority_parity_cases.hpp"
#include "lifecycle_authority_physical_cases.hpp"
#include "lifecycle_authority_cinematic_cases.hpp"
#include "lifecycle_authority_native_cinematic_cases.hpp"
#include "lifecycle_authority_transition_model.hpp"
#include "lifecycle_authority_callback_model.hpp"
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--authority-generated-events")
        return authority_transition_model::RunGenerated() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-transition-model")
        return authority_transition_model::Run() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-native-cinematic")
        return authority_native_cinematic::Run() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-cinematic")
        return authority_cinematic::Run() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-physical")
        return authority_physical::Run() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-parity")
        return authority_parity::Run() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--authority-bindings") {
        m15Verification = true;
        const bool productionPass = CheckRuntimeCombineProductionTransaction();
        const bool windowsPass = CheckM15ObservationWindows();
        const bool pass = productionPass && windowsPass && m15BindingChecksPass && m15FirstWriterChecked;
        std::cout << (pass ? "PASS" : "FAIL")
            << ": M15 binding counterexamples; readiness=NO-GO (expected divergences)\n";
        return pass ? 0 : 1;
    }
    using namespace camera;
    using namespace cinematics;

    std::atomic<CoordinatorState> coordinator{CoordinatorState::CinematicActive};
    CinematicWriterAssociation association;
    CinematicRecoveryOwner owner;
    dialogue::PostCinematicRecoveryExclusion exclusion;
    std::atomic<bool> handoffPending{true};
    std::atomic<std::uintptr_t> handoffSource{0xA};
    std::atomic<float> handoffFov{92.0f};
    std::atomic<bool> continuationPending{true};
    std::atomic<bool> continuationCarryPending{};
    std::atomic<std::uintptr_t> continuationSource{0xA};
    std::atomic<float> continuationExitTarget{127.0f};
    std::atomic<float> continuationPreviousTarget{126.0f};
    std::atomic<bool> continuationInterpolation{true};
    std::atomic<float> continuationStart{90.0f};
    std::atomic<std::uintptr_t> gameplayBaselineSource{0xA};
    std::atomic<float> exitTarget{90.0f};
    std::atomic<float> exitCinematicTarget{127.0f};
    GameplayWriterFixture gameplayState;
    FreshFieldsFixture freshFields{0xA};
    const plugin::CameraWriterEngineOperations freshReader{
        &freshFields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
    std::atomic<bool> cinematicEnabled{};
    std::atomic<std::uint64_t> cinematicSettingsEpoch{};
    cinematics::CinematicEnterActivation enterActivation;
    std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
        config::CinematicAspectPolicy::Auto};
    std::atomic<config::CinematicFovMode> cinematicFovMode{
        config::CinematicFovMode::GameplayHorPlus};
    std::atomic<bool> cinematicSelectionValid{};
    std::atomic<std::uint64_t> cinematicGeneration{};
    std::atomic<bool> cinematicTargetEstablished{};
    std::atomic<float> cinematicAuthoredFov{NAN};
    std::atomic<float> preEnterNativeFov{NAN};
    std::atomic<float> preEnterHorPlusFov{NAN};
    std::atomic<float> preEnterAspect{NAN};
    std::atomic<bool> preEnterPairValid{};
    std::atomic<float> cinematicTargetFov{NAN};
    std::atomic<std::uintptr_t> cinematicContinuationSource{};
    std::atomic<float> cinematicContinuationStart{NAN};
    std::atomic<bool> cinematicContinuationActive{};
    plugin::CameraCorrectionGate gate;
    plugin::CameraWriterRecoveryStateView state{coordinator, association, owner, exclusion,
        handoffPending, handoffSource, handoffFov, continuationPending,
        continuationCarryPending,
        continuationSource, continuationExitTarget, continuationPreviousTarget,
        continuationInterpolation, continuationStart, gameplayBaselineSource,
        exitTarget, exitCinematicTarget};
    plugin::CameraWriterCinematicStateView cinematicState{
        cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
        cinematicSettingsEpoch, cinematicSettingsEpoch,
        cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
        cinematicContinuationSource, cinematicContinuationStart,
        cinematicContinuationActive, association, enterActivation};
    plugin::CinematicLifecycleStateView cinematicLifecycleState{
        coordinator, cinematicSelectionValid, cinematicGeneration,
        cinematicEnabled, cinematicSettingsEpoch, cinematicAspectPolicy,
        cinematicFovMode, cinematicTargetEstablished, cinematicTargetFov,
        exitTarget, exitCinematicTarget, cinematicAuthoredFov,
        preEnterNativeFov, preEnterHorPlusFov, preEnterAspect,
        preEnterPairValid, gameplayBaselineSource, continuationPending,
        continuationCarryPending,
        continuationSource, continuationExitTarget, continuationPreviousTarget,
        continuationInterpolation, cinematicContinuationStart, association,
        owner, exclusion, handoffPending, handoffSource, handoffFov};

    bool pass = true;
    association.BeginGeneration(4, 90.0f, 127.0f,
        {true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus});
    association.ObserveAspectStoreTarget(4, 0xA);
    const auto admittedWriter = association.ObserveWriter(
        CoordinatorState::CinematicActive, 0xA, 0x10, 90.0f,
        3.55556f, true, 0x4, true);
    pass &= Check(association.AdmitWriterOutput(
            CoordinatorState::CinematicActive, true, 4, 0xA, 0x10,
            admittedWriter.latestWriter.sequence),
        "writer_invocation_is_admitted_for_recovery_capture");
    pass &= Check(owner.Capture(association.Read(), 4),
        "accepted lifecycle primitives establish owner A");
    exclusion.Arm(90.0f);
    const auto exit = gameplay::ResolveCinematicExitTransition(true, true,
        config::GameplayMode::HorPlus);
    coordinator.store(exit.nextState, std::memory_order_release);

    plugin::CameraWriterResult sameSource{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        sameSource = plugin::ProcessCameraWriter(gate, state,
            {0xA, 0x10, 92.0f, 3.55556f, true, 0x4, true}, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(sameSource.processed && !sameSource.recoveryRetired &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::CinematicExiting &&
        owner.Read().HasRecoveryAuthority(),
        "owner A keeps recovery on an A writer sample under correction gate");

    camera::CameraFovObservation handoffBaselineObservation{};
    handoffBaselineObservation.boundary = camera::FovObservationBoundary::CameraWriter;
    handoffBaselineObservation.inputFov = {90.0f, camera::FovSpace::Native,
        camera::FovProvenance::NativeRegisterInput, true};
    handoffBaselineObservation.resultFov = {90.0f, camera::FovSpace::Native,
        camera::FovProvenance::PassThroughResult, true};
    handoffBaselineObservation.aspect = {3.55556f,
        camera::FovProvenance::NativeWriterAspect, true};
    handoffBaselineObservation.writerSource = {0xA, true};
    handoffBaselineObservation.writerFlags = 0x04;
    handoffBaselineObservation.writerFlagsValid = true;
    handoffBaselineObservation.publicationSequence = 1;
    handoffBaselineObservation.observedDuringGameplay = true;
    static_cast<void>(gameplayState.baseline.Project(handoffBaselineObservation, true));

    dialogue::PostExitProviderHandoff handoffTracker;
    const dialogue::PostExitProviderActivation providerActivation{
        4, 1, 0x28, 0xA, 1234, 120.46f, 70.0f, 120.46f,
        config::DialogueZoomPolicy::Reduced};
    exitTarget.store(90.3115f, std::memory_order_release);
    exitCinematicTarget.store(126.87f, std::memory_order_release);
    handoffTracker.ObserveArmer(true, providerActivation);
    dialogue::PostExitProviderReject waitingReject{};
    const auto earlyWriterEvidence = handoffTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 120.46f, 0.001f, &waitingReject);
    pass &= Check(!earlyWriterEvidence.valid && handoffTracker.Active() &&
        waitingReject == dialogue::PostExitProviderReject::NoMatchedSetterSample,
        "writer before provider setter waits without discarding the activation");
    pass &= Check(handoffTracker.Active() &&
        handoffTracker.ObserveSetterSample(true, 0x28, 0xA, 1234, 4, 1,
            120.46f, 70.0f, 120.46f, 120.46f),
        "post-exit provider ownership requires validated activation and matching setter sample");
    auto providerWriterEvidence = handoffTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 120.46f);
    pass &= Check(providerWriterEvidence.valid,
        "handoff evidence is consumed only by same-source same-thread matching writer sample");

    plugin::CameraWriterInput providerHandoffInput{
        0xA, 0x10, 120.46f, 3.55556f, true, 0x04, true,
        0.02f, true, config::GameplayMode::RuntimeCombine, 16.0f / 9.0f};
    providerHandoffInput.currentViewportAspect = 3.55556f;
    providerHandoffInput.currentViewportAspectValid = true;
    providerHandoffInput.dialogueCorrectionEnabled = true;
    // The selected policy changes after provider activation. This handoff must
    // keep the policy captured by its validated activation record.
    providerHandoffInput.dialoguePolicy = config::DialogueZoomPolicy::Adaptive;
    providerHandoffInput.dialogueSettingsEpoch = 1;
    providerHandoffInput.threadId = 1234;
    providerHandoffInput.postExitProviderEvidence = providerWriterEvidence;
    plugin::CameraWriterResult providerHandoffResult{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        providerHandoffResult = plugin::ProcessCameraWriter(gate, state,
            providerHandoffInput, cinematicState, freshReader, {}, &gameplayView);
    }
    const float expectedDialogueNativeTarget = dialogue::ResolveDialogueDestinationTarget(
        config::DialogueZoomPolicy::Reduced, 90.0f, 70.0f);
    const float expectedDialogueTarget = camera::HorPlus(
        expectedDialogueNativeTarget, 3.55556f, 16.0f / 9.0f);
    const gameplay::HorPlusRecoverySample expectedRecoveryStart{
        0xA, 0xA, 120.46f, 90.3115f, 126.87f,
        3.55556f, 3.55556f, true, 0x04, true, true};
    const float expectedCorrectedStart =
        gameplay::ResolveHorPlusRecoveryInterpolationFov(
            expectedRecoveryStart, 16.0f / 9.0f);
    pass &= Check(providerHandoffResult.postExitProviderHandoffApplied &&
        providerHandoffResult.mutateFov &&
        std::fabs(providerHandoffResult.postExitProviderCorrectedStart -
            expectedCorrectedStart) < 0.01f &&
        std::fabs(providerHandoffResult.outputFov - expectedCorrectedStart) < 0.01f &&
        std::fabs(providerHandoffResult.outputFov -
            providerActivation.nativeContinuityStart) > 1.0f &&
        std::fabs(providerHandoffResult.postExitProviderHandoffTarget -
            expectedDialogueTarget) < 0.01f &&
        providerHandoffResult.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation &&
        std::fabs(providerHandoffResult.horPlusRecovery.outputFov -
            providerHandoffResult.outputFov) < 0.001f &&
        !providerHandoffResult.gameplay.runtimeCombineFovApplied &&
        providerHandoffResult.outputFov > expectedDialogueTarget &&
        providerHandoffResult.outputFov < 127.0f &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::CinematicExiting &&
        owner.Read().HasRecoveryAuthority() && exclusion.IsActive(),
        "EXIT-to-provider mapping preserves corrected recovery output at the native activation start");

    const auto providerContinuationEvidence = handoffTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 100.0f);
    auto providerContinuationInput = providerHandoffInput;
    providerContinuationInput.incomingFov = 100.0f;
    providerContinuationInput.postExitProviderEvidence = providerContinuationEvidence;
    plugin::CameraWriterResult providerContinuationResult{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        providerContinuationResult = plugin::ProcessCameraWriter(gate, state,
            providerContinuationInput, cinematicState, freshReader, {}, &gameplayView);
    }
    const float expectedContinuationOutput = dialogue::MapProjectionTransitionSample(
        100.0f, providerActivation.nativeStart, providerActivation.nativeTarget,
        expectedCorrectedStart, expectedDialogueTarget);
    pass &= Check(providerContinuationEvidence.valid &&
        providerContinuationResult.postExitProviderHandoffApplied &&
        providerContinuationResult.mutateFov &&
        std::fabs(providerContinuationResult.outputFov - expectedContinuationOutput) <
            0.01f &&
        providerContinuationResult.horPlusRecovery.action ==
            gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation,
        "provider correction owns every monotonic sample after exact activation pairing");

    const auto providerRecoveryCompletion =
        plugin::CompleteCameraWriterRecovery(state, 4);
    coordinator.store(CoordinatorState::Gameplay, std::memory_order_release);
    auto completedRecoveryProviderInput = providerHandoffInput;
    completedRecoveryProviderInput.incomingFov = 100.0f;
    completedRecoveryProviderInput.writerContext =
        camera::WriterContext::ProviderTransitionObserved;
    completedRecoveryProviderInput.postExitProviderEvidence =
        handoffTracker.TakeWriterSample(true, 0xA, 1234, 4, 1, 100.0f);
    plugin::CameraWriterResult completedRecoveryProviderResult{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        completedRecoveryProviderResult = plugin::ProcessCameraWriter(gate,
            state, completedRecoveryProviderInput, cinematicState, freshReader,
            {}, &gameplayView);
    }
    const float expectedCompletedRecoveryOutput =
        dialogue::MapProjectionTransitionSample(100.0f,
            providerActivation.nativeStart, providerActivation.nativeTarget,
            expectedCorrectedStart, expectedDialogueTarget);
    pass &= Check(providerRecoveryCompletion.completed &&
        providerRecoveryCompletion.exclusionActive && owner.Read().valid &&
        owner.Read().recoveryComplete && exclusion.IsActive() &&
        completedRecoveryProviderInput.postExitProviderEvidence.valid &&
        completedRecoveryProviderResult.postExitProviderHandoffApplied &&
        completedRecoveryProviderResult.mutateFov &&
        std::fabs(completedRecoveryProviderResult.outputFov -
            expectedCompletedRecoveryOutput) < 0.01f &&
        completedRecoveryProviderResult.outputFov !=
            gameplay::EvaluateHorPlus(100.0f, 3.55556f, 0x04,
                16.0f / 9.0f).outputFov,
        "validated provider handoff keeps the Dialogue destination after cinematic recovery completes");

    auto completedRecoveryProviderEndpoint = providerHandoffInput;
    completedRecoveryProviderEndpoint.incomingFov = 70.0f;
    completedRecoveryProviderEndpoint.writerContext =
        camera::WriterContext::ProviderTransitionObserved;
    completedRecoveryProviderEndpoint.postExitProviderEvidence =
        handoffTracker.TakeWriterSample(true, 0xA, 1234, 4, 1, 70.0f);
    plugin::CameraWriterResult completedRecoveryProviderEndpointResult{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        completedRecoveryProviderEndpointResult = plugin::ProcessCameraWriter(
            gate, state, completedRecoveryProviderEndpoint, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(completedRecoveryProviderEndpoint.postExitProviderEvidence.valid &&
        completedRecoveryProviderEndpointResult.postExitProviderHandoffApplied &&
        completedRecoveryProviderEndpointResult.postExitProviderRecoveryCompleted &&
        completedRecoveryProviderEndpointResult.recoveryRetired &&
        std::fabs(completedRecoveryProviderEndpointResult.outputFov -
            expectedDialogueTarget) < 0.01f && !owner.Read().valid &&
        !exclusion.IsActive() &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay,
        "provider trajectory reaches its Reduced endpoint and retires completed cinematic recovery");

    pass &= Check(owner.Capture(association.Read(), 4),
        "test cleanup restores the source-bound recovery owner");
    exitTarget.store(90.3115f, std::memory_order_release);
    exitCinematicTarget.store(126.87f, std::memory_order_release);
    exclusion.Arm(90.0f);
    coordinator.store(CoordinatorState::CinematicExiting,
        std::memory_order_release);

    const auto providerHandoffAdmitted = [&](plugin::CameraWriterInput candidate) {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        plugin::CameraWriterResult mapped{};
        return plugin::TryMapPostExitProviderDialogueSample(state, candidate,
            freshReader, &gameplayView, mapped);
    };
    auto staleProviderGeneration = providerHandoffInput;
    --staleProviderGeneration.postExitProviderEvidence.activation.generation;
    auto foreignProviderThread = providerHandoffInput;
    ++foreignProviderThread.threadId;
    auto missingProviderEvidence = providerHandoffInput;
    missingProviderEvidence.postExitProviderEvidence.valid = false;
    pass &= Check(!providerHandoffAdmitted(staleProviderGeneration) &&
        !providerHandoffAdmitted(foreignProviderThread) &&
        !providerHandoffAdmitted(missingProviderEvidence),
        "U2 provider mapper rejects stale generation, foreign thread, and missing evidence");

    bool providerPolicyEndpointsPass = true;
    for (const auto policy : {config::DialogueZoomPolicy::Disabled,
            config::DialogueZoomPolicy::Adaptive,
            config::DialogueZoomPolicy::Reduced}) {
        auto policyInput = providerHandoffInput;
        policyInput.postExitProviderEvidence.activation.dialoguePolicy = policy;
        plugin::CameraWriterResult policyResult{};
        bool mapped{};
        {
            auto writerGate = gate.Acquire();
            auto gameplayView = gameplayState.View();
            mapped = plugin::TryMapPostExitProviderDialogueSample(state,
                policyInput, freshReader, &gameplayView, policyResult);
        }
        const float policyNativeTarget = dialogue::ResolveDialogueDestinationTarget(
            policy, gameplayState.baseline.Read().nativeFov.value, 70.0f);
        const float expectedPolicyEndpoint = camera::HorPlus(
            policyNativeTarget, 3.55556f, 16.0f / 9.0f);
        providerPolicyEndpointsPass = providerPolicyEndpointsPass && mapped &&
            std::fabs(policyResult.postExitProviderHandoffTarget -
                expectedPolicyEndpoint) < 0.01f;
    }
    pass &= Check(providerPolicyEndpointsPass,
        "provider destination follows Disabled, Adaptive, and Reduced policy endpoints");
    auto nativeProviderInput = providerHandoffInput;
    nativeProviderInput.postExitProviderEvidence.activation.dialoguePolicy =
        config::DialogueZoomPolicy::Native;
    pass &= Check(!providerHandoffAdmitted(nativeProviderInput),
        "Native Dialogue policy leaves destination ownership to native/Gameplay routing");

    handoffTracker.ObserveArmer(true, providerActivation);
    dialogue::PostExitProviderReject setterReject{};
    pass &= Check(!handoffTracker.ObserveSetterSample(true, 0x28, 0xB, 1234,
            4, 1, 120.46f, 70.0f, 110.0f, 110.0f, 0.001f,
            &setterReject) && !handoffTracker.Active() &&
        setterReject == dialogue::PostExitProviderReject::ReceiverMismatch,
        "foreign receiver cannot inherit a confirmed provider transition");

    dialogue::PostExitProviderHandoff writerMismatchTracker;
    writerMismatchTracker.ObserveArmer(true, providerActivation);
    pass &= Check(writerMismatchTracker.ObserveSetterSample(true, 0x28, 0xA,
            1234, 4, 1, 120.46f, 70.0f, 110.0f, 110.0f),
        "writer mismatch diagnostic fixture establishes setter evidence");
    dialogue::PostExitProviderReject writerReject{};
    const auto rejectedWriter = writerMismatchTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 111.0f, 0.001f, &writerReject);
    pass &= Check(!rejectedWriter.valid && !writerMismatchTracker.Active() &&
        writerReject == dialogue::PostExitProviderReject::WriterSampleMismatch,
        "writer sample mismatch reports its exact bounded rejection reason");

    dialogue::PostExitProviderHandoff nonMonotonicTracker;
    nonMonotonicTracker.ObserveArmer(true, providerActivation);
    nonMonotonicTracker.ObserveSetterSample(true, 0x28, 0xA, 1234, 4, 1,
        120.46f, 70.0f, 120.46f, 120.46f);
    const auto sequenceStart = nonMonotonicTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 120.46f);
    const bool laterSetterObserved = nonMonotonicTracker.ObserveSetterSample(true,
        0x28, 0xA, 1234, 4, 1, 120.46f, 70.0f, 110.0f, 110.0f);
    const auto forwardSample = nonMonotonicTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 100.0f);
    dialogue::PostExitProviderReject sequenceReject{};
    const auto backwardSample = nonMonotonicTracker.TakeWriterSample(true,
        0xA, 1234, 4, 1, 105.0f, 0.001f, &sequenceReject);
    pass &= Check(sequenceStart.valid && laterSetterObserved && forwardSample.valid &&
        !backwardSample.valid && !nonMonotonicTracker.Active() &&
        sequenceReject == dialogue::PostExitProviderReject::WriterSampleMismatch,
        "provider trajectory cancels on a reversed native sample");
    pass &= Check(std::fabs(dialogue::MapProjectionTransitionSample(
            126.0f, 126.0f, 70.0f, 126.0f, expectedDialogueTarget) - 126.0f) < 0.001f &&
        std::fabs(dialogue::MapProjectionTransitionSample(
            70.0f, 126.0f, 70.0f, 126.0f, expectedDialogueTarget) -
                expectedDialogueTarget) < 0.001f &&
        std::isnan(dialogue::MapProjectionTransitionSample(
            40.0f, 126.0f, 70.0f, 126.0f, expectedDialogueTarget)),
        "provider transition mapping preserves endpoints and rejects out-of-path samples");

    plugin::CameraWriterResult foreign{};
    {
        freshFields.expectedSource = 0xB;
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        plugin::CameraWriterInput foreignInput{
            0xB, 0x20, 90.6557f, 3.55556f, true, 0x4, true};
        foreignInput.currentViewportAspect = 3.55556f;
        foreignInput.currentViewportAspectValid = true;
        foreign = plugin::ProcessCameraWriter(gate, state, foreignInput, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(foreign.recoveryRetired &&
        foreign.coordinatorAfter == CoordinatorState::Gameplay &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay,
        "A to B retires A authority and returns coordinator to Gameplay");
    pass &= Check(!owner.Read().valid && !exclusion.IsActive() &&
        !handoffPending.load() && handoffSource.load() == 0 &&
        std::isnan(handoffFov.load()) && !continuationPending.load() &&
        continuationSource.load() == 0 && std::isnan(continuationExitTarget.load()) &&
        std::isnan(continuationPreviousTarget.load()) &&
        !continuationInterpolation.load() && std::isnan(continuationStart.load()) &&
        gameplayBaselineSource.load() == 0 && std::isnan(exitTarget.load()) &&
        std::isnan(exitCinematicTarget.load()),
        "retirement clears every recovery-dependent field");
    pass &= Check(foreign.gameplay.handled && foreign.mutateFov &&
        std::fabs(foreign.outputFov - 127.393f) < 0.01f,
        "the same post-retirement writer transaction applies Gameplay HorPlus output");

    auto runGameplayWriter = [&](float fov, float aspect, std::uint8_t flags,
        bool fieldsReadable = true,
        config::GameplayMode mode = config::GameplayMode::HorPlus) {
        freshFields.expectedSource = 0xB;
        freshFields.aspect = aspect;
        freshFields.flags = flags;
        freshFields.readable = fieldsReadable;
        plugin::CameraWriterInput input{
            0xB, 0x20, fov, aspect, true, flags, true,
            0.02f, true, mode, 16.0f / 9.0f};
        input.currentViewportAspect = aspect;
        input.currentViewportAspectValid = true;
        auto gameplayView = gameplayState.View();
        auto writerGate = gate.Acquire();
        return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
            freshReader, {}, &gameplayView);
    };
    const auto horPlusWriter = runGameplayWriter(90.6557f, 3.55556f, 0x04);
    pass &= Check(horPlusWriter.gameplay.handled && horPlusWriter.gameplay.applied &&
        horPlusWriter.mutateFov &&
        std::fabs(horPlusWriter.outputFov - 127.393f) < 0.01f &&
        horPlusWriter.gameplay.observationPublished &&
        horPlusWriter.gameplay.baselineProjection.baseline.valid,
        "G01 production transaction returns HorPlus output and publishes observation/baseline");
    const auto nativeAspectWriter = runGameplayWriter(90.6557f, 16.0f / 9.0f, 0x04);
    const auto invalidFlagsWriter = runGameplayWriter(90.6557f, 3.55556f, 0x00);
    const auto invalidFovWriter = runGameplayWriter(180.0f, 3.55556f, 0x04);
    const auto unreadableWriter = runGameplayWriter(90.6557f, 3.55556f, 0x04, false);
    pass &= Check(!nativeAspectWriter.mutateFov &&
        nativeAspectWriter.outputFov == 90.6557f &&
        nativeAspectWriter.gameplay.reason == std::string_view("INPUT_BYPASS") &&
        !invalidFlagsWriter.mutateFov && invalidFlagsWriter.outputFov == 90.6557f &&
        !invalidFovWriter.mutateFov && invalidFovWriter.outputFov == 180.0f &&
        !invalidFovWriter.gameplay.observationPublished &&
        unreadableWriter.gameplay.reason == std::string_view("SOURCE_READ_FAILED") &&
        !unreadableWriter.mutateFov && unreadableWriter.outputFov == 90.6557f,
        "G02 native aspect, invalid flags/FOV and unreadable fields fail through production transaction");

    freshFields.expectedSource = 0xB;
    freshFields.calls = 0;
    freshFields.useAspectSequence = true;
    freshFields.aspectSequence = {16.0f / 9.0f, 3.55556f, 0.0f, 0.0f};
    freshFields.aspectFlagsWriteSucceeds = true;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.aspectRestoration.Update(3.55556f, {0xB, true});
    gameplayState.modeTransitionTarget.store(config::GameplayMode::HorPlus,
        std::memory_order_release);
    gameplayState.modeTransitionPending.store(true, std::memory_order_release);
    const auto restoreModeTransition = runGameplayWriter(90.6557f,
        3.55556f, 0x04);
    pass &= Check(restoreModeTransition.gameplay.modeTransition.event ==
            plugin::GameplayModeTransitionEvent::RestoreApplied &&
        restoreModeTransition.gameplay.modeTransitionPending == false &&
        !gameplayState.modeTransitionPending.load(std::memory_order_acquire) &&
        freshFields.calls == 2 && freshFields.aspectFlagsWriteCalls == 1 &&
        restoreModeTransition.mutateFov &&
        std::fabs(restoreModeTransition.outputFov - 127.393f) < 0.01f,
        "mode transition restores native aspect before the same transaction resolves HorPlus output");

    freshFields.calls = 0;
    freshFields.aspectSequence = {16.0f / 9.0f, 16.0f / 9.0f, 0.0f, 0.0f};
    freshFields.aspectFlagsWriteSucceeds = false;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.modeTransitionPending.store(true, std::memory_order_release);
    const auto failedModeWrite = runGameplayWriter(90.6557f,
        16.0f / 9.0f, 0x04);
    pass &= Check(failedModeWrite.gameplay.modeTransition.event ==
            plugin::GameplayModeTransitionEvent::RestoreWriteFailed &&
        failedModeWrite.gameplay.modeTransitionPending &&
        gameplayState.modeTransitionPending.load(std::memory_order_acquire) &&
        freshFields.calls == 2 && freshFields.aspectFlagsWriteCalls == 1 &&
        !failedModeWrite.mutateFov && failedModeWrite.outputFov == 90.6557f,
        "failed native restoration write preserves pending mode transition and native output");
    gameplayState.modeTransitionPending.store(false, std::memory_order_release);
    freshFields.useAspectSequence = false;

    freshFields.aspectFlagsWriteSucceeds = true;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.replayState.store(gameplay::ReplayState::Complete,
        std::memory_order_release);
    const auto constrainedPass = runGameplayWriter(90.6557f, 2.33333f, 0x5,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(constrainedPass.gameplay.handled &&
        constrainedPass.gameplay.constrainedAspectPass &&
        constrainedPass.gameplay.nativeAspectWriteSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        !constrainedPass.mutateFov && constrainedPass.outputFov == 90.6557f,
        "AspectRecalculation constrained pass requests native aspect/flags write and preserves FOV");

    gameplayState.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
        std::memory_order_release);
    freshFields.aspectFlagsWriteCalls = 0;
    const auto automaticRestore = runGameplayWriter(90.6557f, 3.55556f, 0x4,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(automaticRestore.gameplay.nativeAspectWriteAttempted &&
        automaticRestore.gameplay.nativeAspectWriteSucceeded &&
        gameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::Complete &&
        gameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0xB &&
        freshFields.aspectFlagsWriteCalls == 1 && !automaticRestore.mutateFov &&
        automaticRestore.outputFov == 90.6557f,
        "AspectRecalculation automatic native restore commits state only after successful engine write");

    gameplayState.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
        std::memory_order_release);
    freshFields.aspectFlagsWriteSucceeds = false;
    freshFields.aspectFlagsWriteCalls = 0;
    const auto failedAutomaticRestore = runGameplayWriter(90.6557f, 3.55556f, 0x4,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(failedAutomaticRestore.gameplay.nativeAspectWriteAttempted &&
        !failedAutomaticRestore.gameplay.nativeAspectWriteSucceeded &&
        gameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::WaitingForAutomaticUpdate &&
        freshFields.aspectFlagsWriteCalls == 1 && !failedAutomaticRestore.mutateFov,
        "failed AspectRecalculation write leaves AutomaticUpdate pending for retry");
    freshFields.aspectFlagsWriteSucceeds = true;
    GameplayWriterFixture successfulHandoffGameplayState;
    GameplayWriterFixture failedHandoffGameplayState;
    GameplayWriterFixture endpointGameplayState;
    auto prepareAtomicHandoff = [&](bool gameplayEnabledAtExit,
        GameplayWriterFixture& scenarioGameplayState) {
        const auto selection = plugin::PrepareCinematicEnter(cinematicLifecycleState,
            config::CinematicAspectPolicy::Forced21x9,
            config::CinematicFovMode::GameplayHorPlus);
        plugin::AcceptedCinematicEnterInput enterInput{};
        enterInput.selection = selection;
        enterInput.correctionEnabled = true;
        enterInput.settingsEpoch = 1;
        enterInput.authoredFov = 90.6557f;
        enterInput.effectiveAspect = 3.55556f;
        enterInput.nativeAspect = 16.0f / 9.0f;
        const auto enter = plugin::ProcessAcceptedCinematicEnter(
            cinematicLifecycleState, enterInput);
        pass &= Check(enter.accepted &&
            coordinator.load(std::memory_order_acquire) ==
                CoordinatorState::CinematicActive &&
            cinematicSelectionValid.load(std::memory_order_acquire) &&
            cinematicGeneration.load(std::memory_order_acquire) == enter.generation,
            "atomic handoff setup obtains active generation through production ENTER event");
        const auto aspectStore = plugin::ObserveCinematicAspectStore(
            cinematicLifecycleState, {0xA, true, true, false, true, true,
                16.0f / 9.0f});
        pass &= Check(aspectStore.accepted &&
            association.Read().aspectStoreTargetObject == 0xA,
            "atomic handoff setup obtains target relation through production aspect-store event");
        freshFields.expectedSource = 0xA;
        freshFields.aspect = 3.55556f;
        freshFields.flags = 0x4;
        freshFields.readable = true;
        freshFields.aspectFlagsWriteCalls = 0;
        const plugin::CameraWriterInput cinematicInput{
            0xA, 0x10, 90.6557f, 3.55556f, true, 0x4, true,
            0.02f, true, config::GameplayMode::AspectRecalculation,
            16.0f / 9.0f};
        auto cinematicGameplayView = scenarioGameplayState.View();
        auto cinematicGate = gate.Acquire();
        const auto cinematicWriter = plugin::ProcessCameraWriter(gate, state,
            cinematicInput, cinematicState, freshReader, {},
            &cinematicGameplayView);
        pass &= Check(cinematicWriter.cinematicHandled &&
            association.Read().associationValid &&
            association.Read().associatedWriterRsi == 0xA,
            "atomic handoff setup associates writer through production writer transaction");
        const plugin::AcceptedCinematicExitInput exitInput{
            coordinator.load(std::memory_order_acquire),
            CoordinatorState::CinematicExiting, enter.generation, 90.6557f,
            gameplayEnabledAtExit, config::GameplayMode::AspectRecalculation,
            0xA, cinematicWriter.outputFov};
        const auto exit = plugin::ProcessAcceptedCinematicExit(
            cinematicLifecycleState, exitInput);
        pass &= Check(exit.accepted && exit.recoveryOwnerCaptured &&
            coordinator.load(std::memory_order_acquire) ==
                CoordinatorState::CinematicExiting &&
            owner.Read().HasRecoveryAuthority() &&
            handoffPending.load(std::memory_order_acquire) == gameplayEnabledAtExit,
            "atomic handoff setup obtains recovery owner and pending state through production EXIT event");
        freshFields.expectedSource = 0xA;
        freshFields.aspect = 3.55556f;
        freshFields.flags = 0x4;
        freshFields.readable = true;
    };
    auto runAtomicHandoffWriter = [&](float fov,
        GameplayWriterFixture& scenarioGameplayState) {
        auto gameplayView = scenarioGameplayState.View();
        const plugin::CameraWriterInput input{
            0xA, 0x10, fov, 3.55556f, true, 0x4, true,
            0.02f, true, config::GameplayMode::AspectRecalculation, 16.0f / 9.0f};
        auto writerGate = gate.Acquire();
        return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
            freshReader, {}, &gameplayView);
    };
    prepareAtomicHandoff(true, successfulHandoffGameplayState);
    const auto handoffFirstSample = runAtomicHandoffWriter(126.0f,
        successfulHandoffGameplayState);
    const auto handoffApplied = runAtomicHandoffWriter(120.0f,
        successfulHandoffGameplayState);
    pass &= Check(handoffFirstSample.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::FirstSample &&
        handoffApplied.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::Applied &&
        handoffApplied.atomicExitHandoff.writeSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        successfulHandoffGameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::Complete &&
        successfulHandoffGameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0xA,
        "event-only ENTER/EXIT then writer transaction applies atomic handoff and commits replay state after native write");

    prepareAtomicHandoff(true, failedHandoffGameplayState);
    freshFields.aspectFlagsWriteSucceeds = false;
    static_cast<void>(runAtomicHandoffWriter(126.0f,
        failedHandoffGameplayState));
    const auto handoffWriteFailed = runAtomicHandoffWriter(120.0f,
        failedHandoffGameplayState);
    pass &= Check(handoffWriteFailed.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::WriteFailed &&
        !handoffWriteFailed.atomicExitHandoff.writeSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        failedHandoffGameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::WaitingForAutomaticUpdate &&
        failedHandoffGameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0,
        "failed atomic EXIT handoff write does not commit Gameplay replay state");
    freshFields.aspectFlagsWriteSucceeds = true;

    prepareAtomicHandoff(false, endpointGameplayState);
    const plugin::CameraWriterInput aspectEndpointInput{
        0xA, 0x10, 90.6557f, 3.55556f, true, 0x4, true,
        0.02f, false, config::GameplayMode::AspectRecalculation, 16.0f / 9.0f};
    plugin::CameraWriterResult aspectEndpoint{};
    {
        auto gameplayView = endpointGameplayState.View();
        auto writerGate = gate.Acquire();
        aspectEndpoint = plugin::ProcessCameraWriter(gate, state,
            aspectEndpointInput, cinematicState, freshReader, {}, &gameplayView);
    }
    pass &= Check(aspectEndpoint.aspectRecoveryEndpoint &&
        aspectEndpoint.horPlusRecovery.completed &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay &&
        !owner.Read().HasRecoveryAuthority() && !aspectEndpoint.mutateFov &&
        aspectEndpoint.outputFov == 90.6557f,
        "AspectRecalculation recovery endpoint completes owner and returns native FOV with Gameplay OFF");
    pass &= Check(CheckWriterOffOrder(true),
        "writer-first commits shared HorPlus output before OFF linearizes");
    pass &= Check(CheckWriterOffOrder(false),
        "OFF-first reaches shared writer policy as pass-through");
    pass &= Check(CheckGameplayPolicyUsesValidatedViewportAspect(),
        "Gameplay HorPlus production transaction uses validated viewport policy aspect independently of native writer aspect");
    pass &= Check(CheckRuntimeCombineProductionTransaction(),
        "RuntimeCombine production writer transaction confirms aspect synchronization before HorPlus");
    pass &= Check(CheckSeededWriterSequenceAndDiagnosticParity(),
        "seeded writer sequence covers receiver/FOV/aspect/config changes with Diagnostics parity");
    pass &= Check(CheckCameraStatePublicationAfterWriterChange(),
        "F9 production writer and shared snapshot publisher characterize stale Overlay state");

    EngineOperationFixture effectFixture{};
    const plugin::CameraWriterEngineOperations engineOperations{
        &effectFixture, &RecordEngineRead, &RecordAspectOnlyWrite,
        &RecordAspectFlagsWrite};
    const auto aspectOnlyFields = plugin::ReadCameraWriterAspectFlags(
        engineOperations, 0xA, false);
    const bool aspectOnlySucceeded = plugin::RequestCameraWriterAspectOnly(
        engineOperations, 0xA, 16.0f / 9.0f);
    const bool aspectFlagsSucceeded = plugin::RequestCameraWriterAspectAndFlags(
        engineOperations, 0xA, 3.55556f, 0x04);
    pass &= Check(effectFixture.count == 3 &&
        effectFixture.operations[0] == 2u && effectFixture.operations[1] == 3u &&
        effectFixture.operations[2] == 4u && aspectOnlyFields.aspectReadable &&
        !aspectOnlyFields.flagsReadable && aspectOnlySucceeded &&
        !aspectFlagsSucceeded,
        "engine operation boundary preserves read/write order and explicit success/failure outcomes");

    coordinator.store(CoordinatorState::CinematicActive, std::memory_order_release);
    cinematicEnabled.store(true, std::memory_order_release);
    cinematicSelectionValid.store(true, std::memory_order_release);
    cinematicGeneration.store(99, std::memory_order_release);
    cinematicSettingsEpoch.store(10, std::memory_order_release);
    cinematicTargetEstablished.store(true, std::memory_order_release);
    cinematicAuthoredFov.store(90.0f, std::memory_order_release);
    cinematicTargetFov.store(126.87f, std::memory_order_release);
    cinematicContinuationSource.store(0, std::memory_order_release);
    cinematicContinuationActive.store(false, std::memory_order_release);
    continuationPending.store(false, std::memory_order_release);
    continuationCarryPending.store(false, std::memory_order_release);
    owner.Clear();
    exclusion.Reset();
    association.BeginGeneration(99, 90.0f, 126.87f,
        {true, config::CinematicAspectPolicy::Forced21x9,
            config::CinematicFovMode::GameplayHorPlus}, 90.3115f, 0xA);
    association.ObserveAspectStoreTarget(99, 0xA);
    enterActivation.Arm(99, 10, 0xA, 90.3115f, 127.119f, true, 126.87f);
    enterActivation.Capture(true, 99, 10, 0xC, 0xA, 77,
        90.3115f, 126.87f);
    plugin::CameraWriterResult mappedEnterSample{};
    {
        auto gameplayView = gameplayState.View();
        auto writerGate = gate.Acquire();
        mappedEnterSample = plugin::ProcessCameraWriter(gate, state,
            {0xA, 0x10, (90.3115f + 126.87f) * 0.5f,
                3.55556f, true, 0x04, true}, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(mappedEnterSample.cinematicOutputAdmitted &&
        mappedEnterSample.enterActivationMappingApplied &&
        mappedEnterSample.cinematicAction ==
            CinematicWriterAction::RetainMappedActivationSample &&
        mappedEnterSample.mutateFov &&
        std::fabs(mappedEnterSample.outputFov - (127.119f + 126.87f) * 0.5f) < 0.0001f,
        "camera_writer_transaction_maps_only_current_activation_owned_sample");

    // A receiver replacement must never let a provider activation reuse the
    // previous receiver's baseline. Once a trusted baseline for the new
    // receiver exists, the same activation may be mapped using that baseline.
    constexpr std::uintptr_t oldDialogueReceiver = 0xA;
    constexpr std::uintptr_t newDialogueReceiver = 0xB;
    constexpr std::uint64_t replacementGeneration = 100;
    constexpr float newGameplayNativeFov = 130.3115f;
    coordinator.store(CoordinatorState::Gameplay, std::memory_order_release);
    exclusion.Reset();
    owner.Clear();
    static_cast<void>(gameplayState.baseline.Invalidate());
    freshFields.expectedSource = oldDialogueReceiver;
    freshFields.aspect = 21.0f / 9.0f;
    freshFields.flags = 0x04;
    freshFields.readable = true;
    const auto writeGameplaySample = [&](std::uintptr_t receiver, float fov,
        camera::WriterContext context, float aspect) {
        freshFields.expectedSource = receiver;
        freshFields.aspect = aspect;
        plugin::CameraWriterInput input{receiver, 0x20, fov, aspect,
            true, 0x04, true, 0.02f, true, config::GameplayMode::RuntimeCombine,
            16.0f / 9.0f};
        input.currentViewportAspect = aspect;
        input.currentViewportAspectValid = true;
        input.writerContext = context;
        auto gameplayView = gameplayState.View();
        auto writerGate = gate.Acquire();
        return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
            freshReader, {}, &gameplayView);
    };
    const auto oldReceiverGameplay = writeGameplaySample(oldDialogueReceiver,
        90.3115f, camera::WriterContext::GameplayEligible, 32.0f / 9.0f);
    pass &= Check(oldReceiverGameplay.gameplay.baselineProjection.baseline.source.value ==
            oldDialogueReceiver && oldReceiverGameplay.gameplay.baselineProjection.baseline.current,
        "cross_mode_replay_seeds_receiver_a_through_production_gameplay_writer");
    const auto replacementGameplay = writeGameplaySample(newDialogueReceiver,
        newGameplayNativeFov, camera::WriterContext::TransitionAmbiguous,
        21.0f / 9.0f);
    const float expectedReplacementGameplayOutput = gameplay::EvaluateHorPlus(
        newGameplayNativeFov, 21.0f / 9.0f, 0x04, 16.0f / 9.0f).outputFov;
    pass &= Check(replacementGameplay.mutateFov &&
        std::fabs(replacementGameplay.outputFov - expectedReplacementGameplayOutput) < 0.02f &&
        replacementGameplay.gameplay.baselineProjection.baseline.source.value ==
            oldDialogueReceiver &&
        !camera::IsUsableGameplayBaseline(
            replacementGameplay.gameplay.baselineProjection.baseline),
        "cross_mode_replay_keeps_horplus_but_retires_receiver_a_baseline_on_b_sample");
    const auto cinematicAfterReplacement = cinematics::SelectCinematicBaseline(
        true, 100.0f,
        replacementGameplay.gameplay.baselineProjection.baseline);
    pass &= Check(!cinematicAfterReplacement.usedGameplayBaselineValue &&
        cinematicAfterReplacement.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::AuthoredEnter &&
        cinematicAfterReplacement.authoredReferenceFov == 100.0f,
        "cross_mode_replay_cinematic_uses_authored_fallback_without_current_receiver_baseline");

    coordinator.store(CoordinatorState::CinematicActive, std::memory_order_release);
    cinematicEnabled.store(true, std::memory_order_release);
    cinematicSelectionValid.store(true, std::memory_order_release);
    cinematicGeneration.store(replacementGeneration, std::memory_order_release);
    cinematicSettingsEpoch.store(10, std::memory_order_release);
    cinematicTargetEstablished.store(true, std::memory_order_release);
    cinematicAuthoredFov.store(90.0f, std::memory_order_release);
    cinematicTargetFov.store(expectedReplacementGameplayOutput, std::memory_order_release);
    association.BeginGeneration(replacementGeneration, 90.0f,
        expectedReplacementGameplayOutput,
        {true, config::CinematicAspectPolicy::Forced21x9,
            config::CinematicFovMode::GameplayHorPlus}, 90.3115f,
        oldDialogueReceiver);
    association.ObserveAspectStoreTarget(replacementGeneration,
        newDialogueReceiver);
    const auto newReceiverWriter = association.ObserveWriter(
        CoordinatorState::CinematicActive, newDialogueReceiver, 0x20,
        126.87f, 21.0f / 9.0f, true, 0x04, true);
    pass &= Check(association.AdmitWriterOutput(CoordinatorState::CinematicActive,
            true, replacementGeneration, newDialogueReceiver, 0x20,
            newReceiverWriter.latestWriter.sequence),
        "replacement_receiver_establishes_its_own_cinematic_writer_owner");
    owner.Clear();
    exclusion.Reset();
    const auto replacementExit = plugin::ProcessAcceptedCinematicExit(
        cinematicLifecycleState, {CoordinatorState::CinematicActive,
            CoordinatorState::CinematicExiting, replacementGeneration, 126.87f,
            true, config::GameplayMode::RuntimeCombine, 0, NAN});
    pass &= Check(replacementExit.accepted && replacementExit.recoveryOwnerCaptured &&
        owner.Read().writerRsi == newDialogueReceiver && exclusion.IsActive(),
        "replacement_receiver_cinematic_exit_captures_matching_recovery_owner");

    freshFields.expectedSource = newDialogueReceiver;
    freshFields.aspect = 21.0f / 9.0f;
    freshFields.flags = 0x04;
    freshFields.readable = true;
    plugin::CameraWriterInput replacementProviderInput{
        newDialogueReceiver, 0x20, 126.87f, 21.0f / 9.0f, true, 0x04, true,
        0.02f, true, config::GameplayMode::RuntimeCombine, 16.0f / 9.0f};
    replacementProviderInput.currentViewportAspect = 21.0f / 9.0f;
    replacementProviderInput.currentViewportAspectValid = true;
    replacementProviderInput.dialogueCorrectionEnabled = true;
    replacementProviderInput.dialogueSettingsEpoch = 1;
    replacementProviderInput.threadId = 1234;
    replacementProviderInput.writerContext = camera::WriterContext::TransitionAmbiguous;
    dialogue::PostExitProviderHandoff replacementHandoff;
    const dialogue::PostExitProviderActivation replacementActivation{
        replacementGeneration, 1, 0x28, newDialogueReceiver, 1234,
        126.87f, 70.0f, 126.87f, config::DialogueZoomPolicy::Reduced};
    dialogue::PostExitProviderReject replacementReject{};
    const bool replacementArmed = replacementHandoff.ObserveArmer(
        replacementExit.recoveryOwnerCaptured, replacementActivation,
        &replacementReject);
    const bool replacementSetterMatched = replacementHandoff.ObserveSetterSample(
        replacementExit.recoveryOwnerCaptured, 0x28, newDialogueReceiver,
        1234, replacementGeneration, 1, 126.87f, 70.0f, 126.87f, 126.87f,
        0.001f, &replacementReject);
    replacementProviderInput.postExitProviderEvidence =
        replacementHandoff.TakeWriterSample(true, newDialogueReceiver, 1234,
            replacementGeneration, 1, 126.87f, 0.001f, &replacementReject);
    plugin::CameraWriterResult staleReceiverProvider{};
    bool staleReceiverMapped{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        staleReceiverMapped = plugin::TryMapPostExitProviderDialogueSample(
            state, replacementProviderInput, freshReader, &gameplayView,
            staleReceiverProvider);
    }
    pass &= Check(replacementArmed && replacementSetterMatched &&
        replacementProviderInput.postExitProviderEvidence.valid &&
        !staleReceiverMapped &&
        !staleReceiverProvider.postExitProviderHandoffApplied,
        "replacement_armer_setter_writer_rejects_old_receiver_gameplay_baseline");
    const float reducedWithoutReceiverBaseline =
        dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Reduced,
            std::numeric_limits<float>::quiet_NaN(), 70.0f);
    pass &= Check(!camera::IsUsableGameplayBaseline(gameplayState.baseline.Read()) &&
        gameplayState.baseline.Read().source.value == oldDialogueReceiver &&
        !std::isfinite(reducedWithoutReceiverBaseline) &&
        !staleReceiverProvider.postExitProviderHandoffApplied,
        "dialogue_reduced_fails_closed_without_fresh_receiver_baseline");

    camera::CameraFovObservation newReceiverObservation{};
    newReceiverObservation.boundary = camera::FovObservationBoundary::CameraWriter;
    newReceiverObservation.inputFov = {newGameplayNativeFov,
        camera::FovSpace::Native, camera::FovProvenance::NativeRegisterInput, true};
    newReceiverObservation.resultFov = {expectedReplacementGameplayOutput,
        camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, true};
    newReceiverObservation.aspect = {21.0f / 9.0f,
        camera::FovProvenance::ResolvedAspect, true};
    newReceiverObservation.writerSource = {newDialogueReceiver, true};
    newReceiverObservation.publicationSequence = 501;
    static_cast<void>(gameplayState.baseline.Project(newReceiverObservation, true));
    plugin::CameraWriterResult currentReceiverProvider{};
    bool currentReceiverMapped{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        currentReceiverMapped = plugin::TryMapPostExitProviderDialogueSample(
            state, replacementProviderInput, freshReader, &gameplayView,
            currentReceiverProvider);
    }
    const float independentCurrentDialogueTarget = dialogue::ReducedTarget(
        newGameplayNativeFov);
    const float expectedReplacementProviderTarget = camera::HorPlus(
        independentCurrentDialogueTarget, 21.0f / 9.0f, 16.0f / 9.0f);
    pass &= Check(currentReceiverMapped &&
        currentReceiverProvider.postExitProviderHandoffApplied &&
        std::fabs(currentReceiverProvider.postExitProviderHandoffTarget -
            expectedReplacementProviderTarget) < 0.02f,
        "replacement_provider_uses_fresh_receiver_baseline_after_admission");

    bool replacementProviderMatrixPass = true;
    constexpr float providerAspects[] = {16.0f / 9.0f, 21.0f / 9.0f,
        32.0f / 9.0f};
    for (const float providerAspect : providerAspects) {
        for (const auto mode : {config::GameplayMode::HorPlus,
                config::GameplayMode::RuntimeCombine}) {
            freshFields.aspect = providerAspect;
            for (const auto policy : {config::DialogueZoomPolicy::Disabled,
                    config::DialogueZoomPolicy::Adaptive,
                    config::DialogueZoomPolicy::Reduced}) {
                auto matrixInput = replacementProviderInput;
                matrixInput.aspect = providerAspect;
                matrixInput.currentViewportAspect = providerAspect;
                matrixInput.currentViewportAspectValid = true;
                matrixInput.gameplayMode = mode;
                matrixInput.postExitProviderEvidence.activation.dialoguePolicy = policy;
                plugin::CameraWriterResult matrixResult{};
                bool matrixMapped{};
                {
                    auto writerGate = gate.Acquire();
                    auto gameplayView = gameplayState.View();
                    matrixMapped = plugin::TryMapPostExitProviderDialogueSample(
                        state, matrixInput, freshReader, &gameplayView, matrixResult);
                }
                const float nativeDestination =
                    dialogue::ResolveDialogueDestinationTarget(policy,
                        newGameplayNativeFov, 70.0f);
                const float expectedDestination = gameplay::EvaluateHorPlus(
                    nativeDestination, providerAspect, 0x04, 16.0f / 9.0f).outputFov;
                const bool expectsHorPlusMapping = providerAspect > 16.0f / 9.0f;
                const bool matrixRowPass = expectsHorPlusMapping
                    ? (matrixMapped && matrixResult.postExitProviderHandoffApplied &&
                        std::fabs(matrixResult.postExitProviderHandoffTarget -
                            expectedDestination) < 0.02f)
                    : (!matrixMapped &&
                        !matrixResult.postExitProviderHandoffApplied);
                replacementProviderMatrixPass = replacementProviderMatrixPass &&
                    matrixRowPass &&
                    camera::IsUsableGameplayBaseline(gameplayState.baseline.Read()) &&
                    gameplayState.baseline.Read().source.value == newDialogueReceiver;
                if (!matrixRowPass) {
                    std::cerr << "FAIL: provider_matrix aspect=" << providerAspect
                        << " gameplay_mode=" << static_cast<unsigned>(mode)
                        << " dialogue_policy=" << static_cast<unsigned>(policy)
                        << " mapped=" << matrixMapped
                        << " applied=" << matrixResult.postExitProviderHandoffApplied
                        << " target=" << matrixResult.postExitProviderHandoffTarget
                        << " expected=" << expectedDestination << '\n';
                }
            }
        }
    }
    pass &= Check(replacementProviderMatrixPass,
        "receiver replacement provider matrix covers Dialogue policies, Gameplay modes, and aspects");

    // Finish the same provider trajectory at its native Dialogue endpoint, let
    // the production recovery cleanup return ownership to Gameplay, then run a
    // second accepted Cinematic ENTER and writer transaction on receiver B.
    const bool replacementEndpointSetterMatched =
        replacementHandoff.ObserveSetterSample(true, 0x28,
            newDialogueReceiver, 1234, replacementGeneration, 1,
            126.87f, 70.0f, 70.0f, 70.0f, 0.001f, &replacementReject);
    auto endpointProviderInput = replacementProviderInput;
    endpointProviderInput.incomingFov = 70.0f;
    endpointProviderInput.aspect = 32.0f / 9.0f;
    endpointProviderInput.currentViewportAspect = 32.0f / 9.0f;
    freshFields.aspect = 32.0f / 9.0f;
    endpointProviderInput.postExitProviderEvidence =
        replacementHandoff.TakeWriterSample(true, newDialogueReceiver, 1234,
            replacementGeneration, 1, 70.0f, 0.001f, &replacementReject);
    plugin::CameraWriterResult dialogueEndpoint{};
    bool dialogueEndpointMapped{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        dialogueEndpointMapped = plugin::TryMapPostExitProviderDialogueSample(
            state, endpointProviderInput, freshReader, &gameplayView,
            dialogueEndpoint);
    }
    const float expectedEndpointAtChangedAspect = gameplay::EvaluateHorPlus(
        dialogue::ReducedTarget(newGameplayNativeFov), 32.0f / 9.0f, 0x04,
        16.0f / 9.0f).outputFov;
    pass &= Check(replacementEndpointSetterMatched &&
        endpointProviderInput.postExitProviderEvidence.valid &&
        dialogueEndpointMapped && dialogueEndpoint.postExitProviderRecoveryCompleted &&
        dialogueEndpoint.recoveryRetired &&
        std::fabs(dialogueEndpoint.outputFov - expectedEndpointAtChangedAspect) < 0.03f,
        "cross_mode_replay_dialogue_reduced_reaches_endpoint_on_receiver_b");
    if (dialogueEndpoint.postExitProviderRecoveryCompleted) {
        static_cast<void>(plugin::ClearCameraWriterRecoveryState(state));
        replacementHandoff.CompleteRecovery();
    }

    const auto secondEnterSelection = plugin::PrepareCinematicEnter(
        cinematicLifecycleState, config::CinematicAspectPolicy::Auto,
        config::CinematicFovMode::GameplayHorPlus);
    plugin::AcceptedCinematicEnterInput secondEnterInput{};
    secondEnterInput.selection = secondEnterSelection;
    secondEnterInput.correctionEnabled = true;
    secondEnterInput.settingsEpoch = 11;
    secondEnterInput.authoredFov = 100.0f;
    secondEnterInput.gameplayBaseline = gameplayState.baseline.Read();
    secondEnterInput.effectiveAspect = 32.0f / 9.0f;
    secondEnterInput.nativeAspect = 16.0f / 9.0f;
    const auto secondEnter = plugin::ProcessAcceptedCinematicEnter(
        cinematicLifecycleState, secondEnterInput);
    const auto secondEnterAspect = plugin::ObserveCinematicAspectStore(
        cinematicLifecycleState, {newDialogueReceiver, true, true, false, true,
            true, 32.0f / 9.0f});
    plugin::CameraWriterResult secondCinematicWriter{};
    {
        plugin::CameraWriterInput input{newDialogueReceiver, 0x21, 100.0f,
            32.0f / 9.0f, true, 0x04, true, 0.02f, true,
            config::GameplayMode::RuntimeCombine, 16.0f / 9.0f};
        input.currentViewportAspect = 32.0f / 9.0f;
        input.currentViewportAspectValid = true;
        auto gameplayView = gameplayState.View();
        auto writerGate = gate.Acquire();
        secondCinematicWriter = plugin::ProcessCameraWriter(gate, state,
            input, cinematicState, freshReader, {}, &gameplayView);
    }
    pass &= Check(dialogueEndpoint.postExitProviderRecoveryCompleted &&
        secondEnter.accepted && secondEnter.baselineSelection.usedGameplayBaselineValue &&
        secondEnterInput.gameplayBaseline.source.value == newDialogueReceiver &&
        secondEnterAspect.accepted && secondCinematicWriter.cinematicHandled &&
        secondCinematicWriter.cinematicOutputAdmitted &&
        secondCinematicWriter.mutateFov &&
        std::fabs(secondCinematicWriter.outputFov - secondEnter.outputFov) < 0.03f,
        "cross_mode_replay_cinematic_dialogue_cinematic_uses_fresh_receiver_baseline_end_to_end");
    const auto secondCinematicSelection = cinematics::SelectCinematicBaseline(
        true, 100.0f, gameplayState.baseline.Read());
    pass &= Check(secondCinematicSelection.usedGameplayBaselineValue &&
        secondCinematicSelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline &&
        std::fabs(secondCinematicSelection.targetBaselineFov -
            newGameplayNativeFov) < 0.001f,
        "cross_mode_replay_second_cinematic_uses_fresh_receiver_b_baseline");

    pass &= authority_physical::Run();
    pass &= authority_cinematic::Run();
    pass &= authority_native_cinematic::Run();
    pass &= authority_transition_model::Run();
    pass &= authority_transition_model::RunGenerated();
    std::cout << (pass ? "PASS" : "FAIL") << ": camera writer transaction\n";
    return pass ? 0 : 1;
}
