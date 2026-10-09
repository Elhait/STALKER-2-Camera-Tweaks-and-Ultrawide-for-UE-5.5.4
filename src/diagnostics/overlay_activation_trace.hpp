#pragma once

#include "diagnostic_runtime.hpp"

#include <Windows.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace diagnostics::overlay_activation_trace
{
    using LogFunction = void(*)(const std::string&);

    enum class Stage : std::uint8_t
    {
        VisibilityRequested,
        PresenterReadiness,
        FrameBegin,
        SemanticReadBegin,
        SemanticReadEnd,
        RenderSubmitted,
        FirstVisibleFrame,
    };

    namespace detail
    {
        inline std::atomic<std::uint64_t> nextRequest{1};
        inline std::atomic<std::uint64_t> requestAndStages{};

        inline std::uint32_t StageBit(Stage stage) noexcept
        {
            return 1u << static_cast<unsigned>(stage);
        }

        inline void Emit(LogFunction logger, std::uint64_t request,
            const char* event, bool visible, bool presenterReady,
            bool result) noexcept
        {
            if (!Enabled() || !logger || !request) return;
            LARGE_INTEGER qpc{};
            LARGE_INTEGER frequency{};
            if (!QueryPerformanceCounter(&qpc) ||
                !QueryPerformanceFrequency(&frequency)) return;
            char line[256]{};
            const int length = std::snprintf(line, sizeof(line),
                "OVERLAY_ACTIVATION_TRACE request=%llu event=%s qpc=%lld "
                "qpcFrequency=%lld tid=%lu visible=%u presenterReady=%u result=%u",
                static_cast<unsigned long long>(request), event,
                static_cast<long long>(qpc.QuadPart),
                static_cast<long long>(frequency.QuadPart),
                GetCurrentThreadId(), visible ? 1u : 0u,
                presenterReady ? 1u : 0u, result ? 1u : 0u);
            if (length <= 0 || static_cast<std::size_t>(length) >= sizeof(line)) return;
            try {
                logger(std::string(line, static_cast<std::size_t>(length)));
            } catch (...) {
                // Telemetry cannot escape a window or renderer callback.
            }
        }
    }

    inline std::uint64_t BeginRequest(LogFunction logger, bool visible,
        bool presenterReady) noexcept
    {
        if (!Enabled()) return 0;
        const auto request = detail::nextRequest.fetch_add(1,
            std::memory_order_acq_rel);
        detail::requestAndStages.store(request << 8, std::memory_order_release);
        detail::Emit(logger, request, "toggle_requested", visible,
            presenterReady, true);
        return request;
    }

    inline std::uint64_t CurrentRequest() noexcept
    {
        return Enabled()
            ? detail::requestAndStages.load(std::memory_order_acquire) >> 8 : 0;
    }

    inline bool RecordStage(LogFunction logger, std::uint64_t request,
        Stage stage, const char* event, bool visible, bool presenterReady,
        bool result = true) noexcept
    {
        if (!Enabled() || !request) return false;
        const auto requestToken = request << 8;
        const auto bit = static_cast<std::uint64_t>(detail::StageBit(stage));
        auto observed = detail::requestAndStages.load(std::memory_order_acquire);
        do {
            if ((observed & ~std::uint64_t{0xFF}) != requestToken ||
                (observed & bit)) return false;
        } while (!detail::requestAndStages.compare_exchange_weak(observed,
            observed | bit, std::memory_order_acq_rel,
            std::memory_order_acquire));
        detail::Emit(logger, request, event, visible, presenterReady, result);
        return true;
    }
}
