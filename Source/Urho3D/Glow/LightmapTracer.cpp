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
#include "../Glow/LightmapTracer.h"

#include <future>

namespace Urho3D
{

/// Parallel for loop.
template <class T>
void ParallelFor(unsigned count, unsigned numThreads, const T& callback)
{
    // Post async tasks
    ea::vector<std::future<void>> tasks;
    const unsigned chunkSize = (count + numThreads - 1) / numThreads;
    for (unsigned i = 0; i < numThreads; ++i)
    {
        tasks.push_back(std::async([&, i]()
        {
            const unsigned fromIndex = i * chunkSize;
            const unsigned toIndex = ea::min(fromIndex + chunkSize, count);
            callback(fromIndex, toIndex);
        }));
    }

    // Wait for async tasks
    for (auto& task : tasks)
        task.wait();
}

/// Generate random direction.
static void RandomDirection(Vector3& result)
{
    float len;

    do
    {
        result.x_ = Random(-1.0f, 1.0f);
        result.y_ = Random(-1.0f, 1.0f);
        result.z_ = Random(-1.0f, 1.0f);
        len = result.Length();

    } while (len > 1.0f);

    result /= len;
}

/// Generate random hemisphere direction.
static void RandomHemisphereDirection(Vector3& result, const Vector3& normal)
{
    RandomDirection(result);

    if (result.DotProduct(normal) < 0)
        result = -result;
}

/// Fill 2D Gauss kernel.
template <int N>
void FillGaussKernel2D(IntVector2 (&offsets)[N*N], float (&weights)[N*N], const int (&kernel1D)[N], float denominator)
{
    for (int x = 0; x < N; ++x)
    {
        for (int y = 0; y < N; ++y)
        {
            const int i = x + y * N;
            offsets[i] = { x - N / 2, y - N / 2 };
            weights[i] = kernel1D[x] * kernel1D[y] / denominator;
        }
    }
}

/// Filter indirect light with NxN kernel.
template <int N>
void FilterIndirectLightNxN(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartBakedGeometry& bakedGeometry,
    const IndirectFilterParameters& params, unsigned numThreads, const int (&kernel1D)[N], float denominator)
{
    IntVector2 offsets[N * N];
    float weights[N * N];
    FillGaussKernel2D(offsets, weights, kernel1D, denominator);

    IndirectFilterParameters paramsCopy = params;
    paramsCopy.kernelSize_ = N * N;
    paramsCopy.offsets_ = offsets;
    paramsCopy.weights_ = weights;

    FilterIndirectLight(bakedIndirect, bakedGeometry, paramsCopy, numThreads);
}

ea::vector<LightmapChartBakedDirect> InitializeLightmapChartsBakedDirect(const LightmapChartVector& charts)
{
    ea::vector<LightmapChartBakedDirect> chartsBakedDirect;
    for (const LightmapChart& chart : charts)
        chartsBakedDirect.emplace_back(chart.width_, chart.height_);
    return chartsBakedDirect;
}

ea::vector<LightmapChartBakedIndirect> InitializeLightmapChartsBakedIndirect(const LightmapChartVector& charts)
{
    ea::vector<LightmapChartBakedIndirect> chartsBakedIndirect;
    for (const LightmapChart& chart : charts)
        chartsBakedIndirect.emplace_back(chart.width_, chart.height_);
    return chartsBakedIndirect;
}

void BakeDirectionalLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartBakedGeometry& bakedGeometry,
    const EmbreeScene& embreeScene, const DirectionalLightParameters& light, const LightmapTracingSettings& settings)
{
    const Vector3 rayDirection = -light.direction_.Normalized();
    const float maxDistance = embreeScene.GetMaxDistance();
    const Vector3 lightColor = light.color_.ToVector3();
    RTCScene scene = embreeScene.GetEmbreeScene();

    ParallelFor(bakedDirect.light_.size(), settings.numThreads_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        rayHit.ray.dir_x = rayDirection.x_;
        rayHit.ray.dir_y = rayDirection.y_;
        rayHit.ray.dir_z = rayDirection.z_;
        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const Vector3 position = bakedGeometry.geometryPositions_[i];
            const Vector3 smoothNormal = bakedGeometry.smoothNormals_[i];
            const unsigned geometryId = bakedGeometry.geometryIds_[i];

            if (!geometryId)
                continue;

            // Cast direct ray
            rayHit.ray.org_x = position.x_ + rayDirection.x_ * 0.001f;
            rayHit.ray.org_y = position.y_ + rayDirection.y_ * 0.001f;
            rayHit.ray.org_z = position.z_ + rayDirection.z_ * 0.001f;
            rayHit.ray.tfar = maxDistance;
            rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rtcIntersect1(scene, &rayContext, &rayHit);

            const float shadowFactor = rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID ? 1.0f : 0.0f;
            const float directLight = ea::max(0.0f, smoothNormal.DotProduct(rayDirection));

            bakedDirect.light_[i] += lightColor * shadowFactor * directLight;
        }
    });
}

