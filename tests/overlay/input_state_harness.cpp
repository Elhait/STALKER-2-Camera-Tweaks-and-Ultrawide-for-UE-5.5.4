#include "../../src/overlay/input_state.hpp"
#include "../../src/overlay/hotkey_binding_presentation.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }
}

int main()
{
    overlay::InputState input;
    bool pass = true;
    config::FeatureConfig configuredBindings{};
    plugin::RuntimeSettingsSnapshot displayedBindings{};
    for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
        const auto& binding = config::HotkeyBindingRegistry[index];
        const auto* presentation = overlay::FindHotkeyPresentation(binding.id);
        const int key = VK_F1 + static_cast<int>(index);
        config::SetHotkeyBindingValue(configuredBindings, binding.id, key);
        pass &= Check(presentation && !presentation->label.path.empty(),
            "every_config_binding_has_a_ui_label");
        if (!presentation) continue;
        const plugin::RuntimeSettingsSnapshot runtimeDefaults{};
        pass &= Check(runtimeDefaults.*(presentation->settingKey) ==
                binding.defaultVirtualKey,
            "runtime_default_matches_binding_metadata");
        displayedBindings.*(presentation->settingKey) = key;
        pass &= Check(config::HotkeyBindingValue(configuredBindings, binding.id) ==
                displayedBindings.*(presentation->settingKey),
            "ui_and_config_read_the_same_binding_identity");
        input.BeginRebind(binding.id, key);
        pass &= Check(input.rebindAction() == binding.id &&
                input.IsCapturing(binding.id),
            "input_rebind_uses_config_binding_identity");
        input.CancelRebind();
        for (std::size_t other = index + 1;
            other < config::HotkeyBindingRegistry.size(); ++other)
            pass &= Check(!overlay::FindHotkeyPresentation(
                    config::HotkeyBindingRegistry[other].id) ||
                    config::HotkeyBindingRegistry[other].id != binding.id,
                "binding_identities_are_unique");
    }
    pass &= Check(overlay::HotkeyBindingPresentations.size() ==
            config::HotkeyBindingRegistry.size(),
        "ui_and_config_binding_inventory_sizes_match");
    pass &= Check(!input.visible() && !input.ShouldCapture(WM_MOUSEMOVE),
        "hidden_by_default");
    pass &= Check(overlay::InputState::IsRebindMessage(WM_KEYDOWN) &&
        overlay::InputState::IsRebindMessage(WM_KEYUP) &&
        overlay::InputState::IsRebindMessage(WM_SYSKEYDOWN) &&
        overlay::InputState::IsRebindMessage(WM_SYSKEYUP) &&
        !overlay::InputState::IsRebindMessage(WM_MOUSEMOVE) &&
        !overlay::InputState::IsRebindMessage(WM_INPUT) &&
        !overlay::InputState::IsRebindMessage(WM_CHAR),
        "only_keyboard_message_classes_enter_rebind_path");
    pass &= Check(overlay::InputState::IsRebindKeyDownMessage(WM_KEYDOWN) &&
        overlay::InputState::IsRebindKeyDownMessage(WM_SYSKEYDOWN) &&
        !overlay::InputState::IsRebindKeyDownMessage(WM_KEYUP) &&
        !overlay::InputState::IsRebindKeyDownMessage(WM_SYSKEYUP),
        "modifier_queries_only_for_key_down_classes");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    pass &= Check(input.HandleRebindMessage(WM_MOUSEMOVE, 0) ==
        overlay::RebindMessageResult::PassThrough && input.rebindActive(),
        "irrelevant_message_passes_without_changing_rebind_state");
    input.CancelRebind();
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible(),
        "delete_opens_overlay_by_default");
    pass &= Check(input.ShouldConsumeEscapeFromGame(WM_KEYDOWN, VK_ESCAPE) &&
        input.ShouldConsumeEscapeFromGame(WM_KEYUP, VK_ESCAPE) &&
        !input.ShouldConsumeEscapeFromGame(WM_CHAR, VK_ESCAPE),
        "open_overlay_consumes_escape_keyboard_messages_from_game");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, false) &&
        input.visible() &&
        input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        input.HandleToggleMessage(WM_KEYUP, VK_ESCAPE, VK_ESCAPE) && !input.visible(),
        "escape_dismisses_open_overlay_through_toggle_state_path");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, false) &&
        !input.visible() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        !input.ShouldConsumeEscapeFromGame(WM_KEYDOWN, VK_ESCAPE) &&
        !input.ShouldConsumeEscapeFromGame(WM_KEYUP, VK_ESCAPE) &&
        !input.HandleToggleMessage(WM_KEYUP, VK_ESCAPE, 0) && !input.visible(),
        "escape_never_opens_closed_overlay");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, true) &&
        input.ConsumeEscapePopupDismissRequest() &&
        !input.ConsumeEscapePopupDismissRequest() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        input.visible(),
        "child_popup_requests_own_close_and_owns_escape_press_until_keyup");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, true) &&
        input.visible(),
        "currently_active_child_escape_consumer_has_priority");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && !input.visible() &&
        input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible(),
        "configured_toggle_still_closes_and_opens_after_escape_cases");
    pass &= Check(input.ShouldCapture(WM_MOUSEMOVE) &&
        input.ShouldCapture(WM_LBUTTONDOWN) && input.ShouldCapture(WM_MOUSEWHEEL) &&
        !input.ShouldCapture(WM_INPUT) && input.ShouldCaptureRawInput(true) &&
        !input.ShouldCaptureRawInput(false), "visible_captures_mouse_only");
    pass &= Check(!input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_KEYUP) && !input.ShouldCapture(WM_CHAR) &&
        !input.ShouldCapture(WM_SYSKEYDOWN), "visible_forwards_keyboard");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE), "toggle_message");
    pass &= Check(!input.visible() &&
        !input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_MOUSEMOVE) && !input.ShouldCapture(WM_INPUT) &&
        !input.ShouldCaptureRawInput(true) && !input.ShouldCaptureRawInput(false),
        "hidden_preserves_game_input");
    pass &= Check(!input.HandleToggleMessage(WM_KEYDOWN, VK_DELETE),
        "keydown_does_not_toggle");
    pass &= Check(!input.HandleToggleMessage(WM_KEYUP, 'F'),
        "other_key_does_not_toggle");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible() &&
        !input.ShouldCapture(WM_KEYDOWN) && input.ShouldCapture(WM_RBUTTONDOWN),
        "second_toggle_restores_mouse_only_overlay");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    pass &= Check(input.rebindActive() &&
        input.HandleRebindMessage(WM_KEYDOWN, VK_F24) ==
            overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "extended_function_key_is_captured");
    input.CommitAcceptedRebind(VK_F24);
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, 0xC1) ==
        overlay::RebindMessageResult::PassThrough,
        "undefined_virtual_key_is_not_captured");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_F6, true) ==
        overlay::RebindMessageResult::PassThrough && input.rebindActive(),
        "modifier_combination_is_not_bound");
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_F6) ==
        overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "supported_key_is_captured");
    input.CommitAcceptedRebind(VK_F6);
    pass &= Check(!input.rebindActive() &&
        input.previousBinding() == VK_F6 &&
        input.HandleRebindMessage(WM_KEYUP, VK_F6) ==
            overlay::RebindMessageResult::Consumed &&
        input.HandleRebindMessage(WM_KEYDOWN, VK_F6) ==
            overlay::RebindMessageResult::PassThrough,
        "captured_press_consumed_next_press_passes");
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.BeginRebind(overlay::HotkeyAction::CinematicFov, VK_F11);
    pass &= Check(input.rebindActive() &&
        input.rebindAction() == overlay::HotkeyAction::CinematicFov &&
        input.previousBinding() == VK_F11,
        "only_latest_capture_is_active");
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_F10) ==
        overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "conflicting_candidate_does_not_commit_binding");
    pass &= Check(input.HandleRebindMessage(WM_KEYUP, VK_F10) ==
        overlay::RebindMessageResult::Consumed,
        "conflicting_candidate_release_consumed");
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_ESCAPE) ==
        overlay::RebindMessageResult::Cancelled && !input.rebindActive() &&
        input.previousBinding() == VK_F11,
        "escape_cancels_without_deadlock_and_restores_last_accepted_binding");
    pass &= Check(input.HandleRebindMessage(WM_KEYUP, VK_ESCAPE) ==
        overlay::RebindMessageResult::Consumed && input.visible(),
        "rebind_escape_press_consumed_without_closing_overlay");
    input.BeginRebind(overlay::HotkeyAction::CinematicAspect, VK_F10);
    const auto toggleCandidate = input.HandleRebindMessage(WM_KEYDOWN, VK_DELETE);
    const auto toggleRelease = input.HandleRebindMessage(WM_KEYUP, VK_DELETE);
    pass &= Check(toggleCandidate == overlay::RebindMessageResult::CapturedKey &&
        toggleRelease == overlay::RebindMessageResult::Consumed &&
        input.rebindActive() && input.visible(),
        "toggle_key_candidate_release_is_consumed_before_toggle_dispatch");
    input.CancelRebind();
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.CancelRebind();
    pass &= Check(!input.rebindActive() && input.previousBinding() == VK_F12,
        "focus_loss_or_teardown_cancels_without_changing_binding");
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.CommitAcceptedRebind('K');
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, 'K');
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_ESCAPE) ==
        overlay::RebindMessageResult::Cancelled && input.previousBinding() == 'K',
        "later_cancel_restores_most_recent_accepted_binding");
    input.HandleToggleMessage(WM_KEYUP, VK_DELETE);
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, 'K');
    input.Close();
    pass &= Check(!input.visible() && !input.rebindActive() &&
        !input.ShouldCapture(WM_MOUSEMOVE) &&
        input.HandleRebindMessage(WM_KEYUP, 'K') ==
            overlay::RebindMessageResult::PassThrough,
        "terminal_close_releases_capture_and_rebind_state");
    std::cout << "Overlay input state harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
