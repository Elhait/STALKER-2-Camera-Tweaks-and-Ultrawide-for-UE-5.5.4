#include "../../src/overlay/feature_presentation.hpp"

#include <iostream>

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
        snapshot.gameplayEnabled = {true, true};
        snapshot.gameplayMode = {config::GameplayMode::HorPlus, true};
        snapshot.gameplayHookAvailable = {true, true};
        snapshot.gameplayEnableApplyPending = {false, true};
        snapshot.gameplayDisableRestorePending = {false, true};
        snapshot.gameplayModeTransitionPending = {false, true};
        snapshot.gameplayBaselineUsable = {true, true};
        snapshot.coordinator = {plugin::OverlayCoordinatorState::Gameplay, true};
        snapshot.cinematicAspectComponentAvailable = {true, true};
        snapshot.cinematicFovLifecycleAvailable = {true, true};
        snapshot.cinematicAspectPolicy = {config::CinematicAspectPolicy::Auto, true};
        snapshot.cinematicFovMode = {config::CinematicFovMode::GameplayHorPlus, true};
        snapshot.cinematicSelectionActive = {false, true};
        snapshot.activeCinematicAspectPolicy = {config::CinematicAspectPolicy::Auto, true};
        snapshot.activeCinematicFovMode = {config::CinematicFovMode::GameplayHorPlus, true};
        snapshot.dialogueZoomPolicy = {config::DialogueZoomPolicy::Adaptive, true};
        snapshot.dialogueBoundaryHookAvailable = {true, true};
        snapshot.dialogueNonNativeCapabilityAvailable = {true, true};
        snapshot.dialoguePhase = {plugin::OverlayDialoguePhase::Inactive, true};
        snapshot.activeDialogueZoomPolicy = {config::DialogueZoomPolicy::Adaptive, true};
        return snapshot;
    }
}

int main()
{
    bool pass = true;
    auto snapshot = BaseSnapshot();
    auto status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Active &&
        status.tone == overlay::FeatureStatusTone::Positive, "horplus_enabled_is_active");
    snapshot.gameplayBaselineUsable.value = false;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Waiting &&
        status.tone == overlay::FeatureStatusTone::Warning, "missing_gameplay_baseline_is_waiting");
    snapshot.gameplayBaselineUsable.value = true;
    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicActive;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Waiting,
        "non_gameplay_coordinator_does_not_claim_gameplay_active");
    snapshot.coordinator.value = plugin::OverlayCoordinatorState::Gameplay;

    snapshot.gameplayEnabled.value = false;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Disabled, "gameplay_disabled");

    snapshot.gameplayMode.value = config::GameplayMode::AspectRecalculation;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Disabled &&
        status.tone == overlay::FeatureStatusTone::Warning &&
        !status.detail.path.empty(), "native_recalculation_persistence_is_disclosed");

    snapshot.gameplayEnabled.value = true;
    snapshot.gameplayModeTransitionPending.value = true;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Applying &&
        status.tone == overlay::FeatureStatusTone::Warning, "gameplay_pending_is_warning");

    snapshot.gameplayModeTransitionPending.value = false;
    snapshot.gameplayHookAvailable.value = false;
    status = overlay::ProjectGameplayPresentation(snapshot);
    pass &= Check(status.kind == overlay::FeatureStatusKind::Unavailable &&
        status.tone == overlay::FeatureStatusTone::Error, "gameplay_hook_failure_is_error");

    snapshot = BaseSnapshot();
    auto cinematic = overlay::ProjectCinematicPresentation(snapshot);
    pass &= Check(cinematic.kind == overlay::FeatureStatusKind::Ready,
        "cinematic_inactive_is_ready_for_next");
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Native;
    cinematic = overlay::ProjectCinematicPresentation(snapshot);
    pass &= Check(cinematic.kind == overlay::FeatureStatusKind::Ready &&
        cinematic.tone == overlay::FeatureStatusTone::Neutral, "native_cinematic_is_ready_without_claiming_correction");
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.cinematicSelectionActive.value = true;
    cinematic = overlay::ProjectCinematicPresentation(snapshot);
    pass &= Check(cinematic.kind == overlay::FeatureStatusKind::Active,
        "cinematic_active_requires_observed_selection");
    snapshot.activeCinematicFovMode.valid = false;
    cinematic = overlay::ProjectCinematicPresentation(snapshot);
    pass &= Check(cinematic.kind == overlay::FeatureStatusKind::CannotAssess,
        "cinematic_active_missing_policy_fails_closed");
    snapshot.cinematicSelectionActive.value = false;
    snapshot.activeCinematicFovMode.valid = true;
    snapshot.cinematicAspectComponentAvailable.value = false;
    cinematic = overlay::ProjectCinematicPresentation(snapshot);
    pass &= Check(cinematic.kind == overlay::FeatureStatusKind::Unavailable,
        "cinematic_capability_failure_is_error");

    snapshot = BaseSnapshot();
    auto dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::Ready,
        "dialogue_inactive_is_ready_for_next");
    snapshot.dialogueZoomPolicy.value = config::DialogueZoomPolicy::Disabled;
    dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::Ready &&
        dialogue.tone == overlay::FeatureStatusTone::Neutral, "disabled_dialogue_is_ready_without_added_zoom");
    snapshot.dialogueZoomPolicy.value = config::DialogueZoomPolicy::Adaptive;
    snapshot.dialoguePhase.value = plugin::OverlayDialoguePhase::Active;
    dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::Active,
        "dialogue_active_requires_lifecycle_and_active_policy");
    snapshot.activeDialogueZoomPolicy.valid = false;
    dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::CannotAssess,
        "dialogue_active_missing_policy_fails_closed");
    snapshot.activeDialogueZoomPolicy.valid = true;
    snapshot.dialoguePhase.value = plugin::OverlayDialoguePhase::RearmPending;
    dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::Waiting &&
        dialogue.tone == overlay::FeatureStatusTone::Warning, "dialogue_rearm_is_waiting");
    snapshot.dialoguePhase.value = plugin::OverlayDialoguePhase::Inactive;
    snapshot.dialogueZoomPolicy.value = config::DialogueZoomPolicy::Reduced;
    snapshot.dialogueNonNativeCapabilityAvailable.value = false;
    dialogue = overlay::ProjectDialoguePresentation(snapshot);
    pass &= Check(dialogue.kind == overlay::FeatureStatusKind::Unavailable,
        "non_native_dialogue_capability_failure_is_error");

    std::cout << "Overlay feature presentation harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
