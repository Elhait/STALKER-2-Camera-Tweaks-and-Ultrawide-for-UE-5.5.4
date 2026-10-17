#include "sha256.hpp"

#include <array>
#include <utility>
#include <vector>
#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace platform::win32
{
    namespace
    {
        class ScopedFileHandle
        {
        public:
            explicit ScopedFileHandle(HANDLE value) noexcept : value_{value} {}
            ~ScopedFileHandle() { Close(); }
            ScopedFileHandle(const ScopedFileHandle&) = delete;
            ScopedFileHandle& operator=(const ScopedFileHandle&) = delete;

            HANDLE get() const noexcept { return value_; }
            bool valid() const noexcept { return value_ != INVALID_HANDLE_VALUE; }
            bool Close() noexcept
            {
                if (!valid()) return true;
                if (!CloseHandle(value_)) return false;
                value_ = INVALID_HANDLE_VALUE;
                return true;
            }

        private:
            HANDLE value_{INVALID_HANDLE_VALUE};
        };

        class ScopedAlgorithmHandle
        {
        public:
            ~ScopedAlgorithmHandle() { Close(); }
            ScopedAlgorithmHandle(const ScopedAlgorithmHandle&) = delete;
            ScopedAlgorithmHandle& operator=(const ScopedAlgorithmHandle&) = delete;
            ScopedAlgorithmHandle() = default;

            BCRYPT_ALG_HANDLE* put() noexcept { return &value_; }
            BCRYPT_ALG_HANDLE get() const noexcept { return value_; }
            bool Close() noexcept
            {
                if (!value_) return true;
                if (BCryptCloseAlgorithmProvider(value_, 0) != 0) return false;
                value_ = nullptr;
                return true;
            }

        private:
            BCRYPT_ALG_HANDLE value_{};
        };

        class ScopedHashHandle
        {
        public:
            ~ScopedHashHandle() { Close(); }
            ScopedHashHandle(const ScopedHashHandle&) = delete;
            ScopedHashHandle& operator=(const ScopedHashHandle&) = delete;
            ScopedHashHandle() = default;

            BCRYPT_HASH_HANDLE* put() noexcept { return &value_; }
            BCRYPT_HASH_HANDLE get() const noexcept { return value_; }
            PUCHAR AllocateStorage(ULONG size)
            {
                storage_.resize(size);
                return storage_.data();
            }
            bool Close() noexcept
            {
                if (!value_) return true;
                if (BCryptDestroyHash(value_) != 0) return false;
                value_ = nullptr;
                return true;
            }

        private:
            // The destructor closes the hash before members are destroyed.
            // BCrypt borrows this buffer until BCryptDestroyHash completes.
            std::vector<std::uint8_t> storage_;
            BCRYPT_HASH_HANDLE value_{};
        };
    }

    namespace
    {
    bool ComputeSha256Impl(const std::filesystem::path& path,
        std::string& result, detail::Sha256AllocationCheckpoint checkpoint)
    {
        try {
            ScopedFileHandle file{CreateFileW(path.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
            if (!file.valid()) return false;

            ScopedAlgorithmHandle algorithm;
            ScopedHashHandle hash;
            if (checkpoint) checkpoint(1);
            std::vector<std::uint8_t> buffer(1024 * 1024);
            std::array<std::uint8_t, 32> digest{};
            ULONG objectLength = 0;
            ULONG propertyLength = 0;
            bool success = false;
            if (BCryptOpenAlgorithmProvider(algorithm.put(),
                    BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
                BCryptGetProperty(algorithm.get(), BCRYPT_OBJECT_LENGTH,
                    reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength),
                    &propertyLength, 0) == 0 && objectLength != 0) {
                if (checkpoint) checkpoint(2);
                const auto hashObject = hash.AllocateStorage(objectLength);
                if (BCryptCreateHash(algorithm.get(), hash.put(),
                        hashObject, objectLength, nullptr, 0, 0) == 0) {
                    for (;;) {
                        DWORD count = 0;
                        if (!ReadFile(file.get(), buffer.data(),
                                static_cast<DWORD>(buffer.size()), &count, nullptr))
                            break;
                        if (count == 0) {
                            if (BCryptFinishHash(hash.get(), digest.data(),
                                    static_cast<ULONG>(digest.size()), 0) != 0)
                                break;
                            success = true;
                            break;
                        }
                        if (BCryptHashData(hash.get(), buffer.data(), count, 0) != 0)
                            break;
                    }
                }
            }

            std::string digestText;
            if (success) {
                static constexpr char digits[] = "0123456789ABCDEF";
                if (checkpoint) checkpoint(3);
                digestText.reserve(digest.size() * 2);
                for (const auto byte : digest) {
                    digestText.push_back(digits[byte >> 4]);
                    digestText.push_back(digits[byte & 0x0F]);
                }
            }
            const bool hashClosed = hash.Close();
            const bool algorithmClosed = algorithm.Close();
            const bool fileClosed = file.Close();
            if (!success || !hashClosed || !algorithmClosed || !fileClosed)
                return false;
            result.swap(digestText);
            return true;
        } catch (...) {
            return false;
        }
    }
    }

    bool ComputeSha256(const std::filesystem::path& path, std::string& result)
    {
        return ComputeSha256Impl(path, result, nullptr);
    }

    bool detail::ComputeSha256WithTestCheckpoint(
        const std::filesystem::path& path, std::string& result,
        detail::Sha256AllocationCheckpoint checkpoint)
    {
        return ComputeSha256Impl(path, result, checkpoint);
    }
}
