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

#include "../Glow/RaytracerScene.h"
#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightTracer.h"
#include "../Graphics/Light.h"
#include "../Graphics/LightProbeGroup.h"
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
    /// Lightmaps ownder by this chunk.
    ea::vector<unsigned> lightmaps_;
    /// Raytracer scene.
    SharedPtr<RaytracerScene> raytracerScene_;
    /// Geometry buffer ID to raytracer geometry ID mapping.
    ea::vector<unsigned> geometryBufferToRaytracer_;
    /// Lights to bake.
    ea::vector<BakedDirectLight> bakedLights_;
    /// Light probes collection.
    LightProbeCollection lightProbesCollection_;
};

/// Lightmap cache interface.
class URHO3D_API BakedLightCache
{
public:
    /// Destruct.
    virtual ~BakedLightCache();

    /// Store chunk vicinity in the cache.
    virtual void StoreChunkVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) = 0;
    /// Load chunk vicinity.
    virtual ea::shared_ptr<LightmapChunkVicinity> LoadChunkVicinity(const IntVector3& chunk) = 0;
    /// Called after light probes are updated.
    virtual void CommitLightProbeGroups(const IntVector3& chunk) = 0;

    /// Store lightmap chart geometry buffer in the cache.
    virtual void StoreGeometryBuffer(unsigned lightmapIndex, LightmapChartGeometryBuffer geometryBuffer) = 0;
    /// Load geometry buffer.
    virtual ea::shared_ptr<const LightmapChartGeometryBuffer> LoadGeometryBuffer(unsigned lightmapIndex) = 0;

    /// Store direct light for the lightmap chart.
    virtual void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) = 0;
    /// Load direct light for the lightmap chart.
    virtual ea::shared_ptr<LightmapChartBakedDirect> LoadDirectLight(unsigned lightmapIndex) = 0;
};

/// Memory lightmap cache.
class URHO3D_API BakedLightMemoryCache : public BakedLightCache
{
public:
    /// Store baking context in the cache.
    void StoreChunkVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) override;
    /// Load chunk vicinity.
    ea::shared_ptr<LightmapChunkVicinity> LoadChunkVicinity(const IntVector3& chunk) override;
    /// Called after light probes are updated.
    void CommitLightProbeGroups(const IntVector3& chunk) override;

    /// Store lightmap chart geometry buffer in the cache.
    void StoreGeometryBuffer(unsigned lightmapIndex, LightmapChartGeometryBuffer geometryBuffer) override;
    /// Load geometry buffer.
    ea::shared_ptr<const LightmapChartGeometryBuffer> LoadGeometryBuffer(unsigned lightmapIndex) override;

    /// Store direct light for the lightmap chart.
    void StoreDirectLight(unsigned lightmapIndex, LightmapChartBakedDirect bakedDirect) override;
    /// Load direct light for the lightmap chart.
    ea::shared_ptr<LightmapChartBakedDirect> LoadDirectLight(unsigned lightmapIndex) override;

private:
    /// Baking contexts cache.
    ea::unordered_map<IntVector3, ea::shared_ptr<LightmapChunkVicinity>> chunkVicinityCache_;
    /// Geometry buffers cache.
    ea::unordered_map<unsigned, ea::shared_ptr<const LightmapChartGeometryBuffer>> geometryBufferCache_;
    /// Direct light cache.
    ea::unordered_map<unsigned, ea::shared_ptr<LightmapChartBakedDirect>> directLightCache_;
};

}
