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
    overlay::InputEventBridge bridge;
    std::vector<overlay::InputEvent> inputEvents;
    pass &= Check(!bridge.PushRelativeMouseMotion(1, 1),
        "closed_input_owner_does_not_capture_events");
    overlay::InputEventBridge zeroDeltaBridge;
    zeroDeltaBridge.Activate(40, 50);
    pass &= Check(!zeroDeltaBridge.PushRelativeMouseMotion(0, 0) &&
        zeroDeltaBridge.PushAbsoluteMousePosition(60, 70),
        "zero_delta_raw_packet_does_not_claim_pointer_motion_ownership");
    pass &= Check(zeroDeltaBridge.PushRelativeMouseMotion(2, -1) &&
        !zeroDeltaBridge.PushAbsoluteMousePosition(80, 90),
        "real_relative_motion_selects_the_raw_pointer_stream");

    bridge.Activate(100, 200);
    pass &= Check(bridge.active() && bridge.PushAbsoluteMousePosition(150, 250) &&
        bridge.PushRelativeMouseMotion(4, -3) &&
        !bridge.PushAbsoluteMousePosition(300, 300),
        "overlay_input_owner_switches_to_relative_raw_mouse_source");
    pass &= Check(bridge.PushMouseButton(WM_LBUTTONDOWN, 0) &&
        bridge.PushMouseWheel(WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0) &&
        bridge.PushKeyboardMessage(WM_KEYDOWN, VK_F9, 0),
        "host-window_events_are_copied_into_the_owner-thread_bridge");
    bridge.Drain(inputEvents);
    pass &= Check(inputEvents.size() == 8 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::AbsoluteMousePosition &&
        inputEvents[2].kind == overlay::InputEventKind::Focus &&
        inputEvents[2].wParam == TRUE &&
        inputEvents[3].kind == overlay::InputEventKind::AbsoluteMousePosition &&
        inputEvents[3].x == 150 && inputEvents[3].y == 250 &&
        inputEvents[4].kind == overlay::InputEventKind::RelativeMouseMotion &&
        inputEvents[4].x == 4 && inputEvents[4].y == -3 &&
        inputEvents[5].kind == overlay::InputEventKind::MouseButton &&
        inputEvents[6].kind == overlay::InputEventKind::MouseWheel &&
        inputEvents[7].kind == overlay::InputEventKind::NativeKeyboardMessage,
        "presenter_owner_receives_ordered_absolute_relative_and_click_events");
    bridge.Deactivate();
    pass &= Check(!bridge.active() && !bridge.PushMouseButton(WM_LBUTTONUP, 0),
        "closed_ui_returns_event_ownership_to_the_game");
    bridge.Drain(inputEvents);
    pass &= Check(inputEvents.size() == 2 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::Focus &&
        inputEvents[1].wParam == FALSE,
        "ownership_transition_resets_pointer_and_keyboard_focus_on_presenter_thread");

    overlay::InputState focusInput;
    overlay::InputEventBridge focusBridge;
    focusInput.Toggle();
    focusInput.SetFocused(true);
    focusBridge.Activate(15, 25);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent(),
        "focused_open_panel_owns_the_input_bridge");
    focusBridge.PushKeyboardMessage(WM_KEYDOWN, VK_SHIFT, 0);
    focusInput.SetFocused(false);
    focusBridge.Deactivate();
    focusBridge.Drain(inputEvents);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent() &&
        inputEvents.size() == 2 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::Focus &&
        inputEvents[1].wParam == FALSE,
        "focus_loss_clears_queued_keydown_and_releases_imgui_keyboard_state");
    focusInput.SetFocused(true);
    focusBridge.Activate(15, 25);
    focusBridge.Drain(inputEvents);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent() &&
        inputEvents.back().kind == overlay::InputEventKind::Focus &&
        inputEvents.back().wParam == TRUE,
        "focus_regain_reestablishes_owner_thread_keyboard_input");
    overlay::VirtualCursorPosition cursor;
    cursor.SetAbsolute(150, -5, 100, 50);
    pass &= Check(cursor.known() && cursor.point().x == 99 && cursor.point().y == 0,
        "absolute_seed_is_clamped_to_the_game_client_extent");
    cursor.ApplyRelative(-8, 10, 100, 50);
    pass &= Check(cursor.point().x == 91 && cursor.point().y == 10,
        "raw_relative_delta_moves_the_presenter_owned_cursor");
    cursor.ApplyRelative(1000, -100, 100, 50);
    pass &= Check(cursor.point().x == 99 && cursor.point().y == 0,
        "raw_relative_motion_is_clamped_without_os_cursor_warping");
    cursor.Reset();
    cursor.ApplyRelative(3, -2, 100, 50);
    pass &= Check(cursor.point().x == 53 && cursor.point().y == 23,
        "relative_input_after_focus_reset_reseeds_from_client_center");
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
    input.SetFocused(true);
    pass &= Check(input.ShouldOwnInput(),
        "panel and host focus jointly establish Overlay input ownership");
    pass &= Check(input.ShouldCapture(WM_MOUSEMOVE) &&
        input.ShouldCapture(WM_LBUTTONDOWN) && input.ShouldCapture(WM_MOUSEWHEEL) &&
        !input.ShouldCapture(WM_INPUT), "visible_captures_mouse_only");
    pass &= Check(!input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_KEYUP) && !input.ShouldCapture(WM_CHAR) &&
        !input.ShouldCapture(WM_SYSKEYDOWN), "visible_forwards_keyboard");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE), "toggle_message");
    pass &= Check(!input.visible() &&
        !input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_MOUSEMOVE) && !input.ShouldCapture(WM_INPUT),
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
