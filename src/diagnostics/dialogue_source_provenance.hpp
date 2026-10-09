#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

namespace diagnostics
{
    enum class DialogueSourceStage : std::uint8_t
    {
        Candidate,
        Active,
        Exiting,
        Recovery,
        End,
    };

    struct DialogueWriterSnapshot
    {
        bool valid{};
        std::uint64_t sequence{};
        std::uint64_t identityEpoch{};
        std::uintptr_t rsi{};
        std::uintptr_t rbx{};
    };

    struct DialogueSourceEvent
    {
        const char* marker{};
        const char* owner{};
        const char* reason{};
        DialogueSourceStage stage{DialogueSourceStage::Active};
        std::uint64_t generation{};
        bool lifecycleSourceKnown{};
        std::uintptr_t lifecycleSource{};
        std::uintptr_t callbackSource{};
        bool pointerMismatchKnown{};
        bool pointerMismatch{};
        DialogueWriterSnapshot latestWriter{};
    };

    // Observation-only state. Production creates this object lazily, and only
    // while Diagnostic Mode is enabled. Pointer comparisons describe observed
    // register values only; they do not assert a wider engine identity.
    class DialogueSourceProvenanceTrace
    {
    public:
        std::optional<DialogueSourceEvent> Accepted(bool diagnosticEnabled,
            std::uintptr_t candidateSource, const char* owner,
            DialogueWriterSnapshot latestWriter);
        std::optional<DialogueSourceEvent> Callback(bool diagnosticEnabled,
            DialogueSourceStage stage, std::uintptr_t callbackSource,
            const char* owner, DialogueWriterSnapshot latestWriter);
        std::optional<DialogueSourceEvent> RecoveryStarted(bool diagnosticEnabled,
            std::uintptr_t recoverySource, const char* owner,
            DialogueWriterSnapshot latestWriter);
        std::optional<DialogueSourceEvent> Ended(bool diagnosticEnabled,
            const char* reason, const char* owner,
            DialogueWriterSnapshot latestWriter);

    private:
        std::mutex mutex_;
        std::uint64_t nextGeneration_{};
        bool active_{};
        std::uintptr_t lifecycleSource_{};
        std::uintptr_t lastCallbackSource_{};
        DialogueSourceStage lastCallbackStage_{DialogueSourceStage::Active};
        bool callbackKeyValid_{};
        DialogueSourceStage lastEmittedStage_{DialogueSourceStage::Active};
        std::uintptr_t lastEmittedCallbackSource_{};
        const char* lastEmittedOwner_{};
        bool lastEmittedWriterValid_{};
        std::uint64_t lastEmittedWriterEpoch_{};
        std::uintptr_t lastEmittedWriterRsi_{};
        std::uintptr_t lastEmittedWriterRbx_{};
    };
}
