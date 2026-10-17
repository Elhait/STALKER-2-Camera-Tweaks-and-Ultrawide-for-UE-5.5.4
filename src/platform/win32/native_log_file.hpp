#pragma once

#include "native_file.hpp"

namespace platform::win32
{
    // Session-local limit: retain the beginning and one explicit marker, then
    // suppress further records without repeated I/O or logger-error recursion.
    class NativeLogFile final
    {
    public:
        explicit NativeLogFile(std::size_t maximumBytes = 16u * 1024u * 1024u) noexcept
            : maximumBytes_(maximumBytes) {}
        bool Open(const wchar_t* path, bool write = true) noexcept
        {
            if (!write || !file_.Open(path, true)) return false;
            written_ = 0;
            suppressed_ = false;
            return true;
        }
        bool Close() noexcept { return file_.Close(); }
        bool isOpen() const noexcept { return file_.isOpen(); }
        bool Write(std::string_view text) noexcept
        {
            if (!isOpen()) { SetLastError(ERROR_INVALID_HANDLE); return false; }
            if (suppressed_) return true;
            constexpr std::string_view marker =
                "\r\nLOG_LIMIT_REACHED: further records suppressed for this session.\r\n";
            const auto remaining = maximumBytes_ - written_;
            if (remaining < marker.size() || text.size() > remaining - marker.size()) {
                suppressed_ = true;
                return remaining < marker.size() || file_.Write(marker);
            }
            if (!file_.Write(text)) {
                // A failed write may already have delivered a prefix. Without
                // its exact length, stop rather than undercount and retry.
                suppressed_ = true;
                return false;
            }
            written_ += text.size();
            return true;
        }

    private:
        NativeFile file_;
        std::size_t maximumBytes_{};
        std::size_t written_{};
        bool suppressed_{};
    };
}
