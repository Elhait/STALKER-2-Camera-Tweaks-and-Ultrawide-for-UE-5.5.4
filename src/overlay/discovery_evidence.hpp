#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace overlay
{
    // DXGI_STATUS_* values are non-failing HRESULTs but do not prove that a
    // frame reached the presentation path. Only S_OK advances readiness.
    bool IsSuccessfulPresentResult(std::int32_t result) noexcept;

    enum class FactoryMethod : std::uint32_t
    {
        CreateSwapChain = 1u << 0,
        CreateSwapChainForHwnd = 1u << 1,
        CreateSwapChainForCoreWindow = 1u << 2,
        CreateSwapChainForComposition = 1u << 3,
    };

    class FactoryEvidenceStore
    {
    public:
        bool ObserveFactory(std::uintptr_t object);
        bool RecordMethod(std::uintptr_t object, FactoryMethod method) noexcept;
        std::size_t FactoryCount() const noexcept;
        std::uint32_t Methods(std::uintptr_t object) const noexcept;

    private:
        struct Record
        {
            std::uintptr_t object{};
            std::uint32_t methods{};
        };
        std::vector<Record> records_;
    };

    class ObservationWindow
    {
    public:
        void Arm(std::uint32_t samples) noexcept;
        bool Consume() noexcept;
        bool Open() const noexcept { return remaining_ != 0; }
        std::uint32_t remaining() const noexcept { return remaining_; }

    private:
        std::uint32_t remaining_{};
    };

    enum class AssociationState : std::uint8_t
    {
        Unknown,
        Supported,
        ResizeRevalidationRequired,
        Ambiguous,
    };

    struct QueueEvidence
    {
        std::uintptr_t swapchain{};
        std::uintptr_t device{};
        std::uintptr_t queue{};
        std::uint32_t type{};
        std::uint32_t presents{};
        bool resizePending{};
    };

    class QueueEvidenceStore
    {
    public:
        void BeginSwapchain(std::uintptr_t swapchain) noexcept;
        bool ObserveCandidate(std::uintptr_t swapchain, std::uintptr_t device,
            std::uintptr_t queue, std::uint32_t type);
        void ObservePresent(std::uintptr_t swapchain) noexcept;
        std::uint32_t SuccessfulPresentCount(std::uintptr_t swapchain) const noexcept;
        void BeginResize(std::uintptr_t swapchain) noexcept;
        void CompleteResize(std::uintptr_t swapchain, bool success) noexcept;
        void InvalidateSwapchain(std::uintptr_t swapchain) noexcept;
        std::size_t CandidateCount(std::uintptr_t swapchain) const noexcept;
        AssociationState State(std::uintptr_t swapchain) const noexcept;
        bool GetSingleCandidate(std::uintptr_t swapchain, std::uintptr_t& device,
            std::uintptr_t& queue) const noexcept;
        bool HasSufficientAssociation(std::uintptr_t swapchain) const noexcept;
        bool HasStableAssociation(std::uintptr_t swapchain,
            std::uint32_t requiredSuccessfulPresents) const noexcept;

    private:
        std::vector<QueueEvidence> candidates_;
    };
}
