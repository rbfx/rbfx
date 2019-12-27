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

#pragma once

#include "../Glow/EmbreeScene.h"
#include "../Glow/LightmapGeometryBaker.h"
#include "../Glow/LightmapTracer.h"
#include "../Graphics/Light.h"
#include "../Math/Vector3.h"

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

/// Baked direct light description.
struct BakedDirectLight
{
    /// Light type.
    LightType lightType_{};
    /// Light mode.
    LightMode lightMode_{};
    /// Light color.
    Color lightColor_{};
    /// Position.
    Vector3 position_;
    /// Direction.
    Vector3 direction_;
    /// Rotation.
    Quaternion rotation_;
};

/// Lightmap chunk vicinity. Contains all required baking context from the chunk itself and adjacent chunks.
struct LightmapChunkVicinity
{
    /// Embree scene.
    SharedPtr<EmbreeScene> embreeScene_;
    /// Lights to bake.
    ea::vector<BakedDirectLight> bakedLights_;
};

/// Lightmap cache interface.
class URHO3D_API LightmapCache
{
public:
    /// Destruct.
    virtual ~LightmapCache();

    /// Store lightmap indices for chunk.
    virtual void StoreLightmapsForChunk(const IntVector3& chunk, ea::vector<unsigned> lightmapIndices) = 0;
    /// Load lightmap indices for chunk.
    virtual ea::vector<unsigned> LoadLightmapsForChunk(const IntVector3& chunk) = 0;

    /// Store chunk vicinity in the cache.
    virtual void StoreChunkVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) = 0;
    /// Load chunk vicinity.
    virtual const LightmapChunkVicinity* LoadChunkVicinity(const IntVector3& chunk) = 0;
    /// Release chunk vicinity.
    virtual void ReleaseChunkVicinity(const IntVector3& chunk) = 0;

    /// Store lightmap chart geometry buffer in the cache.
    virtual void StoreGeometryBuffer(unsigned lightmapIndex, LightmapChartGeometryBuffer geometryBuffer) = 0;
    /// Load geometry buffer.
    virtual const LightmapChartGeometryBuffer* LoadGeometryBuffer(unsigned lightmapIndex) = 0;
    /// Release geometry buffer.
    virtual void ReleaseGeometryBuffer(unsigned lightmapIndex) = 0;

    /// Store direct light for the lightmap chart.
    virtual void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) = 0;
    /// Load direct light for the lightmap chart.
    virtual LightmapChartBakedDirect* LoadDirectLight(unsigned lightmapIndex) = 0;
    /// Release direct light for the lightmap chart.
    virtual void ReleaseDirectLight(unsigned lightmapIndex) = 0;
};

/// Memory lightmap cache.
class URHO3D_API LightmapMemoryCache : public LightmapCache
{
public:
    /// Store lightmap indices for chunk.
    void StoreLightmapsForChunk(const IntVector3& chunk, ea::vector<unsigned> lightmapIndices) override;
    /// Load lightmap indices for chunk.
    ea::vector<unsigned> LoadLightmapsForChunk(const IntVector3& chunk) override;

    /// Store baking context in the cache.
    void StoreChunkVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) override;
    /// Load chunk vicinity.
    const LightmapChunkVicinity* LoadChunkVicinity(const IntVector3& chunk) override;
    /// Release chunk vicinity.
    void ReleaseChunkVicinity(const IntVector3& chunk) override;

    /// Store lightmap chart geometry buffer in the cache.
    void StoreGeometryBuffer(unsigned lightmapIndex, LightmapChartGeometryBuffer geometryBuffer) override;
    /// Load geometry buffer.
    const LightmapChartGeometryBuffer* LoadGeometryBuffer(unsigned lightmapIndex) override;
    /// Release geometry buffer.
    void ReleaseGeometryBuffer(unsigned lightmapIndex) override;

    /// Store direct light for the lightmap chart.
    void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) override;
    /// Load direct light for the lightmap chart.
    LightmapChartBakedDirect* LoadDirectLight(unsigned lightmapIndex) override;
    /// Release direct light for the lightmap chart.
    void ReleaseDirectLight(unsigned lightmapIndex) override;

private:
    /// Lightmap indices per chunk.
    ea::unordered_map<IntVector3, ea::vector<unsigned>> lightmapIndicesPerChunk_;
    /// Baking contexts cache.
    ea::unordered_map<IntVector3, LightmapChunkVicinity> chunkVicinityCache_;
    /// Geometry buffers cache.
    ea::unordered_map<unsigned, LightmapChartGeometryBuffer> geometryBufferCache_;
    /// Direct light cache.
    ea::unordered_map<unsigned, LightmapChartBakedDirect> directLightCache_;
};

}
