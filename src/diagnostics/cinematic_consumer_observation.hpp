#pragma once
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cwchar>

namespace diagnostics {
// Two controls in the same manual experiment; consumed only by existing callbacks.
class ManualU2Controls {
public:
    ManualU2Controls() = default;
    ManualU2Controls(const ManualU2Controls&) = delete;
    ManualU2Controls& operator=(const ManualU2Controls&) = delete;
    ~ManualU2Controls() { Close(); }
    bool Initialize(DWORD pid) noexcept {
        if(event_) return false;
        wchar_t name[96]{};
        swprintf_s(name,L"Local\\STALKER2CameraTweaks.U2Control.%lu",pid);
        event_=CreateEventW(nullptr,FALSE,FALSE,name);
        if(event_ && GetLastError()==ERROR_ALREADY_EXISTS) { Close(); return false; }
        return event_!=nullptr;
    }
    unsigned Consume() noexcept {
        if(!event_ || phases_>=2 || WaitForSingleObject(event_,0)!=WAIT_OBJECT_0) return 0;
        return ++phases_;
    }
    void Close() noexcept { if(event_){CloseHandle(event_);event_=nullptr;} }
private:
    HANDLE event_{};
    unsigned phases_{};
};
class ManualFovCapture {
public:
    ManualFovCapture() = default;
    ManualFovCapture(const ManualFovCapture&) = delete;
    ManualFovCapture& operator=(const ManualFovCapture&) = delete;
    ~ManualFovCapture() { Close(); }
    bool Initialize(DWORD pid) noexcept {
        if (event_ || spent_.load(std::memory_order_acquire)) return false;
        swprintf_s(name_, L"Local\\STALKER2CameraTweaks.FovCapture.%lu", pid);
        event_ = CreateEventW(nullptr, FALSE, FALSE, name_);
        if (event_ && GetLastError() == ERROR_ALREADY_EXISTS) { Close(); return false; }
        return event_ != nullptr;
    }
    bool Consume() noexcept {
        if (!event_ || spent_.load(std::memory_order_acquire) ||
            WaitForSingleObject(event_, 0) != WAIT_OBJECT_0) return false;
        return !spent_.exchange(true, std::memory_order_acq_rel);
    }
    bool Spent() const noexcept { return spent_.load(std::memory_order_acquire); }
    const wchar_t* Name() const noexcept { return name_; }
    void Close() noexcept { if (event_) { CloseHandle(event_); event_ = nullptr; } }
private:
    HANDLE event_{};
    std::atomic<bool> spent_{};
    wchar_t name_[96]{};
};

enum class ConsumerOutcome { OutcomeUnobserved, ProviderNull, ResolverObjectFlagRejected, ExistingTargetMatched,
    CreationGuardRejected, InitializerObservationReached };
inline const char* ConsumerOutcomeName(ConsumerOutcome value) noexcept {
    switch (value) {
    case ConsumerOutcome::ProviderNull: return "ProviderNull";
    case ConsumerOutcome::ResolverObjectFlagRejected: return "ResolverObjectFlagRejected";
    case ConsumerOutcome::ExistingTargetMatched: return "ExistingTargetMatched";
    case ConsumerOutcome::CreationGuardRejected: return "CreationGuardRejected";
    case ConsumerOutcome::InitializerObservationReached: return "InitializerObservationReached";
    default: return "OutcomeUnobserved";
    }
}
struct ConsumerInvocation {
    std::uint64_t id{}, generation{};
    std::uintptr_t stack{}, provider{}, component{}, receiver{};
    DWORD thread{};
    float nativeInput{}, forwarded{}, savedTarget{};
    bool resolverSeen{}, guardSeen{}, guardAccepted{}, comparisonSeen{}, equal{}, unordered{};
    bool startSeen{}, lost{}, returned{}, epilogueSeen{}, resolverFlagRejected{};
    ConsumerOutcome Outcome() const noexcept {
        if (!returned || lost || !resolverSeen) return ConsumerOutcome::OutcomeUnobserved;
        if (!provider && epilogueSeen && !guardSeen && !startSeen)
            return ConsumerOutcome::ProviderNull;
        if (provider && epilogueSeen && resolverFlagRejected && !guardSeen && !startSeen)
            return ConsumerOutcome::ResolverObjectFlagRejected;
        if (provider && epilogueSeen && comparisonSeen && equal && !unordered &&
            !guardSeen && !startSeen)
            return ConsumerOutcome::ExistingTargetMatched;
        if (epilogueSeen && guardSeen && !guardAccepted && !startSeen)
            return ConsumerOutcome::CreationGuardRejected;
        if (startSeen) return ConsumerOutcome::InitializerObservationReached;
        return ConsumerOutcome::OutcomeUnobserved;
    }
};
// Per-native-thread bounded invocation stack. No native calls, allocations or
// camera mutation; failed correlation retires pending evidence as unknown.
class ConsumerInvocationStack {
public:
    bool Begin(ConsumerInvocation value) noexcept {
        if (size_ == frames_.size()) { Clear(); return false; }
        frames_[size_++] = value;
        return true;
    }
    ConsumerInvocation* Top() noexcept { return size_ ? &frames_[size_ - 1] : nullptr; }
    ConsumerInvocation Finish(std::uintptr_t stack, DWORD thread, std::uint64_t generation) noexcept {
        if (!size_) return {};
        auto value = frames_[--size_];
        if (value.stack != stack || value.thread != thread || value.generation != generation) {
            value.lost = true; Clear();
        }
        value.returned = true;
        return value;
    }
    void Clear() noexcept { size_ = 0; }
private:
    std::array<ConsumerInvocation, 8> frames_{};
    std::size_t size_{};
};
}
