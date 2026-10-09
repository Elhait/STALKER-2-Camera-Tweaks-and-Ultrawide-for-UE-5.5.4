#include "camera_integration.hpp"
#include "../camera/fov_semantics.hpp"
#include "../camera/horplus.hpp"
#include "../cinematics/cinematic_aspect.hpp"

#include <cmath>

namespace overlay
{
    namespace
    {
        bool IsUsableAspect(float aspect) noexcept
        {
            return std::isfinite(aspect) && aspect > 0.0f;
        }

        bool IsUsableFovFact(const camera::CameraFovFact& fact) noexcept
        {
            return fact.valid && std::isfinite(fact.value) &&
                fact.value > 1.0f && fact.value < 179.0f;
        }

        bool ResolveAspectPreview(
            config::CinematicAspectPolicy policy, float viewportAspect,
            float& resolved) noexcept
        {
            switch (policy) {
            case config::CinematicAspectPolicy::Auto:
                if (!IsUsableAspect(viewportAspect)) return false;
                resolved = viewportAspect;
                return true;
            case config::CinematicAspectPolicy::Native:
                return false;
            case config::CinematicAspectPolicy::Forced16x9:
                resolved = 16.0f / 9.0f;
                return true;
            case config::CinematicAspectPolicy::Forced21x9:
                resolved = cinematics::Forced21x9Aspect;
                return true;
            case config::CinematicAspectPolicy::Forced32x9:
                resolved = 32.0f / 9.0f;
                return true;
            }
            return false;
        }

        bool ResolvePotentialAspect(
            config::CinematicAspectPolicy policy, float viewportAspect,
            float& resolved) noexcept
        {
            if (policy == config::CinematicAspectPolicy::Native) {
                resolved = 16.0f / 9.0f;
                return true;
            }
            return ResolveAspectPreview(policy, viewportAspect, resolved);
        }

        CameraValueMatch CompareAspect(float left, bool leftValid,
            float right, bool rightValid, float tolerance) noexcept
        {
            if (!leftValid || !rightValid || !IsUsableAspect(left) ||
                !IsUsableAspect(right) || !std::isfinite(tolerance) || tolerance < 0.0f)
                return CameraValueMatch::CannotAssess;
            return std::fabs(left - right) <= tolerance
                ? CameraValueMatch::Matched : CameraValueMatch::Mismatch;
        }

        CameraValueMatch CompareFov(float left, bool leftValid,
            float right, bool rightValid) noexcept
        {
            if (!leftValid || !rightValid)
                return CameraValueMatch::CannotAssess;
            if (!camera::AreFovValuesSemanticallyEquivalent(left, right)) {
                return std::isfinite(left) && std::isfinite(right)
                    ? CameraValueMatch::Mismatch : CameraValueMatch::CannotAssess;
            }
            return CameraValueMatch::Matched;
        }

        OverallIntegrationAssessment CombineAssessment(
            CameraValueMatch aspect, CameraValueMatch fov) noexcept
        {
            if (aspect == CameraValueMatch::Mismatch ||
                fov == CameraValueMatch::Mismatch)
                return OverallIntegrationAssessment::Mismatch;
            if (aspect == CameraValueMatch::Unknown ||
                fov == CameraValueMatch::Unknown)
                return OverallIntegrationAssessment::Unknown;
            if (aspect == CameraValueMatch::CannotAssess ||
                fov == CameraValueMatch::CannotAssess)
                return OverallIntegrationAssessment::CannotAssess;
            if (aspect == CameraValueMatch::Waiting ||
                fov == CameraValueMatch::Waiting)
                return OverallIntegrationAssessment::Waiting;
            return OverallIntegrationAssessment::Seamless;
        }

