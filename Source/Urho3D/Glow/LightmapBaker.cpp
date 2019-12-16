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

// Embree includes must be first
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>
#define _SSIZE_T_DEFINED

#include "../Glow/EmbreeScene.h"
#include "../Glow/LightmapBaker.h"
#include "../Glow/LightmapCharter.h"
#include "../Glow/LightmapGeometryBaker.h"
#include "../Glow/LightmapTracer.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/View.h"
#include "../Graphics/Viewport.h"
#include "../Math/AreaAllocator.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/sort.h>

// TODO: Use thread pool?
#include <future>

namespace Urho3D
{

/// Implementation of lightmap baker.
struct LightmapBakerImpl
{
    /// Construct.
    LightmapBakerImpl(Context* context, const LightmapBakingSettings& settings, Scene* scene,
        const ea::vector<Node*>& lightReceivers, const ea::vector<Node*>& lightObstacles, const ea::vector<Node*>& lights)
        : context_(context)
        , settings_(settings)
        , scene_(scene)
        , charts_(GenerateLightmapCharts(lightReceivers, settings_.charting_))
        , bakingScenes_(GenerateLightmapGeometryBakingScenes(context_, charts_, settings_.geometryBaking_))
        , bakedGeometries_(BakeLightmapGeometries(bakingScenes_))
        , bakedDirect_(InitializeLightmapChartsBakedDirect(charts_))
        , bakedIndirect_(InitializeLightmapChartsBakedIndirect(charts_))
        , lights_(lights)
    {
        ApplyLightmapCharts(charts_, 0);
        embreeScene_ = CreateEmbreeScene(context_, lightObstacles);
    }

    /// Context.
    Context* context_{};

    /// Settings.
    const LightmapBakingSettings settings_;
    /// Scene.
    Scene* scene_{};
    /// Lightmap charts.
    LightmapChartVector charts_;
    /// Baking scenes.
    ea::vector<LightmapGeometryBakingScene> bakingScenes_;
    /// Baked geometries.
    ea::vector<LightmapChartBakedGeometry> bakedGeometries_;
    /// Baked direct lighting.
    ea::vector<LightmapChartBakedDirect> bakedDirect_;
    /// Baked indirect lighting.
    ea::vector<LightmapChartBakedIndirect> bakedIndirect_;
    /// Embree scene.
    SharedPtr<EmbreeScene> embreeScene_;

    /// Lights.
    ea::vector<Node*> lights_;

    /// Calculation: current lightmap index.
    unsigned currentLightmapIndex_{ M_MAX_UNSIGNED };
};

LightmapBaker::LightmapBaker(Context* context)
    : Object(context)
{
}

LightmapBaker::~LightmapBaker()
{
}

bool LightmapBaker::Initialize(const LightmapBakingSettings& settings, Scene* scene,
    const ea::vector<Node*>& lightReceivers, const ea::vector<Node*>& lightObstacles, const ea::vector<Node*>& lights)
{
    impl_ = ea::make_unique<LightmapBakerImpl>(context_, settings, scene, lightReceivers, lightObstacles, lights);
    return true;
}

unsigned LightmapBaker::GetNumLightmaps() const
{
    return impl_->charts_.size();
}

static void RandomDirection(Vector3& result)
{
    float  len;

    do
    {
        result.x_ = (Random(1.0f) * 2.0f - 1.0f);
        result.y_ = (Random(1.0f) * 2.0f - 1.0f);
        result.z_ = (Random(1.0f) * 2.0f - 1.0f);
        len = result.Length();

    } while (len > 1.0f);

    result /= len;
}

static void RandomHemisphereDirection(Vector3& result, const Vector3& normal)
{
    RandomDirection(result);

    if (result.DotProduct(normal) < 0)
        result = -result;
}

bool LightmapBaker::RenderLightmapGBuffer(unsigned index)
{
    if (index >= GetNumLightmaps())
        return false;

    impl_->currentLightmapIndex_ = index;

    return true;
}

bool LightmapBaker::BakeLightmap(LightmapBakedData& data)
{
    const LightmapChart& chart = impl_->charts_[impl_->currentLightmapIndex_];
    const LightmapGeometryBakingScene& bakingScene = impl_->bakingScenes_[impl_->currentLightmapIndex_];
    const LightmapChartBakedGeometry& bakedGeometry = impl_->bakedGeometries_[impl_->currentLightmapIndex_];
    LightmapChartBakedDirect& bakedDirect = impl_->bakedDirect_[impl_->currentLightmapIndex_];
    LightmapChartBakedIndirect& bakedIndirect = impl_->bakedIndirect_[impl_->currentLightmapIndex_];
    const int lightmapWidth = chart.allocator_.GetWidth();
    const int lightmapHeight = chart.allocator_.GetHeight();

    // Prepare output buffers
    data.lightmapSize_ = { lightmapWidth, lightmapHeight };
    data.backedLighting_.resize(lightmapWidth * lightmapHeight);
    ea::fill(data.backedLighting_.begin(), data.backedLighting_.end(), Color::WHITE);

    Vector3 lightDirection;
    for (Node* lightNode : impl_->lights_)
    {
        if (auto light = lightNode->GetComponent<Light>())
        {
            if (light->GetLightType() == LIGHT_DIRECTIONAL)
            {
                lightDirection = lightNode->GetWorldDirection();
                break;
            }
        }
    }
    const Vector3 lightRayDirection = -lightDirection.Normalized();

    // Calculate directional light
    BakeDirectionalLight(bakedDirect, bakedGeometry, *impl_->embreeScene_,
        { lightDirection, Color::WHITE }, impl_->settings_.tracing_);

    // Calculate indirect light.
    for (int i = 0; i < impl_->settings_.numIndirectSamples_; ++i)
        BakeIndirectLight(bakedIndirect, impl_->bakedDirect_, bakedGeometry, *impl_->embreeScene_, impl_->settings_.tracing_);

    // Post-process lighting.
    const unsigned numThreads = impl_->settings_.tracing_.numThreads_;
    bakedIndirect.NormalizeLight();
    //CalculateIndirectVariance(bakedIndirect, bakedGeometry, { 3, 1 }, numThreads);
    FilterIndirectLight(bakedIndirect, bakedGeometry, { 3, 1, 10.0f, 4.0f, 1.0f }, numThreads);
    FilterIndirectLight(bakedIndirect, bakedGeometry, { 3, 2, 0.25f, 4.0f, 1.0f }, numThreads);
    //FilterIndirectLight(bakedIndirect, bakedGeometry, { 2, 4, 0.25f, 4.0f, 1.0f }, numThreads);
    //FilterIndirectLight(bakedIndirect, bakedGeometry, { 2, 1, 10.0f, 0.0f, 10.0f }, numThreads);

    // Copy directional light into output
    for (unsigned i = 0; i < data.backedLighting_.size(); ++i)
    {
        const Vector3& directLight = bakedDirect.light_[i];
        const Vector4& indirectLight = bakedIndirect.light_[i];
        data.backedLighting_[i] = Color{ directLight.x_, directLight.y_, directLight.z_ };
        data.backedLighting_[i] += Color{ indirectLight.x_, indirectLight.y_, indirectLight.z_ };
    }

    return true;
}

}
