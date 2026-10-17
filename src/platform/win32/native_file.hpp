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
            return OpenFile(path, write, false);
        }
        // Staging owns only a newly created name. An existing file, directory,
        // hard link or reparse point is never overwritten or removed on refusal.
        bool CreateExclusive(const wchar_t* path) noexcept
        {
            return OpenFile(path, true, true);
        }

    private:
        bool OpenFile(const wchar_t* path, bool write, bool exclusive) noexcept
        {
            const HANDLE next = CreateFileW(path, write ? GENERIC_WRITE : GENERIC_READ,
                FILE_SHARE_READ | (write ? 0 : FILE_SHARE_WRITE), nullptr,
                exclusive ? CREATE_NEW : (write ? OPEN_ALWAYS : OPEN_EXISTING),
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (next == INVALID_HANDLE_VALUE) return false;
            BY_HANDLE_FILE_INFORMATION info{};
            const bool regular = GetFileType(next) == FILE_TYPE_DISK &&
                GetFileInformationByHandle(next, &info) &&
                !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
                (!write || info.nNumberOfLinks == 1);
            if (!regular) {
                CloseHandle(next);
                SetLastError(ERROR_ACCESS_DENIED);
                return false;
            }
            // OPEN_ALWAYS does not truncate through a link before the handle
            // has been checked. Its sharing mode excludes concurrent writers.
            if (write && !SetEndOfFile(next)) {
                const DWORD error = GetLastError();
                CloseHandle(next);
                SetLastError(error);
                return false;
            }
            if (!Close()) {
                const DWORD error = GetLastError();
                CloseHandle(next);
                SetLastError(error);
                return false;
            }
            handle_ = next;
            return true;
        }

    public:
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
#if defined(NATIVE_FILE_TEST_WRITE)
                if (!NATIVE_FILE_TEST_WRITE(handle_, text.data(), count, &written, nullptr)) return false;
#else
                if (!WriteFile(handle_, text.data(), count, &written, nullptr)) return false;
#endif
                if (!written) { SetLastError(ERROR_WRITE_FAULT); return false; }
                text.remove_prefix(written);
            }
            return true;
        }
        bool FlushToDisk() noexcept { return FlushFileBuffers(handle_) != FALSE; }
        bool ReadAll(std::string& text)
        {
            if (!isOpen()) { SetLastError(ERROR_INVALID_HANDLE); return false; }
            constexpr std::size_t MaximumReadBytes = 4u * 1024u * 1024u;
            LARGE_INTEGER size{};
            if (!GetFileSizeEx(handle_, &size)) return false;
            if (size.QuadPart < 0 || size.QuadPart > MaximumReadBytes) {
                SetLastError(ERROR_FILE_TOO_LARGE);
                return false;
            }
            std::string result;
            std::array<char, 65536> buffer;
            for (;;) {
                DWORD read{};
                if (!ReadFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()),
                        &read, nullptr)) return false;
                if (!read) break;
                if (read > MaximumReadBytes - result.size()) {
                    SetLastError(ERROR_FILE_TOO_LARGE);
                    return false;
                }
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
