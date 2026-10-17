#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace platform
{
    // Storage for a process-resident owner. Construction is ordinary; no CRT
    // destructor is registered for T. Release external resources explicitly
    // through their owner while threads are alive, or let Windows reclaim them.
    template<typename T>
    class ProcessLifetime final
    {
    public:
        template<typename... Args>
        explicit ProcessLifetime(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            ::new (static_cast<void*>(storage_)) T(std::forward<Args>(args)...);
        }
        ProcessLifetime(const ProcessLifetime&) = delete;
        ProcessLifetime& operator=(const ProcessLifetime&) = delete;
        T& get() noexcept { return *std::launder(reinterpret_cast<T*>(storage_)); }

    private:
        alignas(T) std::byte storage_[sizeof(T)];
    };
}
