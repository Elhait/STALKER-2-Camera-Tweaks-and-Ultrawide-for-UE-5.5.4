#pragma once

#include <mutex>
#include <functional>
#include <utility>
#include <vector>

namespace plugin
{
    // RuntimeState-owned serialization boundary shared by camera callbacks
    // and runtime camera-setting commits. Keep the guard scoped to the
    // callback/commit; never retain it across persistence or worker waits.
    class CameraCorrectionGate
    {
    public:
        class Guard
        {
        public:
            explicit Guard(CameraCorrectionGate& gate) : gate_(&gate), lock_(gate.mutex_)
            {
                currentGate_ = gate_;
            }

            Guard(const Guard&) = delete;
            Guard& operator=(const Guard&) = delete;

            Guard(Guard&& other) noexcept
                : gate_(std::exchange(other.gate_, nullptr)),
                  lock_(std::move(other.lock_))
            {}

            ~Guard()
            {
                if (!gate_) return;
                lock_.unlock();
                currentGate_ = nullptr;
                auto deferred = std::move(deferredActions_);
                deferredActions_.clear();
                for (auto& action : deferred) {
                    try { action(); } catch (...) {}
                }
            }

        private:
            CameraCorrectionGate* gate_{};
            std::unique_lock<std::mutex> lock_;
        };

        Guard Acquire()
        {
            return Guard(*this);
        }

        template<typename Action>
        bool DeferUntilReleased(Action&& action)
        {
            if (currentGate_ != this) return false;
            deferredActions_.emplace_back(std::forward<Action>(action));
            return true;
        }

        bool IsHeldByCurrentThread() const noexcept
        {
            return currentGate_ == this;
        }

    private:
        inline static thread_local CameraCorrectionGate* currentGate_{};
        inline static thread_local std::vector<std::function<void()>> deferredActions_{};
        std::mutex mutex_;
    };
}
