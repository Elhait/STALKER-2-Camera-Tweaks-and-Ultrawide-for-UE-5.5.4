#pragma once

// Camera lifecycle authority shared by the native callback ports and the
// deterministic fixtures. The runtime owns serialization and callback admission.
// Boundary facts are captured by the adapters; ownership is resolved here.

#include "dialogue_boundary_transaction.hpp"
#include "lifecycle_authority_gameplay_effects.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace plugin::lifecycle_authority
{
    using ScopeId = std::uint64_t;
    using EventId = std::uint64_t;
    using TransactionId = std::uint64_t;

    enum class EventKind
    {
        Initialize,
        CinematicEnter,
        CinematicExit,
        CinematicAspectStore,
        CinematicActivationCaptured,
        RecoveryComplete,
        ProviderArmed,
        ProviderSetterObserved,
        ProviderWriterObserved,
        DialogueCandidate,
        DialogueActivated,
        DialogueTerminated,
        GameplaySample,
        SourceReplaced,
        SettingsChanged,
        PrepareEffect, // Legacy M1 characterization; composite Writer owns new execution permissions.
        EffectObserved,
        EffectClosed,
        Shutdown
    };

    enum class GameplayAdmission { Unknown, PolicyAdmitted, NativeProven };

    enum class SettingsDomain { Global, Gameplay, Cinematic, Dialogue };
    enum class SettingsMutation { EnableGate, PolicySelection, AspectPolicySelection };

    enum class DialoguePhase { None, Candidate, Active, Exiting, RearmPending };
    enum class RecoveryPhase { None, Pending, Complete };
    enum class BaselineTrust { None, Current, Retained, Invalid };
    enum class EffectResult { Applied, Refused, Partial, Unknown };
    enum class BaselineAction { Retain, Update, Invalidate, Exclude };
    enum class CorrectionOwner { PassThrough, Gameplay, Cinematic, Dialogue, Recovery };
    enum class FovProvenance { Unknown, OriginalNative, PreparedDialogue, PreparedCinematic, ModCorrectedOutput, EffectiveReadback };
    enum class FovBoundary { None, GameplayWriterInput, CinematicEnter, DialogueSetterInput, DialogueSetterArgument,
        PrimaryWriterInput, RecoveryEndpoint, WriterDecision, WriterArgument, PostStore, PostHelper };

    struct NativeEvidence
    {
        // These fields are assertions supplied by a future adapter, not proofs
        // synthesized by this relation. M1 binding records define which remain
        // unavailable from current native callbacks.
        bool callerValidated{};
        bool callbackBoundaryValidated{};
        bool sourceRelationValidated{};
        bool gameplayRoleProven{};
        bool recoveryEndpointProven{};
        bool causalArmingJoinProven{};
        bool dialogueDescentProven{};
        bool dialogueTerminalProven{};
        bool effectBoundaryCorrelated{};
    };

    struct EventFacts
    {
        EventKind kind{};
        ScopeId scope{};
        ScopeId previousScope{};
        std::uint64_t receiver{};
        std::uint64_t component{};
        std::uint64_t thread{};
        std::uint64_t generation{};
        std::uint64_t providerGeneration{};
        EventId armingEventId{};
        EventId effectId{};
        TransactionId transactionId{};
        std::uint64_t dependencyRevision{};
        double fov{};
        std::uint64_t outputObject{};
        double aspect{};
        bool aspectValid{};
        std::uint8_t flags{};
        bool flagsValid{};
        double nativeStart{std::numeric_limits<double>::quiet_NaN()};
        double nativeTarget{std::numeric_limits<double>::quiet_NaN()};
        bool nativeTargetReadable{};
        float viewportAspect{NAN};
        bool viewportAspectValid{};
        FovProvenance fovProvenance{FovProvenance::Unknown};
        FovBoundary fovBoundary{FovBoundary::None};
        NativeEvidence evidence{};
        SettingsDomain settingsDomain{SettingsDomain::Global};
        SettingsMutation settingsMutation{SettingsMutation::EnableGate};
        std::uint32_t policySelection{};
        bool enabled{};
        bool initialGlobalEnabled{};
        bool initialGameplayEnabled{};
        bool initialCinematicEnabled{};
        bool initialDialogueEnabled{};
        std::uint32_t initialGameplayPolicy{};
        std::uint32_t initialCinematicPolicy{};
        config::CinematicAspectPolicy initialCinematicAspectPolicy{config::CinematicAspectPolicy::Auto};
        std::uint32_t initialDialoguePolicy{};
        bool aspectContextCompatible{};
        // Explicit compatibility policy request; never promoted to native proof.
        bool requestPolicyAdmission{};
        bool gameplayBaselineDependent{};
        std::optional<EffectResult> effectResult;
    };

    struct EffectIntent
    {
        EventId id{};
        ScopeId scope{};
        std::uint64_t receiver{};
        std::uint64_t generation{};
        TransactionId transactionId{};
        std::uint64_t outputObject{};
        std::uint64_t thread{};
        std::uint64_t settingsEpoch{};
        std::uint64_t dependencyRevision{};
        double plannedOutputFov{};
        bool revoked{};
        bool closed{};
        std::optional<EffectResult> postStoreResult;
        std::optional<double> postStoreFov;
        std::optional<EffectResult> postHelperResult;
        std::optional<double> postHelperFov;

        bool HasObservation() const
        {
            return postStoreResult.has_value() || postHelperResult.has_value();
        }
    };

    struct FovWitness
    {
        FovProvenance provenance{FovProvenance::Unknown};
        FovBoundary boundary{FovBoundary::None};
        ScopeId scope{};
        std::uint64_t receiver{};
        std::uint64_t generation{};
        TransactionId transactionId{};
        double value{};
    };

    struct ScopeState
    {
        bool cinematicActive{};
        bool cinematicCorrectionArmed{};
        config::CinematicAspectPolicy cinematicAspectPolicy{config::CinematicAspectPolicy::Auto};
        float cinematicAspect{NAN};
        std::uint64_t cinematicGeneration{};
        RecoveryPhase recovery{RecoveryPhase::None};
        DialoguePhase dialogue{DialoguePhase::None};
        EventId activeArmingId{};
        std::uint64_t component{};
        std::uint64_t receiver{};
        std::uint64_t thread{};
        std::uint64_t providerGeneration{};
        std::uint64_t dialogueSettingsEpoch{};
        std::uint32_t dialoguePolicySnapshot{};
        std::uint64_t cinematicSettingsEpoch{};
        std::uint32_t cinematicPolicySnapshot{};
        std::optional<FovWitness> originalFov;
        std::optional<FovWitness> preparedDialogueFov;
        std::optional<FovWitness> preparedCinematicFov;
        std::optional<FovWitness> writerInputFov;
        std::optional<FovWitness> plannedOutputFov;
        std::optional<FovWitness> postStoreFov;
        std::optional<FovWitness> postHelperFov;
        bool setterMatched{};
        bool writerConfirmed{};
        std::uint64_t cinematicAspectSource{};
        bool cinematicAssociationConflict{};
        bool cinematicInitialEstablishmentActive{true};
        cinematics::CinematicEnterActivation cinematicActivation;
        cinematics::EnterActivationCorrectedStartCandidate cinematicDialogueStart;
        bool cinematicPredecessor{};
        ScopeId cinematicContinuationSource{};
        float cinematicContinuationStart{NAN};
        double cinematicTargetFov{NAN};
        std::optional<FovWitness> writerArgumentFov;
        std::uint32_t writerThread{};
        float writerAspect{NAN};
        std::uint8_t writerFlags{};
        bool writerSourceAdmitted{};
        std::uint64_t writerOutput{};
        cinematics::AdmittedWriterOutput lastAdmittedOutput;
        double cinematicAuthoredFov{std::numeric_limits<double>::quiet_NaN()};
        double cinematicBaselineFov{std::numeric_limits<double>::quiet_NaN()};
        std::optional<FovWitness> cinematicReference;
        double exitNativeTarget{std::numeric_limits<double>::quiet_NaN()};
        bool postCinematicExclusionActive{};
        bool exclusionDeparted{};
        bool atomicExitPending{};
        float atomicExitPreviousFov{NAN};
        double providerNativeStart{std::numeric_limits<double>::quiet_NaN()};
        double providerNativeTarget{std::numeric_limits<double>::quiet_NaN()};
        bool providerEndpointReached{};
        bool providerAdmissionConsumed{};
        bool providerDialogueOwned{};
        std::optional<dialogue::PostExitProviderHandoff> nativeProvider;
        std::uintptr_t dialogueCallbackSource{};
        std::uint64_t providerSettingsEpoch{};
        float dialogueBaseline{NAN};
        float dialoguePrevious{NAN};
        float dialogueExitStart{NAN};
        float dialogueNativeTarget{NAN};
        dialogue::CandidateTracker candidateTracker;
        dialogue::RecoveryRearm recoveryTracker;
        bool dialogueCorrectionArmed{true};
    };

    struct GameplayBaseline
    {
        ScopeId source{};
        std::uint64_t receiver{};
        std::uint64_t gameplaySettingsEpoch{};
        double fov{};
        BaselineTrust trust{BaselineTrust::None};
        GameplayAdmission admission{GameplayAdmission::Unknown};
        double aspect{std::numeric_limits<double>::quiet_NaN()};
        std::uint64_t observationSequence{};
        std::optional<FovWitness> preparedFov;
    };

    struct ArmingClaim
    {
        ScopeId scope{};
        std::uint64_t receiver{};
        std::uint64_t component{};
        std::uint64_t thread{};
        std::uint64_t generation{};
        std::uint64_t providerGeneration{};
        bool setterMatched{};
        bool writerConfirmed{};
        bool retired{};
    };

    struct Decision
    {
        bool accepted{};
        std::uint64_t revision{};
        std::string reason{"guard-rejected"};
        BaselineAction baseline{BaselineAction::Retain};
        CorrectionOwner correction{CorrectionOwner::PassThrough};
        EventId authorityEventId{};
        EventId effectId{};
        std::uint32_t selectedPolicy{};
        GameplayAdmission admission{GameplayAdmission::Unknown};
        bool correctionOnlyAdmitted{};
        bool cinematicWriterAdmitted{};
        cinematics::EnterActivationStatus cinematicActivationStatus{};
        cinematics::CinematicWriterAction cinematicAction{cinematics::CinematicWriterAction::PassThroughUnclassified};
    };

    struct EffectiveSettings
    {
        bool globalEnabled{};
        bool gameplayEnabled{};
        bool cinematicEnabled{};
        bool dialogueEnabled{};
        std::uint64_t globalEpoch{};
        std::uint64_t gameplayEpoch{};
        std::uint64_t cinematicEpoch{};
        std::uint64_t dialogueEpoch{};
        std::uint32_t gameplayPolicy{};
        std::uint32_t cinematicPolicy{};
        config::CinematicAspectPolicy cinematicAspectPolicy{config::CinematicAspectPolicy::Auto};
        std::uint32_t dialoguePolicy{};
    };

    struct LifecycleSnapshot
    {
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        gameplay::ReplayState replay{gameplay::ReplayState::WaitingForAutomaticUpdate};
        std::uint64_t generation{}, gameplayEpoch{}, cinematicEpoch{}, dialogueEpoch{};
        config::GameplayMode gameplayMode{};
        config::DialogueZoomPolicy dialoguePolicy{};
        bool gameplayEnabled{}, modePending{};
        bool cinematicCorrectionArmed{}, postCinematicExclusion{};
        ScopeId cinematicReceiver{}, continuationSource{};
        float cinematicTarget{NAN}, continuationStart{NAN};
    };

    struct Projection
    {
        std::uint64_t revision{};
        EffectiveSettings settings{};
        GameplayPhysicalState gameplayPhysical{};
        bool ready{};
        bool stopping{};
        std::map<ScopeId, ScopeState> scopes;
        std::optional<GameplayBaseline> baseline;
        std::map<EventId, ArmingClaim> armings;
        std::map<EventId, EffectIntent> effects;
        std::map<EventId, EffectIntent> observations;
        EventId nextAuthorityEventId{1};
        TransactionId nextWriterInvocationId{1};
        std::optional<ScopeState> pendingCinematic;
        ScopeId selectedCinematicScope{};
        ScopeId lastWriterScope{};
        std::uintptr_t lastWriterReceiver{};
        std::uint64_t firstPresentableWriterInvocation{1};
        std::uint64_t cinematicGeneration{};
        bool cinematicSelectionActive{};
        std::optional<cinematics::CinematicEnterActivation> cinematicCarry;
    };


    struct WriterPolicyContext
    {
        CorrectionOwner owner{CorrectionOwner::PassThrough};
        std::uint32_t selectedPolicy{};
        std::optional<GameplayBaseline> baseline;
        double incomingFov{};
        FovProvenance inputProvenance{FovProvenance::Unknown};
        std::optional<ScopeState> scope;
        std::uint64_t receiver{};
        bool cinematicSourceAdmitted{};
        bool writerBoundary{};
    };

    struct WriterProposal
    {
        double outputFov{};
        bool gameplayBaselineDependent{};
        bool endCinematicInitialEstablishment{};
        bool mutateArgument{true};
        cinematics::CinematicWriterAction cinematicAction{cinematics::CinematicWriterAction::PassThroughUnclassified};
    };

    struct WriterEvent
    {
        // Raw boundary/identity/provenance and supported native evidence. The
        // adapter supplies no GameplayEligible or final correction owner.
        EventFacts input;
        bool observeOutput{};
        bool nativeBoundary{};
        std::uint64_t diagnosticFrameId{};
        void* policyContext{};
        std::optional<WriterProposal> (*evaluate)(void*, const WriterPolicyContext&){};
        // Native field access only. The source port supplies this capability
        // while holding CameraCorrectionGate; it receives no lifecycle views.
        std::optional<plugin::CameraWriterEngineOperations> engine;
    };

    struct SetterEvent
    {
        EventFacts input;
        std::uintptr_t callbackSource{};
        std::optional<float> nativeArgumentBefore;
        bool nativeBoundary{};
    };

    struct ProviderArmEvent
    {
        std::uintptr_t component{}, receiver{};
        std::uint32_t thread{};
        float start{}, target{}, receiverFov{};
        bool validatedProviderCaller{};
    };

    struct SetterDecision
    {
        Decision decision;
        double outputFov{};
        bool prepareArgument{};
        plugin::DialogueBoundaryResult observation;
    };

    class Authority;
    class WriterAdapterPort;
    class DialogueAdapterPort;
    class CinematicAdapterPort;
    struct NativeCinematicEnter;
    struct NativeCinematicAspect;
    struct NativeCinematicArmer;
    struct NativeCinematicExit;
    struct NativeCinematicResult;

    class WriterInvocation final
    {
    public:
        WriterInvocation() = default;
        WriterInvocation(const WriterInvocation&) = delete;
        WriterInvocation& operator=(const WriterInvocation&) = delete;
        WriterInvocation(WriterInvocation&& other) noexcept
            : decision(std::move(other.decision)), writerInvocationId(other.writerInvocationId),
              diagnosticFrameId(other.diagnosticFrameId), observationId(other.observationId),
              observationUnavailable(other.observationUnavailable), physical(std::move(other.physical)),
              owner_(std::exchange(other.owner_, nullptr)),
              permission_(std::exchange(other.permission_, false)), output_(other.output_),
              argumentPrepared_(other.argumentPrepared_), argumentFinalized_(other.argumentFinalized_), revision_(other.revision_),
              invocation_(other.invocation_), observation_(other.observation_), outputObject_(other.outputObject_) {}
        WriterInvocation& operator=(WriterInvocation&&) = delete;
        ~WriterInvocation();

        Decision decision;
        TransactionId writerInvocationId{};
        std::uint64_t diagnosticFrameId{};
        EventId observationId{};
        bool observationUnavailable{};
        std::optional<plugin::CameraWriterGameplayResult> physical;
        bool ArgumentPrepared() const { return argumentPrepared_; }
        // No Applied flag: this receipt concerns PRE argument preparation only.
    private:
        friend class Authority;
        Authority* owner_{};
        bool permission_{};
        double output_{};
        bool argumentPrepared_{};
        bool argumentFinalized_{};
        std::uint64_t revision_{};
        TransactionId invocation_{};
        EventId observation_{};
        std::uint64_t outputObject_{};
    };


    // Temporary views exist only inside one Authority working reduction. The
    // production trajectory helpers operate on these private copies, never on
    // a RuntimeState reference supplied by an adapter.
    struct DialogueWorkingState
    {
        std::mutex mutex;
        std::atomic<camera::CoordinatorState> coordinator;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        dialogue::Phase phase;
        bool providerOwned;
        std::uintptr_t callbackSource;
        std::uintptr_t receiver;
        std::uint64_t generation;
        std::uint64_t providerEpoch;
        std::uint32_t thread;
        dialogue::PolicySnapshot policy;
        dialogue::CorrectionActivation activation;
        float baseline, previous, exitStart;
        dialogue::CandidateTracker candidate;
        dialogue::RecoveryRearm recovery;
        std::atomic<bool> enabled;
        std::atomic<config::DialogueZoomPolicy> selected;
        std::atomic<std::uint64_t> epoch, activeEpoch;
        std::atomic<std::uintptr_t> snapshotSource{};
        std::atomic<float> snapshotTarget{NAN};
        std::atomic<bool> snapshotTargetValid{};

        DialogueWorkingState(const ScopeState& scope, const EffectiveSettings& settings)
            : coordinator(scope.cinematicActive ? camera::CoordinatorState::CinematicActive :
                scope.recovery == RecoveryPhase::Pending ? camera::CoordinatorState::CinematicExiting :
                camera::CoordinatorState::Gameplay),
              phase(scope.activeArmingId && !scope.providerDialogueOwned &&
                    scope.dialogue == DialoguePhase::Candidate ? dialogue::Phase::Inactive :
                    static_cast<dialogue::Phase>(scope.dialogue)),
              providerOwned(scope.providerDialogueOwned), callbackSource(scope.dialogueCallbackSource),
              receiver(scope.receiver), generation(scope.cinematicGeneration),
              providerEpoch(scope.providerSettingsEpoch), thread(static_cast<std::uint32_t>(scope.thread)),
              baseline(scope.dialogueBaseline), previous(scope.dialoguePrevious),
              exitStart(scope.dialogueExitStart), candidate(scope.candidateTracker),
              recovery(scope.recoveryTracker), enabled(settings.globalEnabled && settings.dialogueEnabled),
              selected(static_cast<config::DialogueZoomPolicy>(settings.dialoguePolicy)),
              epoch(settings.dialogueEpoch), activeEpoch(scope.dialogueSettingsEpoch)
        {
            if (scope.postCinematicExclusionActive)
                exclusion.Arm(static_cast<float>(scope.exitNativeTarget));
            if (phase == dialogue::Phase::Active || phase == dialogue::Phase::Exiting ||
                phase == dialogue::Phase::RearmPending)
                policy.Capture(static_cast<config::DialogueZoomPolicy>(scope.dialoguePolicySnapshot));
            activation.AcceptedStart(scope.dialogueCorrectionArmed);
        }

        plugin::DialogueBoundaryStateView View()
        {
            return {mutex, coordinator, exclusion, phase, providerOwned, callbackSource,
                receiver, generation, providerEpoch, thread, policy, activation,
                baseline, previous, exitStart, candidate, recovery, enabled, selected,
                epoch, activeEpoch, snapshotSource, snapshotTarget, snapshotTargetValid};
        }

        void Store(ScopeState& scope) const
        {
            scope.dialogue = phase == dialogue::Phase::Inactive && scope.activeArmingId &&
                !scope.providerAdmissionConsumed ? DialoguePhase::Candidate : static_cast<DialoguePhase>(phase);
            scope.providerDialogueOwned = providerOwned;
            scope.dialogueCallbackSource = callbackSource;
            scope.providerSettingsEpoch = providerEpoch;
            scope.dialogueBaseline = baseline;
            scope.dialoguePrevious = previous;
            scope.dialogueExitStart = exitStart;
            scope.dialogueNativeTarget = snapshotTargetValid.load() ? snapshotTarget.load() : NAN;
            scope.candidateTracker = candidate;
            scope.recoveryTracker = recovery;
            scope.postCinematicExclusionActive = exclusion.IsActive();
            if (!scope.activeArmingId || scope.providerAdmissionConsumed)
                scope.dialogueSettingsEpoch = activeEpoch.load();
            scope.dialogueCorrectionArmed = activation.CanCorrect(enabled.load());
            if (policy.IsValid()) scope.dialoguePolicySnapshot = static_cast<std::uint32_t>(policy.Value());
        }
    };

    class Authority final
    {
    public:
        Authority() = default;
        Authority(const Authority&) = delete;
        Authority& operator=(const Authority&) = delete;
        Authority(Authority&&) = delete;
        Authority& operator=(Authority&&) = delete;
        static constexpr std::size_t MaxEffects = 128;
        static constexpr std::size_t MaxArmings = 32;
        static constexpr std::size_t MaxScopes = 64;
        WriterAdapterPort WriterBoundary(const plugin::CameraCorrectionGate& gate) noexcept;
        DialogueAdapterPort DialogueBoundary(const plugin::CameraCorrectionGate& gate) noexcept;
        CinematicAdapterPort CinematicBoundary(const plugin::CameraCorrectionGate& gate) noexcept;
        bool Initialize(const plugin::CameraCorrectionGate& gate, const config::FeatureConfig& config)
        {
            if (!gate.IsHeldByCurrentThread()) return false;
            EventFacts facts;
            facts.kind = EventKind::Initialize;
            facts.evidence.callbackBoundaryValidated = true;
            facts.initialGlobalEnabled = true;
            facts.initialGameplayEnabled = config.gameplayEnabled;
            facts.initialCinematicEnabled = config.cinematicsEnabled;
            facts.initialDialogueEnabled = config.dialogueEnabled;
            facts.initialGameplayPolicy = static_cast<unsigned>(config.gameplayMode);
            facts.initialCinematicPolicy = static_cast<unsigned>(config.cinematicFovMode);
            facts.initialCinematicAspectPolicy = config.cinematicAspectPolicy;
            facts.initialDialoguePolicy = static_cast<unsigned>(config.dialogueZoomPolicy);
            return Accept(facts).accepted;
        }

        plugin::CameraSettingTransactionResult ApplySetting(const plugin::CameraCorrectionGate& gate,
            const plugin::RuntimeSettingMutation& mutation, bool gameplayHookAvailable,
            bool dialogueNonNativeCapabilityAvailable)
        {
            using Kind = plugin::RuntimeSettingKind;
            plugin::CameraSettingTransactionResult result;
            EventFacts facts;
            facts.kind = EventKind::SettingsChanged;
            facts.evidence.callbackBoundaryValidated = true;
            const auto enable = std::get_if<bool>(&mutation.value);
            const auto select = [&](auto type, SettingsDomain domain, SettingsMutation selection) {
                const auto* value = std::get_if<decltype(type)>(&mutation.value);
                if (!value) return false;
                facts.settingsDomain = domain;
                facts.settingsMutation = selection;
                facts.policySelection = static_cast<unsigned>(*value);
                return true;
            };
            bool valid = false;
            switch (mutation.kind) {
            case Kind::GameplayEnabled:
                facts.settingsDomain = SettingsDomain::Gameplay;
                valid = enable && (!*enable || gameplayHookAvailable);
                break;
            case Kind::CinematicsEnabled:
                facts.settingsDomain = SettingsDomain::Cinematic;
                valid = enable != nullptr;
                break;
            case Kind::DialogueEnabled:
                facts.settingsDomain = SettingsDomain::Dialogue;
                valid = enable != nullptr;
                break;
            case Kind::GameplayMode:
                valid = select(config::GameplayMode{}, SettingsDomain::Gameplay, SettingsMutation::PolicySelection);
                break;
            case Kind::CinematicAspectPolicy:
                valid = select(config::CinematicAspectPolicy{}, SettingsDomain::Cinematic, SettingsMutation::AspectPolicySelection);
                break;
            case Kind::CinematicFovMode:
                valid = select(config::CinematicFovMode{}, SettingsDomain::Cinematic, SettingsMutation::PolicySelection);
                break;
            case Kind::DialogueZoomPolicy:
                valid = select(config::DialogueZoomPolicy{}, SettingsDomain::Dialogue, SettingsMutation::PolicySelection) &&
                    (dialogueNonNativeCapabilityAvailable ||
                     facts.policySelection == static_cast<unsigned>(config::DialogueZoomPolicy::Native));
                break;
            default:
                return result;
            }
            result.handled = true;
            if (!gate.IsHeldByCurrentThread() || !state_.ready || state_.stopping || !valid) return result;
            facts.enabled = enable && *enable;
            const auto committed = Accept(facts);
            result.mutation = {true, committed.accepted};
            result.flushCinematicTrace = committed.accepted && mutation.kind == Kind::CinematicsEnabled && !facts.enabled;
            return result;
        }

        LifecycleSnapshot ReadLifecycle(const plugin::CameraCorrectionGate& gate) const noexcept
        {
            LifecycleSnapshot result;
            if (!gate.IsHeldByCurrentThread() || !state_.ready || state_.stopping) return result;
            const auto selected = state_.scopes.find(state_.selectedCinematicScope);
            result.coordinator = state_.cinematicSelectionActive ? camera::CoordinatorState::CinematicActive :
                selected != state_.scopes.end() && selected->second.recovery == RecoveryPhase::Pending
                    ? camera::CoordinatorState::CinematicExiting : camera::CoordinatorState::Gameplay;
            result.replay = state_.gameplayPhysical.replay;
            result.generation = state_.cinematicGeneration;
            result.gameplayEpoch = state_.settings.gameplayEpoch;
            result.cinematicEpoch = state_.settings.cinematicEpoch;
            result.dialogueEpoch = state_.settings.dialogueEpoch;
            result.gameplayMode = static_cast<config::GameplayMode>(state_.settings.gameplayPolicy);
            result.dialoguePolicy = static_cast<config::DialogueZoomPolicy>(state_.settings.dialoguePolicy);
            result.gameplayEnabled = state_.settings.gameplayEnabled;
            result.modePending = state_.gameplayPhysical.modePending;
            const auto* scope = state_.pendingCinematic ? &*state_.pendingCinematic :
                selected == state_.scopes.end() ? nullptr : &selected->second;
            if (scope) {
                result.cinematicReceiver = scope->receiver;
                result.cinematicCorrectionArmed = scope->cinematicCorrectionArmed;
                result.postCinematicExclusion = scope->postCinematicExclusionActive;
                result.cinematicTarget = static_cast<float>(scope->cinematicTargetFov);
                result.continuationSource = scope->cinematicContinuationSource;
                result.continuationStart = scope->cinematicContinuationStart;
            }
            return result;
        }

        bool ReadSettings(const plugin::CameraCorrectionGate& gate, plugin::RuntimeSettingsSnapshot& snapshot) const noexcept
        {
            if (!gate.IsHeldByCurrentThread() || !state_.ready || state_.stopping) return false;
            snapshot.gameplayEnabled = state_.settings.gameplayEnabled;
            snapshot.gameplayMode = static_cast<config::GameplayMode>(state_.settings.gameplayPolicy);
            snapshot.cinematicsEnabled = state_.settings.cinematicEnabled;
            snapshot.cinematicAspectPolicy = state_.settings.cinematicAspectPolicy;
            snapshot.cinematicFovMode = static_cast<config::CinematicFovMode>(state_.settings.cinematicPolicy);
            snapshot.dialogueEnabled = state_.settings.dialogueEnabled;
            snapshot.dialogueZoomPolicy = static_cast<config::DialogueZoomPolicy>(state_.settings.dialoguePolicy);
            return true;
        }

        bool Shutdown(const plugin::CameraCorrectionGate& gate)
        {
            if (!gate.IsHeldByCurrentThread()) return false;
            EventFacts facts;
            facts.kind = EventKind::Shutdown;
            return Accept(facts).accepted;
        }

        std::optional<camera::CameraEvidenceSnapshot> ReadCameraEvidence(
            const plugin::CameraCorrectionGate& gate) const
        {
            if (!gate.IsHeldByCurrentThread() || !state_.ready || state_.stopping) return {};
            camera::CameraEvidenceSnapshot evidence;
            const auto& baseline = state_.baseline;
            const auto observed = state_.scopes.find(state_.lastWriterScope);
            if (baseline) {
                camera::GameplayBaseline reference;
                reference.valid = baseline->trust != BaselineTrust::None;
                reference.current = baseline->trust == BaselineTrust::Current ||
                    baseline->trust == BaselineTrust::Retained;
                reference.nativeFov = {static_cast<float>(baseline->fov), camera::FovSpace::Native,
                    camera::FovProvenance::NativeRegisterInput, true};
                reference.aspect = static_cast<float>(baseline->aspect);
                reference.source = {baseline->receiver, baseline->receiver != 0};
                reference.observationSequence = baseline->observationSequence;
                camera::PublishGameplayReference(evidence, reference, true, state_.lastWriterReceiver,
                    baseline->trust == BaselineTrust::Current && !state_.cinematicSelectionActive &&
                    observed != state_.scopes.end() && observed->second.dialogue == DialoguePhase::None &&
                    observed->second.recovery != RecoveryPhase::Pending);
            }
            if (observed == state_.scopes.end()) return evidence;
            const auto& scope = observed->second;
            const auto convert = [](const FovWitness& sample, camera::CameraFovBoundary boundary,
                camera::SnapshotFreshness freshness, camera::EvidenceProvenance provenance) {
                return camera::CameraFovFact{static_cast<float>(sample.value), true, provenance,
                    freshness, sample.receiver, sample.receiver != 0, sample.transactionId, boundary,
                    sample.transactionId};
            };
            using Boundary = camera::CameraFovBoundary;
            using Provenance = camera::EvidenceProvenance;
            using Freshness = camera::SnapshotFreshness;
            if (scope.originalFov && scope.originalFov->provenance == FovProvenance::OriginalNative) {
                const auto& sample = *scope.originalFov;
                const auto boundary = sample.boundary == FovBoundary::CinematicEnter ? Boundary::CinematicEnter :
                    sample.boundary == FovBoundary::DialogueSetterInput ? Boundary::DialogueSetter : Boundary::GameplayWriter;
                const bool current = sample.transactionId && scope.writerInputFov &&
                    sample.transactionId >= state_.firstPresentableWriterInvocation &&
                    sample.transactionId + 1 == state_.nextWriterInvocationId &&
                    sample.transactionId == scope.writerInputFov->transactionId;
                evidence.nativeFov = convert(sample, boundary,
                    current ? Freshness::Current : Freshness::Retained, Provenance::NativeNumericObservation);
            }
            if (!scope.writerInputFov ||
                scope.writerInputFov->transactionId < state_.firstPresentableWriterInvocation ||
                scope.writerInputFov->transactionId + 1 != state_.nextWriterInvocationId)
                return evidence;
            const auto& input = *scope.writerInputFov;
            evidence.writerObservationHasEvidence = true;
            evidence.aspect = scope.writerAspect;
            evidence.aspectValid = plugin::IsValidCameraWriterAspect(scope.writerAspect);
            evidence.flags = scope.writerFlags;
            if (evidence.aspectValid) evidence.aspectProvenance = Provenance::NativeNumericObservation;
            evidence.writerInputFov = convert(input, Boundary::GameplayWriter, Freshness::Current,
                Provenance::NativeNumericObservation);
            evidence.writerTransaction = {input.transactionId, input.receiver, scope.writerOutput,
                scope.writerThread, input.generation};
            if (scope.writerArgumentFov && scope.writerArgumentFov->transactionId == input.transactionId) {
                const bool corrected = scope.writerArgumentFov->provenance == FovProvenance::ModCorrectedOutput;
                evidence.currentOutputFov = convert(*scope.writerArgumentFov, Boundary::GameplayWriter,
                    Freshness::Current, corrected ? Provenance::ModDerivedState : Provenance::PassThroughResult);
                if (corrected) evidence.horPlusTargetFov = evidence.currentOutputFov;
            }
            if (scope.postStoreFov && scope.postStoreFov->transactionId == input.transactionId)
                evidence.postStoreFov = convert(*scope.postStoreFov, Boundary::PrimaryPostStore,
                    Freshness::Current, Provenance::NativeNumericObservation);
            if (scope.postHelperFov && scope.postHelperFov->transactionId == input.transactionId)
                evidence.postHelperFov = convert(*scope.postHelperFov, Boundary::PostHelper,
                    Freshness::Current, Provenance::NativeNumericObservation);
            return evidence;
        }

        bool ReadOverlayLifecycle(const plugin::CameraCorrectionGate& gate,
            plugin::OverlaySemanticSnapshot& snapshot) const
        {
            const auto evidence = ReadCameraEvidence(gate);
            if (!evidence) return false;
            using Provenance = plugin::SemanticProvenance;
            using Freshness = plugin::SemanticFreshness;
            const auto setting = [](auto value) {
                return plugin::SemanticFact<decltype(value)>{value, true, Provenance::RuntimeSettings, Freshness::AtRead};
            };
            const auto lifecycle = [](auto value) {
                return plugin::SemanticFact<decltype(value)>{value, true, Provenance::PresentationCoordinator,
                    Freshness::LifecycleScoped};
            };
            const auto& settings = state_.settings;
            snapshot.gameplayEnabled = setting(settings.gameplayEnabled);
            snapshot.gameplayMode = setting(static_cast<config::GameplayMode>(settings.gameplayPolicy));
            snapshot.cinematicsEnabled = setting(settings.cinematicEnabled);
            snapshot.cinematicAspectPolicy = setting(settings.cinematicAspectPolicy);
            snapshot.cinematicFovMode = setting(static_cast<config::CinematicFovMode>(settings.cinematicPolicy));
            snapshot.dialogueEnabled = setting(settings.dialogueEnabled);
            snapshot.dialogueZoomPolicy = setting(static_cast<config::DialogueZoomPolicy>(settings.dialoguePolicy));
            snapshot.gameplayEnableApplyPending = lifecycle(state_.gameplayPhysical.enablePending);
            snapshot.gameplayDisableRestorePending = lifecycle(state_.gameplayPhysical.disablePending);
            snapshot.gameplayModeTransitionPending = lifecycle(state_.gameplayPhysical.modePending);
            snapshot.cinematicSelectionActive = lifecycle(state_.cinematicSelectionActive);
            snapshot.activeCinematicAspectPolicy = {};
            snapshot.activeCinematicFovMode = {};
            snapshot.activeCinematicsEnabled = {};
            const auto selected = state_.scopes.find(state_.selectedCinematicScope);
            using Coordinator = plugin::OverlayCoordinatorState;
            snapshot.coordinator = lifecycle(state_.cinematicSelectionActive ? Coordinator::CinematicActive :
                selected != state_.scopes.end() && selected->second.recovery == RecoveryPhase::Pending
                    ? Coordinator::CinematicExiting : Coordinator::Gameplay);
            if (state_.cinematicSelectionActive) {
                const auto* cinematic = state_.pendingCinematic ? &*state_.pendingCinematic :
                    selected == state_.scopes.end() ? nullptr : &selected->second;
                if (cinematic) {
                    snapshot.activeCinematicAspectPolicy = lifecycle(cinematic->cinematicAspectPolicy);
                    snapshot.activeCinematicFovMode = lifecycle(
                        static_cast<config::CinematicFovMode>(cinematic->cinematicPolicySnapshot));
                    snapshot.activeCinematicsEnabled = lifecycle(cinematic->cinematicCorrectionArmed);
                }
            }
            snapshot.activeDialogueZoomPolicy = {};
            snapshot.activeDialogueCorrection = lifecycle(false);
            snapshot.dialoguePhase = lifecycle(plugin::OverlayDialoguePhase::Inactive);
            const auto observed = state_.scopes.find(state_.lastWriterScope);
            if (observed != state_.scopes.end()) {
                const auto& scope = observed->second;
                snapshot.dialoguePhase = lifecycle(static_cast<plugin::OverlayDialoguePhase>(scope.dialogue));
                if (scope.dialogue == DialoguePhase::Active || scope.dialogue == DialoguePhase::Exiting) {
                    snapshot.activeDialogueZoomPolicy = lifecycle(
                        static_cast<config::DialogueZoomPolicy>(scope.dialoguePolicySnapshot));
                    snapshot.activeDialogueCorrection = lifecycle(settings.dialogueEnabled && scope.dialogueCorrectionArmed);
                }
            }
            snapshot.gameplayBaselineUsable = lifecycle(evidence->gameplayReferenceUsable);
            snapshot.cameraState.evidence = *evidence;
            snapshot.cameraStateValid = evidence->writerObservationHasEvidence || evidence->gameplayFov.valid;
            return true;
        }

        Decision Accept(const EventFacts& facts)
        {
            Projection working = state_;
            Decision decision{};
            decision.revision = state_.revision;

            auto scopeIt = working.scopes.find(facts.scope);
            ScopeState* scope = scopeIt == working.scopes.end() ? nullptr : &scopeIt->second;
            auto commit = [&]() {
                PruneHistory(working);
                working.revision = state_.revision + 1;
                decision.accepted = true;
                decision.revision = working.revision;
                state_ = std::move(working);
                return decision;
            };

            switch (facts.kind)
            {
            case EventKind::Initialize:
                if (!facts.evidence.callbackBoundaryValidated || working.stopping || working.ready) return decision;
                working.ready = true;
                working.settings.globalEnabled = facts.initialGlobalEnabled;
                working.settings.gameplayEnabled = facts.initialGameplayEnabled;
                working.settings.cinematicEnabled = facts.initialCinematicEnabled;
                working.settings.dialogueEnabled = facts.initialDialogueEnabled;
                working.settings.gameplayPolicy = facts.initialGameplayPolicy;
                working.settings.cinematicPolicy = facts.initialCinematicPolicy;
                working.settings.cinematicAspectPolicy = facts.initialCinematicAspectPolicy;
                working.settings.dialoguePolicy = facts.initialDialoguePolicy;
                decision.reason = "initialized";
                return commit();

            case EventKind::Shutdown:
                if (working.stopping) return decision;
                working.ready = false;
                working.stopping = true;
                working.scopes.clear();
                working.pendingCinematic.reset();
                working.cinematicCarry.reset();
                working.selectedCinematicScope = 0;
                working.cinematicSelectionActive = false;
                working.observations.clear();
                working.baseline.reset();
                working.armings.clear();
                working.gameplayPhysical = {};
                working.lastWriterScope = 0;
                working.lastWriterReceiver = 0;
                RevokeOutstandingEffects(working);
                decision.baseline = BaselineAction::Invalidate;
                decision.reason = "camera-permissions-revoked";
                return commit();

            default:
                break;
            }

            if (!working.ready || working.stopping) return decision;


            // Observation windows never grant execution/lifecycle permission.
            if ((facts.kind == EventKind::EffectObserved || facts.kind == EventKind::EffectClosed) &&
                working.observations.contains(facts.effectId)) {
                auto& observation = working.observations.at(facts.effectId);
                if (!facts.evidence.callbackBoundaryValidated ||
                    facts.scope != observation.scope || facts.receiver != observation.receiver ||
                    facts.transactionId != observation.transactionId ||
                    facts.outputObject != observation.outputObject || facts.thread != observation.thread ||
                    facts.generation != observation.generation ||
                    facts.dependencyRevision != observation.dependencyRevision) return decision;
                if (facts.kind == EventKind::EffectClosed) {
                    working.observations.erase(facts.effectId);
                    decision.reason = "optional-observation-window-closed";
                    return commit();
                }
                if (!facts.evidence.effectBoundaryCorrelated || !facts.effectResult ||
                    facts.fovProvenance != FovProvenance::EffectiveReadback ||
                    !std::isfinite(facts.fov)) return decision;
                if (facts.fovBoundary == FovBoundary::PostStore && !observation.postStoreResult) {
                    observation.postStoreResult = facts.effectResult;
                    observation.postStoreFov = facts.fov;
                } else if (facts.fovBoundary == FovBoundary::PostHelper && !observation.postHelperResult) {
                    observation.postHelperResult = facts.effectResult;
                    observation.postHelperFov = facts.fov;
                } else return decision;
                // Delayed records remain historical. They cannot resurrect a
                // retired scope or replace another invocation's current output.
                if (scope && scope->receiver == facts.receiver && scope->plannedOutputFov &&
                    scope->plannedOutputFov->transactionId == facts.transactionId &&
                    observation.settingsEpoch == working.settings.globalEpoch) {
                    if (facts.fovBoundary == FovBoundary::PostStore)
                        scope->postStoreFov = MakeFovWitness(facts);
                    else scope->postHelperFov = MakeFovWitness(facts);
                }
                decision.reason = "optional-named-readback-recorded";
                return commit();
            }

            switch (facts.kind)
            {
            case EventKind::CinematicEnter:
            {
                if (!facts.evidence.callbackBoundaryValidated || facts.scope == 0 || facts.receiver == 0 ||
                    facts.generation == 0)
                    return decision;
                if (!scope && !MakeScopeRoom(working)) return decision;
                RevokeScopeEffects(working, facts.scope);
                const auto priorOriginal = scope && scope->receiver == facts.receiver
                    ? scope->originalFov : std::optional<FovWitness>{};
                const auto priorScope = scope ? std::optional<ScopeState>{*scope} : std::nullopt;
                if (scope && scope->activeArmingId != 0)
                    working.armings[scope->activeArmingId].retired = true;
                if (working.baseline && working.baseline->source == facts.scope)
                {
                    if (working.baseline->receiver == facts.receiver)
                        RetainCompatibleBaseline(working, facts.scope, facts.receiver);
                    else
                    {
                        working.baseline->trust = BaselineTrust::Invalid;
                        decision.baseline = BaselineAction::Invalidate;
                    }
                }
                working.scopes[facts.scope] = ScopeState{};
                scope = &working.scopes[facts.scope];
                scope->originalFov = priorOriginal;
                if (priorScope && priorScope->receiver == facts.receiver) {
                    scope->cinematicActivation = priorScope->cinematicActivation;
                    if (priorScope->cinematicAssociationConflict) scope->cinematicActivation.Retire();
                    scope->cinematicPredecessor = priorScope->recovery == RecoveryPhase::Pending ||
                        priorScope->recovery == RecoveryPhase::Complete;
                    if (priorScope->dialogue == DialoguePhase::Active && priorScope->writerArgumentFov &&
                        priorScope->writerInputFov && priorScope->writerThread &&
                        priorScope->writerArgumentFov->transactionId == priorScope->writerInputFov->transactionId) {
                        auto& start = scope->cinematicDialogueStart;
                        start.receiver = facts.receiver;
                        start.dialogueSettingsEpoch = priorScope->dialogueSettingsEpoch;
                        start.activationStartSample = static_cast<float>(priorScope->writerInputFov->value);
                        start.correctedStart = static_cast<float>(priorScope->writerArgumentFov->value);
                        start.dialogueContextActive = start.valid = true;
                        start.dialogueGeneration = priorScope->cinematicGeneration;
                        start.writerSequence = priorScope->writerArgumentFov->transactionId;
                        start.writerThreadId = priorScope->writerThread;
                        start.writerInputFov = start.activationStartSample;
                        start.writerAspect = start.writerPolicyAspect = priorScope->writerAspect;
                        start.writerFlags = priorScope->writerFlags;
                        start.writerFlagsValid = true;
                    }
                }
                scope->cinematicActive = true;
                scope->cinematicCorrectionArmed = working.settings.globalEnabled && working.settings.cinematicEnabled;
                scope->cinematicAspectPolicy = working.settings.cinematicAspectPolicy;
                scope->cinematicAuthoredFov = facts.fov;
                if (working.baseline && working.baseline->source == facts.scope &&
                    working.baseline->receiver == facts.receiver &&
                    (working.baseline->trust == BaselineTrust::Current ||
                     working.baseline->trust == BaselineTrust::Retained))
                {
                    scope->cinematicBaselineFov = working.baseline->fov;
                    scope->cinematicReference = FovWitness{FovProvenance::OriginalNative,
                        FovBoundary::GameplayWriterInput, working.baseline->source, working.baseline->receiver,
                        0, working.baseline->observationSequence, working.baseline->fov};
                }
                scope->cinematicGeneration = facts.generation;
                if (working.cinematicGeneration < facts.generation) working.cinematicGeneration = facts.generation;
                scope->receiver = facts.receiver;
                scope->cinematicSettingsEpoch = working.settings.cinematicEpoch;
                scope->cinematicPolicySnapshot = working.settings.cinematicPolicy;
                if (facts.fovProvenance == FovProvenance::OriginalNative &&
                    facts.fovBoundary == FovBoundary::CinematicEnter && std::isfinite(facts.fov))
                    scope->originalFov = MakeFovWitness(facts);
                decision.reason = "cinematic-ownership-accepted";
                decision.selectedPolicy = scope->cinematicPolicySnapshot;
                decision.correction = facts.aspectContextCompatible &&
                    FeatureEnabled(working.settings, CorrectionOwner::Cinematic)
                    ? CorrectionOwner::Cinematic : CorrectionOwner::PassThrough;
                return commit();
            }

            case EventKind::CinematicAspectStore:
                if (!scope || !scope->cinematicActive || !facts.evidence.callbackBoundaryValidated ||
                    facts.generation != scope->cinematicGeneration || !facts.receiver) return decision;
                if (scope->cinematicAspectSource && scope->cinematicAspectSource != facts.receiver)
                    scope->cinematicAssociationConflict = true;
                else scope->cinematicAspectSource = facts.receiver;
                decision.reason = "cinematic-aspect-source-observed";
                return commit();

            case EventKind::CinematicActivationCaptured:
            {
                if (!scope || !scope->cinematicActive || !facts.evidence.callbackBoundaryValidated ||
                    !facts.evidence.callerValidated || facts.generation != scope->cinematicGeneration ||
                    facts.receiver != scope->receiver || !facts.component || !facts.thread ||
                    !facts.aspectValid || !std::isfinite(facts.aspect) || facts.aspect <= 0 ||
                    !facts.nativeTargetReadable) return decision;
                camera::GameplayBaseline reference;
                reference.valid = reference.current = std::isfinite(scope->cinematicBaselineFov);
                reference.aspect = static_cast<float>(facts.aspect);
                reference.nativeFov = {static_cast<float>(scope->cinematicBaselineFov), camera::FovSpace::Native,
                    camera::FovProvenance::NativeRegisterInput, reference.valid};
                const auto mode = static_cast<config::CinematicFovMode>(scope->cinematicPolicySnapshot);
                const auto selection = cinematics::SelectCinematicBaseline(mode == config::CinematicFovMode::GameplayHorPlus,
                    static_cast<float>(scope->cinematicAuthoredFov), reference);
                float target{};
                if (!FeatureEnabled(working.settings, CorrectionOwner::Cinematic) ||
                    !cinematics::TryTransformSelectedCinematicFov(true, mode,
                        static_cast<float>(scope->cinematicAuthoredFov), static_cast<float>(scope->cinematicAuthoredFov),
                        selection.targetBaselineFov, static_cast<float>(facts.aspect), 16.0f / 9.0f, target)) return decision;
                const bool nativePair = working.baseline && working.baseline->source == facts.scope &&
                    working.baseline->receiver == facts.receiver && working.baseline->preparedFov &&
                    (working.baseline->trust == BaselineTrust::Current || working.baseline->trust == BaselineTrust::Retained);
                scope->cinematicActivation.Arm(facts.generation, scope->cinematicSettingsEpoch,
                    nativePair ? facts.receiver : 0,
                    nativePair ? static_cast<float>(working.baseline->fov) : NAN,
                    nativePair ? static_cast<float>(working.baseline->preparedFov->value) : NAN,
                    nativePair, target, scope->cinematicDialogueStart, scope->cinematicPredecessor,
                    static_cast<float>(facts.aspect), working.settings.gameplayEpoch);
                const auto inherited = scope->cinematicActivation.InheritedWriterStart(facts.receiver,
                    static_cast<float>(facts.nativeStart), static_cast<float>(facts.aspect), facts.flags,
                    static_cast<std::uint32_t>(facts.thread), working.settings.gameplayEpoch,
                    working.settings.dialogueEpoch, facts.component);
                if (!scope->cinematicActivation.Capture(true, facts.generation, scope->cinematicSettingsEpoch,
                    facts.component, facts.receiver, static_cast<std::uint32_t>(facts.thread),
                    static_cast<float>(facts.nativeStart), static_cast<float>(facts.nativeTarget),
                    working.settings.dialogueEpoch, inherited)) return decision;
                scope->cinematicTargetFov = target;
                decision.reason = "native-enter-activation-captured";
                return commit();
            }

            case EventKind::CinematicExit:
                if (!scope || !scope->cinematicActive || !facts.evidence.callbackBoundaryValidated ||
                    scope->cinematicGeneration != facts.generation ||
                    scope->receiver != facts.receiver ||
                    !facts.evidence.sourceRelationValidated) return decision;
                RevokeScopeEffects(working, facts.scope);
                scope->cinematicActive = false;
                scope->recovery = RecoveryPhase::Pending;
                scope->exitNativeTarget = facts.nativeTargetReadable ? facts.nativeTarget : facts.fov;
                scope->postCinematicExclusionActive = true;
                decision.reason = "cinematic-exit-transferred-to-recovery";
                decision.correction = CorrectionOwner::PassThrough;
                return commit();

            case EventKind::RecoveryComplete:
            {
                if (!scope || scope->recovery != RecoveryPhase::Pending || !facts.evidence.callbackBoundaryValidated ||
                    scope->cinematicGeneration != facts.generation || scope->receiver != facts.receiver ||
                    !facts.evidence.recoveryEndpointProven ||
                    !facts.evidence.sourceRelationValidated) return decision;
                RevokeScopeEffects(working, facts.scope);
                scope->recovery = RecoveryPhase::Complete;
                const auto currentArm = working.armings.find(scope->activeArmingId);
                const bool providerConfirmed = scope->activeArmingId != 0 &&
                    currentArm != working.armings.end() && !currentArm->second.retired &&
                    currentArm->second.setterMatched && currentArm->second.writerConfirmed &&
                    scope->setterMatched && scope->writerConfirmed;
                if (providerConfirmed)
                {
                    scope->dialogue = DialoguePhase::Active;
                    decision.reason = "recovery-transferred-to-confirmed-dialogue";
                    decision.selectedPolicy = scope->dialoguePolicySnapshot;
                    decision.correction = FeatureEnabled(working.settings, CorrectionOwner::Dialogue)
                        ? CorrectionOwner::Dialogue : CorrectionOwner::PassThrough;
                    decision.baseline = BaselineAction::Exclude;
                    RetainCompatibleBaseline(working, facts.scope, facts.receiver);
                }
                else if (scope->activeArmingId != 0)
                {
                    decision.reason = "provider-quarantine-retained-without-complete-transfer-proof";
                    decision.baseline = BaselineAction::Exclude;
                    RetainCompatibleBaseline(working, facts.scope, facts.receiver);
                }
                else if (scope->dialogue == DialoguePhase::Active)
                {
                    decision.reason = "active-dialogue-retains-baseline-without-gameplay-update";
                    decision.baseline = BaselineAction::Exclude;
                    RetainCompatibleBaseline(working, facts.scope, facts.receiver);
                }
                else if (!scope->cinematicActive && facts.evidence.gameplayRoleProven &&
                    facts.fovProvenance == FovProvenance::OriginalNative &&
                    facts.fovBoundary == FovBoundary::RecoveryEndpoint && std::isfinite(facts.fov))
                {
                    working.baseline = GameplayBaseline{facts.scope, facts.receiver,
                        working.settings.gameplayEpoch, facts.fov, BaselineTrust::Current};
                    working.baseline->admission = GameplayAdmission::NativeProven;
                    decision.admission = GameplayAdmission::NativeProven;
                    scope->dialogue = DialoguePhase::None;
                    scope->originalFov = MakeFovWitness(facts);
                    decision.reason = "recovery-completed-with-native-gameplay-admission";
                    decision.baseline = BaselineAction::Update;
                    decision.selectedPolicy = working.settings.gameplayPolicy;
                    decision.correction = FeatureEnabled(working.settings, CorrectionOwner::Gameplay)
                        ? CorrectionOwner::Gameplay : CorrectionOwner::PassThrough;
                }
                else
                {
                    decision.reason = "recovery-completed-without-gameplay-proof";
                    decision.baseline = BaselineAction::Exclude;
                }
                return commit();
            }

            case EventKind::ProviderArmed:
                if (!scope || scope->recovery == RecoveryPhase::None || !facts.evidence.callbackBoundaryValidated ||
                    !facts.evidence.callerValidated || facts.component == 0 || facts.thread == 0)
                    return decision;
                if (scope->receiver != facts.receiver || facts.generation != scope->cinematicGeneration)
                    return decision;
                // A validated provider arm is only a Dialogue hypothesis until
                // its setter and writer are joined. Keep that hypothesis in the
                // same scope so the later endpoint activation has an explicit
                // predecessor. A repeated provider arm may supersede the
                // existing provider candidate, but it cannot overwrite an
                // already Active Dialogue lifecycle.
                if (scope->dialogue != DialoguePhase::None &&
                    !(scope->dialogue == DialoguePhase::Candidate &&
                        scope->activeArmingId != 0))
                    return decision;
                // Component/thread/generation are compatibility facts only;
                // accepted armings receive a fresh Authority event identity.
                if (scope->activeArmingId != 0)
                    working.armings[scope->activeArmingId].retired = true;
                PruneHistory(working);
                if (working.armings.size() >= MaxArmings) return decision;
                RevokeScopeEffects(working, facts.scope);
                scope->activeArmingId = working.nextAuthorityEventId++;
                scope->dialogue = DialoguePhase::Candidate;
                scope->component = facts.component;
                scope->receiver = facts.receiver;
                scope->thread = facts.thread;
                scope->providerGeneration = facts.providerGeneration;
                scope->dialogueSettingsEpoch = working.settings.dialogueEpoch;
                scope->dialoguePolicySnapshot = working.settings.dialoguePolicy;
                scope->setterMatched = false;
                scope->writerConfirmed = false;
                scope->providerNativeStart = facts.nativeStart;
                scope->providerNativeTarget = facts.nativeTargetReadable ? facts.nativeTarget : NAN;
                scope->providerEndpointReached = false;
                scope->providerAdmissionConsumed = false;
                working.armings[scope->activeArmingId] = ArmingClaim{
                    facts.scope, facts.receiver, facts.component, facts.thread,
                    facts.generation, facts.providerGeneration, false, false, false};
                decision.authorityEventId = scope->activeArmingId;
                decision.reason = "new-arming-event-superseded-prior-scope-arm";
                decision.baseline = BaselineAction::Exclude;
                return commit();

            case EventKind::ProviderSetterObserved:
            case EventKind::ProviderWriterObserved:
            {
                const auto armIt = working.armings.find(facts.armingEventId);
                if (!scope || armIt == working.armings.end() || armIt->second.retired ||
                    scope->activeArmingId != facts.armingEventId || !facts.evidence.callbackBoundaryValidated ||
                    !facts.evidence.causalArmingJoinProven ||
                    armIt->second.scope != facts.scope || armIt->second.receiver != facts.receiver ||
                    armIt->second.component != facts.component || armIt->second.thread != facts.thread ||
                    armIt->second.generation != facts.generation ||
                    armIt->second.providerGeneration != facts.providerGeneration) return decision;
                auto& arm = armIt->second;
                if (facts.kind == EventKind::ProviderSetterObserved)
                {
                    if (facts.fovBoundary != FovBoundary::DialogueSetterInput ||
                        facts.fovProvenance == FovProvenance::Unknown ||
                        facts.fovProvenance == FovProvenance::EffectiveReadback ||
                        facts.fovProvenance == FovProvenance::ModCorrectedOutput ||
                        !std::isfinite(facts.fov)) return decision;
                    arm.setterMatched = true;
                    scope->setterMatched = true;
                    if (facts.fovProvenance == FovProvenance::OriginalNative)
                        scope->originalFov = MakeFovWitness(facts);
                    else
                        scope->preparedDialogueFov = MakeFovWitness(facts);
                    decision.reason = "setter-bound-to-current-arming";
                }
                else
                {
                    if (!arm.setterMatched || facts.transactionId == 0 ||
                        facts.fovBoundary != FovBoundary::PrimaryWriterInput ||
                        !std::isfinite(facts.fov) ||
                        (facts.fovProvenance != FovProvenance::OriginalNative &&
                         facts.fovProvenance != FovProvenance::PreparedDialogue &&
                         facts.fovProvenance != FovProvenance::PreparedCinematic)) return decision;
                    arm.writerConfirmed = true;
                    scope->writerConfirmed = true;
                    if (std::isfinite(scope->providerNativeTarget) &&
                        std::fabs(facts.fov - scope->providerNativeTarget) <= 0.25) {
                        scope->providerEndpointReached = true;
                        scope->postCinematicExclusionActive = false;
                    }
                    scope->writerInputFov = MakeFovWitness(facts);
                    decision.reason = "writer-confirmed-for-current-arming";
                    decision.selectedPolicy = scope->dialoguePolicySnapshot;
                    decision.correction = FeatureEnabled(working.settings, CorrectionOwner::Dialogue)
                        ? CorrectionOwner::Dialogue : CorrectionOwner::PassThrough;
                }
                return commit();
            }

            case EventKind::DialogueCandidate:
                if (facts.scope == 0 || !facts.evidence.callbackBoundaryValidated) return decision;
                if (!scope && !MakeScopeRoom(working)) return decision;
                scope = &working.scopes[facts.scope];
                if (scope->dialogue != DialoguePhase::None || scope->cinematicActive)
                    return decision;
                if (facts.receiver == 0) return decision;
                scope->dialogue = DialoguePhase::Candidate;
                scope->receiver = facts.receiver;
                decision.reason = "scoped-dialogue-hypothesis-only";
                return commit();

            case EventKind::DialogueActivated:
                if (!scope || scope->dialogue != DialoguePhase::Candidate || !facts.evidence.callbackBoundaryValidated ||
                    !facts.evidence.dialogueDescentProven || scope->cinematicActive ||
                    scope->receiver != facts.receiver) return decision;
                RevokeScopeEffects(working, facts.scope);
                scope->dialogue = DialoguePhase::Active;
                scope->dialogueSettingsEpoch = working.settings.dialogueEpoch;
                scope->dialoguePolicySnapshot = working.settings.dialoguePolicy;
                decision.reason = "dialogue-ownership-accepted";
                decision.baseline = BaselineAction::Exclude;
                decision.selectedPolicy = scope->dialoguePolicySnapshot;
                RetainCompatibleBaseline(working, facts.scope, facts.receiver);
                decision.correction = FeatureEnabled(working.settings, CorrectionOwner::Dialogue)
                    ? CorrectionOwner::Dialogue : CorrectionOwner::PassThrough;
                return commit();

            case EventKind::DialogueTerminated:
                if (!scope || scope->dialogue == DialoguePhase::None || !facts.evidence.callbackBoundaryValidated ||
                    !facts.evidence.dialogueTerminalProven || !facts.evidence.sourceRelationValidated ||
                    scope->receiver != facts.receiver)
                    return decision;
                RevokeScopeEffects(working, facts.scope);
                RetireDialogueScope(working, *scope);
                decision.reason = "dialogue-terminal-proof-released-dialogue-permission";
                return commit();

            case EventKind::GameplaySample:
                if (!facts.evidence.callbackBoundaryValidated || facts.scope == 0 || facts.receiver == 0 ||
                    (!facts.evidence.gameplayRoleProven &&
                     !(facts.requestPolicyAdmission && facts.evidence.sourceRelationValidated)) ||
                    facts.fovProvenance != FovProvenance::OriginalNative ||
                    facts.fovBoundary != FovBoundary::GameplayWriterInput ||
                    !std::isfinite(facts.fov) ||
                    IsQuarantined(working, facts.scope)) return decision;
                if (!scope && !MakeScopeRoom(working)) return decision;
                scope = &working.scopes[facts.scope];
                if (scope->dialogue != DialoguePhase::None || scope->cinematicActive ||
                    scope->recovery == RecoveryPhase::Pending) return decision;
                RevokeScopeEffects(working, facts.scope);
                scope->receiver = facts.receiver;
                scope->originalFov = MakeFovWitness(facts);
                working.baseline = GameplayBaseline{facts.scope, facts.receiver,
                    working.settings.gameplayEpoch, facts.fov, BaselineTrust::Current};
                working.baseline->admission = facts.evidence.gameplayRoleProven
                    ? GameplayAdmission::NativeProven : GameplayAdmission::PolicyAdmitted;
                working.baseline->aspect = facts.aspectValid ? facts.aspect : NAN;
                working.baseline->observationSequence = facts.transactionId;
                decision.admission = working.baseline->admission;
                decision.reason = "source-scoped-gameplay-baseline-admitted";
                decision.baseline = BaselineAction::Update;
                decision.selectedPolicy = working.settings.gameplayPolicy;
                decision.correction = FeatureEnabled(working.settings, CorrectionOwner::Gameplay)
                    ? CorrectionOwner::Gameplay : CorrectionOwner::PassThrough;
                return commit();

            case EventKind::SourceReplaced:
            {
                if (!facts.evidence.callbackBoundaryValidated || !facts.evidence.sourceRelationValidated ||
                    facts.scope == 0 || facts.previousScope == 0 ||
                    facts.previousScope == facts.scope) return decision;
                if (working.baseline && working.baseline->source == facts.previousScope)
                {
                    working.baseline->trust = BaselineTrust::Invalid;
                    decision.baseline = BaselineAction::Invalidate;
                }
                RevokeScopeEffects(working, facts.previousScope);
                const auto previous = working.scopes.find(facts.previousScope);
                if (previous != working.scopes.end()) {
                    if (previous->second.activeArmingId)
                        working.armings[previous->second.activeArmingId].retired = true;
                    working.scopes.erase(previous);
                }
                decision.reason = "source-replacement-invalidated-only-dependent-baseline";
                return commit();
            }
            case EventKind::SettingsChanged:
                if (!facts.evidence.callbackBoundaryValidated) return decision;
                if (!ApplySettingsMutation(working, facts, decision)) return decision;
                decision.reason = "effective-settings-committed";
                return commit();

            case EventKind::PrepareEffect:
            {
                if (!scope || !facts.evidence.callbackBoundaryValidated ||
                    facts.transactionId == 0 || facts.receiver == 0 ||
                    facts.fovProvenance != FovProvenance::ModCorrectedOutput ||
                    facts.fovBoundary != FovBoundary::WriterDecision || !std::isfinite(facts.fov)) return decision;
                const CorrectionOwner owner = ResolveCorrectionOwner(working, facts.scope, facts.receiver, *scope);
                if (owner == CorrectionOwner::PassThrough) return decision;
                if (!FeatureEnabled(working.settings, owner)) return decision;
                if (scope->receiver != facts.receiver) return decision;
                if ((owner == CorrectionOwner::Cinematic || owner == CorrectionOwner::Recovery ||
                     (owner == CorrectionOwner::Dialogue && scope->activeArmingId != 0)) &&
                    facts.generation != scope->cinematicGeneration) return decision;
                if (facts.gameplayBaselineDependent && (!working.baseline ||
                    working.baseline->source != facts.scope || working.baseline->receiver != facts.receiver ||
                    working.baseline->gameplaySettingsEpoch != working.settings.gameplayEpoch ||
                    (working.baseline->trust != BaselineTrust::Current &&
                     working.baseline->trust != BaselineTrust::Retained))) return decision;
                PruneHistory(working);
                if (working.effects.size() >= MaxEffects) return decision;
                for (const auto& [key, existing] : working.effects)
                    if (!existing.revoked && !existing.closed && existing.scope == facts.scope &&
                        existing.transactionId == facts.transactionId) return decision;
                const EventId id = working.nextAuthorityEventId++;
                EffectIntent effect{};
                effect.id = id;
                effect.scope = facts.scope;
                effect.receiver = facts.receiver;
                effect.generation = facts.generation;
                effect.transactionId = facts.transactionId;
                effect.settingsEpoch = working.settings.globalEpoch;
                effect.dependencyRevision = working.revision + 1;
                effect.plannedOutputFov = facts.fov;
                working.effects[id] = effect;
                scope->plannedOutputFov = MakeFovWitness(facts);
                decision.authorityEventId = id;
                decision.effectId = id;
                decision.correction = owner;
                decision.reason = "per-invocation-effect-permission-and-intent-committed";
                return commit();
            }

            case EventKind::EffectObserved:
            {
                auto effectIt = working.effects.find(facts.effectId);
                if (effectIt == working.effects.end() || effectIt->second.revoked || effectIt->second.closed || !facts.effectResult ||
                    !facts.evidence.effectBoundaryCorrelated || facts.transactionId == 0 || facts.receiver == 0 ||
                    facts.fovProvenance != FovProvenance::EffectiveReadback ||
                    (facts.fovBoundary != FovBoundary::PostStore && facts.fovBoundary != FovBoundary::PostHelper) ||
                    !std::isfinite(facts.fov) ||
                    effectIt->second.transactionId != facts.transactionId ||
                    effectIt->second.scope != facts.scope || effectIt->second.receiver != facts.receiver ||
                    effectIt->second.generation != facts.generation ||
                    effectIt->second.settingsEpoch != working.settings.globalEpoch ||
                    effectIt->second.dependencyRevision != facts.dependencyRevision) return decision;
                if (facts.fovBoundary == FovBoundary::PostStore)
                {
                    if (effectIt->second.postStoreResult) return decision;
                    effectIt->second.postStoreResult = facts.effectResult;
                    effectIt->second.postStoreFov = facts.fov;
                    working.scopes[facts.scope].postStoreFov = MakeFovWitness(facts);
                }
                else
                {
                    if (effectIt->second.postHelperResult) return decision;
                    effectIt->second.postHelperResult = facts.effectResult;
                    effectIt->second.postHelperFov = facts.fov;
                    working.scopes[facts.scope].postHelperFov = MakeFovWitness(facts);
                }
                decision.reason = *facts.effectResult == EffectResult::Applied ? "named-effect-boundary-applied" :
                    *facts.effectResult == EffectResult::Refused ? "named-effect-boundary-refused" :
                    *facts.effectResult == EffectResult::Partial ? "named-effect-boundary-partial" :
                    "named-effect-boundary-unknown";
                return commit();
            }

            case EventKind::EffectClosed:
            {
                const auto effect = working.effects.find(facts.effectId);
                if (effect == working.effects.end() || !facts.evidence.callbackBoundaryValidated ||
                    effect->second.scope != facts.scope || effect->second.receiver != facts.receiver ||
                    effect->second.transactionId != facts.transactionId ||
                    effect->second.dependencyRevision != facts.dependencyRevision) return decision;
                effect->second.closed = true;
                decision.reason = "observation-window-explicitly-closed";
                return commit();
            }
            case EventKind::Initialize:
            case EventKind::Shutdown:
                return decision;
            }
            return decision;
        }



        Decision Accept(const ProviderArmEvent& event)
        {
            const auto source = state_.selectedCinematicScope;
            const auto it = state_.scopes.find(source);
            if (!event.validatedProviderCaller || it == state_.scopes.end() ||
                !state_.settings.globalEnabled || !state_.settings.dialogueEnabled ||
                state_.settings.dialoguePolicy == static_cast<unsigned>(config::DialogueZoomPolicy::Native))
                return {};
            const auto& scope = it->second;
            if (!scope.postCinematicExclusionActive || event.receiver != scope.receiver) return {};
            dialogue::PostExitProviderHandoff tracker;
            const dialogue::PostExitProviderActivation activation{scope.cinematicGeneration,
                state_.settings.dialogueEpoch, event.component, event.receiver, event.thread,
                event.start, event.target, event.receiverFov,
                static_cast<config::DialogueZoomPolicy>(state_.settings.dialoguePolicy)};
            if (!tracker.ObserveArmer(true, activation)) return {};
            EventFacts facts;
            facts.kind = EventKind::ProviderArmed;
            facts.scope = source;
            facts.receiver = event.receiver;
            facts.component = event.component;
            facts.thread = event.thread;
            facts.generation = facts.providerGeneration = activation.generation;
            facts.nativeStart = event.start;
            facts.nativeTarget = event.target;
            facts.nativeTargetReadable = true;
            facts.evidence.callbackBoundaryValidated = facts.evidence.callerValidated = true;
            Authority working;
            working.state_ = state_;
            const auto accepted = working.Accept(facts);
            if (accepted.accepted) {
                working.state_.scopes.at(source).nativeProvider = std::move(tracker);
                state_ = std::move(working.state_);
            }
            return accepted;
        }

        SetterDecision Accept(const SetterEvent& event)
        {
            SetterDecision result;
            result.outputFov = event.input.fov;
            result.decision.revision = state_.revision;
            auto raw = event.input;
            if (event.nativeBoundary) raw.generation = state_.cinematicGeneration;
            if (!state_.ready || state_.stopping || !raw.scope || !raw.receiver ||
                !raw.evidence.callbackBoundaryValidated) return result;
            Projection w = state_;
            if (!w.scopes.contains(raw.scope) && !MakeScopeRoom(w)) return result;
            auto& scope = w.scopes[raw.scope];
            if (scope.receiver && scope.receiver != raw.receiver) return result;
            // The provider handoff grants setter authority to its confirmed
            // caller and thread. Sharing the receiver does not make another
            // setter a continuation or terminal observation of that handoff.
            if (event.nativeBoundary && scope.providerDialogueOwned &&
                (event.callbackSource != scope.dialogueCallbackSource || raw.thread != scope.thread ||
                 raw.generation != scope.cinematicGeneration))
                return result;
            scope.receiver = raw.receiver;
            if (scope.postCinematicExclusionActive && !std::isfinite(scope.exitNativeTarget)) return result;
            bool nativeAdmission = false;
            bool nativeSetterMatched = false;
            if (scope.nativeProvider && event.nativeArgumentBefore) {
                auto& tracker = *scope.nativeProvider;
                const auto epoch = w.settings.dialogueEpoch;
                if (tracker.Active())
                    nativeSetterMatched = tracker.ObserveSetterSample(w.settings.dialogueEnabled &&
                        raw.evidence.sourceRelationValidated, raw.component, raw.receiver,
                        static_cast<std::uint32_t>(raw.thread), raw.generation, epoch,
                        static_cast<float>(raw.nativeStart), static_cast<float>(raw.nativeTarget),
                        static_cast<float>(raw.fov), *event.nativeArgumentBefore);
                else if (tracker.RecoveryCompleted() && w.settings.dialogueEnabled &&
                    raw.evidence.sourceRelationValidated)
                    nativeAdmission = tracker.ConsumeDialogueAdmission(raw.component, raw.receiver,
                        static_cast<std::uint32_t>(raw.thread), raw.generation, epoch,
                        static_cast<float>(raw.nativeStart), static_cast<float>(raw.nativeTarget));
            }
            const bool causalProvider = scope.activeArmingId && scope.activeArmingId == raw.armingEventId &&
                w.armings.contains(raw.armingEventId) && !w.armings.at(raw.armingEventId).retired &&
                raw.evidence.causalArmingJoinProven && scope.component == raw.component &&
                scope.thread == raw.thread && scope.cinematicGeneration == raw.generation &&
                scope.providerGeneration == raw.providerGeneration &&
                scope.dialogueSettingsEpoch == w.settings.dialogueEpoch &&
                scope.providerNativeStart == raw.nativeStart && raw.nativeTargetReadable &&
                scope.providerNativeTarget == raw.nativeTarget;
            if (raw.armingEventId && !causalProvider) return result;
            DialogueWorkingState local(scope, w.settings);
            if (w.cinematicSelectionActive)
                local.coordinator.store(camera::CoordinatorState::CinematicActive);
            plugin::DialogueBoundaryInput input;
            input.incomingFov = static_cast<float>(raw.fov);
            input.callbackSource = event.callbackSource;
            input.observedReceiver = raw.receiver;
            input.validatedReceiver = raw.evidence.sourceRelationValidated ? raw.receiver : 0;
            input.nativeTarget = static_cast<float>(raw.nativeTarget);
            input.nativeTargetReadable = raw.nativeTargetReadable;
            input.generation = raw.generation;
            input.threadId = static_cast<std::uint32_t>(raw.thread);
            const bool usableReference = w.baseline && w.baseline->source == raw.scope &&
                w.baseline->receiver == raw.receiver && w.baseline->gameplaySettingsEpoch == w.settings.gameplayEpoch &&
                (w.baseline->trust == BaselineTrust::Current || w.baseline->trust == BaselineTrust::Retained);
            input.handledProviderActivation = nativeAdmission || (causalProvider && scope.providerEndpointReached &&
                scope.setterMatched && scope.writerConfirmed && !scope.providerAdmissionConsumed);
            input.providerGameplayBaseline = usableReference ? static_cast<float>(w.baseline->fov) : NAN;
            input.providerGameplayBaselineValid = usableReference;
            input.providerDialoguePolicy = static_cast<config::DialogueZoomPolicy>(scope.dialoguePolicySnapshot);
            input.providerDialoguePolicyValid = input.handledProviderActivation;
            const bool hadAdmittedProvider = scope.providerDialogueOwned && scope.providerAdmissionConsumed;
            std::lock_guard localLock(local.mutex);
            const auto reduced = plugin::ProcessDialogueBoundary(local.View(), input);
            if (scope.nativeProvider)
                plugin::RecordProviderDialogueSetterResult(local.View(), input, reduced, *scope.nativeProvider);
            if (input.handledProviderActivation) scope.providerAdmissionConsumed = true;
            local.Store(scope);
            if ((causalProvider || nativeSetterMatched) && reduced.inputValid) {
                scope.setterMatched = true;
                w.armings.at(scope.activeArmingId).setterMatched = true;
            }
            if (hadAdmittedProvider && reduced.stateReset)
                RetireNativeProvider(w, scope);
            if (scope.dialogue == DialoguePhase::Active || scope.dialogue == DialoguePhase::Exiting ||
                scope.dialogue == DialoguePhase::RearmPending)
                RetainCompatibleBaseline(w, raw.scope, raw.receiver);
            if (reduced.inputValid && raw.fovProvenance == FovProvenance::OriginalNative)
                scope.originalFov = MakeFovWitness(raw);
            if (reduced.mutateXmm1) {
                auto prepared = raw;
                prepared.fov = reduced.outputFov;
                prepared.fovProvenance = FovProvenance::PreparedDialogue;
                prepared.fovBoundary = FovBoundary::DialogueSetterArgument;
                scope.preparedDialogueFov = MakeFovWitness(prepared);
            } else if (reduced.action != plugin::DialogueBoundaryAction::ProviderCallbackIgnored)
                scope.preparedDialogueFov.reset();
            result.outputFov = reduced.outputFov;
            result.prepareArgument = reduced.mutateXmm1;
            result.observation = reduced;
            result.decision.accepted = true;
            result.decision.revision = state_.revision + 1;
            result.decision.selectedPolicy = static_cast<std::uint32_t>(reduced.activePolicy);
            result.decision.correction = scope.dialogue == DialoguePhase::Active && reduced.mutateXmm1
                ? CorrectionOwner::Dialogue : CorrectionOwner::PassThrough;
            result.decision.reason = "source-bound-setter-committed";
            RevokeScopeEffects(w, raw.scope);
            w.revision = result.decision.revision;
            state_ = std::move(w);
            return result;
        }

        WriterInvocation Accept(const WriterEvent& event)
        {
            WriterInvocation receipt;
            receipt.decision.revision = state_.revision;
            auto raw = event.input;
            if (event.nativeBoundary) BindNativeWriterFacts(raw);
            if (!state_.ready || state_.stopping || !raw.evidence.callbackBoundaryValidated ||
                !raw.scope || !raw.receiver || !std::isfinite(raw.fov) ||
                raw.fov <= 1.0 || raw.fov >= 179.0) return receipt;

            // Only the thread carrying the native provider activation may
            // consume its setter sample or complete its recovery.
            if (event.nativeBoundary) {
                const auto scope = state_.scopes.find(raw.scope);
                if (scope != state_.scopes.end() && scope->second.nativeProvider &&
                    scope->second.nativeProvider->Active() && raw.thread != scope->second.thread)
                    return receipt;
            }

            // Private staged relation. Internal commits affect only this local
            // working state; no projection or adapter can observe them. Policy
            // evaluation receives an immutable numeric context, never this owner.
            Authority working;
            working.state_ = state_;
            auto facts = raw;
            facts.transactionId = working.state_.nextWriterInvocationId;
            BaselineAction baselineAction = BaselineAction::Retain;
            const auto fail = [&]() { return WriterInvocation{}; };
            auto previousScope = raw.previousScope;
            const auto selected = working.state_.scopes.find(working.state_.selectedCinematicScope);
            if (event.engine && selected != working.state_.scopes.end() &&
                selected->second.recovery == RecoveryPhase::Pending && selected->second.receiver != raw.receiver) {
                const auto& recovery = selected->second;
                const cinematics::CinematicRecoveryOwnerSnapshot owner{
                    true, recovery.cinematicGeneration, recovery.receiver};
                cinematics::SharedWriterPairTransition transition;
                transition.changed = working.state_.lastWriterReceiver != raw.receiver;
                transition.previous.source = working.state_.lastWriterReceiver;
                transition.current.source = raw.receiver;
                const bool retired = raw.evidence.sourceRelationValidated &&
                    cinematics::RetireRecoveryForWriterRsiChange(camera::CoordinatorState::CinematicExiting,
                        owner, transition, [&] { previousScope = selected->first; });
                if (!retired) {
                    // Preserve recovery ownership and consume only the writer
                    // observation. A different output object gives no evidence
                    // of a receiver departure.
                    receipt.decision.accepted = true;
                    receipt.decision.reason = "foreign-writer-held-during-recovery";
                    receipt.writerInvocationId = working.state_.nextWriterInvocationId++;
                    working.state_.lastWriterScope = raw.scope;
                    working.state_.lastWriterReceiver = raw.receiver;
                    receipt.decision.revision = ++working.state_.revision;
                    state_ = std::move(working.state_);
                    return receipt;
                }
                working.state_.selectedCinematicScope = 0;
            }
            if (previousScope && previousScope != raw.scope) {
                facts.kind = EventKind::SourceReplaced;
                facts.previousScope = previousScope;
                const auto replaced = working.Accept(facts);
                if (!replaced.accepted) return fail();
                baselineAction = replaced.baseline;
                facts.previousScope = raw.previousScope;
            }
            auto current = working.state_.scopes.find(raw.scope);
            if (current != working.state_.scopes.end() &&
                (current->second.cinematicActive || current->second.recovery == RecoveryPhase::Pending) &&
                raw.generation != current->second.cinematicGeneration) return fail();

            if (current != working.state_.scopes.end() &&
                current->second.dialogue == DialoguePhase::Candidate &&
                !current->second.activeArmingId && !current->second.nativeProvider &&
                !working.state_.cinematicSelectionActive && !current->second.cinematicActive &&
                current->second.recovery != RecoveryPhase::Pending &&
                !current->second.postCinematicExclusionActive &&
                current->second.receiver == raw.receiver &&
                raw.fovProvenance == FovProvenance::OriginalNative &&
                raw.evidence.gameplayRoleProven && raw.evidence.sourceRelationValidated) {
                // Independently proven Gameplay supersedes a hypothesis on the
                // same camera even when its previous numeric reference changed.
                auto terminal = facts;
                terminal.kind = EventKind::DialogueTerminated;
                terminal.evidence.dialogueTerminalProven = true;
                if (!working.Accept(terminal).accepted) return fail();
                current = working.state_.scopes.find(raw.scope);
            }

            if (current != working.state_.scopes.end() &&
                current->second.dialogue == DialoguePhase::Candidate && !current->second.activeArmingId &&
                raw.fovProvenance == FovProvenance::OriginalNative && working.state_.baseline) {
                const auto& reference = *working.state_.baseline;
                camera::GameplayBaseline nativeReference;
                nativeReference.valid = reference.trust == BaselineTrust::Current;
                nativeReference.current = nativeReference.valid;
                nativeReference.nativeFov = {static_cast<float>(reference.fov), camera::FovSpace::Native,
                    camera::FovProvenance::NativeRegisterInput, true};
                nativeReference.aspect = static_cast<float>(reference.aspect);
                nativeReference.source = {reference.receiver, reference.receiver != 0};
                nativeReference.observationSequence = reference.observationSequence;
                DialogueWorkingState local(current->second, working.state_.settings);
                if (plugin::RetireStationaryDialogueCandidateOnGameplayWriter(local.View(), nativeReference,
                    raw.receiver, static_cast<float>(raw.fov), static_cast<float>(raw.aspect), raw.aspectValid,
                    raw.generation, false, 0.02f)) local.Store(current->second);
            }

            if (current != working.state_.scopes.end() &&
                current->second.dialogue != DialoguePhase::None &&
                raw.evidence.dialogueTerminalProven) {
                facts.kind = EventKind::DialogueTerminated;
                if (!working.Accept(facts).accepted) return fail();
            }
            current = working.state_.scopes.find(raw.scope);
            plugin::CameraWriterInput native;
            native.sourceIdentity = raw.receiver;
            native.outputObjectIdentity = raw.outputObject;
            native.threadId = static_cast<std::uint32_t>(raw.thread);
            native.incomingFov = static_cast<float>(raw.fov);
            native.aspect = static_cast<float>(raw.aspect);
            native.aspectValid = raw.aspectValid;
            native.flags = raw.flags;
            native.flagsValid = raw.flagsValid;
            native.currentViewportAspect = raw.viewportAspect;
            native.currentViewportAspectValid = raw.viewportAspectValid;
            native.gameplayEnabled = working.state_.settings.globalEnabled && working.state_.settings.gameplayEnabled;
            native.gameplayMode = static_cast<config::GameplayMode>(working.state_.settings.gameplayPolicy);
            const bool boundRecovery = event.engine && !raw.armingEventId &&
                current != working.state_.scopes.end() && current->second.receiver == raw.receiver &&
                current->second.recovery == RecoveryPhase::Pending && raw.evidence.sourceRelationValidated;
            plugin::AtomicExitHandoffResult atomicHandoff;
            if (boundRecovery) {
                auto& scope = current->second;
                atomicHandoff = detail::ProcessRecoveryAspectHandoff(
                    scope.atomicExitPending, scope.atomicExitPreviousFov,
                    scope.dialogue != DialoguePhase::None &&
                        !(scope.dialogue == DialoguePhase::Candidate && scope.nativeProvider),
                    working.state_.gameplayPhysical, native, static_cast<float>(scope.exitNativeTarget), *event.engine);
            }
            std::optional<float> nativeProviderOutput;
            bool providerAwaitingSetterAtGameplayEndpoint = false;
            if (event.engine && current != working.state_.scopes.end() && current->second.nativeProvider) {
                auto& scope = current->second;
                auto& tracker = *scope.nativeProvider;
                const auto& settings = working.state_.settings;
                dialogue::PostExitProviderReject providerReject{};
                const bool wasActive = tracker.Active();
                const bool dialogueAuthorized = settings.globalEnabled && settings.dialogueEnabled &&
                    raw.evidence.sourceRelationValidated;
                const auto evidence = wasActive
                    ? tracker.TakeWriterSample(dialogueAuthorized && scope.postCinematicExclusionActive,
                        raw.receiver, native.threadId, raw.generation, settings.dialogueEpoch,
                        native.incomingFov, 0.001f, &providerReject)
                    : dialogue::PostExitProviderWriterEvidence{};
                // Writer may keep publishing the proved endpoint before the
                // setter consumes admission. Hold it without extending the
                // permission to unrelated samples or admitting Gameplay.
                const bool awaitingAdmission = dialogueAuthorized && !scope.providerAdmissionConsumed &&
                    scope.dialogue == DialoguePhase::Candidate && scope.providerEndpointReached &&
                    tracker.BlocksGameplayBaselineAdmission(raw.receiver, native.threadId, raw.generation,
                        settings.dialogueEpoch, native.incomingFov, native.recoveryEndpointTolerance);
                const auto& reference = working.state_.baseline;
                const bool trustedReference = reference && reference->source == raw.scope &&
                    reference->receiver == raw.receiver && reference->gameplaySettingsEpoch == settings.gameplayEpoch &&
                    (reference->trust == BaselineTrust::Current || reference->trust == BaselineTrust::Retained);
                const auto& activation = tracker.Activation();
                const auto tolerance = native.recoveryEndpointTolerance;
                // A delayed armer may run after Cinematic recovery. The unchanged
                // Gameplay endpoint keeps its ordinary correction while the
                // provider waits for setter evidence. Baseline admission stays excluded.
                providerAwaitingSetterAtGameplayEndpoint =
                    providerReject == dialogue::PostExitProviderReject::NoMatchedSetterSample &&
                    scope.recovery == RecoveryPhase::Complete && trustedReference &&
                    std::fabs(native.incomingFov - reference->fov) <= tolerance &&
                    std::fabs(native.incomingFov - scope.exitNativeTarget) <= tolerance &&
                    std::fabs(native.incomingFov - activation.nativeStart) <= tolerance;
                if ((evidence.valid || awaitingAdmission) && (!atomicHandoff.handled || atomicHandoff.writeSucceeded) && native.gameplayEnabled &&
                    (native.gameplayMode == config::GameplayMode::HorPlus ||
                     native.gameplayMode == config::GameplayMode::RuntimeCombine) && trustedReference) {
                    const auto mapping = plugin::ResolvePostExitProviderWriterMapping(native, activation,
                        static_cast<float>(reference->fov), static_cast<float>(scope.exitNativeTarget),
                        static_cast<float>(scope.cinematicTargetFov), *event.engine, atomicHandoff.writeSucceeded);
                    if (mapping) {
                        nativeProviderOutput = mapping->output;
                        scope.writerConfirmed = true;
                        if (wasActive && std::fabs(native.incomingFov - activation.nativeTarget) <= 0.25f) {
                            tracker.CompleteRecovery();
                            scope.providerEndpointReached = true;
                            scope.postCinematicExclusionActive = false;
                            scope.recovery = RecoveryPhase::Complete;
                            scope.atomicExitPending = false;
                        }
                    }
                }
                if (wasActive && !tracker.Active() && !tracker.RecoveryCompleted())
                    RetireNativeProvider(working.state_, scope);
            }
            detail::RecoveryWriterEffect recoveryEffect;
            const bool physicalRecovery = boundRecovery && !nativeProviderOutput;
            // An endpoint can arrive ahead of the remaining interpolation.
            // Exclusion still owns the source until departure and return; a
            // sample inside that interval must not receive Gameplay HorPlus.
            if (event.nativeBoundary && event.engine && !physicalRecovery && !nativeProviderOutput &&
                current != working.state_.scopes.end() && raw.evidence.sourceRelationValidated &&
                current->second.receiver == raw.receiver) {
                const auto& scope = current->second;
                const auto nativeEnd = scope.exitNativeTarget;
                const auto correctedStart = scope.cinematicTargetFov;
                if (scope.recovery == RecoveryPhase::Complete && scope.postCinematicExclusionActive &&
                    scope.dialogue == DialoguePhase::None && std::isfinite(nativeEnd) &&
                    std::isfinite(correctedStart) &&
                    std::fabs(raw.fov - nativeEnd) > native.recoveryEndpointTolerance &&
                    raw.fov >= (std::min)(nativeEnd, correctedStart) &&
                    raw.fov <= (std::max)(nativeEnd, correctedStart))
                    recoveryEffect = detail::EvaluateRecoveryWriter(native, static_cast<float>(nativeEnd),
                        static_cast<float>(correctedStart), *event.engine);
            }
            const bool lateRecoveryFrame = !physicalRecovery && recoveryEffect.argument.has_value();
            if (physicalRecovery) {
                if (!atomicHandoff.handled) recoveryEffect = detail::EvaluateRecoveryWriter(native,
                    static_cast<float>(current->second.exitNativeTarget),
                    static_cast<float>(current->second.cinematicTargetFov), *event.engine);
                facts.evidence.recoveryEndpointProven = recoveryEffect.complete;
            }
            const bool recoveryOnlyEndpoint = physicalRecovery && recoveryEffect.complete &&
                (!native.gameplayEnabled || native.gameplayMode == config::GameplayMode::AspectRecalculation);
            // A native recovery endpoint and the Dialogue exclusion have
            // different lifetimes. The exclusion needs a departure and return.
            bool exclusionSuppressed = false;
            bool exclusionReleased = false;
            if (event.engine && current != working.state_.scopes.end() &&
                raw.evidence.sourceRelationValidated && current->second.receiver == raw.receiver &&
                current->second.postCinematicExclusionActive) {
                auto& scope = current->second;
                const bool atTarget = std::fabs(raw.fov - scope.exitNativeTarget) <= native.recoveryEndpointTolerance;
                if (scope.exclusionDeparted && atTarget) {
                    scope.postCinematicExclusionActive = false;
                    exclusionReleased = true;
                    if (scope.nativeProvider && !scope.providerAdmissionConsumed)
                        RetireNativeProvider(working.state_, scope);
                } else {
                    if (!atTarget) scope.exclusionDeparted = true;
                    exclusionSuppressed = true;
                }
            }
            if (current != working.state_.scopes.end() &&
                current->second.recovery == RecoveryPhase::Pending &&
                facts.evidence.recoveryEndpointProven) {
                facts.kind = EventKind::RecoveryComplete;
                facts.fovBoundary = FovBoundary::RecoveryEndpoint;
                if (recoveryOnlyEndpoint || exclusionSuppressed) facts.evidence.gameplayRoleProven = false;
                const auto completed = working.Accept(facts);
                if (!completed.accepted) return fail();
                working.state_.scopes.at(raw.scope).atomicExitPending = false;
                baselineAction = completed.baseline;
            }
            if (raw.armingEventId) {
                facts.kind = EventKind::ProviderWriterObserved;
                facts.fovBoundary = FovBoundary::PrimaryWriterInput;
                // Internal invocation ID, never an optional diagnostics ID.
                facts.transactionId = working.state_.nextWriterInvocationId;
                if (!working.Accept(facts).accepted) return fail();
            }
            // A validated departure and return can release exclusion after
            // recovery is already complete. Classify this invocation from the
            // resulting scope so the first proven Gameplay frame is corrected.
            if (event.nativeBoundary && exclusionReleased && !working.state_.cinematicSelectionActive) {
                const auto released = working.state_.scopes.find(raw.scope);
                if (released != working.state_.scopes.end() &&
                    released->second.recovery == RecoveryPhase::Complete &&
                    released->second.dialogue == DialoguePhase::None && !released->second.activeArmingId) {
                    raw.fovProvenance = facts.fovProvenance = FovProvenance::OriginalNative;
                    facts.requestPolicyAdmission = true;
                }
            }
            facts.kind = EventKind::GameplaySample;
            facts.fovBoundary = FovBoundary::GameplayWriterInput;
            const auto baselineBeforeAdmission = working.state_.baseline;
            const auto admitted = recoveryOnlyEndpoint || exclusionSuppressed ? Decision{} : working.Accept(facts);
            auto& w = working.state_;
            current = w.scopes.find(raw.scope);
            CorrectionOwner owner = current == w.scopes.end() ? CorrectionOwner::PassThrough :
                ResolveCorrectionOwner(w, raw.scope, raw.receiver, current->second);
            // A confirmed provider trajectory permits its scoped transition even
            // before Active Dialogue, as in the existing bounded handoff path.
            if (nativeProviderOutput || (raw.armingEventId && current != w.scopes.end() && current->second.writerConfirmed))
                owner = CorrectionOwner::Dialogue;

            // Ordinary Dialogue is a setter lifecycle. Its writer still goes
            // through Gameplay's physical pipeline. Only the joined provider
            // trajectory has a separate writer mapping before this pipeline.
            const bool dialogueGameplayRoute = event.engine && !raw.armingEventId && !nativeProviderOutput &&
                current != w.scopes.end() && current->second.receiver == raw.receiver &&
                !w.cinematicSelectionActive && !current->second.cinematicActive &&
                current->second.recovery != RecoveryPhase::Pending &&
                (current->second.dialogue == DialoguePhase::Active ||
                 current->second.dialogue == DialoguePhase::Exiting ||
                 current->second.dialogue == DialoguePhase::RearmPending);
            const auto gameplayMode = static_cast<config::GameplayMode>(w.settings.gameplayPolicy);
            const bool gameplayHorPlus = gameplayMode == config::GameplayMode::HorPlus ||
                gameplayMode == config::GameplayMode::RuntimeCombine;
            const bool providerContinuation = dialogueGameplayRoute &&
                current->second.dialogue == DialoguePhase::Active &&
                current->second.providerDialogueOwned && current->second.providerAdmissionConsumed &&
                current->second.thread != 0 && current->second.thread == raw.thread &&
                current->second.cinematicGeneration == raw.generation;
            const bool mapProviderContinuation = providerContinuation && w.settings.dialogueEnabled &&
                current->second.dialogueCorrectionArmed && gameplayHorPlus && w.settings.gameplayEnabled;
            if (dialogueGameplayRoute)
                owner = mapProviderContinuation ? CorrectionOwner::Dialogue : CorrectionOwner::Gameplay;
            else if (event.engine && owner == CorrectionOwner::Dialogue &&
                (!gameplayHorPlus || !w.settings.gameplayEnabled)) owner = CorrectionOwner::PassThrough;

            // RuntimeCombine can synchronize a replacement receiver still at
            // native aspect. Only the physical writer's fresh read-back can
            // authorize the transform after dispatch.
            const bool usesViewport = event.engine && gameplayMode == config::GameplayMode::RuntimeCombine;
            const auto correctionAspect = usesViewport ? native.currentViewportAspect : native.aspect;
            bool correctionOnly = false;
            if (owner == CorrectionOwner::PassThrough && raw.evidence.sourceRelationValidated &&
                raw.fovProvenance == FovProvenance::OriginalNative && raw.outputObject &&
                raw.aspectValid && plugin::IsValidCameraWriterAspect(native.aspect) &&
                (!usesViewport || native.currentViewportAspectValid) &&
                plugin::IsValidCameraWriterAspect(correctionAspect) &&
                correctionAspect > native.nativeAspect + 0.001f &&
                raw.flagsValid && (raw.flags == 4 || raw.flags == 5) && gameplayHorPlus &&
                !IsQuarantined(w, raw.scope) &&
                (current == w.scopes.end() || current->second.dialogue == DialoguePhase::None)) {
                // Preserve production's supported per-writer HorPlus permission
                // without refreshing or transferring a Gameplay reference.
                if (current == w.scopes.end()) {
                    if (!MakeScopeRoom(w)) return fail();
                    w.scopes[raw.scope].receiver = raw.receiver;
                    current = w.scopes.find(raw.scope);
                }
                correctionOnly = true;
                owner = CorrectionOwner::Gameplay;
            }
            if (exclusionSuppressed && !raw.armingEventId && current != w.scopes.end() &&
                current->second.recovery == RecoveryPhase::Complete &&
                ((current->second.dialogue == DialoguePhase::None && !current->second.activeArmingId) ||
                    providerAwaitingSetterAtGameplayEndpoint))
                owner = CorrectionOwner::Gameplay;
            if (!FeatureEnabled(w.settings, owner)) owner = CorrectionOwner::PassThrough;
            if (owner == CorrectionOwner::Cinematic && !current->second.cinematicCorrectionArmed)
                owner = CorrectionOwner::PassThrough;
            if (owner == CorrectionOwner::Dialogue && current->second.providerDialogueOwned &&
                !current->second.dialogueCorrectionArmed) owner = CorrectionOwner::PassThrough;
            if (recoveryOnlyEndpoint || lateRecoveryFrame) owner = CorrectionOwner::Recovery;
            Decision final;

            final.accepted = true;
            final.revision = state_.revision + 1;
            final.correction = owner;
            final.baseline = admitted.accepted ? admitted.baseline : baselineAction;
            final.admission = admitted.admission;
            final.correctionOnlyAdmitted = correctionOnly && owner == CorrectionOwner::Gameplay;
            final.selectedPolicy = owner == CorrectionOwner::Gameplay ? w.settings.gameplayPolicy :
                owner == CorrectionOwner::Cinematic ? current->second.cinematicPolicySnapshot :
                owner == CorrectionOwner::Dialogue ? current->second.dialoguePolicySnapshot : 0;
            final.reason = "composite-writer-committed";
            const auto invocation = w.nextWriterInvocationId++;
            if (current != w.scopes.end()) {
                auto captured = raw;
                captured.transactionId = invocation;
                captured.fovBoundary = FovBoundary::PrimaryWriterInput;
                current->second.writerInputFov = MakeFovWitness(captured);
                current->second.writerThread = static_cast<std::uint32_t>(raw.thread);
                current->second.writerAspect = static_cast<float>(raw.aspect);
                current->second.writerFlags = raw.flags;
                current->second.writerOutput = raw.outputObject;
                current->second.writerSourceAdmitted = owner == CorrectionOwner::Cinematic && raw.outputObject &&
                    current->second.cinematicAspectSource == raw.receiver &&
                    !current->second.cinematicAssociationConflict && raw.evidence.sourceRelationValidated;
                if (current->second.writerSourceAdmitted)
                    current->second.lastAdmittedOutput = {true, raw.generation, invocation, raw.receiver, raw.outputObject};
                current->second.postStoreFov.reset();
                current->second.postHelperFov.reset();
            }
            std::optional<WriterProposal> proposal;
            const bool physicalGameplay = event.engine && w.settings.globalEnabled && !recoveryOnlyEndpoint &&
                (dialogueGameplayRoute || owner == CorrectionOwner::Gameplay ||
                 (owner == CorrectionOwner::PassThrough && !w.settings.gameplayEnabled &&
                  raw.evidence.sourceRelationValidated && !IsQuarantined(w, raw.scope) &&
                  (current == w.scopes.end() || current->second.dialogue == DialoguePhase::None)));
            if (physicalGameplay) {
                native.writerContext = admitted.accepted ? camera::WriterContext::GameplayEligible :
                    providerContinuation ? camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved :
                    exclusionSuppressed ? camera::WriterContext::PostCinematicRecoveryExcluded :
                    dialogueGameplayRoute ? camera::WriterContext::DialogueLifecycleObserved :
                    camera::WriterContext::TransitionAmbiguous;
                if (providerContinuation) {
                    auto& scope = current->second;
                    native.providerDialogueContinuation = true;
                    native.dialogueCorrectionEnabled = mapProviderContinuation;
                    native.dialoguePolicy = static_cast<config::DialogueZoomPolicy>(scope.dialoguePolicySnapshot);
                    native.providerDialogueBaseline = scope.dialogueBaseline;
                    native.providerDialogueNativeTarget = scope.dialogueNativeTarget;
                    native.providerDialogueTarget = dialogue::ResolveDialogueDestinationTarget(native.dialoguePolicy,
                        native.providerDialogueBaseline, native.providerDialogueNativeTarget);
                    if (scope.nativeProvider) {
                        auto& tracker = *scope.nativeProvider;
                        native.providerDialogueInputAlreadyCorrected = tracker.MatchesCorrectedDialogueSetterSample(
                            raw.receiver, native.threadId, raw.generation,
                            tracker.Activation().dialogueSettingsEpoch, native.incomingFov);
                        // After the paired blend finishes, only its validated
                        // native endpoint can still request the forward Dialogue
                        // transform. A departing writer value may already be in
                        // setter space, even if it no longer matches that sample.
                        native.dialogueCorrectionEnabled = mapProviderContinuation &&
                            tracker.BlocksGameplayBaselineAdmission(raw.receiver, native.threadId,
                                raw.generation, tracker.Activation().dialogueSettingsEpoch,
                                native.incomingFov, native.recoveryEndpointTolerance);
                    } else {
                        native.providerDialogueInputAlreadyCorrected = scope.preparedDialogueFov &&
                            scope.preparedDialogueFov->receiver == raw.receiver &&
                            scope.preparedDialogueFov->generation == raw.generation &&
                            std::fabs(scope.preparedDialogueFov->value - raw.fov) <= 0.001;
                    }
                }
                receipt.physical = detail::ProcessGameplayPhysicalWriter(w.gameplayPhysical, native, *event.engine);
                const auto& physical = *receipt.physical;
                if (admitted.accepted) {
                    if (!physical.observationPublished) {
                        w.baseline = baselineBeforeAdmission;
                        final.baseline = baselineAction;
                        final.admission = GameplayAdmission::Unknown;
                    } else if (w.baseline) w.baseline->aspect = physical.aspect;
                }
                // Physical helpers only propose the scalar argument. Applied
                // in their legacy result names never claims a native FOV store.
                if ((owner == CorrectionOwner::Gameplay || owner == CorrectionOwner::Dialogue) &&
                    physical.applied && !physical.modeTransitionPending)
                    proposal = WriterProposal{physical.outputFov, false};
            } else if (nativeProviderOutput && owner == CorrectionOwner::Dialogue) {
                proposal = WriterProposal{*nativeProviderOutput, false, false,
                    std::fabs(*nativeProviderOutput - native.incomingFov) > 0.0001f};
            } else if ((physicalRecovery || lateRecoveryFrame) && owner == CorrectionOwner::Recovery) {
                if (recoveryEffect.argument) proposal = WriterProposal{*recoveryEffect.argument, false};
            } else if (owner != CorrectionOwner::PassThrough && event.evaluate)
                proposal = event.evaluate(event.policyContext,
                    {owner, final.selectedPolicy, w.baseline, raw.fov, raw.fovProvenance,
                     current == w.scopes.end() ? std::optional<ScopeState>{} : current->second,
                     raw.receiver, current != w.scopes.end() && raw.outputObject &&
                        current->second.cinematicAspectSource == raw.receiver &&
                        !current->second.cinematicAssociationConflict && raw.evidence.sourceRelationValidated,
                     true});
            if (owner == CorrectionOwner::Cinematic && current != w.scopes.end()) {
                const auto progress = current->second.cinematicActivation.ObserveWriter(
                    current->second.writerSourceAdmitted, raw.generation,
                    w.settings.cinematicEpoch, raw.receiver, raw.outputObject,
                    static_cast<float>(raw.fov), static_cast<std::uint32_t>(raw.thread),
                    static_cast<float>(raw.aspect), raw.flags, w.settings.gameplayEpoch);
                final.cinematicWriterAdmitted = current->second.writerSourceAdmitted;
                final.cinematicActivationStatus = progress.status;
                if (progress.status == cinematics::EnterActivationStatus::MatchedSample ||
                    progress.status == cinematics::EnterActivationStatus::CompletedEndpoint) {
                    if (std::isfinite(progress.mappedOutput)) proposal = WriterProposal{progress.mappedOutput, false,
                        proposal && proposal->endCinematicInitialEstablishment, true,
                        cinematics::CinematicWriterAction::RetainMappedActivationSample};
                    else proposal.reset();
                }
            }
            if (state_.revision + 1 != final.revision) return fail();
            if (proposal && (!std::isfinite(proposal->outputFov) ||
                proposal->outputFov <= 1.0 || proposal->outputFov >= 179.0)) proposal.reset();
            if (proposal && proposal->gameplayBaselineDependent && (!w.baseline ||
                w.baseline->source != raw.scope || w.baseline->receiver != raw.receiver ||
                w.baseline->gameplaySettingsEpoch != w.settings.gameplayEpoch ||
                (w.baseline->trust != BaselineTrust::Current &&
                 w.baseline->trust != BaselineTrust::Retained))) proposal.reset();

            receipt.owner_ = this;
            receipt.outputObject_ = raw.outputObject;
            receipt.writerInvocationId = invocation;
            receipt.diagnosticFrameId = event.diagnosticFrameId;
            receipt.permission_ = proposal && proposal->mutateArgument;
            receipt.output_ = proposal ? proposal->outputFov : raw.fov;
            receipt.revision_ = final.revision;
            receipt.invocation_ = invocation;
            if (proposal) {
                final.cinematicAction = proposal->cinematicAction;
                if (proposal->endCinematicInitialEstablishment)
                    current->second.cinematicInitialEstablishmentActive = false;
                auto witness = raw;
                witness.transactionId = invocation;
                witness.fov = proposal->outputFov;
                witness.fovProvenance = proposal->mutateArgument
                    ? FovProvenance::ModCorrectedOutput : raw.fovProvenance;
                witness.fovBoundary = FovBoundary::WriterDecision;
                current->second.plannedOutputFov = MakeFovWitness(witness);
                if (event.observeOutput) {
                    for (auto it = w.observations.begin(); it != w.observations.end(); )
                        if (it->second.postStoreResult && it->second.postHelperResult)
                            it = w.observations.erase(it); else ++it;
                    if (w.observations.size() < MaxEffects) {
                        const auto id = w.nextAuthorityEventId++;
                        EffectIntent observation;
                        observation.id = id;
                        observation.scope = raw.scope;
                        observation.receiver = raw.receiver;
                        observation.generation = raw.generation;
                        observation.transactionId = invocation;
                        observation.outputObject = raw.outputObject;
                        observation.thread = raw.thread;
                        observation.settingsEpoch = w.settings.globalEpoch;
                        observation.dependencyRevision = final.revision;
                        observation.plannedOutputFov = proposal->outputFov;
                        w.observations[id] = observation;
                        receipt.observationId = id;
                        receipt.observation_ = id;
                    } else receipt.observationUnavailable = true;
                }
            }
            // Exactly one externally visible lifecycle commit. Internal step
            // revisions are not exported as dependencies or current snapshots.
            // Reject unexpected reentrant mutation rather than overwriting it.
            if (state_.revision + 1 != final.revision) return fail();
            if (receipt.physical) {
                auto& publication = *receipt.physical;
                publication.observation.publicationSequence = invocation;
                // Local delegate stores are implementation scratch, never a
                // second reference publisher. Export only the final Authority
                // reference and the current invocation's observation identity.
                publication.baselineProjection = {};
                publication.baselineProjection.eligible = final.baseline == BaselineAction::Update;
                if (w.baseline) {
                    const auto& reference = *w.baseline;
                    auto& projected = publication.baselineProjection.baseline;
                    projected.valid = true;
                    projected.current = reference.trust == BaselineTrust::Current ||
                        reference.trust == BaselineTrust::Retained;
                    projected.nativeFov = {static_cast<float>(reference.fov), camera::FovSpace::Native,
                        camera::FovProvenance::NativeRegisterInput, true};
                    projected.source = {reference.receiver, reference.receiver != 0};
                    projected.aspect = static_cast<float>(reference.aspect);
                    projected.observationSequence = reference.observationSequence;
                    if (reference.preparedFov)
                        projected.horPlusFov = {static_cast<float>(reference.preparedFov->value),
                            camera::FovSpace::Transformed, camera::FovProvenance::ModTransformResult, true};
                }
            }
            if (!w.cinematicSelectionActive && final.baseline == BaselineAction::Update)
                w.cinematicCarry.reset();
            w.revision = final.revision;
            w.lastWriterScope = raw.scope;
            w.lastWriterReceiver = raw.receiver;
            state_ = std::move(w);
            receipt.decision = std::move(final);
            return receipt;
        }

        bool PrepareWriterArgument(WriterInvocation& invocation, double& argument)
        {
            // Consumed even on refusal. No execution permission is stored in a
            // map or retained by Authority after the caller's invocation ends.
            const bool granted = std::exchange(invocation.permission_, false);
            // A pass-through argument can still have a native store observation
            // window. Let the port finalize it, or the receipt destructor abandon
            // it; absence of a mutation grant is not a failed native invocation.
            if (!granted) return false;
            if (invocation.owner_ != this ||
                invocation.revision_ != state_.revision || state_.stopping ||
                invocation.invocation_ + 1 != state_.nextWriterInvocationId) {
                if (invocation.owner_ == this) AbandonUnpreparedObservation(invocation);
                return false;
            }
            argument = invocation.output_;
            invocation.argumentPrepared_ = true;
            RecordFinalWriterArgument(invocation, argument, true);
            return true;
        }

        Projection Read() const { return state_; }

    private:
        friend class WriterAdapterPort;
        void BindNativeWriterFacts(EventFacts& facts) const
        {
            facts.generation = state_.cinematicGeneration;
            const auto found = state_.scopes.find(facts.scope);
            const auto* scope = found == state_.scopes.end() ? nullptr : &found->second;
            const bool recovery = scope && scope->recovery == RecoveryPhase::Pending;
            const bool providerArmed = scope && scope->activeArmingId;
            const bool dialogueOwnsSource = scope && scope->dialogue != DialoguePhase::None &&
                scope->dialogue != DialoguePhase::Candidate;
            const bool excluded = scope && scope->postCinematicExclusionActive;
            const bool unclaimed = !state_.cinematicSelectionActive &&
                !providerArmed && !dialogueOwnsSource;
            // A raw scalar can establish an original witness only after
            // checking the committed scope and post-cinematic exclusion.
            const bool mayAdmit = unclaimed && (!excluded || recovery);
            if (mayAdmit) facts.fovProvenance = FovProvenance::OriginalNative;
            facts.requestPolicyAdmission = mayAdmit;
            if (unclaimed && state_.baseline && state_.baseline->receiver != facts.receiver &&
                state_.baseline->trust != BaselineTrust::Invalid &&
                !IsQuarantined(state_, state_.baseline->source)) {
                facts.previousScope = state_.baseline->source;
                facts.requestPolicyAdmission = false;
            }
        }

        void FinalizeUnchangedWriterArgument(WriterInvocation& invocation, float argument)
        {
            if (!invocation.argumentPrepared_ && invocation.owner_ == this && !state_.stopping &&
                invocation.revision_ == state_.revision &&
                invocation.invocation_ + 1 == state_.nextWriterInvocationId &&
                argument == static_cast<float>(invocation.output_))
                RecordFinalWriterArgument(invocation, argument, false);
        }

        void RecordFinalWriterArgument(WriterInvocation& invocation, double argument, bool corrected)
        {
            invocation.argumentFinalized_ = true;
            for (auto& [source, scope] : state_.scopes) {
                if (!scope.writerInputFov || scope.writerInputFov->transactionId != invocation.writerInvocationId)
                    continue;
                scope.writerArgumentFov = scope.writerInputFov;
                scope.writerArgumentFov->value = argument;
                scope.writerArgumentFov->boundary = FovBoundary::WriterArgument;
                if (corrected) scope.writerArgumentFov->provenance = FovProvenance::ModCorrectedOutput;
                if (invocation.decision.correction == CorrectionOwner::Gameplay && state_.baseline &&
                    state_.baseline->source == source &&
                    state_.baseline->observationSequence == invocation.writerInvocationId &&
                    corrected && (!invocation.physical || invocation.physical->applied)) {
                    state_.baseline->preparedFov = scope.writerArgumentFov;
                    if (invocation.physical)
                        invocation.physical->baselineProjection.baseline.horPlusFov = {
                            static_cast<float>(argument), camera::FovSpace::Transformed,
                            camera::FovProvenance::ModTransformResult, true};
                }
                if (invocation.decision.correction == CorrectionOwner::Cinematic)
                    scope.cinematicActivation.RecordWriterResult(scope.writerSourceAdmitted,
                        scope.cinematicGeneration, scope.cinematicSettingsEpoch, scope.receiver,
                        invocation.outputObject_, scope.writerThread, invocation.writerInvocationId,
                        static_cast<float>(scope.writerInputFov->value), static_cast<float>(argument),
                        scope.writerAspect, scope.writerFlags, corrected);
                break;
            }
        }
        friend class CinematicAdapterPort;
        NativeCinematicResult Accept(const NativeCinematicEnter&);
        NativeCinematicResult Accept(const NativeCinematicAspect&);
        NativeCinematicResult Accept(const NativeCinematicArmer&);
        NativeCinematicResult Accept(const NativeCinematicExit&);
        static ScopeState* BindNativeCinematic(Projection&, ScopeId);
        friend class WriterInvocation;
        void AbandonUnpreparedObservation(const WriterInvocation& invocation) noexcept
        {
            if (invocation.owner_ == this && !invocation.argumentFinalized_ && invocation.observation_)
                state_.observations.erase(invocation.observation_);
        }

        static bool MakeScopeRoom(Projection& state)
        {
            if (state.scopes.size() < MaxScopes) return true;
            const auto reclaimable = [&](const auto& entry) {
                const auto& [id, scope] = entry;
                if (id == state.selectedCinematicScope || id == state.lastWriterScope ||
                    (state.baseline && state.baseline->source == id) ||
                    scope.cinematicActive || scope.recovery == RecoveryPhase::Pending ||
                    scope.postCinematicExclusionActive || scope.dialogue != DialoguePhase::None ||
                    scope.activeArmingId)
                    return false;
                if (std::ranges::any_of(state.observations, [id](const auto& observation) {
                    return observation.second.scope == id;
                })) return false;
                return std::ranges::none_of(state.effects, [id](const auto& effect) {
                    return effect.second.scope == id && !effect.second.revoked && !effect.second.closed;
                });
            };
            const auto retired = std::ranges::find_if(state.scopes, reclaimable);
            if (retired == state.scopes.end()) return false;
            state.scopes.erase(retired);
            return true;
        }

        static void PruneHistory(Projection& state)
        {
            // Pending observations are never evicted. Closed history is dispensable;
            // recent revoked records are retained for inspection up to the hard cap.
            for (auto it = state.armings.begin(); it != state.armings.end(); )
                if (it->second.retired) it = state.armings.erase(it); else ++it;
            for (auto it = state.effects.begin(); it != state.effects.end(); )
                if (it->second.closed)
                    it = state.effects.erase(it); else ++it;
            for (auto it = state.effects.begin(); state.effects.size() >= MaxEffects && it != state.effects.end(); )
                if (it->second.revoked || (it->second.postStoreResult && it->second.postHelperResult))
                    it = state.effects.erase(it); else ++it;
        }

        static void RevokeScopeEffects(Projection& state, ScopeId scope)
        {
            for (auto& [id, effect] : state.effects)
                if (effect.scope == scope) effect.revoked = true;
        }

        static void RetireNativeProvider(Projection& state, ScopeState& scope)
        {
            if (const auto arm = state.armings.find(scope.activeArmingId); arm != state.armings.end())
                arm->second.retired = true;
            if (scope.activeArmingId && scope.dialogue == DialoguePhase::Candidate)
                scope.dialogue = DialoguePhase::None;
            scope.activeArmingId = 0;
            scope.nativeProvider.reset();
            scope.setterMatched = scope.writerConfirmed = false;
            scope.providerEndpointReached = scope.providerAdmissionConsumed = scope.providerDialogueOwned = false;
            scope.preparedDialogueFov.reset();
        }

        static void RetireDialogueScope(Projection& state, ScopeState& scope)
        {
            RetireNativeProvider(state, scope);
            DialogueWorkingState local(scope, state.settings);
            plugin::ResetDialogueBoundaryState(local.View());
            local.activeEpoch.store(0);
            local.Store(scope);
            scope.dialoguePolicySnapshot = static_cast<std::uint32_t>(config::DialogueZoomPolicy::Native);
        }

        static void RevokeOutstandingEffects(Projection& state)
        {
            for (auto& [id, effect] : state.effects)
            {
                (void)id;
                effect.revoked = true;
            }
        }

        static bool FeatureEnabled(const EffectiveSettings& settings, CorrectionOwner owner)
        {
            if (!settings.globalEnabled) return false;
            switch (owner)
            {
            case CorrectionOwner::Gameplay: return settings.gameplayEnabled;
            case CorrectionOwner::Cinematic: return settings.cinematicEnabled;
            case CorrectionOwner::Dialogue: return settings.dialogueEnabled;
            case CorrectionOwner::Recovery:
                return settings.gameplayEnabled || settings.cinematicEnabled;
            case CorrectionOwner::PassThrough: return false;
            }
            return false;
        }

        static bool ApplySettingsMutation(Projection& state, const EventFacts& facts,
            Decision& decision)
        {
            bool changed = false;
            switch (facts.settingsDomain)
            {
            case SettingsDomain::Global:
                if (facts.settingsMutation != SettingsMutation::EnableGate) return false;
                changed = state.settings.globalEnabled != facts.enabled;
                if (changed)
                {
                    state.settings.globalEnabled = facts.enabled;
                    ++state.settings.globalEpoch;
                }
                break;
            case SettingsDomain::Gameplay:
            {
                const auto oldPolicy = state.settings.gameplayPolicy;
                if (facts.settingsMutation == SettingsMutation::EnableGate)
                {
                    changed = state.settings.gameplayEnabled != facts.enabled;
                    if (changed) {
                        state.settings.gameplayEnabled = facts.enabled;
                        state.gameplayPhysical.enablePending = facts.enabled;
                        state.gameplayPhysical.disablePending = !facts.enabled;
                        state.gameplayPhysical.enableDeferralLogged = false;
                        if (!facts.enabled) {
                            state.gameplayPhysical.modePending = false;
                            state.gameplayPhysical.modeDeferralLogged = false;
                        }
                    }
                }
                else
                {
                    changed = state.settings.gameplayPolicy != facts.policySelection;
                    if (changed) {
                        state.settings.gameplayPolicy = facts.policySelection;
                        auto& physical = state.gameplayPhysical;
                        physical.modeDeferralLogged = false;
                        if (!state.settings.gameplayEnabled) physical.modePending = false;
                        else if (oldPolicy == static_cast<unsigned>(config::GameplayMode::AspectRecalculation) &&
                            facts.policySelection == static_cast<unsigned>(config::GameplayMode::HorPlus)) {
                            physical.replay = gameplay::ReplayState::WaitingForAutomaticUpdate;
                            physical.autoRestoreSource = 0;
                            physical.modeTarget = config::GameplayMode::HorPlus;
                            physical.modePending = true;
                        } else if (facts.policySelection == static_cast<unsigned>(config::GameplayMode::AspectRecalculation)) {
                            physical.modePending = false;
                            physical.replay = gameplay::ReplayState::WaitingForAutomaticUpdate;
                            physical.autoRestoreSource = physical.lastSource = 0;
                            physical.lastFov = NAN;
                            physical.restoration = {};
                        } else if (facts.policySelection == static_cast<unsigned>(config::GameplayMode::RuntimeCombine))
                            physical.modePending = false;
                    }
                }
                if (changed)
                {
                    ++state.settings.gameplayEpoch;
                    ++state.settings.globalEpoch;
                    const bool supportedMode = state.settings.gameplayPolicy <=
                        static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
                    if (facts.settingsMutation == SettingsMutation::PolicySelection &&
                        (!supportedMode || (state.settings.gameplayEnabled &&
                         state.settings.gameplayPolicy == static_cast<unsigned>(config::GameplayMode::AspectRecalculation)))) {
                        if (state.baseline) state.baseline->trust = BaselineTrust::Invalid;
                        decision.baseline = BaselineAction::Invalidate;
                    } else if (state.baseline &&
                        (state.baseline->trust == BaselineTrust::Current ||
                         state.baseline->trust == BaselineTrust::Retained)) {
                        // Enable gates revoke execution, not the independent
                        // native numeric reference consumed by Cinematic policy.
                        state.baseline->gameplaySettingsEpoch = state.settings.gameplayEpoch;
                        decision.baseline = BaselineAction::Retain;
                    }
                }
                break;
            }
            case SettingsDomain::Cinematic:
                if (facts.settingsMutation == SettingsMutation::EnableGate)
                {
                    changed = state.settings.cinematicEnabled != facts.enabled;
                    if (changed) state.settings.cinematicEnabled = facts.enabled;
                }
                else if (facts.settingsMutation == SettingsMutation::AspectPolicySelection)
                {
                    const auto policy = static_cast<config::CinematicAspectPolicy>(facts.policySelection);
                    changed = state.settings.cinematicAspectPolicy != policy;
                    if (changed) state.settings.cinematicAspectPolicy = policy;
                }
                else
                {
                    changed = state.settings.cinematicPolicy != facts.policySelection;
                    if (changed) state.settings.cinematicPolicy = facts.policySelection;
                }
                if (changed)
                {
                    if (facts.settingsMutation == SettingsMutation::EnableGate)
                        ++state.settings.cinematicEpoch;
                    if (facts.settingsMutation == SettingsMutation::EnableGate && !facts.enabled) {
                        for (auto& [id, scope] : state.scopes) {
                            scope.cinematicCorrectionArmed = false;
                            scope.cinematicTargetFov = NAN;
                            scope.cinematicActivation.Retire();
                        }
                        if (state.pendingCinematic) {
                            state.pendingCinematic->cinematicCorrectionArmed = false;
                            state.pendingCinematic->cinematicTargetFov = NAN;
                            state.pendingCinematic->cinematicActivation.Retire();
                        }
                    }
                    ++state.settings.globalEpoch;
                }
                break;
            case SettingsDomain::Dialogue:
                if (facts.settingsMutation == SettingsMutation::EnableGate)
                {
                    changed = state.settings.dialogueEnabled != facts.enabled;
                    if (changed) state.settings.dialogueEnabled = facts.enabled;
                    if (changed && !facts.enabled)
                        for (auto& [id, scope] : state.scopes) {
                            scope.dialogueCorrectionArmed = false;
                            scope.dialogueSettingsEpoch = 0;
                            scope.preparedDialogueFov.reset();
                        }
                }
                else
                {
                    changed = state.settings.dialoguePolicy != facts.policySelection;
                    if (changed) state.settings.dialoguePolicy = facts.policySelection;
                }
                if (changed)
                {
                    ++state.settings.dialogueEpoch;
                    ++state.settings.globalEpoch;
                }
                break;
            }
            if (!changed) return false;
            RevokeOutstandingEffects(state);
            decision.correction = CorrectionOwner::PassThrough;
            return true;
        }

        static bool IsQuarantined(const Projection& state, ScopeId source)
        {
            // A confirmed native ENTER precedes receiver discovery. During
            // that actual primary-view selection, an unbound/foreign writer is
            // not an independent Gameplay admission. A Candidate never creates
            // this global selection guard; EXIT ends it even without a writer.
            if (state.cinematicSelectionActive) return true;
            const auto it = state.scopes.find(source);
            if (it == state.scopes.end()) return false;
            const auto& scope = it->second;
            return scope.cinematicActive || scope.recovery == RecoveryPhase::Pending ||
                scope.dialogue == DialoguePhase::Active || scope.activeArmingId != 0;
        }

        static FovWitness MakeFovWitness(const EventFacts& facts)
        {
            return FovWitness{facts.fovProvenance, facts.fovBoundary, facts.scope,
                facts.receiver, facts.generation, facts.transactionId, facts.fov};
        }

        static CorrectionOwner ResolveCorrectionOwner(const Projection& state,
            ScopeId source, std::uint64_t receiver, const ScopeState& scope)
        {
            if (scope.receiver != receiver) return CorrectionOwner::PassThrough;
            if (scope.dialogue == DialoguePhase::Active) return CorrectionOwner::Dialogue;
            if (scope.cinematicActive) return CorrectionOwner::Cinematic;
            if (scope.recovery == RecoveryPhase::Pending) return CorrectionOwner::Recovery;
            if (state.baseline && state.baseline->source == source &&
                state.baseline->receiver == receiver &&
                state.baseline->gameplaySettingsEpoch == state.settings.gameplayEpoch &&
                state.baseline->trust == BaselineTrust::Current)
                return CorrectionOwner::Gameplay;
            return CorrectionOwner::PassThrough;
        }

        static void RetainCompatibleBaseline(Projection& state, ScopeId source, std::uint64_t receiver)
        {
            if (!state.baseline) return;
            if (state.baseline->source == source && state.baseline->receiver == receiver &&
                state.baseline->gameplaySettingsEpoch == state.settings.gameplayEpoch &&
                state.baseline->trust == BaselineTrust::Current)
                state.baseline->trust = BaselineTrust::Retained;
        }

        Projection state_{};
    };

    // Authority outlives its invocation receipts; adapters keep these local to
    // the PRE callback. Successful preparation transfers only optional readback
    // tracking beyond this scope, never a reusable execution permission.
    inline WriterInvocation::~WriterInvocation()
    {
        if (owner_ && !argumentFinalized_) owner_->AbandonUnpreparedObservation(*this);
        permission_ = false;
    }
}
