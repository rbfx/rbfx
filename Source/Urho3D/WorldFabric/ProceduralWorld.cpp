// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "ProceduralWorld.h"

namespace Urho3D
{

namespace
{

void Mix(unsigned long long& hash, unsigned value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        hash ^= static_cast<unsigned char>((value >> shift) & 0xff);
        hash *= 1099511628211ull;
    }
}

void SetError(ea::string* error, const ea::string& message)
{
    if (error)
        *error = message;
}

} // namespace

bool ProceduralWorldGenerator::ValidateSettings(const ProceduralWorldSettings& settings, ea::string* error)
{
    if (settings.minX > settings.maxX || settings.minY > settings.maxY)
    {
        SetError(error, "Procedural world coordinate ranges are inverted.");
        return false;
    }
    const unsigned long long width = static_cast<unsigned long long>(settings.maxX - settings.minX) + 1;
    const unsigned long long height = static_cast<unsigned long long>(settings.maxY - settings.minY) + 1;
    if (width * height > 1000000ull)
    {
        SetError(error, "Procedural world would create more than one million cells.");
        return false;
    }
    if (!(settings.cellSize > 0.0f) || !(settings.cellRadius > 0.0f) || settings.lodLevels == 0)
    {
        SetError(error, "Procedural world cell size, radius and LOD levels must be positive.");
        return false;
    }
    if (settings.sceneTemplate.empty())
    {
        SetError(error, "Procedural world scene template must not be empty.");
        return false;
    }
    return true;
}

unsigned long long ProceduralWorldGenerator::CellSeed(unsigned seed, int x, int y)
{
    unsigned long long hash = 1469598103934665603ull;
    Mix(hash, seed);
    Mix(hash, static_cast<unsigned>(x));
    Mix(hash, static_cast<unsigned>(y));
    return hash;
}

unsigned ProceduralWorldGenerator::SelectLod(float distance, float cellSize, unsigned lodLevels)
{
    if (lodLevels == 0 || !(cellSize > 0.0f))
        return 0;
    float ratio = Max(0.0f, distance) / cellSize;
    unsigned level = 0;
    while (ratio >= 2.0f && level + 1 < lodLevels)
    {
        ratio *= 0.5f;
        ++level;
    }
    return level;
}

bool ProceduralWorldGenerator::Generate(WorldPartition& partition, const ProceduralWorldSettings& settings,
    ea::vector<ProceduralCellInfo>* generated, ea::string* error)
{
    if (generated)
        generated->clear();
    if (!ValidateSettings(settings, error))
        return false;

    for (int y = settings.minY; y <= settings.maxY; ++y)
    {
        for (int x = settings.minX; x <= settings.maxX; ++x)
        {
            const ea::string id = "proc/" + ea::to_string(x) + "/" + ea::to_string(y);
            const unsigned long long cellSeed = CellSeed(settings.seed, x, y);
            const float height = static_cast<float>(cellSeed % 2001ull) / 100.0f - 10.0f;
            const Vector3 center(static_cast<float>(x) * settings.cellSize, height,
                static_cast<float>(y) * settings.cellSize);
            const unsigned lod = SelectLod(center.Length(), settings.cellSize, settings.lodLevels);

            StreamingCellDescriptor descriptor;
            descriptor.id = id;
            descriptor.coordinates = IntVector2(x, y);
            descriptor.center = center;
            descriptor.radius = settings.cellRadius;
            descriptor.scenePath = settings.sceneTemplate;
            descriptor.memoryCost = settings.memoryCost;

            const StreamingCell* existing = partition.GetCell(id);
            if (!existing)
            {
                if (!partition.AddCell(descriptor, error))
                    return false;
            }
            else if (existing->GetCoordinates() != descriptor.coordinates || existing->GetDescriptor().scenePath != descriptor.scenePath)
            {
                SetError(error, "Existing procedural cell conflicts with generated descriptor: " + id);
                return false;
            }

            if (generated)
            {
                ProceduralCellInfo info;
                info.id = id;
                info.coordinates = descriptor.coordinates;
                info.seed = cellSeed;
                info.lodLevel = lod;
                info.height = height;
                generated->push_back(info);
            }

            if (x == settings.maxX)
                break;
        }
        if (y == settings.maxY)
            break;
    }
    return true;
}

} // namespace Urho3D
