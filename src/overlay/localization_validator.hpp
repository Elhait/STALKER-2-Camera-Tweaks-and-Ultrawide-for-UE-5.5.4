#pragma once

#include "localization_catalog.hpp"

#include <span>
#include <string>
#include <vector>

namespace overlay
{
    class LocalizationValidator
    {
    public:
        static bool ValidateCanonical(const LocalizationCatalog& catalog,
            std::span<const loc::Key> inventory, std::string& error);
        static bool ValidateCompatible(const LocalizationCatalog& canonical,
            const LocalizationCatalog& candidate, std::string& error);
    };
}
