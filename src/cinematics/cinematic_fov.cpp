#include "cinematic_fov.hpp"
#include "../camera/fov_semantics.hpp"
#include "../camera/horplus.hpp"

#include <algorithm>
#include <cmath>

namespace cinematics
{
    namespace
    {
        // Gameplay-baseline retention is stricter than semantic FOV matching:
        // the latter's 1-degree tolerance would classify early camera ramps
        // as the baseline and prolong initial establishment.
        constexpr float kGameplayBaselineRetentionToleranceDegrees = 0.05f;

        bool MatchesGameplayBaseline(float inputFov, float baselineFov) noexcept
        {
            return std::isfinite(inputFov) && std::isfinite(baselineFov) &&
                std::fabs(inputFov - baselineFov) <=
                    kGameplayBaselineRetentionToleranceDegrees;
        }
    }

    void CinematicWriterAssociation::BeginGeneration(std::uint64_t generation,
        float authoredEnterFov, float targetFov, EffectiveCinematicSettings settings,
        float gameplayBaselineFov, std::uintptr_t gameplayBaselineSource)
    {
        std::lock_guard lock(mutex_);
        state_.generationActive = generation != 0;
        state_.generation = generation;
        state_.authoredEnterFov = authoredEnterFov;
        state_.targetFov = targetFov;
        state_.gameplayBaselineFov = gameplayBaselineFov;
        state_.gameplayBaselineSource = std::isfinite(gameplayBaselineFov) &&
            gameplayBaselineFov > 1.0f && gameplayBaselineFov < 179.0f
            ? gameplayBaselineSource : 0;
        state_.effectiveSettings = settings;
        state_.aspectStoreTargetObject = 0;
        state_.conflictingAspectStoreTargetObject = 0;
        state_.associatedWriterRsi = 0;
        state_.observedWriterRbx = 0;
        state_.writerObserved = false;
        state_.associationValid = false;
        state_.associationConflict = false;
        state_.initialEstablishmentActive = settings.enabled &&
            std::isfinite(authoredEnterFov) && authoredEnterFov > 1.0f &&
            authoredEnterFov < 179.0f && std::isfinite(targetFov) &&
            targetFov > 1.0f && targetFov < 179.0f &&
            !camera::AreFovValuesSemanticallyEquivalent(authoredEnterFov, targetFov);
        state_.initialEstablishmentEnded = false;
        state_.initialEstablishmentEndReason = InitialEstablishmentEndReason::None;
    }

    void CinematicWriterAssociation::InvalidateGeneration(std::uint64_t generation)
    {
        std::lock_guard lock(mutex_);
        if (!state_.generationActive || state_.generation != generation) return;
        state_.generationActive = false;
        state_.associationValid = false;
        state_.initialEstablishmentActive = false;
        state_.initialEstablishmentEnded = true;
        state_.initialEstablishmentEndReason = InitialEstablishmentEndReason::CinematicExit;
        state_.associatedWriterRsi = 0;
        state_.observedWriterRbx = 0;
    }

    CinematicWriterAssociationSnapshot
    CinematicWriterAssociation::ObserveAspectStoreTarget(
        std::uint64_t generation, std::uintptr_t targetObject)
    {
        std::lock_guard lock(mutex_);
        if (!state_.generationActive || state_.generation != generation ||
            targetObject == 0)
            return SnapshotLocked();

        if (state_.aspectStoreTargetObject == 0) {
            state_.aspectStoreTargetObject = targetObject;
        } else if (state_.aspectStoreTargetObject != targetObject) {
            state_.conflictingAspectStoreTargetObject = targetObject;
            state_.associationConflict = true;
            state_.associationValid = false;
            state_.initialEstablishmentActive = false;
            state_.initialEstablishmentEnded = true;
            state_.initialEstablishmentEndReason =
                InitialEstablishmentEndReason::AssociationConflict;
            state_.associatedWriterRsi = 0;
            state_.observedWriterRbx = 0;
        }
        return SnapshotLocked();
    }

