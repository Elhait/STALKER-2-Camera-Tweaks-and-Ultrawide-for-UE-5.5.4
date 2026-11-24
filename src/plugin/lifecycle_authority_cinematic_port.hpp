#pragma once

#include "lifecycle_authority_core.hpp"
#include "../cinematics/cinematic_aspect.hpp"
#include <bit>

namespace plugin::lifecycle_authority
{
    struct NativeCinematicEnter
    { float argument{}; float viewportAspect{16.0f / 9.0f}; bool validated{}; std::uint32_t thread{}; };
    struct NativeCinematicAspect { std::uintptr_t target{}; float viewportAspect{16.0f / 9.0f}; bool validated{}; };
    struct NativeCinematicArmer
    {
        std::uintptr_t component{}, receiver{};
        std::uint32_t thread{};
        float start{}, target{}, receiverFov{}, aspect{}, viewportAspect{};
        std::uint8_t flags{};
        bool validated{}, fieldsReadable{};
    };
    struct NativeCinematicExit { float argument{}; bool validated{}, recoveryObserverAvailable{}; };
    struct NativeCinematicResult
    {
        Decision decision;
        std::uint64_t generation{};
        float argument{NAN};
        bool prepareArgument{}, effectAttempted{}, effectSucceeded{};
        bool continuedOwnership{};
        float aspect{NAN};
        config::CinematicAspectPolicy aspectPolicy{config::CinematicAspectPolicy::Native};
        config::CinematicFovMode fovMode{config::CinematicFovMode::NativeHorPlus};
        cinematics::CinematicBaselineSelection reference;
        cinematics::EnterActivationSnapshot activation;
        cinematics::AdmittedWriterOutput lastAdmittedOutput;
        std::uintptr_t aspectSource{};
        bool associationConflict{}, writerObserved{};
        TransactionId reentryWriterBefore{}, reentryWriterAfter{};
        const char* inheritedStartStatus{"not-evaluated"};
    };

