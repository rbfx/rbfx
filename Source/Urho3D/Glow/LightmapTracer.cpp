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

namespace
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

/// Generate random quaternion.
Quaternion RandomQuaternion()
{
    const float u1 = Random(1.0f);
    const float u2 = Random(360.0f);
    const float u3 = Random(360.0f);

    const float su2 = Sin(u2);
    const float cu2 = Cos(u2);
    const float su3 = Sin(u3);
    const float cu3 = Cos(u3);
    return { Sqrt(1 - u1) * su2, Sqrt(1 - u1) * cu2, Sqrt(u1) * su3, Sqrt(u1) * cu3 };
}

/// Generate random direction.
void RandomDirection(Vector3& result)
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
Vector3 RandomHemisphereDirection(const Vector3& normal)
{
    Vector3 result;
    RandomDirection(result);

    if (result.DotProduct(normal) < 0)
        result = -result;

    return result;
}

/// Get Gauss kernel of given radius.
ea::span<const float> GetKernel(int radius)
{
    static const float k0[] = { 1 };
    static const float k1[] = { 0.684538f, 0.157731f };
    static const float k2[] = { 0.38774f, 0.24477f, 0.06136f };
    static const float k3[] = { 0.266346f, 0.215007f, 0.113085f, 0.038735f };
    static const float k4[] = { 0.20236f, 0.179044f, 0.124009f, 0.067234f, 0.028532f };
    static const float k5[] = { 0.163053f, 0.150677f, 0.118904f, 0.080127f, 0.046108f, 0.022657f };

    switch (radius)
    {
    case 0: return k0;
    case 1: return k1;
    case 2: return k2;
    case 3: return k3;
    case 4: return k4;
    case 5: return k5;
    default:
        assert(0);
        return {};
    }
}

/// Get luminance of given color value.
float GetLuminance(const Vector4& color)
{
    return Color{ color.x_, color.y_, color.z_ }.Luma();
}

/// Calculate edge-stopping weight.
float CalculateEdgeWeight(
    float luminance1, float luminance2, float luminanceSigma,
    const Vector3& position1, const Vector3& position2, float positionSigma,
    const Vector3& normal1, const Vector3& normal2, float normalPower)
{
    const float colorWeight = Abs(luminance1 - luminance2) / luminanceSigma;
    const float positionWeight = positionSigma > M_EPSILON ? (position1 - position2).LengthSquared() / positionSigma : 0.0f;
    const float normalWeight = Pow(ea::max(0.0f, normal1.DotProduct(normal2)), normalPower);

    return std::exp(0.0f - colorWeight - positionWeight) * normalWeight;
}

/// Indirect light tracing for charts: tracing element.
struct ChartIndirectTracingElement
{
    /// Position.
    Vector3 position_;
    /// Normal.
    Vector3 normal_;
    /// Geometry ID.
    unsigned geometryId_;

    /// Indirect light value.
    Vector4 indirectLight_;

    /// Returns whether element is valid.
    explicit operator bool() const { return geometryId_ != 0; }
    /// Begin sample. Return position, normal and initial ray direction.
    void BeginSample(unsigned /*sampleIndex*/, Vector3& position, Vector3& normal, Vector3& rayDirection) const
    {
        position = position_;
        normal = normal_;
        rayDirection = RandomHemisphereDirection(normal_);
    }
    /// End sample.
    void EndSample(const Vector3& light)
    {
        indirectLight_ += Vector4(light, 1.0f);
    }
};

/// Indirect light tracing for charts: tracing kernel.
struct ChartIndirectTracingKernel
{
    /// Indirect light chart.
    LightmapChartBakedIndirect* bakedIndirect_{};
    /// Geometry buffer.
    const LightmapChartGeometryBuffer* geometryBuffer_{};
    /// Settings.
    const LightmapTracingSettings* settings_{};

