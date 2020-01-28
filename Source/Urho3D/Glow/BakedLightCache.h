//
// Copyright (c) 2017-2020 the rbfx project.
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

#pragma once

#include "../Glow/BakedSceneChunk.h"
#include "../Glow/RaytracerScene.h"
#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightTracer.h"
#include "../Graphics/Light.h"
#include "../Graphics/LightProbeGroup.h"
#include "../Math/Vector3.h"

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

/// Baked lightmap data.
struct BakedLightmap
{
    /// Construct default.
    BakedLightmap() = default;
    /// Construct.
    explicit BakedLightmap(unsigned lightmapSize)
        : lightmapSize_(lightmapSize)
        , lightmap_(lightmapSize_ * lightmapSize_)
    {}

    /// Size of lightmap.
    unsigned lightmapSize_;
    /// Lightmap data, in linear space.
    ea::vector<Vector3> lightmap_;
};

/// Lightmap cache interface.
class URHO3D_API BakedLightCache
{
public:
    /// Destruct.
    virtual ~BakedLightCache();

    /// Store baked scene chunk in the cache.
    virtual void StoreBakedChunk(const IntVector3& chunk, BakedSceneChunk bakedChunk) = 0;
    /// Load baked scene chunk.
    virtual ea::shared_ptr<const BakedSceneChunk> LoadBakedChunk(const IntVector3& chunk) = 0;

    /// Store direct light for the lightmap chart.
    virtual void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) = 0;
    /// Load direct light for the lightmap chart.
    virtual ea::shared_ptr<const LightmapChartBakedDirect> LoadDirectLight(unsigned lightmapIndex) = 0;

    /// Store baked lightmap.
    virtual void StoreLightmap(unsigned lightmapIndex, BakedLightmap bakedLightmap) = 0;
    /// Load baked lightmap.
    virtual ea::shared_ptr<const BakedLightmap> LoadLightmap(unsigned lightmapIndex) = 0;
};

/// Memory lightmap cache.
class URHO3D_API BakedLightMemoryCache : public BakedLightCache
{
public:
    /// Store baked scene chunk in the cache.
    void StoreBakedChunk(const IntVector3& chunk, BakedSceneChunk bakedChunk) override;
    /// Load baked scene chunk.
    ea::shared_ptr<const BakedSceneChunk> LoadBakedChunk(const IntVector3& chunk) override;

    /// Store direct light for the lightmap chart.
    void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) override;
    /// Load direct light for the lightmap chart.
    ea::shared_ptr<const LightmapChartBakedDirect> LoadDirectLight(unsigned lightmapIndex) override;

    /// Store baked lightmap.
    void StoreLightmap(unsigned lightmapIndex, BakedLightmap bakedLightmap) override;
    /// Load baked lightmap.
    ea::shared_ptr<const BakedLightmap> LoadLightmap(unsigned lightmapIndex) override;

private:
    /// Baking contexts cache.
    ea::unordered_map<IntVector3, ea::shared_ptr<const BakedSceneChunk>> bakedChunkCache_;
    /// Direct light cache.
    ea::unordered_map<unsigned, ea::shared_ptr<const LightmapChartBakedDirect>> directLightCache_;
    /// Baked lightmaps.
    ea::unordered_map<unsigned, ea::shared_ptr<const BakedLightmap>> lightmapCache_;
};

}