        void ProjectPotentialTransition(
            const plugin::OverlaySemanticSnapshot& snapshot,
            config::CinematicAspectPolicy aspectPolicy,
            config::CinematicFovMode fovMode, float aspectTolerance,
            PotentialTransitionAssessment& potential) noexcept
        {
            if (!snapshot.cameraStateValid || !snapshot.gameplayMode.valid ||
                !snapshot.gameplayEnabled.valid || !snapshot.cinematicsEnabled.valid ||
                !snapshot.cinematicsEnabled.value ||
                !snapshot.runtimeViewportAspect.valid ||
                !IsUsableAspect(snapshot.runtimeViewportAspect.value))
                return;

            const auto& evidence = snapshot.cameraState.evidence;
            const auto& freshness = snapshot.cameraState.freshness;
            const float viewportAspect = snapshot.runtimeViewportAspect.value;
            float cinematicAspect = 0.0f;
            if (!ResolvePotentialAspect(aspectPolicy, viewportAspect,
                    cinematicAspect))
                return;
            potential.cinematicAspect = cinematicAspect;
            potential.cinematicAspectValid = true;

            const bool writerCurrent = freshness.writerObservation ==
                camera::SnapshotFreshness::Current;
            float gameplayAspect = 0.0f;
            bool gameplayAspectValid = false;
            if (snapshot.gameplayEnabled.value && writerCurrent &&
                snapshot.gameplayMode.value == config::GameplayMode::RuntimeCombine) {
                // RuntimeCombine's selected target is the live client viewport;
                // this is a potential target, not a claim that the write landed.
                gameplayAspect = viewportAspect;
                gameplayAspectValid = true;
            } else if (writerCurrent && evidence.gameplayPolicyAspectValid &&
                IsUsableAspect(evidence.gameplayPolicyAspect)) {
                gameplayAspect = evidence.gameplayPolicyAspect;
                gameplayAspectValid = true;
            } else if (writerCurrent && evidence.aspectValid &&
                IsUsableAspect(evidence.aspect)) {
                gameplayAspect = evidence.aspect;
                gameplayAspectValid = true;
            }
            potential.gameplayAspect = gameplayAspect;
            potential.gameplayAspectValid = gameplayAspectValid;
            potential.aspect = CompareAspect(gameplayAspect, gameplayAspectValid,
                cinematicAspect, true, aspectTolerance);

            if (snapshot.gameplayModeTransitionPending.valid &&
                snapshot.gameplayModeTransitionPending.value) {
                potential.fov = CameraValueMatch::Waiting;
                potential.overall = CombineAssessment(potential.aspect,
                    potential.fov);
                return;
            }

            const auto& baseline = evidence.gameplayFov;
            if (snapshot.gameplayMode.value ==
                    config::GameplayMode::AspectRecalculation &&
                IsUsableFovFact(evidence.nativeFov) && writerCurrent) {
                potential.gameplayFov = evidence.nativeFov.value;
                potential.gameplayFovValid = true;
            } else if (IsUsableFovFact(baseline)) {
                constexpr float nativeAspect = 16.0f / 9.0f;
                const float gameAspectForFov = gameplayAspectValid
                    ? gameplayAspect : viewportAspect;
                potential.gameplayFov = snapshot.gameplayEnabled.value &&
                    IsUsableAspect(gameAspectForFov)
                    ? camera::HorPlus(baseline.value, gameAspectForFov,
                        nativeAspect)
                    : baseline.value;
                potential.gameplayFovValid = IsUsableFovFact(
                    {potential.gameplayFov, true,
                        camera::EvidenceProvenance::ModDerivedState,
                        camera::SnapshotFreshness::Current, 0, false, 0});
            }

            switch (fovMode) {
            case config::CinematicFovMode::GameplayHorPlus:
                if (IsUsableFovFact(baseline)) {
                    constexpr float nativeAspect = 16.0f / 9.0f;
                    potential.cinematicFov = camera::HorPlus(
                        baseline.value, cinematicAspect, nativeAspect);
                    potential.cinematicFovValid = IsUsableFovFact(
                        {potential.cinematicFov, true,
                            camera::EvidenceProvenance::ModDerivedState,
                            camera::SnapshotFreshness::Current, 0, false, 0});
                }
                potential.fov = CompareFov(potential.gameplayFov,
                    potential.gameplayFovValid, potential.cinematicFov,
                    potential.cinematicFovValid);
                break;
            case config::CinematicFovMode::NativeHorPlus:
                // The authored ENTER FOV does not exist until the game supplies
                // it. Never synthesize an FOV match from the selected mode.
                potential.fov = CameraValueMatch::Unknown;
                break;
            }
            potential.overall = CombineAssessment(potential.aspect,
                potential.fov);
        }
    }

    CameraTransitionAssessment AssessCameraTransition(
        const CameraDomainState& gameplay,
        const CameraDomainState& cinematic,
        float aspectTolerance) noexcept
    {
        CameraTransitionAssessment result{};
        result.aspect = CompareAspect(gameplay.aspect, gameplay.aspectValid,
            cinematic.aspect, cinematic.aspectValid, aspectTolerance);
        result.fov = CompareFov(gameplay.fov, gameplay.fovValid,
            cinematic.fov, cinematic.fovValid);

        if (result.aspect == CameraValueMatch::Mismatch ||
            result.fov == CameraValueMatch::Mismatch) {
            result.overall = OverallIntegrationAssessment::Mismatch;
        } else if (result.aspect == CameraValueMatch::Matched &&
            result.fov == CameraValueMatch::Matched) {
            result.overall = OverallIntegrationAssessment::Seamless;
        }
        return result;
    }