    inline NativeCinematicResult Authority::Accept(const NativeCinematicEnter& input)
    {
        NativeCinematicResult result;
        result.argument = input.argument;
        if (!input.validated || !state_.ready || state_.stopping || state_.cinematicSelectionActive) return result;
        auto w = state_;
        const bool enabled = w.settings.globalEnabled && w.settings.cinematicEnabled;
        const auto aspectPolicy = enabled ? w.settings.cinematicAspectPolicy : config::CinematicAspectPolicy::Native;
        const auto fovMode = enabled ? static_cast<config::CinematicFovMode>(w.settings.cinematicPolicy)
            : config::CinematicFovMode::NativeHorPlus;
        const float autoAspect = plugin::IsValidCameraWriterAspect(input.viewportAspect)
            ? input.viewportAspect : 16.0f / 9.0f;
        const float aspect = cinematics::ResolveAspect(aspectPolicy, 16.0f / 9.0f,
            cinematics::Forced21x9Aspect, 32.0f / 9.0f, autoAspect);
        camera::GameplayBaseline reference;
        if (w.baseline) {
            reference.valid = reference.current = w.baseline->trust == BaselineTrust::Current ||
                w.baseline->trust == BaselineTrust::Retained;
            reference.nativeFov = {static_cast<float>(w.baseline->fov), camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput, true};
            reference.aspect = static_cast<float>(w.baseline->aspect);
            reference.source = {w.baseline->receiver, w.baseline->receiver != 0};
            if (w.baseline->preparedFov)
                reference.horPlusFov = {static_cast<float>(w.baseline->preparedFov->value),
                    camera::FovSpace::Transformed, camera::FovProvenance::ModTransformResult, true};
        }
        result.reference = cinematics::SelectCinematicBaseline(enabled &&
            fovMode == config::CinematicFovMode::GameplayHorPlus, input.argument, reference);
        result.prepareArgument = enabled && aspectPolicy != config::CinematicAspectPolicy::Native &&
            cinematics::TryTransformSelectedCinematicFov(true, fovMode, input.argument, input.argument,
                result.reference.targetBaselineFov, aspect, 16.0f / 9.0f, result.argument);
        ScopeState next;
        next.cinematicActive = true;
        next.cinematicCorrectionArmed = enabled;
        next.cinematicGeneration = ++w.cinematicGeneration;
        next.thread = input.thread;
        next.cinematicAspectPolicy = aspectPolicy;
        next.cinematicPolicySnapshot = static_cast<unsigned>(fovMode);
        next.cinematicAspect = result.aspect = aspect;
        next.cinematicSettingsEpoch = w.settings.cinematicEpoch;
        next.cinematicAuthoredFov = input.argument;
        if (plugin::IsValidCameraWriterFov(input.argument))
            next.originalFov = FovWitness{FovProvenance::OriginalNative, FovBoundary::CinematicEnter,
                0, 0, next.cinematicGeneration, 0, input.argument};
        next.cinematicTargetFov = result.prepareArgument ? result.argument : NAN;
        if (w.cinematicCarry) {
            next.cinematicActivation = *w.cinematicCarry;
            next.cinematicPredecessor = true;
        }
        if (enabled && camera::IsUsableGameplayBaseline(reference)) {
            next.cinematicBaselineFov = reference.nativeFov.value;
            next.cinematicReference = FovWitness{FovProvenance::OriginalNative, FovBoundary::GameplayWriterInput,
                w.baseline->source, w.baseline->receiver, 0, w.baseline->observationSequence, w.baseline->fov};
        }
        if (const auto previous = w.scopes.find(w.selectedCinematicScope); previous != w.scopes.end()) {
            if (!w.cinematicCarry) {
                next.cinematicActivation = previous->second.cinematicActivation;
                next.cinematicPredecessor = previous->second.recovery == RecoveryPhase::Pending;
            }
            if (previous->second.cinematicAssociationConflict) next.cinematicActivation.Retire();
            const auto& prior = previous->second;
            const gameplay::CinematicReentrySample reentry{
                camera::CoordinatorState::CinematicExiting,
                prior.cinematicReference ? prior.cinematicReference->receiver : 0,
                reference.source.value,
                static_cast<float>(prior.exitNativeTarget),
                static_cast<float>(prior.cinematicTargetFov),
                input.argument,
                result.argument,
                prior.recovery == RecoveryPhase::Pending && prior.postCinematicExclusionActive,
                result.prepareArgument
            };
            const bool selectionUnchanged = prior.cinematicAspectPolicy == aspectPolicy &&
                prior.cinematicPolicySnapshot == static_cast<unsigned>(fovMode);
            if (enabled && gameplay::IsCinematicReferenceContinuation(reentry, selectionUnchanged,
                    result.reference.usedGameplayBaselineValue ? result.reference.targetBaselineFov : NAN)) {
                next.cinematicContinuationSource = reentry.currentSource;
                next.cinematicContinuationStart = reentry.previousExitNativeTarget;
                result.continuedOwnership = true;
            }
            RevokeScopeEffects(w, previous->first);
            previous->second.cinematicActive = false;
            previous->second.recovery = RecoveryPhase::None;
            previous->second.postCinematicExclusionActive = false;
        }
        if (const auto prior = w.scopes.find(w.lastWriterScope); prior != w.scopes.end()) {
            auto& scope = prior->second;
            const bool dialogueActive = scope.dialogue == DialoguePhase::Active || scope.dialogue == DialoguePhase::Exiting;
            auto& start = next.cinematicDialogueStart;
            start.dialogueContextActive = dialogueActive;
            if (dialogueActive && scope.dialogueCorrectionArmed && scope.writerArgumentFov &&
                scope.writerInputFov && scope.writerThread &&
                scope.dialogueSettingsEpoch == w.settings.dialogueEpoch &&
                scope.writerInputFov->generation == next.cinematicGeneration - 1 &&
                scope.writerArgumentFov->transactionId == scope.writerInputFov->transactionId) {
                start.receiver = scope.receiver;
                start.dialogueSettingsEpoch = scope.dialogueSettingsEpoch;
                start.activationStartSample = static_cast<float>(scope.writerInputFov->value);
                start.correctedStart = static_cast<float>(scope.writerArgumentFov->value);
                start.dialogueGeneration = scope.writerInputFov->generation;
                start.writerSequence = scope.writerArgumentFov->transactionId;
                start.writerThreadId = scope.writerThread;
                start.writerInputFov = start.activationStartSample;
                start.writerAspect = start.writerPolicyAspect = scope.writerAspect;
                start.writerFlags = scope.writerFlags;
                start.writerFlagsValid = true;
                start.valid = true;
            }
        }
        // ENTER transfers the foreground even before its receiver is bound.
        // An interleaved writer can change the latest sample, but cannot
        // carry a prior Dialogue claim across that explicit transition.
        for (auto& [id, scope] : w.scopes) {
            if (scope.dialogue == DialoguePhase::None && !scope.activeArmingId && !scope.nativeProvider)
                continue;
            RevokeScopeEffects(w, id);
            RetireDialogueScope(w, scope);
        }
        next.cinematicActivation.Arm(next.cinematicGeneration, next.cinematicSettingsEpoch,
            reference.source.value, reference.nativeFov.value, reference.horPlusFov.value,
            result.prepareArgument && camera::IsUsableGameplayBaseline(reference) && reference.horPlusFov.valid,
            static_cast<float>(next.cinematicTargetFov), next.cinematicDialogueStart,
            next.cinematicPredecessor, aspect, w.settings.gameplayEpoch);
        w.pendingCinematic = std::move(next);
        w.cinematicCarry.reset();
        w.selectedCinematicScope = 0;
        w.cinematicSelectionActive = true;
        w.firstPresentableWriterInvocation = w.nextWriterInvocationId;
        if (w.baseline && w.baseline->trust == BaselineTrust::Current) w.baseline->trust = BaselineTrust::Retained;
        result.generation = w.cinematicGeneration;
        result.aspectPolicy = aspectPolicy;
        result.fovMode = fovMode;
        result.decision.accepted = true;
        result.decision.revision = ++w.revision;
        result.decision.reason = "native-enter-accepted-before-receiver-binding";
        PruneHistory(w);
        state_ = std::move(w);
        return result;
    }

