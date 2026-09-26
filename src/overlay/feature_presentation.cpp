#include "feature_presentation.hpp"

namespace overlay
{
    namespace
    {
        FeaturePresentation Make(FeatureStatusKind kind, FeatureStatusTone tone,
            loc::Key label, loc::Key detail) noexcept
        {
            return {kind, tone, label, detail};
        }

        bool HasPendingGameplayWork(
            const plugin::OverlaySemanticSnapshot& snapshot) noexcept
        {
            return snapshot.gameplayEnableApplyPending.value ||
                snapshot.gameplayDisableRestorePending.value ||
                snapshot.gameplayModeTransitionPending.value;
        }
    }

    FeaturePresentation ProjectGameplayPresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept
    {
        if (!snapshot.gameplayEnabled.valid || !snapshot.gameplayMode.valid ||
            !snapshot.gameplayHookAvailable.valid ||
            !snapshot.gameplayEnableApplyPending.valid ||
            !snapshot.gameplayDisableRestorePending.valid ||
            !snapshot.gameplayModeTransitionPending.valid) {
            return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                loc::common::CannotAssess, loc::status::GameplayIncomplete);
        }
        if (!snapshot.gameplayHookAvailable.value) {
            return Make(FeatureStatusKind::Unavailable, FeatureStatusTone::Error,
                loc::common::Unavailable, loc::status::GameplayHookUnavailable);
        }
        if (HasPendingGameplayWork(snapshot)) {
            return Make(FeatureStatusKind::Applying, FeatureStatusTone::Warning,
                loc::common::Applying, loc::status::CameraUpdateWaiting);
        }
        if (!snapshot.gameplayEnabled.value) {
            if (snapshot.gameplayMode.value == config::GameplayMode::AspectRecalculation) {
                return Make(FeatureStatusKind::Disabled, FeatureStatusTone::Warning,
                    loc::common::Disabled, loc::status::NativeRecalculationPersists);
            }
            return Make(FeatureStatusKind::Disabled, FeatureStatusTone::Neutral,
                loc::common::Disabled, loc::status::GameplayDisabled);
        }
        if (snapshot.gameplayMode.value == config::GameplayMode::HorPlus) {
            if (!snapshot.gameplayBaselineUsable.valid || !snapshot.coordinator.valid) {
                return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                    loc::common::CannotAssess, loc::status::GameplayContextUnavailable);
            }
            if (!snapshot.gameplayBaselineUsable.value ||
                snapshot.coordinator.value != plugin::OverlayCoordinatorState::Gameplay) {
                return Make(FeatureStatusKind::Waiting, FeatureStatusTone::Warning,
                    loc::common::Waiting, loc::status::HorPlusWaiting);
            }
            return Make(FeatureStatusKind::Active, FeatureStatusTone::Positive,
                loc::common::Active, loc::status::HorPlusActive);
        }
        return Make(FeatureStatusKind::EnabledStateful, FeatureStatusTone::Warning,
            loc::common::Applying, loc::status::NativeRecalculationEnabled);
    }

    FeaturePresentation ProjectCinematicPresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept
    {
        if (!snapshot.cinematicAspectPolicy.valid || !snapshot.cinematicFovMode.valid ||
            !snapshot.cinematicAspectComponentAvailable.valid ||
            !snapshot.cinematicFovLifecycleAvailable.valid ||
            !snapshot.cinematicSelectionActive.valid) {
            return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                loc::common::CannotAssess, loc::status::CinematicIncomplete);
        }
        if (!snapshot.cinematicAspectComponentAvailable.value ||
            !snapshot.cinematicFovLifecycleAvailable.value) {
            return Make(FeatureStatusKind::Unavailable, FeatureStatusTone::Error,
                loc::common::Unavailable, loc::status::CinematicUnavailable);
        }
        if (!snapshot.cinematicSelectionActive.value) {
            if (snapshot.cinematicAspectPolicy.value == config::CinematicAspectPolicy::Native) {
                return Make(FeatureStatusKind::Ready, FeatureStatusTone::Neutral,
                    loc::common::Ready, loc::status::CinematicNativeReady);
            }
            return Make(FeatureStatusKind::Ready, FeatureStatusTone::Positive,
                loc::common::Ready, loc::status::NextCinematicReady);
        }
        if (!snapshot.activeCinematicAspectPolicy.valid ||
            !snapshot.activeCinematicFovMode.valid) {
            return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                loc::common::CannotAssess, loc::status::CinematicSelectionIncomplete);
        }
        return Make(FeatureStatusKind::Active, FeatureStatusTone::Positive,
            loc::common::Active, {});
    }

    FeaturePresentation ProjectDialoguePresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept
    {
        if (!snapshot.dialogueZoomPolicy.valid ||
            !snapshot.dialogueBoundaryHookAvailable.valid ||
            !snapshot.dialoguePhase.valid) {
            return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                loc::common::CannotAssess, loc::status::DialogueIncomplete);
        }
        if (!snapshot.dialogueBoundaryHookAvailable.value) {
            return Make(FeatureStatusKind::Unavailable, FeatureStatusTone::Error,
                loc::common::Unavailable, loc::status::DialogueHookUnavailable);
        }
        if (snapshot.dialogueZoomPolicy.value != config::DialogueZoomPolicy::Native) {
            if (!snapshot.dialogueNonNativeCapabilityAvailable.valid) {
                return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                    loc::common::CannotAssess, loc::status::DialogueCapabilityUnavailable);
            }
            if (!snapshot.dialogueNonNativeCapabilityAvailable.value) {
                return Make(FeatureStatusKind::Unavailable, FeatureStatusTone::Error,
                    loc::common::Unavailable, loc::status::DialogueModeUnavailable);
            }
        }

        switch (snapshot.dialoguePhase.value) {
        case plugin::OverlayDialoguePhase::Inactive:
            switch (snapshot.dialogueZoomPolicy.value) {
            case config::DialogueZoomPolicy::Native:
                return Make(FeatureStatusKind::Ready, FeatureStatusTone::Neutral,
                    loc::common::Ready, loc::status::DialogueNativeReady);
            case config::DialogueZoomPolicy::Adaptive:
                return Make(FeatureStatusKind::Ready, FeatureStatusTone::Positive,
                    loc::common::Ready, loc::status::DialogueAdaptiveReady);
            case config::DialogueZoomPolicy::Reduced:
                return Make(FeatureStatusKind::Ready, FeatureStatusTone::Positive,
                    loc::common::Ready, loc::status::DialogueReducedReady);
            case config::DialogueZoomPolicy::Disabled:
                return Make(FeatureStatusKind::Ready, FeatureStatusTone::Neutral,
                    loc::common::Ready, loc::status::DialogueDisabledReady);
            }
            return Make(FeatureStatusKind::Ready, FeatureStatusTone::Positive,
                loc::common::Ready, loc::status::DialogueReady);
        case plugin::OverlayDialoguePhase::Candidate:
            return Make(FeatureStatusKind::Starting, FeatureStatusTone::Warning,
                loc::common::Starting, loc::status::DialogueStarting);
        case plugin::OverlayDialoguePhase::Active:
            if (!snapshot.activeDialogueZoomPolicy.valid) {
                return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
                loc::common::CannotAssess, loc::status::ActiveDialoguePolicyUnavailable);
            }
            return Make(FeatureStatusKind::Active, FeatureStatusTone::Positive,
                loc::common::Active, {});
        case plugin::OverlayDialoguePhase::Exiting:
            return Make(FeatureStatusKind::Ending, FeatureStatusTone::Warning,
                loc::common::Ending, loc::status::DialogueEnding);
        case plugin::OverlayDialoguePhase::RearmPending:
            return Make(FeatureStatusKind::Waiting, FeatureStatusTone::Warning,
                loc::common::Waiting, loc::status::DialogueRecoveryWaiting);
        }
        return Make(FeatureStatusKind::CannotAssess, FeatureStatusTone::Neutral,
            loc::common::CannotAssess, loc::status::DialogueStateUnknown);
    }
}
