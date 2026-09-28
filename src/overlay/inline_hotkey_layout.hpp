#pragma once

#include <cstddef>
#include <string_view>

namespace overlay::inline_hotkey_layout
{
    constexpr std::size_t LeadingPunctuationBytes(std::string_view text) noexcept
    {
        if (text.empty()) return 0;
        switch (text.front()) {
        case ',': case ';': case ':': case '!': case '?': case '.':
        case ')': case ']': case '}':
            return 1;
        default:
            return 0;
        }
    }

    constexpr std::string_view TrimLeadingWhitespace(std::string_view text) noexcept
    {
        std::size_t first = 0;
        while (first < text.size() &&
            (text[first] == ' ' || text[first] == '\t' ||
                text[first] == '\r' || text[first] == '\n'))
            ++first;
        return text.substr(first);
    }
}