    inline ScopeState* Authority::BindNativeCinematic(Projection& w, ScopeId receiver)
    {
        if (!receiver || !w.cinematicSelectionActive) return nullptr;
        if (w.pendingCinematic) {
            if (!w.scopes.contains(receiver) && !MakeScopeRoom(w)) return nullptr;
            RevokeScopeEffects(w, receiver);
            if (auto old = w.scopes.find(receiver); old != w.scopes.end() && old->second.activeArmingId)
                w.armings[old->second.activeArmingId].retired = true;
            w.scopes[receiver] = std::move(*w.pendingCinematic);
            w.pendingCinematic.reset();
            w.selectedCinematicScope = receiver;
            w.scopes.at(receiver).receiver = receiver;
        }
        const auto scope = w.scopes.find(w.selectedCinematicScope);
        return scope == w.scopes.end() ? nullptr : &scope->second;
    }

    inline NativeCinematicResult Authority::Accept(const NativeCinematicAspect& input)
    {
        NativeCinematicResult result;
        result.argument = 16.0f / 9.0f;
        if (!input.validated || !state_.ready || state_.stopping || !input.target) return result;
        auto w = state_;
        // Armer receiver and aspect-store target are distinct native facts.
        // Before any writer/association, the first aspect store selects the
        // policy source; the captured blend retains its own receiver check.
        if (!w.pendingCinematic && w.selectedCinematicScope != input.target) {
            auto selected = w.scopes.find(w.selectedCinematicScope);
            if (selected != w.scopes.end() && !selected->second.cinematicAspectSource &&
                !selected->second.writerInputFov) {
                auto activation = std::move(selected->second);
                w.scopes.erase(selected);
                activation.receiver = input.target;
                RevokeScopeEffects(w, input.target);
                if (const auto replaced = w.scopes.find(input.target); replaced != w.scopes.end()) {
                    if (const auto arm = w.armings.find(replaced->second.activeArmingId); arm != w.armings.end())
                        arm->second.retired = true;
                }
                w.scopes[input.target] = std::move(activation);
                w.selectedCinematicScope = input.target;
            }
        }
        auto* scope = BindNativeCinematic(w, input.target);
        if (!scope) return result;
        if (scope->cinematicAspectSource && scope->cinematicAspectSource != input.target)
            scope->cinematicAssociationConflict = true;
        else scope->cinematicAspectSource = input.target;
        const bool enabled = w.settings.globalEnabled && w.settings.cinematicEnabled && scope->cinematicCorrectionArmed;
        result.aspectPolicy = enabled ? scope->cinematicAspectPolicy : config::CinematicAspectPolicy::Native;
        result.argument = cinematics::ResolveAspect(result.aspectPolicy, 16.0f / 9.0f,
            cinematics::Forced21x9Aspect, 32.0f / 9.0f,
            plugin::IsValidCameraWriterAspect(input.viewportAspect) ? input.viewportAspect : 16.0f / 9.0f);
        result.generation = scope->cinematicGeneration;
        result.prepareArgument = true;
        result.decision.accepted = true;
        result.decision.reason = "native-aspect-source-bound";
        result.decision.revision = ++w.revision;
        PruneHistory(w);
        state_ = std::move(w);
        return result;
    }

