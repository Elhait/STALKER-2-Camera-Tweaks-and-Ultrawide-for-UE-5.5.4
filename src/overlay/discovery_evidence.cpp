#include "discovery_evidence.hpp"

#include <algorithm>

namespace overlay
{
    bool FactoryEvidenceStore::ObserveFactory(std::uintptr_t object) noexcept
    {
        if (!object) return false;
        const auto found = std::find_if(records_.begin(), records_.end(),
            [object](const Record& value) { return value.object == object; });
        if (found != records_.end()) return false;
        records_.push_back({object, 0});
        return true;
    }

    bool FactoryEvidenceStore::RecordMethod(std::uintptr_t object,
        FactoryMethod method) noexcept
    {
        if (!object) return false;
        const auto found = std::find_if(records_.begin(), records_.end(),
            [object](const Record& value) { return value.object == object; });
        if (found == records_.end()) return false;
        const auto bit = static_cast<std::uint32_t>(method);
        if ((found->methods & bit) != 0) return false;
        found->methods |= bit;
        return true;
    }

    std::size_t FactoryEvidenceStore::FactoryCount() const noexcept
    {
        return records_.size();
    }

    std::uint32_t FactoryEvidenceStore::Methods(std::uintptr_t object) const noexcept
    {
        const auto found = std::find_if(records_.begin(), records_.end(),
            [object](const Record& value) { return value.object == object; });
        return found == records_.end() ? 0 : found->methods;
    }

    void ObservationWindow::Arm(std::uint32_t samples) noexcept
    {
        remaining_ = samples;
    }

    bool ObservationWindow::Consume() noexcept
    {
        if (!remaining_) return false;
        --remaining_;
        return true;
    }

    void QueueEvidenceStore::BeginSwapchain(std::uintptr_t swapchain) noexcept
    {
        candidates_.erase(std::remove_if(candidates_.begin(), candidates_.end(),
            [swapchain](const QueueEvidence& value) {
                return value.swapchain == swapchain;
            }), candidates_.end());
    }

    bool QueueEvidenceStore::ObserveCandidate(std::uintptr_t swapchain,
        std::uintptr_t device, std::uintptr_t queue, std::uint32_t type) noexcept
    {
        if (!swapchain || !device || !queue) return false;
        const auto duplicate = std::find_if(candidates_.begin(), candidates_.end(),
            [=](const QueueEvidence& value) {
                return value.swapchain == swapchain && value.device == device &&
                    value.queue == queue && value.type == type;
            });
        if (duplicate != candidates_.end()) return false;
        candidates_.push_back({swapchain, device, queue, type, 0});
        return true;
    }

    void QueueEvidenceStore::ObservePresent(std::uintptr_t swapchain) noexcept
    {
        for (auto& value : candidates_)
            if (value.swapchain == swapchain) {
                ++value.presents;
                value.resizePending = false;
            }
    }

    void QueueEvidenceStore::BeginResize(std::uintptr_t swapchain) noexcept
    {
        for (auto& value : candidates_)
            if (value.swapchain == swapchain) value.resizePending = true;
    }

    void QueueEvidenceStore::CompleteResize(std::uintptr_t swapchain,
        bool success) noexcept
    {
        if (!success) InvalidateSwapchain(swapchain);
    }

    void QueueEvidenceStore::InvalidateSwapchain(std::uintptr_t swapchain) noexcept
    {
        BeginSwapchain(swapchain);
    }

    std::size_t QueueEvidenceStore::CandidateCount(std::uintptr_t swapchain) const noexcept
    {
        return static_cast<std::size_t>(std::count_if(candidates_.begin(), candidates_.end(),
            [swapchain](const QueueEvidence& value) {
                return value.swapchain == swapchain;
            }));
    }

    AssociationState QueueEvidenceStore::State(std::uintptr_t swapchain) const noexcept
    {
        const auto count = CandidateCount(swapchain);
        if (count == 0) return AssociationState::Unknown;
        if (count != 1) return AssociationState::Ambiguous;
        const auto found = std::find_if(candidates_.begin(), candidates_.end(),
            [swapchain](const QueueEvidence& value) {
                return value.swapchain == swapchain;
            });
        if (found->resizePending) return AssociationState::ResizeRevalidationRequired;
        return found->presents ? AssociationState::Supported : AssociationState::Unknown;
    }

    bool QueueEvidenceStore::GetSingleCandidate(std::uintptr_t swapchain,
        std::uintptr_t& device, std::uintptr_t& queue) const noexcept
    {
        const auto found = std::find_if(candidates_.begin(), candidates_.end(),
            [swapchain](const QueueEvidence& value) {
                return value.swapchain == swapchain;
            });
        if (CandidateCount(swapchain) != 1 || found == candidates_.end()) return false;
        device = found->device;
        queue = found->queue;
        return true;
    }

    bool QueueEvidenceStore::HasSufficientAssociation(std::uintptr_t swapchain) const noexcept
    {
        return State(swapchain) == AssociationState::Supported;
    }
}
