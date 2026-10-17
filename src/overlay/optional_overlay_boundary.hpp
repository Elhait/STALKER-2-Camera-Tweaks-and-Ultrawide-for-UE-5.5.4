#pragma once

#include <string>
#include <utility>

namespace overlay
{
    enum class OptionalOverlayWorkResult
    {
        Completed,
        Failed,
    };

    template <typename Logger>
    bool TryOptionalOverlayLog(Logger&& logger,
        const std::string& message) noexcept
    {
        try {
            std::forward<Logger>(logger)(message);
            return true;
        } catch (...) {
            return false;
        }
    }

    template <typename Logger>
    bool TryOptionalOverlayLog(Logger&& logger, const char* message) noexcept
    {
        try {
            const std::string ownedMessage = message ? message : "";
            return TryOptionalOverlayLog(
                std::forward<Logger>(logger), ownedMessage);
        } catch (...) {
            return false;
        }
    }

    // Isolates C++ exceptions from optional overlay-owned work only. Native
    // callbacks must remain outside this scope and run exactly as before.
    // With the production /EHsc model this does not catch access violations.
    template <typename Work, typename FailureTransition>
    OptionalOverlayWorkResult RunOptionalOverlayWork(
        Work&& work, FailureTransition&& transitionToFailure) noexcept
    {
        try {
            std::forward<Work>(work)();
            return OptionalOverlayWorkResult::Completed;
        } catch (...) {
            try {
                std::forward<FailureTransition>(transitionToFailure)();
            } catch (...) {
                // A failure transition is best-effort and cannot escape the
                // foreign callback boundary either.
            }
            return OptionalOverlayWorkResult::Failed;
        }
    }
}