    CinematicWriterAssociationSnapshot CinematicWriterAssociation::ObserveWriter(
        camera::CoordinatorState coordinator, std::uintptr_t source,
        std::uintptr_t output, float rawFov, float aspect, bool aspectValid,
        std::uint8_t flags, bool flagsValid,
        SharedWriterPairTransition* pairTransition)
    {
        std::lock_guard lock(mutex_);
        auto& writer = state_.latestWriter;
        if (pairTransition != nullptr) {
            pairTransition->previous = writer;
            pairTransition->initialized = writer.sequence == 0;
            pairTransition->changed = writer.sequence != 0 &&
                (source != writer.source || output != writer.output);
        }
        if (writer.sequence == 0) {
            writer.identityEpoch = 1;
        } else if (source != previousWriterSource_ || output != previousWriterOutput_) {
            ++writer.identityEpoch;
        }
        ++writer.sequence;
        writer.source = source;
        writer.output = output;
        writer.rawFov = rawFov;
        writer.aspect = aspect;
        writer.aspectValid = aspectValid;
        writer.flags = flags;
        writer.flagsValid = flagsValid;
        previousWriterSource_ = source;
        previousWriterOutput_ = output;
        if (coordinator == camera::CoordinatorState::CinematicActive &&
            state_.generationActive)
            state_.writerObserved = true;
        if (pairTransition != nullptr)
            pairTransition->current = writer;

        if (coordinator == camera::CoordinatorState::CinematicActive &&
            state_.generationActive && !state_.associationConflict &&
            state_.aspectStoreTargetObject != 0 &&
            source == state_.aspectStoreTargetObject) {
            if (!state_.associationValid) {
                state_.associatedWriterRsi = source;
                state_.observedWriterRbx = output;
                state_.associationValid = true;
            } else if (state_.associatedWriterRsi != source ||
                state_.observedWriterRbx != output) {
                state_.associationConflict = true;
                state_.associationValid = false;
                state_.initialEstablishmentActive = false;
                state_.initialEstablishmentEnded = true;
                state_.initialEstablishmentEndReason =
                    InitialEstablishmentEndReason::AssociationConflict;
                state_.associatedWriterRsi = 0;
                state_.observedWriterRbx = 0;
            }
        }
        return SnapshotLocked();
    }

    bool CinematicWriterAssociation::AuthorizeInitialEstablishmentRetention(
        camera::CoordinatorState owner, bool correctionEnabled,
        bool acceptedGenerationValid, bool targetEstablished,
        std::uint64_t activeGeneration, std::uintptr_t writerRsi,
        float writerInputFov)
    {
        std::lock_guard lock(mutex_);
        if (owner != camera::CoordinatorState::CinematicActive ||
            !correctionEnabled || !acceptedGenerationValid || !targetEstablished ||
            activeGeneration == 0 || !state_.generationActive ||
            state_.generation != activeGeneration ||
            !state_.effectiveSettings.enabled || !state_.initialEstablishmentActive ||
            !state_.associationValid || state_.associationConflict ||
            state_.aspectStoreTargetObject == 0 ||
            state_.associatedWriterRsi != state_.aspectStoreTargetObject ||
            writerRsi == 0 || writerRsi != state_.associatedWriterRsi ||
            !std::isfinite(writerInputFov))
            return false;

        const bool authoredInput = camera::AreFovValuesSemanticallyEquivalent(
            writerInputFov, state_.authoredEnterFov);
        const bool gameplayBaselineInput = state_.gameplayBaselineSource != 0 &&
            state_.gameplayBaselineSource == writerRsi &&
            MatchesGameplayBaseline(writerInputFov, state_.gameplayBaselineFov);
        if (!authoredInput && !gameplayBaselineInput) {
            state_.initialEstablishmentActive = false;
            state_.initialEstablishmentEnded = true;
            state_.initialEstablishmentEndReason =
                InitialEstablishmentEndReason::AuthoredBaselineDeparture;
            return false;
        }

        return true;
    }

    CinematicWriterAssociationSnapshot CinematicWriterAssociation::Read() const
    {
        std::lock_guard lock(mutex_);
        return SnapshotLocked();
    }

    CinematicWriterAssociationSnapshot
    CinematicWriterAssociation::SnapshotLocked() const
    {
        return state_;
    }

