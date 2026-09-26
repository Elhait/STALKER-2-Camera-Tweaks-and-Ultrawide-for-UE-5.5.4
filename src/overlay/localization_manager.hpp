#pragma once

#include "localization_catalog.hpp"
#include "localization_formatter.hpp"
#include "../localization/locale_registry.hpp"

#include <Windows.h>

#include <string>
#include <string_view>
#include <vector>

namespace overlay
{
    class LocalizationManager
    {
    public:
        bool Initialize(std::string_view localeCode, HMODULE resourceModule,
            std::string& error);
        bool SetLocale(std::string_view localeCode) noexcept;
        std::string Text(loc::Key key) const;
        std::string Format(loc::Key key,
            const TextArguments& arguments = {}) const;
        std::string_view localeCode() const noexcept { return localeCode_; }
        const auto& locales() const noexcept { return localization::LocaleRegistry; }
        bool initialized() const noexcept { return initializationAttempted_; }
        bool ready() const noexcept { return ready_; }

    private:
        struct LoadedCatalog
        {
            const localization::LocaleDescriptor* descriptor{};
            LocalizationCatalog catalog;
        };

        std::string_view localeCode_{localization::CanonicalLocaleCode};
        std::vector<LoadedCatalog> catalogs_;
        const LocalizationCatalog* activeCatalog_{};
        bool initializationAttempted_{};
        bool ready_{};
    };
}
