#pragma once

#include <array>
#include <cstddef>

namespace overlay
{
    // Owner-thread-only storage for records whose addresses may escape into
    // callbacks or foreign native chains. Committed records are never reused.
    template<typename Record, std::size_t Capacity>
    class StableRecordPool final
    {
    public:
        Record* TryAcquire() noexcept
        {
            for (auto& slot : slots_) {
                if (slot.reserved) continue;
                slot.reserved = true;
                return &slot.record;
            }
            return nullptr;
        }

        bool Commit(Record* record) noexcept
        {
            for (auto& slot : slots_) {
                if (&slot.record != record) continue;
                if (!slot.reserved || slot.retained) return false;
                slot.retained = true;
                ++retainedCount_;
                return true;
            }
            return false;
        }

        void ReleaseUncommitted(Record* record) noexcept
        {
            for (auto& slot : slots_) {
                if (&slot.record == record && slot.reserved && !slot.retained) {
                    slot.reserved = false;
                    return;
                }
            }
        }

        std::size_t retainedCount() const noexcept { return retainedCount_; }
        static constexpr std::size_t capacity() noexcept { return Capacity; }

    private:
        struct Slot
        {
            Record record{};
            bool reserved{};
            bool retained{};
        };

        std::array<Slot, Capacity> slots_{};
        std::size_t retainedCount_{};
    };
}
