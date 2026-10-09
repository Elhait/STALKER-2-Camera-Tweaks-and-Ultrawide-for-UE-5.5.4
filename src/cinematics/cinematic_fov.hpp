#pragma once

#include "../camera/gameplay_baseline.hpp"
#include "../camera/presentation_state.hpp"
#include "../config/feature_config.hpp"

#include <cstdint>
#include <limits>
#include <mutex>

namespace cinematics
{
    struct EffectiveCinematicSettings
    {
        bool enabled{};
        config::CinematicAspectPolicy aspectPolicy{};
        config::CinematicFovMode fovMode{};
    };

    struct SharedWriterObservation
    {
        std::uint64_t sequence{};
        std::uint64_t identityEpoch{};
        std::uintptr_t source{};
        std::uintptr_t output{};
        float rawFov{std::numeric_limits<float>::quiet_NaN()};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        bool aspectValid{};
        bool flagsValid{};
    };

    struct SharedWriterPairTransition
    {
        bool initialized{};
        bool changed{};
        SharedWriterObservation previous{};
        SharedWriterObservation current{};
    };

    enum class InitialEstablishmentEndReason : std::uint8_t
    {
        None,
        AuthoredBaselineDeparture,
        AssociationConflict,
        CinematicExit,
    };

    struct CinematicWriterAssociationSnapshot
    {
        bool generationActive{};
        std::uint64_t generation{};
        float authoredEnterFov{std::numeric_limits<float>::quiet_NaN()};
        float targetFov{std::numeric_limits<float>::quiet_NaN()};
        float gameplayBaselineFov{std::numeric_limits<float>::quiet_NaN()};
        std::uintptr_t gameplayBaselineSource{};
        EffectiveCinematicSettings effectiveSettings{};
        std::uintptr_t aspectStoreTargetObject{};
        std::uintptr_t conflictingAspectStoreTargetObject{};
        std::uintptr_t associatedWriterRsi{};
        std::uintptr_t observedWriterRbx{};
        bool writerObserved{};
        bool associationValid{};
        bool associationConflict{};
        bool initialEstablishmentActive{};
        bool initialEstablishmentEnded{};
        InitialEstablishmentEndReason initialEstablishmentEndReason{
            InitialEstablishmentEndReason::None};
        SharedWriterObservation latestWriter{};
    };

    // Generation-scoped RSI association authorizes only initial establishment;
    // it does not authorize continuation or recovery mutation.
    class CinematicWriterAssociation
    {
    public:
        void BeginGeneration(std::uint64_t generation, float authoredEnterFov,
            float targetFov, EffectiveCinematicSettings settings,
            float gameplayBaselineFov = std::numeric_limits<float>::quiet_NaN(),
            std::uintptr_t gameplayBaselineSource = 0);
        void InvalidateGeneration(std::uint64_t generation);
        CinematicWriterAssociationSnapshot ObserveAspectStoreTarget(
            std::uint64_t generation, std::uintptr_t targetObject);
        CinematicWriterAssociationSnapshot ObserveWriter(
            camera::CoordinatorState coordinator, std::uintptr_t source,
            std::uintptr_t output, float rawFov, float aspect, bool aspectValid,
            std::uint8_t flags, bool flagsValid,
            SharedWriterPairTransition* pairTransition = nullptr);
        bool AuthorizeInitialEstablishmentRetention(
            camera::CoordinatorState owner, bool correctionEnabled,
            bool acceptedGenerationValid, bool targetEstablished,
            std::uint64_t activeGeneration, std::uintptr_t writerRsi,
            float writerInputFov);
        CinematicWriterAssociationSnapshot Read() const;

    private:
        CinematicWriterAssociationSnapshot SnapshotLocked() const;
        mutable std::mutex mutex_;
        CinematicWriterAssociationSnapshot state_{};
        std::uintptr_t previousWriterSource_{};
        std::uintptr_t previousWriterOutput_{};
    };

