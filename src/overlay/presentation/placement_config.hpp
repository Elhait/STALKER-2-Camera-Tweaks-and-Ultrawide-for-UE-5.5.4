#pragma once

#include <string>

namespace overlay::placement_config
{
    bool Load(const std::wstring& configPath, const std::wstring& legacyPath,
        float& positionX, float& positionY, bool& migrated);
    bool Save(const std::wstring& configPath, int positionX, int positionY);
}
