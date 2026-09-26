#include "localization_formatter.hpp"

#include <algorithm>
#include <cctype>

namespace overlay
{
    std::string LocalizationFormatter::Format(std::string_view format,
        const TextArguments& arguments)
    {
        std::string result;
        for (std::size_t i = 0; i < format.size();) {
            if (format[i] != '{') { result.push_back(format[i++]); continue; }
            const auto end = format.find('}', i + 1);
            if (end == std::string_view::npos) {
                result.append(format.substr(i));
                break;
            }
            const auto name = format.substr(i + 1, end - i - 1);
            const auto found = std::find_if(arguments.begin(), arguments.end(),
                [name](const auto& argument) { return argument.first == name; });
            if (found == arguments.end()) result.append(format.substr(i, end - i + 1));
            else result.append(found->second);
            i = end + 1;
        }
        return result;
    }

    bool LocalizationFormatter::Placeholders(std::string_view format,
        std::vector<std::string>& names, std::string& error)
    {
        names.clear();
        for (std::size_t i = 0; i < format.size();) {
            if (format[i] == '}') { error = "unmatched closing placeholder brace"; return false; }
            if (format[i] != '{') { ++i; continue; }
            const auto end = format.find('}', i + 1);
            if (end == std::string_view::npos) { error = "unclosed placeholder"; return false; }
            if (end == i + 1) { error = "empty placeholder"; return false; }
            for (auto j = i + 1; j < end; ++j) {
                const auto ch = static_cast<unsigned char>(format[j]);
                if (!(std::isalnum(ch) || ch == '_')) {
                    error = "placeholder names may contain only letters, digits and underscores";
                    return false;
                }
            }
            std::string name(format.substr(i + 1, end - i - 1));
            if (std::find(names.begin(), names.end(), name) == names.end())
                names.push_back(std::move(name));
            i = end + 1;
        }
        return true;
    }
}
