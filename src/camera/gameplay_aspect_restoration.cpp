#include "gameplay_aspect_restoration.hpp"
#include "../diagnostics/performance_telemetry.hpp"

namespace camera
{
    const char* GameplayAspectRestorationDispositionName(
        GameplayAspectRestorationDisposition value) noexcept
    {
        switch (value) {
        case GameplayAspectRestorationDisposition::Updated: return "UPDATE";
        case GameplayAspectRestorationDisposition::Retained: return "RETAIN";
        case GameplayAspectRestorationDisposition::Invalidated: return "INVALIDATE";
        }
        return "RETAIN";
    }

    const char* GameplayAspectRestorationDecisionName(
        GameplayAspectRestorationDecision value) noexcept
    {
        switch (value) {
        case GameplayAspectRestorationDecision::NoAction: return "NO_ACTION";
        case GameplayAspectRestorationDecision::Defer: return "DEFER";
        case GameplayAspectRestorationDecision::ConsumeNoWrite: return "CONSUME_NO_WRITE";
        case GameplayAspectRestorationDecision::Restore: return "RESTORE";
        }
        return "NO_ACTION";
    }

    GameplayAspectRestorationDecisionResult ResolveGameplayAspectRestorationDecision(
        const GameplayAspectRestorationDecisionInput& input,
        float restorationAspect) noexcept
    {
        GameplayAspectRestorationDecisionResult result{};
        result.restorationAspect = restorationAspect;
        if (!input.pending || !input.targetIsHorPlus || !input.gameplayEnabled ||
            !input.coordinatorIsGameplay)
            return result;
        if (!input.currentAspectReadable || !input.currentAspectValid)
            return { GameplayAspectRestorationDecision::Defer, restorationAspect };
        if (input.currentAspectUltrawide)
            return { GameplayAspectRestorationDecision::ConsumeNoWrite, restorationAspect };
        if (!input.restorationValid || !input.restorationAspectValid ||
            !input.restorationAspectUltrawide)
            return { GameplayAspectRestorationDecision::Defer, restorationAspect };
        return { GameplayAspectRestorationDecision::Restore, restorationAspect };
    }

    GameplayAspectRestorationResult GameplayAspectRestorationStore::Update(
        float aspect, FovWriterSourceToken source)
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::AspectRestoration);
        const bool changed = !state_.valid || state_.aspect != aspect ||
            state_.source.value != source.value || state_.source.valid != source.valid;
        state_.aspect = aspect;
        state_.source = source;
        state_.observationSequence = ++nextSequence_;
        state_.valid = true;
        return { GameplayAspectRestorationDisposition::Updated, state_, changed };
    }

    GameplayAspectRestorationResult GameplayAspectRestorationStore::Retain()
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::AspectRestoration);
        return { GameplayAspectRestorationDisposition::Retained, state_, false };
    }

    GameplayAspectRestorationResult GameplayAspectRestorationStore::Invalidate()
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::AspectRestoration);
        const bool changed = state_.valid || state_.source.valid ||
            state_.observationSequence != 0;
        state_ = {};
        return { GameplayAspectRestorationDisposition::Invalidated, state_, changed };
    }

    GameplayAspectRestorationState GameplayAspectRestorationStore::Read() const
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::AspectRestoration);
        return state_;
    }
}
