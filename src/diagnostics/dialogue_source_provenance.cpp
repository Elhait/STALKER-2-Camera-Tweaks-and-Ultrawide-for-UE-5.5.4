#include "dialogue_source_provenance.hpp"

namespace diagnostics
{
    namespace
    {
        DialogueSourceEvent MakeEvent(const char* marker, const char* owner,
            const char* reason, DialogueSourceStage stage, std::uint64_t generation,
            std::uintptr_t lifecycleSource, std::uintptr_t callbackSource,
            DialogueWriterSnapshot latestWriter)
        {
            const bool sourceKnown = lifecycleSource != 0;
            return { marker, owner, reason, stage, generation, sourceKnown,
                lifecycleSource, callbackSource, sourceKnown,
                sourceKnown && callbackSource != lifecycleSource, latestWriter };
        }
    }

    std::optional<DialogueSourceEvent> DialogueSourceProvenanceTrace::Accepted(
        bool diagnosticEnabled, std::uintptr_t candidateSource, const char* owner,
        DialogueWriterSnapshot latestWriter)
    {
        if (!diagnosticEnabled) return std::nullopt;
        std::lock_guard lock(mutex_);
        active_ = true;
        ++nextGeneration_;
        lifecycleSource_ = candidateSource;
        lastCallbackSource_ = candidateSource;
        lastCallbackStage_ = DialogueSourceStage::Active;
        callbackKeyValid_ = false;
        return MakeEvent("DIALOGUE_SOURCE_ACCEPTED", owner, "candidate-accepted",
            DialogueSourceStage::Active, nextGeneration_, lifecycleSource_,
            candidateSource, latestWriter);
    }

    std::optional<DialogueSourceEvent> DialogueSourceProvenanceTrace::Callback(
        bool diagnosticEnabled, DialogueSourceStage stage,
        std::uintptr_t callbackSource, const char* owner,
        DialogueWriterSnapshot latestWriter)
    {
        if (!diagnosticEnabled) return std::nullopt;
        std::lock_guard lock(mutex_);
        const bool writerChanged = !callbackKeyValid_ ||
            latestWriter.valid != lastEmittedWriterValid_ ||
            latestWriter.identityEpoch != lastEmittedWriterEpoch_ ||
            latestWriter.rsi != lastEmittedWriterRsi_ ||
            latestWriter.rbx != lastEmittedWriterRbx_;
        const bool changed = !callbackKeyValid_ || stage != lastEmittedStage_ ||
            callbackSource != lastEmittedCallbackSource_ || owner != lastEmittedOwner_ ||
            writerChanged;
        lastCallbackSource_ = callbackSource;
        lastCallbackStage_ = stage;
        if (!changed) return std::nullopt;

        callbackKeyValid_ = true;
        lastEmittedStage_ = stage;
        lastEmittedCallbackSource_ = callbackSource;
        lastEmittedOwner_ = owner;
        lastEmittedWriterValid_ = latestWriter.valid;
        lastEmittedWriterEpoch_ = latestWriter.identityEpoch;
        lastEmittedWriterRsi_ = latestWriter.rsi;
        lastEmittedWriterRbx_ = latestWriter.rbx;
        const auto marker = active_ && lifecycleSource_ != 0 &&
            callbackSource != lifecycleSource_
            ? "DIALOGUE_SOURCE_MISMATCH" : "DIALOGUE_SOURCE_CALLBACK";
        return MakeEvent(marker, owner, active_ ? "dialogue-callback" :
            "callback-without-diagnostic-accepted-event", stage,
            active_ ? nextGeneration_ : 0, lifecycleSource_, callbackSource,
            latestWriter);
    }

    std::optional<DialogueSourceEvent> DialogueSourceProvenanceTrace::RecoveryStarted(
        bool diagnosticEnabled, std::uintptr_t recoverySource, const char* owner,
        DialogueWriterSnapshot latestWriter)
    {
        if (!diagnosticEnabled) return std::nullopt;
        std::lock_guard lock(mutex_);
        return MakeEvent("DIALOGUE_RECOVERY_SOURCE", owner, "recovery-rearm-begin",
            DialogueSourceStage::Recovery, active_ ? nextGeneration_ : 0,
            lifecycleSource_, recoverySource, latestWriter);
    }

    std::optional<DialogueSourceEvent> DialogueSourceProvenanceTrace::Ended(
        bool diagnosticEnabled, const char* reason, const char* owner,
        DialogueWriterSnapshot latestWriter)
    {
        if (!diagnosticEnabled) return std::nullopt;
        std::lock_guard lock(mutex_);
        if (!active_) return std::nullopt;
        auto event = MakeEvent("DIALOGUE_SOURCE_END", owner, reason,
            DialogueSourceStage::End, nextGeneration_, lifecycleSource_,
            lastCallbackSource_, latestWriter);
        active_ = false;
        lifecycleSource_ = 0;
        lastCallbackSource_ = 0;
        callbackKeyValid_ = false;
        return event;
    }
}
