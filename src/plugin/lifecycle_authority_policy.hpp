#pragma once

// Isolated policy binding. No native memory access and no lifecycle mutation.
#include "lifecycle_authority_core.hpp"
#include "../cinematics/cinematic_fov.hpp"
#include "../camera/horplus.hpp"
#include "../camera/fov_semantics.hpp"
#include "../gameplay/horplus_gameplay.hpp"
#include "../gameplay/gameplay_state.hpp"

namespace plugin::lifecycle_authority
{
    struct PolicyFields
    {
        float aspect{16.0f / 9.0f};
        float nativeAspect{16.0f / 9.0f};
        std::uint8_t flags{4};
    };

    inline std::optional<WriterProposal> EvaluateProductionPolicy(
        const PolicyFields& fields, const WriterPolicyContext& context)
    {
        if (context.owner == CorrectionOwner::Gameplay) {
            const auto mode = static_cast<config::GameplayMode>(context.selectedPolicy);
            if (mode != config::GameplayMode::HorPlus && mode != config::GameplayMode::RuntimeCombine)
                return {};
            const auto value = gameplay::EvaluateHorPlus(static_cast<float>(context.incomingFov),
                fields.aspect, fields.flags, fields.nativeAspect);
            if (!value.eligible) return {};
            return WriterProposal{value.outputFov, false};
        }
        if (context.owner == CorrectionOwner::Cinematic) {
            if (context.scope && context.scope->cinematicAspectPolicy == config::CinematicAspectPolicy::Native)
                return {};
            camera::GameplayBaseline reference;
            if (context.baseline) {
                reference.valid = true;
                reference.current = context.baseline->trust == BaselineTrust::Current ||
                    context.baseline->trust == BaselineTrust::Retained;
                reference.aspect = fields.aspect;
                reference.nativeFov = {static_cast<float>(context.baseline->fov), camera::FovSpace::Native,
                    camera::FovProvenance::NativeRegisterInput, true};
            }
            if (context.scope && std::isfinite(context.scope->cinematicBaselineFov)) {
                // This is the accepted activation's own numeric reference;
                // later Gameplay admission cannot rewrite its retained target.
                reference.valid = reference.current = true;
                reference.aspect = fields.aspect;
                reference.nativeFov = {static_cast<float>(context.scope->cinematicBaselineFov),
                    camera::FovSpace::Native, camera::FovProvenance::NativeRegisterInput, true};
            }
            const float authored = context.scope ? static_cast<float>(context.scope->cinematicAuthoredFov)
                : static_cast<float>(context.incomingFov);
            const auto mode = static_cast<config::CinematicFovMode>(context.selectedPolicy);
            const auto selection = cinematics::SelectCinematicBaseline(
                mode == config::CinematicFovMode::GameplayHorPlus, authored, reference);
            float target{};
            if (!cinematics::TryTransformSelectedCinematicFov(true, mode, authored,
                authored, selection.targetBaselineFov, fields.aspect, fields.nativeAspect, target)) return {};
            if (context.scope && std::isfinite(context.scope->cinematicTargetFov))
                target = static_cast<float>(context.scope->cinematicTargetFov);
            if (!context.writerBoundary) return WriterProposal{target, selection.usedGameplayBaselineValue};
            if (!context.scope) return {};
            cinematics::CinematicWriterSample sample;
            sample.source = context.receiver;
            sample.inputFov = static_cast<float>(context.incomingFov);
            sample.authoredEnterFov = authored;
            sample.transformedTargetFov = target;
            sample.targetEstablished = true;
            if (context.cinematicSourceAdmitted) {
                sample.continuationSource = context.scope->cinematicContinuationSource;
                sample.continuationStartFov = context.scope->cinematicContinuationStart;
                sample.continuationInterpolation = sample.continuationSource != 0;
            }
            sample.initialEstablishmentAuthorized = context.cinematicSourceAdmitted &&
                context.scope->cinematicInitialEstablishmentActive;
            sample.gameplayBaselineFov = selection.usedGameplayBaselineValue &&
                context.scope->receiver == context.receiver ? selection.targetBaselineFov : NAN;
            const auto resolved = cinematics::ResolveCinematicWriterSample(sample);
            const bool departure = sample.initialEstablishmentAuthorized &&
                !camera::AreFovValuesSemanticallyEquivalent(sample.inputFov, authored) &&
                (!std::isfinite(sample.gameplayBaselineFov) ||
                    std::fabs(sample.inputFov - sample.gameplayBaselineFov) > 0.05f);
            const bool mutate = resolved.action == cinematics::CinematicWriterAction::RetainTransformedTarget ||
                resolved.action == cinematics::CinematicWriterAction::RetainContinuationTarget;
            return WriterProposal{resolved.outputFov, false, departure, mutate, resolved.action};
        }
        if (context.owner == CorrectionOwner::Dialogue && context.scope && context.baseline) {
            const auto& scope = *context.scope;
            if (context.inputProvenance == FovProvenance::PreparedDialogue &&
                scope.providerDialogueOwned && scope.preparedDialogueFov &&
                scope.preparedDialogueFov->receiver == context.receiver &&
                scope.preparedDialogueFov->generation == scope.cinematicGeneration &&
                std::fabs(context.incomingFov - scope.preparedDialogueFov->value) <= 0.001) {
                const auto corrected = gameplay::EvaluateHorPlus(static_cast<float>(context.incomingFov),
                    fields.aspect, fields.flags, fields.nativeAspect);
                return corrected.eligible ? std::optional<WriterProposal>{{corrected.outputFov, false}} : std::nullopt;
            }
            const float nativeReference = static_cast<float>(context.baseline->fov);
            const auto policy = static_cast<config::DialogueZoomPolicy>(context.selectedPolicy);
            const float destination = dialogue::ResolveDialogueDestinationTarget(policy, nativeReference,
                static_cast<float>(scope.providerNativeTarget));
            const auto correctedTarget = gameplay::EvaluateHorPlus(destination,
                fields.aspect, fields.flags, fields.nativeAspect);
            if (!correctedTarget.applied) return {};
            gameplay::HorPlusRecoverySample start;
            start.source = context.receiver;
            start.validatedSource = scope.receiver;
            start.inputFov = static_cast<float>(scope.providerNativeStart);
            start.exitNativeTarget = static_cast<float>(scope.exitNativeTarget);
            start.cachedCinematicFov = camera::HorPlus(
                static_cast<float>(scope.cinematicBaselineFov), fields.aspect, fields.nativeAspect);
            start.aspect = fields.aspect;
            start.policyAspect = fields.aspect;
            start.policyAspectValid = true;
            start.flags = fields.flags;
            start.cameraReadable = true;
            start.gameplayEnabled = true;
            const float correctedStart = gameplay::ResolveHorPlusRecoveryInterpolationFov(start, fields.nativeAspect);
            return WriterProposal{dialogue::MapProjectionTransitionSample(
                static_cast<float>(context.incomingFov), static_cast<float>(scope.providerNativeStart),
                static_cast<float>(scope.providerNativeTarget), correctedStart, correctedTarget.outputFov), true};
        }
        return {};
    }
}
