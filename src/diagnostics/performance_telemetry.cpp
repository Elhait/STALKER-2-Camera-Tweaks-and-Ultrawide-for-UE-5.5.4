#include "performance_telemetry.hpp"

#include "diagnostic_runtime.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>

namespace diagnostics
{
    namespace
    {
        constexpr std::size_t kHistogramBuckets = 10;
        constexpr std::array<std::uint64_t, kHistogramBuckets - 1> kBucketUpperNs{
            100, 250, 500, 1000, 2500, 5000, 10000, 50000, 250000 };
        constexpr std::array<const char*, static_cast<std::size_t>(RuntimeLockPath::Count)>
            kLockNames{ "dialogue", "camera-state", "fov-observation",
                "gameplay-baseline", "aspect-restoration" };
        constexpr std::array<const char*, static_cast<std::size_t>(RuntimeEvent::Count)>
            kEventNames{ "gameplay-camera-samples", "dialogue-camera-samples",
                "camera-state-publishes", "semantic-snapshot-reads",
                "cinematic-aspect-callbacks", "cinematic-viewport-queries",
                "cinematic-aspect-log-emissions" };

        struct DurationStats
        {
            std::atomic<std::uint64_t> totalNs{};
            std::atomic<std::uint64_t> maxNs{};
            std::array<std::atomic<std::uint64_t>, kHistogramBuckets> buckets{};
        };

        struct LockStats
        {
            std::atomic<std::uint64_t> acquisitions{};
            DurationStats wait;
            DurationStats hold;
            std::atomic<std::uint64_t> logCalls{};
            DurationStats log;
        };

        std::array<LockStats, static_cast<std::size_t>(RuntimeLockPath::Count)> g_locks;
        std::array<std::atomic<std::uint64_t>, static_cast<std::size_t>(RuntimeEvent::Count)>
            g_events{};
        std::atomic<bool> g_summaryRequested{};
        std::atomic<bool> g_summaryEmitted{};
        std::atomic<long long> g_firstSampleNs{};
        thread_local ScopedRuntimeMutex* g_activeLock{};

        long long NowNs() noexcept
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        void MarkFirstSample() noexcept
        {
            long long expected = 0;
            const auto now = NowNs();
            g_firstSampleNs.compare_exchange_strong(expected, now,
                std::memory_order_relaxed);
        }

        std::size_t BucketFor(std::uint64_t durationNs) noexcept
        {
            for (std::size_t index = 0; index < kBucketUpperNs.size(); ++index)
                if (durationNs <= kBucketUpperNs[index]) return index;
            return kHistogramBuckets - 1;
        }

        void UpdateDuration(DurationStats& stats, std::uint64_t durationNs) noexcept
        {
            stats.totalNs.fetch_add(durationNs, std::memory_order_relaxed);
            auto previous = stats.maxNs.load(std::memory_order_relaxed);
            while (previous < durationNs && !stats.maxNs.compare_exchange_weak(
                previous, durationNs, std::memory_order_relaxed)) {}
            stats.buckets[BucketFor(durationNs)].fetch_add(1, std::memory_order_relaxed);
        }

        std::uint64_t Load(const std::atomic<std::uint64_t>& value) noexcept
        {
            return value.load(std::memory_order_relaxed);
        }

        std::uint64_t PercentileUpperBound(const DurationStats& stats,
            std::uint64_t count, unsigned percentile) noexcept
        {
            if (!count) return 0;
            const std::uint64_t target = (count * percentile + 99) / 100;
            std::uint64_t accumulated = 0;
            for (std::size_t index = 0; index < kHistogramBuckets; ++index) {
                accumulated += Load(stats.buckets[index]);
                if (accumulated >= target)
                    return index < kBucketUpperNs.size()
                        ? kBucketUpperNs[index] : 250001;
            }
            return 250001;
        }

        void AppendDuration(std::ostringstream& out, const char* label,
            const DurationStats& stats, std::uint64_t count)
        {
            out << ' ' << label << "_avg_ns=" << (count ? Load(stats.totalNs) / count : 0)
                << ' ' << label << "_p50_upper_ns=" << PercentileUpperBound(stats, count, 50)
                << ' ' << label << "_p95_upper_ns=" << PercentileUpperBound(stats, count, 95)
                << ' ' << label << "_max_ns=" << Load(stats.maxNs);
        }
    }

