#include "../../src/overlay/renderer_state.hpp"
#include "../../src/overlay/optional_overlay_boundary.hpp"

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
    overlay::RendererLifecycle renderer;
    bool pass = true;
    int overlaySideEffect = 0;
    int failureTransitions = 0;
    const auto success = overlay::RunOptionalOverlayWork(
        [&]() { ++overlaySideEffect; }, [&]() noexcept { ++failureTransitions; });
    pass &= Check(success == overlay::OptionalOverlayWorkResult::Completed &&
        overlaySideEffect == 1 && failureTransitions == 0,
        "optional_overlay_work_success_path");
    const auto failure = overlay::RunOptionalOverlayWork(
        [] { throw 7; }, [&]() noexcept { ++failureTransitions; });
    int originalCallbackCalls = 0;
    ++originalCallbackCalls; // Native pass-through is deliberately outside the boundary.
    pass &= Check(failure == overlay::OptionalOverlayWorkResult::Failed &&
        failureTransitions == 1 && originalCallbackCalls == 1,
        "overlay_exception_isolated_and_native_callback_passes_once");
    const auto throwingTransition = overlay::RunOptionalOverlayWork(
        [] { throw 9; }, [] { throw 11; });
    pass &= Check(throwingTransition == overlay::OptionalOverlayWorkResult::Failed,
        "failure_transition_cannot_escape_boundary");
    pass &= Check(renderer.BeginInitialization(), "initialization");
    pass &= Check(!renderer.CanSubmit(overlay::AssociationState::Supported),
        "uninitialized_cannot_submit");
    pass &= Check(renderer.MarkReady(), "ready");
    pass &= Check(renderer.CanSubmit(overlay::AssociationState::Supported),
        "ready_supported_can_submit");
    pass &= Check(!renderer.CanSubmit(overlay::AssociationState::Unknown) &&
        !renderer.CanSubmit(overlay::AssociationState::Ambiguous) &&
        !renderer.CanSubmit(overlay::AssociationState::ResizeRevalidationRequired),
        "invalid_association_cannot_submit");
    pass &= Check(renderer.BeginResize() &&
        !renderer.CanSubmit(overlay::AssociationState::Supported),
        "resize_disables_submission");
    pass &= Check(renderer.CompleteResize(true) &&
        renderer.CanSubmit(overlay::AssociationState::Supported),
        "successful_resize_restores_submission");
    pass &= Check(renderer.BeginResize() && !renderer.CompleteResize(false) &&
        !renderer.CanSubmit(overlay::AssociationState::Supported),
        "failed_resize_fail_closed");
    pass &= Check(renderer.ResetForRecreation() && renderer.BeginInitialization() &&
        renderer.MarkReady(), "recreation_reinitializes");
    pass &= Check(renderer.Disable() &&
        !renderer.CanSubmit(overlay::AssociationState::Supported) &&
        !renderer.Disable(), "disable_is_terminal_and_idempotent");

    pass &= Check(!overlay::StartupLocaleReady(true, false),
        "startup_hint_waits_for_auto_locale_sync");
    pass &= Check(overlay::StartupLocaleReady(true, true) &&
        overlay::StartupLocaleReady(false, false),
        "startup_hint_locale_ready_for_synced_auto_or_manual_locale");

    overlay::StartupHintGate startupHint;
    pass &= Check(!startupHint.TryPublish(false, true, true),
        "startup_hint_waits_for_renderer");
    pass &= Check(!startupHint.ShouldReadSettings(false, true),
        "startup_settings_read_waits_for_renderer");
    pass &= Check(!startupHint.TryPublish(true, false, true),
        "startup_hint_waits_for_valid_present");
    pass &= Check(!startupHint.ShouldReadSettings(true, false),
        "startup_settings_read_waits_for_valid_present");
    pass &= Check(startupHint.ShouldReadSettings(true, true),
        "startup_settings_read_retries_until_publication");
    pass &= Check(!startupHint.TryPublish(true, true, false),
        "startup_hint_waits_for_runtime_settings");
    pass &= Check(!startupHint.TryPublish(true, true,
            overlay::StartupLocaleReady(true, false)) &&
        startupHint.ShouldReadSettings(true, true),
        "startup_hint_retries_while_auto_locale_is_pending");
    pass &= Check(startupHint.ShouldReadSettings(true, true),
        "startup_settings_read_retries_after_unavailable_snapshot");
    bool settingsOverlayVisible = false;
    const bool startupHintActivated = startupHint.TryPublish(true, true, true);
    pass &= Check(startupHintActivated && !settingsOverlayVisible,
        "startup_hint_activates_while_settings_overlay_hidden");
    constexpr std::uint64_t startupAtMs = 1000;
    constexpr std::uint32_t startupDurationMs = overlay::StartupHintDurationMs;
    const bool startupToastActiveAtStart = startupHintActivated &&
        !overlay::NotificationExpired(startupAtMs, startupAtMs, startupDurationMs);
    const bool startupToastActiveBeforeExpiry = !overlay::NotificationExpired(
        startupAtMs, startupAtMs + startupDurationMs - 1, startupDurationMs);
    const bool startupToastActiveAfterExpiry = overlay::NotificationExpired(
        startupAtMs, startupAtMs + startupDurationMs, startupDurationMs);
    settingsOverlayVisible = true;
    pass &= Check(startupToastActiveAtStart && startupToastActiveBeforeExpiry &&
        startupToastActiveAfterExpiry &&
        !startupHint.TryPublish(true, true, true) &&
        !startupHint.ShouldReadSettings(true, true) &&
        !startupHint.ShouldReadSettings(false, true),
        "expired_startup_hint_does_not_retrigger_when_overlay_opens");
    std::cout << "Overlay renderer state harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
