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

/// Size of Embree ray packed.
static const unsigned RayPacketSize = 16;

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
        , bakedLighting_(InitializeLightmapChartsBakedLighting(charts_))
        , embreeScene_(CreateEmbreeScene(context_, lightObstacles))
        , lights_(lights)
    {
    }
    /// Validate settings and whatever.
    bool Validate() const
    {
        if (settings_.charting_.chartSize_ % settings_.numParallelChunks_ != 0)
            return false;
        if (settings_.charting_.chartSize_ % RayPacketSize != 0)
            return false;
        return true;
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
    /// Baked lighting.
    ea::vector<LightmapChartBakedLighting> bakedLighting_;
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
    if (!impl_->Validate())
        return false;

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

template <class T>
void LightmapBaker::ParallelForEachRow(T callback)
{
    const LightmapChart& chart = impl_->charts_[impl_->currentLightmapIndex_];
    const unsigned lightmapHeight = static_cast<unsigned>(chart.allocator_.GetHeight());
    const unsigned chunkHeight = lightmapHeight / impl_->settings_.numParallelChunks_;

    // Start tasks
    ea::vector<std::future<void>> tasks;
    for (unsigned parallelChunkIndex = 0; parallelChunkIndex < impl_->settings_.numParallelChunks_; ++parallelChunkIndex)
    {
        tasks.push_back(std::async([=]()
        {
            const unsigned fromY = parallelChunkIndex * chunkHeight;
            const unsigned toY = ea::min((parallelChunkIndex + 1) * chunkHeight, lightmapHeight);
            for (unsigned y = fromY; y < toY; ++y)
                callback(y);
        }));
    }

    // Wait for async tasks
    for (auto& task : tasks)
        task.wait();
}

bool LightmapBaker::BakeLightmap(LightmapBakedData& data)
{
    const LightmapChart& chart = impl_->charts_[impl_->currentLightmapIndex_];
    const LightmapGeometryBakingScene& bakingScene = impl_->bakingScenes_[impl_->currentLightmapIndex_];
    const LightmapChartBakedGeometry& bakedGeometry = impl_->bakedGeometries_[impl_->currentLightmapIndex_];
    LightmapChartBakedLighting& bakedLighting = impl_->bakedLighting_[impl_->currentLightmapIndex_];
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

    // Calculate directional lighting
    BakeDirectionalLight(bakedLighting, bakedGeometry, *impl_->embreeScene_,
        { lightDirection, Color::WHITE }, impl_->settings_.tracing_);

    // Copy directional lighting into output
    for (unsigned i = 0; i < data.backedLighting_.size(); ++i)
    {
        const Vector3& directLighting = bakedLighting.directLighting_[i];
        data.backedLighting_[i] = Color{ directLighting.x_, directLighting.y_, directLighting.z_ };
    }

    // Process rows in multiple threads
    const unsigned numBounces = 1;
    const unsigned numRayPackets = static_cast<unsigned>(lightmapWidth / RayPacketSize);
    ParallelForEachRow([&](unsigned y)
    {
        RTCScene scene = impl_->embreeScene_->GetEmbreeScene();

    #if 0
        alignas(64) RTCRayHit16 rayHit16;
        alignas(64) int rayValid[RayPacketSize];
        float diffuseLight[RayPacketSize];
        float indirectLight[RayPacketSize];
        ea::fixed_vector<KDNearestNeighbour, 128> nearestPhotons;
    #endif
        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        for (unsigned rayPacketIndex = 0; rayPacketIndex < numRayPackets; ++rayPacketIndex)
        {
            const unsigned fromX = rayPacketIndex * RayPacketSize;
            const unsigned baseIndex = y * lightmapWidth + fromX;

            for (unsigned i = 0; i < RayPacketSize; ++i)
            {
                const unsigned index = baseIndex + i;
                const Vector3 position = bakedGeometry.geometryPositions_[index];
                const Vector3 smoothNormal = bakedGeometry.smoothNormals_[index];
                const unsigned geometryId = bakedGeometry.geometryIds_[index];

                if (!geometryId)
                    continue;

                float indirectLighting = 0.0f;
                Vector3 currentPosition = position;
                Vector3 currentNormal = smoothNormal;
                for (unsigned j = 0; j < numBounces; ++j)
                {
                    // Get new ray direction
                    Vector3 rayDirection;
                    RandomHemisphereDirection(rayDirection, currentNormal);

                    rayHit.ray.org_x = currentPosition.x_;
                    rayHit.ray.org_y = currentPosition.y_;
                    rayHit.ray.org_z = currentPosition.z_;
                    rayHit.ray.dir_x = rayDirection.x_;
                    rayHit.ray.dir_y = rayDirection.y_;
                    rayHit.ray.dir_z = rayDirection.z_;
                    rayHit.ray.tnear = 0.0f;
                    rayHit.ray.tfar = impl_->embreeScene_->GetMaxDistance() * 2;
                    rayHit.ray.time = 0.0f;
                    rayHit.ray.id = 0;
                    rayHit.ray.mask = 0xffffffff;
                    rayHit.ray.flags = 0xffffffff;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(scene, &rayContext, &rayHit);

                    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                        continue;

                    // Cast direct ray
                    rayHit.ray.org_x += rayHit.ray.dir_x * rayHit.ray.tfar;
                    rayHit.ray.org_y += rayHit.ray.dir_y * rayHit.ray.tfar;
                    rayHit.ray.org_z += rayHit.ray.dir_z * rayHit.ray.tfar;
                    rayHit.ray.dir_x = lightRayDirection.x_;
                    rayHit.ray.dir_y = lightRayDirection.y_;
                    rayHit.ray.dir_z = lightRayDirection.z_;
                    rayHit.ray.tnear = 0.0f;
                    rayHit.ray.tfar = impl_->embreeScene_->GetMaxDistance() * 2;
                    rayHit.ray.time = 0.0f;
                    rayHit.ray.id = 0;
                    rayHit.ray.mask = 0xffffffff;
                    rayHit.ray.flags = 0xffffffff;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(scene, &rayContext, &rayHit);

                    if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
                        continue;

                    const float incoming = 1.0f;
                    const float probability = 1 / (2 * M_PI);
                    const float cosTheta = rayDirection.DotProduct(currentNormal);
                    const float reflectance = 1 / M_PI;
                    const float brdf = reflectance / M_PI;

                    indirectLighting = incoming * brdf * cosTheta / probability;
                }

                data.backedLighting_[index] += Color::WHITE * indirectLighting;
            }
        }
    });

    // Gauss pre-pass
    {
        IntVector2 offsets[9];
        for (int i = 0; i < 9; ++i)
            offsets[i] = IntVector2(i % 3 - 1, i / 3 - 1);
        float kernel[9];
        int kernel1D[3] = { 1, 2, 1 };
        for (int i = 0; i < 9; ++i)
            kernel[i] = kernel1D[i % 3] * kernel1D[i / 3] / 16.0f;

        auto colorCopy = data.backedLighting_;
        ParallelForEachRow([&](unsigned y)
        {
            for (unsigned x = 0; x < lightmapWidth; ++x)
            {
                const IntVector2 sourceTexel{ static_cast<int>(x), static_cast<int>(y) };
                const IntVector2 minOffset = -sourceTexel;
                const IntVector2 maxOffset = IntVector2{ lightmapWidth, lightmapHeight } - sourceTexel - IntVector2::ONE;

                const unsigned index = y * lightmapWidth + x;

                const unsigned geometryId = bakedGeometry.geometryIds_[index];
                if (!geometryId)
                    continue;

                float weightSum = 0.0f;
                Vector4 colorSum;
                for (unsigned i = 0; i < 9; ++i)
                {
                    const IntVector2 offset = offsets[i];
                    if (offset.x_ < minOffset.x_ || offset.x_ > maxOffset.x_ || offset.y_ < minOffset.y_ || offset.y_ > maxOffset.y_)
                        continue;

                    const unsigned otherIndex = index + offset.y_ * lightmapWidth + offset.x_;
                    const Vector4 otherColor = colorCopy[otherIndex].ToVector4();
                    const Vector3 otherPosition = bakedGeometry.geometryPositions_[otherIndex];
                    const unsigned otherGeometryId = bakedGeometry.geometryIds_[otherIndex];
                    if (!otherGeometryId)
                        continue;

                    colorSum += otherColor * kernel[i];
                    weightSum += kernel[i];
                }

                const Vector4 result = colorSum / weightSum;
                data.backedLighting_[index] = Color(result.x_, result.y_, result.z_, result.w_);
            }
        });
    }

    IntVector2 offsets[25];
    for (int i = 0; i < 25; ++i)
        offsets[i] = IntVector2(i % 5 - 2, i / 5 - 2);
    float kernel[25];
    int kernel1D[5] = { 1, 4, 6, 4, 1 };
    for (int i = 0; i < 25; ++i)
        kernel[i] = kernel1D[i % 5] * kernel1D[i / 5] / 256.0f;

    const float colorPhi = 1.0f;
    const float normalPhi = 4.0f;
    const float positionPhi = 1.0f;

    for (unsigned passIndex = 0; passIndex < 3; ++passIndex)
    {
        const int offsetScale = 1 << passIndex;
        const float colorPhiScaled = colorPhi / offsetScale;
        auto colorCopy = data.backedLighting_;
        ParallelForEachRow([&](unsigned y)
        {
            for (unsigned x = 0; x < lightmapWidth; ++x)
            {
                const IntVector2 sourceTexel{ static_cast<int>(x), static_cast<int>(y) };
                const IntVector2 minOffset = -sourceTexel;
                const IntVector2 maxOffset = IntVector2{ lightmapWidth, lightmapHeight } - sourceTexel - IntVector2::ONE;

                const unsigned index = y * lightmapWidth + x;

                const Vector4 baseColor = colorCopy[index].ToVector4();
                const Vector3 basePosition = bakedGeometry.geometryPositions_[index];
                const Vector3 baseNormal = bakedGeometry.smoothNormals_[index];
                const unsigned geometryId = bakedGeometry.geometryIds_[index];
                if (!geometryId)
                    continue;

                Vector4 colorSum;
                float weightSum = 0.0f;
                for (unsigned i = 0; i < 25; ++i)
                {
                    const IntVector2 offset = offsets[i] * offsetScale;
                    const IntVector2 clampedOffset = VectorMax(minOffset, VectorMin(offset, maxOffset));
                    const unsigned otherIndex = index + clampedOffset.y_ * lightmapWidth + clampedOffset.x_;

                    const Vector4 otherColor = colorCopy[otherIndex].ToVector4();
                    const Vector4 colorDelta = baseColor - otherColor;
                    const float colorDeltaSquared = colorDelta.DotProduct(colorDelta);
                    const float colorWeight = ea::min(std::exp(-colorDeltaSquared / colorPhiScaled), 1.0f);

                    const Vector3 otherPosition = bakedGeometry.geometryPositions_[otherIndex];
                    const Vector3 positionDelta = basePosition - otherPosition;
                    const float positionDeltaSquared = positionDelta.DotProduct(positionDelta);
                    const float positionWeight = ea::min(std::exp(-positionDeltaSquared / positionPhi), 1.0f);

                    const Vector3 otherNormal = bakedGeometry.smoothNormals_[otherIndex];
                    const float normalDeltaCos = ea::max(0.0f, baseNormal.DotProduct(otherNormal));
                    const float normalWeight = std::pow(normalDeltaCos, normalPhi);

                    const float weight = colorWeight * positionWeight * normalWeight * kernel[i];
                    colorSum += otherColor * weight;
                    weightSum += weight;
                }

                const Vector4 result = colorSum / weightSum;
                data.backedLighting_[index] = Color(result.x_, result.y_, result.z_, result.w_);
            }
        });
    }

    // Gauss post-pass
    //if (0)
    {
        IntVector2 offsets[9];
        for (int i = 0; i < 9; ++i)
            offsets[i] = IntVector2(i % 3 - 1, i / 3 - 1);
        float kernel[9];
        int kernel1D[3] = { 1, 2, 1 };
        for (int i = 0; i < 9; ++i)
            kernel[i] = kernel1D[i % 3] * kernel1D[i / 3] / 16.0f;

        auto colorCopy = data.backedLighting_;
        ParallelForEachRow([&](unsigned y)
        {
            for (unsigned x = 0; x < lightmapWidth; ++x)
            {
                const IntVector2 sourceTexel{ static_cast<int>(x), static_cast<int>(y) };
                const IntVector2 minOffset = -sourceTexel;
                const IntVector2 maxOffset = IntVector2{ lightmapWidth, lightmapHeight } - sourceTexel - IntVector2::ONE;

                const unsigned index = y * lightmapWidth + x;

                const unsigned geometryId = bakedGeometry.geometryIds_[index];
                if (!geometryId)
                    continue;

                float weightSum = 0.0f;
                Vector4 colorSum;
                for (unsigned i = 0; i < 9; ++i)
                {
                    const IntVector2 offset = offsets[i];
                    if (offset.x_ < minOffset.x_ || offset.x_ > maxOffset.x_ || offset.y_ < minOffset.y_ || offset.y_ > maxOffset.y_)
                        continue;

                    const unsigned otherIndex = index + offset.y_ * lightmapWidth + offset.x_;
                    const Vector4 otherColor = colorCopy[otherIndex].ToVector4();
                    const Vector3 otherPosition = bakedGeometry.geometryPositions_[otherIndex];
                    const unsigned otherGeometryId = bakedGeometry.geometryIds_[otherIndex];
                    if (!otherGeometryId)
                        continue;

                    colorSum += otherColor * kernel[i];
                    weightSum += kernel[i];
                }

                const Vector4 result = colorSum / weightSum;
                data.backedLighting_[index] = Color(result.x_, result.y_, result.z_, result.w_);
            }
        });
    }

    return true;
}

void LightmapBaker::ApplyLightmapsToScene(unsigned baseLightmapIndex)
{
    ApplyLightmapCharts(impl_->charts_, baseLightmapIndex);
}

}
