#include "../../src/camera/gameplay_aspect_restoration.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    bool SameState(const camera::GameplayAspectRestorationState& state,
        float aspect, bool valid, std::uintptr_t source)
    {
        return state.valid == valid && state.source.value == source &&
            (!valid || state.aspect == aspect);
    }

    camera::GameplayAspectRestorationDecisionResult Decide(
        bool pending, bool currentReadable, bool currentValid,
        bool currentUltrawide, bool restorationValid,
        bool restorationValidAspect, bool restorationUltrawide,
        float restorationAspect)
    {
        return camera::ResolveGameplayAspectRestorationDecision({
            pending, true, true, true, currentReadable, currentValid,
            currentUltrawide, restorationValid, restorationValidAspect,
            restorationUltrawide }, restorationAspect);
    }
}

int main()
{
    using namespace camera;
    bool pass = true;
    GameplayAspectRestorationStore store;

    const auto startup = store.Read();
    pass &= Check(!startup.valid && std::isnan(startup.aspect), "startup_invalid");

    const auto first = store.Update(2.38889f, { 0x1000, true });
    pass &= Check(first.disposition == GameplayAspectRestorationDisposition::Updated &&
        first.changed && SameState(first.state, 2.38889f, true, 0x1000),
        "normal_update");

    const auto same = store.Update(2.38889f, { 0x1000, true });
    pass &= Check(same.disposition == GameplayAspectRestorationDisposition::Updated &&
        !same.changed && same.state.observationSequence > first.state.observationSequence,
        "same_value_update_has_provenance");

    const auto invalidFov = store.Update(3.0f, { 0x2000, true });
    pass &= Check(invalidFov.changed && SameState(invalidFov.state, 3.0f, true, 0x2000),
        "valid_aspect_invalid_fov_modeled_update");

    const auto invalidFlags = store.Update(3.55556f, { 0x3000, true });
    pass &= Check(invalidFlags.changed &&
        SameState(invalidFlags.state, 3.55556f, true, 0x3000),
        "valid_aspect_invalid_flags_modeled_update");

    const auto autoRestore = store.Retain();
    pass &= Check(autoRestore.disposition == GameplayAspectRestorationDisposition::Retained &&
        SameState(autoRestore.state, 3.55556f, true, 0x3000),
        "fix_owned_auto_restore_retain");

    const auto pendingAutoRestore = store.Retain();
    pass &= Check(!pendingAutoRestore.changed &&
        SameState(pendingAutoRestore.state, 3.55556f, true, 0x3000),
        "pending_auto_restore_retain");

    const auto disabledGameplay = store.Update(2.4f, { 0x4000, true });
    pass &= Check(SameState(disabledGameplay.state, 2.4f, true, 0x4000),
        "gameplay_disabled_valid_aspect_update");

    const auto cinematic = store.Retain();
    pass &= Check(SameState(cinematic.state, 2.4f, true, 0x4000),
        "cinematic_active_does_not_contaminate");

    const auto invalidated = store.Invalidate();
    pass &= Check(invalidated.disposition == GameplayAspectRestorationDisposition::Invalidated &&
        invalidated.changed && !invalidated.state.valid &&
        !invalidated.state.source.valid && invalidated.state.observationSequence == 0,
        "mode_transition_invalidate");

    const auto repopulated = store.Update(3.0f, { 0x5000, true });
    pass &= Check(SameState(repopulated.state, 3.0f, true, 0x5000) &&
        repopulated.state.observationSequence > 0,
        "repopulate_after_invalidate");

    const auto invalidAspect = store.Retain();
    pass &= Check(SameState(invalidAspect.state, 3.0f, true, 0x5000),
        "invalid_aspect_retain_modeled");

    pass &= Check(Decide(false, true, true, false, true, true, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::NoAction, "no_pending_no_action");
    pass &= Check(Decide(true, false, false, false, true, true, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::Defer, "unreadable_current_defers");
    pass &= Check(Decide(true, true, false, false, true, true, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::Defer, "invalid_current_defers");
    pass &= Check(Decide(true, true, true, true, false, false, false, 3.0f).decision ==
        GameplayAspectRestorationDecision::ConsumeNoWrite, "already_ultrawide_consumes");
    pass &= Check(Decide(true, true, true, false, false, true, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::Defer, "invalid_coherent_state_defers");
    pass &= Check(Decide(true, true, true, false, true, false, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::Defer, "invalid_restoration_aspect_defers");
    const auto restore = Decide(true, true, true, false, true, true, true, 3.0f);
    pass &= Check(restore.decision == GameplayAspectRestorationDecision::Restore &&
        restore.restorationAspect == 3.0f, "valid_state_restores_exact_target");
    const auto legacyDiffers = Decide(true, true, true, false, true, true, true, 2.33333f);
    pass &= Check(legacyDiffers.decision == GameplayAspectRestorationDecision::Restore &&
        legacyDiffers.restorationAspect == 2.33333f,
        "coherent_state_has_no_legacy_fallback");
    pass &= Check(Decide(true, true, true, false, true, true, true, 3.0f).decision ==
        GameplayAspectRestorationDecision::Restore, "current_flags_are_consumer_local");

    // Regression sequence for fullscreen/resolution recovery: an unavailable
    // coherent target defers, the first newly established arbitrary aspect
    // supplies the target, and a completed request has no further action.
    pass &= Check(Decide(true, true, true, false, true, false, false, 3.0f).decision ==
        GameplayAspectRestorationDecision::Defer,
        "recovery_defers_without_established_target");
    const auto established = store.Update(2.4f, { 0x6000, true });
    pass &= Check(established.state.valid && established.state.aspect == 2.4f,
        "recovery_establishes_arbitrary_target");
    const auto retry = Decide(true, true, true, false, true, true, true, 2.4f);
    pass &= Check(retry.decision == GameplayAspectRestorationDecision::Restore &&
        retry.restorationAspect == 2.4f,
        "recovery_retries_with_new_target");
    pass &= Check(Decide(false, true, true, false, true, true, true, 2.4f).decision ==
        GameplayAspectRestorationDecision::NoAction,
        "completed_recovery_does_not_reapply");
    pass &= Check(Decide(true, true, false, false, true, true, true, 2.4f).decision ==
        GameplayAspectRestorationDecision::Defer,
        "invalid_aspect_does_not_trigger_retry");

    std::cout << "Gameplay aspect restoration harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
