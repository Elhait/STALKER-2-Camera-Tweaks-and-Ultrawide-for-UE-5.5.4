#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace diagnostics
{
    // The current native armer prologue is push RSI; push RDI; sub RSP,0x28.
    // The return slot at the mid-hook is therefore entry-RSP + 0x38.
    inline constexpr std::size_t kBlendArmerReturnSlotOffset = 0x38;

    enum class BlendCallerKind : std::uint8_t
    {
        Unreadable,
        Unknown,
        CinematicEnter,
        CinematicExit,
        ProviderProducer,
    };

    struct BlendCallerEvidence
    {
        std::uintptr_t returnAddress{};
        BlendCallerKind kind{BlendCallerKind::Unreadable};
        bool readable{};
    };

    // Only accept the exact relocated E8 return after validating that both
    // instructions call the same target. A nearby executable address is not evidence.
    inline std::uintptr_t ResolveRelocatedBlendCallReturn(std::uintptr_t original,
        const std::uint8_t* originalBytes, std::size_t originalSize,
        const std::uint8_t* relocatedBytes, std::size_t relocatedSize) noexcept
    {
        const auto relocated = reinterpret_cast<std::uintptr_t>(relocatedBytes);
        constexpr auto maximum = (std::numeric_limits<std::uintptr_t>::max)();
        if (!original || !originalBytes || !relocatedBytes || originalSize < 5 ||
            relocatedSize < 5 || original > maximum - 5 || relocated > maximum - 5 ||
            originalBytes[0] != 0xE8 || relocatedBytes[0] != 0xE8) return 0;
        std::int32_t originalDisplacement{}, relocatedDisplacement{};
        std::memcpy(&originalDisplacement, originalBytes + 1, 4);
        std::memcpy(&relocatedDisplacement, relocatedBytes + 1, 4);
        const auto target = [maximum](std::uintptr_t end, std::int32_t displacement) {
            const auto magnitude = displacement < 0
                ? static_cast<std::uint64_t>(-static_cast<std::int64_t>(displacement))
                : static_cast<std::uint64_t>(displacement);
            if ((displacement < 0 && end < magnitude) ||
                (displacement >= 0 && maximum - end < magnitude)) return std::uintptr_t{};
            return displacement < 0 ? end - magnitude : end + magnitude;
        };
        const auto originalTarget = target(original + 5, originalDisplacement);
        return originalTarget && originalTarget == target(relocated + 5, relocatedDisplacement)
            ? relocated + 5 : 0;
    }

    template<class Reader>
    BlendCallerEvidence ReadBlendCallerEvidence(std::uintptr_t nativeRsp,
        std::size_t returnSlotOffset, std::uintptr_t enterReturn,
        std::uintptr_t exitReturn, Reader&& read,
        std::uintptr_t relocatedEnterReturn = 0,
        std::uintptr_t relocatedExitReturn = 0,
        std::uintptr_t producerReturn = 0) noexcept
    {
        BlendCallerEvidence evidence{};
        if (!nativeRsp || nativeRsp >
                (std::numeric_limits<std::uintptr_t>::max)() - returnSlotOffset)
            return evidence;
        if (!read(nativeRsp + returnSlotOffset, evidence.returnAddress)) return evidence;
        evidence.readable = true;
        const bool enter = (enterReturn && evidence.returnAddress == enterReturn) ||
            (relocatedEnterReturn && evidence.returnAddress == relocatedEnterReturn);
        const bool exit = (exitReturn && evidence.returnAddress == exitReturn) ||
            (relocatedExitReturn && evidence.returnAddress == relocatedExitReturn);
        const bool producer = producerReturn && evidence.returnAddress == producerReturn;
        if (enter && !exit && !producer)
            evidence.kind = BlendCallerKind::CinematicEnter;
        else if (exit && !enter && !producer)
            evidence.kind = BlendCallerKind::CinematicExit;
        else if (producer && !enter && !exit)
            evidence.kind = BlendCallerKind::ProviderProducer;
        else
            evidence.kind = BlendCallerKind::Unknown;
        return evidence;
    }

    inline const char* BlendCallerKindName(BlendCallerKind kind) noexcept
    {
        switch (kind) {
        case BlendCallerKind::CinematicEnter: return "cinematic_enter_return";
        case BlendCallerKind::CinematicExit: return "cinematic_exit_return";
        case BlendCallerKind::ProviderProducer: return "provider_producer_return";
        case BlendCallerKind::Unknown: return "unmatched_return";
        default: return "unreadable_return";
        }
    }

    struct BlendStartCapture
    {
        std::uintptr_t component{}, receiver{}, vtable{};
        std::uint32_t thread{};
        float start{std::numeric_limits<float>::quiet_NaN()};
        float target{std::numeric_limits<float>::quiet_NaN()};
        float receiverField{std::numeric_limits<float>::quiet_NaN()};
        BlendCallerEvidence caller{};
        std::uint64_t gameplaySettingsEpoch{}, cinematicSettingsEpoch{};
        float shadowAspect{NAN};
    };

    struct BlendActivationCandidate
    {
        std::uint64_t epoch{}, generation{};
        BlendStartCapture capture{};
        bool tickObserved{};
    };

    enum class BlendActivationMatchStatus : std::uint8_t
    {
        NoCandidate,
        Matched,
        FieldsChanged,
    };

    inline const char* BlendActivationMatchStatusName(
        BlendActivationMatchStatus status) noexcept
    {
        switch (status) {
        case BlendActivationMatchStatus::Matched: return "matched_tick";
        case BlendActivationMatchStatus::FieldsChanged: return "fields_changed_retired";
        default: return "no_candidate";
        }
    }

    struct BlendActivationMatch
    {
        BlendActivationMatchStatus status{BlendActivationMatchStatus::NoCandidate};
        BlendActivationCandidate candidate{};
    };

    // Fixed-capacity, callback-serialized observation state. It records
    // candidate epochs only; a matching later tick is the evidence that the
    // captured start/target reached the active tick path.
    class PostExitBlendActivationObservation
    {
    public:
        static constexpr std::size_t Capacity = 8;

        void Begin(std::uint64_t generation) noexcept
        {
            slots_ = {};
            generation_ = generation;
            active_ = generation != 0;
            overflow_ = 0;
            captures_ = matchedTicks_ = invalidated_ = 0;
        }

        void Retire() noexcept
        {
            slots_ = {};
            generation_ = 0;
            active_ = false;
        }

        bool Active() const noexcept { return active_; }
        std::uint64_t Generation() const noexcept { return generation_; }
        std::uint64_t Overflow() const noexcept { return overflow_; }
        std::uint64_t Captures() const noexcept { return captures_; }
        std::uint64_t MatchedTicks() const noexcept { return matchedTicks_; }
        std::uint64_t Invalidated() const noexcept { return invalidated_; }

        bool IsMatchedEpoch(std::uint64_t epoch, std::uint64_t generation,
            std::uintptr_t component, std::uintptr_t receiver) const noexcept
        {
            if (!active_ || !epoch || generation != generation_) return false;
            for (const auto& candidate : slots_)
                if (candidate.epoch == epoch && candidate.generation == generation &&
                    candidate.tickObserved && candidate.capture.component == component &&
                    candidate.capture.receiver == receiver)
                    return true;
            return false;
        }

        const BlendActivationCandidate* ObserveStartCapture(
            const BlendStartCapture& capture) noexcept
        {
            if (!active_ || !capture.component || !capture.receiver ||
                !std::isfinite(capture.start) || !std::isfinite(capture.target))
                return nullptr;

            BlendActivationCandidate* slot{};
            for (auto& candidate : slots_)
                if (candidate.epoch && candidate.capture.component == capture.component) {
                    slot = &candidate;
                    break;
                }
            if (!slot)
                for (auto& candidate : slots_)
                    if (!candidate.epoch) { slot = &candidate; break; }
            if (!slot || nextEpoch_ == (std::numeric_limits<std::uint64_t>::max)()) {
                ++overflow_;
                return nullptr;
            }

            *slot = {++nextEpoch_, generation_, capture};
            ++captures_;
            return slot;
        }

        BlendActivationMatch MatchTick(std::uintptr_t component,
            std::uintptr_t receiver, float start, float target) noexcept
        {
            if (!active_ || !component || !receiver) return {};
            for (auto& candidate : slots_) {
                if (!candidate.epoch || candidate.capture.component != component) continue;
                const bool sameValues = std::isfinite(start) && std::isfinite(target) &&
                    std::memcmp(&candidate.capture.start, &start, sizeof(float)) == 0 &&
                    std::memcmp(&candidate.capture.target, &target, sizeof(float)) == 0;
                if (!sameValues || candidate.capture.receiver != receiver ||
                    candidate.generation != generation_) {
                    candidate = {};
                    ++invalidated_;
                    return {BlendActivationMatchStatus::FieldsChanged, {}};
                }
                candidate.tickObserved = true;
                ++matchedTicks_;
                return {BlendActivationMatchStatus::Matched, candidate};
            }
            return {};
        }

    private:
        std::array<BlendActivationCandidate, Capacity> slots_{};
        std::uint64_t generation_{}, nextEpoch_{}, overflow_{};
        std::uint64_t captures_{}, matchedTicks_{}, invalidated_{};
        bool active_{};
    };
}
