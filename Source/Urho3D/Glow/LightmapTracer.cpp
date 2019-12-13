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

}
