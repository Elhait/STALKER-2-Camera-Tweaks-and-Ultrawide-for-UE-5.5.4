#pragma once

#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace overlay
{
    enum class DxgiHookInstallStatus : std::uint8_t
    {
        Installed,
        InvalidArgument,
        ObjectUnreadable,
        DuplicateVtable,
        VtableUnreadable,
        NullMethod,
        ReplacementMismatch,
        RegionUnavailable,
        VtableOutsideRegion,
        ProtectionChangeFailed,
        ProtectionRestoreFailed,
    };

    // Capture SDK-defined entries, but patch only selected slots in the original
    // table. A foreign implementation may append private virtual methods: a
    // truncated clone would break those calls and its RTTI/internal layout.
    struct DxgiHookRecord
    {
        void** table{};
        std::vector<void*> original;
        std::vector<void*> replacement;
        std::atomic<bool> active{};
        std::uint32_t presentObservations{}; // guarded by discovery's evidence mutex
        std::uint32_t resizeObservations{};

        template <typename Function>
        Function Original(std::size_t slot) const noexcept
        {
            return slot < original.size()
                ? reinterpret_cast<Function>(original[slot]) : nullptr;
        }
    };

    class DxgiHookRegistry
    {
    public:
        using Lease = std::shared_ptr<DxgiHookRecord>;

        template <typename Prepare>
        bool Install(void* object, std::size_t methodCount,
            Prepare&& prepare, DxgiHookInstallStatus* status = nullptr)
        {
            const auto reject = [status](DxgiHookInstallStatus reason) {
                if (status) *status = reason;
                return false;
            };
            if (status) *status = DxgiHookInstallStatus::Installed;
            if (!object || methodCount < 3 || methodCount > 64)
                return reject(DxgiHookInstallStatus::InvalidArgument);
            std::scoped_lock lock{mutex_};
            void** table{};
            SIZE_T copied{};
            if (!ReadProcessMemory(GetCurrentProcess(), object, &table,
                    sizeof(table), &copied) || copied != sizeof(table) || !table)
                return reject(DxgiHookInstallStatus::ObjectUnreadable);
            const auto current = records_.load(std::memory_order_acquire);
            if (current && current->contains(table))
                return reject(DxgiHookInstallStatus::DuplicateVtable);
            auto record = std::make_shared<DxgiHookRecord>();
            record->table = table;
            record->original.resize(methodCount);
            const auto bytes = methodCount * sizeof(void*);
            if (!ReadProcessMemory(GetCurrentProcess(), table,
                    record->original.data(), bytes, &copied) || copied != bytes)
                return reject(DxgiHookInstallStatus::VtableUnreadable);
            if (std::any_of(record->original.begin(), record->original.end(),
                    [](void* method) { return method == nullptr; }))
                return reject(DxgiHookInstallStatus::NullMethod);
            record->replacement = record->original;
            prepare(*record);
            if (record->replacement.size() != methodCount)
                return reject(DxgiHookInstallStatus::ReplacementMismatch);
            MEMORY_BASIC_INFORMATION region{};
            if (VirtualQuery(table, &region, sizeof(region)) != sizeof(region)) {
                return reject(DxgiHookInstallStatus::RegionUnavailable);
            }
            const auto begin = reinterpret_cast<std::uintptr_t>(table);
            const auto regionEnd = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
            if (begin > regionEnd || bytes > regionEnd - begin) {
                return reject(DxgiHookInstallStatus::VtableOutsideRegion);
            }
            const bool executable = (region.Protect & (PAGE_EXECUTE |
                PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
            auto next = std::make_shared<RecordMap>(current ? *current : RecordMap{});
            next->emplace(table, record);
            DWORD protection{};
            if (!VirtualProtect(table, bytes,
                    executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &protection)) {
                return reject(DxgiHookInstallStatus::ProtectionChangeFailed);
            }
            // Publish the immutable originals before any dispatch slot can point
            // at our callback. Readers need no mutex and can always pass through
            // to the saved method, including during partial publication rollback.
            records_.store(std::move(next), std::memory_order_release);
            for (std::size_t slot = 0; slot < methodCount; ++slot) {
                if (record->replacement[slot] != record->original[slot])
                    InterlockedExchangePointer(
                        reinterpret_cast<void* volatile*>(table + slot),
                        record->replacement[slot]);
            }
            DWORD unused{};
            if (!VirtualProtect(table, bytes, protection, &unused)) {
                // Roll back dispatch, but retain originals for callbacks that
                // entered during publication and are waiting on this mutex.
                for (std::size_t slot = 0; slot < methodCount; ++slot) {
                    if (record->replacement[slot] != record->original[slot])
                        InterlockedExchangePointer(
                            reinterpret_cast<void* volatile*>(table + slot),
                            record->original[slot]);
                }
                VirtualProtect(table, bytes, protection, &unused);
                return reject(DxgiHookInstallStatus::ProtectionRestoreFailed);
            }
            record->active.store(true, std::memory_order_release);
            if (status) *status = DxgiHookInstallStatus::Installed;
            return true;
        }

        Lease Find(void* object) const noexcept
        {
            void** table{};
            SIZE_T copied{};
            if (!object || !ReadProcessMemory(GetCurrentProcess(), object, &table,
                    sizeof(table), &copied) || copied != sizeof(table)) return {};
            const auto records = records_.load(std::memory_order_acquire);
            if (!records) return {};
            const auto found = records->find(table);
            return found == records->end() ? Lease{} : found->second;
        }

    private:
        std::mutex mutex_;
        // Table records are process-resident like the installed export hooks;
        // they own no COM reference and never inspect destroyed object storage.
        using RecordMap = std::unordered_map<void**, Lease>;
        std::atomic<std::shared_ptr<const RecordMap>> records_{};
    };
}
