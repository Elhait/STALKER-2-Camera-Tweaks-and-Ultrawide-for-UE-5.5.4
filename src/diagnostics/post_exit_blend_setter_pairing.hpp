#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace diagnostics
{
    struct BlendSetterInvocation
    {
        std::uint64_t generation{}, epoch{};
        std::uintptr_t component{}, receiver{}, stackPointer{};
        std::uint32_t thread{};
        float start{}, target{}, remaining{}, sample{}, setterArgument{};
        float progressXmm7{}, progressXmm8{};
        bool recordSelected{};
    };

    enum class BlendSetterPairStatus : std::uint8_t
    {
        Matched,
        Empty,
        Mismatch,
        Overflow,
    };

    struct BlendSetterPairResult
    {
        BlendSetterPairStatus status{BlendSetterPairStatus::Empty};
        BlendSetterInvocation invocation{};
    };

    // A per-thread bounded LIFO pairs the pre-call and validated return-site
    // callbacks. Any structural mismatch clears pending entries so stale
    // samples cannot be attached to a later setter call.
    class PostExitBlendSetterPairing
    {
    public:
        static constexpr std::size_t Capacity = 8;

        bool Push(const BlendSetterInvocation& invocation) noexcept
        {
            if (!invocation.thread || !invocation.stackPointer ||
                !invocation.component || !invocation.receiver) {
                ++invalid_;
                return false;
            }
            if (depth_ == Capacity) {
                depth_ = 0;
                ++overflow_;
                return false;
            }
            entries_[depth_++] = invocation;
            return true;
        }

        BlendSetterPairResult Pop(std::uint32_t thread,
            std::uintptr_t stackPointer, std::uintptr_t component) noexcept
        {
            if (!depth_) return {};
            const auto& top = entries_[depth_ - 1];
            if (thread != top.thread || stackPointer != top.stackPointer ||
                component != top.component) {
                const auto unmatched = top;
                depth_ = 0;
                ++mismatch_;
                return {BlendSetterPairStatus::Mismatch, unmatched};
            }
            auto invocation = top;
            entries_[--depth_] = {};
            return {BlendSetterPairStatus::Matched, invocation};
        }

        void Reset() noexcept
        {
            entries_ = {};
            depth_ = 0;
        }

        std::uint64_t Overflow() const noexcept { return overflow_; }
        std::uint64_t Mismatch() const noexcept { return mismatch_; }
        std::uint64_t Invalid() const noexcept { return invalid_; }
        std::size_t Pending() const noexcept { return depth_; }

    private:
        std::array<BlendSetterInvocation, Capacity> entries_{};
        std::size_t depth_{};
        std::uint64_t overflow_{}, mismatch_{}, invalid_{};
    };
}
