#pragma once

#include <string_view>
#include <span>

namespace overlay::loc
{
    struct Key { std::string_view path; };

    namespace app { inline constexpr Key Title{"app.title"}; }
    namespace common {
        inline constexpr Key Enabled{"common.enabled"};
        inline constexpr Key Disabled{"common.disabled"};
        inline constexpr Key Unavailable{"common.unavailable"};
        inline constexpr Key UnavailableValue{"common.unavailable_value"};
        inline constexpr Key CannotAssess{"common.cannot_assess"};
        inline constexpr Key Active{"common.active"};
        inline constexpr Key Ready{"common.ready"};
        inline constexpr Key Applying{"common.applying"};
        inline constexpr Key Starting{"common.starting"};
        inline constexpr Key Ending{"common.ending"};
        inline constexpr Key Waiting{"common.waiting"};
        inline constexpr Key Applied{"common.applied"};
        inline constexpr Key Pending{"common.pending"};
        inline constexpr Key Error{"common.error"};
        inline constexpr Key Yes{"common.yes"};
        inline constexpr Key No{"common.no"};
        inline constexpr Key Examples{"common.examples"};
    }
    namespace section {
        inline constexpr Key RuntimeSettings{"section.runtime_settings"};
        inline constexpr Key CameraTransition{"section.camera_transition"};
        inline constexpr Key Gameplay{"section.gameplay"};
        inline constexpr Key Cinematics{"section.cinematics"};
        inline constexpr Key Dialogue{"section.dialogue"};
        inline constexpr Key Overlay{"section.overlay"};
        inline constexpr Key Hotkeys{"section.hotkeys"};
        inline constexpr Key Runtime{"section.runtime"};
        inline constexpr Key CameraIntegration{"section.camera_integration"};
        inline constexpr Key CameraState{"section.camera_state"};
        inline constexpr Key Presentation{"section.presentation"};
        inline constexpr Key Zoom{"section.zoom"};
        inline constexpr Key Fov{"section.fov"};
        inline constexpr Key Sources{"section.sources"};
        inline constexpr Key Generation{"section.generation"};
    }
    namespace ui {
        inline constexpr Key ViewportAspect{"ui.viewport_aspect"};
    }
    namespace setting {
        inline constexpr Key GameplayMode{"setting.gameplay_mode"};
        inline constexpr Key CinematicAspect{"setting.cinematic_aspect"};
        inline constexpr Key CinematicFov{"setting.cinematic_fov"};
        inline constexpr Key DialogueZoom{"setting.dialogue_zoom"};
        inline constexpr Key OverlayToggle{"setting.overlay_toggle"};
        inline constexpr Key Language{"setting.language"};
        inline constexpr Key FontSize{"setting.font_size"};
        inline constexpr Key Mode{"setting.mode"};
        inline constexpr Key AspectRatio{"setting.aspect_ratio"};
        inline constexpr Key FovMode{"setting.fov_mode"};
        inline constexpr Key Zoom{"setting.zoom"};
    }
    namespace notice { inline constexpr Key MouseCapture{"notice.mouse_capture"}; }
    namespace rebind { inline constexpr Key PressKey{"rebind.press_key"}; }
    namespace transition {
        inline constexpr Key Waiting{"transition.waiting"};
        inline constexpr Key Aligned{"transition.aligned"};
        inline constexpr Key NotAligned{"transition.not_aligned"};
        inline constexpr Key DetailWaiting{"transition.detail.waiting"};
        inline constexpr Key DetailAligned{"transition.detail.aligned"};
        inline constexpr Key DetailNotAligned{"transition.detail.not_aligned"};
        inline constexpr Key DetailCannotAssess{"transition.detail.cannot_assess"};
        inline constexpr Key ActiveValue{"transition.active_value"};
    }
    namespace status {
        inline constexpr Key GameplayIncomplete{"status.detail.gameplay_incomplete"};
        inline constexpr Key GameplayHookUnavailable{"status.detail.gameplay_hook_unavailable"};
        inline constexpr Key CameraUpdateWaiting{"status.detail.camera_update_waiting"};
        inline constexpr Key NativeRecalculationPersists{"status.detail.native_recalculation_persists"};
        inline constexpr Key GameplayDisabled{"status.detail.gameplay_disabled"};
        inline constexpr Key GameplayContextUnavailable{"status.detail.gameplay_context_unavailable"};
        inline constexpr Key HorPlusWaiting{"status.detail.horplus_waiting"};
        inline constexpr Key HorPlusActive{"status.detail.horplus_active"};
        inline constexpr Key NativeRecalculationEnabled{"status.detail.native_recalculation_enabled"};
        inline constexpr Key CinematicIncomplete{"status.detail.cinematic_incomplete"};
        inline constexpr Key CinematicUnavailable{"status.detail.cinematic_unavailable"};
        inline constexpr Key CinematicNativeReady{"status.detail.cinematic_native_ready"};
        inline constexpr Key NextCinematicReady{"status.detail.next_cinematic_ready"};
        inline constexpr Key CinematicActive{"status.detail.cinematic_active"};
        inline constexpr Key CinematicSelectionIncomplete{"status.detail.cinematic_selection_incomplete"};
        inline constexpr Key DialogueIncomplete{"status.detail.dialogue_incomplete"};
        inline constexpr Key DialogueHookUnavailable{"status.detail.dialogue_hook_unavailable"};
        inline constexpr Key DialogueCapabilityUnavailable{"status.detail.dialogue_capability_unavailable"};
        inline constexpr Key DialogueModeUnavailable{"status.detail.dialogue_mode_unavailable"};
        inline constexpr Key DialogueNativeReady{"status.detail.dialogue_native_ready"};
        inline constexpr Key DialogueAdaptiveReady{"status.detail.dialogue_adaptive_ready"};
        inline constexpr Key DialogueReducedReady{"status.detail.dialogue_reduced_ready"};
        inline constexpr Key DialogueDisabledReady{"status.detail.dialogue_disabled_ready"};
        inline constexpr Key DialogueReady{"status.detail.dialogue_ready"};
        inline constexpr Key DialogueActive{"status.detail.dialogue_active"};
        inline constexpr Key DialogueStarting{"status.detail.dialogue_starting"};
        inline constexpr Key ActiveDialoguePolicyUnavailable{"status.detail.active_dialogue_policy_unavailable"};
        inline constexpr Key DialogueEnding{"status.detail.dialogue_ending"};
        inline constexpr Key DialogueRecoveryWaiting{"status.detail.dialogue_recovery_waiting"};
        inline constexpr Key DialogueStateUnknown{"status.detail.dialogue_state_unknown"};
        inline constexpr Key RuntimeUnavailable{"status.detail.runtime_unavailable"};
    }
    namespace tooltip {
        namespace gameplay {
            inline constexpr Key Enabled{"tooltip.gameplay.enabled"};
            inline constexpr Key Disabled{"tooltip.gameplay.disabled"};
            inline constexpr Key HorPlus{"tooltip.gameplay.horplus"};
            inline constexpr Key HorPlusExamplesContext{"tooltip.gameplay.horplus_examples_context"};
            inline constexpr Key HorPlusExample90{"tooltip.gameplay.horplus_example_90"};
            inline constexpr Key HorPlusExample100{"tooltip.gameplay.horplus_example_100"};
            inline constexpr Key HorPlusExample110{"tooltip.gameplay.horplus_example_110"};
            inline constexpr Key AspectRecalculation{"tooltip.gameplay.aspect_recalculation"};
        }
        namespace cinematics {
            inline constexpr Key AspectAuto{"tooltip.cinematics.aspect.auto"};
            inline constexpr Key AspectNative{"tooltip.cinematics.aspect.native"};
            inline constexpr Key Aspect169{"tooltip.cinematics.aspect.16_9"};
            inline constexpr Key Aspect219{"tooltip.cinematics.aspect.21_9"};
            inline constexpr Key Aspect329{"tooltip.cinematics.aspect.32_9"};
            inline constexpr Key AspectForcedExample{"tooltip.cinematics.aspect.forced_example"};
            inline constexpr Key FovGameplay{"tooltip.cinematics.fov.gameplay"};
            inline constexpr Key FovGameplayExamplesContext{"tooltip.cinematics.fov.gameplay_examples_context"};
            inline constexpr Key FovGameplayExample90{"tooltip.cinematics.fov.gameplay_example_90"};
            inline constexpr Key FovGameplayExample112_6{"tooltip.cinematics.fov.gameplay_example_112_6"};
            inline constexpr Key FovNative{"tooltip.cinematics.fov.native"};
            inline constexpr Key FovNativeExamplesContext{"tooltip.cinematics.fov.native_examples_context"};
            inline constexpr Key FovNativeExample90{"tooltip.cinematics.fov.native_example_90"};
        }
        namespace dialogue {
            inline constexpr Key ZoomNative{"tooltip.dialogue.zoom.native"};
            inline constexpr Key ZoomExamplesContext{"tooltip.dialogue.zoom.examples_context"};
            inline constexpr Key ZoomNativeExample90{"tooltip.dialogue.zoom.native_example_90"};
            inline constexpr Key ZoomNativeExample110{"tooltip.dialogue.zoom.native_example_110"};
            inline constexpr Key ZoomAdaptive{"tooltip.dialogue.zoom.adaptive"};
            inline constexpr Key ZoomAdaptiveExample90{"tooltip.dialogue.zoom.adaptive_example_90"};
            inline constexpr Key ZoomAdaptiveExample110{"tooltip.dialogue.zoom.adaptive_example_110"};
            inline constexpr Key ZoomReduced{"tooltip.dialogue.zoom.reduced"};
            inline constexpr Key ZoomReducedExample90{"tooltip.dialogue.zoom.reduced_example_90"};
            inline constexpr Key ZoomReducedExample110{"tooltip.dialogue.zoom.reduced_example_110"};
            inline constexpr Key ZoomDisabled{"tooltip.dialogue.zoom.disabled"};
            inline constexpr Key ZoomDisabledExample90{"tooltip.dialogue.zoom.disabled_example_90"};
            inline constexpr Key ZoomDisabledExample110{"tooltip.dialogue.zoom.disabled_example_110"};
        }
        namespace hotkeys {
            inline constexpr Key Enabled{"tooltip.hotkeys.enabled"};
            inline constexpr Key Disabled{"tooltip.hotkeys.disabled"};
            inline constexpr Key Change{"tooltip.hotkeys.change"};
            inline constexpr Key ChangeOverlay{"tooltip.hotkeys.change_overlay"};
            inline constexpr Key SingleKey{"tooltip.hotkeys.single_key"};
            inline constexpr Key Rebind{"tooltip.hotkeys.rebind"};
            inline constexpr Key GameplayCycle{"tooltip.hotkeys.gameplay_cycle"};
            inline constexpr Key CinematicAspectCycle{"tooltip.hotkeys.cinematic_aspect_cycle"};
            inline constexpr Key CinematicFovCycle{"tooltip.hotkeys.cinematic_fov_cycle"};
            inline constexpr Key DialogueCycle{"tooltip.hotkeys.dialogue_cycle"};
        }
        namespace overlay { inline constexpr Key AlwaysActive{"tooltip.overlay.always_active"}; }
        namespace runtime {
            inline constexpr Key Viewport{"tooltip.runtime.viewport"};
            inline constexpr Key ViewportAuto{"tooltip.runtime.viewport_auto"};
            inline constexpr Key ViewportUnavailable{"tooltip.runtime.viewport_unavailable"};
            inline constexpr Key AspectMatched{"tooltip.runtime.aspect_matched"};
            inline constexpr Key AspectMismatch{"tooltip.runtime.aspect_mismatch"};
            inline constexpr Key AspectCannotAssess{"tooltip.runtime.aspect_cannot_assess"};
            inline constexpr Key CinematicAspect{"tooltip.runtime.cinematic_aspect"};
            inline constexpr Key CinematicAspectUnavailable{"tooltip.runtime.cinematic_aspect_unavailable"};
            inline constexpr Key GameplayFov{"tooltip.runtime.gameplay_fov"};
            inline constexpr Key GameplayFovUnavailable{"tooltip.runtime.gameplay_fov_unavailable"};
            inline constexpr Key NativeFov{"tooltip.runtime.native_fov"};
            inline constexpr Key NativeFovUnavailable{"tooltip.runtime.native_fov_unavailable"};
            inline constexpr Key HorPlusFov{"tooltip.runtime.horplus_fov"};
            inline constexpr Key HorPlusFovUnavailable{"tooltip.runtime.horplus_fov_unavailable"};
        }
    }
    namespace camera_state {
        inline constexpr Key Waiting{"camera_state.waiting"};
        inline constexpr Key State{"camera_state.field.state"};
        inline constexpr Key GameplayFov{"camera_state.field.gameplay_fov"};
        inline constexpr Key Provenance{"camera_state.field.provenance"};
        inline constexpr Key Epoch{"camera_state.field.epoch"};
        inline constexpr Key Mode{"camera_state.field.mode"};
        inline constexpr Key Valid{"camera_state.field.valid"};
        inline constexpr Key Direction{"camera_state.field.direction"};
        inline constexpr Key Active{"camera_state.field.active"};
        inline constexpr Key EvidenceValid{"camera_state.field.evidence_valid"};
        inline constexpr Key Sequence{"camera_state.field.sequence"};
        inline constexpr Key Primary{"camera_state.field.primary"};
        inline constexpr Key Secondary{"camera_state.field.secondary"};
        inline constexpr Key Source{"camera_state.field.source"};
        inline constexpr Key Confidence{"camera_state.field.confidence"};
        inline constexpr Key PolicyValid{"camera_state.field.policy_valid"};
        inline constexpr Key RecoveryExcluded{"camera_state.field.recovery_excluded"};
        inline constexpr Key Target{"camera_state.field.target"};
        inline constexpr Key SettingKnown{"camera_state.field.setting_known"};
        inline constexpr Key GameSource{"camera_state.field.game_source"};
        inline constexpr Key Native{"camera_state.field.native"};
        inline constexpr Key NativeSource{"camera_state.field.native_source"};
        inline constexpr Key HorPlus{"camera_state.field.horplus"};
        inline constexpr Key HorPlusSource{"camera_state.field.horplus_source"};
        inline constexpr Key Aspect{"camera_state.field.aspect"};
        inline constexpr Key AspectSource{"camera_state.field.aspect_source"};
        inline constexpr Key Flags{"camera_state.field.flags"};
        inline constexpr Key GameplayWriter{"camera_state.field.gameplay_writer"};
        inline constexpr Key PresentationEpoch{"camera_state.field.presentation_epoch"};
        inline constexpr Key EventSequence{"camera_state.field.event_sequence"};
        inline constexpr Key UnavailableCounter{"camera_state.value.unavailable_counter"};
    }
    namespace notification {
        inline constexpr Key HotkeyConflict{"notification.hotkey_conflict"};
        inline constexpr Key HotkeyTitle{"notification.hotkey_title"};
        inline constexpr Key BindingValue{"notification.binding_value"};
        inline constexpr Key ConflictValue{"notification.conflict_value"};
        inline constexpr Key SettingChanged{"notification.setting_changed"};
        inline constexpr Key OpenOverlayHint{"notification.open_overlay_hint"};
    }

    // Keep the catalog inventory explicit; validator parity prevents unused
    // and missing translation entries from silently accumulating.
    std::span<const Key> Inventory() noexcept;
}
