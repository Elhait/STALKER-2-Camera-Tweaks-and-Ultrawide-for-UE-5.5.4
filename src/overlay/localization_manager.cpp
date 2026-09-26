#include "localization_manager.hpp"
#include "localization_validator.hpp"

#include <algorithm>
#include <utility>

namespace overlay
{
    namespace
    {
        bool LoadEmbeddedCatalog(HMODULE resourceModule, WORD resourceId,
            LocalizationCatalog& catalog, std::string& error)
        {
            if (!resourceModule || resourceId == 0) {
                error = "embedded locale resource owner is unavailable";
                return false;
            }
            const HRSRC resource = FindResourceW(resourceModule,
                MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10));
            if (!resource) {
                error = "embedded locale resource was not found";
                return false;
            }
            const DWORD size = SizeofResource(resourceModule, resource);
            if (size == 0 || size > 1024 * 1024) {
                error = "embedded locale resource size is invalid";
                return false;
            }
            const HGLOBAL loaded = LoadResource(resourceModule, resource);
            const void* bytes = loaded ? LockResource(loaded) : nullptr;
            if (!bytes) {
                error = "embedded locale resource could not be read";
                return false;
            }
            return catalog.LoadJson(std::string_view(
                static_cast<const char*>(bytes), static_cast<std::size_t>(size)), error);
        }
    }

    bool LocalizationManager::Initialize(std::string_view localeCode,
        HMODULE resourceModule, std::string& error)
    {
        if (initializationAttempted_) return ready_;
        initializationAttempted_ = true;
        const auto* canonical = localization::CanonicalLocaleDescriptor();
        if (!canonical) {
            error = "locale registry has no canonical catalog";
            return false;
        }

        catalogs_.reserve(localization::LocaleRegistry.size());
        for (const auto& descriptor : localization::LocaleRegistry) {
            LoadedCatalog loaded{&descriptor, {}};
            if (!LoadEmbeddedCatalog(resourceModule, descriptor.embeddedResourceId,
                    loaded.catalog, error)) {
                catalogs_.clear();
                return false;
            }
            catalogs_.push_back(std::move(loaded));
        }

        const auto findCatalog = [this](const localization::LocaleDescriptor* descriptor)
            -> const LocalizationCatalog* {
            const auto found = std::find_if(catalogs_.begin(), catalogs_.end(),
                [descriptor](const LoadedCatalog& loaded) {
                    return loaded.descriptor == descriptor;
                });
            return found == catalogs_.end() ? nullptr : &found->catalog;
        };
        const auto* canonicalCatalog = findCatalog(canonical);
        if (!canonicalCatalog || !LocalizationValidator::ValidateCanonical(
                *canonicalCatalog, loc::Inventory(), error)) {
            catalogs_.clear();
            return false;
        }
        for (const auto& loaded : catalogs_) {
            if (loaded.descriptor == canonical) continue;
            if (!LocalizationValidator::ValidateCompatible(*canonicalCatalog,
                    loaded.catalog, error)) {
                catalogs_.clear();
                return false;
            }
        }

        ready_ = true;
        activeCatalog_ = canonicalCatalog;
        localeCode_ = canonical->code;
        SetLocale(localeCode);
        return ready_;
    }

    bool LocalizationManager::SetLocale(std::string_view localeCode) noexcept
    {
        if (!ready_ && initializationAttempted_) return false;
        const auto* descriptor = localization::FindLocaleDescriptor(localeCode);
        if (!descriptor) return false;
        const auto found = std::find_if(catalogs_.begin(), catalogs_.end(),
            [descriptor](const LoadedCatalog& loaded) {
                return loaded.descriptor == descriptor;
            });
        if (found == catalogs_.end()) return false;
        localeCode_ = descriptor->code;
        activeCatalog_ = &found->catalog;
        return true;
    }

    std::string LocalizationManager::Text(loc::Key key) const
    {
        const auto value = ready_ && activeCatalog_
            ? activeCatalog_->Find(key) : std::string_view{};
        return value.empty() ? "[missing: " + std::string(key.path) + "]" : std::string(value);
    }

    std::string LocalizationManager::Format(loc::Key key,
        const TextArguments& arguments) const
    { return LocalizationFormatter::Format(Text(key), arguments); }
}
