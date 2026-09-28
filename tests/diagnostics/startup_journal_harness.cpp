#include "../../src/diagnostics/startup_journal.hpp"

#include <cstdio>

int main()
{
    using diagnostics::startup_journal::RecordSuccessfulPendingPresent;
    using diagnostics::startup_journal::TrackPendingTarget;

    constexpr std::uintptr_t firstTarget = 0x1234;
    constexpr std::uintptr_t otherTarget = 0x5678;
    TrackPendingTarget(firstTarget);
    const bool mismatchedTargetIgnored = RecordSuccessfulPendingPresent(otherTarget) == 0;
    const bool firstSuccessRecorded = RecordSuccessfulPendingPresent(firstTarget) == 1;
    const bool secondSuccessRecorded = RecordSuccessfulPendingPresent(firstTarget) == 2;
    const bool trackingStopsAfterThreshold = RecordSuccessfulPendingPresent(firstTarget) == 0;

    constexpr std::uintptr_t replacementTarget = 0x9abc;
    TrackPendingTarget(replacementTarget);
    const bool replacementTargetStartsFresh =
        RecordSuccessfulPendingPresent(replacementTarget) == 1;

    const bool passed = mismatchedTargetIgnored && firstSuccessRecorded &&
        secondSuccessRecorded && trackingStopsAfterThreshold && replacementTargetStartsFresh;
    std::printf("pending_target_mismatch_ignored=%s\n",
        mismatchedTargetIgnored ? "PASS" : "FAIL");
    std::printf("two_successful_presents_recorded=%s\n",
        firstSuccessRecorded && secondSuccessRecorded ? "PASS" : "FAIL");
    std::printf("tracking_stops_after_threshold=%s\n",
        trackingStopsAfterThreshold ? "PASS" : "FAIL");
    std::printf("replacement_target_resets_evidence=%s\n",
        replacementTargetStartsFresh ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}
