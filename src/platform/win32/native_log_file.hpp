#pragma once

#include "native_file.hpp"

namespace platform::win32
{
    // Overlapping game launches must not compete for one diagnostic file.
    // Keep the canonical path when available; use one reusable fallback slot
    // for sharing conflicts. Never overwrite a slot held by a live owner.
    // Configuration/staging NativeFile callers retain their strict semantics.
    inline bool OpenProcessLogFile(NativeFile& file, const wchar_t* path) noexcept
    {
        const bool canonicalOpened = file.Open(path, true);
        if (!canonicalOpened && GetLastError() != ERROR_SHARING_VIOLATION) return false;
        try {
            std::wstring fallback(path);
            if (fallback.ends_with(L".log")) fallback.resize(fallback.size() - 4);
            fallback += L".PID.log";
            if (canonicalOpened) {
                // Best effort only: a live NativeFile owner denies delete
                // sharing. Cleanup failure must not disable the canonical log.
                DeleteFileW(fallback.c_str());
                return true;
            }
            if (!file.Open(fallback.c_str(), true)) return false;
            OutputDebugStringW((L"STALKER2CameraTweaks: process log fallback: " +
                fallback + L"\n").c_str());
            return true;
        } catch (...) {
            if (canonicalOpened) return true;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return false;
        }
    }

    // Session-local limit: retain the beginning and one explicit marker, then
    // suppress further records without repeated I/O or logger-error recursion.
    class NativeLogFile final
    {
    public:
        explicit NativeLogFile(std::size_t maximumBytes = 16u * 1024u * 1024u) noexcept
            : maximumBytes_(maximumBytes) {}
        bool Open(const wchar_t* path, bool write = true) noexcept
        {
            if (!write || !OpenProcessLogFile(file_, path)) return false;
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