    struct CinematicRecoveryOwnerSnapshot
    {
        bool valid{};
        std::uint64_t generation{};
        std::uintptr_t writerRsi{};
        std::uintptr_t observedWriterRbx{};
        bool recoveryComplete{};

        constexpr bool HasRecoveryAuthority() const noexcept
        {
            return valid && !recoveryComplete;
        }
    };

    bool IsValidCinematicRecoveryOwner(
        const CinematicWriterAssociationSnapshot& association,
        std::uint64_t expectedGeneration) noexcept;

    class CinematicRecoveryOwner
    {
    public:
        bool Capture(const CinematicWriterAssociationSnapshot& association,
            std::uint64_t expectedGeneration);
        CinematicRecoveryOwnerSnapshot Read() const;
        bool MatchesSource(std::uintptr_t source) const;
        bool MarkComplete(std::uint64_t generation);
        void Clear();

    private:
        mutable std::mutex mutex_;
        CinematicRecoveryOwnerSnapshot state_{};
    };

    enum class CinematicWriterAction : std::uint8_t
    {
        RetainTransformedTarget,
        RetainContinuationTarget,
        PassThroughInterpolation,
        PassThroughUnclassified,
    };

    struct CinematicWriterSample
    {
        std::uintptr_t source{};
        // Preserves existing source-bound continuation classification only.
        // It does not authorize initial target retention.
        std::uintptr_t continuationSource{};
        float inputFov{std::numeric_limits<float>::quiet_NaN()};
        float authoredEnterFov{std::numeric_limits<float>::quiet_NaN()};
        float transformedTargetFov{std::numeric_limits<float>::quiet_NaN()};
        float continuationStartFov{std::numeric_limits<float>::quiet_NaN()};
        bool targetEstablished{};
        bool continuationInterpolation{};
        bool initialEstablishmentAuthorized{};
        float gameplayBaselineFov{std::numeric_limits<float>::quiet_NaN()};
    };

    struct CinematicWriterResolution
    {
        CinematicWriterAction action{CinematicWriterAction::PassThroughUnclassified};
        float outputFov{std::numeric_limits<float>::quiet_NaN()};
    };

    struct CinematicWriterDispatch
    {
        bool handled{};
        CinematicWriterResolution resolution{};
    };

    CinematicWriterResolution ResolveCinematicWriterSample(
        const CinematicWriterSample& sample) noexcept;
    CinematicWriterDispatch DispatchCinematicWriter(
        camera::CoordinatorState owner, bool correctionEnabled,
        const CinematicWriterSample& sample) noexcept;

    enum class CinematicBaselineValueOrigin
    {
        AuthoredEnter,
        GameplayNativeNumericBaseline,
    };

    struct CinematicBaselineSelection
    {
        float targetBaselineFov{};
        CinematicBaselineValueOrigin valueOrigin{
            CinematicBaselineValueOrigin::AuthoredEnter};
        bool usedGameplayBaselineValue{};
    };

    CinematicBaselineSelection SelectCinematicBaseline(bool gameplayHorPlusRequested,
        float authoredEnterFov, const camera::GameplayBaseline& gameplayBaseline);

    bool TryTransformCinematicFov(float cinematicFov, float cinematicReferenceFov,
        float targetBaselineFov, float aspect, float nativeAspect,
        float& transformedFov);

    bool TryTransformSelectedCinematicFov(bool correctionEnabled,
        config::CinematicFovMode fovMode, float cinematicFov,
        float cinematicReferenceFov, float targetBaselineFov, float aspect,
        float nativeAspect, float& transformedFov);

    bool TryTransformMatchGameplay(float cinematicFov, float gameplayBaselineFov,
        float cinematicReferenceFov, float aspect, float nativeAspect,
        float& transformedFov);

    bool TryTransformEnterFov(float authoredFov, float aspect, bool enabled,
        float nativeAspect, float& transformedFov);
}
