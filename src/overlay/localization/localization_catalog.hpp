#pragma once

#include "localization_keys.hpp"

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <vector>

namespace overlay
{
    class LocalizationCatalog
    {
        struct TransparentStringHash
        {
            using is_transparent = void;
            std::size_t operator()(std::string_view value) const noexcept
            { return std::hash<std::string_view>{}(value); }
        };
    public:
        bool LoadJson(std::string_view json, std::string& error);
        std::string_view Find(loc::Key key) const;
        bool Contains(loc::Key key) const;
        std::size_t size() const noexcept { return entries_.size(); }
        bool loaded() const noexcept { return loaded_; }
        std::vector<std::string_view> Keys() const;

    private:
        std::unordered_map<std::string, std::string, TransparentStringHash,
            std::equal_to<>> entries_;
        bool loaded_{};
    };
}
