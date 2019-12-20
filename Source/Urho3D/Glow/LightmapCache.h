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
#include "../Math/Vector3.h"

namespace Urho3D
{

/// Lightmap chunk vicinity. Contains all required baking context from the chunk itself and adjacent chunks.
struct LightmapChunkVicinity
{
    /// Embree scene.
    SharedPtr<EmbreeScene> embreeScene_;
};

/// Lightmap cache interface.
class URHO3D_API LightmapCache
{
public:
    /// Store geometry buffers in the cache.
    virtual void StoreGeometryBuffer(const IntVector3& chunk, LightmapChartGeometryBufferVector geometryBuffer) = 0;
    /// Store baking context in the cache.
    virtual void StoreVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) = 0;
};

/// Memory lightmap cache.
class URHO3D_API LightmapMemoryCache : public LightmapCache
{
public:
    /// Store lightmap charts geometry buffers in the cache.
    void StoreGeometryBuffer(const IntVector3& chunk, LightmapChartGeometryBufferVector geometryBuffer) override;
    /// Store baking context in the cache.
    void StoreVicinity(const IntVector3& chunk, LightmapChunkVicinity vicinity) override;

private:
    /// Geometry buffers cache.
    ea::unordered_map<IntVector3, LightmapChartGeometryBufferVector> geometryBufferCache_;
    /// Baking contexts cache.
    ea::unordered_map<IntVector3, LightmapChunkVicinity> vicinityCache_;
};

}
