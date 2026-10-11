#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace overlay
{
    using TextArguments = std::vector<std::pair<std::string, std::string>>;

    class LocalizationFormatter
    {
    public:
        static std::string Format(std::string_view format,
            const TextArguments& arguments = {});
        static bool Placeholders(std::string_view format,
            std::vector<std::string>& names, std::string& error);
    };
}
