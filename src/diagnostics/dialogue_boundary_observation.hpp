#pragma once

#include "dialogue_source_provenance.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <chrono>

namespace diagnostics
{
    struct DialogueBoundaryObservation
    {
        std::uint64_t callbackSequence{};
        std::uint64_t cinematicGeneration{};
        std::uintptr_t callbackSource{};
        float callbackFov{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t coordinator{};
        std::uint8_t phaseBefore{};
        std::uint8_t action{};
        bool postCinematicExclusionActive{};
        DialogueWriterSnapshot latestWriter{};
    };

    enum class RawAddressRelation : std::uint8_t
    {
        Unavailable,
        Equal,
        Different,
    };

    inline RawAddressRelation CompareRawAddresses(std::uintptr_t callbackSource,
        std::uintptr_t writerSource, bool writerValid) noexcept
    {
        if (!writerValid || callbackSource == 0 || writerSource == 0)
            return RawAddressRelation::Unavailable;
        return callbackSource == writerSource
            ? RawAddressRelation::Equal : RawAddressRelation::Different;
    }

    // This is a bounded, diagnostic-only sampler. Address equality and nearby
    // FOV values remain observations; neither establishes engine object identity.
    class DialogueBoundaryObservationGate
    {
    public:
        static constexpr std::uint32_t RecordLimit = 128;
        static constexpr float FovChangeThreshold = 0.5f;
        using Clock = std::chrono::steady_clock;

        void Arm(std::uint64_t cinematicGeneration, Clock::time_point now = Clock::now())
        {
            std::lock_guard lock(mutex_);
            active_ = true;
            deadline_ = now + std::chrono::seconds(20);
            generation_ = cinematicGeneration;
            emitted_ = 0;
            limitNoticeTaken_ = false;
            previousValid_ = false;
            previous_ = {};
        }

        void Disarm()
        {
            std::lock_guard lock(mutex_);
            active_ = false;
            previousValid_ = false;
        }

        std::optional<DialogueBoundaryObservation> Observe(bool diagnosticsEnabled,
            const DialogueBoundaryObservation& observation)
        {
            std::lock_guard lock(mutex_);
            if (!diagnosticsEnabled || !active_ || Clock::now() >= deadline_ ||
                observation.cinematicGeneration != generation_ ||
                emitted_ >= RecordLimit)
                return std::nullopt;

            const bool changed = !previousValid_ ||
                observation.callbackSource != previous_.callbackSource ||
                observation.coordinator != previous_.coordinator ||
                observation.phaseBefore != previous_.phaseBefore ||
                observation.action != previous_.action ||
                observation.postCinematicExclusionActive !=
                    previous_.postCinematicExclusionActive ||
                observation.latestWriter.identityEpoch !=
                    previous_.latestWriter.identityEpoch ||
                observation.latestWriter.rsi != previous_.latestWriter.rsi ||
                observation.latestWriter.rbx != previous_.latestWriter.rbx ||
                FovChanged(observation.callbackFov, previous_.callbackFov) ||
                FovChanged(observation.latestWriter.rawFov,
                    previous_.latestWriter.rawFov);
            if (!changed) return std::nullopt;
            previous_ = observation; // Accumulate changes since last emitted sample.
            previousValid_ = true;
            ++emitted_;
            return observation;
        }

        bool TakeLimitNotice()
        {
            std::lock_guard lock(mutex_);
            if (!active_ || emitted_ < RecordLimit || limitNoticeTaken_) return false;
            limitNoticeTaken_ = true;
            return true;
        }

        std::uint32_t Emitted() const
        {
            std::lock_guard lock(mutex_);
            return emitted_;
        }

    private:
        static bool FovChanged(float current, float previous) noexcept
        {
            if (std::isfinite(current) != std::isfinite(previous)) return true;
            return std::isfinite(current) &&
                std::fabs(current - previous) >= FovChangeThreshold;
        }

        mutable std::mutex mutex_;
        bool active_{};
        bool limitNoticeTaken_{};
        bool previousValid_{};
        std::uint64_t generation_{};
        std::uint32_t emitted_{};
        DialogueBoundaryObservation previous_{};
        Clock::time_point deadline_{};
    };
}
