#include "../../src/dialogue/dialogue_state.hpp"

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
    dialogue::PostCinematicRecoveryExclusion exclusion;
    bool pass = true;

    exclusion.Arm(90.0f);
    pass &= Check(exclusion.IsActive(), "exit_arms_exclusion");
    pass &= Check(exclusion.ObserveValidated(0x1000, 100.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "descending_sample_suppressed");
    pass &= Check(exclusion.IsActive(), "recovery_remains_suppressed");
    pass &= Check(exclusion.Observe(0x1000, 106.0f, 0.01f), "legacy_dialogue_exclusion_contract");
    pass &= Check(exclusion.ObserveValidated(0x1000, 106.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "multiple_recovery_samples_suppressed");
    pass &= Check(exclusion.IsActive(), "multiple_samples_remain_active");

    pass &= Check(exclusion.ObserveValidated(0x1000, 90.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Recovered,
        "convergence_releases");
    pass &= Check(!exclusion.IsActive(), "convergence_inactive");

    exclusion.Arm(90.0f);
    pass &= Check(exclusion.ObserveValidated(0x1000, 90.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "neutral_first_sample_stays_suppressed");
    pass &= Check(exclusion.IsActive(), "neutral_first_sample_waits");
    pass &= Check(exclusion.ObserveValidated(0x1000, 89.8f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "later_recovery_start_is_suppressed");
    pass &= Check(exclusion.IsActive(), "recovery_stage_is_active");
    pass &= Check(exclusion.ObserveValidated(0x1000, 90.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Recovered,
        "later_convergence_releases");
    pass &= Check(!exclusion.IsActive(), "later_convergence_inactive");

    exclusion.Arm(90.0f);
    pass &= Check(exclusion.ObserveValidated(0x1000, 100.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "source_replacement_setup");
    pass &= Check(exclusion.ObserveValidated(0x2000, 100.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled,
        "source_replacement_cancels");
    pass &= Check(!exclusion.IsActive(), "source_replacement_inactive");

    exclusion.Arm(90.0f);
    pass &= Check(exclusion.ObserveValidated(0x1000,
        std::numeric_limits<float>::quiet_NaN(), 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled,
        "invalid_sample_cancels");
    pass &= Check(!exclusion.IsActive(), "invalid_sample_inactive");

    exclusion.Arm(90.0f);
    pass &= Check(exclusion.ObserveValidated(0x1000, 100.0f, 0.01f) ==
        dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed,
        "rearm_after_cancel");
    pass &= Check(exclusion.Reset(), "explicit_reset");
    pass &= Check(!exclusion.IsActive(), "explicit_reset_inactive");

    std::cout << "post_cinematic_exclusion=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
