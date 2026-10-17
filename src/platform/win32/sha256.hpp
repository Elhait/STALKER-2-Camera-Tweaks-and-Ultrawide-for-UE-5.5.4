#pragma once

#include <filesystem>
#include <string>

namespace platform::win32
{
    bool ComputeSha256(const std::filesystem::path& path, std::string& result);

    namespace detail
    {
        using Sha256AllocationCheckpoint = void(*)(unsigned);
        bool ComputeSha256WithTestCheckpoint(const std::filesystem::path& path,
            std::string& result, Sha256AllocationCheckpoint checkpoint);
    }
}
