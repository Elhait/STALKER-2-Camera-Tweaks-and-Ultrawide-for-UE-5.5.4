#include "localization_validator.hpp"
#include "localization_formatter.hpp"

#include <algorithm>
#include <unordered_set>

namespace overlay
{
    bool LocalizationValidator::ValidateCanonical(const LocalizationCatalog& catalog,
        std::span<const loc::Key> inventory, std::string& error)
    {
        if (!catalog.loaded()) { error = "canonical catalog is not loaded"; return false; }
        if (catalog.size() != inventory.size()) {
            error = "catalog key count does not match canonical inventory";
            return false;
        }
        std::unordered_set<std::string_view> expected;
        for (const auto key : inventory) {
            if (!expected.emplace(key.path).second) {
                error = "duplicate key in canonical inventory: " + std::string(key.path);
                return false;
            }
            if (!catalog.Contains(key)) { error = "missing catalog key: " + std::string(key.path); return false; }
            std::vector<std::string> placeholders;
            if (!LocalizationFormatter::Placeholders(catalog.Find(key), placeholders, error)) {
                error = std::string(key.path) + ": " + error;
                return false;
            }
        }
        for (const auto catalogKey : catalog.Keys()) {
            if (!expected.contains(catalogKey)) {
                error = "obsolete catalog key: " + std::string(catalogKey);
                return false;
            }
        }
        return true;
    }

    bool LocalizationValidator::ValidateCompatible(const LocalizationCatalog& canonical,
        const LocalizationCatalog& candidate, std::string& error)
    {
        if (!canonical.loaded() || !candidate.loaded() || canonical.size() != candidate.size()) {
            error = "candidate catalog key set differs from canonical catalog";
            return false;
        }
        for (const auto key : canonical.Keys()) {
            const auto canonicalText = canonical.Find({key});
            const auto candidateText = candidate.Find({key});
            if (candidateText.empty()) { error = "missing candidate key: " + std::string(key); return false; }
            std::vector<std::string> expected, actual;
            if (!LocalizationFormatter::Placeholders(canonicalText, expected, error) ||
                !LocalizationFormatter::Placeholders(candidateText, actual, error)) return false;
            std::sort(expected.begin(), expected.end());
            std::sort(actual.begin(), actual.end());
            if (expected != actual) { error = "placeholder mismatch for key: " + std::string(key); return false; }
        }
        return true;
    }
}
