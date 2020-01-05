//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#include "../Glow/LightmapCache.h"

namespace Urho3D
{

LightmapCache::~LightmapCache() = default;

void LightmapMemoryCache::StoreLightmapsForChunk(const IntVector3& chunk, ea::vector<unsigned> lightmapIndices)
{
    lightmapIndicesPerChunk_[chunk] = ea::move(lightmapIndices);
}

ea::vector<unsigned> LightmapMemoryCache::LoadLightmapsForChunk(const IntVector3& chunk)
{
    auto iter = lightmapIndicesPerChunk_.find(chunk);
    if (iter != lightmapIndicesPerChunk_.end())
        return iter->second;
    return {};
}

void LightmapMemoryCache::StoreChunkVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity)
{
    chunkVicinityCache_[chunk] = ea::move(vicinity);
}

LightmapChunkVicinity* LightmapMemoryCache::LoadChunkVicinity(const IntVector3& chunk)
{
    auto iter = chunkVicinityCache_.find(chunk);
    return iter != chunkVicinityCache_.end() ? &iter->second : nullptr;
}

void LightmapMemoryCache::CommitLightProbeGroups(const IntVector3& /*chunk*/)
{
}

void LightmapMemoryCache::ReleaseChunkVicinity(const IntVector3& /*chunk*/)
{
    // Nothing to do
}

void LightmapMemoryCache::StoreGeometryBuffer(unsigned lightmapIndex, LightmapChartGeometryBuffer geometryBuffer)
{
    geometryBufferCache_[lightmapIndex] = ea::move(geometryBuffer);
}

const LightmapChartGeometryBuffer* LightmapMemoryCache::LoadGeometryBuffer(unsigned lightmapIndex)
{
    auto iter = geometryBufferCache_.find(lightmapIndex);
    return iter != geometryBufferCache_.end() ? &iter->second : nullptr;
}

void LightmapMemoryCache::ReleaseGeometryBuffer(unsigned /*lightmapIndex*/)
{
    // Nothing to do
}

void LightmapMemoryCache::StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect)
{
    directLightCache_[lightmapIndex] = ea::move(bakedDirect);
}

LightmapChartBakedDirect* LightmapMemoryCache::LoadDirectLight(unsigned lightmapIndex)
{
    auto iter = directLightCache_.find(lightmapIndex);
    return iter != directLightCache_.end() ? &iter->second : nullptr;
}

void LightmapMemoryCache::ReleaseDirectLight(unsigned /*lightmapIndex*/)
{
    // Nothing to do
}

}