void BakeIndirectLight(LightmapChartBakedIndirect& bakedIndirect,
    const ea::vector<LightmapChartBakedDirect>& bakedDirect, const LightmapChartBakedGeometry& bakedGeometry,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    ParallelFor(bakedIndirect.light_.size(), settings.numThreads_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCScene scene = embreeScene.GetEmbreeScene();
        const float maxDistance = embreeScene.GetMaxDistance();
        const auto& geometryIndex = embreeScene.GetEmbreeGeometryIndex();

        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const Vector3 position = bakedGeometry.geometryPositions_[i];
            const Vector3 smoothNormal = bakedGeometry.smoothNormals_[i];
            const unsigned geometryId = bakedGeometry.geometryIds_[i];

            if (!geometryId)
                continue;

            float indirectLight = 0.0f;
            Vector3 currentPosition = position;
            Vector3 currentNormal = smoothNormal;
            for (unsigned j = 0; j < settings.numBounces_; ++j)
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
                rayHit.ray.tfar = maxDistance;
                rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIntersect1(scene, &rayContext, &rayHit);

                if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    continue;

                // Sample lightmap UV
                RTCGeometry geometry = geometryIndex[rayHit.hit.geomID].embreeGeometry_;
                Vector2 lightmapUV;
                rtcInterpolate0(geometry, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &lightmapUV.x_, 2);
                const Vector3 bakedDirectLight = bakedDirect[0].SampleNearest(lightmapUV);

                const float incoming = bakedDirectLight.x_ * 1.0f;
                const float probability = 1 / (2 * M_PI);
                const float cosTheta = rayDirection.DotProduct(currentNormal);
                const float reflectance = 1 / M_PI;
                const float brdf = reflectance / M_PI;

                indirectLight = incoming * brdf * cosTheta / probability;
            }

            bakedIndirect.light_[i] += Vector4(indirectLight, indirectLight, indirectLight, 1);
        }
    });
}

void FilterIndirectLight(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartBakedGeometry& bakedGeometry,
    const IndirectFilterParameters& params, unsigned numThreads)
{
    assert(params.offsets_.size() == params.kernelSize_);
    assert(params.weights_.size() == params.kernelSize_);

    ParallelFor(bakedIndirect.light_.size(), numThreads,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        const IntVector2 sizeMinusOne{ static_cast<int>(bakedIndirect.width_ - 1), static_cast<int>(bakedIndirect.height_ - 1) };
        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const unsigned geometryId = bakedGeometry.geometryIds_[i];
            if (!geometryId)
                continue;

            const IntVector2 baseIndex{ static_cast<int>(i % bakedIndirect.width_), static_cast<int>(i / bakedIndirect.width_) };
            const IntVector2 minOffset = -baseIndex;
            const IntVector2 maxOffset = sizeMinusOne - baseIndex;

            const Vector4 baseColor = bakedIndirect.light_[i];
            const Vector3 basePosition = bakedGeometry.geometryPositions_[i];
            const Vector3 baseNormal = bakedGeometry.smoothNormals_[i];

            Vector4 colorSum;
            float weightSum = 0.0f;
            for (unsigned k = 0; k < params.kernelSize_; ++k)
            {
                const IntVector2 offset = params.offsets_[k] * params.upscale_;
                const IntVector2 clampedOffset = VectorMax(minOffset, VectorMin(offset, maxOffset));
                const unsigned j = i + clampedOffset.y_ * bakedIndirect.width_ + clampedOffset.x_;

                const unsigned otherGeometryId = bakedGeometry.geometryIds_[j];
                if (!otherGeometryId)
                    continue;

                const Vector4 otherColor = bakedIndirect.light_[j];
                const Vector4 colorDelta = baseColor - otherColor;
                const float colorDeltaSquared = colorDelta.DotProduct(colorDelta);
                const float colorWeight = ea::min(std::exp(-colorDeltaSquared / params.colorWeight_), 1.0f);

                const Vector3 otherPosition = bakedGeometry.geometryPositions_[j];
                const Vector3 positionDelta = basePosition - otherPosition;
                const float positionDeltaSquared = positionDelta.DotProduct(positionDelta);
                const float positionWeight = ea::min(std::exp(-positionDeltaSquared / params.positionWeight_), 1.0f);

                const Vector3 otherNormal = bakedGeometry.smoothNormals_[j];
                const float normalDeltaCos = ea::max(0.0f, baseNormal.DotProduct(otherNormal));
                const float normalWeight = std::pow(normalDeltaCos, params.normalWeight_);

                const float weight = colorWeight * positionWeight * normalWeight * params.weights_[k];
                colorSum += otherColor * weight;
                weightSum += weight;
            }

            if (weightSum > 0.0f)
                bakedIndirect.lightSwap_[i] = colorSum / weightSum;
            else
                bakedIndirect.lightSwap_[i] = Vector4::ZERO;
        }
    });

    // Swap buffers
    ea::swap(bakedIndirect.light_, bakedIndirect.lightSwap_);
}

void FilterIndirectLight3x3(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartBakedGeometry& bakedGeometry,
    const IndirectFilterParameters& params, unsigned numThreads)
{
    const int kernel1D[] = { 1, 2, 1 };
    FilterIndirectLightNxN(bakedIndirect, bakedGeometry, params, numThreads, kernel1D, 16.0f);
}

void FilterIndirectLight5x5(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartBakedGeometry& bakedGeometry,
    const IndirectFilterParameters& params, unsigned numThreads)
{
    const int kernel1D[] = { 1, 4, 6, 4, 1 };
    FilterIndirectLightNxN(bakedIndirect, bakedGeometry, params, numThreads, kernel1D, 256.0f);
}

}
