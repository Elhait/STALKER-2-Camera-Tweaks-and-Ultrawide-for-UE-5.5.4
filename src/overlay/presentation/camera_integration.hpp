#pragma once

#include "../../plugin/runtime_settings.hpp"

#include <cstdint>

namespace overlay
{
    enum class CameraValueMatch
    {
        Waiting,
        Matched,
        Mismatch,
        CannotAssess,
        Unknown,
    };

    enum class OverallIntegrationAssessment
    {
        Waiting,
        Seamless,
        Mismatch,
        CannotAssess,
        Unknown,
    };

    enum class CinematicAssessmentContext
    {
        CannotAssess,
        NextSelection,
        ActiveSelection,
    };

    // Values are comparable only when they are observations of the camera
    // domains themselves, not settings-derived targets.
    struct CameraDomainState
    {
        float aspect{};
        float fov{};
        bool aspectValid{};
        bool fovValid{};
    };

    struct CameraTransitionAssessment
    {
        CameraValueMatch aspect{CameraValueMatch::CannotAssess};
        CameraValueMatch fov{CameraValueMatch::CannotAssess};
        OverallIntegrationAssessment overall{
            OverallIntegrationAssessment::CannotAssess};
    };

    struct PotentialTransitionAssessment
    {
        CameraValueMatch aspect{CameraValueMatch::CannotAssess};
        CameraValueMatch fov{CameraValueMatch::CannotAssess};
        OverallIntegrationAssessment overall{
            OverallIntegrationAssessment::CannotAssess};
        float gameplayAspect{};
        float cinematicAspect{};
        float gameplayFov{};
        float cinematicFov{};
        bool gameplayAspectValid{};
        bool cinematicAspectValid{};
        bool gameplayFovValid{};
        bool cinematicFovValid{};
    };

    struct CameraIntegrationStatus
    {
        CinematicAssessmentContext cinematicContext{
            CinematicAssessmentContext::CannotAssess};
        CameraValueMatch aspect{CameraValueMatch::CannotAssess};
        CameraValueMatch fov{CameraValueMatch::CannotAssess};
        OverallIntegrationAssessment overall{
            OverallIntegrationAssessment::CannotAssess};
        PotentialTransitionAssessment potential{};
        float gameplayFov{};
        float nativeFov{};
        float nativeWriterAspect{};
        float horPlusTargetFov{};
        float currentOutputFov{};
        float horPlusPolicyAspect{};
        float gameplayBoundaryFov{};
        float cinematicEnterFov{};
        float gameplayBoundaryAspect{};
        float cinematicEnterAspect{};
        float aspectDelta{};
        float fovDelta{};
        std::uint64_t transitionGeneration{};
        bool gameplayFovValid{};
        bool nativeFovValid{};
        bool nativeWriterAspectValid{};
        bool horPlusTargetFovValid{};
        bool currentOutputFovValid{};
        camera::SnapshotFreshness gameplayFovFreshness{
            camera::SnapshotFreshness::Unavailable};
        camera::SnapshotFreshness nativeFovFreshness{
            camera::SnapshotFreshness::Unavailable};
        camera::SnapshotFreshness horPlusTargetFovFreshness{
            camera::SnapshotFreshness::Unavailable};
        camera::SnapshotFreshness currentOutputFovFreshness{
            camera::SnapshotFreshness::Unavailable};
        bool horPlusPolicyAspectValid{};
        bool gameplayBoundaryFovValid{};
        bool cinematicEnterFovValid{};
        bool gameplayBoundaryAspectValid{};
        bool cinematicEnterAspectValid{};
        bool aspectDeltaValid{};
        bool fovDeltaValid{};
        bool cinematicCorrectionEnabled{};
        bool cinematicEnabledKnown{};
        // UI-only selection preview. Production resolution has additional
        // client/display/native fallback and store-acceptance outcomes.
        float cinematicAspectPreview{};
        bool cinematicAspectPreviewValid{};
        bool transitionBoundaryObserved{};
    };

    CameraTransitionAssessment AssessCameraTransition(
        const CameraDomainState& gameplay,
        const CameraDomainState& cinematic,
        float aspectTolerance = 0.01f) noexcept;

    CameraIntegrationStatus ProjectCameraIntegration(
        const plugin::OverlaySemanticSnapshot& snapshot,
        float aspectTolerance = 0.01f) noexcept;
}
