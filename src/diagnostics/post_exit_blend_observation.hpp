#pragma once

#include <array>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "post_exit_blend_activation_observation.hpp"
#include "u2_shadow.hpp"

namespace diagnostics
{
    struct PostExitBlendSample
    {
        std::uintptr_t context{}, component{}, receiver{}, vtable{};
        std::uintptr_t callerReturn{};
        std::uint64_t activationEpoch{};
        std::uint32_t thread{}, activationThread{}, readable{};
        BlendCallerKind callerKind{BlendCallerKind::Unreadable};
        BlendActivationMatchStatus activationMatch{BlendActivationMatchStatus::NoCandidate};
        float raw{NAN}, argument{NAN}, finalArgument{NAN};
        float progressXmm7{NAN}, progressXmm8{NAN};
        float start{NAN}, target{NAN}, remaining{NAN}, receiverField{NAN};
        U2ShadowResult shadow{};
        float shadowEndpoint{NAN}, shadowDestination{NAN}, shadowAspect{NAN};
        std::uint64_t shadowGameplayEpoch{}, shadowCinematicEpoch{};
    };

    struct PostExitBlendPair
    {
        PostExitBlendSample latest{};
        std::uint64_t callbacks{}, serial{}, writerObservations{};
        bool targetObserved{}, pending{};
    };

    // Pure observation under the existing camera gate. Addresses are metadata:
    // this model never dereferences them or grants camera correction authority.
    class PostExitBlendObservation
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto CaptureDuration = std::chrono::seconds(60);
        static constexpr std::uint32_t RecordLimit = 192;
        static constexpr std::uint32_t CallbackLimit = 4096;
        static constexpr std::size_t PairLimit = 4;

        bool TakeRecord() noexcept
        {
            if (records_ >= RecordLimit) return false;
            ++records_;
            return true;
        }
        void Begin(std::uint64_t generation, std::uintptr_t camera,
            float target, Clock::time_point now = Clock::now()) noexcept
        {
            if (!windowStarted_) {
                deadline_ = now + CaptureDuration;
                windowStarted_ = true;
            }
            pairs_ = {};
            generation_ = generation;
            camera_ = camera;
            exitTarget_ = target;
            callbacks_ = rejected_ = 0;
            active_ = true;
            // records_ and serial_ deliberately survive new generations/reset.
        }
        void End() noexcept { active_ = false; }
        bool Active(Clock::time_point now = Clock::now()) const noexcept
        {
            return active_ && windowStarted_ && now < deadline_ &&
                records_ < RecordLimit && callbacks_ < CallbackLimit;
        }
        std::uint64_t Generation() const noexcept { return generation_; }
        std::uintptr_t Camera() const noexcept { return camera_; }
        float ExitTarget() const noexcept { return exitTarget_; }
        std::uint32_t Records() const noexcept { return records_; }
        std::uint32_t Callbacks() const noexcept { return callbacks_; }
        std::uint32_t Rejected() const noexcept { return rejected_; }

        const PostExitBlendPair* Observe(const PostExitBlendSample& sample) noexcept
        {
            if (!Active()) return nullptr;
            ++callbacks_;
            if (!sample.context || !sample.receiver) { ++rejected_; return nullptr; }
            PostExitBlendPair* selected{};
            for (auto& pair : pairs_)
                if (pair.callbacks && pair.latest.context == sample.context &&
                    pair.latest.receiver == sample.receiver) { selected = &pair; break; }
            if (!selected)
                for (auto& pair : pairs_)
                    if (!pair.callbacks) { selected = &pair; break; }
            if (!selected) { ++rejected_; return nullptr; }
            auto& pair = *selected;
            ++pair.callbacks;
            pair.latest = sample;
            pair.serial = ++serial_;
            pair.pending = true;
            const bool endpoint = (sample.readable & 2) && sample.raw == sample.target;
            const bool firstEndpoint = endpoint && !pair.targetObserved;
            pair.targetObserved |= endpoint;
            const bool powerOfTwo = (pair.callbacks & (pair.callbacks - 1)) == 0;
            return (powerOfTwo || firstEndpoint) && TakeRecord() ? &pair : nullptr;
        }

        const PostExitBlendPair* Writer(std::uintptr_t camera,
            std::uint32_t thread) noexcept
        {
            if (!Active()) return nullptr;
            ++callbacks_;
            PostExitBlendPair* latest{};
            for (auto& pair : pairs_) {
                if (!pair.pending || pair.latest.receiver != camera ||
                    pair.latest.thread != thread) continue;
                if (!latest || pair.serial > latest->serial) latest = &pair;
            }
            if (!latest) return nullptr;
            // A second writer without a new blend must not fall back to an
            // older pending instance for the same camera/thread.
            for (auto& pair : pairs_)
                if (pair.latest.receiver == camera && pair.latest.thread == thread)
                    pair.pending = false;
            ++latest->writerObservations;
            const auto count = latest->writerObservations;
            const bool powerOfTwo = (count & (count - 1)) == 0;
            return powerOfTwo && TakeRecord() ? latest : nullptr;
        }

    private:
        std::array<PostExitBlendPair, PairLimit> pairs_{};
        std::uint64_t generation_{}, serial_{};
        std::uintptr_t camera_{};
        float exitTarget_{NAN};
        std::uint32_t records_{}, callbacks_{}, rejected_{};
        bool active_{};
        bool windowStarted_{};
        Clock::time_point deadline_{};
    };
}