    inline NativeCinematicResult Authority::Accept(const NativeCinematicArmer& input)
    {
        NativeCinematicResult result;
        if (!input.validated || !input.fieldsReadable || !input.component || !input.receiver || !input.thread ||
            !state_.ready || state_.stopping || !plugin::IsValidCameraWriterFov(input.start) ||
            !plugin::IsValidCameraWriterFov(input.target)) return result;
        auto w = state_;
        auto* scope = BindNativeCinematic(w, input.receiver);
        if (!scope || (scope->thread && scope->thread != input.thread)) return result;
        auto start = scope->cinematicActivation.InheritedWriterStart(input.receiver, input.start, input.aspect,
            input.flags, input.thread, w.settings.gameplayEpoch, w.settings.dialogueEpoch,
            input.component, &result.inheritedStartStatus, input.target, input.receiverFov);
        if (!start.cinematicContextActive && !scope->cinematicDialogueStart.dialogueContextActive &&
            plugin::IsValidCameraWriterFov(input.receiverFov) && std::fabs(input.start - input.receiverFov) <= 0.05f) {
            float corrected = input.receiverFov;
            bool usable = true;
            const auto mode = static_cast<config::GameplayMode>(w.settings.gameplayPolicy);
            if (w.settings.gameplayEnabled && (mode == config::GameplayMode::HorPlus || mode == config::GameplayMode::RuntimeCombine)) {
                usable = plugin::IsValidCameraWriterAspect(input.aspect) && (input.flags == 4 || input.flags == 5) &&
                    (mode != config::GameplayMode::RuntimeCombine ||
                        (plugin::IsValidCameraWriterAspect(input.viewportAspect) &&
                         std::fabs(input.viewportAspect - input.aspect) <= 0.001f));
                const auto mapped = gameplay::EvaluateHorPlus(input.receiverFov, input.aspect, input.flags, 16.0f / 9.0f);
                usable = usable && mapped.applied;
                corrected = mapped.outputFov;
            }
            if (usable) {
                start.valid = true;
                start.receiver = input.receiver;
                start.activationStartSample = input.start;
                start.correctedStart = corrected;
                start.dialogueSettingsEpoch = w.settings.dialogueEpoch;
            }
        }
        result.reentryWriterBefore = scope->cinematicActivation.ReentryWriterSequence();
        if (!scope->cinematicActivation.Capture(true, scope->cinematicGeneration,
            scope->cinematicSettingsEpoch, input.component, input.receiver, input.thread,
            input.start, input.target, w.settings.dialogueEpoch, start)) return result;
        result.activation = scope->cinematicActivation.Read();
        result.decision.accepted = true;
        result.decision.authorityEventId = w.nextAuthorityEventId++;
        scope->writerInputFov.reset();
        scope->writerArgumentFov.reset();
        scope->plannedOutputFov.reset();
        scope->postStoreFov.reset();
        scope->postHelperFov.reset();
        scope->writerSourceAdmitted = false;
        result.decision.reason = "native-enter-armer-bound-to-selection";
        result.decision.revision = ++w.revision;
        result.generation = scope->cinematicGeneration;
        PruneHistory(w);
        state_ = std::move(w);
        return result;
    }

