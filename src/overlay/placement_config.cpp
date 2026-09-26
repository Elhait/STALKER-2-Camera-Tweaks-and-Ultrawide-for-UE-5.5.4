#include "placement_config.hpp"
#include "../config/config_repository.hpp"

bool overlay::placement_config::Load(const std::wstring& configPath,
    const std::wstring& legacyPath, float& positionX, float& positionY,
    bool& migrated)
{
    int x = 0;
    int y = 0;
    if (!config::LoadOverlayPlacement(configPath, legacyPath, x, y, migrated))
        return false;
    positionX = static_cast<float>(x);
    positionY = static_cast<float>(y);
    return true;
}

bool overlay::placement_config::Save(const std::wstring& configPath,
    int positionX, int positionY)
{
    return config::PersistOverlayPlacement(configPath, positionX, positionY);
}
