// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Glow/BakedLightCache.h"

namespace Urho3D
{

BakedLightCache::~BakedLightCache() = default;

void BakedLightMemoryCache::StoreBakedChunk(const IntVector3& chunk, BakedSceneChunk bakedChunk)
{
    bakedChunkCache_[chunk] = ea::make_shared<BakedSceneChunk>(ea::move(bakedChunk));
}

ea::shared_ptr<const BakedSceneChunk> BakedLightMemoryCache::LoadBakedChunk(const IntVector3& chunk)
{
    auto iter = bakedChunkCache_.find(chunk);
    return iter != bakedChunkCache_.end() ? iter->second : nullptr;
}

void BakedLightMemoryCache::StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect)
{
    directLightCache_[lightmapIndex] = ea::make_shared<LightmapChartBakedDirect>(ea::move(bakedDirect));
}

ea::shared_ptr<const LightmapChartBakedDirect> BakedLightMemoryCache::LoadDirectLight(unsigned lightmapIndex)
{
    auto iter = directLightCache_.find(lightmapIndex);
    return iter != directLightCache_.end() ? iter->second : nullptr;
}

void BakedLightMemoryCache::StoreLightmap(unsigned lightmapIndex, BakedLightmap bakedLightmap)
{
    lightmapCache_[lightmapIndex] = ea::make_shared<BakedLightmap>(ea::move(bakedLightmap));
}

ea::shared_ptr<const BakedLightmap> BakedLightMemoryCache::LoadLightmap(unsigned lightmapIndex)
{
    auto iter = lightmapCache_.find(lightmapIndex);
    return iter != lightmapCache_.end() ? iter->second : nullptr;
}

}
