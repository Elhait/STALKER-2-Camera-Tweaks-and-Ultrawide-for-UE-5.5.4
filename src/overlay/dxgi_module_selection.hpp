#pragma once

#include <string_view>

namespace overlay
{
    inline bool IsExactSystemDxgiPath(std::wstring_view actual,
        std::wstring_view expected) noexcept
    {
        if (actual.size() != expected.size()) return false;
        for (std::size_t index = 0; index < actual.size(); ++index) {
            wchar_t left = actual[index] == L'/' ? L'\\' : actual[index];
            wchar_t right = expected[index] == L'/' ? L'\\' : expected[index];
            if (left >= L'A' && left <= L'Z') left += L'a' - L'A';
            if (right >= L'A' && right <= L'Z') right += L'a' - L'A';
            if (left != right) return false;
        }
        return true;
    }
}
