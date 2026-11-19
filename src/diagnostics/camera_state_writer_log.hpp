#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace diagnostics
{
    enum class CameraStateWriterEventKind : std::uint8_t
    {
        Before,
        Decision,
        PostStore,
        PostStoreUnavailable,
        PostProcess,
        PostProcessUnavailable,
    };

    enum class CameraStatePostStoreStatus : std::uint8_t
    {
        Unavailable,
        MatchedAtCorrelatedSite,
        DifferentAtCorrelatedSite,
        IdentityMismatch,
    };

    enum class CameraPostProcessBranch : std::uint8_t
    {
        Unavailable,
        Inactive,
        Active,
        UnorderedActive,
    };

    struct CameraStateWriterEvent
    {
        std::uint64_t eventSequence{};
        std::uint64_t writerTransactionId{};
        std::uint64_t lifecycleGeneration{};
        std::uint64_t gameplaySettingsEpoch{};
        std::uint64_t cinematicSettingsEpoch{};
        std::uint64_t dialogueSettingsEpoch{};
        std::uintptr_t receiver{};
        std::uintptr_t outputObject{};
        std::uintptr_t fovFieldAddress{};
        std::uint32_t threadId{};
        float inputFov{};
        float plannedAfter{};
        float actualAfter{};
        float previousFieldValue{};
        float writerAspect{};
        float policyAspect{};
        float postProcessInputFov{};
        float postProcessOutputFov{};
        float postProcessNativeCameraFov{};
        float postProcessWriterInputFov{};
        float postProcessModifier{};
        float postProcessWriterFallbackDelta{};
        float postProcessSmoothingBefore{};
        float postProcessSmoothingAfter{};
        std::uint8_t flags{};
        std::uint8_t writerSiteId{};
        std::uint8_t kind{};
        std::uint8_t coordinator{};
        std::uint8_t writerContext{};
        std::uint8_t gameplayMode{};
        std::uint8_t dialoguePolicy{};
        std::uint8_t cinematicAction{};
        std::uint8_t continuation{};
        std::uint8_t disposition{};
        std::uint8_t postStoreStatus{};
        std::uint8_t postProcessBranch{};
        std::uint8_t postProcessWriterBranchFlags{};
        const char* decisionReason{};
        const char* postProcessCorrelation{};
        bool decisionAvailable{};
        bool mutateFov{};
        bool previousFieldValid{};
        bool actualAfterValid{};
        bool writerAspectValid{};
        bool policyAspectValid{};
        bool flagsValid{};
        bool postProcessInputValid{};
        bool postProcessOutputValid{};
        bool postProcessNativeCameraFovValid{};
        bool postProcessWriterInputValid{};
        bool postProcessModifierValid{};
        bool postProcessWriterFallbackDeltaValid{};
        bool postProcessWriterBranchFlagsValid{};
        bool postProcessSmoothingBeforeValid{};
        bool postProcessSmoothingAfterValid{};
    };

    inline bool SameCameraStateWriterPayload(
        const CameraStateWriterEvent& left,
        const CameraStateWriterEvent& right) noexcept
    {
        const auto sameFloat = [](float a, float b) noexcept {
            return std::memcmp(&a, &b, sizeof(float)) == 0;
        };
        return left.lifecycleGeneration == right.lifecycleGeneration &&
            left.gameplaySettingsEpoch == right.gameplaySettingsEpoch &&
            left.cinematicSettingsEpoch == right.cinematicSettingsEpoch &&
            left.dialogueSettingsEpoch == right.dialogueSettingsEpoch &&
            left.receiver == right.receiver && left.outputObject == right.outputObject &&
            left.fovFieldAddress == right.fovFieldAddress && left.threadId == right.threadId &&
            sameFloat(left.inputFov, right.inputFov) &&
            sameFloat(left.plannedAfter, right.plannedAfter) &&
            sameFloat(left.actualAfter, right.actualAfter) &&
            sameFloat(left.previousFieldValue, right.previousFieldValue) &&
            sameFloat(left.writerAspect, right.writerAspect) &&
            sameFloat(left.policyAspect, right.policyAspect) &&
            sameFloat(left.postProcessInputFov, right.postProcessInputFov) &&
            sameFloat(left.postProcessOutputFov, right.postProcessOutputFov) &&
            sameFloat(left.postProcessNativeCameraFov, right.postProcessNativeCameraFov) &&
            sameFloat(left.postProcessWriterInputFov, right.postProcessWriterInputFov) &&
            sameFloat(left.postProcessModifier, right.postProcessModifier) &&
            sameFloat(left.postProcessWriterFallbackDelta, right.postProcessWriterFallbackDelta) &&
            sameFloat(left.postProcessSmoothingBefore, right.postProcessSmoothingBefore) &&
            sameFloat(left.postProcessSmoothingAfter, right.postProcessSmoothingAfter) &&
            left.flags == right.flags && left.writerSiteId == right.writerSiteId &&
            left.kind == right.kind && left.coordinator == right.coordinator &&
            left.writerContext == right.writerContext && left.gameplayMode == right.gameplayMode &&
            left.dialoguePolicy == right.dialoguePolicy &&
            left.cinematicAction == right.cinematicAction &&
            left.continuation == right.continuation && left.disposition == right.disposition &&
            left.postStoreStatus == right.postStoreStatus &&
            left.postProcessBranch == right.postProcessBranch &&
            left.postProcessWriterBranchFlags == right.postProcessWriterBranchFlags &&
            left.decisionReason == right.decisionReason &&
            left.postProcessCorrelation == right.postProcessCorrelation &&
            left.decisionAvailable == right.decisionAvailable &&
            left.mutateFov == right.mutateFov &&
            left.previousFieldValid == right.previousFieldValid &&
            left.actualAfterValid == right.actualAfterValid &&
            left.writerAspectValid == right.writerAspectValid &&
            left.policyAspectValid == right.policyAspectValid &&
            left.flagsValid == right.flagsValid &&
            left.postProcessInputValid == right.postProcessInputValid &&
            left.postProcessOutputValid == right.postProcessOutputValid &&
            left.postProcessNativeCameraFovValid == right.postProcessNativeCameraFovValid &&
            left.postProcessWriterInputValid == right.postProcessWriterInputValid &&
            left.postProcessModifierValid == right.postProcessModifierValid &&
            left.postProcessWriterFallbackDeltaValid == right.postProcessWriterFallbackDeltaValid &&
            left.postProcessWriterBranchFlagsValid == right.postProcessWriterBranchFlagsValid &&
            left.postProcessSmoothingBeforeValid == right.postProcessSmoothingBeforeValid &&
            left.postProcessSmoothingAfterValid == right.postProcessSmoothingAfterValid;
    }

    struct CameraStateWriterEventRun
    {
        CameraStateWriterEvent first{};
        CameraStateWriterEvent last{};
        std::uint64_t count{};
    };

    enum class CameraStateWriterRunUpdate : std::uint8_t
    {
        Started,
        Coalesced,
        Replaced,
    };

    // Adjacent identical post-process transactions can be represented by one
    // record plus their exact sequence/id range. Any payload change ends the run.
    class CameraStateWriterLogCoalescer
    {
    public:
        CameraStateWriterRunUpdate Observe(const CameraStateWriterEvent& event,
            CameraStateWriterEventRun& completed) noexcept
        {
            completed = {};
            if (!run_.count) {
                run_ = {event, event, 1};
                return CameraStateWriterRunUpdate::Started;
            }
            if (SameCameraStateWriterPayload(run_.first, event)) {
                run_.last = event;
                ++run_.count;
                return CameraStateWriterRunUpdate::Coalesced;
            }
            completed = run_;
            run_ = {event, event, 1};
            return CameraStateWriterRunUpdate::Replaced;
        }

        bool Flush(CameraStateWriterEventRun& completed) noexcept
        {
            if (!run_.count) return false;
            completed = run_;
            run_ = {};
            return true;
        }

    private:
        CameraStateWriterEventRun run_{};
    };

    struct CameraStateWriterLogCounters
    {
        std::uint64_t accepted{};
        std::uint64_t droppedFull{};
        std::uint64_t droppedContended{};
    };

    template<std::size_t Capacity>
    class CameraStateWriterLogQueue
    {
        static_assert(Capacity > 0);
    public:
        bool TryPush(CameraStateWriterEvent event) noexcept
        {
            std::unique_lock lock(mutex_, std::try_to_lock);
            if (!lock.owns_lock()) {
                droppedContended_.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            if (size_ == Capacity) {
                droppedFull_.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            event.eventSequence = ++nextEventSequence_;
            entries_[writeIndex_] = event;
            writeIndex_ = (writeIndex_ + 1) % Capacity;
            ++size_;
            accepted_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        bool TryPop(CameraStateWriterEvent& event) noexcept
        {
            std::lock_guard lock(mutex_);
            if (size_ == 0) return false;
            event = entries_[readIndex_];
            readIndex_ = (readIndex_ + 1) % Capacity;
            --size_;
            return true;
        }

        CameraStateWriterLogCounters Counters() const noexcept
        {
            return {accepted_.load(std::memory_order_relaxed),
                droppedFull_.load(std::memory_order_relaxed),
                droppedContended_.load(std::memory_order_relaxed)};
        }

        std::size_t Pending() const noexcept
        {
            std::lock_guard lock(mutex_);
            return size_;
        }

    private:
        mutable std::mutex mutex_;
        std::array<CameraStateWriterEvent, Capacity> entries_{};
        std::size_t readIndex_{};
        std::size_t writeIndex_{};
        std::size_t size_{};
        std::uint64_t nextEventSequence_{};
        std::atomic<std::uint64_t> accepted_{};
        std::atomic<std::uint64_t> droppedFull_{};
        std::atomic<std::uint64_t> droppedContended_{};
    };

    inline const char* CameraStateWriterEventKindName(
        CameraStateWriterEventKind value) noexcept
    {
        switch (value) {
        case CameraStateWriterEventKind::Before: return "BEFORE";
        case CameraStateWriterEventKind::Decision: return "DECISION";
        case CameraStateWriterEventKind::PostStore: return "POST_STORE";
        case CameraStateWriterEventKind::PostStoreUnavailable: return "POST_STORE_UNAVAILABLE";
        case CameraStateWriterEventKind::PostProcess: return "POST_PROCESS";
        case CameraStateWriterEventKind::PostProcessUnavailable:
            return "POST_PROCESS_UNAVAILABLE";
        }
        return "UNKNOWN";
    }

    inline const char* CameraPostProcessBranchName(
        CameraPostProcessBranch value) noexcept
    {
        switch (value) {
        case CameraPostProcessBranch::Unavailable: return "Unavailable";
        case CameraPostProcessBranch::Inactive: return "Inactive";
        case CameraPostProcessBranch::Active: return "Active";
        case CameraPostProcessBranch::UnorderedActive: return "UnorderedActive";
        }
        return "Unknown";
    }

    inline const char* CameraStatePostStoreStatusName(
        CameraStatePostStoreStatus value) noexcept
    {
        switch (value) {
        case CameraStatePostStoreStatus::Unavailable: return "Unavailable";
        case CameraStatePostStoreStatus::MatchedAtCorrelatedSite:
            return "MatchedAtCorrelatedSite";
        case CameraStatePostStoreStatus::DifferentAtCorrelatedSite:
            return "DifferentAtCorrelatedSite";
        case CameraStatePostStoreStatus::IdentityMismatch: return "IdentityMismatch";
        }
        return "Unknown";
    }
}
