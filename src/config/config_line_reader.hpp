#pragma once

#include <istream>
#include <sstream>
#include "../platform/win32/native_file.hpp"
#include <string>
#include <utility>
#include <vector>

namespace config::detail
{
    struct ConfigTextFormat
    {
        std::string lineEnding{"\r\n"};
        bool trailingLineEnding{};
    };

    inline bool ReadConfigLines(std::istream& input,
        std::vector<std::string>& lines, ConfigTextFormat* format = nullptr)
    {
        bool lineEndingDetected = false;
        if (format) *format = {};
        std::string line;
        while (std::getline(input, line)) {
            const bool carriageReturn = !line.empty() && line.back() == '\r';
            if (format && !lineEndingDetected) {
                format->lineEnding = carriageReturn ? "\r\n" : "\n";
                lineEndingDetected = true;
            }
            if (carriageReturn) line.pop_back();
            if (format) format->trailingLineEnding = !input.eof();
            lines.push_back(std::move(line));
        }
        return !input.bad() && (input.eof() || !input.fail());
    }

    inline bool ReadConfigFile(const wchar_t* path,
        std::vector<std::string>& lines, ConfigTextFormat* format = nullptr)
    {
        std::string bytes;
        if (!platform::win32::ReadNativeFile(path, bytes)) return false;
        std::istringstream input(std::move(bytes));
        return ReadConfigLines(input, lines, format);
    }
}
