#pragma once

#include "native_file.hpp"

#include <spdlog/sinks/base_sink.h>
#include <mutex>

namespace platform::win32
{
    // Construct with spdlog::logger directly; do not register in spdlog's
    // process-static registry. The resident Runtime owns the production logger.
    class NativeLogSink final : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        explicit NativeLogSink(const wchar_t* path)
        {
            if (!file_.Open(path, true))
                throw spdlog::spdlog_ex("Native log open failed", GetLastError());
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& message) override
        {
            spdlog::memory_buf_t formatted;
            formatter_->format(message, formatted);
            if (!file_.Write({formatted.data(), formatted.size()}))
                throw spdlog::spdlog_ex("Native log write failed", GetLastError());
        }
        // WriteFile has already delivered every formatted record to Windows.
        // Like fflush, this does not promise persistence through power loss.
        void flush_() override {}

    private:
        NativeFile file_;
    };
}