    /// Return number of elements to trace.
    unsigned GetNumElements() const { return bakedIndirect_->light_.size(); }
    /// Return number of samples.
    unsigned GetNumSamples() const { return settings_->numIndirectChartSamples_; }
    /// Begin tracing element.
    ChartIndirectTracingElement BeginElement(unsigned elementIndex) const
    {
        const Vector3& position = geometryBuffer_->geometryPositions_[elementIndex];
        const unsigned& geometryId = geometryBuffer_->geometryIds_[elementIndex];
        const Vector3& normal = geometryBuffer_->smoothNormals_[elementIndex];
        return { position + normal * settings_->rayPositionOffset_, normal, geometryId };
    };
    /// Begin tracing value.
    void EndElement(unsigned elementIndex, const ChartIndirectTracingElement& element)
    {
        bakedIndirect_->light_[elementIndex] += element.indirectLight_;
    }
};

/// Light probe indirect tracing: tracing element.
struct LightProbeIndirectTracingElement
{
    /// Position.
    Vector3 position_;

    /// Current direction.
    Vector3 currentDirection_;
    /// Indirect light SH.
    SphericalHarmonicsColor9 sh_;
    /// Indirect light average value.
    Vector3 average_;
    /// Weight.
    float weight_{};

    /// Returns whether element is valid.
    explicit operator bool() const { return true; }
    /// Begin sample. Return position, normal and initial ray direction.
    void BeginSample(unsigned /*sampleIndex*/, Vector3& position, Vector3& normal, Vector3& rayDirection)
    {
        position = position_;
        RandomDirection(currentDirection_);
        normal = currentDirection_;
        rayDirection = currentDirection_;
    }
    /// End sample.
    void EndSample(const Vector3& light)
    {
        sh_ += SphericalHarmonicsColor9(currentDirection_, light);
        average_ += light;
        weight_ += 1.0f;
    }
};

/// Light probe indirect tracing: tracing kernel.
struct LightProbeIndirectTracingKernel
{
    /// Light probes collection.
    LightProbeCollection* collection_{};
    /// Settings.
    const LightmapTracingSettings* settings_{};

    /// Return number of elements to trace.
    unsigned GetNumElements() const { return collection_->Size(); }
    /// Return number of samples.
    unsigned GetNumSamples() const { return settings_->numIndirectProbeSamples_; }
    /// Begin tracing element.
    LightProbeIndirectTracingElement BeginElement(unsigned elementIndex) const
    {
        const Vector3& position = collection_->worldPositions_[elementIndex];
        return { position };
    };
    /// Begin tracing value.
    void EndElement(unsigned elementIndex, const LightProbeIndirectTracingElement& element)
    {
        const SphericalHarmonicsDot9 sh{ element.sh_ * (M_PI / element.weight_) };
        collection_->lightProbes_[elementIndex].bakedLight_ += sh;

        Vector3 av1 = element.average_ / element.weight_;
        Vector3 av2 = element.sh_.EvaluateAverage();
        av2 = av1;
    }
};

