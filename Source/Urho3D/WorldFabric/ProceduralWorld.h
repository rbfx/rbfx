// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Scene/WorldPartition.h>

namespace Urho3D
{

struct URHO3D_API ProceduralWorldSettings
{
    unsigned seed{1};
    int minX{-8};
    int maxX{8};
    int minY{-8};
    int maxY{8};
    float cellSize{128.0f};
    float cellRadius{90.0f};
    unsigned memoryCost{1024};
    unsigned lodLevels{4};
    ea::string sceneTemplate{"Scenes/ProceduralCell.xml"};
};

struct URHO3D_API ProceduralCellInfo
{
    ea::string id;
    IntVector2 coordinates;
    unsigned long long seed{};
    unsigned lodLevel{};
    float height{};
};

/// Deterministic procedural cell generator feeding the production WorldPartition.
class URHO3D_API ProceduralWorldGenerator
{
public:
    static bool ValidateSettings(const ProceduralWorldSettings& settings, ea::string* error = nullptr);
    static bool Generate(WorldPartition& partition, const ProceduralWorldSettings& settings,
        ea::vector<ProceduralCellInfo>* generated = nullptr, ea::string* error = nullptr);
    static unsigned long long CellSeed(unsigned seed, int x, int y);
    static unsigned SelectLod(float distance, float cellSize, unsigned lodLevels);
};

} // namespace Urho3D
