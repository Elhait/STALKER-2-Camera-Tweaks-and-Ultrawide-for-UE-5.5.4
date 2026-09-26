#include "../../src/plugin/feature_status.hpp"

#include <cstdio>
#include <string_view>

namespace
{
    bool IsNamed(plugin::FeatureStatus status, std::string_view expected)
    {
        return std::string_view(plugin::FeatureStatusName(status)) == expected;
    }

    bool TestDisabledIsDistinct()
    {
        return IsNamed(plugin::FeatureStatus::Disabled, "DISABLED") &&
            !IsNamed(plugin::FeatureStatus::Disabled, "FAILED");
    }

    bool TestFeatureFailureDoesNotInvalidatePeers()
    {
        auto gameplay = plugin::FeatureStatus::Available;
        const auto cinematics = plugin::FeatureStatus::Available;
        const auto dialogue = plugin::FeatureStatus::Available;
        gameplay = plugin::FeatureStatus::Failed;
        return IsNamed(gameplay, "FAILED") &&
            IsNamed(cinematics, "AVAILABLE") &&
            IsNamed(dialogue, "AVAILABLE");
    }

    bool TestDisabledConfigurationRemainsValid()
    {
        const auto gameplay = plugin::FeatureStatus::Disabled;
        const auto cinematics = plugin::FeatureStatus::Available;
        const auto dialogue = plugin::FeatureStatus::Available;
        return IsNamed(gameplay, "DISABLED") &&
            IsNamed(cinematics, "AVAILABLE") &&
            IsNamed(dialogue, "AVAILABLE");
    }

    bool TestDisabledGameplayCanRetainRecoveryObservation()
    {
        const auto gameplayCorrection = plugin::FeatureStatus::Disabled;
        const bool gameplayWriterObserverInstalled = true;
        return IsNamed(gameplayCorrection, "DISABLED") &&
            plugin::DialogueLifecycleCapabilityAvailable(
                true, true, gameplayWriterObserverInstalled);
    }

    bool TestDialogueLifecycleCapabilityMatrix()
    {
        return plugin::DialogueLifecycleCapabilityAvailable(true, true, true) &&
            plugin::DialogueLifecycleCapabilityAvailable(false, false, false) &&
            plugin::DialogueLifecycleCapabilityAvailable(false, false, true) &&
            plugin::DialogueLifecycleCapabilityAvailable(false, true, false) &&
            plugin::DialogueLifecycleCapabilityAvailable(false, true, true) &&
            !plugin::DialogueLifecycleCapabilityAvailable(true, false, true) &&
            !plugin::DialogueLifecycleCapabilityAvailable(true, false, false) &&
            !plugin::DialogueLifecycleCapabilityAvailable(true, true, false);
    }
}

int main()
{
    const bool disabledDistinct = TestDisabledIsDistinct();
    const bool gracefulFailure = TestFeatureFailureDoesNotInvalidatePeers();
    const bool disabledConfiguration = TestDisabledConfigurationRemainsValid();
    const bool disabledGameplayObservation = TestDisabledGameplayCanRetainRecoveryObservation();
    const bool dialogueCapability = TestDialogueLifecycleCapabilityMatrix();
    std::printf("disabled_distinct=%s graceful_failure=%s disabled_configuration=%s disabled_gameplay_observation=%s dialogue_capability=%s\n",
        disabledDistinct ? "PASS" : "FAIL", gracefulFailure ? "PASS" : "FAIL",
        disabledConfiguration ? "PASS" : "FAIL",
        disabledGameplayObservation ? "PASS" : "FAIL",
        dialogueCapability ? "PASS" : "FAIL");
    return disabledDistinct && gracefulFailure && disabledConfiguration &&
        disabledGameplayObservation && dialogueCapability ? 0 : 1;
}
