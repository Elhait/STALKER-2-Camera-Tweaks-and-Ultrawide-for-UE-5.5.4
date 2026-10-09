#include "../../src/dialogue/dialogue_state.hpp"
#include "../../src/camera/fov_semantics.hpp"

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
}

int main()
{
    constexpr float nativeTarget = 90.0f;
    constexpr std::uintptr_t source = 0x1000;
    bool pass = true;

    dialogue::RecoveryRearm recovery;
    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(recovery.IsPending(), "recovery_tail_enters_pending");
    pass &= Check(recovery.Observe(source, 89.7f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "moving_tail_does_not_rearm");
    pass &= Check(recovery.IsPending(), "moving_tail_remains_pending");
    pass &= Check(recovery.Observe(source, 89.995f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Complete,
        "first_target_sample_rearms");
    pass &= Check(!recovery.IsPending(), "rearm_clears_pending_state");

    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(recovery.Observe(source, 89.974f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "sample_more_than_one_degree_from_target_does_not_rearm");
    pass &= Check(recovery.IsPending(), "tail_leaving_tolerance_remains_pending");
    pass &= Check(recovery.Observe(source, 89.985f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "returning_tail_still_requires_exact_endpoint");
    pass &= Check(recovery.Observe(source, 89.999f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Complete,
        "returning_tail_first_target_sample");

    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(recovery.Observe(source, 89.9736f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "next_dialogue_not_rearmed_immediately");
    pass &= Check(recovery.Observe(source, 89.9949f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Complete,
        "target_sample_without_followup_rearms");
    pass &= Check(!recovery.IsPending(), "target_sample_without_followup_clears_pending");
    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(recovery.Observe(source, 89.9736f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "baseline_proximity_does_not_rearm");
    pass &= Check(recovery.Observe(source, 89.9949f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Complete,
        "native_target_completion_rearms_after_continuation");

    recovery.Begin(source, 89.4f, std::numeric_limits<float>::quiet_NaN());
    pass &= Check(recovery.Observe(source, 90.0f,
        std::numeric_limits<float>::quiet_NaN(), 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "invalid_target_fails_closed");
    pass &= Check(recovery.IsPending(), "invalid_target_keeps_pending");

    pass &= Check(recovery.Observe(0x2000, 90.0f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Cancel,
        "source_change_cancels_recovery");
    pass &= Check(!recovery.IsPending(), "source_change_clears_pending");

    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(camera::AreFovValuesSemanticallyEquivalent(90.0f, 90.6557f) &&
        recovery.Observe(source, 90.6557f, nativeTarget, 0.01f) ==
            dialogue::RecoveryRearm::Decision::Pending,
        "semantic_equivalence_does_not_substitute_for_recovery_endpoint_evidence");
    recovery.Begin(source, 89.4f, nativeTarget);
    pass &= Check(recovery.Observe(source, 91.01f, nativeTarget, 0.01f) ==
        dialogue::RecoveryRearm::Decision::Pending,
        "greater_than_one_degree_does_not_complete_recovery");

    std::cout << "dialogue_recovery_rearm=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
