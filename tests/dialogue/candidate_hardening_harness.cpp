#include "../../src/dialogue/dialogue_state.hpp"

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
    constexpr float epsilon = 0.01f;
    bool pass = true;

    dialogue::CandidateTracker candidate;
    candidate.Begin(90.0f);
    pass &= Check(candidate.Observe(89.991f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Pending, "initial_jitter_pending");
    pass &= Check(candidate.Observe(89.97f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Activate, "slow_descent_activates");
    pass &= Check(candidate.Baseline() == 90.0f, "baseline_preserved");

    candidate.Reset();
    candidate.Begin(90.0f);
    pass &= Check(candidate.Observe(90.0f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Pending, "stabilization_first_sample");
    pass &= Check(candidate.Observe(90.0f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Pending, "stabilization_second_sample");
    pass &= Check(candidate.Observe(90.0f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Cancel, "stabilization_third_sample_cancels");
    pass &= Check(!candidate.IsPending(), "stabilization_clears_candidate");

    candidate.Begin(90.0f);
    pass &= Check(candidate.Observe(89.995f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Pending, "small_step_pending");
    pass &= Check(candidate.Observe(90.02f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Cancel, "clear_reversal_cancels");

    candidate.Begin(90.0f);
    pass &= Check(candidate.Observe(89.995f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Pending, "slow_descent_start");
    pass &= Check(candidate.Observe(89.990f, epsilon, epsilon) ==
        dialogue::CandidateDecision::Activate, "slow_multi_sample_descent");

    candidate.Begin(90.0f);
    pass &= Check(candidate.Observe(std::numeric_limits<float>::quiet_NaN(),
        epsilon, epsilon) == dialogue::CandidateDecision::Cancel,
        "invalid_sample_cancels");
    pass &= Check(!candidate.IsPending(), "invalid_sample_clears_candidate");

    candidate.Begin(90.0f, 0x2000, 70.0f);
    pass &= Check(candidate.HasContext(), "context_is_captured");
    pass &= Check(candidate.Observe(89.97f, 0x2000, 70.6557f,
        epsilon, epsilon) == dialogue::CandidateDecision::Activate,
        "subdegree_native_target_variance_preserves_same_context");

    candidate.Begin(90.0f, 0x2000, 70.0f);
    pass &= Check(candidate.Observe(89.97f, 0x3000, 70.0f,
        epsilon, epsilon) == dialogue::CandidateDecision::Cancel,
        "source_change_cancels");
    pass &= Check(!candidate.IsPending(), "source_change_clears_candidate");

    candidate.Begin(90.0f, 0x2000, 70.0f);
    pass &= Check(candidate.Observe(89.97f, 0x2000, 80.0f,
        epsilon, epsilon) == dialogue::CandidateDecision::Cancel,
        "target_change_cancels");

    candidate.Begin(90.0f, 0, 70.0f);
    pass &= Check(!candidate.HasContext(), "zero_source_fails_closed");
    pass &= Check(candidate.Observe(89.97f, 0, 70.0f,
        epsilon, epsilon) == dialogue::CandidateDecision::Cancel,
        "zero_source_does_not_activate");

    candidate.Begin(90.0f, 0x2000, std::numeric_limits<float>::quiet_NaN());
    pass &= Check(!candidate.HasContext(), "invalid_target_fails_closed");

    pass &= Check(dialogue::CandidatePromotionAllowed(
        config::DialogueZoomPolicy::Native, true, true),
        "correction_policy_does_not_gate_shared_dialogue_observation");
    pass &= Check(!dialogue::CandidatePromotionAllowed(
        config::DialogueZoomPolicy::Adaptive, false, true),
        "non_gameplay_blocks_promotion");
    pass &= Check(!dialogue::CandidatePromotionAllowed(
        config::DialogueZoomPolicy::Adaptive, true, false),
        "recovery_exclusion_blocks_promotion");
    pass &= Check(dialogue::CandidatePromotionAllowed(
        config::DialogueZoomPolicy::Adaptive, true, true) &&
        dialogue::CandidatePromotionAllowed(
            config::DialogueZoomPolicy::Reduced, true, true),
        "dialogue_observation_is_independent_of_active_mode");

    dialogue::RecoveryRearm recovery;
    recovery.Begin(0x1000, 89.5f, 90.0f);
    pass &= Check(recovery.IsPending(), "task4_rearm_gate_remains_authoritative");

    std::cout << "dialogue_candidate_hardening=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
