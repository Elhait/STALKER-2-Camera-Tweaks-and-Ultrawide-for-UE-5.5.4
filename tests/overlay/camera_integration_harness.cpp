#include "../../src/overlay/camera_integration.hpp"
#include "../../src/cinematics/cinematic_aspect.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }

    plugin::OverlaySemanticSnapshot BaseSnapshot()
    {
        plugin::OverlaySemanticSnapshot snapshot{};
        snapshot.gameplayEnabled = {true, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.gameplayMode = {config::GameplayMode::HorPlus, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicAspectPolicy = {config::CinematicAspectPolicy::Auto, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicFovMode = {config::CinematicFovMode::GameplayHorPlus, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.dialogueZoomPolicy = {config::DialogueZoomPolicy::Adaptive, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.gameplayBaselineUsable = {true, true,
            plugin::SemanticProvenance::GameplayBaselineStore,
            plugin::SemanticFreshness::RetainedUntilInvalidated};
        snapshot.cinematicAspectComponentAvailable = {true, true,
            plugin::SemanticProvenance::CinematicHookState,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicFovLifecycleAvailable = {true, true,
            plugin::SemanticProvenance::CinematicHookState,
            plugin::SemanticFreshness::AtRead};
        snapshot.runtimeViewportAspect = {32.0f / 9.0f, true,
            plugin::SemanticProvenance::RuntimeViewportResolver,
            plugin::SemanticFreshness::AtRead};
        return snapshot;
    }
}

int main()
{
    bool pass = true;
    auto snapshot = BaseSnapshot();
    auto result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Matched &&
        result.fovPath == overlay::FovPathAssessment::GameplayLinked &&
        result.overall == overlay::OverallIntegrationAssessment::ConfigurationAligned,
        "auto_aspect_and_gameplay_baseline_align");

    snapshot.runtimeViewportAspect.value = 3.0f;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Matched &&
        result.resolvedCinematicAspectValid && result.resolvedCinematicAspect == 3.0f,
        "auto_uses_arbitrary_custom_aspect");
    snapshot.runtimeViewportAspect.value = 32.0f / 9.0f;

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced32x9;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Matched &&
        result.overall == overlay::OverallIntegrationAssessment::ConfigurationAligned,
        "forced_32_9_matches_numeric_viewport");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced21x9;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Mismatch &&
        result.overall == overlay::OverallIntegrationAssessment::NotAligned,
        "forced_21_9_mismatches_32_9_viewport");

    snapshot.runtimeViewportAspect.value = 3440.0f / 1440.0f;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.resolvedCinematicAspectValid &&
        std::fabs(result.resolvedCinematicAspect - cinematics::Forced21x9Aspect) < 0.0001f &&
        result.aspect == overlay::AspectAssessment::Matched,
        "forced_21_9_uses_authoritative_production_aspect");

    snapshot.runtimeViewportAspect.value = 16.0f / 9.0f;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced16x9;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Matched &&
        result.resolvedCinematicAspectValid &&
        std::fabs(result.resolvedCinematicAspect - (16.0f / 9.0f)) < 0.0001f,
        "forced_16_9_behavior_unchanged");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced21x9;
    snapshot.runtimeViewportAspect.value = 3440.0f / 1440.0f;
    snapshot.runtimeViewportAspect.valid = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::CannotAssess,
        "forced_21_9_unavailable_viewport_cannot_assess");

    snapshot.runtimeViewportAspect.valid = true;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.resolvedCinematicAspectValid &&
        std::fabs(result.resolvedCinematicAspect - (3440.0f / 1440.0f)) < 0.0001f &&
        result.aspect == overlay::AspectAssessment::Matched,
        "auto_uses_production_viewport_value");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.cinematicFovMode.value = config::CinematicFovMode::NativeHorPlus;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.fovPath == overlay::FovPathAssessment::Independent &&
        result.overall == overlay::OverallIntegrationAssessment::NotAligned,
        "native_horplus_remains_independent");

    // No cinematic/reference FOV values enter this projection: numeric equality
    // cannot turn the semantically independent NativeHorPlus path into linked.
    snapshot.cinematicFovMode.value = config::CinematicFovMode::GameplayHorPlus;
    snapshot.gameplayBaselineUsable.value = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.fovPath == overlay::FovPathAssessment::AuthoredFallback &&
        result.overall == overlay::OverallIntegrationAssessment::Waiting,
        "compatible_configuration_waits_for_gameplay_camera_data");

    snapshot.runtimeViewportAspect.value = 32.0f / 9.0f;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced21x9;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Mismatch &&
        result.overall == overlay::OverallIntegrationAssessment::NotAligned,
        "confirmed_aspect_mismatch_overrides_waiting");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.gameplayBaselineUsable.value = true;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Native;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::CannotAssess &&
        result.fovPath == overlay::FovPathAssessment::CannotAssess &&
        result.overall == overlay::OverallIntegrationAssessment::CannotAssess,
        "native_aspect_does_not_claim_active_gameplay_fov_path");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.runtimeViewportAspect.value = std::numeric_limits<float>::quiet_NaN();
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::CannotAssess &&
        !result.resolvedCinematicAspectValid,
        "nan_viewport_fails_closed");
    snapshot.runtimeViewportAspect.value = std::numeric_limits<float>::infinity();
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::CannotAssess,
        "infinite_viewport_fails_closed");

    snapshot.runtimeViewportAspect.value = 32.0f / 9.0f;
    snapshot.runtimeViewportAspect.valid = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::CannotAssess,
        "invalid_viewport_fact_fails_closed");

    snapshot.runtimeViewportAspect.valid = true;
    snapshot.cinematicFovLifecycleAvailable.value = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.fovPath == overlay::FovPathAssessment::CannotAssess &&
        result.overall == overlay::OverallIntegrationAssessment::CannotAssess,
        "missing_fov_lifecycle_capability_prevents_alignment");

    snapshot.cinematicFovLifecycleAvailable.value = true;
    snapshot.cinematicAspectComponentAvailable.value = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.aspect == overlay::AspectAssessment::Matched &&
        result.overall == overlay::OverallIntegrationAssessment::CannotAssess,
        "missing_aspect_component_prevents_alignment");

    snapshot.cinematicAspectComponentAvailable.value = true;
    snapshot.gameplayEnabled.value = false;
    result = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(result.fovPath == overlay::FovPathAssessment::GameplayLinked &&
        result.overall != overlay::OverallIntegrationAssessment::ConfigurationAligned,
        "disabled_gameplay_never_reports_aligned");

    std::cout << "Overlay camera integration harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
