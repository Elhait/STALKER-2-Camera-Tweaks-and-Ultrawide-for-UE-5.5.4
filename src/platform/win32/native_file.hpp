#pragma once

#include <Windows.h>

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace platform::win32
{
    // No CRT FILE registration: /MT stdio termination cannot flush/close this
    // handle after killing its owner thread. Callers serialize shared use.
    class NativeFile final
    {
    public:
        NativeFile() noexcept = default;
        NativeFile(const NativeFile&) = delete;
        NativeFile& operator=(const NativeFile&) = delete;
        ~NativeFile() { Close(); }

        bool Open(const wchar_t* path, bool write) noexcept
        {
            const HANDLE next = CreateFileW(path, write ? GENERIC_WRITE : GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                write ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (next == INVALID_HANDLE_VALUE) return false;
            if (!Close()) {
                const DWORD error = GetLastError();
                CloseHandle(next);
                SetLastError(error);
                return false;
            }
            handle_ = next;
            return true;
        }
        bool Close() noexcept
        {
            if (!isOpen()) return true;
            if (!CloseHandle(handle_)) return false;
            handle_ = INVALID_HANDLE_VALUE;
            return true;
        }
        bool isOpen() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }
        bool Write(std::string_view text) noexcept
        {
            if (!isOpen()) { SetLastError(ERROR_INVALID_HANDLE); return false; }
            while (!text.empty()) {
                const auto count = static_cast<DWORD>((std::min)(text.size(),
                    static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
                DWORD written{};
                if (!WriteFile(handle_, text.data(), count, &written, nullptr)) return false;
                if (!written) { SetLastError(ERROR_WRITE_FAULT); return false; }
                text.remove_prefix(written);
            }
            return true;
        }
        bool FlushToDisk() noexcept { return FlushFileBuffers(handle_) != FALSE; }
        bool ReadAll(std::string& text)
        {
            if (!isOpen()) { SetLastError(ERROR_INVALID_HANDLE); return false; }
            std::string result;
            std::array<char, 65536> buffer;
            for (;;) {
                DWORD read{};
                if (!ReadFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()),
                        &read, nullptr)) return false;
                if (!read) break;
                result.append(buffer.data(), read);
            }
            text = std::move(result);
            return true;
        }

    private:
        HANDLE handle_{INVALID_HANDLE_VALUE};
    };

    inline bool ReadNativeFile(const wchar_t* path, std::string& text)
    {
        NativeFile input;
        return input.Open(path, false) && input.ReadAll(text);
    }
}