/// Trace indirect lighting.
template <class T>
void TraceIndirectLight(T& kernel, const ea::vector<const LightmapChartBakedDirect*>& bakedDirect,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    assert(settings.numBounces_ <= LightmapTracingSettings::MaxBounces);

    ParallelFor(kernel.GetNumElements(), settings.numThreads_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCScene scene = embreeScene.GetEmbreeScene();
        const float maxDistance = embreeScene.GetMaxDistance();
        const auto& geometryIndex = embreeScene.GetEmbreeGeometryIndex();

        Vector3 incomingSamples[LightmapTracingSettings::MaxBounces];
        float incomingFactors[LightmapTracingSettings::MaxBounces];

        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;

        for (unsigned elementIndex = fromIndex; elementIndex < toIndex; ++elementIndex)
        {
            auto element = kernel.BeginElement(elementIndex);
            if (!element)
                continue;

            for (unsigned sampleIndex = 0; sampleIndex < settings.numIndirectChartSamples_; ++sampleIndex)
            {
                Vector3 currentPosition;
                Vector3 currentNormal;
                Vector3 currentRayDirection;
                element.BeginSample(sampleIndex, currentPosition, currentNormal, currentRayDirection);

                int numBounces = 0;
                for (unsigned bounceIndex = 0; bounceIndex < settings.numBounces_; ++bounceIndex)
                {
                    rayHit.ray.org_x = currentPosition.x_;
                    rayHit.ray.org_y = currentPosition.y_;
                    rayHit.ray.org_z = currentPosition.z_;
                    rayHit.ray.dir_x = currentRayDirection.x_;
                    rayHit.ray.dir_y = currentRayDirection.y_;
                    rayHit.ray.dir_z = currentRayDirection.z_;
                    rayHit.ray.tfar = maxDistance;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(scene, &rayContext, &rayHit);

                    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                        break;

                    // Check normal orientation
                    if (currentRayDirection.DotProduct({ rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z }) > 0.0f)
                        break;

                    // Sample lightmap UV
                    const EmbreeGeometry& geometry = geometryIndex[rayHit.hit.geomID];
                    Vector2 lightmapUV;
                    rtcInterpolate0(geometry.embreeGeometry_, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &lightmapUV.x_, 2);

                    // Modify incoming flux
                    const float probability = 1 / (2 * M_PI);
                    const float cosTheta = currentRayDirection.DotProduct(currentNormal);
                    const float reflectance = 1 / M_PI;
                    const float brdf = reflectance / M_PI;

                    // TODO: Use real index here
                    const unsigned lightmapIndex = geometry.lightmapIndex_;
                    const IntVector2 sampleLocation = bakedDirect[lightmapIndex]->GetNearestLocation(lightmapUV);
                    incomingSamples[bounceIndex] = bakedDirect[lightmapIndex]->GetSurfaceLight(sampleLocation);
                    incomingFactors[bounceIndex] = brdf * cosTheta / probability;
                    ++numBounces;

                    // Go to next hemisphere
                    if (numBounces < settings.numBounces_)
                    {
                        // Move to hit position
                        currentPosition.x_ = rayHit.ray.org_x + rayHit.ray.dir_x * rayHit.ray.tfar;
                        currentPosition.y_ = rayHit.ray.org_y + rayHit.ray.dir_y * rayHit.ray.tfar;
                        currentPosition.z_ = rayHit.ray.org_z + rayHit.ray.dir_z * rayHit.ray.tfar;

                        // Offset position a bit
                        const Vector3 hitNormal = Vector3(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z).Normalized();
                        currentPosition.x_ += hitNormal.x_ * settings.rayPositionOffset_;
                        currentPosition.y_ += hitNormal.y_ * settings.rayPositionOffset_;
                        currentPosition.z_ += hitNormal.z_ * settings.rayPositionOffset_;

                        currentNormal = hitNormal;
                        currentRayDirection = RandomHemisphereDirection(currentNormal);
                    }
                }

                // Accumulate samples back-to-front
                Vector3 sampleIndirectLight;
                for (int bounceIndex = numBounces - 1; bounceIndex >= 0; --bounceIndex)
                {
                    sampleIndirectLight += incomingSamples[bounceIndex];
                    sampleIndirectLight *= incomingFactors[bounceIndex];
                }

                element.EndSample(sampleIndirectLight);
            }
            kernel.EndElement(elementIndex, element);
        }
    });
}

}

ea::vector<LightmapChartBakedDirect> InitializeLightmapChartsBakedDirect(
    const LightmapChartGeometryBufferVector& geometryBuffers)
{
    ea::vector<LightmapChartBakedDirect> chartsBakedDirect;
    for (const LightmapChartGeometryBuffer& geometryBuffer : geometryBuffers)
        chartsBakedDirect.emplace_back(geometryBuffer.width_, geometryBuffer.height_);
    return chartsBakedDirect;
}

ea::vector<LightmapChartBakedIndirect> InitializeLightmapChartsBakedIndirect(
    const LightmapChartGeometryBufferVector& geometryBuffers)
{
    ea::vector<LightmapChartBakedIndirect> chartsBakedIndirect;
    for (const LightmapChartGeometryBuffer& geometryBuffer : geometryBuffers)
        chartsBakedIndirect.emplace_back(geometryBuffer.width_, geometryBuffer.height_);
    return chartsBakedIndirect;
}

void BakeDirectionalLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const DirectionalLightParameters& light, const LightmapTracingSettings& settings)
{
    const Vector3 rayDirection = -light.direction_.Normalized();
    const float maxDistance = embreeScene.GetMaxDistance();
    const Vector3 lightColor = light.color_.ToVector3();
    RTCScene scene = embreeScene.GetEmbreeScene();

    ParallelFor(bakedDirect.directLight_.size(), settings.numThreads_,
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
            const Vector3 position = geometryBuffer.geometryPositions_[i];
            const Vector3 smoothNormal = geometryBuffer.smoothNormals_[i];
            const unsigned geometryId = geometryBuffer.geometryIds_[i];

            if (!geometryId)
                continue;

            // Cast direct ray
            rayHit.ray.org_x = position.x_ + rayDirection.x_ * settings.rayPositionOffset_;
            rayHit.ray.org_y = position.y_ + rayDirection.y_ * settings.rayPositionOffset_;
            rayHit.ray.org_z = position.z_ + rayDirection.z_ * settings.rayPositionOffset_;
            rayHit.ray.tfar = maxDistance;
            rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rtcIntersect1(scene, &rayContext, &rayHit);

            if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
                continue;

            const float intensity = ea::max(0.0f, smoothNormal.DotProduct(rayDirection));
            const Vector3 lightValue = lightColor * intensity;

            if (light.bakeDirect_)
                bakedDirect.directLight_[i] += Vector4(lightValue, 0.0f);

            if (light.bakeIndirect_)
                bakedDirect.surfaceLight_[i] += lightValue;
        }
    });
}

void BakeIndirectLightForCharts(LightmapChartBakedIndirect& bakedIndirect,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    ChartIndirectTracingKernel kernel{ &bakedIndirect, &geometryBuffer, &settings };
    TraceIndirectLight(kernel, bakedDirect, embreeScene, settings);
}

void BakeIndirectLightForLightProbes(LightProbeCollection& collection,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    LightProbeIndirectTracingKernel kernel{ &collection };
    TraceIndirectLight(kernel, bakedDirect, embreeScene, settings);
}

void FilterIndirectLight(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const IndirectFilterParameters& params, unsigned numThreads)
{
    const ea::span<const float> kernelWeights = GetKernel(params.kernelRadius_);
    ParallelFor(bakedIndirect.light_.size(), numThreads,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        for (unsigned index = fromIndex; index < toIndex; ++index)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[index];
            if (!geometryId)
                continue;

            const IntVector2 centerLocation = geometryBuffer.IndexToLocation(index);

            const Vector4 centerColor = bakedIndirect.light_[index];
            const float centerLuminance = GetLuminance(centerColor);
            const Vector3 centerPosition = geometryBuffer.geometryPositions_[index];
            const Vector3 centerNormal = geometryBuffer.smoothNormals_[index];

            float colorWeight = kernelWeights[0] * kernelWeights[0];
            Vector4 colorSum = centerColor * colorWeight;
            for (int dy = -params.kernelRadius_; dy <= params.kernelRadius_; ++dy)
            {
                for (int dx = -params.kernelRadius_; dx <= params.kernelRadius_; ++dx)
                {
                    if (dx == 0 && dy == 0)
                        continue;

                    const IntVector2 offset = IntVector2{ dx, dy } * params.upscale_;
                    const IntVector2 otherLocation = centerLocation + offset;
                    if (!geometryBuffer.IsValidLocation(otherLocation))
                        continue;

                    const float dxdy = Vector2{ static_cast<float>(dx), static_cast<float>(dy) }.Length();
                    const float kernel = kernelWeights[Abs(dx)] * kernelWeights[Abs(dy)];

                    const unsigned otherIndex = geometryBuffer.LocationToIndex(otherLocation);
                    const unsigned otherGeometryId = geometryBuffer.geometryIds_[otherIndex];
                    if (!otherGeometryId)
                        continue;

                    const Vector4 otherColor = bakedIndirect.light_[otherIndex];
                    const float weight = CalculateEdgeWeight(centerLuminance, GetLuminance(otherColor), params.luminanceSigma_,
                        centerPosition, geometryBuffer.geometryPositions_[otherIndex], dxdy * params.positionSigma_,
                        centerNormal, geometryBuffer.smoothNormals_[otherIndex], params.normalPower_);

                    colorSum += otherColor * weight * kernel;
                    colorWeight += weight * kernel;
                }
            }

            bakedIndirect.lightSwap_[index] = colorSum / ea::max(M_EPSILON, colorWeight);
        }
    });

    // Swap buffers
    ea::swap(bakedIndirect.light_, bakedIndirect.lightSwap_);
}

}