    bool IsValidCinematicRecoveryOwner(
        const CinematicWriterAssociationSnapshot& association,
        std::uint64_t expectedGeneration) noexcept
    {
        return expectedGeneration != 0 && association.generationActive &&
            association.generation == expectedGeneration &&
            association.associationValid && !association.associationConflict &&
            association.aspectStoreTargetObject != 0 &&
            association.associatedWriterRsi != 0 &&
            association.associatedWriterRsi == association.aspectStoreTargetObject;
    }

    bool CinematicRecoveryOwner::Capture(
        const CinematicWriterAssociationSnapshot& association,
        std::uint64_t expectedGeneration)
    {
        if (!IsValidCinematicRecoveryOwner(association, expectedGeneration))
            return false;
        std::lock_guard lock(mutex_);
        state_ = { true, expectedGeneration, association.associatedWriterRsi,
            association.observedWriterRbx, false };
        return true;
    }

    CinematicRecoveryOwnerSnapshot CinematicRecoveryOwner::Read() const
    {
        std::lock_guard lock(mutex_);
        return state_;
    }

    bool CinematicRecoveryOwner::MatchesSource(std::uintptr_t source) const
    {
        std::lock_guard lock(mutex_);
        return state_.HasRecoveryAuthority() && source != 0 &&
            state_.writerRsi == source;
    }

    bool CinematicRecoveryOwner::MarkComplete(std::uint64_t generation)
    {
        std::lock_guard lock(mutex_);
        if (!state_.valid || state_.generation != generation) return false;
        state_.recoveryComplete = true;
        return true;
    }

    void CinematicRecoveryOwner::Clear()
    {
        std::lock_guard lock(mutex_);
        state_ = {};
    }

    CinematicWriterResolution ResolveCinematicWriterSample(
        const CinematicWriterSample& sample) noexcept
    {
        if (!sample.targetEstablished || sample.source == 0 ||
            !std::isfinite(sample.inputFov) || !std::isfinite(sample.authoredEnterFov) ||
            !std::isfinite(sample.transformedTargetFov) ||
            sample.authoredEnterFov <= 1.0f || sample.authoredEnterFov >= 179.0f ||
            sample.transformedTargetFov <= 1.0f || sample.transformedTargetFov >= 179.0f)
            return { CinematicWriterAction::PassThroughUnclassified, sample.inputFov };

        if (camera::AreFovValuesSemanticallyEquivalent(
                sample.transformedTargetFov, sample.authoredEnterFov))
            return { CinematicWriterAction::PassThroughUnclassified, sample.inputFov };
        const bool semanticNativeBaseline = camera::AreFovValuesSemanticallyEquivalent(
            sample.inputFov, sample.authoredEnterFov);
        const bool semanticGameplayBaseline = MatchesGameplayBaseline(
            sample.inputFov, sample.gameplayBaselineFov);
        if (sample.initialEstablishmentAuthorized &&
            (semanticNativeBaseline || semanticGameplayBaseline))
            return { CinematicWriterAction::RetainTransformedTarget,
                sample.transformedTargetFov };

        // Keep the pre-existing Gameplay-baseline source binding only for the
        // continuation/interpolation policy. It never falls back to authorize
        // the initial native-baseline overwrite above.
        if (sample.continuationSource == 0 ||
            sample.source != sample.continuationSource)
            return { CinematicWriterAction::PassThroughUnclassified, sample.inputFov };

        if (sample.continuationInterpolation && std::isfinite(sample.continuationStartFov)) {
            const float lower = (std::min)(sample.continuationStartFov,
                sample.transformedTargetFov);
            const float upper = (std::max)(sample.continuationStartFov,
                sample.transformedTargetFov);
            if (sample.inputFov >= lower && sample.inputFov <= upper)
                return { CinematicWriterAction::RetainContinuationTarget,
                    sample.transformedTargetFov };
        }
        if (semanticNativeBaseline)
            return { CinematicWriterAction::PassThroughUnclassified, sample.inputFov };
        const float lower = (std::min)(sample.authoredEnterFov,
            sample.transformedTargetFov);
        const float upper = (std::max)(sample.authoredEnterFov,
            sample.transformedTargetFov);
        if (sample.inputFov >= lower && sample.inputFov <= upper)
            return { CinematicWriterAction::PassThroughInterpolation, sample.inputFov };
        return { CinematicWriterAction::PassThroughUnclassified, sample.inputFov };
    }

