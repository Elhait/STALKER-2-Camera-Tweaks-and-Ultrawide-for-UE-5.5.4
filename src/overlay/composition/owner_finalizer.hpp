#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

namespace overlay
{
    enum class OwnerFinalizationOutcome : std::uint8_t
    {
        Pending,
        Running,
        Complete,
        Retained,
        Unverified,
    };

    // Owner-thread-only one-shot teardown. Every step is attempted even when
    // an independent step fails; Complete requires both successful steps and
    // an affirmative final ownership check.
    class OwnerFinalizer
    {
    public:
        template<typename Verify, typename... Steps>
        OwnerFinalizationOutcome Run(Verify&& verify,
            Steps&&... steps) noexcept
        {
            auto expected = OwnerFinalizationOutcome::Pending;
            if (!outcome_.compare_exchange_strong(expected,
                    OwnerFinalizationOutcome::Running,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
                return expected;

            auto finalOutcome = OwnerFinalizationOutcome::Complete;
            (RunStep(finalOutcome, std::forward<Steps>(steps)), ...);
            RunStep(finalOutcome, std::forward<Verify>(verify));
            outcome_.store(finalOutcome, std::memory_order_release);
            return finalOutcome;
        }

        OwnerFinalizationOutcome outcome() const noexcept
        {
            return outcome_.load(std::memory_order_acquire);
        }

    private:
        template<typename Function>
        static void RunStep(OwnerFinalizationOutcome& aggregate,
            Function&& function) noexcept
        {
            try {
                const auto result = std::forward<Function>(function)();
                if (result == OwnerFinalizationOutcome::Retained)
                    aggregate = OwnerFinalizationOutcome::Retained;
                else if (result != OwnerFinalizationOutcome::Complete &&
                    aggregate == OwnerFinalizationOutcome::Complete)
                    aggregate = OwnerFinalizationOutcome::Unverified;
            } catch (...) {
                if (aggregate == OwnerFinalizationOutcome::Complete)
                    aggregate = OwnerFinalizationOutcome::Unverified;
            }
        }

        std::atomic<OwnerFinalizationOutcome> outcome_{
            OwnerFinalizationOutcome::Pending};
    };
}
