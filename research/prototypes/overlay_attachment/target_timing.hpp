#pragma once
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace overlay::target_timing_experiment {
    enum class Operation { SetRoot, Commit, SurfaceCommit, AttachmentCommit, ReleaseTarget, WindowMutexBefore, WindowMutexAfter };
    struct Sample { Operation operation{}; LONGLONG begin{}, end{}; DWORD thread{}; };
    struct Slot { std::atomic<unsigned> state{0}; Sample sample{}; };
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    static_assert(std::atomic<LONGLONG>::is_always_lock_free);
    inline std::array<Slot, 64> slots;
    inline std::atomic<LONGLONG> frequency{0};
    inline std::atomic<unsigned> cursor{0}, dropped{0};
    inline std::atomic<DWORD> ownerThread{0};
    inline std::atomic<bool> wakePending{false};
    constexpr UINT TimingWakeMessage = WM_APP + 0x5A5;

    inline void Initialize() noexcept {
        LARGE_INTEGER value{};
        if (QueryPerformanceFrequency(&value)) frequency.store(value.QuadPart);
        ownerThread.store(GetCurrentThreadId());
    }
    inline LONGLONG Now() noexcept {
        LARGE_INTEGER value{};
        return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
    }
    inline bool Record(Operation operation, LONGLONG begin, LONGLONG end) noexcept {
        const auto hz = frequency.load(std::memory_order_relaxed);
        // Inclusive 2 ms threshold; do not record unavailable clocks or fast calls.
        if (hz <= 0 || begin <= 0 || end < begin || end - begin < (hz + 499) / 500) return false;
        const auto offset = cursor.fetch_add(1, std::memory_order_relaxed);
        for (unsigned index = 0; index < slots.size(); ++index) {
            auto& slot = slots[(offset + index) % slots.size()];
            unsigned expected = 0;
            if (!slot.state.compare_exchange_strong(expected, 1, std::memory_order_acquire)) continue;
            slot.sample = {operation, begin, end, GetCurrentThreadId()};
            slot.state.store(2, std::memory_order_release);
            return true;
        }
        dropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    struct Span {
        Operation operation;
        LONGLONG begin{Now()};
        explicit Span(Operation value) noexcept : operation(value) {}
        ~Span() { Record(operation, begin, Now()); }
    };
    template<class Function> decltype(auto) Invoke(Operation operation, Function&& function) {
        const Span span{operation};
        return std::forward<Function>(function)();
    }
    class WindowLock {
        std::unique_lock<std::recursive_mutex> lock_;
    public:
        WindowLock(std::recursive_mutex& mutex, Operation operation) : lock_(mutex, std::defer_lock) {
            const auto begin = Now();
            lock_.lock();
            if (Record(operation, begin, Now()) && !wakePending.exchange(true)) {
                const auto thread = ownerThread.load();
                if (!thread || !PostThreadMessageW(thread, TimingWakeMessage, 0, 0)) wakePending.store(false);
            }
        }
    };
    template<class Consumer> void Consume(Consumer&& consumer) {
        for (auto& slot : slots) {
            unsigned expected = 2;
            if (!slot.state.compare_exchange_strong(expected, 1, std::memory_order_acquire)) continue;
            const Sample sample = slot.sample;
            slot.state.store(0, std::memory_order_release);
            consumer(sample);
        }
    }
    inline const char* Name(Operation operation) noexcept {
        switch (operation) {
        case Operation::SetRoot: return "SetRoot";
        case Operation::Commit: return "Commit";
        case Operation::SurfaceCommit: return "CommitSurface";
        case Operation::AttachmentCommit: return "CommitAttachment";
        case Operation::ReleaseTarget: return "ReleaseTarget";
        case Operation::WindowMutexBefore: return "WindowMutexBefore";
        case Operation::WindowMutexAfter: return "WindowMutexAfter";
        }
        return "unknown";
    }
    template<class Logger> void Drain(Logger&& logger) noexcept {
        wakePending.store(false);
        const auto hz = frequency.load(std::memory_order_relaxed);
        try {
            Consume([&](const Sample& sample) {
                logger(std::string("OVERLAY_TARGET_SLOW operation=") + Name(sample.operation) +
                    " duration_us=" + std::to_string((sample.end - sample.begin) * 1000000 / hz) +
                    " begin_qpc=" + std::to_string(sample.begin) + " end_qpc=" + std::to_string(sample.end) +
                    " tid=" + std::to_string(sample.thread) + " qpc_frequency=" + std::to_string(hz));
            });
            const auto lost = dropped.exchange(0);
            if (lost) logger("OVERLAY_TARGET_TIMING_DROPPED count=" + std::to_string(lost));
        } catch (...) { /* Experimental diagnostics cannot escape the worker boundary. */ }
    }
}