    ScopedRuntimeMutex::ScopedRuntimeMutex(std::mutex& mutex, RuntimeLockPath path)
        : mutex_(mutex), path_(path)
    {
        if (!Enabled()) {
            mutex_.lock();
            return;
        }
        const auto waitStart = NowNs();
        mutex_.lock();
        const auto acquiredAt = NowNs();
        measured_ = true;
        acquiredAtNs_ = acquiredAt;
        waitNs_ = static_cast<std::uint64_t>(acquiredAt - waitStart);
        previousLock_ = g_activeLock;
        g_activeLock = this;
    }

    ScopedRuntimeMutex::~ScopedRuntimeMutex() noexcept
    {
        if (measured_) {
            const auto endedAt = NowNs();
            g_activeLock = previousLock_;
            mutex_.unlock();
            auto& stats = g_locks[static_cast<std::size_t>(path_)];
            stats.acquisitions.fetch_add(1, std::memory_order_relaxed);
            UpdateDuration(stats.wait, waitNs_);
            UpdateDuration(stats.hold,
                static_cast<std::uint64_t>(endedAt - acquiredAtNs_));
            if (logCalls_ != 0) {
                stats.logCalls.fetch_add(logCalls_, std::memory_order_relaxed);
                stats.log.totalNs.fetch_add(logTotalNs_, std::memory_order_relaxed);
                for (std::size_t index = 0; index < kHistogramBuckets; ++index)
                    stats.log.buckets[index].fetch_add(logBuckets_[index],
                        std::memory_order_relaxed);
                auto previous = stats.log.maxNs.load(std::memory_order_relaxed);
                while (previous < logMaxNs_ && !stats.log.maxNs.compare_exchange_weak(
                    previous, logMaxNs_, std::memory_order_relaxed)) {}
            }
            MarkFirstSample();
            return;
        }
        mutex_.unlock();
    }

    void ScopedRuntimeMutex::RecordLog(std::uint64_t durationNs) noexcept
    {
        ++logCalls_;
        logTotalNs_ += durationNs;
        logMaxNs_ = logMaxNs_ < durationNs ? durationNs : logMaxNs_;
        ++logBuckets_[BucketFor(durationNs)];
    }

    ScopedRuntimeLog::ScopedRuntimeLog() noexcept
    {
        if (!Enabled() || !g_activeLock) return;
        measured_ = true;
        lock_ = g_activeLock;
        startedAtNs_ = NowNs();
    }

    ScopedRuntimeLog::~ScopedRuntimeLog() noexcept
    {
        if (!measured_) return;
        lock_->RecordLog(static_cast<std::uint64_t>(NowNs() - startedAtNs_));
    }

    void RecordRuntimeEvent(RuntimeEvent event) noexcept
    {
        if (!Enabled()) return;
        g_events[static_cast<std::size_t>(event)].fetch_add(1, std::memory_order_relaxed);
        MarkFirstSample();
    }

    void RequestRuntimePerformanceSummary() noexcept
    {
        if (Enabled() && !g_summaryEmitted.load(std::memory_order_relaxed))
            g_summaryRequested.store(true, std::memory_order_release);
    }

    bool TakeRuntimePerformanceSummary(std::string& summary) noexcept
    {
        if (!Enabled() || !g_summaryRequested.exchange(false, std::memory_order_acq_rel) ||
            g_summaryEmitted.load(std::memory_order_relaxed))
            return false;
        try {
            std::ostringstream out;
            const auto firstSample = g_firstSampleNs.load(std::memory_order_relaxed);
            const auto elapsedNs = firstSample != 0
                ? static_cast<std::uint64_t>(NowNs() - firstSample) : 0;
            out << "PERF_RUNTIME_SUMMARY diagnostics=on elapsed_ms="
                << elapsedNs / 1000000;
            for (std::size_t index = 0; index < g_events.size(); ++index)
                out << ' ' << kEventNames[index] << '=' << Load(g_events[index]);
            for (std::size_t index = 0; index < g_locks.size(); ++index) {
                const auto& lock = g_locks[index];
                const auto acquisitions = Load(lock.acquisitions);
                out << " | lock=" << kLockNames[index]
                    << " acquisitions=" << acquisitions;
                AppendDuration(out, "wait", lock.wait, acquisitions);
                AppendDuration(out, "hold", lock.hold, acquisitions);
                const auto logCalls = Load(lock.logCalls);
                out << " log_calls=" << logCalls;
                AppendDuration(out, "log", lock.log, logCalls);
            }
            summary = out.str();
            g_summaryEmitted.store(true, std::memory_order_release);
            return true;
        } catch (...) {
            return false;
        }
    }
}