    inline NativeCinematicResult Authority::Accept(const NativeCinematicExit& input)
    {
        NativeCinematicResult result;
        result.argument = input.argument;
        if (!input.validated || !state_.ready || state_.stopping || !state_.cinematicSelectionActive) return result;
        auto w = state_;
        const auto current = w.scopes.find(w.selectedCinematicScope);
        const auto retireActivation = [&](ScopeState& scope) {
            cinematics::CinematicWriterAssociationSnapshot association;
            association.generationActive = true;
            association.generation = scope.cinematicGeneration;
            association.aspectStoreTargetObject = scope.cinematicAspectSource;
            association.associationConflict = scope.cinematicAssociationConflict;
            association.writerObserved = scope.writerInputFov.has_value();
            association.associationValid = scope.lastAdmittedOutput.valid;
            association.lastAdmittedOutput = scope.lastAdmittedOutput;
            if (scope.writerInputFov) {
                association.latestWriter.sequence = scope.writerInputFov->transactionId;
            }
            result.aspectSource = association.aspectStoreTargetObject;
            result.associationConflict = association.associationConflict;
            result.writerObserved = association.writerObserved;
            result.lastAdmittedOutput = association.lastAdmittedOutput;
            result.reentryWriterBefore = scope.cinematicActivation.ReentryWriterSequence();
            scope.cinematicActivation.RetireForReentry(association);
            result.reentryWriterAfter = scope.cinematicActivation.ReentryWriterSequence();
        };
        if (current != w.scopes.end()) {
            auto& scope = current->second;
            RevokeScopeEffects(w, current->first);
            scope.cinematicActive = false;
            const bool admittedWriter = scope.lastAdmittedOutput.valid && !scope.cinematicAssociationConflict &&
                scope.lastAdmittedOutput.generation == scope.cinematicGeneration &&
                scope.lastAdmittedOutput.source == scope.cinematicAspectSource &&
                scope.lastAdmittedOutput.output != 0;
            scope.recovery = input.recoveryObserverAvailable && admittedWriter &&
                plugin::IsValidCameraWriterFov(input.argument) ? RecoveryPhase::Pending : RecoveryPhase::None;
            scope.exitNativeTarget = input.argument;
            scope.postCinematicExclusionActive = scope.recovery == RecoveryPhase::Pending;
            scope.atomicExitPending = scope.postCinematicExclusionActive && w.settings.gameplayEnabled &&
                w.settings.gameplayPolicy == static_cast<unsigned>(config::GameplayMode::AspectRecalculation);
            scope.atomicExitPreviousFov = NAN;
            retireActivation(scope);
            if (input.recoveryObserverAvailable &&
                (!admittedWriter || !plugin::IsValidCameraWriterFov(input.argument)))
                w.cinematicCarry = scope.cinematicActivation;
            // Empty generations do not invent a recovery owner or refresh B.
            // An accepted subsequent ENTER still validates any retained start.
        }
        else if (w.pendingCinematic && input.recoveryObserverAvailable) {
            retireActivation(*w.pendingCinematic);
            w.cinematicCarry = w.pendingCinematic->cinematicActivation;
        }
        w.pendingCinematic.reset();
        w.cinematicSelectionActive = false;
        w.firstPresentableWriterInvocation = w.nextWriterInvocationId;
        result.generation = w.cinematicGeneration;
        result.decision.accepted = true;
        result.decision.reason = "native-exit-selection-retired";
        result.decision.revision = ++w.revision;
        PruneHistory(w);
        state_ = std::move(w);
        return result;
    }

    class CinematicAdapterPort final
    {
    public:
        NativeCinematicResult Enter(const NativeCinematicEnter& input, float& argument) const
        {
            if (!gate_->IsHeldByCurrentThread() ||
                std::bit_cast<std::uint32_t>(input.argument) != std::bit_cast<std::uint32_t>(argument)) return {};
            auto result = owner_->Accept(input);
            if (result.decision.accepted && result.prepareArgument) argument = result.argument;
            return result;
        }
        NativeCinematicResult Aspect(const NativeCinematicAspect& input,
            const plugin::CameraWriterEngineOperations& engine) const
        {
            if (!gate_->IsHeldByCurrentThread()) return {};
            auto result = owner_->Accept(input);
            if (result.decision.accepted && result.prepareArgument) {
                result.effectAttempted = true;
                result.effectSucceeded = plugin::RequestCameraWriterAspectOnly(engine, input.target, result.argument);
            }
            return result;
        }
        NativeCinematicResult Armer(const NativeCinematicArmer& input) const
        { return gate_->IsHeldByCurrentThread() ? owner_->Accept(input) : NativeCinematicResult{}; }
        NativeCinematicResult Exit(const NativeCinematicExit& input) const
        { return gate_->IsHeldByCurrentThread() ? owner_->Accept(input) : NativeCinematicResult{}; }
    private:
        friend class Authority;
        CinematicAdapterPort(Authority& owner, const plugin::CameraCorrectionGate& gate) : owner_(&owner), gate_(&gate) {}
        Authority* owner_;
        const plugin::CameraCorrectionGate* gate_;
    };
    inline CinematicAdapterPort Authority::CinematicBoundary(const plugin::CameraCorrectionGate& gate) noexcept
    { return CinematicAdapterPort(*this, gate); }
}