    CameraIntegrationStatus ProjectCameraIntegration(
        const plugin::OverlaySemanticSnapshot& snapshot,
        float aspectTolerance) noexcept
    {
        CameraIntegrationStatus result{};
        CameraDomainState gameplayDomain{};
        if (snapshot.cameraStateValid) {
            const auto& evidence = snapshot.cameraState.evidence;
            result.gameplayFov = evidence.gameplayFov.value;
            result.nativeFov = evidence.nativeFov.value;
            result.nativeWriterAspect = snapshot.cameraState.evidence.aspect;
            result.horPlusTargetFov = evidence.horPlusTargetFov.value;
            result.currentOutputFov = evidence.currentOutputFov.value;
            result.gameplayFovFreshness = evidence.gameplayFov.freshness;
            result.nativeFovFreshness = evidence.nativeFov.freshness;
            result.horPlusTargetFovFreshness = evidence.horPlusTargetFov.freshness;
            result.currentOutputFovFreshness = evidence.currentOutputFov.freshness;
            result.gameplayFovValid = IsUsableFovFact(evidence.gameplayFov);
            result.nativeFovValid = IsUsableFovFact(evidence.nativeFov);
            result.horPlusTargetFovValid = IsUsableFovFact(evidence.horPlusTargetFov);
            result.currentOutputFovValid = IsUsableFovFact(evidence.currentOutputFov);
            result.nativeWriterAspectValid = snapshot.cameraState.evidence.aspectValid &&
                snapshot.cameraState.freshness.writerObservation ==
                    camera::SnapshotFreshness::Current &&
                IsUsableAspect(result.nativeWriterAspect);
            result.horPlusPolicyAspect =
                snapshot.cameraState.evidence.gameplayPolicyAspect;
            result.horPlusPolicyAspectValid =
                snapshot.cameraState.evidence.gameplayPolicyAspectValid &&
                snapshot.cameraState.freshness.writerObservation ==
                    camera::SnapshotFreshness::Current &&
                IsUsableAspect(result.horPlusPolicyAspect);

        }

        bool boundaryAvailable = false;
        const auto& boundaryFact = snapshot.cameraTransitionBoundary;
        if (boundaryFact.valid && boundaryFact.value.accepted &&
            boundaryFact.value.cinematicValid && boundaryFact.value.gameplayValid &&
            boundaryFact.value.lifecycleGeneration != 0 &&
            boundaryFact.value.gameplay.observedDuringGameplay &&
            boundaryFact.value.gameplay.boundary ==
                camera::FovObservationBoundary::CameraWriter &&
            boundaryFact.value.cinematic.boundary ==
                camera::FovObservationBoundary::CinematicEnter &&
            boundaryFact.value.gameplay.publicationSequence != 0 &&
            boundaryFact.value.cinematic.publicationSequence >
                boundaryFact.value.gameplay.publicationSequence) {
            const auto& gameplay = boundaryFact.value.gameplay;
            const auto& cinematic = boundaryFact.value.cinematic;
            const auto gameplayPolicyAspect = gameplay.policyAspect.valid
                ? gameplay.policyAspect : gameplay.aspect;
            gameplayDomain = {gameplayPolicyAspect.value, gameplay.resultFov.value,
                gameplayPolicyAspect.valid, gameplay.resultFov.valid};
            CameraDomainState cinematicDomain{cinematic.aspect.value,
                cinematic.resultFov.value, cinematic.aspect.valid,
                cinematic.resultFov.valid};
            const auto transition = AssessCameraTransition(
                gameplayDomain, cinematicDomain, aspectTolerance);
            result.aspect = transition.aspect;
            result.fov = transition.fov;
            result.overall = transition.overall;
            result.transitionBoundaryObserved = true;
            result.transitionGeneration = boundaryFact.value.lifecycleGeneration;
            if (IsUsableAspect(gameplayDomain.aspect)) {
                result.gameplayBoundaryAspect = gameplayDomain.aspect;
                result.gameplayBoundaryAspectValid = true;
            }
            if (IsUsableAspect(cinematicDomain.aspect)) {
                result.cinematicEnterAspect = cinematicDomain.aspect;
                result.cinematicEnterAspectValid = true;
            }
            if (result.gameplayBoundaryAspectValid &&
                result.cinematicEnterAspectValid) {
                result.aspectDelta = std::fabs(result.gameplayBoundaryAspect -
                    result.cinematicEnterAspect);
                result.aspectDeltaValid = true;
            }
            if (std::isfinite(gameplay.resultFov.value) && gameplay.resultFov.valid) {
                result.gameplayBoundaryFov = gameplay.resultFov.value;
                result.gameplayBoundaryFovValid = true;
            }
            if (std::isfinite(cinematic.resultFov.value) && cinematic.resultFov.valid) {
                result.cinematicEnterFov = cinematic.resultFov.value;
                result.cinematicEnterFovValid = true;
            }
            if (result.gameplayBoundaryFovValid && result.cinematicEnterFovValid) {
                result.fovDelta = std::fabs(result.gameplayBoundaryFov -
                    result.cinematicEnterFov);
                result.fovDeltaValid = true;
            }
            boundaryAvailable = true;
        }

        const bool coordinatorKnown = snapshot.coordinator.valid;
        const auto coordinator = snapshot.coordinator.value;
        if (!boundaryAvailable && coordinatorKnown && coordinator == plugin::OverlayCoordinatorState::Gameplay) {
            // No current Cinematic boundary exists in Gameplay. This is a
            // lifecycle wait, not a failed comparison or a config-derived guess.
            result.aspect = CameraValueMatch::Waiting;
            result.fov = CameraValueMatch::Waiting;
            result.overall = OverallIntegrationAssessment::Waiting;
        } else if (!boundaryAvailable && coordinatorKnown && coordinator ==
            plugin::OverlayCoordinatorState::CinematicActive) {
        } else if (!boundaryAvailable) {
            result.aspect = CameraValueMatch::CannotAssess;
            result.fov = CameraValueMatch::CannotAssess;
            result.overall = OverallIntegrationAssessment::CannotAssess;
        }

        const bool selectionActiveKnown = snapshot.cinematicSelectionActive.valid;
        const bool useActiveSelection = selectionActiveKnown &&
            snapshot.cinematicSelectionActive.value;
        bool selectedCinematicsEnabled = false;
        config::CinematicAspectPolicy selectedAspectPolicy{};
        bool selectedAspectPolicyValid = false;
        if (useActiveSelection) {
            if (snapshot.activeCinematicsEnabled.valid) {
                result.cinematicContext =
                    CinematicAssessmentContext::ActiveSelection;
                result.cinematicEnabledKnown = true;
                selectedCinematicsEnabled = snapshot.activeCinematicsEnabled.value;
                selectedAspectPolicyValid =
                    snapshot.activeCinematicAspectPolicy.valid;
                if (selectedAspectPolicyValid)
                    selectedAspectPolicy = snapshot.activeCinematicAspectPolicy.value;
            }
        } else if (selectionActiveKnown && snapshot.cinematicsEnabled.valid) {
            result.cinematicContext = CinematicAssessmentContext::NextSelection;
            result.cinematicEnabledKnown = true;
            selectedCinematicsEnabled = snapshot.cinematicsEnabled.value;
            selectedAspectPolicyValid = snapshot.cinematicAspectPolicy.valid;
            if (selectedAspectPolicyValid)
                selectedAspectPolicy = snapshot.cinematicAspectPolicy.value;
        }
        result.cinematicCorrectionEnabled = selectedCinematicsEnabled;

        // Potential is always evaluated against the currently configured
        // selection (the next accepted ENTER), independently of a live or
        // historical Actual boundary.
        if (snapshot.cinematicsEnabled.valid &&
            snapshot.cinematicAspectPolicy.valid && snapshot.cinematicFovMode.valid &&
            snapshot.cinematicsEnabled.value) {
            ProjectPotentialTransition(snapshot,
                snapshot.cinematicAspectPolicy.value,
                snapshot.cinematicFovMode.value, aspectTolerance,
                result.potential);
        }

        if (!selectedCinematicsEnabled || !selectedAspectPolicyValid)
            return result;

        const bool viewportValid = snapshot.runtimeViewportAspect.valid &&
            IsUsableAspect(snapshot.runtimeViewportAspect.value);
        const float viewportAspect = viewportValid
            ? snapshot.runtimeViewportAspect.value : 0.0f;
        if (ResolveAspectPreview(selectedAspectPolicy, viewportAspect,
            result.cinematicAspectPreview)) {
            result.cinematicAspectPreviewValid = true;
        }
        return result;
    }
}
