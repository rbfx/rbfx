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

#include "../Scene/Scene.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Light;
class Model;
class StaticModel;
struct LightmapBakerImpl;

/// Lightmap baked data.
struct LightmapBakedData
{
    /// Lightmap size.
    IntVector2 lightmapSize_{};
    /// Backed lighting data.
    ea::vector<Color> backedLighting_;
};

/// Lightmap baking settings.
struct LightmapBakingSettings
{
    /// Lightmap size.
    unsigned lightmapSize_{ 1024 };
    /// Lightmap padding.
    unsigned lightmapPadding_{ 2 };
    /// Texel density.
    unsigned texelDensity_{ 10 };
    /// Min scale factor for node lightmaps.
    float minLightmapScale_{ 1.0f };
    /// Number of parallel chunks.
    unsigned numParallelChunks_{ 32 };
    /// Baking render path.
    ea::string bakingRenderPath_{ "RenderPaths/LightmapGBuffer.xml" };
    /// Baking materials.
    ea::string bakingMaterial_{ "Materials/LightmapBaker.xml" };
};

/// Lightmap baker API.
class URHO3D_API LightmapBaker : public Object
{
    URHO3D_OBJECT(LightmapBaker, Object);

public:
    /// Construct.
    LightmapBaker(Context* context);
    /// Destruct.
    ~LightmapBaker() override;

    /// Initialize. Children nodes are ignored. Scene must stay immutable until the end. Must be called from rendering thread.
    bool Initialize(const LightmapBakingSettings& settings, Scene* scene,
        const ea::vector<Node*>& lightReceivers, const ea::vector<Node*>& lightObstacles, const ea::vector<Node*>& lights);
    /// Cook raytracing scene. May be called from working thread.
    void CookRaytracingScene();

    /// Return number of lightmaps.
    unsigned GetNumLightmaps() const;

    /// Build photon map.
    bool BuildPhotonMap();
    /// Render lightmap G-Buffer. Must be called from rendering thread.
    bool RenderLightmapGBuffer(unsigned index);
    /// Bake lightmap.
    bool BakeLightmap(LightmapBakedData& data);

    /// Append lightmaps to scene and apply parameters to nodes.
    void ApplyLightmapsToScene(unsigned baseLightmapIndex);

private:
    /// Process rows of current image in multiple threads.
    template <class T>
    void ParallelForEachRow(T callback);

    /// Impl.
    ea::unique_ptr<LightmapBakerImpl> impl_;
};

}