    CinematicWriterDispatch DispatchCinematicWriter(
        camera::CoordinatorState owner, bool correctionEnabled,
        const CinematicWriterSample& sample) noexcept
    {
        if (owner != camera::CoordinatorState::CinematicActive) return {};
        if (!correctionEnabled) return { true,
            { CinematicWriterAction::PassThroughUnclassified, sample.inputFov } };
        return { true, ResolveCinematicWriterSample(sample) };
    }

    CinematicBaselineSelection SelectCinematicBaseline(bool gameplayHorPlusRequested,
        float authoredEnterFov, const camera::GameplayBaseline& gameplayBaseline)
    {
        if (gameplayHorPlusRequested && camera::IsUsableGameplayBaseline(gameplayBaseline)) {
            return { gameplayBaseline.nativeFov.value,
                CinematicBaselineValueOrigin::GameplayNativeNumericBaseline, true };
        }
        return { authoredEnterFov, CinematicBaselineValueOrigin::AuthoredEnter, false };
    }

    bool TryTransformCinematicFov(float cinematicFov, float cinematicReferenceFov,
        float targetBaselineFov, float aspect, float nativeAspect,
        float& transformedFov)
    {
        transformedFov = cinematicFov;
        if (!std::isfinite(cinematicFov) || !std::isfinite(cinematicReferenceFov) ||
            !std::isfinite(targetBaselineFov) || !std::isfinite(aspect) ||
            !std::isfinite(nativeAspect) || cinematicFov <= 1.0f || cinematicFov >= 179.0f ||
            cinematicReferenceFov <= 1.0f || cinematicReferenceFov >= 179.0f ||
            targetBaselineFov <= 1.0f || targetBaselineFov >= 179.0f || aspect <= 1.0f ||
            nativeAspect <= 1.0f)
            return false;

        constexpr float pi = 3.14159265358979323846f;
        const float radiansPerDegree = pi / 180.0f;
        const float referenceProjection =
            std::tan(cinematicReferenceFov * 0.5f * radiansPerDegree);
        if (!std::isfinite(referenceProjection) || std::fabs(referenceProjection) <= 1.0e-6f)
            return false;

        const float matchedProjection = std::tan(cinematicFov * 0.5f * radiansPerDegree) *
            (std::tan(targetBaselineFov * 0.5f * radiansPerDegree) / referenceProjection);
        if (!std::isfinite(matchedProjection)) return false;

        const float matchedNative = 2.0f * std::atan(matchedProjection) / radiansPerDegree;
        if (!std::isfinite(matchedNative) || matchedNative <= 1.0f || matchedNative >= 179.0f)
            return false;

        transformedFov = camera::HorPlus(matchedNative, aspect, nativeAspect);
        return std::isfinite(transformedFov) && transformedFov > 1.0f && transformedFov < 179.0f;
    }

    bool TryTransformSelectedCinematicFov(bool correctionEnabled,
        config::CinematicFovMode fovMode, float cinematicFov,
        float cinematicReferenceFov, float targetBaselineFov, float aspect,
        float nativeAspect, float& transformedFov)
    {
        transformedFov = cinematicFov;
        if (!correctionEnabled) return false;
        switch (fovMode) {
        case config::CinematicFovMode::NativeHorPlus:
        case config::CinematicFovMode::GameplayHorPlus:
            return TryTransformCinematicFov(cinematicFov, cinematicReferenceFov,
                targetBaselineFov, aspect, nativeAspect, transformedFov);
        }
        return false;
    }

    bool TryTransformEnterFov(float authoredFov, float aspect, bool enabled,
        float nativeAspect, float& transformedFov)
    {
        transformedFov = authoredFov;
        if (!enabled)
            return false;
        return TryTransformCinematicFov(authoredFov, authoredFov, authoredFov,
            aspect, nativeAspect, transformedFov);
    }

    bool TryTransformMatchGameplay(float cinematicFov, float gameplayBaselineFov,
        float cinematicReferenceFov, float aspect, float nativeAspect,
        float& transformedFov)
    {
        return TryTransformCinematicFov(cinematicFov, cinematicReferenceFov,
            gameplayBaselineFov, aspect, nativeAspect, transformedFov);
    }
}
