#pragma once

#include <mutex>
#include <array>
#include <cstdint>
#include <string>

namespace diagnostics
{
    enum class RuntimeLockPath
    {
        Dialogue,
        CameraState,
        FovObservation,
        GameplayBaseline,
        AspectRestoration,
        Count,
    };

    enum class RuntimeEvent
    {
        GameplayCameraSample,
        DialogueCameraSample,
        CameraStatePublish,
        SemanticSnapshotRead,
        CinematicAspectCallback,
        CinematicViewportQuery,
        CinematicAspectLog,
        Count,
    };

    class ScopedRuntimeMutex
    {
    public:
        ScopedRuntimeMutex(std::mutex& mutex, RuntimeLockPath path);
        ~ScopedRuntimeMutex() noexcept;
        ScopedRuntimeMutex(const ScopedRuntimeMutex&) = delete;
        ScopedRuntimeMutex& operator=(const ScopedRuntimeMutex&) = delete;

    private:
        friend class ScopedRuntimeLog;
        void RecordLog(std::uint64_t durationNs) noexcept;
        std::mutex& mutex_;
        RuntimeLockPath path_;
        ScopedRuntimeMutex* previousLock_{};
        long long acquiredAtNs_{};
        std::uint64_t waitNs_{};
        std::uint64_t logCalls_{};
        std::uint64_t logTotalNs_{};
        std::uint64_t logMaxNs_{};
        std::array<std::uint64_t, 10> logBuckets_{};
        bool measured_{};
    };

    class ScopedRuntimeLog
    {
    public:
        ScopedRuntimeLog() noexcept;
        ~ScopedRuntimeLog() noexcept;
        ScopedRuntimeLog(const ScopedRuntimeLog&) = delete;
        ScopedRuntimeLog& operator=(const ScopedRuntimeLog&) = delete;

    private:
        ScopedRuntimeMutex* lock_{};
        long long startedAtNs_{};
        bool measured_{};
    };

    void RecordRuntimeEvent(RuntimeEvent event) noexcept;
    void RequestRuntimePerformanceSummary() noexcept;
    bool TakeRuntimePerformanceSummary(std::string& summary) noexcept;
}
